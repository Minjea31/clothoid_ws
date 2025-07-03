# 환경 세팅

conda를 사용해 가상환경을 만듦

conda create -n yolov8 python=3.8 -y
conda activate yolov8


만약 ultralystics 가 깔려있다면 삭제해야함
pip uninstall ultraltstics


pip install -e .
pip install torch torchvision pyyaml

# 실행 

