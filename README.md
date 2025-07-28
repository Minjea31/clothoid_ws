# 환경 세팅

### 만약 ultralystics 가 깔려있다면 삭제해야함

    pip uninstall ultralystics

### conda를 사용해 가상환경을 만듦

    cd clothoid_ws

    conda env create -f environment.yml

    conda activate clothoid
    
    cd yolov8-prune
    
    pip install -e .
    
    pip install torch torchvision pyyaml
    
    source /opt/ros/noetic/setup.bash

# 실행 

    catkin_make
    
    source devel/setup.bash
    
### 터미널 3개 띄워서

    source /opt/ros/noetic/setup.bash
    
### 1번 터미널
    
    roscore
    
### 2번 터미널 (yolo)

    conda activate clothoid
    
    roslaunch yolov8_ws yolo_detect.launch 

### 3번 터미널 (detect)
    
    roslaunch detect_ws detect.launch 

---
## iDAR와 카메라 융합 기반 객체 탐지 및 추적

이 패키지는 yolov8에서 detect한 bbox와 Livox Lidar와 클러스터링을 통해 객체의 (x,y)를 pub을 한다.

### 파이프 라인은 다음과 같다

    ROS Topic 동기화(LiDAR+카메라+YOLO)
    ↓
    LiDAR 포인트를 카메라 좌표계로 투영
    ↓
    YOLO bbox와 매칭된 LiDAR 포인트 필터링
    ↓
    ROI 추출 및 RANSAC 기반 Ground 제거
    ↓
    Point Cloud 군집화 및 중심점(Centroid) 계산
    ↓
    Kalman Filter 기반 객체 추적(Tracking)
    ↓
    결과 시각화 및 PointCloud 발행

