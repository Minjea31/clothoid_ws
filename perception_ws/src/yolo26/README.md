# yolo26

`CompressedImage`를 YOLO26s로 추론하고 원본 이미지 좌표 bbox를 발행하는 ROS1 Python 패키지입니다. 기본 engine은 `models/small_fp16.engine`, 원본은 `models/small.pt`입니다.

```bash
source /opt/ros/noetic/setup.bash
source devel/setup.bash
conda activate yolotrt
roslaunch yolo26 yolo_detect.launch python:="$(command -v python)"
# 직접 실행
rosrun yolo26 yolo_detect.py _show_detection_image:=false _print_detections:=false
```

사용자 홈의 고정 경로 대신 workspace의 `yolo26/ultralytics`를 우선 import합니다.

## ROS private parameters

| 이름 | 기본값 | 의미 |
|---|---|---|
| `~source` | `/camera/image_raw/compressed` | 입력 |
| `~output_topic` | `/perception/camera/yolo` | 출력 |
| `~engine_weights` | `models/small_fp16.engine` | engine 또는 pt 경로 |
| `~imgsz` | 640 | 정적 engine 입력과 일치 필요 |
| `~frame_id` | `camera_link` | 입력 frame_id가 비었을 때 |
| `~show_detection_image` | 직접 실행 true / launch false | GUI |
| `~print_detections` | 직접 실행 true / launch false | 프레임별 로그 |
| `~erp42_confidence`, `~drum_confidence`, `~cone_confidence` | 0.5 | 진입 threshold |
| `~erp42_keep_confidence`, `~drum_keep_confidence`, `~cone_keep_confidence` | 0.3 | 유지 threshold |
| `~postprocess_nms_iou` | 0.5 | 클래스별 추가 NMS |
| `~postprocess_containment_ioa` | 0.8 | 포함된 작은 bbox 억제 |
| `~postprocess_area_weight` | 0.1 | NMS 점수에 정규화 면적 가산 |
| `~postprocess_max_det` | 0 | 추가 후처리 개수 제한, 0은 제한 없음 |
| `~postprocess_debug` | false | raw/최종 bbox 수 로그 |

유지 매칭 IoU 0.3, publish 여부·클래스 이름은 `DEFAULT_CLASS_CONFIG`에서 설정합니다. 현재 세 클래스 모두 publish합니다. 추가 후처리 제한이 0이어도 모델 출력 자체에는 300개 후보 한도가 있습니다.

JPEG decode → LetterBox 640×640 → 추론 → 진입/유지 confidence → 면적 점수 NMS → containment 억제 → `Yolo_Objects` 발행입니다. stamp는 입력 이미지를 유지하고 빈 검출 배열도 발행합니다. `id`는 프레임 내 순번이며 confidence·지속 tracking ID는 메시지에 없습니다. 히스테리시스는 바로 이전 프레임만 참조합니다.

[전체 실행](../../README.md), [TensorRT](../../docs/tensorrt.md), [성능](../../docs/performance.md).
