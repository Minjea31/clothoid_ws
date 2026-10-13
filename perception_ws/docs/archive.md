# 배포 archive

실제 Git 루트는 상위 `/home/a/clothoid_ws`입니다. 다른 상위 폴더까지 배포하지 않도록 `perception_ws` subtree를 기준으로 묶습니다.

| 포함 | 제외 |
|---|---|
| ROS 소스·메시지·launch·캘리브레이션 | `build/`, `devel/`, `install/`, `logs/` |
| Ultralytics 포크·설치 manifest | `__pycache__/`, `.pyc`, `.egg-info/` |
| 원본 `small.pt`, 기존 `best.pt` | `.engine`, `.onnx`, `.engine.layers.json` |
| 문서·실측 JSON/CSV·도구 | bag, MP4, 로컬 재측정·bag 미리보기 생성물 |

`.gitignore`는 새 생성물의 추적을 막고 `.gitattributes`의 `export-ignore`는 이미 추적된 생성물도 archive에서 제외합니다. 작업 폴더의 기존 생성물은 보존합니다. 과거 발표 초안 `docs/notes/legacy-yolo26-seminar-prompt.md`도 원본을 남기고 배포에서 제외합니다.

## 현재 작업본

```bash
conda activate yolotrt
python scripts/export_archive.py
# dist/perception_ws.tar.gz
```

추적 파일과 ignore되지 않은 새 파일을 임시 폴더에 복사하고 임시 Git의 `write-tree` → `archive`로 압축합니다. 원래 Git index/commit을 수정하지 않습니다. 미커밋 모델 변경도 현재 파일 내용으로 포함합니다. 기존 출력은 덮어쓰지 않으므로 재실행은 `--output dist/another-name.tar.gz`를 사용하세요.

## 커밋 기준

새 문서·도구·기본 가중치 `src/yolo26/models/small.pt`까지 검토·커밋한 뒤:

```bash
mkdir -p dist
git archive --format=tar.gz --prefix=perception_ws/ \
  --output="$(pwd)/dist/perception_ws-committed.tar.gz" HEAD:perception_ws
```

workspace가 Git 최상위인 별도 저장소에서는 `HEAD:perception_ws`를 `HEAD`로 바꿉니다. `git archive HEAD`에는 미커밋 수정과 새 파일이 포함되지 않습니다.

## 해제·재빌드

```bash
mkdir -p /path/to/deployment
tar -xzf perception_ws.tar.gz -C /path/to/deployment
cd /path/to/deployment/perception_ws
source /opt/ros/noetic/setup.bash
catkin_make -DPYTHON_EXECUTABLE=/usr/bin/python3 -j2
source devel/setup.bash
conda activate yolotrt
python yolo26/build_trt_engine.py --fp16
roslaunch perception_bringup perception.launch yolo_python:="$(command -v python)"
```

환경이 없으면 README의 conda 설치 단계를 먼저 수행합니다. 절대경로 symlink `src/CMakeLists.txt`와 `.catkin_workspace`도 제외하며 catkin이 대상 ROS 설치에 맞게 다시 생성합니다. engine은 대상 GPU에서 빌드하고 bag은 외부 경로에서 제공합니다.
