#include <cstdint>
#include "rengine/board.h"
#include "rengine/perft.h"
#include "rengine/make_move.h"
#include "rengine/movegen.h"

namespace rengine {
    uint64_t perft(Board& board, int depth) {
        if (depth == 0) return 1;
        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);
        if (depth == 1) {
            return legal_moves.size();
        }
        uint64_t result = 0;
        Undo undo;
        for (Move move : legal_moves) {
            make_move(board, move, undo);
            result += perft(board, depth - 1);
            unmake_move(board, undo);
        }
        return result;
    }
}