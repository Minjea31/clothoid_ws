#!/usr/bin/env python3
"""Compare models on identical ROS1 CompressedImage messages without a ROS master.

Measures JPEG decode + prediction + the deployment node's filters, excluding bag
I/O, drawing, video encoding, ROS transport and LiDAR fusion. CUDA is synchronized
at the timing boundaries. See docs/performance.md for interpretation.
"""
from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys
import time

WORKSPACE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(WORKSPACE / "yolo_video"))


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bag", type=Path, required=True)
    parser.add_argument("--models", type=Path, nargs="+", required=True)
    parser.add_argument("--topic", default="/camera/image_raw/compressed")
    parser.add_argument("--frames", type=int, default=120)
    parser.add_argument("--warmup", type=int, default=30)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--start", type=float, default=0.0, help="Seconds from bag start")
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--output", type=Path, default=WORKSPACE / "benchmarks/runs/result.json")
    args = parser.parse_args()
    if args.frames < 1 or args.warmup < 0 or args.repeats < 1 or args.start < 0:
        parser.error("frames/repeats must be positive; warmup/start must be nonnegative")
    for path in [args.bag, *args.models]:
        if not path.is_file():
            parser.error(f"File not found: {path}")
    return args


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def stats(values, np):
    return {"mean": float(np.mean(values)), "p50": float(np.percentile(values, 50)),
            "p95": float(np.percentile(values, 95))}


def main():
    args = parse_args()
    # This imports the same detector and unencrypted-rosbag compatibility layer
    # used by the video tool. It does not initialize a ROS node.
    import yolo_bag_to_video as video
    import torch
    import tensorrt
    import ultralytics
    cv2, np, rosbag = video.cv2, video.np, video.rosbag
    if not torch.cuda.is_available():
        raise SystemExit("CUDA GPU required for a PyTorch/TensorRT comparison")

    samples = []
    with rosbag.Bag(str(args.bag)) as bag:
        origin = bag.get_start_time()
        topic_info = bag.get_type_and_topic_info().topics.get(args.topic)
        if topic_info is None or topic_info.msg_type != "sensor_msgs/CompressedImage":
            raise SystemExit(f"Missing CompressedImage topic: {args.topic}")
        for _, msg, stamp in bag.read_messages(topics=[args.topic]):
            if stamp.to_sec() < origin + args.start:
                continue
            frame = cv2.imdecode(np.frombuffer(msg.data, np.uint8), cv2.IMREAD_COLOR)
            if frame is None:
                raise SystemExit("Invalid compressed image in benchmark interval")
            samples.append((bytes(msg.data), stamp.to_sec() - origin, frame.shape[:2]))
            if len(samples) >= args.frames:
                break
    if not samples:
        raise SystemExit("No camera frames in the requested interval")
    input_hash = hashlib.sha256(b"".join(item[0] for item in samples)).hexdigest()
    gpu_query = subprocess.run(
        ["nvidia-smi", "--query-gpu=name,driver_version", "--format=csv,noheader"],
        check=True, capture_output=True, text=True).stdout.strip()
    report = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(),
        "environment": {"os": platform.platform(), "python": platform.python_version(),
                        "torch": torch.__version__, "torch_cuda": torch.version.cuda,
                        "tensorrt": tensorrt.__version__, "opencv": cv2.__version__,
                        "numpy": np.__version__, "ultralytics": ultralytics.__version__,
                        "ultralytics_path": str(Path(ultralytics.__file__).resolve()),
                        "gpu": gpu_query, "compute_capability": torch.cuda.get_device_capability(0)},
        "input": {"bag": str(args.bag.resolve()), "bag_bytes": args.bag.stat().st_size,
                  "topic": args.topic, "frames": len(samples), "camera_messages": topic_info.message_count,
                  "first_offset_s": samples[0][1], "last_offset_s": samples[-1][1],
                  "frame_shapes_hw": sorted({item[2] for item in samples}),
                  "compressed_frames_sha256": input_hash},
        "settings": {"imgsz": args.imgsz, "rect": False, "batch": 1, "device": 0,
                     "warmup": args.warmup, "repeats": args.repeats,
                     "class_config": video.node.DEFAULT_CLASS_CONFIG,
                     "nms_iou": video.node.DEFAULT_POSTPROCESS_NMS_IOU,
                     "containment_ioa": video.node.DEFAULT_POSTPROCESS_CONTAINMENT_IOA,
                     "area_weight": video.node.DEFAULT_POSTPROCESS_AREA_WEIGHT},
        "timing_scope": "in-memory JPEG decode + YOLO predict + deployment filters; excludes bag I/O, ROS, drawing, encoding and fusion",
        "models": [],
    }
    rows = []
    for model_path in args.models:
        detector = video.Detector(str(model_path.resolve()), args.imgsz)
        detector.model.overrides["device"] = 0
        detector.model.overrides["half"] = False  # PyTorch baseline in FP32; engine defines its own precision.
        frame = cv2.imdecode(np.frombuffer(samples[0][0], np.uint8), cv2.IMREAD_COLOR)
        for _ in range(args.warmup):
            detector(frame)
        torch.cuda.synchronize()
        model_rows = []
        for repeat in range(args.repeats):
            detector.reset()
            for index, (encoded, offset, _) in enumerate(samples):
                torch.cuda.synchronize()
                started = time.perf_counter()
                frame = cv2.imdecode(np.frombuffer(encoded, np.uint8), cv2.IMREAD_COLOR)
                detections = detector(frame)
                torch.cuda.synchronize()
                elapsed = (time.perf_counter() - started) * 1000
                row = {"model": model_path.name, "repeat": repeat, "frame": index,
                       "bag_offset_s": offset, "pipeline_ms": elapsed,
                       "inference_ms": detector.last_speed["inference"],
                       "preprocess_ms": detector.last_speed["preprocess"],
                       "postprocess_ms": detector.last_speed["postprocess"],
                       "detections": len(detections)}
                model_rows.append(row)
                rows.append(row)
        pipeline = stats([r["pipeline_ms"] for r in model_rows], np)
        entry = {"file": model_path.name, "bytes": model_path.stat().st_size,
                 "sha256": sha256(model_path), "samples": len(model_rows),
                 "pipeline_ms": pipeline,
                 "inference_ms": stats([r["inference_ms"] for r in model_rows], np),
                 "pipeline_fps": 1000 / pipeline["mean"],
                 "mean_detections": float(np.mean([r["detections"] for r in model_rows])),
                 "predictor_imgsz": detector.model.predictor.imgsz}
        report["models"].append(entry)
        print(json.dumps(entry, ensure_ascii=False), flush=True)
        del detector
        torch.cuda.empty_cache()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
    with args.output.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]), lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"Saved {args.output} and {args.output.with_suffix('.csv')}")


if __name__ == "__main__":
    main()
