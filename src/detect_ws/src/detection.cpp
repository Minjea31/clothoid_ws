#include "detection.h"
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_conversions/pcl_conversions.h>

Object_Detection::Object_Detection(ros::NodeHandle* nodeHandle) : counter(0){
    ROS_INFO("start detection");
    nh = *nodeHandle;

    cloud_centeroid = nh.advertise<sensor_msgs::PointCloud2>("/cloud_centeroid", 1);

    nh.param<std::string>("lidar_topic",    lidar_topic,  "/livox/lidar");
    nh.param<std::string>("camera_topic",   camera_topic, "/camera/image_raw/compressed");
    nh.param<std::string>("yolo_topic",     yolo_topic,   "/yolov8_pub");
    nh.param<std::string>("frame_name",     frame_name,   "livox_frame");

    nh.param<double>("xMinRange",  xMinRange,  0.0);
    nh.param<double>("xMaxRange",  xMaxRange, 20.0);
    nh.param<double>("yMinRange",  yMinRange, -6.0);
    nh.param<double>("yMaxRange",  yMaxRange, 6.0);
    nh.param<double>("zMinRange",  zMinRange, -0.5);
    nh.param<double>("zMaxRange",  zMaxRange, 0.0);

    nh.param<double>("cluster_tolerance", cluster_tolerance, 1.0);
    nh.param<int>("cluster_min", cluster_min, 10);
    nh.param<int>("cluster_max", cluster_max, 100);

    message_filters::Subscriber<sensor_msgs::PointCloud2>     lidar_sub(nh, lidar_topic,  10);
    message_filters::Subscriber<sensor_msgs::CompressedImage> camera_sub(nh, camera_topic, 10);
    message_filters::Subscriber<detect_msgs::Yolo_Objects>    yolo_sub( nh, yolo_topic,   10);

    typedef message_filters::sync_policies::ApproximateTime<
        sensor_msgs::PointCloud2,
        sensor_msgs::CompressedImage,
        detect_msgs::Yolo_Objects
    > Sync;
    auto syncer = boost::make_shared<message_filters::Synchronizer<Sync>>(Sync(10),
                lidar_sub, camera_sub, yolo_sub);
    syncer->registerCallback(boost::bind(&Object_Detection::detectionCallback,
                                         this, _1, _2, _3));

    filter_range.reset(new pcl::ConditionAnd<pcl::PointXYZI>());
    filter_range->addComparison(pcl::FieldComparison<pcl::PointXYZI>::ConstPtr(
        new pcl::FieldComparison<pcl::PointXYZI>("x", pcl::ComparisonOps::LT, xMaxRange)));
    filter_range->addComparison(pcl::FieldComparison<pcl::PointXYZI>::ConstPtr(
        new pcl::FieldComparison<pcl::PointXYZI>("x", pcl::ComparisonOps::GT, xMinRange)));
    filter_range->addComparison(pcl::FieldComparison<pcl::PointXYZI>::ConstPtr(
        new pcl::FieldComparison<pcl::PointXYZI>("y", pcl::ComparisonOps::LT, yMaxRange)));
    filter_range->addComparison(pcl::FieldComparison<pcl::PointXYZI>::ConstPtr(
        new pcl::FieldComparison<pcl::PointXYZI>("y", pcl::ComparisonOps::GT, yMinRange)));
    filter_range->addComparison(pcl::FieldComparison<pcl::PointXYZI>::ConstPtr(
        new pcl::FieldComparison<pcl::PointXYZI>("z", pcl::ComparisonOps::LT, zMaxRange)));
    filter_range->addComparison(pcl::FieldComparison<pcl::PointXYZI>::ConstPtr(
        new pcl::FieldComparison<pcl::PointXYZI>("z", pcl::ComparisonOps::GT, zMinRange)));

    read_projection_matrix();
    ros::spin();
}

Object_Detection::~Object_Detection(){
    ROS_INFO("finish detection");
}

void Object_Detection::detectionCallback(const sensor_msgs::PointCloud2::ConstPtr& lidar_msg,
                                         const sensor_msgs::CompressedImage::ConstPtr& camera_msg,
                                         const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg){
    ROS_INFO("Callback...");

    auto cv_ptr = cv_bridge::toCvCopy(camera_msg, sensor_msgs::image_encodings::BGR8);
    camera_image = cv_ptr->image;

    pcl::PointCloud<pcl::PointXYZI>::Ptr pc(new pcl::PointCloud<pcl::PointXYZI>());
    pcl::fromROSMsg(*lidar_msg, *pc);
    filter.setCondition(filter_range);
    filter.setInputCloud(pc);
    filter.setKeepOrganized(false);
    filter.filter(*pc);

    lidar_points.clear();
    distance_list.clear();
    for (auto& p : pc->points){
        lidar_points.emplace_back(p.x, p.y, p.z);
        distance_list.push_back(std::sqrt(p.x*p.x + p.y*p.y + p.z*p.z));
    }

    if (!lidar_points.empty()){
        cv::perspectiveTransform(lidar_points, projected_list, projection_matrix);
        convert_msg(yolo_msg, lidar_msg->header);
    }
}

void Object_Detection::convert_msg(const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg,
                                   const std_msgs::Header& header){
    std::vector<cv::Point2d> current_centroids;
    for (auto& Y : yolo_msg->yolo_objects){
        int x_min = Y.x1, y_min = Y.y1, x_max = Y.x2, y_max = Y.y2;
        if (x_max - x_min < 30) continue;

        double sx = 0, sy = 0;
        int count = 0;
        for (size_t i = 0; i < projected_list.size(); ++i){
            double u = projected_list[i].x, v = projected_list[i].y;
            if (u >= x_min+10 && u <= x_max-10 && v >= y_min+10 && v <= y_max-10){
                sx += lidar_points[i].x;
                sy += lidar_points[i].y;
                ++count;
            }
        }
        if (count == 0) continue;
        current_centroids.emplace_back(sx / count, sy / count);
    }

    // EMA smoothing
    std::vector<cv::Point2d> smoothed_centroids;
    if (is_first_frame){
        smoothed_centroids = current_centroids;
        is_first_frame = false;
    } else {
        for (size_t i = 0; i < current_centroids.size(); ++i){
            if (i >= prev_centroids.size()){
                smoothed_centroids.push_back(current_centroids[i]);
                continue;
            }
            cv::Point2d s;
            s.x = alpha * current_centroids[i].x + (1.0 - alpha) * prev_centroids[i].x;
            s.y = alpha * current_centroids[i].y + (1.0 - alpha) * prev_centroids[i].y;
            smoothed_centroids.push_back(s);
        }
    }
    prev_centroids = smoothed_centroids;

    // publish pointcloud
    publish_2D_pointcloud(smoothed_centroids, header);
}

void Object_Detection::publish_2D_pointcloud(const std::vector<cv::Point2d>& pts, const std_msgs::Header& header){
    pcl::PointCloud<pcl::PointXYZ> cloud;
    for (const auto& pt : pts){
        pcl::PointXYZ p;
        p.x = pt.x;
        p.y = pt.y;
        p.z = 0.0;
        cloud.points.push_back(p);
    }
    sensor_msgs::PointCloud2 msg;
    pcl::toROSMsg(cloud, msg);
    msg.header = header;
    cloud_centeroid.publish(msg);
}

void Object_Detection::read_projection_matrix(){
    double fx = 1.8555e+03, fy = 1.8549e+03;
    double cx = 950.6236, cy = 575.6943;
    cv::Mat camera_matrix = (cv::Mat_<double>(3,3)<< fx,0,cx, 0,fy,cy, 0,0,1);
    cv::Mat T = (cv::Mat_<double>(3,4)<<
        -0.0259,-0.9994,0.0212,0.0269,
        -0.0177,-0.0207,-0.9996,-0.1107,
         0.9995,-0.0263,-0.0172,-0.0358,
         0.0,0.0,0.0,1.0);
    projection_matrix = camera_matrix * T;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr Object_Detection::ground_filter(pcl::PointCloud<pcl::PointXYZ> cloud){
    double height_thresh = 0.0;
    int grid_dim = 320; double per_cell = 0.2;
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>());
    static bool init[320][320];
    static float min_h[320][320], max_h[320][320];
    memset(init, 0, sizeof(init));

    for (auto& p : cloud.points){
        int xi = int(grid_dim/2 + p.x/per_cell);
        int yi = int(grid_dim/2 + p.y/per_cell);
        if (xi>=0 && xi<grid_dim && yi>=0 && yi<grid_dim){
            if (!init[xi][yi]){
                init[xi][yi] = true;
                min_h[xi][yi] = max_h[xi][yi] = p.z;
            } else {
                min_h[xi][yi] = std::min(min_h[xi][yi], p.z);
                max_h[xi][yi] = std::max(max_h[xi][yi], p.z);
            }
        }
    }

    double offset = grid_dim/2.0 * per_cell;
    for (int xi=0; xi<grid_dim; ++xi){
        for (int yi=0; yi<grid_dim; ++yi){
            if (init[xi][yi] && (max_h[xi][yi] - min_h[xi][yi] > height_thresh)){
                pcl::PointXYZ pt;
                pt.x = -offset + (xi*per_cell + per_cell/2.0);
                pt.y = -offset + (yi*per_cell + per_cell/2.0);
                pt.z = -0.4;
                filtered->points.push_back(pt);
            }
        }
    }
    return filtered;
}

