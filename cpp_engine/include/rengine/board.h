#ifndef CPP_ENGINE_BOARD_H
#define CPP_ENGINE_BOARD_H
#include<rengine/types.h>
#include<cassert>
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
    void set_piece(Board& board, Square square, Piece piece);
    void remove_piece(Board&board, Square square);
    void move_piece(Board&board, Square from, Square to); //assume to_square is empty
    inline bool is_empty(const Board& board) {
        return board.all==0;
    }
    inline Color color_of(Piece piece) {
        assert(piece != NO_PIECE);
        return static_cast<Color>((static_cast<int>(piece)-1)/6);
    }
    inline PieceType piece_type_of(Piece piece) {
        assert(piece != NO_PIECE);
        return static_cast<PieceType>((static_cast<int>(piece)-1)%6);
    }
}
#endif //CPP_ENGINE_BOARD_H