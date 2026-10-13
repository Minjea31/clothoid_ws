#!/usr/bin/env python3
"""Export documented detection examples from real ROS1 camera messages."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

WORKSPACE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(WORKSPACE / "yolo_video"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bag-root", type=Path, default=WORKSPACE.parent / "bag")
    parser.add_argument("--engine", type=Path, default=WORKSPACE / "src/yolo26/models/small_fp16.engine")
    parser.add_argument("--output", type=Path, default=WORKSPACE / "docs/assets")
    args = parser.parse_args()
    import yolo_bag_to_video as video
    import rospy
    cv2, np, rosbag = video.cv2, video.np, video.rosbag
    args.output.mkdir(parents=True, exist_ok=True)
    detector = video.Detector(str(args.engine), 640)
    examples = [
        ("erp42-cones", "2026_3차_대회/bag1001/20261001_170302/record_noimg.bag", 172.0, "ERP-42 + cone"),
        ("dense-cones", "2026_3차_대회/bag1002/20261002_144630/record.bag", 32.9, "Dense cones"),
    ]
    provenance = []
    for name, relative_bag, offset, caption in examples:
        with rosbag.Bag(str(args.bag_root / relative_bag)) as bag:
            _, message, stamp = next(bag.read_messages(
                topics=[video.node.DEFAULT_SOURCE_TOPIC],
                start_time=rospy.Time.from_sec(bag.get_start_time() + offset)))
            frame = cv2.imdecode(np.frombuffer(message.data, np.uint8), cv2.IMREAD_COLOR)
            detector.reset()
            detections = detector(frame)
            height, width = frame.shape[:2]
            scale = 960 / width
            display = cv2.resize(frame, (960, round(height * scale)))
            display_detections = [{**detection, **{key: round(detection[key] * scale)
                                                  for key in ("x1", "y1", "x2", "y2")}}
                                  for detection in detections]
            actual_offset = stamp.to_sec() - bag.get_start_time()
            video.draw(display, display_detections, detector.class_config,
                       f"{caption} | t={actual_offset:.2f}s",
                       label_mode="conf" if name == "dense-cones" else "none", thickness=2)
            if name == "erp42-cones":
                # Keep the closely spaced cone labels from covering each other.
                for detection in display_detections:
                    if detection["cls_id"] != 0:
                        continue
                    x, y = detection["x1"], max(24, detection["y1"])
                    label = f"ERP-42 {detection['conf']:.2f}"
                    (text_width, text_height), baseline = cv2.getTextSize(
                        label, cv2.FONT_HERSHEY_SIMPLEX, 0.55, 1)
                    cv2.rectangle(display, (x, y - text_height - baseline - 5),
                                  (x + text_width + 5, y), video.CLASS_COLORS[0], -1)
                    cv2.putText(display, label, (x + 2, y - baseline - 2),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 0, 0), 1, cv2.LINE_AA)
            destination = args.output / f"{name}.jpg"
            if not cv2.imwrite(str(destination), display, [cv2.IMWRITE_JPEG_QUALITY, 90]):
                raise RuntimeError(f"Could not save {destination}")
            provenance.append({"image": destination.name, "bag": relative_bag,
                               "topic": video.node.DEFAULT_SOURCE_TOPIC,
                               "bag_offset_s": actual_offset, "header_stamp_s": message.header.stamp.to_sec(),
                               "source_hw": [height, width], "detections": detections})
            print(f"Saved {destination}: {len(detections)} detections", flush=True)
    (args.output / "examples.json").write_text(json.dumps(provenance, indent=2, ensure_ascii=False) + "\n")


if __name__ == "__main__":
    main()
