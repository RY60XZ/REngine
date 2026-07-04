#ifndef CPP_ENGINE_BOARD_H
#define CPP_ENGINE_BOARD_H
#include <bit>
#include "rengine/types.h"
#include<cassert>
namespace rengine {
    struct Board {
        Bitboard pieces[2][6]{};
        Bitboard occupied[2]{};
        Bitboard all{};
        Piece squares[64]{};

        Color side_to_move = WHITE;
        unsigned castling_rights = 0;
        Square en_passant_square = INVALID_SQUARE;
        ZobristKey zobrist_key = 0;
        int half_move_clock = 0;
        int full_move_number = 1;
    };

    void clear(Board& board);

    constexpr Bitboard square_bb(Square square) {
        assert(square < 64);
        return Bitboard{1} << static_cast<int>(square);
    }

    constexpr bool is_empty(const Board& board) {
        return board.all==0;
    }

    constexpr Color color_of(Piece piece) {
        assert(piece != NO_PIECE);
        return static_cast<Color>((static_cast<int>(piece)-1)/6);
    }

    constexpr PieceType piece_type_of(Piece piece) {
        assert(piece != NO_PIECE);
        return static_cast<PieceType>((static_cast<int>(piece)-1)%6);
    }

    constexpr bool is_border_square(Square square) {
        return (BORDER & (Bitboard{1} << square)) != 0;
    }

    inline Square king_square(const Board& board, Color color) {
        Bitboard king_bitboard = board.pieces[color][KING];
        assert(king_bitboard != 0);
        return static_cast<Square>(std::countr_zero(king_bitboard));
    }

    inline Color opposite_color(Color color) {
        return static_cast<Color>(!color);
    }

    inline void set_piece(Board &board, Square square, Piece piece) {
        assert(square<64);
        assert(piece != NO_PIECE);
        assert(board.squares[square] == NO_PIECE);
        int piece_color = color_of(piece), piece_type = piece_type_of(piece);
        Bitboard pos_mask = Bitboard{1} << square;
        board.pieces[piece_color][piece_type] |= pos_mask;
        board.occupied[piece_color] |= pos_mask;
        board.all |= pos_mask;
        board.squares[square] = piece;
    }

    inline void remove_piece(Board &board, Square square) {
        assert(square<64);
        Piece piece = board.squares[square];
        assert(piece != NO_PIECE);
        int piece_color = color_of(piece), piece_type = piece_type_of(piece);
        Bitboard pos_mask = ~(Bitboard{1}<<square);
        board.pieces[piece_color][piece_type] &= pos_mask;
        board.occupied[piece_color] &= pos_mask;
        board.all &= pos_mask;
        board.squares[square] = NO_PIECE;
    }

    inline void move_piece(Board &board, Square from, Square to) {
        assert(from<64);
        assert(to<64);
        Piece from_piece = board.squares[from];
        assert(from_piece != NO_PIECE);
        assert(board.squares[to] == NO_PIECE);
        remove_piece(board, from);
        set_piece(board, to, from_piece);
    }
}
#endif //CPP_ENGINE_BOARD_H
