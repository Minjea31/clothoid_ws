#pragma once
/************************************************
 *  detection.h   (ROI · Debug topics 포함)
 ************************************************/

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

#include <ros/ros.h>
#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Geometry>

#include <opencv2/opencv.hpp>
#include <cv_bridge/cv_bridge.h>

#include <pcl_ros/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl_conversions/pcl_conversions.h>

#include <std_msgs/Header.h>
#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/PointCloud2.h>
#include <detect_msgs/Yolo_Objects.h>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <cstring>

/* ───────── 하이퍼파라미터 ───────── */
static constexpr double VOXEL_SIZE          = 0.05;

/* DROR */
static constexpr int    DROR_MIN_NEIGHBORS  = 8;
static constexpr double DROR_MIN_RADIUS     = 0.05;
static constexpr double DROR_RADIUS_SCALE   = 0.20;
static constexpr double DROR_MAX_RADIUS     = 1.00;

/* 지면 제거 */
static constexpr double GROUND_THRESH       = 0.3;

/* bbox 후처리 & ROI */
static constexpr double BBOX_SCALE_RATIO    = 0.5;
static constexpr int    MIN_BBOX_EDGE_PX    = 0;
static constexpr double ROI_RADIUS_PX       = 300;   // 원 반경(px)

/* Kalman 추적 */
static constexpr double MATCH_DIST          = 30.0;
static constexpr int    TRACKER_MAX_MISS    = 10;

/* 공간 ROI */
static constexpr double ROI_X_MIN = -20.0;
static constexpr double ROI_X_MAX =  40.0;
static constexpr double ROI_Y_MIN = -10.0;
static constexpr double ROI_Y_MAX =  10.0;
static constexpr double ROI_Z_MIN =  -3.0;
static constexpr double ROI_Z_MAX =   3.0;

/* ───────── KalmanTracker ───────── */
struct KalmanTracker {
    int id{-1};
    int miss_count{0};
    cv::KalmanFilter kf;
    cv::Point2f last_pos;

    KalmanTracker() = default;
    KalmanTracker(const cv::Point2f& pt, int tracker_id, float dt = 0.1);
    cv::Point2f predict();
    void update(const cv::Point2f& pt);
    void miss();
};

/* ───────── Object_Detection ───────── */
class Object_Detection {
private:
    ros::NodeHandle nh;

    /* publishers */
    ros::Publisher cloud_centroid;      // /cloud_centroid
    ros::Publisher cloud_filter_pub;    // /cloud_filter
    ros::Publisher cloud_roi_pub;       // /cloud_roi
    ros::Publisher cloud_bbox_roi_pub;  // /cloud_bbox_roi

    std::string lidar_topic, camera_topic, yolo_topic, frame_name;

    /* per-frame data */
    cv::Mat projection_matrix;
    cv::Mat camera_image;
    std::vector<cv::Point3d> lidar_points;
    std::vector<cv::Point2d> projected_list;

    /* helpers */
    void read_projection_matrix();
    void detectionCallback(const sensor_msgs::PointCloud2::ConstPtr& lidar_msg,
                           const sensor_msgs::CompressedImage::ConstPtr& cam_msg,
                           const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg);
    void filter_pointcloud(std::vector<cv::Point3d>& pts);
    void convert_msg(const detect_msgs::Yolo_Objects::ConstPtr& yolo,
                     const std_msgs::Header& header);

    void publish_2D_pointcloud(const std::vector<cv::Point2d>& pts,
                               const std_msgs::Header& header);

    void track_and_visualize(const std::vector<cv::Point2d>& cents);
    void match_and_update_trackers(const std::vector<cv::Point2f>& cents,
                                   double match_dist = MATCH_DIST,
                                   int    max_miss   = TRACKER_MAX_MISS);

    std::vector<int> remove_ground_ransac(const std::vector<cv::Point3f>& pts,
                                          double threshold = GROUND_THRESH);
    std::vector<int> dror_filter(const std::vector<cv::Point3f>& pts);
    pcl::PointCloud<pcl::PointXYZ>::Ptr ground_filter(pcl::PointCloud<pcl::PointXYZ> cloud);

public:
    explicit Object_Detection(ros::NodeHandle* nh);
    ~Object_Detection();
};
