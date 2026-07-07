#include"rengine/fen.h"
#include"rengine/move_format.h"
#include"rengine/search.h"
#include"rengine/uci.h"
#include<chrono>
#include<cstdint>
#include<iostream>
#include<string>

namespace {
    constexpr const char* STARTPOS_FEN =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    struct CliOptions {
        std::string fen = STARTPOS_FEN;
        int depth = 1;
        std::uint64_t node_limit = rengine::NO_NODE_LIMIT;
        int movetime_ms = 0;
        bool infinite = false;
    };

    void print_usage(const char* program) {
        std::cout
            << "usage: " << program << " [--uci]\n"
            << "       " << program << " [--fen FEN] [--depth N] [--nodes N] [--movetime MS] [--infinite]\n"
            << "example: " << program << " --fen \"" << STARTPOS_FEN << "\" --depth 1\n";
    }

    bool parse_options(int argc, char** argv, CliOptions& options) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--help" || arg == "-h") {
                print_usage(argv[0]);
                return false;
            }
            if ((arg == "--fen" || arg == "-f") && i + 1 < argc) {
                options.fen = argv[++i];
            }
            else if ((arg == "--depth" || arg == "-d") && i + 1 < argc) {
                options.depth = std::stoi(argv[++i]);
            }
            else if (arg == "--nodes" && i + 1 < argc) {
                options.node_limit = std::stoull(argv[++i]);
            }
            else if (arg == "--movetime" && i + 1 < argc) {
                options.movetime_ms = std::stoi(argv[++i]);
            }
            else if (arg == "--infinite") {
                options.infinite = true;
            }
            else {
                std::cerr << "unknown or incomplete option: " << arg << '\n';
                print_usage(argv[0]);
                return false;
            }
        }
        return true;
    }

    double elapsed_seconds(const rengine::SearchResult& result) {
        return std::chrono::duration<double>(result.stats.elapsed_time).count();
    }

    std::uint64_t total_nodes(const rengine::SearchResult& result) {
        return result.stats.nodes + result.stats.qnodes;
    }

    double nodes_per_second(const rengine::SearchResult& result) {
        double seconds = elapsed_seconds(result);
        if (seconds == 0.0) return 0.0;
        return static_cast<double>(total_nodes(result)) / seconds;
    }
}

int main(int argc, char** argv) {
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--uci")) {
        return rengine::run_uci_loop(std::cin, std::cout);
    }

    CliOptions options;
    if (!parse_options(argc, argv, options)) {
        return argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") ? 0 : 1;
    }

    rengine::Board board;
    rengine::set_from_fen(board, options.fen);

    rengine::SearchLimits limits;
    limits.depth = options.depth;
    limits.node_limit = options.node_limit;
    limits.movetime = std::chrono::milliseconds(options.movetime_ms);
    limits.infinite = options.infinite;

    rengine::SearchResult result = rengine::search_position(board, limits);

    std::cout << "bestmove "
              << (result.has_best_move ? rengine::move_to_uci(result.best_move) : "0000") << '\n';
    std::cout << "score " << result.score << '\n';
    std::cout << "depth " << result.completed_depth << '\n';
    std::cout << "nodes " << result.stats.nodes << '\n';
    std::cout << "qnodes " << result.stats.qnodes << '\n';
    std::cout << "totalnodes " << total_nodes(result) << '\n';
    std::cout << "time " << elapsed_seconds(result) << "s\n";
    std::cout << "nps " << nodes_per_second(result) << '\n';
}
