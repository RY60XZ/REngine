#include"rengine/attack.h"
#include"rengine/fen.h"
#include"rengine/make_move.h"
#include"rengine/move_format.h"
#include"rengine/movegen.h"
#include"rengine/nnue.h"
#include"rengine/search.h"
#include"rengine/square.h"
#include<algorithm>
#include<cctype>
#include<iostream>
#include<string>
#include<vector>

namespace rengine {
    namespace {
        constexpr const char* STARTPOS_FEN =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

        std::string trim(const std::string& text) {
            std::size_t begin = 0;
            while (begin < text.size() &&
                   std::isspace(static_cast<unsigned char>(text[begin]))) {
                ++begin;
            }

            std::size_t end = text.size();
            while (end > begin &&
                   std::isspace(static_cast<unsigned char>(text[end - 1]))) {
                --end;
            }
            return text.substr(begin, end - begin);
        }

        std::string lower_copy(std::string text) {
            for (char& ch : text) {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            return text;
        }

        std::string normalized_move_text(const std::string& text) {
            std::string result;
            for (char ch : trim(text)) {
                if (std::isspace(static_cast<unsigned char>(ch)) ||
                    ch == 'x' || ch == 'X' ||
                    ch == '+' || ch == '#' ||
                    ch == '!' || ch == '?' ||
                    ch == '=') {
                    continue;
                }
                if (ch == '0') {
                    ch = 'O';
                }
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }
            return result;
        }

        char piece_letter(PieceType piece_type) {
            switch (piece_type) {
                case KNIGHT: return 'N';
                case BISHOP: return 'B';
                case ROOK: return 'R';
                case QUEEN: return 'Q';
                case KING: return 'K';
                case PAWN: return '\0';
            }
            return '\0';
        }

        char promotion_letter(PieceType piece_type) {
            switch (piece_type) {
                case KNIGHT: return 'N';
                case BISHOP: return 'B';
                case ROOK: return 'R';
                case QUEEN: return 'Q';
                default: return '?';
            }
        }

        bool same_piece_kind(const Board& board, Move lhs, Move rhs) {
            Piece lhs_piece = board.squares[move_from(lhs)];
            Piece rhs_piece = board.squares[move_from(rhs)];
            return lhs_piece != NO_PIECE &&
                   rhs_piece != NO_PIECE &&
                   color_of(lhs_piece) == color_of(rhs_piece) &&
                   piece_type_of(lhs_piece) == piece_type_of(rhs_piece);
        }

        std::string disambiguation(const Board& board, Move move, const MoveList& legal_moves) {
            Piece moving_piece = board.squares[move_from(move)];
            PieceType moving_type = piece_type_of(moving_piece);
            if (moving_type == PAWN || moving_type == KING) {
                return "";
            }

            bool has_conflict = false;
            bool same_file = false;
            bool same_rank = false;
            Square from = move_from(move);
            for (Move other : legal_moves) {
                if (other == move || move_to(other) != move_to(move) ||
                    !same_piece_kind(board, move, other)) {
                    continue;
                }
                has_conflict = true;
                Square other_from = move_from(other);
                same_file = same_file || file_of(other_from) == file_of(from);
                same_rank = same_rank || rank_of(other_from) == rank_of(from);
            }

            if (!has_conflict) {
                return "";
            }
            if (!same_file) {
                return std::string(1, static_cast<char>('a' + file_of(from)));
            }
            if (!same_rank) {
                return std::string(1, static_cast<char>('1' + rank_of(from)));
            }
            return square_name(from);
        }

        std::string move_to_san(Board& board, Move move, const MoveList& legal_moves) {
            if (is_castling(move)) {
                return move_to(move) == 6 || move_to(move) == 62 ? "O-O" : "O-O-O";
            }

            Piece moving_piece = board.squares[move_from(move)];
            PieceType moving_type = piece_type_of(moving_piece);
            std::string san;
            char letter = piece_letter(moving_type);
            if (letter != '\0') {
                san += letter;
                san += disambiguation(board, move, legal_moves);
            }
            else if (is_capture(move)) {
                san += static_cast<char>('a' + file_of(move_from(move)));
            }

            if (is_capture(move)) {
                san += 'x';
            }
            san += square_name(move_to(move));

            if (is_promotion(move)) {
                san += '=';
                san += promotion_letter(move_promotion_type(move));
            }

            Undo undo{};
            make_move(board, move, undo);
            if (in_check(board, board.side_to_move)) {
                MoveList replies;
                generate_legal_moves(board, replies);
                san += replies.empty() ? '#' : '+';
            }
            unmake_move(board, undo);

            return san;
        }

        bool parse_legal_move(Board& board, const std::string& text, Move& parsed_move,
                              std::vector<std::string>& matching_sans) {
            MoveList legal_moves;
            generate_legal_moves(board, legal_moves);

            const std::string normalized_input = normalized_move_text(text);
            const std::string input_lower = lower_copy(trim(text));
            std::vector<Move> matches;
            for (Move move : legal_moves) {
                std::string san = move_to_san(board, move, legal_moves);
                std::string uci = move_to_uci(move);
                if (normalized_input == normalized_move_text(san) ||
                    normalized_input == normalized_move_text(uci) ||
                    input_lower == lower_copy(uci)) {
                    matches.push_back(move);
                    matching_sans.push_back(san);
                }
            }

            if (matches.size() == 1) {
                parsed_move = matches[0];
                return true;
            }
            return false;
        }

        void print_board(const Board& board, Color human_color) {
            const int rank_begin = human_color == WHITE ? 7 : 0;
            const int rank_end = human_color == WHITE ? -1 : 8;
            const int rank_step = human_color == WHITE ? -1 : 1;
            const int file_begin = human_color == WHITE ? 0 : 7;
            const int file_end = human_color == WHITE ? 8 : -1;
            const int file_step = human_color == WHITE ? 1 : -1;

            std::cout << '\n';
            for (int rank = rank_begin; rank != rank_end; rank += rank_step) {
                std::cout << (rank + 1) << "  ";
                for (int file = file_begin; file != file_end; file += file_step) {
                    Piece piece = board.squares[make_square(file, rank)];
                    std::cout << (piece == NO_PIECE ? '.' : piece_to_char(piece)) << ' ';
                }
                std::cout << '\n';
            }

            std::cout << "\n   ";
            for (int file = file_begin; file != file_end; file += file_step) {
                std::cout << static_cast<char>('a' + file) << ' ';
            }
            std::cout << "\n\n";
        }

        void print_legal_moves(Board& board) {
            MoveList legal_moves;
            generate_legal_moves(board, legal_moves);
            std::cout << "Legal moves:";
            for (Move move : legal_moves) {
                std::cout << ' ' << move_to_san(board, move, legal_moves);
            }
            std::cout << '\n';
        }

        bool game_is_over(Board& board) {
            MoveList legal_moves;
            generate_legal_moves(board, legal_moves);
            if (!legal_moves.empty()) {
                return false;
            }

            if (in_check(board, board.side_to_move)) {
                std::cout << (board.side_to_move == WHITE ? "White" : "Black")
                          << " is checkmated.\n";
            }
            else {
                std::cout << "Stalemate.\n";
            }
            return true;
        }

        int ask_depth() {
            while (true) {
                std::cout << "Engine depth: ";
                std::string line;
                if (!std::getline(std::cin, line)) {
                    return 4;
                }
                try {
                    int depth = std::stoi(trim(line));
                    if (depth > 0) {
                        return depth;
                    }
                }
                catch (...) {
                }
                std::cout << "Please enter a positive integer depth.\n";
            }
        }

        Color ask_human_color() {
            while (true) {
                std::cout << "Play as white or black? [white/black]: ";
                std::string line;
                if (!std::getline(std::cin, line)) {
                    return WHITE;
                }
                line = lower_copy(trim(line));
                if (line == "white" || line == "w") {
                    return WHITE;
                }
                if (line == "black" || line == "b") {
                    return BLACK;
                }
                std::cout << "Please enter white or black.\n";
            }
        }

        void play_game() {
            int engine_depth = ask_depth();
            Color human_color = ask_human_color();

            Board board;
            set_from_fen(board, STARTPOS_FEN);
            clear_search_tables();

            std::cout << "Enter moves in SAN notation. Type board, moves, fen, or quit.\n";

            while (true) {
                if (game_is_over(board)) {
                    return;
                }

                if (board.side_to_move == human_color) {
                    std::cout << "Your move: ";
                    std::string input;
                    if (!std::getline(std::cin, input)) {
                        return;
                    }
                    input = trim(input);
                    std::string command = lower_copy(input);
                    if (command == "quit" || command == "exit" || command == "resign") {
                        std::cout << "Game ended.\n";
                        return;
                    }
                    if (command == "help" || command == "moves") {
                        print_legal_moves(board);
                        continue;
                    }
                    if (command == "board") {
                        print_board(board, human_color);
                        continue;
                    }
                    if (command == "fen") {
                        std::cout << board_to_fen(board) << '\n';
                        continue;
                    }

                    Move move = 0;
                    std::vector<std::string> matching_sans;
                    if (!parse_legal_move(board, input, move, matching_sans)) {
                        if (matching_sans.size() > 1) {
                            std::cout << "Ambiguous move. Matches:";
                            for (const std::string& san : matching_sans) {
                                std::cout << ' ' << san;
                            }
                            std::cout << '\n';
                        }
                        else {
                            std::cout << "Illegal or unrecognized move.\n";
                        }
                        print_legal_moves(board);
                        continue;
                    }

                    MoveList legal_moves;
                    generate_legal_moves(board, legal_moves);
                    Undo undo{};
                    make_move(board, move, undo);
                }
                else {
                    MoveList legal_moves;
                    generate_legal_moves(board, legal_moves);
                    SearchLimits limits;
                    limits.depth = engine_depth;
                    SearchResult result = search_position(board, limits);
                    if (!result.has_best_move) {
                        std::cout << "Engine has no move.\n";
                        return;
                    }

                    std::string san = move_to_san(board, result.best_move, legal_moves);
                    Undo undo{};
                    make_move(board, result.best_move, undo);
                    std::cout << "Engine: " << san
                              << " (" << move_to_uci(result.best_move) << ")\n";
                }
            }
        }
    }
}

int main() {
    rengine::load_default_nnue();
    rengine::play_game();
}
