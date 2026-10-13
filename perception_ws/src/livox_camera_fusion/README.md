# livox_camera_fusion

YOLO bbox에 들어오는 Livox 포인트로 객체의 2D 중심 좌표를 발행하는 ROS1 C++ 패키지입니다.

| 방향 | 토픽 | 타입 |
|---|---|---|
| 입력 | `/livox/lidar` | `sensor_msgs/PointCloud2` |
| 입력 | `/camera/image_raw/compressed` | `sensor_msgs/CompressedImage` |
| 입력 | `/perception/camera/yolo` | `detect_msgs/Yolo_Objects` |
| 출력 | `/perception/fusion/centroids` | `sensor_msgs/PointCloud` |
| 출력 | `/perception/fusion/filtered_cloud` | `sensor_msgs/PointCloud2` |

`ApproximateTime` queue 20, 최대 header stamp 간격 50ms로 세 입력을 동기화합니다. 출력 stamp는 LiDAR 입력을 사용합니다.

## 알고리즘

1. 원 LiDAR 좌표를 `K × [R|t]`로 카메라에 투영합니다.
2. pitch 0.73°를 보정한 좌표로 회전하고 3D ROI를 적용합니다.
3. bbox 내부 포인트 중 bbox 중심 반경 10px에 들어오는 포인트를 추출합니다.
4. 지면 제거 함수를 거친 뒤 Euclidean clustering을 수행합니다.
5. 가장 큰 cluster의 3D AABB를 검사하고 평균 `(x,y)`를 중심으로 사용합니다.
6. 별도 Kalman tracker를 갱신하고 현재 프레임의 **측정 중심**을 발행합니다.

클래스별 조건 분리 없이 같은 설정을 적용합니다.

## 설정

토픽과 `frame_name`은 launch/private parameter로 변경합니다. projection은 `config/projection.yaml`의 `camera_matrix` 3×3, `extrinsic_matrix` 3×4를 사용합니다. 알고리즘 상수는 `include/livox_camera_fusion.h`에 있습니다.

| 항목 | 값 |
|---|---|
| LiDAR ROI | x `[0,15]`, y `[-7,7]`, z `[-2,2]` m |
| pitch / bbox 배율 | 0.73° / 1.0 |
| bbox 중심 ROI | 반경 10px |
| cluster tolerance / 수 | 0.4m / 3~100포인트 |
| AABB length/width/height | 각각 0.1~10m |
| 지면 threshold | **0.0** |
| tracker match / max miss | 0.7m / 10 callbacks |

## 현재 제한

- `GROUND_THRESH=0.0`이므로 양의 폭의 지면 band를 제거하지 않습니다. RANSAC 함수가 있다는 사실만으로 지면 제거가 활성화되었다고 해석하지 않습니다.
- `cfg/LidarClustering.cfg`는 빌드되지만 C++ 노드에 dynamic reconfigure server/callback이 없어 UI 변경이 위 상수에 반영되지 않습니다.
- 발행값은 현재 cluster 측정 중심이며 Kalman ID·미검출 예측 좌표를 publish하지 않습니다.
- centroid는 pitch 보정 좌표인데 기본 frame_id는 `livox_frame`입니다. filtered cloud도 보정 좌표를 담으며 LiDAR header를 그대로 사용합니다. TF 소비자는 좌표 의미를 확인해야 합니다.
- centroid `z=0`, channel `dummy=1.0`이며 클래스·confidence·tracking ID는 포함하지 않습니다.

```bash
roslaunch livox_camera_fusion livox_camera_fusion.launch
```

[전체 실행](../../README.md), [bag 검증](../../docs/operations.md).
