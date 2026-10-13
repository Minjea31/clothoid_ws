# 모델 파일

| 원본 가중치 | 모델 | 용도 |
|---|---|---|
| `small.pt` | YOLO26s, ERP-42/drum/cone 3 classes | 현재 기본 배포 모델 |
| `best.pt` | YOLO26n, 같은 3 classes | 기존 Nano 모델 원본 보존 |

`.pt`는 Git과 archive에 포함합니다. `.onnx`, `.engine`, `.engine.layers.json`은 재생성 가능한 산출물로 추적·배포에서 제외하고 대상 GPU에서 빌드합니다.

```bash
# workspace 루트에서
conda activate yolotrt
python yolo26/build_trt_engine.py --fp16
```

기본 출력은 `small.onnx`, `small_fp16.engine`, `small_fp16.engine.layers.json`입니다. Nano FP32와 Small FP16은 모델 크기도 다르므로 같은 모델의 정밀도 비교로 해석하지 않습니다. [TensorRT 구현](../../../docs/tensorrt.md), [성능 기록](../../../docs/performance.md).
