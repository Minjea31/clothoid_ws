#!/bin/bash
set -e
# clothoid_ws 도커 컨테이너 안에서 실행합니다.
# 호스트에서 실행하면 자동으로 컨테이너(clothoid_ws) 안에서 다시 실행합니다.
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTAINER="${YOLO_CONTAINER:-clothoid_ws}"
HOST_WS="$(dirname "$(dirname "$DIR")")"
DOCKER_WS="${YOLO_DOCKER_WS:-/root/clothoid_ws}"

if [ ! -f /.dockerenv ]; then
    # 호스트 경로를 컨테이너 경로로 바꿔서 전달
    args=()
    for a in "$@"; do
        [ -e "$a" ] && a="$(realpath "$a")"   # 상대 경로 → 절대 경로
        a="${a/#$HOST_WS/$DOCKER_WS}"
        args+=("$a")
    done
    tty_flag=-i; [ -t 0 ] && tty_flag=-it
    exec docker exec $tty_flag --env "YOLO_CONDA_ENV=${YOLO_CONDA_ENV:-yolotrt}" \
        --env "YOLO_PYTHON=${YOLO_PYTHON:-}" "$CONTAINER" \
        bash "$DOCKER_WS/perception_ws/yolo_video/run.sh" "${args[@]}"
fi

source /opt/ros/noetic/setup.bash
source "$DIR/../devel/setup.bash"

if [ -n "${YOLO_PYTHON:-}" ]; then
    exec "$YOLO_PYTHON" "$DIR/yolo_bag_to_video.py" "$@"
elif [ -n "${CONDA_PREFIX:-}" ]; then
    exec "$CONDA_PREFIX/bin/python" "$DIR/yolo_bag_to_video.py" "$@"
fi

# 현재 환경이 활성화되지 않았으면 conda로 환경을 선택합니다.
CONDA_COMMAND="$(command -v conda || true)"
if [ -z "$CONDA_COMMAND" ]; then
    for candidate in /opt/conda/bin/conda /opt/miniconda3/bin/conda \
                     "$HOME/anaconda3/bin/conda" "$HOME/miniconda3/bin/conda"; do
        if [ -x "$candidate" ]; then
            CONDA_COMMAND="$candidate"
            break
        fi
    done
fi
if [ -z "$CONDA_COMMAND" ]; then
    echo 'conda를 활성화하거나 YOLO_PYTHON에 Python 실행 파일을 지정하세요.' >&2
    exit 1
fi
exec "$CONDA_COMMAND" run --no-capture-output -n "${YOLO_CONDA_ENV:-yolotrt}" \
    python "$DIR/yolo_bag_to_video.py" "$@"
