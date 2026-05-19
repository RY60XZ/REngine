#ifndef CPP_ENGINE_MOVE_H
#define CPP_ENGINE_MOVE_H
#include<rengine/types.h>
#include<cassert>
#include<cstdint>

namespace rengine {
    using Move = std::uint32_t;
    enum MoveFlag {
        QUIET = 0,
        CAPTURE = 1,
        DOUBLE_PAWN_PUSH = 2,
        EN_PASSANT = 3,
        CASTLING = 4,
        PROMOTION = 5,
        PROMOTION_CAPTURE = 6
    };

    inline Move encode_move(Square from, Square to, MoveFlag flag = QUIET, PieceType promotion=PAWN) {
        assert(from>=0 && from<64);
        assert(to>=0 && to<64);
        assert(static_cast<int>(flag)>=0 && static_cast<int>(flag)<16);
        assert(static_cast<int>(promotion)>=0 && static_cast<int>(promotion)<8);
        return static_cast<Move>(
            static_cast<int>(from) |
            (static_cast<int>(to)<<6) |
            (static_cast<int>(flag)<<12) |
            (static_cast<int>(promotion)<<16)
        );
    }

    inline Square move_from(Move move) {
        return static_cast<Square>(move & 0x3F);
    }

    inline Square move_to(Move move) {
        return static_cast<Square>((move>>6) & 0x3F);
    }

    inline MoveFlag move_flag(Move move) {
        return static_cast<MoveFlag>((move>>12) & 0xF);
    }

    inline PieceType promotion_type(Move move) {
        return static_cast<PieceType>((move>>16) & 0x7);
    }

    inline bool is_capture(Move move) {
        MoveFlag flag = move_flag(move);
        return flag == CAPTURE || flag == PROMOTION_CAPTURE || flag == EN_PASSANT;
    }

    inline bool is_promotion(Move move) {
        MoveFlag flag = move_flag(move);
        return flag == PROMOTION || flag == PROMOTION_CAPTURE;
    }

    inline bool is_en_passant(Move move) {
        return move_flag(move) == EN_PASSANT;
    }

    inline bool is_castling(Move move) {
        return move_flag(move) == CASTLING;
    }

    inline bool is_double_pawn_push(Move move) {
        return move_flag(move) == DOUBLE_PAWN_PUSH;
    }
}
#endif //CPP_ENGINE_MOVE_H
