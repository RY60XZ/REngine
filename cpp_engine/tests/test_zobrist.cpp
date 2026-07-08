#include "rengine/zobrist.h"
#include "rengine/fen.h"
#include "rengine/make_move.h"
#include "rengine/square.h"
#include <cassert>
#include <cstdint>
#include <string>

namespace rengine {
    Board board_from(const std::string& fen) {
        Board board;
        set_from_fen(board, fen);
        assert(board.zobrist_key == compute_zobrist_key(board));
        return board;
    }

    void test_splitmix64_known_sequence() {
        SplitMix64 generator(0);

        assert(generator.next() == 0xE220A8397B1DCDAFULL);
        assert(generator.next() == 0x6E789E6AA1B965F4ULL);
        assert(generator.next() == 0x06C45D188009454FULL);
        assert(generator.next() == 0xF88BB8A8724C81ECULL);
    }

    void test_splitmix64_is_reproducible() {
        SplitMix64 first(0x123456789ABCDEF0ULL);
        SplitMix64 second(0x123456789ABCDEF0ULL);

        for (int i = 0; i < 32; ++i) {
            assert(first.next() == second.next());
        }
    }

    void test_splitmix64_seed_changes_stream() {
        SplitMix64 first(1);
        SplitMix64 second(2);

        assert(first.next() != second.next());
    }

    void test_zobrist_table_values_are_distinct() {
        assert(ZOBRIST_TABLE.piece_square[WHITE][PAWN][0] != 0);
        assert(ZOBRIST_TABLE.piece_square[WHITE][PAWN][0] != ZOBRIST_TABLE.piece_square[WHITE][PAWN][1]);
        assert(ZOBRIST_TABLE.piece_square[WHITE][PAWN][0] != ZOBRIST_TABLE.piece_square[BLACK][PAWN][0]);
        assert(ZOBRIST_TABLE.castling[WHITE_KINGSIDE_CASTLING] != ZOBRIST_TABLE.castling[WHITE_QUEENSIDE_CASTLING]);
        assert(ZOBRIST_TABLE.en_passant_file[0] != ZOBRIST_TABLE.en_passant_file[1]);
        assert(ZOBRIST_TABLE.side_to_move != 0);
    }

    void test_compute_zobrist_key_tracks_board_state() {
        Board start = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Board same = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 12 42");
        Board black_to_move = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1");
        Board no_castling = board_from("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1");
        Board en_passant = board_from("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");

        assert(start.zobrist_key == same.zobrist_key);
        assert(start.zobrist_key != black_to_move.zobrist_key);
        assert(start.zobrist_key != no_castling.zobrist_key);
        assert(start.zobrist_key != en_passant.zobrist_key);
    }

    void assert_incremental_hash_round_trip(const std::string& fen, Move move) {
        Board board = board_from(fen);
        ZobristKey original_key = board.zobrist_key;
        Undo undo;

        make_move(board, move, undo);
        assert(board.zobrist_key == compute_zobrist_key(board));

        unmake_move(board, undo);
        assert(board.zobrist_key == original_key);
        assert(board.zobrist_key == compute_zobrist_key(board));
    }

    void test_make_unmake_keeps_zobrist_key_synced() {
        assert_incremental_hash_round_trip("8/8/8/8/8/8/8/1N6 w - - 3 7",
                                           encode_move(parse_square("b1"), parse_square("c3"), QUIET));
        assert_incremental_hash_round_trip("8/8/8/8/8/3n4/4P3/8 w KQkq - 5 12",
                                           encode_move(parse_square("e2"), parse_square("d3"), CAPTURE));
        assert_incremental_hash_round_trip("8/8/8/8/8/8/4P3/8 w - - 5 12",
                                           encode_move(parse_square("e2"), parse_square("e4"), DOUBLE_PAWN_PUSH));
        assert_incremental_hash_round_trip("8/8/8/3pP3/8/8/8/8 w - d6 5 12",
                                           encode_move(parse_square("e5"), parse_square("d6"), EN_PASSANT));
        assert_incremental_hash_round_trip("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 3 7",
                                           encode_move(parse_square("e1"), parse_square("g1"), CASTLING));
        assert_incremental_hash_round_trip("8/P7/8/8/8/8/8/8 w - - 5 12",
                                           encode_move(parse_square("a7"), parse_square("a8"), PROMOTION, QUEEN));
        assert_incremental_hash_round_trip("1n6/P7/8/8/8/8/8/8 w - - 5 12",
                                           encode_move(parse_square("a7"), parse_square("b8"), PROMOTION_CAPTURE, QUEEN));
    }

    void test_null_move_keeps_zobrist_key_synced() {
        Board board = board_from("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2");
        ZobristKey original_key = board.zobrist_key;
        Undo undo;

        make_null_move(board, undo);
        assert(board.zobrist_key == compute_zobrist_key(board));

        unmake_null_move(board, undo);
        assert(board.zobrist_key == original_key);
        assert(board.zobrist_key == compute_zobrist_key(board));
    }
}

int main() {
    rengine::test_splitmix64_known_sequence();
    rengine::test_splitmix64_is_reproducible();
    rengine::test_splitmix64_seed_changes_stream();
    rengine::test_zobrist_table_values_are_distinct();
    rengine::test_compute_zobrist_key_tracks_board_state();
    rengine::test_make_unmake_keeps_zobrist_key_synced();
    rengine::test_null_move_keeps_zobrist_key_synced();
}
