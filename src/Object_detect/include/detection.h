#pragma once

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <ros/ros.h>

#include <eigen3/Eigen/Core>
#include <eigen3/Eigen/Eigen>
#include <eigen3/Eigen/Geometry>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <cv_bridge/cv_bridge.h>

#include <pcl_ros/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/conditional_removal.h>
#include <pcl/search/kdtree.h>
#include <pcl/segmentation/extract_clusters.h>
#include <pcl_conversions/pcl_conversions.h>

#include <std_msgs/Header.h>
#include <sensor_msgs/PointCloud.h>
#include <sensor_msgs/CompressedImage.h>

#include <detect_msgs/Yolo_Objects.h>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

class Object_Detection {
private:
    ros::NodeHandle nh;
    ros::Publisher  cloud_centeroid;

    std::string lidar_topic, camera_topic, yolo_topic, frame_name;

    pcl::ConditionalRemoval<pcl::PointXYZI> filter;
    pcl::ConditionAnd<pcl::PointXYZI>::Ptr filter_range;
    double xMinRange, xMaxRange, yMinRange, yMaxRange, zMinRange, zMaxRange;

    double cluster_tolerance;
    int    cluster_min;
    int    cluster_max;

    cv::Mat projection_matrix;
    cv::Mat camera_image;
    std::vector<cv::Point3d> lidar_points;
    std::vector<cv::Point2d> projected_list;
    std::vector<double> distance_list;

    std::vector<cv::Point2d> prev_centroids;
    bool is_first_frame;
    double alpha;

    int counter;

    void read_projection_matrix();
    void detectionCallback(const sensor_msgs::PointCloud2::ConstPtr& lidar_msg,
                           const sensor_msgs::CompressedImage::ConstPtr& camera_msg,
                           const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg);
    void convert_msg(const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg,
                     const std_msgs::Header& header);
    void publish_2D_pointcloud(const std::vector<cv::Point2d>& pts, const std_msgs::Header& header);
    pcl::PointCloud<pcl::PointXYZ>::Ptr ground_filter(pcl::PointCloud<pcl::PointXYZ> cloud);

public:
    Object_Detection(ros::NodeHandle* nodeHandle);
    ~Object_Detection();
};

