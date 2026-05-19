#ifndef CPP_ENGINE_TYPES_H
#define CPP_ENGINE_TYPES_H

#include <cstdint>

namespace rengine {
    using Bitboard = std::uint64_t;
    using Square = unsigned; //0xFFFFFFFF
    enum Color {WHITE = 0, BLACK =1};
    enum PieceType {
        PAWN = 0,
        KNIGHT,
        BISHOP,
        ROOK,
        QUEEN,
        KING,
    };
    enum Piece {
        NO_PIECE = 0,
        WHITE_PAWN,
        WHITE_KNIGHT,
        WHITE_BISHOP,
        WHITE_ROOK,
        WHITE_QUEEN,
        WHITE_KING,
        BLACK_PAWN,
        BLACK_KNIGHT,
        BLACK_BISHOP,
        BLACK_ROOK,
        BLACK_QUEEN,
        BLACK_KING
    };
    constexpr unsigned WHITE_KINGSIDE_CASTLING = 1;
    constexpr unsigned WHITE_QUEENSIDE_CASTLING = 2;
    constexpr unsigned BLACK_KINGSIDE_CASTLING = 4;
    constexpr unsigned BLACK_QUEENSIDE_CASTLING = 8;
    constexpr unsigned INVALID_SQUARE = ~Square{0};

}
#endif //CPP_ENGINE_TYPES_H