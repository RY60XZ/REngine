#ifndef CPP_ENGINE_BOARD_H
#define CPP_ENGINE_BOARD_H
#include<rengine/types.h>
namespace rengine {
    struct Board {
        Bitboard pieces[2][6]{};
        Bitboard occupied[2]{};
        Bitboard all{};
        Piece squares[64]{};

        Color side_to_move = WHITE;
        unsigned castling_rights = 0;
        Square en_passant_square = -1;
        int half_move_clock = 0;
        int full_move_number = 1;
    };

    void clear(Board& board);
    bool is_empty(const Board& board);
}
#endif //CPP_ENGINE_BOARD_H