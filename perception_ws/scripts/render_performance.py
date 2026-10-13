#!/usr/bin/env python3
"""Render the measured PyTorch/TensorRT timing comparison for the README."""
import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, default=Path("benchmarks/2026-10-14-rtx3070-small-current-bag.json"))
    parser.add_argument("--output", type=Path, default=Path("docs/assets/performance.svg"))
    args = parser.parse_args()
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    data = json.loads(args.result.read_text())
    models = data["models"]
    figure, axes = plt.subplots(1, 2, figsize=(10.4, 3.6), layout="constrained")
    colors = ["#536273", "#29946f"]
    for axis, key, title in zip(axes, ["inference_ms", "pipeline_ms"],
                                ["Model inference", "JPEG decode + detection"]):
        values = [model[key]["mean"] for model in models]
        axis.barh([1, 0], values, height=0.46, color=colors)
        axis.set_yticks([1, 0], ["PyTorch FP32", "TensorRT FP16"])
        axis.set_xlim(0, max(values) * 1.32)
        axis.set_xlabel("Mean time per frame (ms)")
        axis.set_title(f"{title}\n{values[0] / values[1]:.2f}x speedup", fontsize=12, loc="left", pad=13)
        axis.spines[["top", "right", "left"]].set_visible(False)
        axis.tick_params(axis="y", length=0)
        axis.xaxis.grid(True, alpha=0.16)
        axis.set_axisbelow(True)
        for y, value in zip([1, 0], values):
            axis.text(value + max(values) * 0.025, y, f"{value:.2f}", va="center", fontsize=11)
    figure.suptitle("YOLO26s / RTX 3070 Laptop GPU / 640 x 640 / batch 1", fontsize=13, fontweight="bold")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(args.output, bbox_inches="tight", facecolor="white")
    if args.output.suffix.lower() == ".svg":
        # Keep generated vector paths readable and whitespace checks clean.
        args.output.write_text("\n".join(line.rstrip() for line in args.output.read_text().splitlines()) + "\n")
    plt.close(figure)
    print(f"Saved {args.output}")


if __name__ == "__main__":
    main()
