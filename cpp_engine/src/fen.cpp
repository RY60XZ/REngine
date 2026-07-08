#include "rengine/fen.h"
#include "rengine/types.h"
#include "rengine/square.h"
#include "rengine/zobrist.h"
#include<cctype>

namespace rengine {
    void set_from_fen(Board &board, const std::string &fen) {
        clear(board);
        int rank = 7;
        int file = 0;
        int i = 0;

        //position
        while (fen[i]!=' ') {
            char curr_symbol = fen[i];
            if (curr_symbol == '/') {
                --rank;
                file = 0;
                ++i;
                continue;
            }
            if (std::isdigit(static_cast<unsigned char>(curr_symbol))) {
                file += curr_symbol-'0';
                ++i;
                continue;
            }
            Piece curr_piece = parse_piece(curr_symbol);
            Square curr_square = make_square(file, rank);
            set_piece(board, curr_square, curr_piece);
            ++i;
            ++file;
        }

        //side_to_move
        ++i;
        if (fen[i]=='w') board.side_to_move=WHITE;
        else board.side_to_move=BLACK;
        ++i;

        //castling rights
        ++i;
        while (fen[i]!=' ') {
            board.castling_rights |= parse_castling(fen[i]);
            ++i;
        }

        //en_passant
        ++i;
        if (fen[i]=='-') {
            ++i;
        }
        else {
            board.en_passant_square = parse_square(std::string{fen[i], fen[i+1]});
            i+=2;
        }

        //half_move_clock
        ++i;
        board.half_move_clock = 0;
        while (fen[i]!=' ') {
            board.half_move_clock = board.half_move_clock*10 + (fen[i]-'0');
            ++i;
        }

        //full_move_number
        ++i;
        board.full_move_number = 0;
        while (i<static_cast<int>(fen.size()) && fen[i]!=' ') {
            board.full_move_number = board.full_move_number*10 + (fen[i]-'0');
            ++i;
        }

        update_zobrist_key(board);
        reset_position_history(board);
    }

    std::string board_to_fen(const Board &board) {
        std::string fen;

        //position
        for (int rank=7; rank>=0; --rank) {
            int empty_count = 0;
            for (int file=0; file<8; ++file) {
                Square square = make_square(file, rank);
                Piece piece = board.squares[square];
                if (piece == NO_PIECE) {
                    ++empty_count;
                    continue;
                }
                if (empty_count>0) {
                    fen += static_cast<char>('0' + empty_count);
                    empty_count = 0;
                }
                fen += piece_to_char(piece);
            }
            if (empty_count>0) {
                fen += static_cast<char>('0' + empty_count);
            }
            if (rank>0) {
                fen += '/';
            }
        }

        //side_to_move
        fen += ' ';
        fen += board.side_to_move == WHITE ? 'w' : 'b';

        //castling rights
        fen += ' ';
        if (board.castling_rights == 0) {
            fen += '-';
        }
        else {
            if (board.castling_rights & WHITE_KINGSIDE_CASTLING) fen += 'K';
            if (board.castling_rights & WHITE_QUEENSIDE_CASTLING) fen += 'Q';
            if (board.castling_rights & BLACK_KINGSIDE_CASTLING) fen += 'k';
            if (board.castling_rights & BLACK_QUEENSIDE_CASTLING) fen += 'q';
        }

        //en_passant
        fen += ' ';
        if (board.en_passant_square == INVALID_SQUARE) {
            fen += '-';
        }
        else {
            fen += square_name(board.en_passant_square);
        }

        //half_move_clock
        fen += ' ';
        fen += std::to_string(board.half_move_clock);

        //full_move_number
        fen += ' ';
        fen += std::to_string(board.full_move_number);

        return fen;
    }
}
