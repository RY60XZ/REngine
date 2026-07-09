#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CHECKPOINT = ROOT / "models/eval_mlp/eval_mlp.pt"
DEFAULT_OUT = ROOT / "models/eval_mlp/eval_mlp.nnue"

MAGIC = b"RENNUE4\0"
FORMAT_VERSION = 4
EXPECTED_ARCHITECTURE = "halfkp_lite_two_accumulator_residual"
ACCUMULATOR_SCALE = 1024
FC1_WEIGHT_SCALE = 2048
OUT_WEIGHT_SCALE = 2048


def quantize_tensor(tensor: torch.Tensor, scale: int, dtype: torch.dtype, name: str) -> torch.Tensor:
    quantized = torch.round(tensor.detach().cpu() * scale)
    info = torch.iinfo(dtype)
    min_value = int(quantized.min().item())
    max_value = int(quantized.max().item())
    if min_value < info.min or max_value > info.max:
        raise ValueError(
            f"{name} quantization overflow: range [{min_value}, {max_value}] "
            f"does not fit {dtype}"
        )
    return quantized.to(dtype).contiguous()


def write_tensor(handle, tensor: torch.Tensor) -> None:
    dtype = tensor.numpy().dtype
    if dtype.kind == "i" and dtype.itemsize == 2:
        array = tensor.numpy().astype("<i2", copy=False)
    elif dtype.kind == "i" and dtype.itemsize == 4:
        array = tensor.numpy().astype("<i4", copy=False)
    else:
        raise TypeError(f"unsupported export dtype: {dtype}")
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
    embed_q = quantize_tensor(embed, ACCUMULATOR_SCALE, torch.int16, "embed")
    pad_base_q = quantize_tensor(pad_base, ACCUMULATOR_SCALE, torch.int32, "pad_base")
    fc1_weight_q = quantize_tensor(fc1_weight, FC1_WEIGHT_SCALE, torch.int16, "fc1_weight")
    fc1_bias_q = quantize_tensor(fc1_bias, ACCUMULATOR_SCALE * FC1_WEIGHT_SCALE, torch.int32, "fc1_bias")
    out_weight_q = quantize_tensor(out_weight, OUT_WEIGHT_SCALE, torch.int16, "out_weight")
    out_bias_q = quantize_tensor(out_bias, ACCUMULATOR_SCALE * OUT_WEIGHT_SCALE, torch.int32, "out_bias")

    expected_shapes = {
        "embed": (features, hidden),
        "pad_base": (hidden,),
        "fc1_weight": (head_hidden, hidden * 2),
        "fc1_bias": (head_hidden,),
        "out_weight": (head_hidden,),
        "out_bias": (1,),
    }
    actual_shapes = {
        "embed": tuple(embed_q.shape),
        "pad_base": tuple(pad_base_q.shape),
        "fc1_weight": tuple(fc1_weight_q.shape),
        "fc1_bias": tuple(fc1_bias_q.shape),
        "out_weight": tuple(out_weight_q.shape),
        "out_bias": tuple(out_bias_q.shape),
    }
    if actual_shapes != expected_shapes:
        raise ValueError(f"shape mismatch: expected {expected_shapes}, got {actual_shapes}")

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as handle:
        handle.write(MAGIC)
        handle.write(
            struct.pack(
                "<6I2f3I",
                FORMAT_VERSION,
                features,
                hidden,
                head_hidden,
                max_active,
                0,
                target_scale,
                activation_clip,
                ACCUMULATOR_SCALE,
                FC1_WEIGHT_SCALE,
                OUT_WEIGHT_SCALE,
            )
        )
        write_tensor(handle, embed_q)
        write_tensor(handle, pad_base_q)
        write_tensor(handle, fc1_weight_q)
        write_tensor(handle, fc1_bias_q)
        write_tensor(handle, out_weight_q)
        write_tensor(handle, out_bias_q)

    print(
        f"exported {out_path} "
        f"(features={features}, hidden={hidden}, head_hidden={head_hidden}, "
        f"target_scale={target_scale:g}, activation_clip={activation_clip:g}, "
        f"scales={ACCUMULATOR_SCALE}/{FC1_WEIGHT_SCALE}/{OUT_WEIGHT_SCALE})"
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
