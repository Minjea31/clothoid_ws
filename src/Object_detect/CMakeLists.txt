cmake_minimum_required(VERSION 3.0.2)
project(Object_detect)

## Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
  message_generation
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

## Ensure message generation dependencies are built before the node
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

