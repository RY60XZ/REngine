#include"rengine/move_format.h"
#include"rengine/square.h"
#include<cassert>

namespace rengine {
    void test_four_character_moves() {
        assert(move_to_uci(encode_move(parse_square("e2"), parse_square("e4"), QUIET)) == "e2e4");
        assert(move_to_uci(encode_move(parse_square("e5"), parse_square("d6"), CAPTURE)) == "e5d6");
        assert(move_to_uci(encode_move(parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH)) == "e2e4");
        assert(move_to_uci(encode_move(parse_square("e5"), parse_square("d6"), EN_PASSANT)) == "e5d6");
        assert(move_to_uci(encode_move(parse_square("e1"), parse_square("g1"), CASTLING)) == "e1g1");
    }

    void test_promotion_moves() {
        assert(move_to_uci(encode_move(parse_square("e7"), parse_square("e8"), PROMOTION, QUEEN)) == "e7e8q");
        assert(move_to_uci(encode_move(parse_square("e7"), parse_square("e8"), PROMOTION, ROOK)) == "e7e8r");
        assert(move_to_uci(encode_move(parse_square("e7"), parse_square("e8"), PROMOTION, BISHOP)) == "e7e8b");
        assert(move_to_uci(encode_move(parse_square("a2"), parse_square("a1"), PROMOTION, KNIGHT)) == "a2a1n");
        assert(move_to_uci(encode_move(parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, QUEEN)) == "a7b8q");
        assert(move_to_uci(encode_move(parse_square("a2"), parse_square("b1"), PROMOTION_CAPTURE, KNIGHT)) == "a2b1n");
    }
}

int main() {
    rengine::test_four_character_moves();
    rengine::test_promotion_moves();
}
