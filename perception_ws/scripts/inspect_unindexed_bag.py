#!/usr/bin/env python3
"""Inspect an unindexed ROS1 bag with a read-only file handle; never repairs it."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bag", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    import rosbag
    report = {"path": str(args.bag), "bytes": args.bag.stat().st_size,
              "mode": "read-only; in-memory index scan", "scan_error": None}
    with rosbag.Bag(str(args.bag), mode="r", allow_unindexed=True) as bag:
        assert bag._file.mode == "rb", "Refusing to inspect a writable bag"
        chunks = 0
        try:
            for position in bag.reindex():
                chunks += 1
                if chunks % 1000 == 0:
                    print(f"Scanned {chunks} chunks, offset {position / 1024**3:.2f} GiB", flush=True)
        except Exception as exc:
            # rosbag may attempt to truncate a corrupt tail. The rb handle
            # prevents this and retains the readable chunks' in-memory index.
            report["scan_error"] = f"{type(exc).__name__}: {exc}"
        report["scanned_chunks"] = chunks
        report["topics"] = {name: {"type": info.msg_type, "messages": info.message_count}
                            for name, info in bag.get_type_and_topic_info().topics.items()}
        try:
            report["duration_s"] = bag.get_end_time() - bag.get_start_time()
        except Exception as exc:
            report["duration_error"] = str(exc)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    print(json.dumps(report, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
