#include"rengine/uci.h"
#include"rengine/fen.h"
#include"rengine/make_move.h"
#include"rengine/move_format.h"
#include"rengine/movegen.h"
#include"rengine/search.h"
#include"rengine/time.h"
#include<atomic>
#include<chrono>
#include<cstdint>
#include<iostream>
#include<mutex>
#include<sstream>
#include<string>
#include<thread>
#include<vector>

namespace rengine {
    namespace {
        constexpr const char* ENGINE_NAME = "REngine";
        constexpr const char* ENGINE_AUTHOR = "Ryan Yao";
        constexpr const char* STARTPOS_FEN =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

        std::vector<std::string> split_words(const std::string& line) {
            std::istringstream stream(line);
            std::vector<std::string> words;
            std::string word;
            while (stream >> word) {
                words.push_back(word);
            }
            return words;
        }

        std::string join_words(const std::vector<std::string>& words, std::size_t begin, std::size_t end) {
            std::string result;
            for (std::size_t i = begin; i < end; ++i) {
                if (!result.empty()) {
                    result += ' ';
                }
                result += words[i];
            }
            return result;
        }

        std::uint64_t total_nodes(const SearchResult& result) {
            return result.stats.nodes + result.stats.qnodes;
        }

        long long elapsed_ms(const SearchResult& result) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                result.stats.elapsed_time
            ).count();
        }

        std::uint64_t nodes_per_second(const SearchResult& result) {
            long long ms = elapsed_ms(result);
            if (ms <= 0) {
                return 0;
            }
            return total_nodes(result) * 1000ULL / static_cast<std::uint64_t>(ms);
        }

        void append_score(std::ostream& output, Score score) {
            if (is_mate_score(score)) {
                int plies = score > 0 ? VALUE_MATE - score : VALUE_MATE + score;
                int moves = (plies + 1) / 2;
                if (score < 0) {
                    moves = -moves;
                }
                output << "score mate " << moves;
                return;
            }
            output << "score cp " << score;
        }

        TimeSettings parse_go_settings(const std::vector<std::string>& words) {
            TimeSettings settings;
            for (std::size_t i = 1; i < words.size(); ++i) {
                const std::string& word = words[i];
                if (word == "depth" && i + 1 < words.size()) {
                    settings.depth = std::stoi(words[++i]);
                }
                else if (word == "nodes" && i + 1 < words.size()) {
                    settings.nodes = std::stoull(words[++i]);
                }
                else if (word == "movetime" && i + 1 < words.size()) {
                    settings.movetime = std::chrono::milliseconds(std::stoi(words[++i]));
                }
                else if (word == "wtime" && i + 1 < words.size()) {
                    settings.white_time = std::chrono::milliseconds(std::stoi(words[++i]));
                }
                else if (word == "btime" && i + 1 < words.size()) {
                    settings.black_time = std::chrono::milliseconds(std::stoi(words[++i]));
                }
                else if (word == "winc" && i + 1 < words.size()) {
                    settings.white_increment = std::chrono::milliseconds(std::stoi(words[++i]));
                }
                else if (word == "binc" && i + 1 < words.size()) {
                    settings.black_increment = std::chrono::milliseconds(std::stoi(words[++i]));
                }
                else if (word == "movestogo" && i + 1 < words.size()) {
                    settings.moves_to_go = std::stoi(words[++i]);
                }
                else if (word == "infinite") {
                    settings.infinite = true;
                }
            }
            return settings;
        }

        class UciSession {
        public:
            explicit UciSession(std::ostream& output)
                : output(output) {
                set_from_fen(board, STARTPOS_FEN);
            }

            ~UciSession() {
                stop_search(true);
            }

            void handle_command(const std::string& line) {
                std::vector<std::string> words = split_words(line);
                if (words.empty()) {
                    return;
                }

                const std::string& command = words[0];
                if (command == "uci") {
                    write_line(std::string("id name ") + ENGINE_NAME);
                    write_line(std::string("id author ") + ENGINE_AUTHOR);
                    write_line("uciok");
                }
                else if (command == "isready") {
                    write_line("readyok");
                }
                else if (command == "ucinewgame") {
                    stop_search(true);
                    clear_search_tables();
                    set_from_fen(board, STARTPOS_FEN);
                }
                else if (command == "position") {
                    stop_search(true);
                    set_position(words);
                }
                else if (command == "go") {
                    start_search(parse_go_settings(words));
                }
                else if (command == "stop") {
                    stop_search(true);
                }
            }

            void finish() {
                stop_search(search_may_run_forever);
            }

        private:
            std::ostream& output;
            std::mutex output_mutex;
            Board board;
            std::thread search_thread;
            std::atomic_bool stop_requested{false};
            bool search_may_run_forever = false;

            void write_line(const std::string& line) {
                std::lock_guard<std::mutex> lock(output_mutex);
                output << line << '\n' << std::flush;
            }

            void write_info(const SearchResult& result) {
                std::ostringstream line;
                line << "info depth " << result.completed_depth << ' ';
                append_score(line, result.score);
                line << " nodes " << total_nodes(result)
                     << " nps " << nodes_per_second(result)
                     << " time " << elapsed_ms(result);

                if (!result.principal_variation.empty()) {
                    line << " pv";
                    for (Move move : result.principal_variation) {
                        line << ' ' << move_to_uci(move);
                    }
                }

                write_line(line.str());
            }

            void write_bestmove(const SearchResult& result) {
                std::string bestmove = result.has_best_move ? move_to_uci(result.best_move) : "0000";
                write_line(std::string("bestmove ") + bestmove);
            }

            void start_search(const TimeSettings& settings) {
                stop_search(true);
                stop_requested.store(false, std::memory_order_relaxed);

                Board search_board = board;
                SearchLimits limits = make_search_limits(settings, search_board.side_to_move);
                limits.stop = &stop_requested;
                limits.info_callback = [this](const SearchResult& result) {
                    write_info(result);
                };

                search_may_run_forever =
                    limits.infinite &&
                    limits.movetime.count() == 0 &&
                    limits.node_limit == NO_NODE_LIMIT;

                search_thread = std::thread([this, search_board, limits]() mutable {
                    SearchResult result = search_position(search_board, limits);
                    write_bestmove(result);
                });
            }

            void stop_search(bool request_stop) {
                if (!search_thread.joinable()) {
                    return;
                }

                if (request_stop) {
                    stop_requested.store(true, std::memory_order_relaxed);
                }
                search_thread.join();
                search_may_run_forever = false;
            }

            void set_position(const std::vector<std::string>& words) {
                if (words.size() < 2) {
                    return;
                }

                std::size_t move_index = words.size();
                if (words[1] == "startpos") {
                    set_from_fen(board, STARTPOS_FEN);
                    move_index = 2;
                }
                else if (words[1] == "fen") {
                    std::size_t fen_begin = 2;
                    std::size_t fen_end = fen_begin;
                    while (fen_end < words.size() && words[fen_end] != "moves") {
                        ++fen_end;
                    }

                    std::vector<std::string> fen_words(words.begin() + static_cast<long>(fen_begin),
                                                       words.begin() + static_cast<long>(fen_end));
                    if (fen_words.size() == 4) {
                        fen_words.push_back("0");
                        fen_words.push_back("1");
                    }
                    else if (fen_words.size() == 5) {
                        fen_words.push_back("1");
                    }

                    if (fen_words.size() >= 6) {
                        set_from_fen(board, join_words(fen_words, 0, 6));
                    }
                    move_index = fen_end;
                }
                else {
                    return;
                }

                if (move_index < words.size() && words[move_index] == "moves") {
                    ++move_index;
                }
                while (move_index < words.size()) {
                    if (!make_uci_move(board, words[move_index])) {
                        break;
                    }
                    ++move_index;
                }
            }
        };
    }

    bool make_uci_move(Board& board, const std::string& move_text) {
        MoveList legal_moves;
        generate_legal_moves(board, legal_moves);

        for (Move move : legal_moves) {
            if (move_to_uci(move) == move_text) {
                Undo undo{};
                make_move(board, move, undo);
                return true;
            }
        }
        return false;
    }

    int run_uci_loop(std::istream& input, std::ostream& output) {
        UciSession session(output);
        std::string line;
        while (std::getline(input, line)) {
            std::vector<std::string> words = split_words(line);
            if (!words.empty() && words[0] == "quit") {
                session.finish();
                return 0;
            }
            session.handle_command(line);
        }
        session.finish();
        return 0;
    }
}
