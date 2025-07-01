cmake_minimum_required(VERSION 3.0.2)
project(detect_ws)

## Compile as C++14 (PCL 요구)
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
  detect_msgs
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
    detect_msgs
    message_runtime
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

## 메시지 생성 후 이 노드를 빌드하도록 순서 보장
add_dependencies(object_detection_node
  ${${PROJECT_NAME}_EXPORTED_TARGETS}
  ${catkin_EXPORTED_TARGETS}
)

## Specify libraries to link a library or executable target against
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

