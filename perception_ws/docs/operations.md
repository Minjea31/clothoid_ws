# 실행과 bag 검증

## 기존 Docker

컨테이너는 `clothoid_ws`, base image는 `osrf/ros:noetic-desktop-full`입니다. 호스트 `/home/a/clothoid_ws`가 `/root/clothoid_ws`로 mount되고 GPU가 연결됩니다. 설치가 추가된 기존 컨테이너에서 검증했으며 이 저장소에는 그 전체 환경을 재생성하는 Dockerfile이 없습니다.

```bash
docker start clothoid_ws
docker exec -it clothoid_ws bash
cd /root/clothoid_ws/perception_ws
source /opt/ros/noetic/setup.bash
source devel/setup.bash
conda activate yolotrt
nvidia-smi
python -c 'import torch; print(torch.cuda.is_available())'
```

셸에 conda가 초기화되지 않았다면 설치된 conda의 `etc/profile.d/conda.sh`를 source합니다. 설치 위치는 `conda info --base`로 확인합니다.

## Bag과 센서

```bash
rosbag info /path/to/record.bag
```

카메라는 `sensor_msgs/CompressedImage`, Livox는 `sensor_msgs/PointCloud2`여야 합니다. Livox `CustomMsg`는 fusion에서 직접 받지 않습니다. 카메라+Livox와 ERP-42·cone 영상이 함께 있는 bag을 우선 보관하고 sparse sample 미검출만으로 bag 삭제를 결정하지 않습니다.

1. `roscore`를 실행합니다.
2. `rosparam set /use_sim_time true` 후 perception을 시작합니다.
3. 다른 터미널에서 `rosbag play --clock`으로 센서 토픽만 재생합니다.
4. bbox·centroid 출력 주파수, stamp, frame_id를 확인합니다.

```bash
roslaunch perception_bringup perception.launch yolo_python:="$(command -v python)"
rosbag play --clock /path/to/record.bag \
  --topics /camera/image_raw/compressed /livox/lidar
rostopic hz /perception/camera/yolo
rostopic hz /perception/fusion/centroids
```

저장된 YOLO 결과로 fusion만 검사하려면 `run_yolo:=false`로 실행하고 `/perception/camera/yolo`도 재생합니다.

## 문제 확인

| 현상 | 확인 |
|---|---|
| `detect_msgs` import 실패 | catkin build 후 `source devel/setup.bash` |
| torch/TRT import 실패 | conda 확인, `yolo_python:="$(command -v python)"` |
| engine load/shape 실패 | 대상 GPU에서 재빌드, TRT 버전·입력 640 확인 |
| bbox만 있고 fusion 없음 | 3개 header stamp 간격 50ms, LiDAR 타입, 캘리브레이션 |
| 일부 centroid 없음 | 최소 3포인트, ROI 반경 10px, cluster/AABB gate |
| GUI 연결 실패 | `show_detection_image:=false` |

카메라 subscriber/publisher queue는 1입니다. 고속 입력에서는 모든 프레임보다 최신 프레임 중심으로 동작할 수 있습니다. 모든 프레임을 순서대로 검사할 때는 bag을 직접 읽는 영상 도구·벤치마크를 사용합니다.
