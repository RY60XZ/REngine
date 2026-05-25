#include"rengine/make_move.h"

namespace rengine {
    void make_move(Board& board, Move move, Undo& undo) {
        Square from = move_from(move), to = move_to(move);
        MoveFlag flag = move_flag(move);
        PieceType promotion_type = move_promotion_type(move);
        Piece from_piece = board.squares[from];
        Color by = board.side_to_move;
        undo.move = move;
        undo.captured_piece = board.squares[to];
        undo.castling_rights = update_castling_rights(board, from, to);
        undo.en_passant_square = board.en_passant_square;
        undo.half_move_clock = board.half_move_clock;
        undo.full_move_number = board.full_move_number;
        board.half_move_clock++;
        board.full_move_number+= by; //only increment full_move in Black's turn
        board.side_to_move = opposite_color(board.side_to_move);
        if (piece_type_of(from_piece) == PAWN) board.half_move_clock = 0; //handle capture in case CAPTURE
        switch (flag) {
            case QUIET:
                set_piece(board, to, from_piece);
                remove_piece(board, from);
                break;
            case CAPTURE:
                board.half_move_clock = 0;
                remove_piece(board, to);
                set_piece(board, to, from_piece);
                remove_piece(board, from);
                break;
            case DOUBLE_PAWN_PUSH:
                set_piece(board, to, from_piece);
                remove_piece(board, from);
                if (by == WHITE) {
                    board.en_passant_square = from + 8;
                }
                else {
                    board.en_passant_square = from - 8;
                }
                break;
            case EN_PASSANT:
                if (by == WHITE) {
                    set_piece(board, to, from_piece);
                    remove_piece(board, from);
                    remove_piece(board, to-8);
                    undo.captured_piece = BLACK_PAWN;
                }
                else {
                    set_piece(board, to, from_piece);
                    remove_piece(board, from);
                    remove_piece(board, to+8);
                    undo.captured_piece = WHITE_PAWN;
                }
                break;
            case CASTLING:
                set_piece(board, to, from_piece);
                remove_piece(board, from);
                switch (to) {
                    case 2:
                        set_piece(board, 3, WHITE_ROOK);
                        remove_piece(board, 0);
                        break;
                    case 6:
                        set_piece(board, 5, WHITE_ROOK);
                        remove_piece(board, 7);
                        break;
                    case 58:
                        set_piece(board, 59, BLACK_ROOK);
                        remove_piece(board, 56);
                        break;
                    case 62:
                        set_piece(board, 61, BLACK_ROOK);
                        remove_piece(board, 63);
                        break;
                    default:
                        break;
                }
                break;
            case PROMOTION:
                set_piece(board, to, static_cast<Piece>(by * 6 + promotion_type + 1));
                remove_piece(board, from);
                break;
            case PROMOTION_CAPTURE:
                remove_piece(board, to);
                set_piece(board, to, static_cast<Piece>(by * 6 + promotion_type + 1));
                remove_piece(board, from);
        }
        if (flag != DOUBLE_PAWN_PUSH) board.en_passant_square = INVALID_SQUARE;
    }

    void unmake_move(Board& board, const Undo& undo) {
        Move move = undo.move;
        Square from = move_from(move), to = move_to(move);
        MoveFlag flag = move_flag(move);
        Color by = opposite_color(board.side_to_move);
        board.side_to_move = by;
        Piece to_piece = board.squares[to];
        Piece pawn = static_cast<Piece>(by * 6 + PAWN + 1);

        switch (flag) {
            case QUIET:
            case DOUBLE_PAWN_PUSH:
                set_piece(board, from, to_piece);
                remove_piece(board, to);
                break;
            case CAPTURE:
                remove_piece(board, to);
                set_piece(board, to, undo.captured_piece);
                set_piece(board, from, to_piece);
                break;
            case EN_PASSANT:
                remove_piece(board, to);
                set_piece(board, from, to_piece);
                if (by == WHITE) {
                    set_piece(board, to - 8, undo.captured_piece);
                }
                else {
                    set_piece(board, to + 8, undo.captured_piece);
                }
                break;
            case CASTLING:
                set_piece(board, from, to_piece);
                remove_piece(board, to);
                switch (to) {
                    case 2:
                        set_piece(board, 0, WHITE_ROOK);
                        remove_piece(board, 3);
                        break;
                    case 6:
                        set_piece(board, 7, WHITE_ROOK);
                        remove_piece(board, 5);
                        break;
                    case 58:
                        set_piece(board, 56, BLACK_ROOK);
                        remove_piece(board, 59);
                        break;
                    case 62:
                        set_piece(board, 63, BLACK_ROOK);
                        remove_piece(board, 61);
                        break;
                    default:
                        break;
                }
                break;
            case PROMOTION:
                remove_piece(board, to);
                set_piece(board, from, pawn);
                break;
            case PROMOTION_CAPTURE:
                remove_piece(board, to);
                set_piece(board, to, undo.captured_piece);
                set_piece(board, from, pawn);
                break;
        }

        board.castling_rights = undo.castling_rights;
        board.en_passant_square = undo.en_passant_square;
        board.half_move_clock = undo.half_move_clock;
        board.full_move_number = undo.full_move_number;
    }
}
