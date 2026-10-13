# 현재 보관 중인 ROS1 bag

2026-10-14 최종 확인: **8개 / 59.74 GiB**. 최초 조사 때는 28개 / 165.13 GiB였고 현재는 10월 1일·2일 기록만 남아 있습니다. 다른 파일의 당시 조사 결과는 로컬 `reports/bag-audit/`에 역사 기록으로 보존합니다.

## 현재 목록

| bag | GiB | 카메라 | Livox | 상태 |
|---|---:|---|---|---|
| `bag1001/20261001_163444/record_noimg.bag` | 4.008 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1001/20261001_164055/record_noimg.bag` | 4.147 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1001/20261001_164254/record_noimg.bag` | 3.056 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1001/20261001_164553/record_noimg.bag` | 8.563 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1001/20261001_165948/record_noimg.bag` | 5.577 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1001/20261001_170302/record_noimg.bag` | 7.841 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1002/20261002_144630/record.bag` | 10.819 | 있음 | 있음 | 두 센서 입력 토픽 확인 |
| `bag1002/20261002_144630/record.orig.bag` | 15.726 | 있음 | 있음 | 인덱스 없음; 읽기 전용 검사에서 센서 구간 확인 |

## 우선 사용할 데이터

| 목적 | bag | 확인 구간 |
|---|---|---|
| ERP-42 + cone | `bag1001/20261001_164254/record_noimg.bag` | +23.5s, +31.3s |
| ERP-42 + cone, 가까운 차량 | `bag1001/20261001_170302/record_noimg.bag` | +47s, +156.6s, +172초 |
| 밀집 cone + ERP-42, 현재 성능 측정 | `bag1002/20261002_144630/record.bag` | cone +0~42s, ERP +47~52s |

**`record_noimg.bag`에도 `/camera/image_raw/compressed`가 있습니다.** Livox PointCloud2도 함께 있으므로 fusion 입력으로 사용할 수 있습니다. 보관 기준은 RGB 카메라와 Livox 원본 입력이며 BEV 디버그 이미지나 filtered cloud는 원본을 대신하지 않습니다.

## drum 자료

최초 조사에서는 `bag0929/20260929_174924/0929_5.bag`의 +28.8초에서 drum을 실제 사진으로 확인했습니다. 그 원본 bag은 현재 없고 [보존한 사진](assets/drum-historical.jpg)만 남아 있습니다. **현재 8개 bag에서는 최대 12프레임씩 샘플링한 범위에서 drum을 검출하지 못했습니다.** 이는 전체 영상에 drum이 전혀 없다는 증거는 아니지만, 현재 확인된 drum 실험용 원본을 보유한 상태로 안내하지 않습니다.

## 인덱스 없는 원본

`bag1002/.../record.orig.bag`는 일반 rosbag에서 `Unindexed bag` 오류가 납니다. 파일을 `rb`로 열어 메모리에서만 인덱스를 스캔해 읽을 수 있는 구간에 압축 카메라 3103개·Livox 517개, 약 51.86초가 있음을 확인했습니다. 원본은 수정하지 않았습니다. `record.bag`와 전체 파일이 동일하다는 검증은 하지 않았으므로 자동 삭제하지 않습니다.

## 조사 방식

토픽 타입과 메시지 수를 확인하고 bag당 최대 12개 시점을 TensorRT로 샘플링했습니다. 실제 RGB와 `/perception/velodyne/bev_image`를 구분했습니다. 추천 데이터는 미리보기로 추가 확인했고 camera/Livox의 가까운 header stamp 간격도 샘플링했습니다. 검출 수는 유일 객체 수가 아니며 미검출이나 오검출만으로 삭제 여부를 결정하지 않습니다.

```bash
source /opt/ros/noetic/setup.bash
source devel/setup.bash
conda activate yolotrt
python scripts/audit_bags.py --root ../bag --samples 12
```

보고서와 전체 미리보기는 `reports/bag-audit/`에 생성되며 배포 archive에서는 제외합니다.
