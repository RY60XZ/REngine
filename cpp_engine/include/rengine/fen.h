#ifndef CPP_ENGINE_FEN_H
#define CPP_ENGINE_FEN_H
#include<rengine/board.h>
#include<rengine/types.h>
#include<string>

namespace rengine {
    inline Piece parse_piece(char c) {
        switch (c) {
            case 'P': return WHITE_PAWN;
            case 'N': return WHITE_KNIGHT;
            case 'B': return WHITE_BISHOP;
            case 'R': return WHITE_ROOK;
            case 'Q': return WHITE_QUEEN;
            case 'K': return WHITE_KING;

            case 'p': return BLACK_PAWN;
            case 'n': return BLACK_KNIGHT;
            case 'b': return BLACK_BISHOP;
            case 'r': return BLACK_ROOK;
            case 'q': return BLACK_QUEEN;
            case 'k': return BLACK_KING;

            default:  return NO_PIECE;
        }
    }
    inline char piece_to_char(Piece piece) {
        switch (piece) {
            case WHITE_PAWN: return 'P';
            case WHITE_KNIGHT: return 'N';
            case WHITE_BISHOP: return 'B';
            case WHITE_ROOK: return 'R';
            case WHITE_QUEEN: return 'Q';
            case WHITE_KING: return 'K';

            case BLACK_PAWN: return 'p';
            case BLACK_KNIGHT: return 'n';
            case BLACK_BISHOP: return 'b';
            case BLACK_ROOK: return 'r';
            case BLACK_QUEEN: return 'q';
            case BLACK_KING: return 'k';

            default: return ' ';
        }
    }
    inline unsigned parse_castling(char c) {
        switch (c) {
            case 'K': return WHITE_KINGSIDE_CASTLING;
            case 'Q': return WHITE_QUEENSIDE_CASTLING;
            case 'k': return BLACK_KINGSIDE_CASTLING;
            case 'q': return BLACK_QUEENSIDE_CASTLING;
            default: return 0;
        }
    }
    void set_from_fen(Board& board, const std::string& fen);
    std::string board_to_fen(const Board& board);
}
#endif //CPP_ENGINE_FEN_H
