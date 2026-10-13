# README 이미지 출처

AI로 만든 예시 이미지가 아니라 실제 ROS1 bag 프레임과 실측 데이터입니다. 색은 ERP-42 파랑, drum 주황, cone 노랑입니다.

| 파일 | 출처 |
|---|---|
| `erp42-cones.jpg` | `bag1001/20261001_170302/record_noimg.bag`, 약 +172초, 현재 보관 중 |
| `dense-cones.jpg` | `bag1002/20261002_144630/record.bag`, 약 +32.9초, 현재 보관 중 |
| `drum-historical.jpg` | 이전 `bag0929/20260929_174924/0929_5.bag` 조사 이미지, +28.8초. 현재 원본 bag은 없음 |
| `performance.svg` | `benchmarks/2026-10-14-rtx3070-small-current-bag.json`의 평균 시간 |

카메라 사진은 CompressedImage를 디코딩하고 배포 detector로 bbox를 그린 후 README용 해상도로 저장했습니다. 상세 시간과 원본 좌표는 [examples.json](examples.json)에 있습니다. drum 사진은 최초 조사 때 저장한 montage의 해당 프레임을 보존했습니다.

```bash
python scripts/export_readme_examples.py
python scripts/render_performance.py
```
