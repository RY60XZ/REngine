#include"rengine/fen.h"
#include"rengine/move_format.h"
#include"rengine/movegen.h"
#include"rengine/uci.h"
#include<cassert>
#include<sstream>
#include<string>

namespace rengine {
    namespace {
        bool contains(const std::string& text, const std::string& needle) {
            return text.find(needle) != std::string::npos;
        }

        std::string run_uci_script(const std::string& script) {
            std::istringstream input(script);
            std::ostringstream output;
            int status = run_uci_loop(input, output);
            assert(status == 0);
            return output.str();
        }

        std::string bestmove_from_output(const std::string& output) {
            std::istringstream lines(output);
            std::string word;
            while (lines >> word) {
                if (word == "bestmove") {
                    std::string move;
                    lines >> move;
                    return move;
                }
            }
            return {};
        }

        bool is_legal_uci_move(const Board& board, const std::string& move_text) {
            Board board_copy = board;
            MoveList moves;
            generate_legal_moves(board_copy, moves);
            for (Move move : moves) {
                if (move_to_uci(move) == move_text) {
                    return true;
                }
            }
            return false;
        }
    }

    void test_uci_handshake() {
        std::string output = run_uci_script("uci\nisready\n");

        assert(contains(output, "id name REngine\n"));
        assert(contains(output, "id author Ryan Yao\n"));
        assert(contains(output, "uciok\n"));
        assert(contains(output, "readyok\n"));
    }

    void test_position_startpos_go_depth() {
        std::string output = run_uci_script(
            "position startpos\n"
            "go depth 1\n"
        );

        Board board;
        set_from_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        std::string bestmove = bestmove_from_output(output);

        assert(contains(output, "info depth 1 "));
        assert(!bestmove.empty());
        assert(is_legal_uci_move(board, bestmove));
    }

    void test_position_moves_are_applied() {
        Board board;
        set_from_fen(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        assert(make_uci_move(board, "e2e4"));
        assert(make_uci_move(board, "e7e5"));
        assert(board_to_fen(board) == "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
    }

    void test_go_movetime_returns_bestmove() {
        std::string output = run_uci_script(
            "position startpos\n"
            "go movetime 1\n"
        );

        assert(contains(output, "bestmove "));
    }
}

int main() {
    rengine::test_uci_handshake();
    rengine::test_position_startpos_go_depth();
    rengine::test_position_moves_are_applied();
    rengine::test_go_movetime_returns_bestmove();
}
