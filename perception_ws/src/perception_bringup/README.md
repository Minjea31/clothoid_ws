# perception_bringup

YOLO26 카메라 검출과 Livox-camera fusion을 실행합니다.

```bash
roslaunch perception_bringup perception.launch yolo_python:="$(command -v python)"
```

`/yolo_detect_node`는 지정한 conda Python, `/livox_camera_fusion`은 시스템 C++ ROS로 실행합니다.

| 인자 | 기본값 |
|---|---|
| `run_yolo` | true, false이면 fusion만 실행 |
| `yolo_python` | python3 |
| `engine_weights` | `$(find yolo26)/models/small_fp16.engine` |
| `imgsz` | 640 |
| `show_detection_image`, `print_detections` | false |
| `livox_lidar_topic` | `/livox/lidar` |
| `camera_image_topic` | `/camera/image_raw/compressed` |
| `camera_yolo_topic` | `/perception/camera/yolo` |
| `fusion_centroid_topic` | `/perception/fusion/centroids` |
| `fusion_projection_config` | fusion의 `config/projection.yaml` |

```bash
rviz -d "$(rospack find perception_bringup)/rviz/perception.rviz"
```

RViz의 fixed frame과 TF는 장비에 맞춥니다. 검증할 토픽은 fusion의 centroid와 filtered cloud입니다.
