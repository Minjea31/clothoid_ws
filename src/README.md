# 2026 Camera Perception Packages

이 저장소는 `ROS Noetic + catkin` 환경에서 사용되는 카메라/LiDAR 기반 객체 인식 패키지 모음입니다.  
구성은 크게 다음 3개 패키지로 나뉩니다.

- `detect_msgs`: YOLO 검출 결과를 전달하기 위한 커스텀 메시지 패키지
- `yolov12`: 카메라 영상을 입력으로 사용하는 YOLOv12 검출 노드
- `Object_detect`: LiDAR와 카메라 검출 결과를 융합하거나, LiDAR 단독 검출을 수행하는 패키지

이 문서는 현재 저장소에 포함된 코드 기준으로 패키지 역할, 주요 토픽, 빌드/실행 방법을 정리한 것입니다.

## 1. 작업 개요

이 저장소의 주요 목적은 다음 두 흐름을 지원하는 것입니다.

- 카메라 기반 YOLO 객체 검출
- LiDAR 포인트클라우드와 카메라 2D 검출 결과를 결합한 객체 위치 추정 및 추적

추가로 `Object_detect` 패키지에는 LiDAR만 사용해 객체를 검출하는 보조 스크립트도 포함되어 있습니다.

## 2. 패키지 구성

### `detect_msgs`

YOLO 검출 결과를 ROS 메시지로 주고받기 위한 커스텀 메시지 패키지입니다.

### `yolov12`

압축 카메라 이미지를 입력받아 YOLOv12로 객체를 검출하는 패키지입니다.

특징:

- 입력 토픽 기본값: `/camera/image_raw/compressed`
- 출력 토픽 기본값: `/yolo`
- publish할 클래스는 스크립트 내부 설정값으로 선택 가능

### `Object_detect`

카메라 YOLO 검출 결과와 LiDAR 포인트클라우드를 동기화하여 3D 위치를 추정하는 C++ 노드와, LiDAR 단독 기반 보조 검출 스크립트를 포함합니다.

## 3. 처리 파이프라인

### A. 카메라 + LiDAR 융합 파이프라인

1. 카메라 압축 이미지가 입력됩니다.
2. `yolov12` 노드가 2D bounding box를 검출합니다.
3. `Object_detect`의 `object_detection_node`가 아래 3개 입력을 ApproximateTime 방식으로 동기화합니다.
   - LiDAR: `sensor_msgs/PointCloud2`
   - Camera: `sensor_msgs/CompressedImage`
   - YOLO: `detect_msgs/Yolo_Objects`
4. LiDAR 포인트를 카메라 평면으로 투영합니다.
5. 각 2D bbox 내부에 들어오는 포인트를 모읍니다.
6. ROI 추출, 간단한 지면 제거, Euclidean clustering을 적용합니다.
7. 가장 큰 클러스터의 중심을 계산하고 Kalman 기반으로 추적합니다.
8. 결과를 point cloud 토픽으로 publish 합니다.

출력 토픽:

- `/minjae`: 추적된 centroid를 `sensor_msgs/PointCloud`로 publish
- `/cloud_fillter`: 필터링된 3D 포인트를 `sensor_msgs/PointCloud2`로 publish

### B. LiDAR 단독 처리 파이프라인

`Object_detect/scripts/euclidean_clustering.py`

- `/livox/lidar`를 입력으로 사용
- ROI 제한, voxel downsampling, DROR, 지면 제거, 거리 기반 Euclidean clustering 적용
- centroid를 `/jagyeong` 토픽으로 publish
- 전처리된 포인트를 `/preprocessed_points`로 publish

`Object_detect/scripts/velodyne_euclidean_clustering.py`

- `velodyne` 계열 포인트클라우드를 대상으로 유사한 전처리/클러스터링 수행

`Object_detect/scripts/bev_object_detector.py`

- `/velodyne_points`를 입력으로 받아 BEV 이미지 생성
- YOLO + OC-SORT 기반 추적 수행
- 출력:
  - `/detected_objects`
  - `/visualization_marker_array`
  - `/bev_detection_image`

## 4. 주요 토픽 정리

### 공통 입력

- `/camera/image_raw/compressed`
- `/livox/lidar`
- `/velodyne_points`

### YOLO 출력

- `yolov12`: `/yolo`

## 5. 한 줄 요약

이 저장소는 카메라 기반 YOLO 검출과 LiDAR 기반 객체 위치 추정을 결합해, ROS 환경에서 객체 인식 및 추적을 수행하기 위한 패키지 모음입니다.
