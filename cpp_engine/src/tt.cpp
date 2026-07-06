#include "rengine/tt.h"
#include<algorithm>

namespace rengine{
    namespace {
        std::size_t floor_power_of_two(std::size_t value) {
            if (value == 0) {
                return 1;
            }

            std::size_t result = 1;
            while (result <= value / 2) {
                result *= 2;
            }
            return result;
        }
    }

    TranspositionTable::TranspositionTable(std::size_t sz) {
        std::size_t entry_count = floor_power_of_two(sz);
        entries.resize(entry_count);
        mask = entry_count - 1;
    }

    TTEntry *TranspositionTable::probe(ZobristKey key) {
        TTEntry& entry = entries[key & mask];
        if (entry.depth >= 0 && entry.key == key) {
            return &entry;
        }
        return nullptr;
    }

    const TTEntry *TranspositionTable::probe(ZobristKey key) const {
        const TTEntry& entry = entries[key & mask];
        if (entry.depth >= 0 && entry.key == key) {
            return &entry;
        }
        return nullptr;
    }

    void TranspositionTable::store(ZobristKey key, int depth, Score score, TTFlag flag, Move best_move) {
        TTEntry& entry = entries[key & mask];
        bool occupied = entry.depth >= 0;
        bool same_position = entry.key == key;
        bool old_entry = entry.age != current_age;
        if (occupied && !same_position && !old_entry && entry.depth > depth) {
            return;
        }

        entry.key = key;
        entry.best_move = best_move;
        entry.score = score;
        entry.depth = depth;
        entry.flag = flag;
        entry.age = current_age;
    }

    void TranspositionTable::advance_age() {
        ++current_age;
    }

    void TranspositionTable::clear() {
        std::fill(entries.begin(), entries.end(), TTEntry{});
        current_age = 0;
    }

    std::size_t TranspositionTable::size() const {
        return entries.size();
    }

    std::uint8_t TranspositionTable::age() const {
        return current_age;
    }
}
