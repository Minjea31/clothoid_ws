# 환경 세팅

### 만약 ultralystics 가 깔려있다면 삭제해야함

    pip uninstall ultralystics

### conda를 사용해 가상환경을 만듦

    cd clothoid_ws

    conda env create -f environment.yml

    conda activate clothoid
    
    cd yolo-V8
    
    pip install -e .
    
    pip install torch torchvison pyyaml
    
    source /opt/ros/noetic/setup.bash

# 실행 

    catkin_make
    
    source devel/setup.bash
    
### 터미널 3개 띄워서

    source /opt/ros/noetic/setup.bash
    
### 1번 터미널
    
    roscore
    
### 2번 터미널 (yolo)
    
    roslaunch yolov8_ws yolo_detect.launch 

### 3번 터미널 (detect)
    
    roslaunch detect_ws detect.launch 
