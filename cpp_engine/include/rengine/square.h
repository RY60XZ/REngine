#ifndef CPP_ENGINE_SQUARE_H
#define CPP_ENGINE_SQUARE_H
#include<rengine/types.h>
#include<cassert>
#include<string>
namespace rengine {
    constexpr int file_of(Square square) {
        assert(square<64);
        return static_cast<int>(square) & 7;
    }
    constexpr int rank_of(Square square) {
        assert(square<64);
        return static_cast<int>(square)>>3;
    }
    constexpr Square make_square(int file, int rank) {
        assert(file>=0 && file<8);
        assert(rank>=0 && rank<8);
        return static_cast<Square>((rank<<3) + file);
    }
    inline std::string square_name(Square square) {
        char f = 'a' + file_of(square);
        char r = '1' + rank_of(square);
        return std::string{f, r};
    }
    inline Square parse_square(const std::string& name) {
        assert(name.size() == 2);
        assert(name[0]>='a' && name[0]<='h');
        assert(name[1]>='1' && name[1]<='8');
        return static_cast<Square>(((name[1]-'1')<<3) | (name[0]-'a'));
    }
}
#endif //CPP_ENGINE_SQUARE_H
