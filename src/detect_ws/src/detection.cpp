// detection.cpp
#include <detection.h>
#include <detect_msgs/detected_array.h>
#include <detect_msgs/detected_object.h>
#include <sensor_msgs/CompressedImage.h>

Object_Detection::Object_Detection(ros::NodeHandle* nodeHandle) : counter(0){
    ROS_INFO("start detection");
    nh = *nodeHandle;

    cloud_centeroid = nh.advertise<sensor_msgs::PointCloud2>("/cloud_centeroid", 1);
    pub_detected    = nh.advertise<detect_msgs::detected_array>("/detected_objects_3d", 1);

    nh.param<std::string>("lidar_topic", lidar_topic, "/livox/lidar");
    nh.param<std::string>("camera_topic", camera_topic, "/camera/image_raw/compressed");
    nh.param<std::string>("yolo_topic", yolo_topic, "/yolov8_pub");
    nh.param<std::string>("frame_name", frame_name, "livox_frame");

    nh.param<double>("xMinRange", xMinRange, 0);
    nh.param<double>("xMaxRange", xMaxRange, 20);
    nh.param<double>("yMinRange", yMinRange, -6);
    nh.param<double>("yMaxRange", yMaxRange, 6);
    nh.param<double>("zMinRange", zMinRange, -0.5);
    nh.param<double>("zMaxRange", zMaxRange, 0.0);

    nh.param<double>("cluster_tolerance", cluster_tolerance, 1.0);
    nh.param<int>("cluster_min", cluster_min, 4);
    nh.param<int>("cluster_max", cluster_max, 100);

    message_filters::Subscriber<sensor_msgs::PointCloud2>     lidar_sub(nh, lidar_topic, 10);
    message_filters::Subscriber<sensor_msgs::CompressedImage> camera_sub(nh, camera_topic, 10);
    message_filters::Subscriber<detect_msgs::Yolo_Objects>    yolo_sub(nh, yolo_topic, 10);

    typedef message_filters::sync_policies::ApproximateTime<
        sensor_msgs::PointCloud2,
        sensor_msgs::CompressedImage,
        detect_msgs::Yolo_Objects
    > Sync;

    boost::shared_ptr<message_filters::Synchronizer<Sync>> sync;
    sync.reset(new message_filters::Synchronizer<Sync>(Sync(10),
                lidar_sub, camera_sub, yolo_sub));
    sync->registerCallback(boost::bind(&Object_Detection::detectionCallback,
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

    // compressed → cv::Mat
    cv_bridge::CvImagePtr cv_ptr = cv_bridge::toCvCopy(
        camera_msg, sensor_msgs::image_encodings::BGR8);
    camera_image = cv_ptr->image;

    pcl::PointCloud<pcl::PointXYZI>::Ptr pointcloud(new pcl::PointCloud<pcl::PointXYZI>());
    pcl::fromROSMsg(*lidar_msg, *pointcloud);

    filter.setCondition(filter_range);
    filter.setInputCloud(pointcloud);
    filter.setKeepOrganized(false);
    filter.filter(*pointcloud);

    lidar_points.clear();
    intensity_list.clear();
    distance_list.clear();
    for (size_t i = 0; i < pointcloud->size(); ++i){
        cv::Point3d pt(pointcloud->points[i].x,
                       pointcloud->points[i].y,
                       pointcloud->points[i].z);
        lidar_points.push_back(pt);
        intensity_list.push_back(pointcloud->points[i].intensity);
        distance_list.push_back(sqrt(pt.x*pt.x + pt.y*pt.y + pt.z*pt.z));
    }

    if (!lidar_points.empty()){
        cv::perspectiveTransform(lidar_points, projected_list, projection_matrix);
        convert_msg(yolo_msg, lidar_msg->header);
    }
}

void Object_Detection::convert_msg(const detect_msgs::Yolo_Objects::ConstPtr& yolo_msg,
                                   const std_msgs::Header& header){
    detect_msgs::detected_array out;
    out.header = header;

    for (size_t i = 0; i < yolo_msg->yolo_objects.size(); ++i){
        const auto& Y = yolo_msg->yolo_objects[i];
        int id    = Y.id;
        int x_min = Y.x1, y_min = Y.y1, x_max = Y.x2, y_max = Y.y2;
        if (x_max - x_min < 30) continue;

        double min_dist = 1e3;
        int best_idx = -1;
        for (size_t j = 0; j < projected_list.size(); ++j){
            double u = projected_list[j].x, v = projected_list[j].y;
            if (u < x_min+10 || u > x_max-10 ||
                v < y_min+10 || v > y_max-10)
                continue;
            if (distance_list[j] < min_dist){
                min_dist = distance_list[j];
                best_idx = j;
            }
        }
        if (best_idx < 0) continue;

        detect_msgs::detected_object obj;
        obj.id = id;
        obj.world_point.position.x = lidar_points[best_idx].x;
        obj.world_point.position.y = lidar_points[best_idx].y;
        obj.world_point.position.z = lidar_points[best_idx].z;
        obj.world_point.orientation.x = 0.0;
        obj.world_point.orientation.y = 0.0;
        obj.world_point.orientation.z = 0.0;
        obj.world_point.orientation.w = 1.0;
        out.objects.push_back(obj);
    }

    pub_detected.publish(out);

    // 기존 센트로이드 퍼블리싱 로직 그대로…
    std::vector<object_struct> object_struct_list;
    for (int i = 0; i < (int)yolo_msg->yolo_objects.size(); ++i){
        int x_min = yolo_msg->yolo_objects[i].x1;
        int y_min = yolo_msg->yolo_objects[i].y1;
        int x_max = yolo_msg->yolo_objects[i].x2;
        int y_max = yolo_msg->yolo_objects[i].y2;
        double min_distance = 1e3;
        if (x_max - x_min < 30) continue;
        object_struct obj_info = {999};
        for (size_t j = 0; j < projected_list.size(); ++j){
            double u = projected_list[j].x, v = projected_list[j].y;
            if (u < x_min+10 || u > x_max-10 ||
                v < y_min+10 || v > y_max-10)
                continue;
            double d = distance_list[j];
            if (d < min_distance) min_distance = d;
        }
        for (size_t j = 0; j < projected_list.size(); ++j){
            double u = projected_list[j].x, v = projected_list[j].y;
            if (u < x_min+10 || u > x_max-10 ||
                v < y_min+10 || v > y_max-10)
                continue;
            double d = distance_list[j];
            if (fabs(min_distance - d) > 1.5) continue;
            obj_info.x = lidar_points[j].x;
            obj_info.y = lidar_points[j].y;
            obj_info.z = lidar_points[j].z;
            object_struct_list.push_back(obj_info);
        }
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    for (auto& o : object_struct_list){
        pcl::PointXYZ p; p.x=o.x; p.y=o.y; p.z=o.z;
        cloud->points.push_back(p);
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered = ground_filter(*cloud);

    pcl::search::KdTree<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    std::vector<pcl::PointIndices> cluster_indices;
    if (!filtered->empty()){
        tree->setInputCloud(filtered);
        pcl::EuclideanClusterExtraction<pcl::PointXYZ> ec;
        ec.setClusterTolerance(cluster_tolerance);
        ec.setMinClusterSize(cluster_min);
        ec.setMaxClusterSize(cluster_max);
        ec.setSearchMethod(tree);
        ec.setInputCloud(filtered);
        ec.extract(cluster_indices);
    }

    pcl::PointCloud<pcl::PointXYZI> centeroid_list;
    std::vector<cv::Point3d> centeroid_list_cv;
    int intensity = 0;
    for (auto& cl : cluster_indices){
        pcl::PointXYZI cen; cv::Point3d cen_cv; int cnt=0;
        for (auto& idx : cl.indices){
            auto& pt = filtered->points[idx];
            cen.x += pt.x; cen.y += pt.y; cen.z += pt.z; ++cnt;
        }
        cen.intensity = static_cast<float>(intensity+1);
        cen.x /= cnt; cen.y /= cnt; cen.z /= cnt;
        centeroid_list.push_back(cen);
        cen_cv.x=cen.x; cen_cv.y=cen.y; cen_cv.z=cen.z;
        centeroid_list_cv.push_back(cen_cv);
        ++intensity;
    }

    sensor_msgs::PointCloud2 cent_msg;
    pcl::toROSMsg(centeroid_list, cent_msg);
    cent_msg.header.frame_id = frame_name;
    cloud_centeroid.publish(cent_msg);
}

void Object_Detection::read_projection_matrix(){
    double fx = 1.8555e+03, fy = 1.8549e+03;
    double cx = 950.6236, cy = 575.6943;
    cv::Mat camera_matrix = (cv::Mat_<double>(3,3)<<
        fx,0,cx, 0,fy,cy, 0,0,1);
    cv::Mat T = (cv::Mat_<double>(3,4)<<
        -0.0259,-0.9994,0.0212,0.0269,
        -0.0177,-0.0207,-0.9996,-0.1107,
         0.9995,-0.0263,-0.0172,-0.0358,
         0.0,0.0,0.0,1.0);
    projection_matrix = camera_matrix * T;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr Object_Detection::ground_filter(
    pcl::PointCloud<pcl::PointXYZ> cloud){
    double height_thresh=0.0; int grid_dim=320; double per_cell=0.2;
    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(
        new pcl::PointCloud<pcl::PointXYZ>);
    filtered->points.resize(cloud.size());
    float min[320][320], max[320][320]; bool init[320][320];
    size_t cnt=0;
    memset(min,0,sizeof(min)); memset(max,0,sizeof(max));
    memset(init,false,sizeof(init));
    for (size_t i=0;i<cloud.size();++i){
        int x=int(grid_dim/2+cloud[i].x/per_cell);
        int y=int(grid_dim/2+cloud[i].y/per_cell);
        if (x>=0&&x<grid_dim&&y>=0&&y<grid_dim){
            if(!init[x][y]){min[x][y]=max[x][y]=cloud[i].z; init[x][y]=true;}
            else {min[x][y]=std::min(min[x][y],cloud[i].z);
                  max[x][y]=std::max(max[x][y],cloud[i].z);}
        }
    }
    double offset=grid_dim/2.0*per_cell;
    for(int xi=0;xi<grid_dim;++xi) for(int yi=0;yi<grid_dim;++yi){
        if(init[xi][yi]&&(max[xi][yi]-min[xi][yi])>height_thresh){
            pcl::PointXYZ p; p.x=-offset+(xi*per_cell+per_cell/2.0);
            p.y=-offset+(yi*per_cell+per_cell/2.0); p.z=-0.4;
            filtered->points.push_back(p); ++cnt;
        }
    }
    filtered->points.resize(cnt);
    return filtered;
}
