#include"rengine/make_move.h"
#include"rengine/square.h"
#include"rengine/zobrist.h"

namespace rengine {
    namespace {
        void xor_piece_key(Board& board, Piece piece, Square square) {
            board.zobrist_key ^= ZOBRIST_TABLE.piece_square[color_of(piece)][piece_type_of(piece)][square];
        }

        void xor_en_passant_key(Board& board, Square square) {
            if (square != INVALID_SQUARE) {
                board.zobrist_key ^= ZOBRIST_TABLE.en_passant_file[file_of(square)];
            }
        }

        void xor_castling_key(Board& board) {
            board.zobrist_key ^= ZOBRIST_TABLE.castling[board.castling_rights & 0xF];
        }
    }

    void make_move(Board& board, Move move, Undo& undo) {
        Square from = move_from(move), to = move_to(move);
        MoveFlag flag = move_flag(move);
        PieceType promotion_type = move_promotion_type(move);
        Piece from_piece = board.squares[from];
        Color by = board.side_to_move;
        undo.move = move;
        undo.captured_piece = board.squares[to];
        undo.zobrist_key = board.zobrist_key;
        xor_castling_key(board);
        xor_en_passant_key(board, board.en_passant_square);
        board.zobrist_key ^= ZOBRIST_TABLE.side_to_move;
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
            {
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, from_piece, to);
                Bitboard from_bb = square_bb(from);
                Bitboard to_bb = square_bb(to);
                Bitboard move_bb = from_bb | to_bb;
                PieceType from_piece_type = piece_type_of(from_piece);

                board.pieces[by][from_piece_type] ^= move_bb;
                board.occupied[by] ^= move_bb;
                board.all ^= move_bb;

                board.squares[from] = NO_PIECE;
                board.squares[to] = from_piece;
                break;
            }
            case CAPTURE: {
                Piece captured_piece = board.squares[to];
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, from_piece, to);
                xor_piece_key(board, captured_piece, to);
                board.half_move_clock = 0;
                Bitboard from_bb = square_bb(from);
                Bitboard to_bb = square_bb(to);
                Bitboard move_bb = from_bb | to_bb;
                PieceType from_piece_type = piece_type_of(from_piece);
                PieceType to_piece_type = piece_type_of(board.squares[to]);

                board.pieces[by][from_piece_type] ^= move_bb;
                board.occupied[by] ^= move_bb;

                Color opposite = opposite_color(by);
                board.pieces[opposite][to_piece_type] &= ~to_bb;
                board.occupied[opposite] &= ~to_bb;

                board.all &= ~from_bb;

                board.squares[from] = NO_PIECE;
                board.squares[to] = from_piece;
                break;
            }
            case DOUBLE_PAWN_PUSH:
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, from_piece, to);
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
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, from_piece, to);
                if (by == WHITE) {
                    xor_piece_key(board, BLACK_PAWN, to-8);
                    set_piece(board, to, from_piece);
                    remove_piece(board, from);
                    remove_piece(board, to-8);
                    undo.captured_piece = BLACK_PAWN;
                }
                else {
                    xor_piece_key(board, WHITE_PAWN, to+8);
                    set_piece(board, to, from_piece);
                    remove_piece(board, from);
                    remove_piece(board, to+8);
                    undo.captured_piece = WHITE_PAWN;
                }
                break;
            case CASTLING:
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, from_piece, to);
                set_piece(board, to, from_piece);
                remove_piece(board, from);
                switch (to) {
                    case 2:
                        xor_piece_key(board, WHITE_ROOK, 0);
                        xor_piece_key(board, WHITE_ROOK, 3);
                        set_piece(board, 3, WHITE_ROOK);
                        remove_piece(board, 0);
                        break;
                    case 6:
                        xor_piece_key(board, WHITE_ROOK, 7);
                        xor_piece_key(board, WHITE_ROOK, 5);
                        set_piece(board, 5, WHITE_ROOK);
                        remove_piece(board, 7);
                        break;
                    case 58:
                        xor_piece_key(board, BLACK_ROOK, 56);
                        xor_piece_key(board, BLACK_ROOK, 59);
                        set_piece(board, 59, BLACK_ROOK);
                        remove_piece(board, 56);
                        break;
                    case 62:
                        xor_piece_key(board, BLACK_ROOK, 63);
                        xor_piece_key(board, BLACK_ROOK, 61);
                        set_piece(board, 61, BLACK_ROOK);
                        remove_piece(board, 63);
                        break;
                    default:
                        break;
                }
                break;
            case PROMOTION:
            {
                Piece promoted_piece = static_cast<Piece>(by * 6 + promotion_type + 1);
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, promoted_piece, to);
                set_piece(board, to, promoted_piece);
                remove_piece(board, from);
                break;
            }
            case PROMOTION_CAPTURE:
            {
                Piece captured_piece = board.squares[to];
                Piece promoted_piece = static_cast<Piece>(by * 6 + promotion_type + 1);
                xor_piece_key(board, from_piece, from);
                xor_piece_key(board, captured_piece, to);
                xor_piece_key(board, promoted_piece, to);
                remove_piece(board, to);
                set_piece(board, to, promoted_piece);
                remove_piece(board, from);
                break;
            }
        }
        if (flag != DOUBLE_PAWN_PUSH) board.en_passant_square = INVALID_SQUARE;
        xor_castling_key(board);
        xor_en_passant_key(board, board.en_passant_square);
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
        board.zobrist_key = undo.zobrist_key;
        board.half_move_clock = undo.half_move_clock;
        board.full_move_number = undo.full_move_number;
    }

    void make_null_move(Board& board, Undo& undo) {
        undo.castling_rights = board.castling_rights;
        undo.en_passant_square = board.en_passant_square;
        undo.zobrist_key = board.zobrist_key;
        undo.half_move_clock = board.half_move_clock;
        undo.full_move_number = board.full_move_number;

        xor_en_passant_key(board, board.en_passant_square);
        board.en_passant_square = INVALID_SQUARE;
        board.side_to_move = opposite_color(board.side_to_move);
        board.zobrist_key ^= ZOBRIST_TABLE.side_to_move;
    }

    void unmake_null_move(Board& board, const Undo& undo) {
        board.side_to_move = opposite_color(board.side_to_move);
        board.castling_rights = undo.castling_rights;
        board.en_passant_square = undo.en_passant_square;
        board.zobrist_key = undo.zobrist_key;
        board.half_move_clock = undo.half_move_clock;
        board.full_move_number = undo.full_move_number;
    }
}
