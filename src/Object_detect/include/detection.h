#pragma once
/************************************************
 *  detection.h   (완전 병합 버전)
 ************************************************/

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <ros/ros.h>

#include <eigen3/Eigen/Core>
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

/* ===== 전역 constexpr 파라미터 ===== */

/* 1) 포인트 클라우드  ↓↓↓ */
static constexpr double VOXEL_SIZE = 0.05;
/*  - LiDAR point cloud down‑sampling voxel 크기(m).  
 *    작게   → 해상도 ↑(세밀) / 연산량 ↑  
 *    크게   → 해상도 ↓(거칠) / 연산량 ↓                                               */


/* 2) DROR(Density‐Based Radial Outlier Removal) ↓↓↓ */
static constexpr int    DROR_MIN_NEIGHBORS  = 5;
/*  - 한 점이 ‘유효’ 판정되기 위한 최소 이웃 수.  
 *    작게 → 더 많은 점 통과(노이즈 ↑)  
 *    크게 → 고밀도 영역만 통과(노이즈 ↓)                                       */

static constexpr double BBOX_SCALE_RATIO    =   0.8;

static constexpr double DROR_MIN_RADIUS     =     0.1;   // m
/*  - 근거리(작은 range) 점들에 적용되는 최소 반경.  
 *    작게 → 촘촘한 근접 점까지 검출  
 *    크게 → 근거리에서도 성긴 점 제거                                           */

static constexpr double DROR_RADIUS_SCALE   = 0.7;   // 계수
/*  - 거리(r) 증가에 따라 반경 = MIN_RADIUS + SCALE·r 로 선형 확장.  
 *    작게 → 원거리에서도 작은 검색 반경(더 엄격)  
 *    크게 → 원거리 점도 넉넉히 살펴봄(노이즈↑ / 실측 유지↑)                     */

static constexpr double DROR_MAX_RADIUS     = 0.7;   // m
/*  - 검색 반경의 상한.  
 *    작게 → 매우 먼 점 검증에 제한, 노이즈 제거↑  
 *    크게 → 먼 거리까지 포용, 객체 놓칠 위험 ↓                                   */


/* 3) 지면 제거(RANSAC Plane) ↓↓↓ */
static constexpr double GROUND_THRESH       = 0.;   // m
/*  - 평면으로부터 허용되는 최대 높이 오차.  
 *    작게 → 얇게 잘려 깨끗한 지면 분리(과제거 위험)  
 *    크게 → 지면 일부 남을 수 있으나 과제거 ↓                                     */


/* 4) Euclidean 클러스터링 ↓↓↓ */
static constexpr double CLUSTER_TOLERANCE   = 0.4;   // m
/*  - 점‑점 연결 임계 거리.  
 *    작게 → 객체가 더 세분화(과분할)  
 *    크게 → 여러 객체가 하나로 뭉칠 위험                                           */

static constexpr int    CLUSTER_MIN_SIZE    = 0;     // points
/*  - 이보다 작은 군집은 버림.  
 *    작게 → 작고 희박한 객체까지 검출(노이즈↑)  
 *    크게 → 작은 객체 무시(오검↓)                                                */

static constexpr double ROI_RADIUS_PX       = 10.0;  // pixel
/*  - bbox 내부에서 LiDAR 점 재선택 시 원형 반경.  
 *    작게 → 중심부 집중(외곽 잡음 ↓)  
 *    크게 → 더 많은 점 포함(대상 전체 포착)                                       */


static constexpr double MATCH_DIST          = 7.0;   // 2‑D 이미지 거리(px) 또는 정규화 거리
/*  - 칼만 예측 ↔ 관측 센트로이드 매칭 허용 거리.  
 *    작게 → 보수적 매칭(스킵 ↑)  
 *    크게 → 오매칭(잘못 연결) 위험 ↑ 


 ##############################################################################

/* 6) 추적‑매칭 ↓↓↓ */
static constexpr int    TRACKER_MAX_MISS    = 15;     // frames
/*  - 추적기 미검출 허용 프레임 수.  
 *    작게 → 빠른 삭제(유실↑)  
 *    크게 → 오래 유지(유령 Tracker ↑)                                            */

/* 5) 이미지 ROI / bbox 후처리 ↓↓↓ */
static constexpr int    MIN_BBOX_EDGE_PX    = 0;    // pixel
/*  - YOLO bbox 최소 길이·높이.  
 *    작게 → 작은 bbox 유지(소형 물체 검출)  
 *    크게 → 작은 물체 필터링(잡음 ↓)                                            */

static constexpr int    CLUSTER_MAX_SIZE    = 100;   // points
/*  - 너무 큰 군집(지면·벽 등) 제거용 상한.  
 *    작게 → 대형 객체 잘림  
 *    크게 → 배경이 클러스터로 남을 가능 ↑                                         */

/* ===== 데이터 구조 ===== */
struct object_struct{
    double x{0}, y{0}, z{0};
    double u{0}, v{0};
    int    id{-1};
    int    x_min{0}, y_min{0}, x_max{0}, y_max{0};
};

/* ===== Kalman Tracker ===== */
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

/* ===== Object_Detection ===== */
class Object_Detection {
private:
    ros::NodeHandle nh;
    ros::Publisher  cloud_centroid;   // <- 주의: centroid (타이포 수정)
    ros::Publisher cloud_fillter_pub; // ★ 추가

    std::string lidar_topic, camera_topic, yolo_topic, frame_name;

    pcl::ConditionalRemoval<pcl::PointXYZI> filter;
    pcl::ConditionAnd<pcl::PointXYZI>::Ptr   filter_range;
    double xMinRange, xMaxRange, yMinRange, yMaxRange, zMinRange, zMaxRange;

    double cluster_tolerance;
    int    cluster_min;
    int    cluster_max;

    cv::Mat projection_matrix;
    cv::Mat camera_image;
    std::vector<cv::Point3d> lidar_points;
    std::vector<cv::Point2d> projected_list;
    std::vector<double>      distance_list;

    std::vector<cv::Point2d> prev_centroids;
    bool   is_first_frame;
    double alpha;
    int    counter;

    /* === 내부 함수 === */
    void read_projection_matrix();
    void detectionCallback(const sensor_msgs::PointCloud2::ConstPtr& lidar_msg,
                           const sensor_msgs::CompressedImage::ConstPtr& camera_msg,
                           const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg);
    void convert_msg(const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg,
                     const std_msgs::Header& header);
    void publish_2D_pointcloud(const std::vector<cv::Point2d>& pts,
                               const std_msgs::Header& header);

    void track_and_visualize(const std::vector<cv::Point2d>& cents);
    void match_and_update_trackers(const std::vector<cv::Point2f>& cents,
                                   double match_dist  = MATCH_DIST,
                                   int    max_miss    = TRACKER_MAX_MISS);

    /* ★★ 새로 추가할 한 줄 ★★ */
    void filter_pointcloud(std::vector<cv::Point3d>& pts,
                           std::vector<cv::Point2d>& proj);
    std::vector<int> dror_filter(const std::vector<cv::Point3f>& pts);
    std::vector<int> remove_ground_ransac(const std::vector<cv::Point3f>& pts,
                                          double threshold = GROUND_THRESH);
    pcl::PointCloud<pcl::PointXYZ>::Ptr ground_filter(pcl::PointCloud<pcl::PointXYZ> cloud);

public:
    explicit Object_Detection(ros::NodeHandle* nh);
    ~Object_Detection();
};
