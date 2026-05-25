#include"rengine/perft.h"
#include"rengine/fen.h"
#include<cassert>
#include<cstdint>

namespace rengine {
    void check_perft(const char* fen, int depth, uint64_t expected) {
        Board board;
        set_from_fen(board, fen);
        assert(perft(board, depth) == expected);
    }

    void test_start_position() {
        const char* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

        check_perft(fen, 0, 1);
        check_perft(fen, 1, 20);
        check_perft(fen, 2, 400);
        check_perft(fen, 3, 8902);
        check_perft(fen, 4, 197281);
        check_perft(fen, 5, 4865609);
    }

    void test_kiwipete_position() {
        const char* fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

        check_perft(fen, 1, 48);
        check_perft(fen, 2, 2039);
        check_perft(fen, 3, 97862);
    }

    void test_en_passant_pressure_position() {
        const char* fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";

        check_perft(fen, 1, 14);
        check_perft(fen, 2, 191);
        check_perft(fen, 3, 2812);
        check_perft(fen, 4, 43238);
    }

    void test_promotion_pressure_position() {
        const char* fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";

        check_perft(fen, 1, 44);
        check_perft(fen, 2, 1486);
        check_perft(fen, 3, 62379);
    }
}

int main() {
    rengine::test_start_position();
    rengine::test_kiwipete_position();
    rengine::test_en_passant_pressure_position();
    rengine::test_promotion_pressure_position();
}
