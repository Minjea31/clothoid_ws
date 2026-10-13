# yolo_video — rosbag → YOLO bbox mp4

rosbag 의 카메라 영상(`/camera/image_raw/compressed`)에 **yolo26 노드(`src/yolo26/scripts/yolo_detect.py`)와 동일한 검출 로직**으로 bbox + confidence 를 그려 mp4 로 저장합니다.

- 모델, 클래스별 confidence / keep_confidence(히스테리시스), NMS·containment 후처리 설정을 노드에서 그대로 import 하므로 **노드 설정을 바꾸면 영상에도 자동 반영**됩니다.
- `rosbag play` 없이 bag 을 직접 읽으므로 roscore 가 필요 없고, 카메라 메시지를 순서대로 처리합니다.
- 기본 출력 fps는 카메라 토픽의 frequency 추정값을 사용합니다. 카메라가 일부 구간에만 기록됐거나 메시지 간격이 불규칙하면 영상 길이가 bag 전체 길이와 다를 수 있습니다.

## 파일

| 파일 | 설명 |
|---|---|
| `run.sh` | 실행 스크립트 (ROS/devel source + 활성 conda 또는 `yolotrt` 선택) |
| `yolo_bag_to_video.py` | 본체 |
| `output/` | 기본 출력 폴더 (자동 생성) |

## 사용법

컨테이너 안이든 호스트든 `run.sh` 로 실행하면 됩니다. 호스트에서 실행하면 자동으로 `clothoid_ws` 컨테이너 안에서 다시 실행합니다(컨테이너가 켜져 있어야 함).

```bash
cd ~/clothoid_ws/perception_ws/yolo_video     # 컨테이너 안: /root/clothoid_ws/perception_ws/yolo_video
./run.sh
```

### 1) bag 을 목록에서 선택 (인자 없이 실행)

```
$ ./run.sh

[/root/clothoid_ws/bag] 아래 bag 목록
    1) 2026_3차_대회/bag0920/2026-09-20-16-55-35.bag  (9.9 GB)
    2) 2026_3차_대회/bag0920/2026-09-20-16-57-33.bag  (0.5 GB)
  ...
변환할 bag 번호 (예: 3 / 1,4 / 2-5 / all, q=종료):
```

| 입력 | 의미 |
|---|---|
| `3` | 3번 하나 |
| `1,4` 또는 `1 4` | 1번, 4번 |
| `2-5` | 2~5번 |
| `all` | 전체 |
| `q` | 종료 |

다른 폴더의 bag 목록을 보려면 `--bag-root` 를 지정합니다.

```bash
./run.sh --bag-root ~/clothoid_ws/bag/2026_3차_대회/bag0929
```

### 2) bag 경로를 직접 지정

```bash
# 파일 하나
./run.sh --bag ~/clothoid_ws/bag/2026_3차_대회/bag0927/20260927_162055/record.bag

# 여러 개 / 폴더 (폴더 안의 .bag 전부)
./run.sh --bag a.bag b.bag
./run.sh --bag ~/clothoid_ws/bag/2026_3차_대회/bag0929
```

### 자주 쓰는 예시

```bash
# 10초~40초 구간만
./run.sh --bag record.bag --start 10 --end 40

# 콘이 촘촘해서 라벨이 겹칠 때: confidence 숫자만 표시
./run.sh --label conf

# 용량 줄이기: 해상도 절반 (1920x1080 → 960x540)
./run.sh --scale 0.5

# 프레임별 검출 결과(클래스, confidence, bbox)를 터미널에도 출력
./run.sh --print-dets

# 처리하면서 화면으로 보기 (q 로 중단, X11 필요)
./run.sh --show

# 다른 모델 사용
./run.sh --engine ../src/yolo26/models/best.pt
```

## 옵션

| 옵션 | 기본값 | 설명 |
|---|---|---|
| `--bag PATH...` | (선택 목록) | bag 파일/폴더. 생략하면 목록에서 선택 |
| `--bag-root DIR` | `clothoid_ws/bag` | 선택 목록에 보여줄 bag 검색 폴더 |
| `--topic` | `/camera/image_raw/compressed` | CompressedImage 토픽 |
| `--out-dir DIR` | `yolo_video/output` | 출력 폴더 |
| `--out FILE` | | 출력 파일 경로 (bag 1개일 때만) |
| `--engine` | 노드와 동일 (`models/small_fp16.engine`) | 모델 (.engine / .pt / .onnx) |
| `--imgsz` | 640 | 추론 입력 크기 (engine 은 빌드한 크기와 같아야 함) |
| `--fps` | bag 카메라 fps | 출력 fps |
| `--start` / `--end` | 처음 / 끝 | bag 시작 기준 초 단위 구간 |
| `--scale` | 1.0 | 출력 해상도 배율 |
| `--thickness` | 1 | bbox 선 두께(px) |
| `--label` | `full` | `full`=`cone 0.87`, `conf`=`0.87`, `none`=라벨 없음 |
| `--print-dets` | off | 프레임별 검출 결과를 터미널 출력 |
| `--show` | off | 미리보기 창 표시 |

## 출력

- 파일명: bag 의 상대 경로를 `__` 로 이어 붙인 이름 + `_yolo.mp4`
  예) `bag0927/20260927_162055/record.bag` → `output/2026_3차_대회__bag0927__20260927_162055__record_yolo.mp4`
- 영상 좌상단: bag 시작 기준 시간(`t=`)과 해당 프레임 검출 개수(`det=`)
- bbox 색상: ERP-42 = 파랑, drum = 주황, cone = 노랑
- 코덱은 `mp4v` (MPEG-4 Part 2) 입니다. VLC / 일반 플레이어에서는 재생되지만 브라우저·슬랙 미리보기에서 안 나오면 H.264 로 변환하세요.
  ```bash
  ffmpeg -i in.mp4 -c:v libx264 -pix_fmt yuv420p out_h264.mp4
  ```
- 컨테이너(root)에서 만들어도 출력 파일 소유자는 출력 폴더 소유자로 맞춰져서 호스트에서 바로 지울 수 있습니다.
- 처리 중 `Ctrl+C` 로 중단해도 그때까지의 영상은 저장됩니다.

## 참고

- 처리 속도: MP4 인코딩과 drawing까지 포함하므로 모델 추론 FPS와 다릅니다. 같은 GPU의 디코딩+검출 실측은 43.48 FPS이며 인코딩은 제외한 값입니다. [측정 조건](../docs/performance.md)을 참고하세요.
- 검출 기준(클래스별 confidence 등)을 바꾸려면 `src/yolo26/scripts/yolo_detect.py` 의 `DEFAULT_CLASS_CONFIG` 를 수정하세요. 노드의 rosparam(`~cone_confidence` 등)은 이 스크립트에는 적용되지 않습니다.
- `yolotrt` conda 환경에는 `pycryptodomex` 가 없어서 ROS 의 `rosbag` import 가 실패하는데, 스크립트 안에서 빈 모듈로 대체해서 해결합니다(암호화되지 않은 bag 만 지원).

## conda 환경 선택

기존 conda를 사용합니다. 활성 환경이 없으면 `conda run -n yolotrt`로 실행합니다. 다른 환경은 `YOLO_CONDA_ENV=환경이름 ./run.sh`, 특정 인터프리터는 `YOLO_PYTHON=/path/to/env/bin/python ./run.sh`로 지정합니다. 호스트에서 지정한 Python 경로는 컨테이너 안에서 존재하는 경로여야 합니다.
