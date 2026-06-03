#include"rengine/fen.h"
#include"rengine/move_format.h"
#include"rengine/search.h"
#include<chrono>
#include<cstdint>
#include<iomanip>
#include<iostream>

namespace rengine {
    namespace {
        struct BenchPosition {
            const char* name;
            const char* fen;
            int depth;
        };

        double elapsed_seconds(const SearchResult& result) {
            return std::chrono::duration<double>(result.stats.elapsed_time).count();
        }

        double nodes_per_second(const SearchResult& result) {
            double seconds = elapsed_seconds(result);
            if (seconds == 0.0) return 0.0;
            return static_cast<double>(result.stats.nodes) / seconds;
        }

        void print_result(const BenchPosition& position, const SearchResult& result) {
            std::cout << "position: " << position.name << '\n';
            std::cout << "depth: " << result.completed_depth << '\n';
            std::cout << "bestmove: "
                      << (result.has_best_move ? move_to_uci(result.best_move) : "0000") << '\n';
            std::cout << "score: " << result.score << '\n';
            std::cout << "nodes: " << result.stats.nodes << '\n';
            std::cout << "qnodes: " << result.stats.qnodes << '\n';
            std::cout << "cutoffs: " << result.stats.cutoffs << '\n';
            std::cout << "time: " << std::fixed << std::setprecision(6)
                      << elapsed_seconds(result) << "s\n";
            std::cout << "nps: " << std::fixed << std::setprecision(2)
                      << nodes_per_second(result) << "\n\n";
        }
    }

    void bench_search() {
        const BenchPosition positions[] = {
            {
                "startpos",
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                1
            },
            {
                "kiwipete",
                "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                1
            },
            {
                "winning_capture",
                "q6k/8/8/8/8/8/8/R3K3 w - - 0 1",
                1
            },
        };

        for (const BenchPosition& position : positions) {
            Board board;
            set_from_fen(board, position.fen);

            SearchLimits limits;
            limits.depth = position.depth;

            SearchResult result = search_position(board, limits);
            print_result(position, result);
        }
    }
}

int main() {
    rengine::bench_search();
}
