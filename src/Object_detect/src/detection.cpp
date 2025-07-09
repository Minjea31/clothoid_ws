// detection.cpp
#include "detection.h"

#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl/search/kdtree.h>
#include <random>

/* ===== Object_Detection ctor / dtor ===== */
Object_Detection::Object_Detection(ros::NodeHandle *nodeHandle)
    : nh(*nodeHandle),
      oc_sort_(30 /*max_age*/, 3 /*min_hits*/, 1.0 /*iou_threshold*/)
{
    cloud_minjae_pub  = nh.advertise<sensor_msgs::PointCloud> ("/minjae",       1);
    cloud_fillter_pub = nh.advertise<sensor_msgs::PointCloud2>("/cloud_fillter",1);

    nh.param("lidar_topic",  lidar_topic,  std::string("/livox/lidar"));
    nh.param("camera_topic", camera_topic, std::string("/camera/image_raw/compressed"));
    nh.param("yolo_topic",   yolo_topic,   std::string("/yolov8_pub"));
    nh.param("frame_name",   frame_name,   std::string("livox_frame"));

    message_filters::Subscriber<sensor_msgs::PointCloud2>     subL(nh, lidar_topic, 10);
    message_filters::Subscriber<sensor_msgs::CompressedImage> subC(nh, camera_topic, 10);
    message_filters::Subscriber<detect_msgs::Yolo_Objects>     subY(nh, yolo_topic, 10);

    using SyncPolicy = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::PointCloud2,
        sensor_msgs::CompressedImage,
        detect_msgs::Yolo_Objects>;
    auto syncer = std::make_shared<message_filters::Synchronizer<SyncPolicy>>
        (SyncPolicy(20), subL, subC, subY);
    syncer->setMaxIntervalDuration(ros::Duration(0.05));
    syncer->registerCallback(
        boost::bind(&Object_Detection::detectionCallback, this, _1, _2, _3));

    read_projection_matrix();
    ROS_INFO("start detection");
    ros::spin();
}

Object_Detection::~Object_Detection() {
    ROS_INFO("finish detection");
}

/* ===== detectionCallback ===== */
void Object_Detection::detectionCallback(
    const sensor_msgs::PointCloud2::ConstPtr &lidar_msg,
    const sensor_msgs::CompressedImage::ConstPtr &cam_msg,
    const detect_msgs::Yolo_Objects::ConstPtr &yolo_msg)
{
    camera_image = cv_bridge::toCvCopy(cam_msg, "bgr8")->image;

    pcl::PointCloud<pcl::PointXYZI>::Ptr pc(new pcl::PointCloud<pcl::PointXYZI>());
    pcl::fromROSMsg(*lidar_msg, *pc);

    lidar_points.clear();
    for (const auto &p : pc->points) {
        lidar_points.emplace_back(p.x, p.y, p.z);
    }
    if (lidar_points.empty()) return;

    cv::perspectiveTransform(lidar_points, projected_list, projection_matrix);
    convert_msg(yolo_msg, lidar_msg->header);
}

/* ===== convert_msg ===== */
void Object_Detection::convert_msg(
    const detect_msgs::Yolo_Objects::ConstPtr &yolo,
    const std_msgs::Header &header)
{
    // 1) Prepare 2D detections for OC-SORT
    std::vector<Detection> dets;
    dets.reserve(yolo->yolo_objects.size());
    for (const auto &Y : yolo->yolo_objects) {
        dets.push_back({
            float(Y.x1), float(Y.y1),
            float(Y.x2), float(Y.y2),
            float(Y.score)  // confidence
        });
    }

    // 2) Run OC-SORT tracker
    auto tracks = oc_sort_.update(dets);  // returns vector of {track_id,x1,y1,x2,y2}

    // 3) Compute centroids from tracks
    std::vector<cv::Point2d> oc_centroids;
    oc_centroids.reserve(tracks.size());
    for (const auto &t : tracks) {
        double cx = (t.x1 + t.x2) * 0.5;
        double cy = (t.y1 + t.y2) * 0.5;
        oc_centroids.emplace_back(cx, cy);
    }

    // 4) Publish 2D centroids
    publish_2D_pointcloud(oc_centroids, header);

    // 5) (Optional) publish filtered 3D ROI cloud as before
    pcl::PointCloud<pcl::PointXYZ>::Ptr out_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    // ... (your existing 3D filtering logic here) ...
    if (!out_cloud->empty()) {
        sensor_msgs::PointCloud2 msg_pc2;
        pcl::toROSMsg(*out_cloud, msg_pc2);
        msg_pc2.header = header;
        cloud_fillter_pub.publish(msg_pc2);
    }
}

/* ===== publish 2D PointCloud ===== */
void Object_Detection::publish_2D_pointcloud(
    const std::vector<cv::Point2d> &pts,
    const std_msgs::Header &hdr)
{
    sensor_msgs::PointCloud cloud;
    cloud.header = hdr;
    cloud.header.frame_id = frame_name;
    for (const auto &p : pts) {
        geometry_msgs::Point32 q;
        q.x = p.x; q.y = p.y; q.z = 0;
        cloud.points.push_back(q);
    }
    sensor_msgs::ChannelFloat32 ch;
    ch.name = "dummy";
    ch.values.resize(cloud.points.size(), 1.0f);
    cloud.channels.push_back(ch);
    cloud_minjae_pub.publish(cloud);
}

/* ===== read_projection_matrix ===== */
void Object_Detection::read_projection_matrix()
{
    double fx=1.79e3, fy=1.7869e3, cx=960.4433, cy=595.1015;
    cv::Mat K = (cv::Mat_<double>(3,3)<< fx,0,cx, 0,fy,cy, 0,0,1);
    cv::Mat T = (cv::Mat_<double>(3,4)<<
        -0.0119,-0.9999,-0.0025, 0.0593,
        -0.0423, 0.0030,-0.9991, 0.0446,
         0.9990,-0.0118,-0.0423,-0.1091);
    projection_matrix = K * T;
}

/* ===== (unchanged) RANSAC ground removal ===== */
std::vector<int> Object_Detection::remove_ground_ransac(
    const std::vector<cv::Point3f> &pts, double th)
{
    if (pts.size() < 10) {
        std::vector<int> id(pts.size());
        std::iota(id.begin(), id.end(), 0);
        return id;
    }
    int max_in = 0; double a=0,b=0,c=0;
    std::default_random_engine gen;
    std::uniform_int_distribution<> d(0, pts.size()-1);
    for (int iter=0; iter<30; ++iter) {
        int i1=d(gen), i2=d(gen), i3=d(gen);
        double x1=pts[i1].x, y1=pts[i1].y, z1=pts[i1].z,
               x2=pts[i2].x, y2=pts[i2].y, z2=pts[i2].z,
               x3=pts[i3].x, y3=pts[i3].y, z3=pts[i3].z;
        double den = x1*(y2-y3)+x2*(y3-y1)+x3*(y1-y2);
        if (std::abs(den) < 1e-6) continue;
        double ta = (z1*(y2-y3)+z2*(y3-y1)+z3*(y1-y2)) / den;
        double tb = (x1*(z2-z3)+x2*(z3-z1)+x3*(z1-z2)) / den;
        double tc = z1 - ta*x1 - tb*y1;
        int in=0;
        for (const auto &p: pts)
            if (std::abs(p.z - (ta*p.x + tb*p.y + tc)) < th)
                ++in;
        if (in > max_in) { max_in=in; a=ta; b=tb; c=tc; }
    }
    std::vector<int> idx;
    for (size_t i=0; i<pts.size(); ++i)
        if (std::abs(pts[i].z - (a*pts[i].x + b*pts[i].y + c)) > th)
            idx.push_back((int)i);
    return idx;
}
