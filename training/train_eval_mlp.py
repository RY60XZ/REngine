#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import math
import time
import zlib
from pathlib import Path

import torch
from torch import nn
import torch.nn.functional as F


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CSV = ROOT / "data/lichess_evals/processed/lichess_db_eval.first_2GB.depth_10_20.csv"
DEFAULT_OUT = ROOT / "models/eval_mlp/eval_mlp.pt"

KING_BUCKETS = 64
PIECE_KINDS = 12
SQUARES = 64
HALFKP_FEATURES = KING_BUCKETS * PIECE_KINDS * SQUARES
CASTLING_FEATURES = 4
CASTLING_FEATURE_BASE = HALFKP_FEATURES
FEATURES = HALFKP_FEATURES + CASTLING_FEATURES
PAD = FEATURES
MAX_ACTIVE = 36
PIECE_TYPE = {"p": 0, "n": 1, "b": 2, "r": 3, "q": 4, "k": 5}
WHITE, BLACK = 0, 1

PIECE_VALUES = [100, 320, 330, 500, 900, 0]
PIECE_SQUARE_VALUES = [
    [
         0,   0,   0,   0,   0,   0,   0,   0,
         5,  10,  10,  -5,  -5,  10,  10,   5,
         5,  -5, -10,   0,   0, -10,  -5,   5,
         0,   0,   0,  20,  20,   0,   0,   0,
         5,   5,  10,  25,  25,  10,   5,   5,
        10,  10,  20,  30,  30,  20,  10,  10,
        50,  50,  50,  50,  50,  50,  50,  50,
         0,   0,   0,   0,   0,   0,   0,   0,
    ],
    [
       -50, -35, -30, -25, -25, -30, -35, -50,
       -35, -20,   0,   5,   5,   0, -20, -35,
       -30,   5,  10,  15,  15,  10,   5, -30,
       -25,   5,  15,  20,  20,  15,   5, -25,
       -25,   5,  15,  20,  20,  15,   5, -25,
       -30,   5,  10,  15,  15,  10,   5, -30,
       -35, -20,   0,   5,   5,   0, -20, -35,
       -50, -35, -30, -25, -25, -30, -35, -50,
    ],
    [
       -20, -10, -10, -10, -10, -10, -10, -20,
       -10,   5,   0,   0,   0,   0,   5, -10,
       -10,  10,  10,  10,  10,  10,  10, -10,
       -10,   0,  10,  15,  15,  10,   0, -10,
       -10,   5,  10,  15,  15,  10,   5, -10,
       -10,   0,  10,  10,  10,  10,   0, -10,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -20, -10, -10, -10, -10, -10, -10, -20,
    ],
    [
         0,   0,   5,  10,  10,   5,   0,   0,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
        -5,   0,   0,   0,   0,   0,   0,  -5,
         5,  10,  10,  10,  10,  10,  10,   5,
         0,   0,   5,  10,  10,   5,   0,   0,
    ],
    [
       -20, -10, -10,  -5,  -5, -10, -10, -20,
       -10,   0,   0,   0,   0,   0,   0, -10,
       -10,   0,   5,   5,   5,   5,   0, -10,
        -5,   0,   5,  10,  10,   5,   0,  -5,
         0,   0,   5,  10,  10,   5,   0,  -5,
       -10,   5,   5,   5,   5,   5,   0, -10,
       -10,   0,   5,   0,   0,   0,   0, -10,
       -20, -10, -10,  -5,  -5, -10, -10, -20,
    ],
    [
        20,  30,  10,   0,   0,  10,  30,  20,
        20,  20,   0,   0,   0,   0,  20,  20,
       -10, -20, -20, -20, -20, -20, -20, -10,
       -20, -30, -30, -40, -40, -30, -30, -20,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
       -30, -40, -40, -50, -50, -40, -40, -30,
    ],
]


def relative_square(square: int, perspective: int) -> int:
    return square if perspective == WHITE else square ^ 56


def halfkp_feature(
    king_square: int,
    piece_color: int,
    piece_type: int,
    piece_square: int,
    perspective: int,
) -> int:
    rel_king = relative_square(king_square, perspective)
    rel_square = relative_square(piece_square, perspective)
    rel_piece = (0 if piece_color == perspective else 1) * 6 + piece_type
    return (rel_king * PIECE_KINDS + rel_piece) * SQUARES + rel_square


def append_aux_features(feats: list[int], castling: str, perspective: int) -> None:
    if perspective == WHITE:
        rights = ("K", "Q", "k", "q")
    else:
        rights = ("k", "q", "K", "Q")

    for offset, marker in enumerate(rights):
        if marker in castling:
            feats.append(CASTLING_FEATURE_BASE + offset)


def features_for_perspective(
    pieces: list[tuple[int, int, int]],
    kings: dict[int, int],
    castling: str,
    perspective: int,
) -> list[int]:
    king_square = kings[perspective]
    feats = [
        halfkp_feature(king_square, color, ptype, square, perspective)
        for color, ptype, square in pieces
    ]
    append_aux_features(feats, castling, perspective)

    if len(feats) > MAX_ACTIVE:
        raise ValueError("too many active features")
    return feats + [PAD] * (MAX_ACTIVE - len(feats))


def fen6_to_features(fen6: str) -> tuple[list[int], list[int], int]:
    fields = fen6.split()
    board, stm = fields[:2]
    castling = fields[2] if len(fields) > 2 else "-"
    side_to_move = WHITE if stm == "w" else BLACK
    pieces: list[tuple[int, int, int]] = []
    kings: dict[int, int | None] = {WHITE: None, BLACK: None}
    rank, file = 7, 0

    for ch in board:
        if ch == "/":
            rank, file = rank - 1, 0
        elif ch.isdigit():
            file += int(ch)
        else:
            color = WHITE if ch.isupper() else BLACK
            ptype = PIECE_TYPE[ch.lower()]
            square = rank * 8 + file
            pieces.append((color, ptype, square))
            if ptype == PIECE_TYPE["k"]:
                kings[color] = square
            file += 1

    if kings[WHITE] is None or kings[BLACK] is None:
        raise ValueError(f"missing king in FEN: {fen6}")

    concrete_kings = {WHITE: int(kings[WHITE]), BLACK: int(kings[BLACK])}
    return (
        features_for_perspective(pieces, concrete_kings, castling, WHITE),
        features_for_perspective(pieces, concrete_kings, castling, BLACK),
        side_to_move,
    )


def row_cp_from_white_perspective(row: dict[str, str]) -> float | None:
    cp = (row.get("cp") or "").strip()
    if cp:
        return float(cp)
    return None


def side_to_move_cp(fen6: str, white_cp: float) -> float:
    side_to_move = fen6.split()[1]
    return white_cp if side_to_move == "w" else -white_cp


def pst_eval_cp(fen6: str) -> float:
    fields = fen6.split()
    board, stm = fields[:2]
    score = [0, 0]
    rank, file = 7, 0

    for ch in board:
        if ch == "/":
            rank, file = rank - 1, 0
        elif ch.isdigit():
            file += int(ch)
        else:
            color = WHITE if ch.isupper() else BLACK
            ptype = PIECE_TYPE[ch.lower()]
            square = rank * 8 + file
            pst_square = square if color == WHITE else square ^ 56
            score[color] += PIECE_VALUES[ptype] + PIECE_SQUARE_VALUES[ptype][pst_square]
            file += 1

    white_minus_black = score[WHITE] - score[BLACK]
    return float(white_minus_black if stm == "w" else -white_minus_black)


def squash_cp(cp: float, squash_cp: float) -> float:
    if squash_cp <= 0:
        return cp
    return squash_cp * math.tanh(cp / squash_cp)


def target_cp_from_row(row: dict[str, str], target_squash_cp: float) -> float | None:
    fen6 = row["fen6"]
    white_cp = row_cp_from_white_perspective(row)
    if white_cp is None:
        return None
    return squash_cp(side_to_move_cp(fen6, white_cp), target_squash_cp)


def is_val(fen6: str, val_mod: int) -> bool:
    return zlib.crc32(fen6.encode("utf-8")) % val_mod == 0


def batches(
    csv_path: Path,
    batch_size: int,
    target_scale: float,
    target_squash_cp: float,
    want_val: bool,
    val_mod: int,
):
    xs_white, xs_black, stms, ys = [], [], [], []
    with csv_path.open("r", encoding="utf-8", newline="") as f:
        for row in csv.DictReader(f):
            fen6 = row["fen6"]
            if is_val(fen6, val_mod) != want_val:
                continue
            cp = target_cp_from_row(row, target_squash_cp)
            if cp is None:
                continue
            try:
                white_features, black_features, stm = fen6_to_features(fen6)
            except (KeyError, ValueError):
                continue
            xs_white.append(white_features)
            xs_black.append(black_features)
            stms.append(stm)
            ys.append(cp / target_scale)
            if len(ys) == batch_size:
                yield (
                    torch.tensor(xs_white, dtype=torch.long),
                    torch.tensor(xs_black, dtype=torch.long),
                    torch.tensor(stms, dtype=torch.long),
                    torch.tensor(ys, dtype=torch.float32),
                )
                xs_white, xs_black, stms, ys = [], [], [], []
    if ys:
        yield (
            torch.tensor(xs_white, dtype=torch.long),
            torch.tensor(xs_black, dtype=torch.long),
            torch.tensor(stms, dtype=torch.long),
            torch.tensor(ys, dtype=torch.float32),
        )


class EvalNNUE(nn.Module):
    def __init__(self, hidden: int, head_hidden: int, activation_clip: float):
        super().__init__()
        self.activation_clip = activation_clip
        self.embed = nn.Embedding(FEATURES + 1, hidden, padding_idx=PAD)
        self.fc1 = nn.Linear(hidden * 2, head_hidden)
        self.out = nn.Linear(head_hidden, 1)
        nn.init.normal_(self.embed.weight, mean=0.0, std=1.0 / math.sqrt(hidden))
        with torch.no_grad():
            self.embed.weight[PAD].zero_()

    def forward(self, white_features: torch.Tensor, black_features: torch.Tensor, stm: torch.Tensor) -> torch.Tensor:
        white_acc = self.embed(white_features).sum(dim=1).clamp(0.0, self.activation_clip)
        black_acc = self.embed(black_features).sum(dim=1).clamp(0.0, self.activation_clip)
        black_to_move = stm.to(torch.bool).unsqueeze(1)
        first = torch.where(black_to_move, black_acc, white_acc)
        second = torch.where(black_to_move, white_acc, black_acc)
        h = torch.cat((first, second), dim=1)
        h = self.fc1(h).clamp(0.0, self.activation_clip)
        return self.out(h).squeeze(1)


@torch.no_grad()
def mae_cp(model: EvalNNUE, csv_path: Path, args) -> float:
    model.eval()
    total_abs, n = 0.0, 0
    for x_white, x_black, stm, y in batches(
        csv_path,
        args.batch_size,
        args.target_scale,
        args.target_squash_cp,
        True,
        args.val_mod,
    ):
        x_white = x_white.to(args.device)
        x_black = x_black.to(args.device)
        stm = stm.to(args.device)
        y = y.to(args.device)
        pred = model(x_white, x_black, stm)
        total_abs += (pred - y).abs().sum().item() * args.target_scale
        n += y.numel()
    return total_abs / max(n, 1)


def pst_mae_cp(csv_path: Path, args) -> float:
    total_abs, n = 0.0, 0
    with csv_path.open("r", encoding="utf-8", newline="") as f:
        for row in csv.DictReader(f):
            fen6 = row["fen6"]
            if not is_val(fen6, args.val_mod):
                continue
            target = target_cp_from_row(row, args.target_squash_cp)
            if target is None:
                continue
            pred = squash_cp(pst_eval_cp(fen6), args.target_squash_cp)
            total_abs += abs(pred - target)
            n += 1
    return total_abs / max(n, 1)


def train(args) -> None:
    torch.manual_seed(args.seed)
    model = EvalNNUE(args.hidden, args.head_hidden, args.activation_clip).to(args.device)
    opt = torch.optim.AdamW(model.parameters(), lr=args.lr, weight_decay=args.weight_decay)
    pst_val_mae = pst_mae_cp(args.csv, args)
    val_name = "val_squashed_mae" if args.target_squash_cp > 0 else "val_mae"
    print(f"pst_{val_name}={pst_val_mae:.1f}cp")

    for epoch in range(1, args.epochs + 1):
        model.train()
        started = time.time()
        total_loss, n = 0.0, 0
        for x_white, x_black, stm, y in batches(
            args.csv,
            args.batch_size,
            args.target_scale,
            args.target_squash_cp,
            False,
            args.val_mod,
        ):
            x_white = x_white.to(args.device)
            x_black = x_black.to(args.device)
            stm = stm.to(args.device)
            y = y.to(args.device)
            opt.zero_grad(set_to_none=True)
            loss = F.smooth_l1_loss(model(x_white, x_black, stm), y, beta=args.huber_beta)
            loss.backward()
            opt.step()
            total_loss += loss.item() * y.numel()
            n += y.numel()

        val_mae = mae_cp(model, args.csv, args)
        print(
            f"epoch {epoch:02d} "
            f"loss={total_loss / max(n, 1):.5f} "
            f"mlp_{val_name}={val_mae:.1f}cp "
            f"pst_{val_name}={pst_val_mae:.1f}cp "
            f"time={time.time() - started:.1f}s"
        )

    args.out.parent.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "model": model.state_dict(),
            "architecture": "halfkp_lite_two_accumulator",
            "hidden": args.hidden,
            "head_hidden": args.head_hidden,
            "features": FEATURES,
            "halfkp_features": HALFKP_FEATURES,
            "king_buckets": KING_BUCKETS,
            "piece_kinds": PIECE_KINDS,
            "squares": SQUARES,
            "pad": PAD,
            "max_active": MAX_ACTIVE,
            "target_scale": args.target_scale,
            "target_squash_cp": args.target_squash_cp,
            "activation_clip": args.activation_clip,
            "csv_target_perspective": "white",
            "trained_target_perspective": "side_to_move",
            "pst_baseline": "copied from cpp_engine/src/eval.cpp: material + PST, side-to-move-relative",
            "feature_mapping": "0..49151 HalfKP: rel_king*768 + rel_piece*64 + rel_square; 49152..49155 castling own-ks,own-qs,opp-ks,opp-qs per accumulator perspective",
        },
        args.out,
    )
    args.out.with_suffix(".json").write_text(json.dumps(vars(args), indent=2, default=str) + "\n")
    print(f"saved {args.out}")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--csv", type=Path, default=DEFAULT_CSV)
    p.add_argument("--out", type=Path, default=DEFAULT_OUT)
    p.add_argument("--hidden", type=int, default=256)
    p.add_argument("--head-hidden", type=int, default=32)
    p.add_argument("--epochs", type=int, default=5)
    p.add_argument("--batch-size", type=int, default=4096)
    p.add_argument("--lr", type=float, default=1e-3)
    p.add_argument("--weight-decay", type=float, default=1e-5)
    p.add_argument("--target-scale", type=float, default=1000.0)
    p.add_argument("--target-squash-cp", type=float, default=3000.0, help="use N*tanh(cp/N); set <=0 to disable")
    p.add_argument("--activation-clip", type=float, default=1.0)
    p.add_argument("--huber-beta", type=float, default=1.0)
    p.add_argument("--val-mod", type=int, default=20, help="1/val_mod positions go to validation")
    p.add_argument("--seed", type=int, default=1)
    p.add_argument("--device", default="mps" if torch.backends.mps.is_available() else "cpu")
    args = p.parse_args()
    train(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
