import time

try:
    import chess
except ImportError:
    print("python-chess is not installed")
    print("install: python3 -m pip install python-chess")
    raise SystemExit(1)


POSITIONS = [
    (
        "startpos",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        5,
    ),
    (
        "kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        4,
    ),
    (
        "en_passant_pressure",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        5,
    ),
    (
        "promotion_pressure",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        4,
    ),
]


def perft(board, depth):
    if depth == 0:
        return 1

    if depth == 1:
        return board.legal_moves.count()

    nodes = 0
    for move in board.legal_moves:
        board.push(move)
        nodes += perft(board, depth - 1)
        board.pop()
    return nodes


def print_nps(nps):
    if nps >= 1_000_000.0:
        print(f"{nps / 1_000_000.0:.2f}M", end="")
    elif nps >= 1_000.0:
        print(f"{nps / 1_000.0:.2f}K", end="")
    else:
        print(f"{nps:.2f}", end="")


def print_seconds(seconds):
    if seconds < 0.001:
        print(f"{seconds:.6f}s", end="")
    else:
        print(f"{seconds:.3f}s", end="")


def bench_position(name, fen, depth):
    min_seconds = 0.050
    max_runs = 1_000_000

    board = chess.Board(fen)
    nodes = 0
    total_nodes = 0
    total_seconds = 0.0
    runs = 0

    while runs < max_runs and (runs == 0 or total_seconds < min_seconds):
        start = time.perf_counter()
        nodes = perft(board, depth)
        end = time.perf_counter()

        total_seconds += end - start
        total_nodes += nodes
        runs += 1

    seconds = total_seconds / runs
    nps = total_nodes / total_seconds if total_seconds != 0.0 else 0.0

    return {
        "name": name,
        "depth": depth,
        "nodes": nodes,
        "runs": runs,
        "seconds": seconds,
        "nps": nps,
    }


def print_result(result):
    print(f"position: {result['name']}")
    print(f"depth: {result['depth']}")
    print(f"nodes: {result['nodes']}")
    print(f"runs: {result['runs']}")
    print("time: ", end="")
    print_seconds(result["seconds"])
    print()
    print("nps: ", end="")
    print_nps(result["nps"])
    print("\n")


def main():
    slowest_time = None
    slowest_nps = None

    for name, fen, max_depth in POSITIONS:
        for depth in range(1, max_depth + 1):
            result = bench_position(name, fen, depth)
            print_result(result)

            if slowest_time is None or result["seconds"] > slowest_time["seconds"]:
                slowest_time = result
            if slowest_nps is None or result["nps"] < slowest_nps["nps"]:
                slowest_nps = result

    print(
        f"slowest_time: {slowest_time['name']} depth {slowest_time['depth']} (",
        end="",
    )
    print_seconds(slowest_time["seconds"])
    print(")")

    print(
        f"slowest_nps: {slowest_nps['name']} depth {slowest_nps['depth']} (",
        end="",
    )
    print_nps(slowest_nps["nps"])
    print(")")


if __name__ == "__main__":
    main()
