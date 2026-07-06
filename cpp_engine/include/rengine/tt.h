#ifndef CPP_ENGINE_TT_H
#define CPP_ENGINE_TT_H
#include"rengine/search_types.h"
#include<cstddef>
#include<cstdint>
#include<vector>

namespace rengine {
    enum TTFlag {
        TT_EXACT,
        TT_LOWER,
        TT_UPPER
    };

    struct TTEntry {
        ZobristKey key = 0;
        Move best_move = 0;
        Score score = 0;
        int depth = -1;
        TTFlag flag = TT_EXACT;
        std::uint8_t age = 0;
    };

    class TranspositionTable {
    public:
        explicit TranspositionTable(std::size_t sz);

        TTEntry* probe(ZobristKey key);
        const TTEntry* probe(ZobristKey key) const;
        void store(ZobristKey key, int depth, Score score, TTFlag flag, Move best_move);
        void advance_age();
        void clear();

        std::size_t size() const;
        std::uint8_t age() const;

    private:
        std::vector<TTEntry> entries;
        std::size_t mask = 0;
        std::uint8_t current_age = 0;
    };
}
#endif //CPP_ENGINE_TT_H
