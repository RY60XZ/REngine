#include "rengine/nnue.h"
#include "rengine/eval.h"
#include "rengine/search_types.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace rengine {
    namespace {
        constexpr char NNUE_MAGIC[8] = {'R', 'E', 'N', 'N', 'U', 'E', '4', '\0'};
        constexpr std::uint32_t NNUE_FORMAT_VERSION = 4;
        constexpr int KING_BUCKETS = 64;
        constexpr int PIECE_KINDS = 12;
        constexpr int SQUARES = 64;
        constexpr int HALFKP_FEATURES = KING_BUCKETS * PIECE_KINDS * SQUARES;
        constexpr int CASTLING_FEATURE_BASE = HALFKP_FEATURES;
        constexpr int CASTLING_FEATURES = 4;
        constexpr int NNUE_FEATURES = HALFKP_FEATURES + CASTLING_FEATURES;
        constexpr int NNUE_INPUT_SIZE = NNUE_HIDDEN_SIZE * 2;

        struct Network {
            bool loaded = false;
            int features = 0;
            int hidden = 0;
            int head_hidden = 0;
            int max_active = 0;
            int accumulator_scale = 0;
            int fc1_weight_scale = 0;
            int out_weight_scale = 0;
            int activation_clip_q = 0;
            float target_scale = 1000.0f;
            float activation_clip = 1.0f;
            double output_to_cp = 0.0;
            std::vector<std::int16_t> embedding;
            std::array<std::int32_t, NNUE_HIDDEN_SIZE> pad_base{};
            std::vector<std::int16_t> fc1_weight;
            std::vector<std::int32_t> fc1_bias;
            std::vector<std::int16_t> out_weight;
            std::int32_t out_bias = 0;
        };

        Network g_nnue;

        template <typename T>
        bool read_value(std::ifstream& in, T& value) {
            in.read(reinterpret_cast<char*>(&value), sizeof(T));
            return static_cast<bool>(in);
        }

        template <typename T>
        bool read_values(std::ifstream& in, std::vector<T>& values, std::size_t count) {
            values.resize(count);
            in.read(reinterpret_cast<char*>(values.data()),
                    static_cast<std::streamsize>(count * sizeof(T)));
            return static_cast<bool>(in);
        }

        constexpr Square relative_square(Square square, Color perspective) {
            return perspective == WHITE ? square : square ^ 56;
        }

        Square find_king_square(const Board& board, Color color) {
            Bitboard king = board.pieces[color][KING];
            if (king == 0) {
                return INVALID_SQUARE;
            }
            return static_cast<Square>(std::countr_zero(king));
        }

        int halfkp_feature(Square king, Piece piece, Square piece_square, Color perspective) {
            Color piece_color = color_of(piece);
            PieceType piece_type = piece_type_of(piece);
            int rel_king = static_cast<int>(relative_square(king, perspective));
            int rel_square = static_cast<int>(relative_square(piece_square, perspective));
            int rel_piece = (piece_color == perspective ? 0 : 6) + static_cast<int>(piece_type);
            return (rel_king * PIECE_KINDS + rel_piece) * SQUARES + rel_square;
        }

        void add_feature(NnueAccumulator& accumulator, Color perspective, int feature, int sign) {
            const std::int16_t* row = g_nnue.embedding.data() +
                                      static_cast<std::size_t>(feature) * NNUE_HIDDEN_SIZE;
            auto& acc = accumulator[perspective];
            for (int i = 0; i < NNUE_HIDDEN_SIZE; ++i) {
                acc[i] += sign * row[i];
            }
        }

        void add_piece_feature(const Board& board, NnueAccumulator& accumulator,
                               Piece piece, Square square, int sign) {
            for (Color perspective : {WHITE, BLACK}) {
                Square king = find_king_square(board, perspective);
                if (king == INVALID_SQUARE) {
                    continue;
                }
                int feature = halfkp_feature(king, piece, square, perspective);
                add_feature(accumulator, perspective, feature, sign);
            }
        }

        void add_castling_features(NnueAccumulator& accumulator, unsigned rights, int sign) {
            if (rights & WHITE_KINGSIDE_CASTLING) {
                add_feature(accumulator, WHITE, CASTLING_FEATURE_BASE + 0, sign);
                add_feature(accumulator, BLACK, CASTLING_FEATURE_BASE + 2, sign);
            }
            if (rights & WHITE_QUEENSIDE_CASTLING) {
                add_feature(accumulator, WHITE, CASTLING_FEATURE_BASE + 1, sign);
                add_feature(accumulator, BLACK, CASTLING_FEATURE_BASE + 3, sign);
            }
            if (rights & BLACK_KINGSIDE_CASTLING) {
                add_feature(accumulator, WHITE, CASTLING_FEATURE_BASE + 2, sign);
                add_feature(accumulator, BLACK, CASTLING_FEATURE_BASE + 0, sign);
            }
            if (rights & BLACK_QUEENSIDE_CASTLING) {
                add_feature(accumulator, WHITE, CASTLING_FEATURE_BASE + 3, sign);
                add_feature(accumulator, BLACK, CASTLING_FEATURE_BASE + 1, sign);
            }
        }

        bool recompute_accumulator(const Board& board, NnueAccumulator& accumulator) {
            for (auto& side_accumulator : accumulator) {
                side_accumulator = g_nnue.pad_base;
            }

            if (find_king_square(board, WHITE) == INVALID_SQUARE ||
                find_king_square(board, BLACK) == INVALID_SQUARE) {
                return false;
            }

            for (Color color : {WHITE, BLACK}) {
                for (int piece_type = PAWN; piece_type <= KING; ++piece_type) {
                    Bitboard pieces = board.pieces[color][piece_type];
                    while (pieces != 0) {
                        Square square = static_cast<Square>(std::countr_zero(pieces));
                        Piece piece = static_cast<Piece>(color * 6 + piece_type + 1);
                        add_piece_feature(board, accumulator, piece, square, 1);
                        pieces &= pieces - 1;
                    }
                }
            }

            add_castling_features(accumulator, board.castling_rights, 1);
            return true;
        }

        Piece promoted_piece_for(Piece moved_piece, PieceType promotion_type) {
            return static_cast<Piece>(color_of(moved_piece) * 6 + promotion_type + 1);
        }

        std::int32_t clipped_accumulator(std::int32_t value) {
            return std::clamp(value, 0, g_nnue.activation_clip_q);
        }

        std::int32_t rounded_divide(std::int64_t value, std::int32_t divisor) {
            if (value >= 0) {
                return static_cast<std::int32_t>((value + divisor / 2) / divisor);
            }
            return -static_cast<std::int32_t>((-value + divisor / 2) / divisor);
        }

        int forward_correction_cp(const Board& board) {
            const auto& first = board.nnue_accumulator[board.side_to_move];
            const auto& second = board.nnue_accumulator[opposite_color(board.side_to_move)];
            std::array<std::int32_t, NNUE_INPUT_SIZE> input{};
            for (int i = 0; i < NNUE_HIDDEN_SIZE; ++i) {
                input[i] = clipped_accumulator(first[i]);
                input[NNUE_HIDDEN_SIZE + i] = clipped_accumulator(second[i]);
            }

            std::int64_t output = g_nnue.out_bias;
            for (int hidden_index = 0; hidden_index < g_nnue.head_hidden; ++hidden_index) {
                const std::int16_t* weights = g_nnue.fc1_weight.data() +
                                              static_cast<std::size_t>(hidden_index) * NNUE_INPUT_SIZE;
                std::int64_t value = g_nnue.fc1_bias[hidden_index];

                for (int i = 0; i < NNUE_INPUT_SIZE; ++i) {
                    value += static_cast<std::int64_t>(weights[i]) * input[i];
                }

                std::int32_t hidden_value = std::clamp(
                    rounded_divide(value, g_nnue.fc1_weight_scale),
                    0,
                    g_nnue.activation_clip_q
                );
                output += static_cast<std::int64_t>(g_nnue.out_weight[hidden_index]) * hidden_value;
            }
            return static_cast<int>(std::lround(static_cast<double>(output) * g_nnue.output_to_cp));
        }
    }

    bool load_nnue(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return false;
        }

        char magic[8]{};
        in.read(magic, sizeof(magic));
        if (!in || std::memcmp(magic, NNUE_MAGIC, sizeof(magic)) != 0) {
            return false;
        }

        std::uint32_t version = 0;
        std::uint32_t features = 0;
        std::uint32_t hidden = 0;
        std::uint32_t head_hidden = 0;
        std::uint32_t max_active = 0;
        std::uint32_t flags = 0;
        std::uint32_t accumulator_scale = 0;
        std::uint32_t fc1_weight_scale = 0;
        std::uint32_t out_weight_scale = 0;
        float target_scale = 0.0f;
        float activation_clip = 0.0f;

        if (!read_value(in, version) ||
            !read_value(in, features) ||
            !read_value(in, hidden) ||
            !read_value(in, head_hidden) ||
            !read_value(in, max_active) ||
            !read_value(in, flags) ||
            !read_value(in, target_scale) ||
            !read_value(in, activation_clip) ||
            !read_value(in, accumulator_scale) ||
            !read_value(in, fc1_weight_scale) ||
            !read_value(in, out_weight_scale)) {
            return false;
        }

        if (version != NNUE_FORMAT_VERSION ||
            features != NNUE_FEATURES ||
            hidden != NNUE_HIDDEN_SIZE ||
            head_hidden == 0 ||
            max_active == 0 ||
            activation_clip <= 0.0f ||
            target_scale <= 0.0f ||
            accumulator_scale == 0 ||
            fc1_weight_scale == 0 ||
            out_weight_scale == 0) {
            return false;
        }

        Network next;
        next.features = static_cast<int>(features);
        next.hidden = static_cast<int>(hidden);
        next.head_hidden = static_cast<int>(head_hidden);
        next.max_active = static_cast<int>(max_active);
        next.accumulator_scale = static_cast<int>(accumulator_scale);
        next.fc1_weight_scale = static_cast<int>(fc1_weight_scale);
        next.out_weight_scale = static_cast<int>(out_weight_scale);
        next.target_scale = target_scale;
        next.activation_clip = activation_clip;
        next.activation_clip_q = static_cast<int>(
            std::lround(static_cast<double>(activation_clip) * next.accumulator_scale)
        );
        next.output_to_cp = static_cast<double>(target_scale) /
                            (static_cast<double>(next.accumulator_scale) * next.out_weight_scale);

        if (!read_values(in, next.embedding,
                         static_cast<std::size_t>(next.features) * next.hidden) ||
            !read_value(in, next.pad_base) ||
            !read_values(in, next.fc1_weight,
                         static_cast<std::size_t>(next.head_hidden) * NNUE_INPUT_SIZE) ||
            !read_values(in, next.fc1_bias, next.head_hidden) ||
            !read_values(in, next.out_weight, next.head_hidden) ||
            !read_value(in, next.out_bias)) {
            return false;
        }

        next.loaded = true;
        g_nnue = std::move(next);
        (void)flags;
        return true;
    }

    bool load_default_nnue() {
        if (const char* path = std::getenv("RENGINE_NNUE")) {
            if (load_nnue(path)) {
                return true;
            }
        }

        const std::array<std::string, 5> candidates = {
            "../models/eval_mlp/eval_mlp.nnue",
            "../../models/eval_mlp/eval_mlp.nnue",
            "models/eval_mlp/eval_mlp.nnue",
            "../../../models/eval_mlp/eval_mlp.nnue",
            "eval_mlp.nnue",
        };

        for (const std::string& path : candidates) {
            if (load_nnue(path)) {
                return true;
            }
        }
        return false;
    }

    bool nnue_is_loaded() {
        return g_nnue.loaded;
    }

    void refresh_nnue(Board& board) {
        if (!g_nnue.loaded) {
            board.nnue_dirty = true;
            board.nnue_initialized = false;
            return;
        }

        if (!recompute_accumulator(board, board.nnue_accumulator)) {
            board.nnue_dirty = true;
            board.nnue_initialized = false;
            return;
        }

        board.nnue_dirty = false;
        board.nnue_initialized = true;
    }

    void update_nnue_after_move(Board& board, Move move, Piece moved_piece,
                                Piece captured_piece, unsigned old_castling_rights) {
        if (!g_nnue.loaded) {
            board.nnue_dirty = true;
            board.nnue_initialized = false;
            return;
        }
        if (!board.nnue_initialized || board.nnue_dirty || moved_piece == NO_PIECE) {
            return;
        }

        MoveFlag flag = move_flag(move);
        if (piece_type_of(moved_piece) == KING || flag == CASTLING) {
            refresh_nnue(board);
            return;
        }

        Square from = move_from(move);
        Square to = move_to(move);
        add_piece_feature(board, board.nnue_accumulator, moved_piece, from, -1);

        if (flag == CAPTURE || flag == PROMOTION_CAPTURE) {
            if (captured_piece != NO_PIECE) {
                add_piece_feature(board, board.nnue_accumulator, captured_piece, to, -1);
            }
        }
        else if (flag == EN_PASSANT) {
            Square captured_square = color_of(moved_piece) == WHITE ? to - 8 : to + 8;
            if (captured_piece != NO_PIECE) {
                add_piece_feature(board, board.nnue_accumulator, captured_piece, captured_square, -1);
            }
        }

        Piece placed_piece = moved_piece;
        if (flag == PROMOTION || flag == PROMOTION_CAPTURE) {
            placed_piece = promoted_piece_for(moved_piece, move_promotion_type(move));
        }
        add_piece_feature(board, board.nnue_accumulator, placed_piece, to, 1);

        unsigned new_castling_rights = board.castling_rights;
        if (old_castling_rights != new_castling_rights) {
            add_castling_features(board.nnue_accumulator, old_castling_rights, -1);
            add_castling_features(board.nnue_accumulator, new_castling_rights, 1);
        }
    }

    void update_nnue_after_unmove(Board& board, Move move, Piece moved_piece,
                                  Piece captured_piece, unsigned post_castling_rights,
                                  bool previous_dirty, bool previous_initialized) {
        if (!g_nnue.loaded) {
            board.nnue_dirty = true;
            board.nnue_initialized = false;
            return;
        }
        if (!previous_initialized || previous_dirty || moved_piece == NO_PIECE) {
            board.nnue_dirty = previous_dirty;
            board.nnue_initialized = previous_initialized;
            return;
        }

        MoveFlag flag = move_flag(move);
        if (piece_type_of(moved_piece) == KING || flag == CASTLING ||
            !board.nnue_initialized || board.nnue_dirty) {
            refresh_nnue(board);
            return;
        }

        Square from = move_from(move);
        Square to = move_to(move);
        Piece placed_piece = moved_piece;
        if (flag == PROMOTION || flag == PROMOTION_CAPTURE) {
            placed_piece = promoted_piece_for(moved_piece, move_promotion_type(move));
        }

        add_piece_feature(board, board.nnue_accumulator, placed_piece, to, -1);
        if (flag == CAPTURE || flag == PROMOTION_CAPTURE) {
            if (captured_piece != NO_PIECE) {
                add_piece_feature(board, board.nnue_accumulator, captured_piece, to, 1);
            }
        }
        else if (flag == EN_PASSANT) {
            Square captured_square = color_of(moved_piece) == WHITE ? to - 8 : to + 8;
            if (captured_piece != NO_PIECE) {
                add_piece_feature(board, board.nnue_accumulator, captured_piece, captured_square, 1);
            }
        }
        add_piece_feature(board, board.nnue_accumulator, moved_piece, from, 1);

        unsigned previous_castling_rights = board.castling_rights;
        if (post_castling_rights != previous_castling_rights) {
            add_castling_features(board.nnue_accumulator, post_castling_rights, -1);
            add_castling_features(board.nnue_accumulator, previous_castling_rights, 1);
        }

        board.nnue_dirty = false;
        board.nnue_initialized = true;
    }

    Score evaluate_nnue(const Board& board) {
        if (!g_nnue.loaded) {
            return evaluate_pst(board);
        }

        if (!board.nnue_initialized || board.nnue_dirty) {
            refresh_nnue(const_cast<Board&>(board));
        }
        if (!board.nnue_initialized) {
            return evaluate_pst(board);
        }

        int score = evaluate_pst(board) + forward_correction_cp(board);
        return std::clamp(score, VALUE_MATED_IN_MAX_PLY + 1, VALUE_MATE_IN_MAX_PLY - 1);
    }

    bool nnue_accumulator_matches_recompute(const Board& board, int epsilon) {
        if (!g_nnue.loaded || !board.nnue_initialized || board.nnue_dirty) {
            return false;
        }

        NnueAccumulator expected{};
        if (!recompute_accumulator(board, expected)) {
            return false;
        }

        for (int perspective = WHITE; perspective <= BLACK; ++perspective) {
            for (int i = 0; i < NNUE_HIDDEN_SIZE; ++i) {
                if (std::abs(expected[perspective][i] - board.nnue_accumulator[perspective][i]) > epsilon) {
                    return false;
                }
            }
        }
        return true;
    }
}
