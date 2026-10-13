# YOLO26 runtime

이 디렉터리는 ROS 배포에 사용하는 Ultralytics 포크와 TensorRT 변환 도구입니다.
ROS 패키지는 `../src/yolo26`, 모델 원본은 `../src/yolo26/models`에 있습니다.

기존 `conda` 설치를 사용합니다.

```bash
conda create -n yolotrt python=3.10 -y
conda activate yolotrt
cd yolo26
python -m pip install -r requirements.txt
cd ..
python yolo26/build_trt_engine.py --fp16
```

이미 `yolotrt` 환경이 있으면 생성 단계는 생략합니다. `requirements.txt` 마지막의
`-e .`는 현재 디렉터리의 포크를 설치하므로 반드시 `yolo26/`에서 설치합니다.
코드의 라이선스 표기와 `pyproject.toml`은 upstream Ultralytics의 AGPL-3.0을 따릅니다.

- [전체 설치·실행](../README.md)
- [TensorRT 변환 방식](../docs/tensorrt.md)
- [성능 측정](../docs/performance.md)

원본 학습·프루닝 데이터셋과 fine-tuning 저장소는 이 배포 workspace에 포함되어 있지 않습니다.
