#!/usr/bin/env python3
"""
Download the first capped prefix of the Lichess Stockfish eval database and
stream it into a filtered CSV dataset.

Defaults are anchored to the parent REngine directory:
    raw zst: <REngine>/data/lichess_evals/raw/
    csv:     <REngine>/data/lichess_evals/processed/

For each static position, the CSV keeps one eval only: the highest-depth eval
at or above the requested minimum depth whose first PV has either `cp` or
`mate`. Duplicate positions are collapsed by board/side/castling/en-passant
identity while still writing a six-field FEN (`fen6`) to the CSV.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import io
import json
import re
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any


DEFAULT_URL = "https://database.lichess.org/lichess_db_eval.jsonl.zst"
DEFAULT_LIMIT = "1GB"
PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RAW_DIR = PROJECT_ROOT / "data/lichess_evals/raw"
DEFAULT_CSV_DIR = PROJECT_ROOT / "data/lichess_evals/processed"
USER_AGENT = "REngine eval-data downloader/1.0 (+https://database.lichess.org/)"


@dataclass
class DownloadSummary:
    url: str
    output_path: str
    requested_bytes: int
    actual_bytes: int
    resumed_from_bytes: int
    completed: bool
    downloaded_at_utc: str


@dataclass
class ExtractSummary:
    input_path: str
    output_path: str
    records_read: int
    rows_written: int
    duplicate_positions: int
    labels_replaced_by_deeper_eval: int
    skipped_without_eval: int
    skipped_outside_depth: int
    skipped_bad_json: int
    truncated_tail: bool


def parse_size(value: str) -> int:
    match = re.fullmatch(r"\s*(\d+(?:\.\d+)?)\s*([kmgt]?i?b?|b)?\s*", value, re.IGNORECASE)
    if match is None:
        raise argparse.ArgumentTypeError(f"invalid size: {value!r}")

    amount = float(match.group(1))
    unit = (match.group(2) or "B").upper()
    unit = {"K": "KB", "M": "MB", "G": "GB", "T": "TB"}.get(unit, unit)

    multipliers = {
        "B": 1,
        "KB": 1000,
        "MB": 1000**2,
        "GB": 1000**3,
        "TB": 1000**4,
        "KIB": 1024,
        "MIB": 1024**2,
        "GIB": 1024**3,
        "TIB": 1024**4,
    }
    if unit not in multipliers:
        raise argparse.ArgumentTypeError(f"invalid size unit in {value!r}")
    size = int(amount * multipliers[unit])
    if size <= 0:
        raise argparse.ArgumentTypeError("size must be positive")
    return size


def format_bytes(byte_count: int) -> str:
    units = ("B", "KB", "MB", "GB", "TB")
    value = float(byte_count)
    for unit in units:
        if value < 1000 or unit == units[-1]:
            return f"{value:.2f} {unit}" if unit != "B" else f"{byte_count} B"
        value /= 1000
    return f"{byte_count} B"


def size_slug(byte_count: int) -> str:
    if byte_count % 1_000_000_000 == 0:
        return f"{byte_count // 1_000_000_000}GB"
    if byte_count % 1_000_000 == 0:
        return f"{byte_count // 1_000_000}MB"
    return str(byte_count)


def default_output_path(url: str, output_dir: Path, byte_limit: int) -> Path:
    source_name = Path(urllib.parse.urlparse(url).path).name
    slug = size_slug(byte_limit)
    if source_name.endswith(".jsonl.zst"):
        base = source_name.removesuffix(".jsonl.zst")
        name = f"{base}.first_{slug}.jsonl.zst"
    else:
        name = f"{source_name}.first_{slug}"
    return output_dir / name


def print_progress(path: Path, current: int, target: int, started_at: float) -> None:
    elapsed = max(time.monotonic() - started_at, 0.001)
    rate = current / elapsed
    pct = min(100.0, 100.0 * current / target)
    print(
        f"{path.name}: {format_bytes(current)} / {format_bytes(target)} "
        f"({pct:.1f}%, {format_bytes(int(rate))}/s)",
        file=sys.stderr,
        flush=True,
    )


def download_prefix(
    url: str,
    output_path: Path,
    byte_limit: int,
    chunk_size: int,
    timeout: float,
    resume: bool,
) -> DownloadSummary:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    existing_size = output_path.stat().st_size if output_path.exists() else 0

    if existing_size >= byte_limit:
        print(
            f"{output_path} already has at least {format_bytes(byte_limit)}; skipping download.",
            file=sys.stderr,
        )
        return DownloadSummary(
            url=url,
            output_path=str(output_path),
            requested_bytes=byte_limit,
            actual_bytes=existing_size,
            resumed_from_bytes=0,
            completed=True,
            downloaded_at_utc=dt.datetime.now(dt.UTC).isoformat(),
        )

    start = existing_size if resume else 0
    mode = "ab" if start > 0 else "wb"
    headers = {
        "Accept-Encoding": "identity",
        "User-Agent": USER_AGENT,
        "Range": f"bytes={start}-{byte_limit - 1}",
    }
    request = urllib.request.Request(url, headers=headers)

    print(
        f"Downloading {format_bytes(byte_limit)} compressed prefix from {url}",
        file=sys.stderr,
    )
    if start > 0:
        print(f"Resuming at {format_bytes(start)}", file=sys.stderr)

    try:
        response = urllib.request.urlopen(request, timeout=timeout)
    except urllib.error.HTTPError as exc:
        if exc.code == 416 and output_path.exists() and output_path.stat().st_size >= byte_limit:
            return DownloadSummary(
                url=url,
                output_path=str(output_path),
                requested_bytes=byte_limit,
                actual_bytes=output_path.stat().st_size,
                resumed_from_bytes=start,
                completed=True,
                downloaded_at_utc=dt.datetime.now(dt.UTC).isoformat(),
            )
        raise

    with response:
        status = getattr(response, "status", response.getcode())
        if start > 0 and status != 206:
            print(
                "Server did not honor Range resume; restarting this capped download from byte 0.",
                file=sys.stderr,
            )
            start = 0
            mode = "wb"

        current = start
        remaining = byte_limit - current
        started_at = time.monotonic()
        last_progress = 0.0

        with output_path.open(mode) as output:
            while remaining > 0:
                chunk = response.read(min(chunk_size, remaining))
                if not chunk:
                    break
                output.write(chunk)
                current += len(chunk)
                remaining -= len(chunk)

                now = time.monotonic()
                if now - last_progress >= 5.0:
                    print_progress(output_path, current, byte_limit, started_at)
                    last_progress = now

    actual_size = output_path.stat().st_size
    print_progress(output_path, min(actual_size, byte_limit), byte_limit, started_at)

    return DownloadSummary(
        url=url,
        output_path=str(output_path),
        requested_bytes=byte_limit,
        actual_bytes=actual_size,
        resumed_from_bytes=existing_size if resume else 0,
        completed=actual_size >= byte_limit,
        downloaded_at_utc=dt.datetime.now(dt.UTC).isoformat(),
    )


def write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_path = path.with_suffix(path.suffix + ".tmp")
    with temp_path.open("w", encoding="utf-8") as output:
        json.dump(payload, output, indent=2, sort_keys=True)
        output.write("\n")
    temp_path.replace(path)


def fen_with_counters(fen: str) -> str:
    fields = fen.split()
    if len(fields) == 4:
        return f"{fen} 0 1"
    return fen


def position_key(fen: str) -> str:
    fields = fen.split()
    if len(fields) < 4:
        return fen
    return " ".join(fields[:4])


def default_csv_path(output_dir: Path, byte_limit: int, min_depth: int, max_depth: int | None) -> Path:
    depth_slug = f"depth_{min_depth}_plus" if max_depth is None else f"depth_{min_depth}_{max_depth}"
    return output_dir / f"lichess_db_eval.first_{size_slug(byte_limit)}.{depth_slug}.csv"


def mate_to_cp(mate: int) -> int:
    sign = 1 if mate > 0 else -1
    return sign * (20100 - abs(mate))


def pick_highest_depth_eval(
    position: dict[str, Any],
    min_depth: int,
    max_depth: int | None,
    clamp_cp: int | None,
    clamp_mate: int | None,
) -> tuple[tuple[int, int, int], dict[str, Any]] | None:
    best: tuple[int, int, int, dict[str, Any]] | None = None
    for eval_entry in position.get("evals", []):
        depth = int(eval_entry.get("depth", -1))
        if depth < min_depth or (max_depth is not None and depth > max_depth):
            continue

        pvs = eval_entry.get("pvs") or []
        if not pvs:
            continue
        first_pv = pvs[0]
        if "cp" not in first_pv and "mate" not in first_pv:
            continue

        knodes = int(eval_entry.get("knodes", -1))
        cp = ""
        mate = ""
        if "cp" in first_pv:
            cp_value = int(first_pv["cp"])
            if clamp_cp is not None:
                cp_value = max(-clamp_cp, min(clamp_cp, cp_value))
            cp = cp_value
        if "mate" in first_pv:
            mate_value = int(first_pv["mate"])
            if clamp_mate is not None:
                mate_value = max(-clamp_mate, min(clamp_mate, mate_value))
            mate = mate_value
            cp = mate_to_cp(mate_value)

        candidate = (depth, knodes, len(pvs), {
            "fen6": fen_with_counters(position["fen"]),
            "depth": depth,
            "cp": cp,
            "mate": mate,
        })
        if best is None or candidate[:3] > best[:3]:
            best = candidate

    return None if best is None else (best[:3], best[3])


def extract_csv(
    input_path: Path,
    output_path: Path,
    min_depth: int,
    max_depth: int | None,
    clamp_cp: int | None,
    clamp_mate: int | None,
    max_rows: int | None,
) -> ExtractSummary:
    try:
        import zstandard as zstd
    except ImportError as exc:
        zstd = None
        zstd_import_error = exc
    else:
        zstd_import_error = None

    output_path.parent.mkdir(parents=True, exist_ok=True)

    records_read = 0
    rows_written = 0
    duplicate_positions = 0
    labels_replaced_by_deeper_eval = 0
    skipped_without_eval = 0
    skipped_outside_depth = 0
    skipped_bad_json = 0
    truncated_tail = False
    columns = ["fen6", "depth", "cp", "mate"]
    best_by_position: dict[str, tuple[tuple[int, int, int], dict[str, Any]]] = {}
    position_order: list[str] = []

    process: subprocess.Popen[str] | None = None
    text_reader: io.TextIOWrapper | Any | None = None
    reader: Any | None = None

    with input_path.open("rb") as compressed:
        if zstd is not None:
            dctx = zstd.ZstdDecompressor(max_window_size=2**31)
            reader = dctx.stream_reader(compressed)
            text_reader = io.TextIOWrapper(reader, encoding="utf-8")
            zstd_errors = (zstd.ZstdError,)
        else:
            zstd_path = shutil.which("zstd")
            if zstd_path is None:
                raise SystemExit(
                    "CSV extraction needs either the Python `zstandard` package or the `zstd` CLI. "
                    "Install one of them, e.g. `python3 -m pip install zstandard` or `brew install zstd`."
                ) from zstd_import_error
            process = subprocess.Popen(
                [zstd_path, "-dc", str(input_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                encoding="utf-8",
            )
            if process.stdout is None:
                raise SystemExit("failed to open zstd stdout")
            text_reader = process.stdout
            zstd_errors = ()

        try:
            while text_reader is not None:
                line = text_reader.readline()
                if line == "":
                    break
                if not line.endswith("\n"):
                    truncated_tail = True
                    break

                try:
                    position = json.loads(line)
                except json.JSONDecodeError:
                    skipped_bad_json += 1
                    continue

                records_read += 1
                selected = pick_highest_depth_eval(
                    position,
                    min_depth=min_depth,
                    max_depth=max_depth,
                    clamp_cp=clamp_cp,
                    clamp_mate=clamp_mate,
                )
                if selected is None:
                    has_any_scored_eval = any(
                        ("cp" in ((eval_entry.get("pvs") or [{}])[0]) or
                         "mate" in ((eval_entry.get("pvs") or [{}])[0]))
                        for eval_entry in position.get("evals", [])
                        if eval_entry.get("pvs")
                    )
                    if has_any_scored_eval:
                        skipped_outside_depth += 1
                    else:
                        skipped_without_eval += 1
                    continue

                sort_key, label = selected
                key = position_key(position["fen"])
                previous = best_by_position.get(key)
                if previous is None:
                    position_order.append(key)
                    best_by_position[key] = selected
                else:
                    duplicate_positions += 1
                    if sort_key > previous[0]:
                        labels_replaced_by_deeper_eval += 1
                        best_by_position[key] = selected
        except (EOFError, UnicodeDecodeError, *zstd_errors):
            truncated_tail = True
        finally:
            if zstd is not None:
                try:
                    assert isinstance(text_reader, io.TextIOWrapper)
                    text_reader.detach()
                except (AssertionError, ValueError, *zstd_errors):
                    pass
                try:
                    if reader is not None:
                        reader.close()
                except zstd_errors:
                    pass
            elif process is not None:
                if process.stdout is not None:
                    process.stdout.close()
                stderr = process.stderr.read() if process.stderr is not None else ""
                returncode = process.wait()
                if returncode != 0:
                    truncated_tail = True
                    if "unexpected end" not in stderr.lower() and "premature end" not in stderr.lower():
                        print(stderr.strip(), file=sys.stderr)

    with output_path.open("w", encoding="utf-8", newline="") as output:
        csv_writer = csv.DictWriter(output, fieldnames=columns)
        csv_writer.writeheader()
        for key in position_order:
            if max_rows is not None and rows_written >= max_rows:
                break
            selected = best_by_position.get(key)
            if selected is None:
                continue
            csv_writer.writerow(selected[1])
            rows_written += 1

    return ExtractSummary(
        input_path=str(input_path),
        output_path=str(output_path),
        records_read=records_read,
        rows_written=rows_written,
        duplicate_positions=duplicate_positions,
        labels_replaced_by_deeper_eval=labels_replaced_by_deeper_eval,
        skipped_without_eval=skipped_without_eval,
        skipped_outside_depth=skipped_outside_depth,
        skipped_bad_json=skipped_bad_json,
        truncated_tail=truncated_tail,
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download a capped Lichess eval .zst prefix and stream filtered rows into CSV.",
    )
    parser.add_argument("--url", default=DEFAULT_URL, help=f"source URL (default: {DEFAULT_URL})")
    parser.add_argument(
        "--limit",
        default=DEFAULT_LIMIT,
        help="compressed byte cap; supports KB/MB/GB and KiB/MiB/GiB (default: 1GB)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_RAW_DIR,
        help=f"directory for the raw prefix (default: {DEFAULT_RAW_DIR})",
    )
    parser.add_argument("--output", type=Path, help="raw .zst output path")
    parser.add_argument(
        "--csv-output",
        type=Path,
        help=f"processed CSV output path (default: {DEFAULT_CSV_DIR}/lichess_db_eval.first_<limit>.depth_<min>_plus.csv)",
    )
    parser.add_argument("--no-resume", action="store_true", help="restart instead of appending a partial file")
    parser.add_argument("--chunk-size", default="8MiB", help="download chunk size (default: 8MiB)")
    parser.add_argument("--timeout", type=float, default=60.0, help="HTTP timeout in seconds")
    parser.add_argument("--min-depth", type=int, default=20, help="minimum Stockfish depth to keep")
    parser.add_argument("--max-depth", type=int, default=None, help="optional maximum Stockfish depth to keep")
    parser.add_argument("--clamp-cp", type=int, default=None, help="optional absolute centipawn clamp")
    parser.add_argument("--clamp-mate", type=int, default=None, help="optional absolute mate-distance clamp")
    parser.add_argument("--max-rows", type=int, help="optional cap on extracted CSV rows")
    parser.add_argument(
        "--download-only",
        action="store_true",
        help="only download the compressed prefix; skip CSV decompression/extraction",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    byte_limit = parse_size(args.limit)
    chunk_size = parse_size(args.chunk_size)
    output_path = args.output or default_output_path(args.url, args.output_dir, byte_limit)
    csv_output_path = args.csv_output or default_csv_path(
        DEFAULT_CSV_DIR,
        byte_limit,
        args.min_depth,
        args.max_depth,
    )

    if args.max_depth is not None and args.min_depth > args.max_depth:
        raise SystemExit("--min-depth must be <= --max-depth")

    download_summary = download_prefix(
        url=args.url,
        output_path=output_path,
        byte_limit=byte_limit,
        chunk_size=chunk_size,
        timeout=args.timeout,
        resume=not args.no_resume,
    )
    write_json(output_path.with_suffix(output_path.suffix + ".meta.json"), asdict(download_summary))

    if not args.download_only:
        extract_summary = extract_csv(
            input_path=output_path,
            output_path=csv_output_path,
            min_depth=args.min_depth,
            max_depth=args.max_depth,
            clamp_cp=args.clamp_cp,
            clamp_mate=args.clamp_mate,
            max_rows=args.max_rows,
        )
        write_json(csv_output_path.with_suffix(csv_output_path.suffix + ".meta.json"), asdict(extract_summary))
        print(
            f"Wrote {extract_summary.rows_written} CSV rows to {csv_output_path}",
            file=sys.stderr,
        )

    if not download_summary.completed:
        print(
            f"Warning: only downloaded {format_bytes(download_summary.actual_bytes)} "
            f"of requested {format_bytes(download_summary.requested_bytes)}.",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
