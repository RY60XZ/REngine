# REngine
A from-scratch chess engine written in C++, aiming for a strong neural-network-based evaluation (NNUE or AlphaZero-style)

## Motivation
This project started as a Python chess engine using `python-chess`. After building out alpha-beta search and basic evaluation, I ran into a wall: `python-chess` topped out at ~1-2M nodes/sec on my hardware, which made anything beyond shallow search painfully slow. Even with a half-decent search algorithm, the engine couldn't see deep enough to play coherently.

So I rewrote the engine in C++ from scratch, with two goals:
1. Make move generation fast enough that search depth is bounded by the search algorithm's intelligence, not by raw speed.
2. Build a foundation capable of supporting modern neural-network-based evaluation (NNUE or AlphaZero-style) in later phases.

## Current Status
- ✅ Legal move generation, optimized with bitboard implementations
- 🚧 Search Skeleton (alpha-beta with iterative deepening)
- ⬜ Aggressive Search Pruning
- ⬜ Evaluation function
- ⬜ Neural network integration

## Build

The C++ engine lives in `cpp_engine/` and uses CMake.

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
cmake --build cmake-build-release --target perft_test
cmake --build cmake-build-release --target bench_perft
```

Run the tests and benchmark:

```sh
./cmake-build-release/test_movegen
./cmake-build-release/test_make_unmake
./cmake-build-release/perft_test
./cmake-build-release/bench_perft
```

`bench_perft` is intended to be run optimized. The CMake target adds `-O3` and `NDEBUG`, but using a Release build directory keeps the rest of the engine consistent too.

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

### Comparison vs python-chess (same M4 hardware, same workload)

| Test | python-chess | This engine | Speedup |
|---|---|---|---|
| startpos depth 5 | 3.059s (1.59M nps) | 0.055s (89.0M nps) | **56x** |
| startpos depth 6 | 84.53s (1.41M nps) | 1.331s (89.4M nps) | **64x** |
| kiwipete depth 4 | 2.552s (1.60M nps) | 0.048s (84.5M nps) | **53x** |
| en passant pressure depth 5 | 0.559s (1.21M nps) | 0.009s (72.6M nps) | **62x** |
| promotion pressure depth 4 | 1.214s (1.73M nps) | 0.058s (36.1M nps) | **21x** |


## Architecture

### Movegen
1. 64-bit bitboard representation (one bitboard per piece type per color)
2. Magic bitboards (specially designed hashing) for sliding piece attacks (rook, bishop, queen)
3. Precomputed attack tables
4. Make/unmake with incremental state updates
