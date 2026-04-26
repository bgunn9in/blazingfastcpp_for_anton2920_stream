#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import math
import os
import shutil
import subprocess
import sys
from pathlib import Path


TARGETS = ("ispc_parallel", "blazingfastcpp")


def run_command(args: list[str], cwd: Path) -> None:
    print("+ " + " ".join(args), flush=True)
    subprocess.run(args, cwd=cwd, check=True)


def executable_path(build_dir: Path, config: str, name: str) -> Path:
    suffixes = (".exe", "") if os.name == "nt" else ("", ".exe")
    candidates: list[Path] = []

    if config:
        for suffix in suffixes:
            candidates.append(build_dir / config / f"{name}{suffix}")
    for suffix in suffixes:
        candidates.append(build_dir / f"{name}{suffix}")

    for candidate in candidates:
        if candidate.is_file():
            return candidate

    searched = "\n  ".join(str(path) for path in candidates)
    raise FileNotFoundError(f"Executable for {name!r} not found. Searched:\n  {searched}")


def parse_one_csv(output: str, name: str) -> dict[str, str]:
    rows = list(csv.DictReader(output.splitlines()))
    if len(rows) != 1:
        raise RuntimeError(f"Unexpected CSV output from {name}: {output!r}")
    return rows[0]


def run_one(
    exe: Path,
    name: str,
    run_index: int,
    frames: int,
    warmup: int,
    env: dict[str, str] | None,
) -> dict[str, object]:
    completed = subprocess.run(
        [
            str(exe),
            "--frames",
            str(frames),
            "--warmup",
            str(warmup),
            "--no-output",
            "--csv",
        ],
        check=True,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if completed.stderr:
        print(completed.stderr, file=sys.stderr, end="")

    row = parse_one_csv(completed.stdout, name)
    return {
        "implementation": name,
        "run": run_index,
        "width": int(row["width"]),
        "height": int(row["height"]),
        "frames": int(row["frames"]),
        "warmup": int(row["warmup"]),
        "total_ms": float(row["total_ms"]),
        "avg_ms": float(row["avg_ms"]),
        "fps": float(row["fps"]),
    }


def stats(rows: list[dict[str, object]], name: str) -> dict[str, float | int | str]:
    values = [float(row["avg_ms"]) for row in rows if row["implementation"] == name]
    fps_values = [float(row["fps"]) for row in rows if row["implementation"] == name]
    if not values:
        raise RuntimeError(f"No benchmark rows for {name}")

    mean = sum(values) / len(values)
    variance = sum((value - mean) ** 2 for value in values) / len(values)
    return {
        "implementation": name,
        "runs": len(values),
        "avg_ms": mean,
        "min_ms": min(values),
        "max_ms": max(values),
        "stddev_ms": math.sqrt(variance),
        "fps": sum(fps_values) / len(fps_values),
    }


def print_summary(
    summary: list[dict[str, float | int | str]],
    frames: int,
    warmup: int,
    runs: int,
    config: str,
    omp_num_threads: int | None,
) -> None:
    baseline = next(item for item in summary if item["implementation"] == "ispc_parallel")
    baseline_avg = float(baseline["avg_ms"])
    rows = []
    for item in summary:
        speedup = baseline_avg / float(item["avg_ms"])
        rows.append(
            [
                str(item["implementation"]),
                str(item["runs"]),
                f"{float(item['avg_ms']):.4f}",
                f"{float(item['min_ms']):.4f}",
                f"{float(item['max_ms']):.4f}",
                f"{float(item['stddev_ms']):.4f}",
                f"{float(item['fps']):.2f}",
                f"{speedup:.3f}x",
            ]
        )

    headers = ["implementation", "runs", "avg_ms", "min_ms", "max_ms", "stddev_ms", "fps", "speedup_vs_ispc"]
    widths = [len(header) for header in headers]
    for row in rows:
        widths = [max(width, len(value)) for width, value in zip(widths, row)]

    print()
    thread_info = f" omp_num_threads={omp_num_threads}" if omp_num_threads is not None else ""
    print(f"Benchmark: frames={frames} warmup={warmup} runs={runs} config={config}{thread_info}")
    print("  ".join(header.ljust(width) for header, width in zip(headers, widths)))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(value.ljust(width) for value, width in zip(row, widths)))

    fastest = min(summary, key=lambda item: float(item["avg_ms"]))
    print(f"Fastest: {fastest['implementation']} ({float(fastest['avg_ms']):.4f} ms/frame)")


def write_csv(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = ["implementation", "run", "width", "height", "frames", "warmup", "total_ms", "avg_ms", "fps"]
    with path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.DictWriter(file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Raw results written to {path}")


def main() -> int:
    script_dir = Path(__file__).resolve().parent
    source_dir = script_dir.parent

    parser = argparse.ArgumentParser(description="Build and compare ispc_parallel vs blazingfastcpp.")
    parser.add_argument("--frames", type=int, default=300, help="Measured frames per run.")
    parser.add_argument("--warmup", type=int, default=20, help="Warmup frames before timing.")
    parser.add_argument("--runs", type=int, default=7, help="Benchmark runs per executable.")
    parser.add_argument("--build-dir", type=Path, default=source_dir / "build", help="CMake build directory.")
    parser.add_argument("--config", default="Release", help="CMake build configuration for multi-config generators.")
    parser.add_argument("--out-csv", type=Path, default=None, help="Write per-run raw results to CSV.")
    parser.add_argument("--skip-build", action="store_true", help="Do not configure/build before running.")
    parser.add_argument("--omp-num-threads", type=int, default=None, help="Set OMP_NUM_THREADS for benchmarked processes.")
    args = parser.parse_args()

    if args.frames <= 0:
        parser.error("--frames must be greater than zero")
    if args.warmup < 0:
        parser.error("--warmup must be zero or greater")
    if args.runs <= 0:
        parser.error("--runs must be greater than zero")
    if args.omp_num_threads is not None and args.omp_num_threads <= 0:
        parser.error("--omp-num-threads must be greater than zero")

    build_dir = args.build_dir.resolve()
    if not args.skip_build:
        cmake = shutil.which("cmake")
        if not cmake:
            raise RuntimeError("cmake not found in PATH")
        run_command([cmake, "-S", str(source_dir), "-B", str(build_dir)], cwd=source_dir)
        run_command([cmake, "--build", str(build_dir), "--config", args.config], cwd=source_dir)

    executables = {name: executable_path(build_dir, args.config, name) for name in TARGETS}
    child_env = os.environ.copy()
    if args.omp_num_threads is not None:
        child_env["OMP_NUM_THREADS"] = str(args.omp_num_threads)

    rows: list[dict[str, object]] = []
    for run_index in range(1, args.runs + 1):
        for name in TARGETS:
            rows.append(run_one(executables[name], name, run_index, args.frames, args.warmup, child_env))

    if args.out_csv is not None:
        write_csv(args.out_csv.resolve(), rows)

    summary = [stats(rows, name) for name in TARGETS]
    print_summary(summary, args.frames, args.warmup, args.runs, args.config, args.omp_num_threads)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
