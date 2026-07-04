#include "rengine/zobrist.h"
#include "rengine/square.h"

namespace rengine {
    namespace {
        constexpr ZobristTable make_zobrist_table() {
            SplitMix64 rng(0x9E3779B97F4A7C15ULL);
            ZobristTable table{};

            for (auto& color : table.piece_square)
                for (auto& piece : color)
                    for (auto& square : piece)
                        square = rng.next();

            for (auto& key : table.castling)
                key = rng.next();

            for (auto& key : table.en_passant_file)
                key = rng.next();

            table.side_to_move = rng.next();
            return table;
        }
    }
    const ZobristTable ZOBRIST_TABLE = make_zobrist_table();

    ZobristKey compute_zobrist_key(const Board &board) {
        ZobristKey key = 0ULL;
        for (Square sq=0; sq<64; ++sq) {
            Piece piece = board.squares[sq];
            if (piece == NO_PIECE) {
                continue;
            }
            PieceType piece_type = piece_type_of(piece);
            Color color = color_of(piece);
            key ^= ZOBRIST_TABLE.piece_square[color][piece_type][sq];
        }

        key ^= ZOBRIST_TABLE.castling[board.castling_rights & 0xF];

        if (board.en_passant_square != INVALID_SQUARE) {
            key ^= ZOBRIST_TABLE.en_passant_file[file_of(board.en_passant_square)];
        }

        if (board.side_to_move == BLACK) {
            key ^= ZOBRIST_TABLE.side_to_move;
        }

        return key;
    }

    void update_zobrist_key(Board& board) {
        board.zobrist_key = compute_zobrist_key(board);
    }

}
