#!/usr/bin/env python3
"""rosbag 의 카메라 영상에 yolo26 노드(yolo_detect.py)와 동일한 검출 로직으로
bbox 를 그려 mp4 로 저장합니다.

bag 을 재생(rosbag play)하지 않고 직접 읽기 때문에 프레임 드랍 없이
모든 프레임을 처리합니다. 실행은 같은 폴더의 run.sh 를 사용하세요.
"""
import argparse
import glob
import os
import sys
import time
import types

# ROS noetic 의 rosbag 은 bag 암호화 기능 때문에 Cryptodome/gnupg 를 import 하는데,
# yolotrt conda 환경에는 없습니다. 암호화되지 않은 bag 은 이 모듈을 쓰지 않으므로
# 빈 모듈로 대체합니다.
for _name in ("Cryptodome", "Cryptodome.Cipher", "Cryptodome.Cipher.AES",
              "Cryptodome.Random", "gnupg"):
    sys.modules.setdefault(_name, types.ModuleType(_name))
sys.modules["Cryptodome.Cipher"].AES = sys.modules["Cryptodome.Cipher.AES"]
sys.modules["Cryptodome.Random"].get_random_bytes = None

import cv2
import numpy as np
import rosbag

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
PERCEPTION_WS = os.path.dirname(THIS_DIR)
YOLO_SCRIPTS_DIR = os.path.join(PERCEPTION_WS, "src", "yolo26", "scripts")
sys.path.insert(0, YOLO_SCRIPTS_DIR)

# yolo 노드의 설정/후처리를 그대로 가져다 씁니다.
# (노드에서 confidence, NMS 등을 바꾸면 이 스크립트에도 자동 반영됩니다.)
import yolo_detect as node  # noqa: E402

DEFAULT_BAG_ROOT = os.path.join(os.path.dirname(PERCEPTION_WS), "bag")
DEFAULT_OUT_DIR = os.path.join(THIS_DIR, "output")

# 클래스별 bbox 색상 (BGR)
CLASS_COLORS = {
    0: (255, 128, 0),   # ERP-42
    1: (0, 128, 255),   # drum
    2: (0, 255, 255),   # cone
}


def parse_args():
    p = argparse.ArgumentParser(
        description="rosbag 카메라 영상에 YOLO bbox 를 그려 mp4 로 저장합니다.")
    p.add_argument("--bag", nargs="+",
                   help="bag 파일 또는 폴더 경로(여러 개 가능). 생략하면 목록에서 선택합니다.")
    p.add_argument("--bag-root", default=DEFAULT_BAG_ROOT,
                   help=f"선택 목록에 보여줄 bag 검색 폴더 (기본: {DEFAULT_BAG_ROOT})")
    p.add_argument("--topic", default=node.DEFAULT_SOURCE_TOPIC,
                   help=f"CompressedImage 토픽 (기본: {node.DEFAULT_SOURCE_TOPIC})")
    p.add_argument("--out-dir", default=DEFAULT_OUT_DIR,
                   help=f"mp4 저장 폴더 (기본: {DEFAULT_OUT_DIR})")
    p.add_argument("--out", help="출력 mp4 경로 (bag 1개일 때만 사용)")
    p.add_argument("--engine", default=node.DEFAULT_ENGINE_WEIGHTS,
                   help="모델 파일 (.engine/.pt/.onnx, 기본: 노드와 동일)")
    p.add_argument("--imgsz", type=int, default=node.DEFAULT_INFER_IMGSZ)
    p.add_argument("--fps", type=float,
                   help="출력 fps (기본: bag 의 실제 카메라 fps)")
    p.add_argument("--start", type=float, default=0.0,
                   help="bag 시작 기준 몇 초부터 처리할지")
    p.add_argument("--end", type=float,
                   help="bag 시작 기준 몇 초까지 처리할지")
    p.add_argument("--scale", type=float, default=1.0,
                   help="출력 영상 크기 배율 (예: 0.5 → 1920x1080 을 960x540 으로)")
    p.add_argument("--thickness", type=int, default=1,
                   help="bbox 선 두께(px, 기본 1)")
    p.add_argument("--label", choices=["full", "conf", "none"], default="full",
                   help="bbox 라벨: full='cone 0.87', conf='0.87'(촘촘할 때), none=라벨 없음")
    p.add_argument("--print-dets", action="store_true",
                   help="프레임별 검출 결과(클래스, confidence, bbox)를 터미널에 출력")
    p.add_argument("--show", action="store_true",
                   help="처리 중 화면에 미리보기 표시 (q 로 중단)")
    return p.parse_args()


def find_bags(root):
    return sorted(glob.glob(os.path.join(root, "**", "*.bag"), recursive=True))


def parse_selection(text, n):
    text = text.strip().lower()
    if text in ("a", "all"):
        return list(range(n))
    picked = []
    for token in text.replace(",", " ").split():
        if "-" in token:
            lo, hi = token.split("-", 1)
            picked.extend(range(int(lo), int(hi) + 1))
        else:
            picked.append(int(token))
    for i in picked:
        if not 1 <= i <= n:
            raise ValueError(f"범위를 벗어난 번호: {i}")
    return [i - 1 for i in dict.fromkeys(picked)]


def select_bags_interactive(root):
    bags = find_bags(root)
    if not bags:
        sys.exit(f"[ERROR] {root} 아래에 .bag 파일이 없습니다.")

    print(f"\n[{root}] 아래 bag 목록")
    for i, path in enumerate(bags, 1):
        size_gb = os.path.getsize(path) / 1024 ** 3
        print(f"  {i:3d}) {os.path.relpath(path, root)}  ({size_gb:.1f} GB)")

    while True:
        try:
            text = input("\n변환할 bag 번호 (예: 3 / 1,4 / 2-5 / all, q=종료): ")
        except EOFError:
            sys.exit(1)
        if text.strip().lower() == "q":
            sys.exit(0)
        try:
            idx = parse_selection(text, len(bags))
        except ValueError as e:
            print(f"  잘못된 입력입니다: {e}")
            continue
        if idx:
            return [bags[i] for i in idx]


def expand_bag_args(paths):
    bags = []
    for path in paths:
        if os.path.isdir(path):
            bags.extend(find_bags(path))
        elif os.path.isfile(path):
            bags.append(path)
        else:
            sys.exit(f"[ERROR] bag 을 찾을 수 없습니다: {path}")
    return bags


def output_path_for(bag_path, bag_root, out_dir):
    # 같은 이름(record.bag 등)이 여러 폴더에 있어서 상위 폴더명까지 붙입니다.
    try:
        rel = os.path.relpath(bag_path, bag_root)
    except ValueError:
        rel = bag_path
    if rel.startswith(".."):
        rel = os.path.join(os.path.basename(os.path.dirname(bag_path)),
                           os.path.basename(bag_path))
    name = os.path.splitext(rel)[0].replace(os.sep, "__")
    return os.path.join(out_dir, f"{name}_yolo.mp4")


class Detector:
    """yolo_detect.YoloDetectNode.callback 과 동일한 필터링/후처리 로직."""

    def __init__(self, engine, imgsz):
        self.class_config = {k: v.copy() for k, v in node.DEFAULT_CLASS_CONFIG.items()}
        confs = [min(c["confidence"], c["keep_confidence"])
                 for c in self.class_config.values() if c["publish"]]
        self.conf_thres = min(confs) if confs else 1.0
        self.imgsz = imgsz
        self.model = node.YOLO(engine, task="detect")
        self.prev_detections = []

    def reset(self):
        self.prev_detections = []

    def _matches_prev(self, det, track_iou):
        return any(prev["cls_id"] == det["cls_id"] and node.iou(prev, det) >= track_iou
                   for prev in self.prev_detections)

    def __call__(self, frame):
        results = self.model(frame, imgsz=self.imgsz, conf=self.conf_thres,
                             rect=False, verbose=False)[0]
        self.last_speed = results.speed.copy()
        candidates = []
        for box in results.boxes:
            cls_id = int(box.cls.cpu().item())
            conf = float(box.conf.cpu().item())
            x1, y1, x2, y2 = map(int, box.xyxy[0].cpu().tolist())
            cfg = self.class_config.get(cls_id)
            if cfg is None or not cfg["publish"]:
                continue
            det = {"cls_id": cls_id, "conf": conf,
                   "x1": x1, "y1": y1, "x2": x2, "y2": y2}
            if conf < cfg["confidence"]:
                if conf < cfg["keep_confidence"]:
                    continue
                if not self._matches_prev(det, cfg["track_iou"]):
                    continue
            candidates.append(det)

        dets = node.postprocess_detections(
            candidates,
            iou_threshold=node.DEFAULT_POSTPROCESS_NMS_IOU,
            containment_ioa_threshold=node.DEFAULT_POSTPROCESS_CONTAINMENT_IOA,
            area_weight=node.DEFAULT_POSTPROCESS_AREA_WEIGHT,
            max_det=node.DEFAULT_POSTPROCESS_MAX_DET,
        )
        self.prev_detections = [d.copy() for d in dets]
        return dets


def match_owner(path):
    """컨테이너(root)에서 만든 파일을 호스트 사용자도 지울 수 있게
    상위 폴더 소유자로 맞춥니다."""
    if os.geteuid() != 0:
        return
    parent = os.path.dirname(os.path.abspath(path))
    ref = parent
    while os.stat(ref).st_uid == 0 and os.path.dirname(ref) != ref:
        ref = os.path.dirname(ref)
    st = os.stat(ref)
    for target in (parent, path):
        try:
            os.chown(target, st.st_uid, st.st_gid)
        except OSError:
            pass


def draw(frame, dets, class_config, info_text, label_mode="full", thickness=1):
    for d in dets:
        cls_id = d["cls_id"]
        color = CLASS_COLORS.get(cls_id, (0, 255, 0))
        name = class_config.get(cls_id, {}).get("name", f"cls{cls_id}")
        cv2.rectangle(frame, (d["x1"], d["y1"]), (d["x2"], d["y2"]), color, thickness)
        if label_mode == "none":
            continue
        label = f"{d['conf']:.2f}" if label_mode == "conf" else f"{name} {d['conf']:.2f}"
        (tw, th), base = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.55, 1)
        ty = max(d["y1"], th + base + 4)
        cv2.rectangle(frame, (d["x1"], ty - th - base - 4), (d["x1"] + tw + 4, ty),
                      color, -1)
        cv2.putText(frame, label, (d["x1"] + 2, ty - base - 2),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 0), 1, cv2.LINE_AA)
    cv2.putText(frame, info_text, (10, 35), cv2.FONT_HERSHEY_SIMPLEX, 1.0,
                (0, 0, 0), 4, cv2.LINE_AA)
    cv2.putText(frame, info_text, (10, 35), cv2.FONT_HERSHEY_SIMPLEX, 1.0,
                (255, 255, 255), 2, cv2.LINE_AA)
    return frame


def process_bag(bag_path, out_path, detector, args):
    print(f"\n[BAG] {bag_path}")
    with rosbag.Bag(bag_path) as bag:
        total = bag.get_message_count(args.topic)
        if total == 0:
            topics = bag.get_type_and_topic_info().topics
            images = [t for t, info in topics.items()
                      if info.msg_type == "sensor_msgs/CompressedImage"]
            print(f"  [SKIP] '{args.topic}' 토픽이 없습니다. "
                  f"CompressedImage 토픽: {images or '없음'}")
            return False

        bag_start = bag.get_start_time()
        cam_info = bag.get_type_and_topic_info().topics[args.topic]
        fps = args.fps or (cam_info.frequency if cam_info.frequency else 30.0)

        import rospy
        start_t = rospy.Time.from_sec(bag_start + args.start)
        end_t = rospy.Time.from_sec(bag_start + args.end) if args.end else None

        detector.reset()
        writer = None
        n = 0
        t0 = time.time()
        try:
            for _, msg, t in bag.read_messages(topics=[args.topic],
                                               start_time=start_t, end_time=end_t):
                frame = cv2.imdecode(np.frombuffer(msg.data, np.uint8),
                                     cv2.IMREAD_COLOR)
                if frame is None:
                    continue
                dets = detector(frame)
                rel = t.to_sec() - bag_start
                draw(frame, dets, detector.class_config,
                     f"t={rel:6.2f}s  det={len(dets)}", args.label,
                     args.thickness)
                if args.scale != 1.0:
                    frame = cv2.resize(frame, None, fx=args.scale, fy=args.scale,
                                       interpolation=cv2.INTER_AREA)

                if writer is None:
                    h, w = frame.shape[:2]
                    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
                    writer = cv2.VideoWriter(out_path,
                                             cv2.VideoWriter_fourcc(*"mp4v"),
                                             fps, (w, h))
                    if not writer.isOpened():
                        sys.exit(f"[ERROR] VideoWriter 를 열 수 없습니다: {out_path}")
                    print(f"  topic={args.topic}  frames={total}  "
                          f"size={w}x{h}  fps={fps:.2f}")
                writer.write(frame)
                n += 1

                if args.print_dets:
                    items = ", ".join(
                        f"{detector.class_config[d['cls_id']]['name']}"
                        f"({d['conf']:.2f}) [{d['x1']},{d['y1']},{d['x2']},{d['y2']}]"
                        for d in dets) or "NO DETECT"
                    print(f"  t={rel:6.2f}s  {items}")

                if args.show:
                    cv2.imshow("yolo_bag_to_video", frame)
                    if cv2.waitKey(1) & 0xFF == ord("q"):
                        print("  [STOP] q 입력으로 중단")
                        break
                if n % 100 == 0:
                    speed = n / (time.time() - t0)
                    print(f"  {n}/{total} frames  ({speed:.1f} fps)", flush=True)
        finally:
            if writer is not None:
                writer.release()
                match_owner(out_path)
            if args.show:
                cv2.destroyAllWindows()

    if n == 0:
        print("  [SKIP] 처리된 프레임이 없습니다 (--start/--end 확인).")
        return False
    print(f"  [DONE] {n} frames, {time.time() - t0:.1f}s → {out_path}")
    return True


def main():
    args = parse_args()
    bags = expand_bag_args(args.bag) if args.bag else select_bags_interactive(args.bag_root)
    if not bags:
        sys.exit("[ERROR] 처리할 bag 이 없습니다.")
    if args.out and len(bags) > 1:
        sys.exit("[ERROR] --out 은 bag 1개일 때만 쓸 수 있습니다. --out-dir 를 사용하세요.")

    print(f"\n[MODEL] {args.engine} (imgsz={args.imgsz})")
    detector = Detector(args.engine, args.imgsz)

    done = []
    try:
        for bag_path in bags:
            out_path = args.out or output_path_for(bag_path, args.bag_root, args.out_dir)
            if process_bag(bag_path, out_path, detector, args):
                done.append(out_path)
    except KeyboardInterrupt:
        print("\n[STOP] Ctrl+C 로 중단했습니다. 지금까지 만든 영상은 저장되었습니다.")

    print("\n[RESULT]")
    for path in done:
        print(f"  {path}")


if __name__ == "__main__":
    main()
