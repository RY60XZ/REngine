# REngine

REngine is a from-scratch C++ chess engine with bitboard move generation, alpha-beta search, and a Stockfish-label-distilled NNUE-style evaluation.

The repository also includes the data pipeline and training/export tooling used to build the neural evaluation weights.

## Engine

The C++ engine lives in `cpp_engine/`.

Implemented engine components:

- Bitboard board representation with one bitboard per piece type and color
- Magic-bitboard sliding attacks for bishops, rooks, and queens
- Precomputed attack tables for kings, knights, and pawns
- Pseudo-legal and legal move generation
- Full make/unmake support for quiet moves, captures, en passant, castling, promotions, and null moves
- Zobrist hashing
- FEN parsing and serialization
- Perft test coverage for move-generation correctness
- UCI protocol loop
- CLI search runner with depth, node, movetime, infinite, and FEN options
- Interactive terminal play mode

## Search

Implemented search features:

- Iterative deepening
- Alpha-beta negamax
- Quiescence search
- Transposition table with mate-distance-aware score storage
- Hash move ordering
- MVV-LVA capture ordering
- Static exchange evaluation for capture ordering
- SEE pruning of losing captures in quiescence search
- Killer move heuristic
- History heuristic
- Null-move pruning
- Late move reductions
- Draw detection for fifty-move rule, insufficient material, and threefold repetition
- Mate-score helpers for stable checkmate distance handling

## Evaluation

The current evaluation is `PST + NNUE residual`.

The baseline term is a material plus piece-square-table evaluator. The neural term is trained as a residual correction against Stockfish eval labels, so the final runtime score is:

```text
final_eval_cp = pst_eval_cp(position) + nnue_residual_cp(position)
```

Implemented NNUE features:

- HalfKP-style sparse piece features
- Two accumulators, one from White's perspective and one from Black's perspective
- Side-to-move-relative output ordering
- Castling-right features
- Incremental accumulator updates on make/unmake
- Reverse accumulator deltas on unmake instead of full accumulator snapshots
- Quantized integer runtime format
- Automatic model loading from `models/eval_mlp/eval_mlp.nnue`
- Optional model override through `RENGINE_NNUE`

The exported runtime weight uses the `RENNUE4` binary format with int16 feature/layer weights and int32 accumulators/biases.

## Data And Training

The project includes tools for building the NNUE training set from Lichess Stockfish eval data.

Implemented pipeline pieces:

- Download Lichess eval data from `database.lichess.org`
- Stream and decompress zstd eval files
- Parse FEN/eval entries into CSV
- Keep `fen6`, `depth`, `cp`, and `mate`
- Convert mate labels into large centipawn targets
- Depth filtering
- Static-position deduplication
- Highest-depth label selection for duplicate positions

Training code lives in `training/train_eval_mlp.py`.

Implemented training/export pieces:

- PyTorch sparse-feature NNUE-style model
- Residual training against `Stockfish side-to-move target - PST baseline`
- Squashed centipawn targets
- Validation MAE reporting for PST baseline and `PST + NNUE`
- Quantized export script at `training/export_eval_nnue.py`
- Runtime model output at `models/eval_mlp/eval_mlp.nnue`

## Build

Use a Release build for meaningful search and perft numbers.

```sh
cd cpp_engine
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

Useful targets:

```sh
cmake --build cmake-build-release --target cpp_engine
cmake --build cmake-build-release --target play
cmake --build cmake-build-release --target test_movegen
cmake --build cmake-build-release --target test_make_unmake
cmake --build cmake-build-release --target test_search_smoke
cmake --build cmake-build-release --target test_nnue
cmake --build cmake-build-release --target perft_test
cmake --build cmake-build-release --target bench_perft
cmake --build cmake-build-release --target bench_search
```

Run the full test suite:

```sh
ctest --test-dir cmake-build-release --output-on-failure
```

## Run

Run UCI mode:

```sh
cd cpp_engine
cmake-build-release/cpp_engine --uci
```

Run a search from the starting position:

```sh
cd cpp_engine
cmake-build-release/cpp_engine --depth 10
```

Run a search from a FEN:

```sh
cd cpp_engine
cmake-build-release/cpp_engine --depth 10 --fen "3r1rk1/pp2q2p/1n1b3Q/5p2/Pp1Bp3/4P2P/5P2/2RR2K1 b - - 2 29"
```

Run the interactive terminal player:

```sh
cd cpp_engine
cmake-build-release/play
```

Use a specific NNUE file:

```sh
cd cpp_engine
RENGINE_NNUE=/absolute/path/to/eval_mlp.nnue cmake-build-release/cpp_engine --depth 10
```

## Performance

### Perft

All node counts match canonical published values.

| Position | Depth | Nodes | Time | NPS |
|---|---:|---:|---:|---:|
| startpos | 5 | 4,865,609 | 0.055s | 89.0M |
| startpos | 6 | 119,060,324 | 1.331s | 89.4M |
| kiwipete | 4 | 4,085,603 | 0.048s | 84.5M |
| en passant pressure | 5 | 674,624 | 0.009s | 72.6M |
| promotion pressure | 4 | 2,103,487 | 0.058s | 36.1M |

### Search Smoke From Startpos

Measured with `cmake-build-release/cpp_engine --depth N` from the standard starting position. NPS counts total searched nodes (`nodes + qnodes`) with the quantized NNUE evaluator enabled.

| Depth | Best move | Score | Nodes | QNodes | Total nodes | Time | NPS |
|---|---|---:|---:|---:|---:|---:|---:|
| 8 | e2e4 | 35 | 99,278 | 86,567 | 185,845 | 0.149s | 1.25M |
| 10 | e2e4 | 33 | 595,840 | 538,060 | 1,133,900 | 0.901s | 1.26M |

### Move Generation Comparison

Same M4 hardware, same perft workload.

| Test | python-chess | REngine | Speedup |
|---|---:|---:|---:|
| startpos depth 5 | 3.059s, 1.59M nps | 0.055s, 89.0M nps | 56x |
| startpos depth 6 | 84.53s, 1.41M nps | 1.331s, 89.4M nps | 64x |
| kiwipete depth 4 | 2.552s, 1.60M nps | 0.048s, 84.5M nps | 53x |
| en passant pressure depth 5 | 0.559s, 1.21M nps | 0.009s, 72.6M nps | 62x |
| promotion pressure depth 4 | 1.214s, 1.73M nps | 0.058s, 36.1M nps | 21x |

