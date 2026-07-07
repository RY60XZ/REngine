#ifndef CPP_ENGINE_MOVE_LIST_H
#define CPP_ENGINE_MOVE_LIST_H
#include<array>
#include<cassert>
#include "rengine/move.h"
namespace rengine {
    inline constexpr int MAX_MOVES = 512;
    struct MoveList {
        std::array<Move, MAX_MOVES> moves{};
        int count = 0;

        void clear() {
            count = 0;
        }

        void push_back(Move move) {
            assert(count < MAX_MOVES);
            moves[count++] = move;
        }

        void append(const Move* begin, const Move* end) {
            int len = static_cast<int>(end - begin);
            assert(count + len <= MAX_MOVES);
            for (int i = 0; i < len; ++i) {
                moves[count++] = begin[i];
            }
        }

        [[nodiscard]] int size() const {
            return count;
        }

        [[nodiscard]] bool empty() const {
            return count == 0;
        }

        [[nodiscard]] Move operator[](int index) const {
            assert(index>=0 && index < count);
            return moves[index];
        }

        Move* begin() {
            return moves.data();
        }

        Move* end() {
            return moves.data() + count;
        }

        [[nodiscard]] const Move* begin() const {
            return moves.data();
        }

        [[nodiscard]] const Move* end() const {
            return moves.data() + count;
        }

    };
}
#endif //CPP_ENGINE_MOVE_LIST_H
