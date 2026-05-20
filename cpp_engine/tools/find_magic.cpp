#include "rengine/board.h"
#include "rengine/magic_tables.h"
#include "rengine/square.h"
#include "rengine/types.h"
#include <array>
#include <bit>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace rengine;

// Performance is not the biggest concern, since the script is for precomputation.
// It still needs to be reasonably fast, however.

static Bitboard random_state = 1804289383ULL;

Bitboard random_u64() {
    Bitboard z = (random_state += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;

    return z ^ (z >> 31);
}

constexpr bool is_on_board(int file, int rank) {
    return file >= 0 && file < 8 && rank >= 0 && rank < 8;
}

vector<Square> generate_bishop_squares(Square from) {
    vector<Square> bishop_squares;
    int from_file = file_of(from), from_rank = rank_of(from);
    int curr_file, curr_rank;
    int count;

    curr_file = from_file+1, curr_rank = from_rank+1, count =0;
    while (is_on_board(curr_file, curr_rank)) {
        bishop_squares.push_back(make_square(curr_file++, curr_rank++));
        count++;
    }
    if (count) bishop_squares.pop_back();

    curr_file = from_file+1, curr_rank = from_rank-1, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        bishop_squares.push_back(make_square(curr_file++, curr_rank--));
        count++;
    }
    if (count) bishop_squares.pop_back();

    curr_file = from_file-1, curr_rank = from_rank-1, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        bishop_squares.push_back(make_square(curr_file--, curr_rank--));
        count++;
    }
    if (count) bishop_squares.pop_back();

    curr_file = from_file-1, curr_rank = from_rank+1, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        bishop_squares.push_back(make_square(curr_file--, curr_rank++));
        count++;
    }
    if (count) bishop_squares.pop_back();

    return bishop_squares;
}

Bitboard generate_bishop_bitboard_mask(const vector<Square>& bishop_squares) {
    auto bishop_bitboard = Bitboard{0};
    for (auto s : bishop_squares) {
        bishop_bitboard |= Bitboard{1} << s;
    }
    return bishop_bitboard;
}

vector<Square> generate_rook_squares(Square from) {
    vector<Square> rook_squares;
    int from_file = file_of(from), from_rank = rank_of(from);
    int curr_file, curr_rank;
    int count;

    curr_file = from_file, curr_rank = from_rank+1, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        rook_squares.push_back(make_square(curr_file, curr_rank++));
        count++;
    }
    if (count) rook_squares.pop_back();

    curr_file = from_file, curr_rank = from_rank-1, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        rook_squares.push_back(make_square(curr_file, curr_rank--));
        count++;
    }
    if (count) rook_squares.pop_back();

    curr_file = from_file+1, curr_rank = from_rank, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        rook_squares.push_back(make_square(curr_file++, curr_rank));
        count++;
    }
    if (count) rook_squares.pop_back();

    curr_file = from_file-1, curr_rank = from_rank, count = 0;
    while (is_on_board(curr_file, curr_rank)) {
        rook_squares.push_back(make_square(curr_file--, curr_rank));
        count++;
    }
    if (count) rook_squares.pop_back();

    return rook_squares;
}

Bitboard generate_rook_bitboard_mask(const vector<Square>& rook_squares) {
    auto rook_bitboard = Bitboard{0};
    for (auto s : rook_squares) {
        rook_bitboard |= Bitboard{1} << s;
    }
    return rook_bitboard;
}

void generate_blockage_bitboards(size_t i, Bitboard curr, const vector<Square>& possible_squares, vector<Bitboard>& result) {
    if (i == possible_squares.size()) {
        result.push_back(curr);
        return;
    }
    curr |= Bitboard{1} << possible_squares[i];
    generate_blockage_bitboards(i+1, curr, possible_squares, result);
    curr &= ~(Bitboard{1} << possible_squares[i]);
    generate_blockage_bitboards(i+1, curr, possible_squares, result);
}

Bitboard scan_bishop_attack(Square from, Bitboard blockage) {
    auto attack = Bitboard{0};
    int from_file = file_of(from), from_rank = rank_of(from);
    int curr_file, curr_rank;
    curr_file = from_file+1, curr_rank = from_rank+1;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        ++curr_file;
        ++curr_rank;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    curr_file = from_file+1, curr_rank = from_rank-1;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        ++curr_file;
        --curr_rank;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    curr_file = from_file-1, curr_rank = from_rank-1;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        --curr_file;
        --curr_rank;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    curr_file = from_file-1, curr_rank = from_rank+1;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        --curr_file;
        ++curr_rank;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    return attack;
}

Bitboard scan_rook_attack(Square from, Bitboard blockage) {
    auto attack = Bitboard{0};
    int from_file = file_of(from), from_rank = rank_of(from);
    int curr_file, curr_rank;
    curr_file = from_file, curr_rank = from_rank+1;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        ++curr_rank;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    curr_file = from_file, curr_rank = from_rank-1;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        --curr_rank;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    curr_file = from_file+1, curr_rank = from_rank;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        ++curr_file;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    curr_file = from_file-1, curr_rank = from_rank;
    while (is_on_board(curr_file, curr_rank) && (Bitboard{1} << make_square(curr_file, curr_rank) & blockage) == 0) {
        attack |= Bitboard{1} << make_square(curr_file, curr_rank);
        --curr_file;
    }
    if (is_on_board(curr_file, curr_rank)) attack |= Bitboard{1} << make_square(curr_file, curr_rank);
    return attack;
}

Bitboard find_magic(const vector<Bitboard>& blockage_bitboards, const vector<Bitboard>& attacks, Bitboard mask, int index_bits) {
    vector<Bitboard> used(size_t{1} << index_bits);
    vector<bool> occupied(size_t{1} << index_bits);

    while (true) {
        bool flag = true;
        Bitboard magic = random_u64() & random_u64() & random_u64();
        if (popcount((mask * magic) & 0xFF00000000000000ULL) < 6) {
            continue;
        }
        fill(occupied.begin(), occupied.end(), false);

        for (size_t i = 0; i < blockage_bitboards.size(); ++i) {
            auto key = static_cast<size_t>((blockage_bitboards[i] * magic) >> (64 - index_bits));
            if (occupied[key]) {
                if (used[key] != attacks[i]) {
                    flag = false;
                    break;
                }
            }
            else {
                occupied[key] = true;
                used[key] = attacks[i];
            }
        }

        if (flag) return magic;
    }
}

void write_bitboard(ostream& out, Bitboard b) {
    out << "0x"
        << hex << setw(16) << setfill('0') << b
        << "ULL" << dec;
}

void write_magic_entries(ostream& out, const char* name, const array<MagicEntry, 64>& entries) {
    out << "const std::array<MagicEntry, 64> " << name << " = {{\n";
    for (const MagicEntry& entry : entries) {
        out << "    {";
        write_bitboard(out, entry.mask);
        out << ", ";
        write_bitboard(out, entry.magic);
        out << ", " << entry.index_bits << ", " << entry.offset << "},\n";
    }
    out << "}};\n\n";
}

template <size_t N>
void write_attack_table(ostream& out, const char* name, const char* size_name, const array<Bitboard, N>& attacks) {
    out << "const std::array<Bitboard, " << size_name << "> " << name << " = {{\n";
    for (Bitboard b : attacks) {
        out << "    ";
        write_bitboard(out, b);
        out << ",\n";
    }
    out << "}};\n\n";
}

array<MagicEntry, 64> generated_rook_magic_entries{};
array<MagicEntry, 64> generated_bishop_magic_entries{};
array<Bitboard, ROOK_ATTACK_TABLE_SIZE> rook_attack_table{};
array<Bitboard, BISHOP_ATTACK_TABLE_SIZE> bishop_attack_table{};

int main() {
    size_t bishop_offset = 0;
    size_t rook_offset = 0;

    for (Square s = 0; s < 64; ++s) {
        auto bishop_squares = generate_bishop_squares(s);
        vector<Bitboard> bishop_blockage_bitboards;
        generate_blockage_bitboards(0, Bitboard{0}, bishop_squares, bishop_blockage_bitboards);
        vector<Bitboard> bishop_scan_attacks(bishop_blockage_bitboards.size());
        for (size_t i = 0; i < bishop_blockage_bitboards.size(); ++i) {
            bishop_scan_attacks[i] = scan_bishop_attack(s, bishop_blockage_bitboards[i]);
        }

        int bishop_index_bits = static_cast<int>(bishop_squares.size());
        Bitboard bishop_mask = generate_bishop_bitboard_mask(bishop_squares);
        Bitboard bishop_magic = find_magic(bishop_blockage_bitboards, bishop_scan_attacks, bishop_mask, bishop_index_bits);
        generated_bishop_magic_entries[s] = {bishop_mask, bishop_magic, bishop_index_bits, bishop_offset};
        for (size_t i = 0; i < bishop_blockage_bitboards.size(); ++i) {
            auto key = static_cast<size_t>((bishop_blockage_bitboards[i] * bishop_magic) >> (64 - bishop_index_bits));
            bishop_attack_table[bishop_offset + key] = bishop_scan_attacks[i];
        }
        bishop_offset += size_t{1} << bishop_index_bits;

        auto rook_squares = generate_rook_squares(s);
        vector<Bitboard> rook_blockage_bitboards;
        generate_blockage_bitboards(0, Bitboard{0}, rook_squares, rook_blockage_bitboards);
        vector<Bitboard> rook_scan_attacks(rook_blockage_bitboards.size());
        for (size_t i = 0; i < rook_blockage_bitboards.size(); ++i) {
            rook_scan_attacks[i] = scan_rook_attack(s, rook_blockage_bitboards[i]);
        }

        int rook_index_bits = static_cast<int>(rook_squares.size());
        Bitboard rook_mask = generate_rook_bitboard_mask(rook_squares);
        Bitboard rook_magic = find_magic(rook_blockage_bitboards, rook_scan_attacks, rook_mask, rook_index_bits);
        generated_rook_magic_entries[s] = {rook_mask, rook_magic, rook_index_bits, rook_offset};
        for (size_t i = 0; i < rook_blockage_bitboards.size(); ++i) {
            auto key = static_cast<size_t>((rook_blockage_bitboards[i] * rook_magic) >> (64 - rook_index_bits));
            rook_attack_table[rook_offset + key] = rook_scan_attacks[i];
        }
        rook_offset += size_t{1} << rook_index_bits;
    }

        assert(bishop_offset == BISHOP_ATTACK_TABLE_SIZE);
        assert(rook_offset == ROOK_ATTACK_TABLE_SIZE);

        ofstream out("src/magic_tables.cpp");
        if (!out) {
            cerr << "failed to open src/magic_tables.cpp\n";
            return 1;
        }

        out << "#include \"rengine/magic_tables.h\"\n\n";
        out << "namespace rengine {\n\n";
        write_magic_entries(out, "rook_magic_entries", generated_rook_magic_entries);
        write_magic_entries(out, "bishop_magic_entries", generated_bishop_magic_entries);
        write_attack_table(out, "rook_attacks", "ROOK_ATTACK_TABLE_SIZE", rook_attack_table);
        write_attack_table(out, "bishop_attacks", "BISHOP_ATTACK_TABLE_SIZE", bishop_attack_table);
        out << "} // namespace rengine\n";
        return 0;

}