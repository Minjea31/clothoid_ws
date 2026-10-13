#!/usr/bin/env python3
"""Inventory ROS1 bags and visually sample camera frames; never deletes data."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import sys
import time

WORKSPACE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(WORKSPACE / "yolo_video"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--engine", type=Path, default=WORKSPACE / "src/yolo26/models/small_fp16.engine")
    parser.add_argument("--samples", type=int, default=12)
    parser.add_argument("--output", type=Path, default=WORKSPACE / "reports/bag-audit")
    args = parser.parse_args()
    if args.samples < 2:
        parser.error("At least 2 samples required")
    import yolo_bag_to_video as video
    import rospy
    cv2, np, rosbag = video.cv2, video.np, video.rosbag
    args.output.mkdir(parents=True, exist_ok=True)
    detector = video.Detector(str(args.engine), 640)
    detector.model.overrides["device"] = 0
    report = {"created_at_utc": datetime.now(timezone.utc).isoformat(),
              "root": str(args.root), "engine": str(args.engine), "samples_per_bag": args.samples,
              "criterion": "Prefer camera+Livox PointCloud2 bags for fusion reproduction",
              "limitations": "Sparse samples; non-detection does not prove object absence; stamp gaps need separate validation",
              "bags": []}
    bags = sorted(args.root.rglob("*.bag"))
    for index, path in enumerate(bags, 1):
        entry = {"path": str(path.relative_to(args.root)), "bytes": path.stat().st_size,
                 "gib": round(path.stat().st_size / 1024**3, 3), "samples": [], "error": None}
        started = time.monotonic()
        try:
            with rosbag.Bag(str(path)) as bag:
                topics = bag.get_type_and_topic_info().topics
                entry["duration_s"] = bag.get_end_time() - bag.get_start_time()
                entry["topics"] = {name: {"type": info.msg_type, "messages": info.message_count,
                                          "frequency": info.frequency}
                                   for name, info in topics.items()}
                # Derived BEV/segmentation/debug images are not RGB camera data.
                cameras = [name for name, info in topics.items()
                           if info.msg_type in ("sensor_msgs/CompressedImage", "sensor_msgs/Image")
                           and not name.startswith("/perception/") and "bev" not in name.lower()]
                entry["derived_image_topics"] = [name for name, info in topics.items()
                                                  if info.msg_type in ("sensor_msgs/CompressedImage", "sensor_msgs/Image")
                                                  and name not in cameras]
                compressed = [name for name in cameras if topics[name].msg_type == "sensor_msgs/CompressedImage"]
                camera = "/camera/image_raw/compressed" if "/camera/image_raw/compressed" in cameras else (compressed or cameras or [None])[0]
                lidar = [name for name, info in topics.items() if "livox" in name.lower() and info.msg_type == "sensor_msgs/PointCloud2"]
                entry["camera_topic"] = camera
                entry["livox_pointcloud2_topics"] = lidar
                entry["fusion_inputs"] = bool(camera and lidar and topics[camera].msg_type == "sensor_msgs/CompressedImage")
                entry["default_fusion_topics"] = bool(camera == "/camera/image_raw/compressed" and "/livox/lidar" in lidar)
                thumbs = []
                totals = {"ERP-42": 0, "drum": 0, "cone": 0}
                if camera:
                    offsets = np.linspace(0, max(0, entry["duration_s"] - 0.1), args.samples)
                    seen = set()
                    for sample_index, offset in enumerate(offsets):
                        messages = bag.read_messages(topics=[camera], start_time=rospy.Time.from_sec(bag.get_start_time() + float(offset)))
                        item = next(messages, None)
                        if item is None:
                            continue
                        _, msg, stamp = item
                        key = (stamp.secs, stamp.nsecs)
                        if key in seen:
                            continue
                        seen.add(key)
                        if topics[camera].msg_type == "sensor_msgs/CompressedImage":
                            frame = cv2.imdecode(np.frombuffer(msg.data, np.uint8), cv2.IMREAD_COLOR)
                        else:
                            # Avoid cv_bridge's system-Python ABI dependency.
                            if msg.encoding not in ("bgr8", "rgb8"):
                                continue
                            rows = np.frombuffer(msg.data, np.uint8).reshape(msg.height, msg.step)
                            frame = rows[:, :msg.width * 3].reshape(msg.height, msg.width, 3).copy()
                            if msg.encoding == "rgb8":
                                frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
                        if frame is None:
                            continue
                        detector.reset()  # Random samples have no immediately preceding tracked frame.
                        detections = detector(frame)
                        counts = {"ERP-42": 0, "drum": 0, "cone": 0}
                        for detection in detections:
                            name = detector.class_config[detection["cls_id"]]["name"]
                            counts[name] += 1
                            totals[name] += 1
                        sample = {"offset_s": stamp.to_sec() - bag.get_start_time(),
                                  "camera_header_s": msg.header.stamp.to_sec(),
                                  "image_hw": frame.shape[:2], "counts": counts,
                                  "detections": detections}
                        if lidar:
                            nearby = bag.read_messages(
                                topics=[lidar[0]],
                                start_time=rospy.Time.from_sec(max(0, stamp.to_sec() - 0.2)),
                                end_time=rospy.Time.from_sec(stamp.to_sec() + 0.2))
                            gaps = [abs(point_msg.header.stamp.to_sec() - msg.header.stamp.to_sec()) * 1000
                                    for _, point_msg, _ in nearby]
                            sample["closest_lidar_header_gap_ms"] = min(gaps) if gaps else None
                        entry["samples"].append(sample)
                        label = f"t={sample['offset_s']:.1f}s ERP={counts['ERP-42']} drum={counts['drum']} cone={counts['cone']}"
                        video.draw(frame, detections, detector.class_config, label, thickness=2)
                        thumbs.append(cv2.resize(frame, (640, 360)))
                    if thumbs:
                        while len(thumbs) % 3:
                            thumbs.append(np.zeros_like(thumbs[0]))
                        montage = np.vstack([np.hstack(thumbs[i:i + 3]) for i in range(0, len(thumbs), 3)])
                        image_name = f"{index:02d}_samples.jpg"
                        if not cv2.imwrite(str(args.output / image_name), montage, [cv2.IMWRITE_JPEG_QUALITY, 80]):
                            raise RuntimeError("Could not save montage")
                        entry["montage"] = image_name
                entry["sample_detection_totals"] = totals
                entry["recommendation"] = "KEEP_FUSION" if entry["fusion_inputs"] else ("REVIEW_CAMERA_ONLY" if camera else "REVIEW_NO_CAMERA")
        except Exception as exc:
            entry["error"] = str(exc)
            entry["recommendation"] = "REVIEW_ERROR"
        report["bags"].append(entry)
        (args.output / "inventory.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
        print(f"[{index}/{len(bags)}] {entry['path']} {entry['gib']}GiB {entry['recommendation']} {entry.get('sample_detection_totals', {})} ({time.monotonic()-started:.1f}s)", flush=True)
    lines = ["# ROS1 bag inventory", "", "카메라+Livox PointCloud2를 우선 보관합니다. 샘플 미검출은 객체 부재의 증거가 아닙니다.", "",
             "| # | bag | GiB | sec | camera | Livox PC2 | sample ERP/drum/cone | recommendation |",
             "|---|---|---:|---:|---|---|---|---|"]
    for index, entry in enumerate(report["bags"], 1):
        totals = entry.get("sample_detection_totals", {})
        counts = "/".join(str(totals.get(name, 0)) for name in ("ERP-42", "drum", "cone"))
        label = entry["path"]
        if entry.get("montage"):
            label = f"[{label}]({entry['montage']})"
        lines.append(f"| {index} | {label} | {entry['gib']} | {entry.get('duration_s', 0):.1f} | {entry.get('camera_topic') or '-'} | {', '.join(entry.get('livox_pointcloud2_topics', [])) or '-'} | {counts} | {entry['recommendation']} |")
    (args.output / "README.md").write_text("\n".join(lines) + "\n")
    print(f"Saved {args.output / 'README.md'}", flush=True)


if __name__ == "__main__":
    main()
