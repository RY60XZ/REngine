#ifndef CPP_ENGINE_SEARCH_TYPES_H
#define CPP_ENGINE_SEARCH_TYPES_H

namespace rengine {
    using Score = int;

    constexpr Score VALUE_DRAW = 0;
    constexpr Score VALUE_MATE = 32000;
    constexpr Score VALUE_INF = 32767;
    constexpr int MAX_PLY = 256;
    constexpr Score VALUE_MATE_IN_MAX_PLY = VALUE_MATE - MAX_PLY;
    constexpr Score VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + MAX_PLY;

    constexpr Score mate_in(int ply) {
        return VALUE_MATE - ply;
    }

    constexpr Score mated_in(int ply) {
        return -VALUE_MATE + ply;
    }

    constexpr bool is_mate_score(Score score) {
        return (score >= VALUE_MATE_IN_MAX_PLY && score < VALUE_INF) ||
               (score <= VALUE_MATED_IN_MAX_PLY && score > -VALUE_INF);
    }

    constexpr Score score_to_tt(Score score, int ply) {
        if (score >= VALUE_MATE_IN_MAX_PLY && score < VALUE_INF) {
            return score + ply;
        }
        if (score <= VALUE_MATED_IN_MAX_PLY && score > -VALUE_INF) {
            return score - ply;
        }
        return score;
    }

    constexpr Score score_from_tt(Score score, int ply) {
        if (score >= VALUE_MATE_IN_MAX_PLY && score < VALUE_INF) {
            return score - ply;
        }
        if (score <= VALUE_MATED_IN_MAX_PLY && score > -VALUE_INF) {
            return score + ply;
        }
        return score;
    }
}

#endif //CPP_ENGINE_SEARCH_TYPES_H
