/*****************************************************************************************
 *  ▷▷  Object_Detection 전체 파이프라인 한눈에 보기  ◁◁
 *
 *                             +----------------------------+
 *       topics in  ─────────► | detectionCallback()        |
 *  (LiDAR / Image / YOLO)     |   ① sensor_msgs sync       | //카메라 30hz -> 10hz
 *                             +-------------┬--------------+
 *                                           │
 *                                           ▼
 *                             +----------------------------+
 *                             | filter_pointcloud()        |   // *선택적 사전 필터*
 *                             |   ├─ dror_filter()         |   // ① 밀도 기반 outlier
 *                             |   └─ remove_ground_ransac()|   // ② 지면 평면 제거
 *                             +-------------┬--------------+
 *                                           │ (lidar_points / projected_list 축소)
 *                                           ▼
 *                             +----------------------------+
 *                             | convert_msg()              |
 *                             |   ① bbox ↔ LiDAR 점 매칭   |
 *                             |   ② ROI(원) + Euclidean    |
 *                             |      클러스터링            |
 *                             |   ③ centroid 계산          |
 *                             +-------------┬--------------+
 *                                           │
 *                                           ▼
 *                             +----------------------------+
 *                             | track_and_visualize()      |
 *                             |   └─ match_and_update_…    |
 *                             |         └─ KalmanTracker   |
 *                             |             • predict()    |
 *                             |             • correct()    |
 *                             +-------------┬--------------+
 *         2-D pixel centroid                │                  Kalman 예측
 *       (관측 or 예측)                      └────────┐
 *                                                    │
 *                                                    ▼
 *                             +----------------------------+
 *                             | publish_2D_pointcloud()    |  // /cloud_centroid 토픽
 *                             +----------------------------+
 *
 *  ※  흐름 정리
 *      1. `detectionCallback()` 에서 3-sensor 메시지 동기화 후 진입
 *      2. LiDAR 포인트를 **DROR + RANSAC** 으로 깨끗이 → 이미지 투영
 *      3. YOLO bbox 안의 점을 클러스터링 → **centroid 추출**
 *      4. **KalmanTracker** 로 ID 유지 & 보간 / 시각화
 *      5. centroid 결과를 `sensor_msgs::PointCloud` 로 **Publish**
 *
 *****************************************************************************************/


#include "detection.h"

#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/PointCloud.h>
#include <pcl_conversions/pcl_conversions.h>

/* ===== KalmanTracker 구현 ===== */
KalmanTracker::KalmanTracker(const cv::Point2f& pt,int tracker_id,float dt)
: id(tracker_id)
{
    kf.init(4,2,0);
    kf.transitionMatrix = (cv::Mat_<float>(4,4)<<1,0,dt,0, 0,1,0,dt, 0,0,1,0, 0,0,0,1);
    kf.measurementMatrix = cv::Mat::eye(2,4,CV_32F);
    setIdentity(kf.processNoiseCov,  cv::Scalar::all(5e-2));
    setIdentity(kf.measurementNoiseCov,cv::Scalar::all(1e-1));
    setIdentity(kf.errorCovPost,cv::Scalar::all(1));
    kf.statePost=(cv::Mat_<float>(4,1)<<pt.x,pt.y,0,0);
    last_pos = pt;
}
cv::Point2f KalmanTracker::predict(){
    cv::Mat pr = kf.predict();
    last_pos = { pr.at<float>(0), pr.at<float>(1) };
    return last_pos;
}
void KalmanTracker::update(const cv::Point2f& pt){
    cv::Mat m(2,1,CV_32F); m.at<float>(0)=pt.x; m.at<float>(1)=pt.y;
    kf.correct(m); last_pos = pt; miss_count = 0;
}
void KalmanTracker::miss(){ predict(); ++miss_count; }

/* ===== Kalman 컨테이너 ===== */
static std::map<int,KalmanTracker> trackers;
static int next_tracker_id = 0;

/* ===== Object_Detection ctor / dtor ===== */
Object_Detection::Object_Detection(ros::NodeHandle* nodeHandle)
: nh(*nodeHandle), counter(0), is_first_frame(true), alpha(0.3)
{
    cloud_centroid = nh.advertise<sensor_msgs::PointCloud>("/cloud_centroid",1);
    cloud_fillter_pub = nh.advertise<sensor_msgs::PointCloud2>("/cloud_fillter",  1);  // ★ 추가

    nh.param<std::string>("lidar_topic",  lidar_topic,  "/livox/lidar");
    nh.param<std::string>("camera_topic", camera_topic, "/camera/image_raw/compressed");
    nh.param<std::string>("yolo_topic",   yolo_topic,   "/yolov8_pub");
    nh.param<std::string>("frame_name",   frame_name,   "livox_frame");

    /* ▶ 하이퍼파라미터에서 초기화 */
    cluster_tolerance = CLUSTER_TOLERANCE;
    cluster_min       = CLUSTER_MIN_SIZE;
    cluster_max       = CLUSTER_MAX_SIZE;

   /* ── 메시지 필터 동기화: slop = 0.05 s (±50 ms) ── */
    message_filters::Subscriber<sensor_msgs::PointCloud2>     subL(nh, lidar_topic , 10);
    message_filters::Subscriber<sensor_msgs::CompressedImage> subC(nh, camera_topic, 10);
    message_filters::Subscriber<detect_msgs::Yolo_Objects>    subY(nh, yolo_topic  , 10);

    using SyncPolicy = message_filters::sync_policies::ApproximateTime<
                         sensor_msgs::PointCloud2,
                         sensor_msgs::CompressedImage,
                         detect_msgs::Yolo_Objects>;

    const int QUEUE_SIZE = 20;                    // 버퍼 길이
    auto syncer = std::make_shared< message_filters::Synchronizer<SyncPolicy> >(
                      SyncPolicy(QUEUE_SIZE),     // ★ 생성자엔 queue_size 만
                      subL, subC, subY);

    /* 슬롭(허용 간격) 설정 */
    syncer->setMaxIntervalDuration( ros::Duration(0.05) );   // ±50 ms

    /* 콜백 등록 */
    syncer->registerCallback(
        boost::bind(&Object_Detection::detectionCallback, this, _1, _2, _3));

    read_projection_matrix();
    ROS_INFO("start detection");
    ros::spin();
}
Object_Detection::~Object_Detection(){ ROS_INFO("finish detection"); }

/* ===== detectionCallback ===== */
void Object_Detection::detectionCallback(const sensor_msgs::PointCloud2::ConstPtr& lidar_msg,
                                         const sensor_msgs::CompressedImage::ConstPtr& cam_msg,
                                         const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg)
{       
    camera_image = cv_bridge::toCvCopy(cam_msg,"bgr8")->image;

    pcl::PointCloud<pcl::PointXYZI>::Ptr pc(new pcl::PointCloud<pcl::PointXYZI>());
    pcl::fromROSMsg(*lidar_msg,*pc);

    lidar_points.clear(); distance_list.clear();
    for(const auto& p:pc->points){
        lidar_points.emplace_back(p.x,p.y,p.z);
        distance_list.push_back(std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z));
    }
    if(!lidar_points.empty()){
        cv::perspectiveTransform(lidar_points,projected_list,projection_matrix);
        convert_msg(yolo_msg,lidar_msg->header);
    }
}

/* ===== convert_msg (YOLO + 클러스터 & 필터 결과 Publish) ===== */
void Object_Detection::convert_msg(
        const detect_msgs::Yolo_Objects::ConstPtr& yolo,
        const std_msgs::Header&                    header)
{
    std::vector<cv::Point2d>             cur_centroids;
    pcl::PointCloud<pcl::PointXYZ>::Ptr out_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    out_cloud->reserve(1024);                // 대략 – 필요에 따라 조정

    for (const auto& Y : yolo->yolo_objects)
    {
        /* ① bbox 축소 ------------------------------------------------ */
        double x1 = Y.x1, y1 = Y.y1, x2 = Y.x2, y2 = Y.y2;

        const double cx = 0.5 * (x1 + x2);
        const double cy = 0.5 * (y1 + y2);
        const double hw = 0.5 * (x2 - x1) * BBOX_SCALE_RATIO;
        const double hh = 0.5 * (y2 - y1) * BBOX_SCALE_RATIO;

        x1 = std::max(0.0,                         cx - hw);
        y1 = std::max(0.0,                         cy - hh);
        x2 = std::min<double>(camera_image.cols-1, cx + hw);
        y2 = std::min<double>(camera_image.rows-1, cy + hh);

        if ((x2 - x1) < MIN_BBOX_EDGE_PX || (y2 - y1) < MIN_BBOX_EDGE_PX)
            continue;

        /* ② bbox 내 LiDAR 점 추출 ----------------------------------- */
        std::vector<cv::Point2d> matched_px;
        pcl::PointCloud<pcl::PointXYZ>::Ptr local(new pcl::PointCloud<pcl::PointXYZ>);

        for (size_t i = 0; i < projected_list.size(); ++i) {
            double u = projected_list[i].x, v = projected_list[i].y;
            if (std::isnan(u) || std::isnan(v)) continue;
            if (u >= x1 && u <= x2 && v >= y1 && v <= y2) {
                matched_px.emplace_back(u, v);
                local->points.emplace_back(lidar_points[i].x,
                                            lidar_points[i].y,
                                            lidar_points[i].z);
            }
        }
        if (matched_px.size() < static_cast<size_t>(CLUSTER_MIN_SIZE)) continue;

        /* ③ ROI(원형) 필터 ─ bbox 중심 기준 -------------------------- */
        const cv::Point2d c2d(cx, cy);   // ★ centroid 대신 bbox 중심 사용
            
        pcl::PointCloud<pcl::PointXYZ>::Ptr roi(new pcl::PointCloud<pcl::PointXYZ>);
        for (size_t i = 0; i < matched_px.size(); ++i)
            if (cv::norm(matched_px[i] - c2d) <= ROI_RADIUS_PX)
                roi->points.push_back(local->points[i]);
            
        if (roi->points.size() < static_cast<size_t>(CLUSTER_MIN_SIZE))
            continue;

        /* ④ 지면 제거(RANSAC) -------------------------------------- */
        {
            std::vector<cv::Point3f> tmp;  tmp.reserve(roi->points.size());
            for (const auto& p : *roi) tmp.emplace_back(p.x, p.y, p.z);

            const std::vector<int> keep = remove_ground_ransac(tmp, GROUND_THRESH);
            if (keep.empty()) continue;

            pcl::PointCloud<pcl::PointXYZ>::Ptr ng(new pcl::PointCloud<pcl::PointXYZ>);
            ng->points.reserve(keep.size());
            for (int idx : keep) ng->points.push_back(roi->points[idx]);
            roi = ng;                                   // ground-free
        }

        /* ⑤ Euclidean 클러스터링 ----------------------------------- */
        pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
        std::vector<pcl::PointIndices> clusters;

        pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
        ec.setClusterTolerance(CLUSTER_TOLERANCE);
        ec.setMinClusterSize(CLUSTER_MIN_SIZE);
        ec.setMaxClusterSize(CLUSTER_MAX_SIZE);
        ec.setSearchMethod(tree);
        ec.setInputCloud(roi);
        ec.extract(clusters);
        if (clusters.empty()) continue;

        const auto& largest = *std::max_element(
            clusters.begin(), clusters.end(),
            [](const auto& a, const auto& b){ return a.indices.size() < b.indices.size(); });

        /* ⑥ centroid 계산 ----------------------------------------- */
        double sx = 0.0, sy = 0.0;
        for (int idx : largest.indices) {
            sx += roi->points[idx].x;
            sy += roi->points[idx].y;
        }
        cur_centroids.emplace_back(sx / largest.indices.size(),
                                   sy / largest.indices.size());

        /* ⑥-b) 2-D 시각화 (선택) ----------------------------------- */
        cv::rectangle(camera_image,
                      {static_cast<int>(x1), static_cast<int>(y1)},
                      {static_cast<int>(x2), static_cast<int>(y2)},
                      {0,255,0}, 2);
        cv::circle(camera_image, c2d, 4, {0,0,255}, 2);

        /* ⑦ ROI 점을 최종 cloud 로 누적 ---------------------------- */
        *out_cloud += *roi;
    }

    /* ⑧ centroid & tracker 처리 ------------------------------------ */
    prev_centroids = cur_centroids;
    track_and_visualize(prev_centroids);
    publish_2D_pointcloud(prev_centroids, header);

    /* ⑨ 필터된 3-D 점 Publish  (/cloud_fillter) -------------------- */
    if (!out_cloud->empty())
    {
        sensor_msgs::PointCloud2 msg_pc2;
        pcl::toROSMsg(*out_cloud, msg_pc2);
        msg_pc2.header = header;          // stamp, frame_id 유지
        cloud_fillter_pub.publish(msg_pc2);
    }
}

/* ===== track_and_visualize + 매칭 ===== */
void Object_Detection::match_and_update_trackers(const std::vector<cv::Point2f>& cents,
                                                 double match_dist,int max_miss)
{
    std::set<int> matched_t, matched_o;
    std::vector<cv::Point2f> preds; std::vector<int> keys;

    /* ❌ const 제거 */
    for(auto& kv : trackers){                       // ← 수정
        preds.push_back(kv.second.predict());
        keys.push_back(kv.first);
    }

    if(!cents.empty() && !preds.empty()){
        cv::Mat D((int)cents.size(),(int)preds.size(),CV_32F);
        for(int i=0;i<(int)cents.size();++i)
            for(int j=0;j<(int)preds.size();++j)
                D.at<float>(i,j)=cv::norm(cents[i]-preds[j]);
        while(true){
            double vmin; cv::Point loc;
            cv::minMaxLoc(D,&vmin,nullptr,&loc,nullptr);
            if(vmin>match_dist) break;
            int oi=loc.y, tj=loc.x;
            trackers[keys[tj]].update(cents[oi]);
            matched_t.insert(keys[tj]); matched_o.insert(oi);
            D.row(oi).setTo(1e9); D.col(tj).setTo(1e9);
        }
    }
    for(auto& kv:trackers) if(!matched_t.count(kv.first)) kv.second.miss();
    std::vector<int> del;
    for(const auto& kv:trackers) if(kv.second.miss_count>max_miss) del.push_back(kv.first);
    for(int k:del) trackers.erase(k);
    for(size_t i=0;i<cents.size();++i)
        if(!matched_o.count(i)) trackers[next_tracker_id]=KalmanTracker(cents[i],next_tracker_id++);
}
void Object_Detection::track_and_visualize(const std::vector<cv::Point2d>& cents)
{
    std::vector<cv::Point2f> c2f;
    for(const auto& c:cents) c2f.emplace_back((float)c.x,(float)c.y);

    match_and_update_trackers(c2f,MATCH_DIST,TRACKER_MAX_MISS);

    for(const auto& kv:trackers){
        cv::circle(camera_image,kv.second.last_pos,6,{255,0,255},2);
        cv::putText(camera_image,std::to_string(kv.first),
                    kv.second.last_pos+cv::Point2f(5,-5),
                    cv::FONT_HERSHEY_SIMPLEX,0.5,{255,255,0},1);
    }
}

/* ===== DROR / RANSAC ===== */
std::vector<int> Object_Detection::dror_filter(const std::vector<cv::Point3f>& pts)
{
    std::vector<int> keep;
    std::vector<double> rxy(pts.size());
    for(size_t i=0;i<pts.size();++i) rxy[i]=std::hypot(pts[i].x,pts[i].y);

    for(size_t i=0;i<pts.size();++i){
        /* ❌ std::clamp → 수동 min/max */
        double R = std::min(std::max(
                     DROR_MIN_RADIUS + DROR_RADIUS_SCALE*rxy[i],
                     DROR_MIN_RADIUS),
                     DROR_MAX_RADIUS);
        int cnt=0;
        for(size_t j=0;j<pts.size();++j){
            if(i==j) continue;
            if(cv::norm(cv::Point3f(pts[i].x-pts[j].x,
                                    pts[i].y-pts[j].y,
                                    pts[i].z-pts[j].z)) < R) ++cnt;
        }
        if(cnt>=DROR_MIN_NEIGHBORS) keep.push_back((int)i);
    }
    return keep;
}
std::vector<int> Object_Detection::remove_ground_ransac(const std::vector<cv::Point3f>& pts,
                                                        double th)
{
    if(pts.size()<10){ std::vector<int> id(pts.size()); std::iota(id.begin(),id.end(),0); return id; }
    int max_in=0; double a=0,b=0,c=0;
    std::default_random_engine gen;
    std::uniform_int_distribution<> d(0,pts.size()-1);

    for(int iter=0;iter<30;++iter){
        int i1=d(gen), i2=d(gen), i3=d(gen);
        double x1=pts[i1].x,y1=pts[i1].y,z1=pts[i1].z,
               x2=pts[i2].x,y2=pts[i2].y,z2=pts[i2].z,
               x3=pts[i3].x,y3=pts[i3].y,z3=pts[i3].z;
        double den = x1*(y2-y3)+x2*(y3-y1)+x3*(y1-y2);
        if(std::abs(den)<1e-6) continue;
        double ta = (z1*(y2-y3)+z2*(y3-y1)+z3*(y1-y2))/den;
        double tb = (x1*(z2-z3)+x2*(z3-z1)+x3*(z1-z2))/den;
        double tc = z1-ta*x1-tb*y1;

        int in=0;
        for(const auto& p:pts)
            if(std::abs(p.z-(ta*p.x+tb*p.y+tc))<th) ++in;

        if(in>max_in){ max_in=in; a=ta;b=tb;c=tc; }
    }
    std::vector<int> idx;
    for(size_t i=0;i<pts.size();++i)
        if(std::abs(pts[i].z-(a*pts[i].x+b*pts[i].y+c))>th) idx.push_back((int)i);
    return idx;
}

/* ===== publish & projection ===== */
void Object_Detection::publish_2D_pointcloud(const std::vector<cv::Point2d>& pts,
                                             const std_msgs::Header& hdr)
{
    sensor_msgs::PointCloud cloud;
    cloud.header = hdr; cloud.header.frame_id = frame_name;
    for(const auto& p:pts){
        geometry_msgs::Point32 q; q.x=p.x; q.y=p.y; q.z=0;
        cloud.points.push_back(q);
    }
    sensor_msgs::ChannelFloat32 ch; ch.name="dummy";
    ch.values.resize(cloud.points.size(),1.0f);
    cloud.channels.push_back(ch);
    cloud_centroid.publish(cloud);
}
void Object_Detection::read_projection_matrix(){
    double fx=1.7900e3, fy=1.7869e3, cx=960.4433, cy=595.1015;
    cv::Mat K=(cv::Mat_<double>(3,3)<<fx,0,cx, 0,fy,cy, 0,0,1);
    cv::Mat T=(cv::Mat_<double>(3,4)<<
        -0.0119,-0.9999,-0.0025, 0.0593,
        -0.0423, 0.0030,-0.9991,-0.0446,
         0.9990,-0.0118,-0.0423,-0.1091);
    projection_matrix = K*T;
}

/* 기존 ground_filter 그대로 (타이포 수정 없음) */
pcl::PointCloud<pcl::PointXYZ>::Ptr
Object_Detection::ground_filter(pcl::PointCloud<pcl::PointXYZ> cloud){
    double height_thresh=0.0;
    int grid_dim=320; double per_cell=0.2;
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>());
    static bool  init[320][320];
    static float min_h[320][320], max_h[320][320];
    std::memset(init,0,sizeof(init));

    for(auto& p:cloud.points){
        int xi=(int)(grid_dim/2 + p.x/per_cell);
        int yi=(int)(grid_dim/2 + p.y/per_cell);
        if(xi>=0 && xi<grid_dim && yi>=0 && yi<grid_dim){
            if(!init[xi][yi]){
                init[xi][yi]=true; min_h[xi][yi]=max_h[xi][yi]=p.z;
            }else{
                min_h[xi][yi]=std::min(min_h[xi][yi],p.z);
                max_h[xi][yi]=std::max(max_h[xi][yi],p.z);
            }
        }
    }
    double offset=grid_dim/2.0*per_cell;
    for(int xi=0;xi<grid_dim;++xi)
        for(int yi=0;yi<grid_dim;++yi)
            if(init[xi][yi] && (max_h[xi][yi]-min_h[xi][yi]>height_thresh)){
                pcl::PointXYZ q;
                q.x=-offset+(xi*per_cell+per_cell/2.0);
                q.y=-offset+(yi*per_cell+per_cell/2.0);
                q.z=-0.4; filtered->points.push_back(q);
            }
    return filtered;
}
