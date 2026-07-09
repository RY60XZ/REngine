#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CHECKPOINT = ROOT / "models/eval_mlp/eval_mlp.pt"
DEFAULT_OUT = ROOT / "models/eval_mlp/eval_mlp.nnue"

MAGIC = b"RENNUE3\0"
FORMAT_VERSION = 3
EXPECTED_ARCHITECTURE = "halfkp_lite_two_accumulator_residual"


def write_tensor(handle, tensor: torch.Tensor) -> None:
    array = tensor.detach().cpu().contiguous().numpy().astype("<f4", copy=False)
    array.tofile(handle)


def export_nnue(checkpoint_path: Path, out_path: Path) -> None:
    checkpoint = torch.load(checkpoint_path, map_location="cpu")
    if checkpoint.get("architecture") != EXPECTED_ARCHITECTURE:
        raise ValueError(f"unexpected architecture: {checkpoint.get('architecture')!r}")
    if checkpoint.get("trained_output") != "residual_correction_cp":
        raise ValueError(f"expected residual checkpoint, got {checkpoint.get('trained_output')!r}")

    state = checkpoint["model"]
    features = int(checkpoint["features"])
    hidden = int(checkpoint["hidden"])
    head_hidden = int(checkpoint["head_hidden"])
    max_active = int(checkpoint["max_active"])
    target_scale = float(checkpoint["target_scale"])
    activation_clip = float(checkpoint["activation_clip"])

    pad_row = state["embed.weight"][features]
    embed = state["embed.weight"][:features] - pad_row.unsqueeze(0)
    pad_base = pad_row * max_active
    fc1_weight = state["fc1.weight"]
    fc1_bias = state["fc1.bias"]
    out_weight = state["out.weight"].reshape(-1)
    out_bias = state["out.bias"].reshape(-1)

    expected_shapes = {
        "embed": (features, hidden),
        "pad_base": (hidden,),
        "fc1_weight": (head_hidden, hidden * 2),
        "fc1_bias": (head_hidden,),
        "out_weight": (head_hidden,),
        "out_bias": (1,),
    }
    actual_shapes = {
        "embed": tuple(embed.shape),
        "pad_base": tuple(pad_base.shape),
        "fc1_weight": tuple(fc1_weight.shape),
        "fc1_bias": tuple(fc1_bias.shape),
        "out_weight": tuple(out_weight.shape),
        "out_bias": tuple(out_bias.shape),
    }
    if actual_shapes != expected_shapes:
        raise ValueError(f"shape mismatch: expected {expected_shapes}, got {actual_shapes}")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(
            struct.pack(
                "<6I2f",
                FORMAT_VERSION,
                features,
                hidden,
                head_hidden,
                max_active,
                0,
                target_scale,
                activation_clip,
            )
        )
        write_tensor(handle, embed)
        write_tensor(handle, pad_base)
        write_tensor(handle, fc1_weight)
        write_tensor(handle, fc1_bias)
        write_tensor(handle, out_weight)
        write_tensor(handle, out_bias)

    print(
        f"exported {out_path} "
        f"(features={features}, hidden={hidden}, head_hidden={head_hidden}, "
        f"target_scale={target_scale:g}, activation_clip={activation_clip:g})"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=Path, default=DEFAULT_CHECKPOINT)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    export_nnue(args.checkpoint, args.out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
