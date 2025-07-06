/************************************************
 *  detection.cpp  (구현부)
 ************************************************/

#include "detection.h"

#include <numeric>   // for std::iota

/* ===== KalmanTracker 구현 ===== */
KalmanTracker::KalmanTracker(const cv::Point2f& pt, int tracker_id, float dt)
    : id(tracker_id)
{
    kf.init(4, 2, 0);
    kf.transitionMatrix = (cv::Mat_<float>(4,4) <<
        1,0,dt,0,
        0,1,0,dt,
        0,0,1,0,
        0,0,0,1);
    kf.measurementMatrix = cv::Mat::eye(2,4,CV_32F);
    setIdentity(kf.processNoiseCov,     cv::Scalar::all(5e-2));
    setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
    setIdentity(kf.errorCovPost,        cv::Scalar::all(1));
    kf.statePost = (cv::Mat_<float>(4,1)<< pt.x, pt.y, 0, 0);
    last_pos = pt;
}
cv::Point2f KalmanTracker::predict()
{
    cv::Mat pr = kf.predict();
    last_pos = {pr.at<float>(0), pr.at<float>(1)};
    return last_pos;
}
void KalmanTracker::update(const cv::Point2f& pt)
{
    cv::Mat m(2,1,CV_32F); m.at<float>(0)=pt.x; m.at<float>(1)=pt.y;
    kf.correct(m);
    last_pos   = pt;
    miss_count = 0;
}
void KalmanTracker::miss() { predict(); ++miss_count; }

/* ===== Tracker 컨테이너 ===== */
static std::map<int,KalmanTracker> trackers;
static int next_tracker_id = 0;

/* ===== Object_Detection ctor/dtor ===== */
Object_Detection::Object_Detection(ros::NodeHandle* nh_) : nh(*nh_)
{
    cloud_centroid     = nh.advertise<sensor_msgs::PointCloud> ("/cloud_centroid" ,1);
    cloud_filter_pub   = nh.advertise<sensor_msgs::PointCloud2>("/cloud_filter"   ,1);
    cloud_roi_pub      = nh.advertise<sensor_msgs::PointCloud2>("/cloud_roi"      ,1);
    cloud_bbox_roi_pub = nh.advertise<sensor_msgs::PointCloud2>("/cloud_bbox_roi" ,1);

    nh.param<std::string>("lidar_topic",  lidar_topic , "/livox/lidar");
    nh.param<std::string>("camera_topic", camera_topic, "/camera/image_raw/compressed");
    nh.param<std::string>("yolo_topic",   yolo_topic  , "/yolov8_pub");
    nh.param<std::string>("frame_name",   frame_name  , "livox_frame");

    message_filters::Subscriber<sensor_msgs::PointCloud2>     subL(nh,lidar_topic ,10);
    message_filters::Subscriber<sensor_msgs::CompressedImage> subC(nh,camera_topic,10);
    message_filters::Subscriber<detect_msgs::Yolo_Objects>    subY(nh,yolo_topic  ,10);

    using Sync = message_filters::sync_policies::ApproximateTime<
                   sensor_msgs::PointCloud2,
                   sensor_msgs::CompressedImage,
                   detect_msgs::Yolo_Objects>;
    auto syncer = std::make_shared< message_filters::Synchronizer<Sync> >(Sync(20),
                                                                          subL,subC,subY);
    syncer->setMaxIntervalDuration(ros::Duration(0.05));
    syncer->registerCallback(boost::bind(&Object_Detection::detectionCallback,
                                         this,_1,_2,_3));

    read_projection_matrix();
    ROS_INFO("[Object_Detection] node started");
    ros::spin();
}
Object_Detection::~Object_Detection()
{
    ROS_INFO("[Object_Detection] node terminated");
}

/* ===== projection matrix ===== */
void Object_Detection::read_projection_matrix()
{
    const double fx=1.79e3, fy=1.7869e3, cx=960.4433, cy=595.1015;
    cv::Mat K = (cv::Mat_<double>(3,3)<< fx,0,cx, 0,fy,cy, 0,0,1);
    cv::Mat T = (cv::Mat_<double>(3,4)<<
        -0.0119,-0.9999,-0.0025, 0.0593,
        -0.0423, 0.0030,-0.9991,-0.0446,
         0.9990,-0.0118,-0.0423,-0.1091);
    projection_matrix = K * T;
}

/* ===== ROI + Voxel ===== */
void Object_Detection::filter_pointcloud(std::vector<cv::Point3d>& pts)
{
    /* ① XYZ ROI */
    std::vector<cv::Point3d> roi; roi.reserve(pts.size());
    for(const auto& p:pts)
        if(p.x>=ROI_X_MIN && p.x<=ROI_X_MAX &&
           p.y>=ROI_Y_MIN && p.y<=ROI_Y_MAX &&
           p.z>=ROI_Z_MIN && p.z<=ROI_Z_MAX)
            roi.push_back(p);
    pts.swap(roi);
    if(pts.empty()) return;

    /* ② VoxelGrid 다운샘플 */
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->reserve(pts.size());
    for(const auto& p:pts) cloud->push_back({(float)p.x,(float)p.y,(float)p.z});

    pcl::VoxelGrid<pcl::PointXYZ> vg; vg.setInputCloud(cloud);
    vg.setLeafSize(VOXEL_SIZE,VOXEL_SIZE,VOXEL_SIZE);
    pcl::PointCloud<pcl::PointXYZ>::Ptr ds(new pcl::PointCloud<pcl::PointXYZ>);
    vg.filter(*ds);

    pts.clear(); pts.reserve(ds->size());
    for(const auto& p:*ds) pts.emplace_back(p.x,p.y,p.z);

    
    std::vector<cv::Point3f> tmp; tmp.reserve(pts.size());
    for(auto& p:pts) tmp.emplace_back(p.x,p.y,p.z);
    auto keep = dror_filter(tmp);
    std::vector<cv::Point3d> clean; clean.reserve(keep.size());
    for(int id:keep) clean.push_back(pts[id]);
    pts.swap(clean);
}

/* ===== main callback ===== */
void Object_Detection::detectionCallback(
        const sensor_msgs::PointCloud2::ConstPtr& lidar_msg,
        const sensor_msgs::CompressedImage::ConstPtr& cam_msg,
        const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg)
{
    camera_image = cv_bridge::toCvCopy(cam_msg,"bgr8")->image;

    pcl::PointCloud<pcl::PointXYZI>::Ptr pc(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::fromROSMsg(*lidar_msg,*pc);
    lidar_points.clear(); lidar_points.reserve(pc->size());
    for(const auto& p:*pc) lidar_points.emplace_back(p.x,p.y,p.z);

    filter_pointcloud(lidar_points);
    if(lidar_points.empty()) return;

    /* /cloud_roi 디버그 */
    {
        pcl::PointCloud<pcl::PointXYZ> dbg; dbg.reserve(lidar_points.size());
        for(const auto& p:lidar_points) dbg.push_back({(float)p.x,(float)p.y,(float)p.z});
        sensor_msgs::PointCloud2 m; pcl::toROSMsg(dbg,m);
        m.header = lidar_msg->header;
        cloud_roi_pub.publish(m);
    }

    /* LiDAR → pixel 투영 */
    cv::perspectiveTransform(lidar_points, projected_list, projection_matrix);

    /* bbox 처리 */
    convert_msg(yolo_msg, lidar_msg->header);
}

/* ===== bbox loop ===== */
void Object_Detection::convert_msg(const detect_msgs::Yolo_Objects::ConstPtr& yolo,
                                   const std_msgs::Header& header)
{
    std::vector<cv::Point2d> cur_centroids;
    pcl::PointCloud<pcl::PointXYZ>::Ptr out_groundfree(new pcl::PointCloud<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr out_bbox_roi (new pcl::PointCloud<pcl::PointXYZ>);

    for(const auto& box : yolo->yolo_objects)
    {
        /* ① bbox 축소 & 크기체크 */
        double x1 = box.x1, y1 = box.y1, x2 = box.x2, y2 = box.y2;
        const double cx = 0.5*(x1+x2), cy = 0.5*(y1+y2);
        const double hw = 0.5*(x2-x1)*BBOX_SCALE_RATIO;
        const double hh = 0.5*(y2-y1)*BBOX_SCALE_RATIO;
        x1 = std::max(0.0          , cx-hw);
        y1 = std::max(0.0          , cy-hh);
        x2 = std::min<double>(camera_image.cols-1, cx+hw);
        y2 = std::min<double>(camera_image.rows-1, cy+hh);
        if((x2-x1) < MIN_BBOX_EDGE_PX || (y2-y1) < MIN_BBOX_EDGE_PX) continue;

        /* ② bbox 내부 LiDAR 점 */
        std::vector<cv::Point2d> matched_px;
        pcl::PointCloud<pcl::PointXYZ>::Ptr local(new pcl::PointCloud<pcl::PointXYZ>);
        for(size_t i=0;i<projected_list.size();++i){
            double u = projected_list[i].x, v = projected_list[i].y;
            if(std::isnan(u)||std::isnan(v)) continue;
            if(u>=x1 && u<=x2 && v>=y1 && v<=y2){
                matched_px.emplace_back(u,v);
                local->points.emplace_back(lidar_points[i].x,
                                           lidar_points[i].y,
                                           lidar_points[i].z);
            }
        }
        if(matched_px.empty()) continue;

        /* ③ bbox 중심-원 ROI */
        const cv::Point2d c2d(cx,cy);
        pcl::PointCloud<pcl::PointXYZ>::Ptr roi(new pcl::PointCloud<pcl::PointXYZ>);
        for(size_t i=0;i<matched_px.size();++i)
            if(cv::norm(matched_px[i]-c2d) <= ROI_RADIUS_PX)
                roi->points.push_back(local->points[i]);
        if(roi->empty()) continue;
        *out_bbox_roi += *roi;           // 디버그용

        /* ④ RANSAC ground 제거 */
        pcl::PointCloud<pcl::PointXYZ>::Ptr roi_ng(new pcl::PointCloud<pcl::PointXYZ>);
        {
            std::vector<cv::Point3f> tmp; tmp.reserve(roi->size());
            for(const auto& p:*roi) tmp.emplace_back(p.x,p.y,p.z);
            const auto keep = remove_ground_ransac(tmp, GROUND_THRESH);
            if(keep.empty()) continue;
            roi_ng->points.reserve(keep.size());
            for(int id:keep) roi_ng->points.push_back(roi->points[id]);
        }

        /* ⑤ roi_ng 전체 평균 → centroid */
        double sx=0, sy=0;
        for(const auto& p:roi_ng->points){ sx += p.x;  sy += p.y; }
        if(!roi_ng->empty())
            cur_centroids.emplace_back(sx/roi_ng->size(), sy/roi_ng->size());

        /* ⑥ 시각화 (카메라 이미지) */
        cv::rectangle(camera_image,{(int)x1,(int)y1},{(int)x2,(int)y2},{0,255,0},2);
        cv::circle   (camera_image,c2d,4,{0,0,255},2);

        *out_groundfree += *roi_ng;
    }

    /* 디버그 publish */
    if(!out_bbox_roi->empty()){
        sensor_msgs::PointCloud2 m; pcl::toROSMsg(*out_bbox_roi,m);
        m.header = header; cloud_bbox_roi_pub.publish(m);
    }
    if(!out_groundfree->empty()){
        sensor_msgs::PointCloud2 m; pcl::toROSMsg(*out_groundfree,m);
        m.header = header; cloud_filter_pub.publish(m);
    }

    /* 추적 & 결과 publish */
    track_and_visualize(cur_centroids);
    publish_2D_pointcloud(cur_centroids, header);
}

/* ===== tracker helpers ===== */
void Object_Detection::match_and_update_trackers(const std::vector<cv::Point2f>& cents,
                                                 double match_dist,
                                                 int    max_miss)
{
    std::set<int> matched_t, matched_o;
    std::vector<cv::Point2f> preds; std::vector<int> ids;
    for(auto& kv:trackers){ preds.push_back(kv.second.predict()); ids.push_back(kv.first); }

    if(!cents.empty() && !preds.empty()){
        cv::Mat D((int)cents.size(), (int)preds.size(), CV_32F);
        for(int i=0;i<(int)cents.size();++i)
            for(int j=0;j<(int)preds.size();++j)
                D.at<float>(i,j) = cv::norm(cents[i]-preds[j]);

        while(true){
            double vmin; cv::Point loc;
            cv::minMaxLoc(D,&vmin,nullptr,&loc,nullptr);
            if(vmin > match_dist) break;
            int oi = loc.y, tj = loc.x;
            trackers[ids[tj]].update(cents[oi]);
            matched_t.insert(ids[tj]);
            matched_o.insert(oi);
            D.row(oi).setTo(1e9); D.col(tj).setTo(1e9);
        }
    }
    for(auto& kv:trackers) if(!matched_t.count(kv.first)) kv.second.miss();

    std::vector<int> del;
    for(const auto& kv:trackers)
        if(kv.second.miss_count > max_miss) del.push_back(kv.first);
    for(int id:del) trackers.erase(id);

    for(size_t i=0;i<cents.size();++i)
        if(!matched_o.count(i))
            trackers[next_tracker_id] = KalmanTracker(cents[i], next_tracker_id++);
}

void Object_Detection::track_and_visualize(const std::vector<cv::Point2d>& cents)
{
    std::vector<cv::Point2f> v; v.reserve(cents.size());
    for(const auto& p:cents) v.emplace_back((float)p.x,(float)p.y);
    match_and_update_trackers(v, MATCH_DIST, TRACKER_MAX_MISS);

    for(const auto& kv:trackers){
        cv::circle(camera_image, kv.second.last_pos, 6, {255,0,255}, 2);
        cv::putText(camera_image, std::to_string(kv.first),
                    kv.second.last_pos + cv::Point2f(5,-5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, {255,255,0}, 1);
    }
}

/* ===== publish centroid PointCloud (/cloud_centroid) ===== */
void Object_Detection::publish_2D_pointcloud(const std::vector<cv::Point2d>& pts,
                                             const std_msgs::Header& header)
{
    sensor_msgs::PointCloud cloud; cloud.header = header; cloud.header.frame_id = frame_name;
    for(const auto& p:pts){
        geometry_msgs::Point32 q; q.x = p.x; q.y = p.y; q.z = 0;
        cloud.points.push_back(q);
    }
    sensor_msgs::ChannelFloat32 dummy; dummy.name="dummy";
    dummy.values.resize(cloud.points.size(), 1.0f);
    cloud.channels.push_back(dummy);
    cloud_centroid.publish(cloud);
}

/* ===== DROR ===== */
std::vector<int> Object_Detection::dror_filter(const std::vector<cv::Point3f>& pts)
{
    std::vector<int> keep;
    std::vector<double> rxy(pts.size());
    for(size_t i=0;i<pts.size();++i) rxy[i] = std::hypot(pts[i].x, pts[i].y);

    for(size_t i=0;i<pts.size();++i){
        double R = DROR_MIN_RADIUS + DROR_RADIUS_SCALE * rxy[i];
        R = std::max(DROR_MIN_RADIUS, std::min(R, DROR_MAX_RADIUS));
        int cnt = 0;
        for(size_t j=0;j<pts.size();++j){
            if(i==j) continue;
            if(cv::norm(cv::Point3f(pts[i].x-pts[j].x,
                                    pts[i].y-pts[j].y,
                                    pts[i].z-pts[j].z)) < R) ++cnt;
        }
        if(cnt >= DROR_MIN_NEIGHBORS) keep.push_back((int)i);
    }
    return keep;
}

/* ===== 지면 제거 (RANSAC) ===== */
std::vector<int> Object_Detection::remove_ground_ransac(const std::vector<cv::Point3f>& pts,
                                                        double th)
{
    if(pts.size() < 10){
        std::vector<int> id(pts.size());
        std::iota(id.begin(), id.end(), 0);
        return id;
    }

    std::default_random_engine gen;
    std::uniform_int_distribution<> d(0, pts.size()-1);

    double a=0,b=0,c=0; int best_in=0;
    for(int it=0; it<30; ++it){
        int i1=d(gen), i2=d(gen), i3=d(gen);
        const auto &p1=pts[i1], &p2=pts[i2], &p3=pts[i3];
        double den = p1.x*(p2.y-p3.y) + p2.x*(p3.y-p1.y) + p3.x*(p1.y-p2.y);
        if(std::abs(den) < 1e-6) continue;
        double ta = (p1.z*(p2.y-p3.y) + p2.z*(p3.y-p1.y) + p3.z*(p1.y-p2.y)) / den;
        double tb = (p1.x*(p2.z-p3.z) + p2.x*(p3.z-p1.z) + p3.x*(p1.z-p2.z)) / den;
        double tc = p1.z - ta*p1.x - tb*p1.y;

        int in=0; for(const auto& p:pts)
            if(std::abs(p.z - (ta*p.x + tb*p.y + tc)) < th) ++in;
        if(in > best_in){ best_in = in; a=ta; b=tb; c=tc; }
    }

    std::vector<int> idx;
    for(size_t i=0;i<pts.size();++i)
        if(std::abs(pts[i].z - (a*pts[i].x + b*pts[i].y + c)) > th)
            idx.push_back((int)i);
    return idx;
}

/* ===== grid-based ground_filter (옵션 디버그용) ===== */
pcl::PointCloud<pcl::PointXYZ>::Ptr
Object_Detection::ground_filter(pcl::PointCloud<pcl::PointXYZ> cloud)
{
    const double h_th = 0.0;
    const int    N    = 320;
    const double cell = 0.2;

    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>);
    static bool  init[N][N]; static float min_h[N][N], max_h[N][N];
    std::memset(init,0,sizeof(init));

    for(auto& p:cloud.points){
        int xi = (int)(N/2 + p.x/cell);
        int yi = (int)(N/2 + p.y/cell);
        if(xi>=0 && xi<N && yi>=0 && yi<N){
            if(!init[xi][yi]){
                init[xi][yi] = true;
                min_h[xi][yi] = max_h[xi][yi] = p.z;
            }else{
                min_h[xi][yi] = std::min(min_h[xi][yi], p.z);
                max_h[xi][yi] = std::max(max_h[xi][yi], p.z);
            }
        }
    }
    double offset = N/2.0 * cell;
    for(int xi=0; xi<N; ++xi)
        for(int yi=0; yi<N; ++yi)
            if(init[xi][yi] && (max_h[xi][yi] - min_h[xi][yi] > h_th)){
                pcl::PointXYZ q;
                q.x = -offset + (xi*cell + cell/2.0);
                q.y = -offset + (yi*cell + cell/2.0);
                q.z = -0.4;
                filtered->push_back(q);
            }
    return filtered;
}
