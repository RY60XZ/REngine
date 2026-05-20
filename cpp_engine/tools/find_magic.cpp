#include "rengine/board.h"
#include "rengine/types.h"
#include "rengine/square.h"
#include<vector>
#include<unordered_map>
#include<array>
#include <fstream>
#include <iomanip>
using namespace std;
using namespace rengine;

//performance is not the biggest concern, since the script is for precomputation
//still need to be reasonably fast, however


static Bitboard random_state = 1804289383ULL;

Bitboard random_u64() {
    Bitboard z = (random_state += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;

    return z ^ (z >> 31);
}

constexpr int BISHOP_INDEX_BITS = 11;
constexpr int ROOK_INDEX_BITS = 13;

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

Bitboard generate_bishop_bitboard_mask(Square from, const vector<Square>& bishop_squares) {
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

Bitboard generate_rook_bitboard_mask(Square from, const vector<Square>& rook_squares) {
    auto rook_bitboard = Bitboard{0};
    for (auto s : rook_squares) {
        rook_bitboard |= Bitboard{1} << s;
    }
    return rook_bitboard;
}

void generate_blockage_bitboards(int i, Bitboard curr, const vector<Square>& possible_squares, vector<Bitboard>& result) {
    if (i==possible_squares.size()) {
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

Bitboard find_bishop_magic(Square from) {
    auto bishop_squares = generate_bishop_squares(from);
    vector<Bitboard> blockage_bitboards;
    generate_blockage_bitboards(0, Bitboard{0}, bishop_squares, blockage_bitboards);
    vector<Bitboard> attacks(blockage_bitboards.size());
    for (int i = 0; i < blockage_bitboards.size(); ++i) {
        attacks[i] = scan_bishop_attack(from, blockage_bitboards[i]);
    }
    unordered_map<Bitboard, Bitboard> hash; //first: hash  second: attack bitboard
    Bitboard magic;
    while (true) {
        bool flag = true;
        magic = random_u64() & random_u64() & random_u64();
        for (int i = 0; i < blockage_bitboards.size(); ++i) {
            Bitboard key = (blockage_bitboards[i] * magic) >> (64 - BISHOP_INDEX_BITS);
            if (hash.contains(key)) {
                if (hash[key] != attacks[i]) {
                    flag = false;
                    break;
                }
            }
            else {
                hash[key] = attacks[i];
            }

        }
        hash.clear();
        if (flag) break;
    }
    return magic;
}

Bitboard find_rook_magic(Square from) {
    auto rook_squares = generate_rook_squares(from);
    vector<Bitboard> blockage_bitboards;
    generate_blockage_bitboards(0, Bitboard{0}, rook_squares, blockage_bitboards);
    vector<Bitboard> attacks(blockage_bitboards.size());
    for (int i = 0; i < blockage_bitboards.size(); ++i) {
        attacks[i] = scan_rook_attack(from, blockage_bitboards[i]);
    }
    unordered_map<Bitboard, Bitboard> hash; //first: hash  second: attack bitboard
    Bitboard magic;
    while (true) {
        bool flag = true;
        magic = random_u64() & random_u64() & random_u64();
        for (int i = 0; i < blockage_bitboards.size(); ++i) {
            Bitboard key = (blockage_bitboards[i] * magic) >> (64 - ROOK_INDEX_BITS);
            if (hash.contains(key)) {
                if (hash[key] != attacks[i]) {
                    flag = false;
                    break;
                }
            }
            else {
                hash[key] = attacks[i];
            }

        }
        hash.clear();
        if (flag) break;
    }
    return magic;
}

void write_bitboard(ostream& out, Bitboard b) {
    out << "0x"
        << hex << setw(16) << setfill('0') << b
        << "ULL" << dec;
}

array<Bitboard, 64> rook_masks;
array<Bitboard, 64> bishop_masks;
array<Bitboard, 64> rook_magic;
array<Bitboard, 64> bishop_magic;
array<array<Bitboard, 1<<BISHOP_INDEX_BITS>, 64> bishop_attacks{};
array<array<Bitboard, 1<<ROOK_INDEX_BITS>, 64> rook_attacks{};

int main() {
    for (Square s = 0; s < 64; ++s) {
        auto bishop_squares = generate_bishop_squares(s);
        vector<Bitboard> bishop_blockage_bitboards;
        generate_blockage_bitboards(0, Bitboard{0}, bishop_squares, bishop_blockage_bitboards);
        bishop_masks[s] = generate_bishop_bitboard_mask(s, bishop_squares);
        bishop_magic[s] = find_bishop_magic(s);
        for (auto b : bishop_blockage_bitboards) {
            bishop_attacks[s][(b * bishop_magic[s]) >> (64 - BISHOP_INDEX_BITS)] = scan_bishop_attack(s, b);
        }
        auto rook_squares = generate_rook_squares(s);
        vector<Bitboard> rook_blockage_bitboards;
        generate_blockage_bitboards(0, Bitboard{0}, rook_squares, rook_blockage_bitboards);
        rook_masks[s] = generate_rook_bitboard_mask(s, rook_squares);
        rook_magic[s] = find_rook_magic(s);
        for (auto b : rook_blockage_bitboards) {
            rook_attacks[s][(b * rook_magic[s]) >> (64 - ROOK_INDEX_BITS)] = scan_rook_attack(s, b);
        }
    }
    ofstream out("src/magic_tables.cpp");
    out << "#include \"rengine/magic_tables.h\"\n\n";
    out << "namespace rengine {\n\n";

    out << "const std::array<Bitboard, 64> rook_masks = {{\n";
    for (Bitboard b : rook_masks) {
        out << "    ";
        write_bitboard(out, b);
        out << ",\n";
    }
    out << "}};\n\n";

    out << "const std::array<Bitboard, 64> bishop_masks = {{\n";
    for (Bitboard b : bishop_masks) {
        out << "    ";
        write_bitboard(out, b);
        out << ",\n";
    }
    out << "}};\n\n";

    out << "const std::array<Bitboard, 64> rook_magic = {{\n";
    for (Bitboard b : rook_magic) {
        out << "    ";
        write_bitboard(out, b);
        out << ",\n";
    }
    out << "}};\n\n";

    out << "const std::array<Bitboard, 64> bishop_magic = {{\n";
    for (Bitboard b : bishop_magic) {
        out << "    ";
        write_bitboard(out, b);
        out << ",\n";
    }
    out << "}};\n\n";

    out << "const std::array<std::array<Bitboard, 1 << ROOK_INDEX_BITS>, 64> rook_attacks = {{\n";
    for (const auto& row : rook_attacks) {
        out << "    {{\n";
        for (Bitboard b : row) {
            out << "        ";
            write_bitboard(out, b);
            out << ",\n";
        }
        out << "    }},\n";
    }
    out << "}};\n\n";

    out << "const std::array<std::array<Bitboard, 1 << BISHOP_INDEX_BITS>, 64> bishop_attacks = {{\n";
    for (const auto& row : bishop_attacks) {
        out << "    {{\n";
        for (Bitboard b : row) {
            out << "        ";
            write_bitboard(out, b);
            out << ",\n";
        }
        out << "    }},\n";
    }
    out << "}};\n\n";

    out << "} // namespace rengine\n";
    return 0;
}
