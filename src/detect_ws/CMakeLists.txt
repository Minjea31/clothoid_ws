cmake_minimum_required(VERSION 3.0.2)
project(detect_ws)

## Compile as C++14
add_compile_options(-std=c++14)

## Find catkin macros and libraries
find_package(catkin REQUIRED COMPONENTS
  roscpp
  sensor_msgs
  std_msgs
  geometry_msgs
  visualization_msgs
  cv_bridge
  image_transport
  pcl_ros
  message_filters
  detect_msgs          # ✅ 추가
  message_generation   # ✅ 추가
)

## Find external libraries
find_package(OpenCV REQUIRED)
find_package(PCL REQUIRED)

###################################
## catkin specific configuration ##
###################################
catkin_package(
  INCLUDE_DIRS include
  CATKIN_DEPENDS
    roscpp
    sensor_msgs
    std_msgs
    geometry_msgs
    visualization_msgs
    cv_bridge
    image_transport
    pcl_ros
    message_filters
    detect_msgs        # ✅ 추가
    message_runtime    # ✅ 추가
)

###########
## Build ##
###########
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}
  ${PCL_INCLUDE_DIRS}
)

## Declare C++ executable
add_executable(object_detection_node
  src/main.cpp
  src/detection.cpp
)

## 메시지 의존성 보장
add_dependencies(object_detection_node
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

## Link libraries
target_link_libraries(object_detection_node
  ${catkin_LIBRARIES}
  ${OpenCV_LIBS}
  ${PCL_LIBRARIES}
)

#############
## Install ##
#############
install(TARGETS object_detection_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY launch
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)

