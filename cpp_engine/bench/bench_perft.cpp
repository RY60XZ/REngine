#include"rengine/perft.h"
#include"rengine/fen.h"
#include<chrono>
#include<cstdint>
#include<iomanip>
#include<iostream>

namespace rengine {
    struct BenchPosition {
        const char* name;
        const char* fen;
        int max_depth;
    };

    struct BenchResult {
        const char* name = "";
        int depth = 0;
        uint64_t nodes = 0;
        double seconds = 0.0;
        double nps = 0.0;
        int runs = 0;
    };

    double nodes_per_second(uint64_t nodes, double seconds) {
        if (seconds == 0.0) return 0.0;
        return static_cast<double>(nodes) / seconds;
    }

    void print_nps(double nps) {
        if (nps >= 1'000'000.0) {
            std::cout << std::fixed << std::setprecision(2) << nps / 1'000'000.0 << "M";
        }
        else if (nps >= 1'000.0) {
            std::cout << std::fixed << std::setprecision(2) << nps / 1'000.0 << "K";
        }
        else {
            std::cout << std::fixed << std::setprecision(2) << nps;
        }
    }

    void print_seconds(double seconds) {
        if (seconds < 0.001) {
            std::cout << std::fixed << std::setprecision(6) << seconds << "s";
        }
        else {
            std::cout << std::fixed << std::setprecision(3) << seconds << "s";
        }
    }

    BenchResult bench_position(const BenchPosition& position, int depth) {
        Board board;
        set_from_fen(board, position.fen);

        constexpr double MIN_SECONDS = 0.050;
        constexpr int MAX_RUNS = 1'000'000;

        uint64_t nodes = 0;
        uint64_t total_nodes = 0;
        double total_seconds = 0.0;
        int runs = 0;

        while (runs < MAX_RUNS && (runs == 0 || total_seconds < MIN_SECONDS)) {
            auto start = std::chrono::steady_clock::now();
            nodes = perft(board, depth);
            auto end = std::chrono::steady_clock::now();

            total_seconds += std::chrono::duration<double>(end - start).count();
            total_nodes += nodes;
            ++runs;
        }

        double seconds = total_seconds / runs;
        return BenchResult{
            position.name,
            depth,
            nodes,
            seconds,
            nodes_per_second(total_nodes, total_seconds),
            runs
        };
    }

    void print_result(const BenchResult& result) {
        std::cout << "position: " << result.name << '\n';
        std::cout << "depth: " << result.depth << '\n';
        std::cout << "nodes: " << result.nodes << '\n';
        std::cout << "runs: " << result.runs << '\n';
        std::cout << "time: ";
        print_seconds(result.seconds);
        std::cout << '\n';
        std::cout << "nps: ";
        print_nps(result.nps);
        std::cout << "\n\n";
    }

    void bench_perft() {
        BenchPosition positions[] = {
            {
                "startpos",
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                5
            },
            {
                "kiwipete",
                "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                4
            },
            {
                "en_passant_pressure",
                "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                5
            },
            {
                "promotion_pressure",
                "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                4
            },
        };

        BenchResult slowest_time;
        BenchResult slowest_nps;
        bool has_result = false;

        for (const BenchPosition& position : positions) {
            for (int depth = 1; depth <= position.max_depth; ++depth) {
                BenchResult result = bench_position(position, depth);
                print_result(result);

                if (!has_result || result.seconds > slowest_time.seconds) {
                    slowest_time = result;
                }
                if (!has_result || result.nps < slowest_nps.nps) {
                    slowest_nps = result;
                }
                has_result = true;
            }
        }

        std::cout << "slowest_time: " << slowest_time.name << " depth " << slowest_time.depth
                  << " (";
        print_seconds(slowest_time.seconds);
        std::cout << ")\n";
        std::cout << "slowest_nps: " << slowest_nps.name << " depth " << slowest_nps.depth
                  << " (";
        print_nps(slowest_nps.nps);
        std::cout << ")\n";
    }
}

int main() {
    rengine::bench_perft();
}
