#ifndef CPP_ENGINE_DRAW_H
#define CPP_ENGINE_DRAW_H
#include"rengine/board.h"

namespace rengine {
    inline bool is_fifty_move_rule_draw(const Board& board) {
        return board.half_move_clock>=100;
    }
    inline bool is_insufficient_material_draw(const Board& board) {
        (void)board;
        return false;
    }
    inline bool is_threefold_draw(const Board& board) {
        (void)board;
        return false;
    }
}
#endif //CPP_ENGINE_DRAW_H
