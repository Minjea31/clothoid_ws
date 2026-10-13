# 성능 측정

## 현재 보관 중인 bag: 2026-10-14 측정

`bag1002/20261002_144630/record.bag`의 +46초부터 240프레임을 두 모델에서 같은 순서로 3회 처리했습니다. 현재 보관 중인 bag으로 재현할 수 있는 결과입니다.

![PyTorch와 TensorRT 실측](assets/performance.svg)

| 백엔드 | 추론 평균 | 추론 p95 | 디코딩+검출 평균 | 처리 p95 | 환산 FPS |
|---|---:|---:|---:|---:|---:|
| PyTorch FP32 | 7.63ms | 12.16ms | 28.52ms | 41.36ms | 35.06 |
| TensorRT FP16 | 2.00ms | 2.33ms | 23.00ms | 31.76ms | 43.48 |

추론 약 **3.82배**, 디코딩과 후처리를 포함한 처리량 약 **1.24배**이며 포함 구간의 평균 지연은 **19.4% 감소**했습니다. GPU/라이브러리/후처리 설정은 아래 이전 측정과 같고, 모델별 30회 워밍업과 CUDA 동기화를 적용했습니다. 각 모델 720건의 측정이며 배포 필터 이후 평균 검출은 둘 다 약 6.029개였습니다. 이는 정확도 검증을 대신하지 않습니다.

- [현재 bag 실측 JSON](../benchmarks/2026-10-14-rtx3070-small-current-bag.json)
- [프레임별 CSV](../benchmarks/2026-10-14-rtx3070-small-current-bag.csv)

```bash
source /opt/ros/noetic/setup.bash
source devel/setup.bash
conda activate yolotrt
python scripts/benchmark_bag.py \
  --bag ../bag/2026_3차_대회/bag1002/20261002_144630/record.bag \
  --start 46 --frames 240 --warmup 30 --repeats 3 \
  --models src/yolo26/models/small.pt src/yolo26/models/small_fp16.engine \
  --output benchmarks/runs/small-current-bag.json
```

## Archive 실행 검증

별도 `/tmp` 경로에 archive를 풀고 기존 build/devel 없이 catkin 빌드, ONNX export, TensorRT FP16 엔진 재생성을 확인했습니다. 다시 만든 엔진을 사용해 독립 ROS master에서 위 bag의 +32~36초 센서 입력을 0.3배속으로 재생한 결과, **YOLO 223개 메시지 모두 검출 포함**, **fusion 39개 메시지 중 3개가 유효 centroid 포함**을 확인했습니다. 출력 생성 여부를 검사한 것으로 정확도나 실시간 성능 시험은 아닙니다.

[ROS 실행 검증 JSON](../benchmarks/2026-10-14-archive-smoke.json). 재생성 엔진의 Inspector 레이어는 222개였고, 아래 성능 측정에 사용한 기존 엔진의 217개와 다릅니다. 엔진 빌드 결과는 환경과 tactic 선택에 따라 달라지므로 hash와 측정 대상을 구분합니다.

## 이전 측정 기록

아래 기록의 9월 20일·27일 원본 bag은 현재 폴더에 없습니다. 수치는 당시 측정 결과로 보존하고, 재현 명령은 위의 현재 bag을 사용합니다.

## 이전 측정: 9월 27일 주행 구간

2026-10-13 `clothoid_ws` 컨테이너의 RTX 3070 Laptop GPU에서 같은 YOLO26s 가중치·카메라 프레임·배포 후처리로 비교했습니다.

| 백엔드 | 추론 평균 | 추론 p95 | 디코딩+검출 처리 평균 | 처리 p95 | 평균 시간 환산 FPS |
|---|---:|---:|---:|---:|---:|
| PyTorch FP32, `small.pt` | 8.59ms | 12.38ms | 36.46ms | 55.98ms | 27.43 |
| TensorRT FP16, `small_fp16.engine` | 2.09ms | 3.59ms | 25.79ms | 30.77ms | 38.78 |

모델 추론은 약 **4.11배**, 디코딩과 후처리를 포함한 평균 처리량은 약 **1.41배**입니다. 포함 구간의 평균 지연은 약 **29.3% 감소**했습니다. JPEG 디코딩·전처리·Python bbox 처리 등이 남아서 전체 개선 폭은 추론 개선보다 작습니다.

- [환경·입력 hash·모델 hash·통계 JSON](../benchmarks/2026-10-13-rtx3070-small-objects.json)
- [프레임별 원본 CSV](../benchmarks/2026-10-13-rtx3070-small-objects.csv)
- [재측정 스크립트](../scripts/benchmark_bag.py)

## 측정 조건

| 조건 | 값 |
|---|---|
| GPU / driver | RTX 3070 Laptop GPU 8GB, SM 8.6 / 595.91.07 |
| OS / Python | Ubuntu 20.04.6 / conda Python 3.10.21 |
| PyTorch / CUDA runtime | 2.2.2+cu121 / 12.1 |
| TensorRT / OpenCV / NumPy | 10.7.0 / 4.9.0 / 1.26.4 |
| bag | `2026_3차_대회/bag0927/20260927_162055/record.bag` |
| 입력 | `/camera/image_raw/compressed`, 1920×1080 |
| 구간 | bag 시작 +10초부터 120프레임 |
| 반복 / 워밍업 | 모델별 3회 = 360건 / 30회 |
| 모델 입력 | batch 1 / `[1,3,640,640]`, `rect=False` |
| confidence | 진입 0.5 / 유지 0.3 / 이전 bbox IoU 0.3 |
| 추가 후처리 | NMS IoU 0.5 / containment IoA 0.8 / area weight 0.1 |

압축 이미지 bytes를 메모리에 보관하고 두 모델이 같은 순서로 처리합니다. 각 반복마다 히스테리시스 상태를 초기화합니다. 전체 처리 시간은 CUDA를 앞뒤에서 동기화한 wall time, 추론 시간은 Ultralytics `Results.speed['inference']`입니다. 모델 로딩·워밍업은 제외했습니다.

**포함:** JPEG decode → LetterBox·GPU 입력 전처리 → 추론 → Ultralytics 결과 처리 → 클래스/confidence/히스테리시스/추가 bbox 억제.

**제외:** bag disk I/O, ROS 전송·대기·직렬화, 화면 표시·drawing, MP4 인코딩, LiDAR-camera fusion.

38.78 FPS는 이 측정 구간의 평균 시간 역수이며 ROS 전체 출력 주파수나 MP4 생성 속도는 아닙니다. 모델을 순서대로 측정했고 GPU clock·발열·다른 GPU 작업을 고정하지 않았으므로 장비 상태에 따라 달라집니다.

## 검출이 없는 구간의 보조 측정

`bag0920/2026-09-20-16-57-33.bag`의 처음 120프레임을 같은 조건으로 측정했습니다. 두 모델의 필터 후 평균 검출은 0개였습니다.

| 백엔드 | 추론 평균 | 디코딩+검출 처리 평균 | 환산 FPS |
|---|---:|---:|---:|
| PyTorch FP32 | 8.20ms | 31.22ms | 32.03 |
| TensorRT FP16 | 1.96ms | 25.24ms | 39.63 |

[JSON](../benchmarks/2026-10-13-rtx3070-small.json), [CSV](../benchmarks/2026-10-13-rtx3070-small.csv). 주 결과는 bbox 후처리가 발생하는 위의 주행 구간입니다.

## 정확도

주행 샘플의 평균 검출 수는 PyTorch **12.683개**, TensorRT **12.667개**였습니다. 개수가 비슷하다고 mAP·precision·recall 보존을 입증할 수는 없습니다. bag의 평가용 ground truth와 현재 모델의 검증 데이터셋이 없어 **정확도 수치·FP16 정확도 손실은 미측정**입니다. 동일 labeled validation set에서 두 모델을 평가해야 합니다. 과거 발표 초안의 CAR baseline·pruning 수치는 다른 모델/데이터셋이므로 사용하지 않았습니다.
