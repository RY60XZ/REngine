#ifndef CPP_ENGINE_MOVE_FORMAT_H
#define CPP_ENGINE_MOVE_FORMAT_H
#include"rengine/move.h"
#include"rengine/square.h"
#include<cassert>
#include<string>

namespace rengine {
    inline char promotion_suffix(PieceType promotion) {
        switch (promotion) {
            case QUEEN: return 'q';
            case ROOK: return 'r';
            case BISHOP: return 'b';
            case KNIGHT: return 'n';
            default:
                assert(false && "invalid promotion piece type");
                return '?';
        }
    }

    inline std::string move_to_uci(Move move) {
        std::string result = square_name(move_from(move)) + square_name(move_to(move));
        if (is_promotion(move)) {
            result += promotion_suffix(move_promotion_type(move));
        }
        return result;
    }
}

#endif //CPP_ENGINE_MOVE_FORMAT_H
