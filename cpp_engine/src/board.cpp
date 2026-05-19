#include "rengine/board.h"
#include "rengine/types.h"
#include "cassert"
namespace rengine{
    void clear(Board& board) {
        for (auto& color_pieces : board.pieces) {
            for (auto& pieces : color_pieces) {
                pieces = 0;
            }
        }
        board.occupied[WHITE] = 0;
        board.occupied[BLACK] = 0;
        board.all = 0;
        for (auto& square : board.squares) {
            square = NO_PIECE;
        }

        board.side_to_move = WHITE;
        board.castling_rights = 0;
        board.en_passant_square = INVALID_SQUARE;
        board.half_move_clock = 0;
        board.full_move_number = 1;
    }

    void set_piece(Board &board, Square square, Piece piece) {
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

    void remove_piece(Board &board, Square square) {
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

    void move_piece(Board &board, Square from, Square to) {
        assert(from<64);
        assert(to<64);
        Piece from_piece = board.squares[from];
        assert(from_piece != NO_PIECE);
        assert(board.squares[to] == NO_PIECE);
        remove_piece(board, from);
        set_piece(board, to, from_piece);
    }
}
