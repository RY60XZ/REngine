# REngine
A from-scratch chess engine written in C++, aiming for a strong neural-network-based NNUE-style evaluation.

## Motivation
This project started as a Python chess engine using `python-chess`. After building out alpha-beta search and basic evaluation, I ran into a wall: `python-chess` topped out at ~1-2M nodes/sec on my hardware, which made anything beyond shallow search painfully slow. Even with a half-decent search algorithm, the engine couldn't see deep enough to play coherently.

So I rewrote the engine in C++ from scratch, with two goals:
1. Make move generation fast enough that search depth is bounded by the search algorithm's intelligence, not by raw speed.
2. Build a foundation capable of supporting modern neural-network-based evaluation in later phases.

## Current Status

## Done
- Bitboard board representation with one bitboard per piece type and color
- Magic-bitboard sliding attacks for bishops, rooks, and queens
- Precomputed attack tables for non-sliding pieces
- Legal and pseudo-legal move generation
- Perft tests for move-generation correctness
- Alpha-beta negamax search with iterative deepening
- Quiescence search with qnode accounting
- Move ordering using captures/promotions, hash move, killer moves, and history heuristic
- Zobrist hashing and fixed-size transposition table
- Mate-distance-aware score storage for TT entries
- Basic material plus piece-square-table evaluation
- CLI search runner with depth, node, movetime, and FEN options

## In Progress / Planned
- Stronger pruning: null-move pruning, late move reductions, futility pruning, and check extensions
- Static exchange evaluation for capture ordering and pruning support
- Aspiration windows around iterative-deepening scores
- Better time management and UCI protocol support
- Stronger evaluation, eventually NNUE-style and/or Stockfish-distilled
- Parallel search and lockless transposition-table experiments

## Build
The C++ engine lives in `cpp_engine/` and uses CMake. Use a Release build for meaningful search and perft numbers.

```sh
cd cpp_engine
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release
```

Useful targets:

```sh
cmake --build cmake-build-release --target cpp_engine
cmake --build cmake-build-release --target test_movegen
cmake --build cmake-build-release --target test_make_unmake
cmake --build cmake-build-release --target test_search_smoke
cmake --build cmake-build-release --target perft_test
cmake --build cmake-build-release --target bench_perft
cmake --build cmake-build-release --target bench_search
```

Run the full test suite:

```sh
ctest --test-dir cmake-build-release --output-on-failure
```

Run a search from the starting position:

```sh
cmake-build-release/cpp_engine --depth 10
```

Run a search from a FEN:

```sh
cmake-build-release/cpp_engine --depth 10 --fen "3r1rk1/pp2q2p/1n1b3Q/5p2/Pp1Bp3/4P2P/5P2/2RR2K1 b - - 2 29"
```

## Performance

### Perft (move generation correctness + speed)

All node counts match canonical published values.

| Position | Depth | Nodes | Time | NPS |
|---|---|---|---|---|
| startpos | 5 | 4,865,609 | 0.055s | 89.0M |
| startpos | 6 | 119,060,324 | 1.331s | 89.4M |
| kiwipete | 4 | 4,085,603 | 0.048s | 84.5M |
| en passant pressure | 5 | 674,624 | 0.009s | 72.6M |
| promotion pressure | 4 | 2,103,487 | 0.058s | 36.1M |

### Search speed from startpos

Measured with `cmake-build-release/cpp_engine --depth N` from the standard starting position. NPS counts total searched nodes (`nodes + qnodes`).

| Depth | Best move | Score | Nodes | QNodes | Total nodes | Time | NPS |
|---|---|---:|---:|---:|---:|---:|---:|
| 8 | b1c3 | 0 | 544,783 | 679,458 | 1,224,241 | 0.148s | 8.25M |
| 9 | b1c3 | 20 | 2,873,311 | 3,287,862 | 6,161,173 | 0.644s | 9.56M |
| 10 | e2e4 | 10 | 18,423,917 | 23,116,119 | 41,540,036 | 4.962s | 8.37M |

### Comparison vs python-chess (same M4 hardware, same workload)

| Test | python-chess | This engine | Speedup |
|---|---|---|---|
| startpos depth 5 | 3.059s (1.59M nps) | 0.055s (89.0M nps) | **56x** |
| startpos depth 6 | 84.53s (1.41M nps) | 1.331s (89.4M nps) | **64x** |
| kiwipete depth 4 | 2.552s (1.60M nps) | 0.048s (84.5M nps) | **53x** |
| en passant pressure depth 5 | 0.559s (1.21M nps) | 0.009s (72.6M nps) | **62x** |
| promotion pressure depth 4 | 1.214s (1.73M nps) | 0.058s (36.1M nps) | **21x** |

