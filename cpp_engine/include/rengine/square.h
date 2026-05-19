#ifndef CPP_ENGINE_SQUARE_H
#define CPP_ENGINE_SQUARE_H
#include "types.h"
#include "string"
namespace rengine {
    inline int file_of(Square square) {
        return static_cast<int>(square)>>3;
    }
    inline int rank_of(Square square) {
        return static_cast<int>(square) & 7;
    }
    inline Square make_square(int file, int rank) {
        return static_cast<Square>((file<<3) + rank);
    }
    inline std::string square_name(Square square) {
        char f = 'a' + file_of(square);
        char r = '1' + rank_of(square);
        return std::string{f, r};
    }
    inline Square parse_square(const std::string& name) {
        return static_cast<Square>(((name[1]-'1')<<3) | (name[0]-'a'));
    }
}
#endif //CPP_ENGINE_SQUARE_H