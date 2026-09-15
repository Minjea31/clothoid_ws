# Clothoid-R Perception — YOLO26 / TensorRT

Clothoid-R 자율주행 perception 스택 중 **카메라 YOLO 객체 검출** 파트를 정리한 저장소입니다.
브랜치 `yolo26_version` 기준으로, 카메라 검출 노드가 **커스텀 YOLO26(Ultralytics 포크) + TensorRT 엔진** 으로 동작합니다.

## 저장소 구성

```
clothoid_ws/
├─ perception_ws/                     # ROS Noetic catkin 워크스페이스
│  ├─ src/
│  │  ├─ detect_msgs/                 # 공통 perception 메시지 (Objects, Yolo_Objects, ...)
│  │  ├─ yolo26/                      # 카메라 YOLO ROS 패키지 (핵심 패키지)
│  │  │  ├─ scripts/yolo_detect.py    #   카메라 검출 노드
│  │  │  └─ models/best.engine        #   TensorRT 엔진 (정적 입력 1x3x640x640)
│  │  ├─ livox_camera_fusion/         # LiDAR-카메라 융합 (C++)
│  │  └─ perception_bringup/          # 통합 launch
│  └─ yolo26/                         # ROS 노드가 import 하는 커스텀 Ultralytics 포크
├─ save_images.py                     # ROS2(rclpy) 이미지 프레임 추출 유틸 (독립 실행)
└─ README.md
```

> `perception_bringup/perception.launch` 는 `livox_clustering` 패키지를 참조하지만 이 체크아웃에는 포함돼 있지 않습니다. 여기서는 `yolo26` 노드 + `livox_camera_fusion` 위주로 봅니다.

## 파이프라인

```
/camera/image_raw/compressed ──▶ yolo_detect.py ──▶ /perception/camera/yolo   (detect_msgs/Yolo_Objects)
                                                         └──▶ livox_camera_fusion 구독 → /perception/fusion/centroids
```

---

## 이번 브랜치에서 적용한 것

### 1. 카메라 추론을 PyTorch(.pt/.yaml) → TensorRT 엔진으로 교체

`perception_ws/src/yolo26/scripts/yolo_detect.py`

| | 이전 | 현재 |
|---|---|---|
| 모델 로딩 | `YOLO(best.yaml, task="detect").load(best.pt)` | `YOLO(best.engine, task="detect")` |
| 추론 입력 | 원본 해상도 `imgsz=(h0, w0)` (1920×1080) | `imgsz=640` 고정 |
| ROS 파라미터 | `~yaml_cfg`, `~pt_weights` | `~engine_weights`, `~imgsz` |
| 모델 파일 | `models/best.pt`, `models/best.yaml` | `models/best.engine` |

- `best.engine` 은 정적 입력 `[1, 3, 640, 640]` 으로 빌드돼 있어 추론 `imgsz` 는 **반드시 640** 이어야 합니다 (학습 `imgsz=640` 과 동일).
- 원본 1920×1080 프레임은 Ultralytics `LetterBox` 가 비율 유지 + 패딩으로 640×640 에 맞추고, `results.boxes.xyxy` 는 다시 원본 해상도 좌표로 복원돼 나옵니다.
- 후처리 로직은 그대로: 클래스별 confidence 필터 → area-aware NMS → containment(포함 관계) suppression.
- 초기화 로그 문구는 `YOLOv12 MODEL LOADED` 로 남아 있지만 실제로는 YOLO26 엔진입니다 (표기만 옛날 것).

### 2. YOLO26 포크 `requirements.txt` 정리

`perception_ws/yolo26/requirements.txt`

- `from ultralytics import YOLO` 에 필요한 런타임 의존성 명시 (matplotlib, pillow, pandas, seaborn, ultralytics-thop 등)
- `tensorrt-cu12==10.7.0` 추가 — ONNX → TensorRT 엔진 빌드용 (CUDA 12 기준, 다른 CUDA 면 교체)
- `graphviz` 추가 — 모델 구조 시각화용 (시스템 `dot` 는 `apt` 로 별도 설치)
- 맨 끝 `-e .` — 이 포크를 `ultralytics` 패키지로 editable 설치
- fresh machine 셋업 순서를 파일 상단 주석에 정리

### 3. Ultralytics 포크에 pruned 모델 파싱 지원

`nn/modules/{block,conv,head}.py`, `nn/tasks.py` 에 `recon()` / `exact_channels` 처리 추가 — 학습·프루닝 저장소에서 나온 pruned yaml/weight 를 그대로 로딩할 수 있게 맞춤.

### 4. 패키지 리네이밍

브랜치 레벨에서 기존 `yolov12/` 벤더 트리를 제거하고 `perception_ws/yolo26/` (YOLO26 포크) 로 교체, ROS 패키지명도 `yolov12` → `yolo26`. 실행은 `rosrun yolo26 yolo_detect.py`.

---

## 요구 환경

| 항목 | 버전 / 비고 |
|---|---|
| OS | Ubuntu 20.04 |
| ROS | Noetic |
| Python | 3.10 (conda env, 이름 `yolo`) |
| GPU | NVIDIA GPU + 드라이버, CUDA 12 |
| TensorRT | 10.7 (`tensorrt-cu12==10.7.0`) — 엔진 빌드·실행에 필요 |

### 하드코딩 경로 주의

`scripts/yolo_detect.py` 첫 두 줄이 특정 PC 기준으로 고정돼 있습니다. 다른 환경이면 반드시 수정:

```python
#!/home/a/anaconda3/envs/yolo/bin/python                        # ← conda env yolo 의 python 절대경로
sys.path.insert(0, "/home/a/Clothoid-R/perception_ws/yolo26")   # ← YOLO26 포크 절대경로
```

- shebang 을 그대로 쓰려면 conda 환경 이름을 `yolo` 로 만드는 게 가장 편합니다. 이름이 다르면 첫 줄을 새 경로로 바꿔야 합니다.
- 이 저장소 위치가 `~/Clothoid-R` 와 다르더라도 `sys.path` 는 `~/Clothoid-R` 를 가리키므로, 실제 사용하는 포크 위치에 맞춰 수정하세요.

---

## 환경 설정

### 1. 시스템 / ROS 패키지

```bash
source /opt/ros/noetic/setup.bash
sudo apt update
sudo apt install -y build-essential cmake python3-pip python3-rosdep \
  ros-noetic-cv-bridge ros-noetic-pcl-ros ros-noetic-pcl-conversions \
  ros-noetic-message-filters ros-noetic-dynamic-reconfigure
sudo apt install -y graphviz          # 모델 구조 시각화용 (선택)
```

### 2. conda 환경 + YOLO26 포크

```bash
conda create -n yolo python=3.10 -y
conda activate yolo
python -m pip install --upgrade pip setuptools wheel

cd perception_ws/yolo26
pip install -r requirements.txt        # 런타임 의존성 + 마지막 줄 -e . (포크 설치)

# ROS <-> conda python 연동
pip install rospkg catkin_pkg empy pyyaml
```

> 장비 CUDA 와 설치된 PyTorch wheel 이 안 맞으면 GPU 인식이 안 될 수 있습니다. 그럴 땐 장비 CUDA 에 맞는 `torch` / `torchvision` 으로 재설치하세요.

### 3. catkin 빌드

```bash
source /opt/ros/noetic/setup.bash
cd perception_ws
catkin_make
source devel/setup.bash
```

---

## 실행

터미널마다 순서: `source /opt/ros/noetic/setup.bash` → `source perception_ws/devel/setup.bash` → `conda activate yolo`

### 카메라 YOLO 노드 단독

```bash
roscore                                    # (별도 터미널, 마스터가 없을 때)
rosrun yolo26 yolo_detect.py
```

센서 입력은 보통 `rosbag play <bag>` 로 재생하면서 확인합니다.

파라미터 지정 예:

```bash
rosrun yolo26 yolo_detect.py \
  _source:=/camera/image_raw/compressed \
  _output_topic:=/perception/camera/yolo \
  _imgsz:=640 \
  _engine_weights:=$(rospack find yolo26)/models/best.engine \
  _erp42_confidence:=0.5
```

### 융합까지 같이

```bash
roslaunch livox_camera_fusion livox_camera_fusion.launch    # LiDAR-카메라 융합
rosrun   yolo26 yolo_detect.py                              # 카메라 검출
```

### 확인

```bash
rosnode list                            # /yolo_detect_node 존재 확인
rostopic echo /perception/camera/yolo   # 검출 결과 메시지
rostopic hz   /perception/camera/yolo   # 발행 주기
```

`yolo_detect.py` 상단의 `SHOW_DETECTION_IMAGE = True` 이면 `YOLO BBox` 창에 bbox 가 표시됩니다. 헤드리스 환경이면 `False` 로 두세요.

---

## 주요 파라미터 (`yolo_detect.py`)

| 파라미터 | 기본값 | 설명 |
|---|---|---|
| `~source` | `/camera/image_raw/compressed` | 입력 이미지 토픽 (`CompressedImage`) |
| `~output_topic` | `/perception/camera/yolo` | 출력 토픽 (`detect_msgs/Yolo_Objects`) |
| `~engine_weights` | `<pkg>/models/best.engine` | TensorRT 엔진 경로 |
| `~imgsz` | `640` | 추론 입력 크기 (엔진 정적 입력과 일치해야 함) |
| `~frame_id` | `camera_link` | 입력 헤더에 frame_id 가 없을 때 사용 |
| `~erp42_confidence` / `~drum_confidence` / `~cone_confidence` | `0.5` | 클래스별 최소 confidence |
| `~postprocess_nms_iou` | `0.5` | area-aware NMS IoU 임계 |
| `~postprocess_containment_ioa` | `0.8` | 포함 관계 억제 임계 |
| `~postprocess_max_det` | `0` (무제한) | 프레임당 최대 검출 수 |

### 클래스 설정 (`DEFAULT_CLASS_CONFIG`)

| id | 이름 | publish |
|---|---|---|
| 0 | ERP-42 | ✅ |
| 1 | drum | ❌ |
| 2 | cone | ❌ |

`publish: True` 인 클래스만 `/perception/camera/yolo` 로 발행합니다. 검출이 없어도 빈 `Yolo_Objects` 는 매 프레임 발행됩니다.

### 출력 메시지 (`detect_msgs`)

```
Yolo_Objects
├─ std_msgs/Header header          # stamp 는 입력 이미지 stamp 그대로
└─ Objects[] yolo_objects
   └─ Objects { int32 Class, int32 id, int32 x1, int32 x2, int32 y1, int32 y2 }   # bbox 픽셀 좌표
```

---

## `best.engine` 재생성 (참고)

TensorRT 엔진은 GPU / TensorRT 버전에 종속적이라 장비가 바뀌면 다시 빌드해야 합니다. 학습·프루닝 저장소의 `.pt` 에서:

```bash
conda activate yolo
yolo export model=best.pt format=engine imgsz=640 half=True
# 또는 포크의 build_trt_engine.py (ONNX -> TensorRT) 사용
```

빌드된 `best.engine` 을 `perception_ws/src/yolo26/models/` 에 둡니다.
`best.pt` / `best.yaml` 원본은 이 저장소에 포함하지 않고 학습·프루닝 저장소에서 가져옵니다.

---

## 부록: `save_images.py`

ROS2(`rclpy`) 로 이미지 토픽을 받아 프레임을 파일로 저장하는 독립 유틸입니다. 위 ROS1 워크스페이스와는 무관합니다.

```bash
python3 save_images.py --topic-name /car1/camera/image_raw --save-dir ./extracted_images --save-interval 0.1
```
