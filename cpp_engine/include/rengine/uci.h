#ifndef CPP_ENGINE_UCI_H
#define CPP_ENGINE_UCI_H

#include"rengine/board.h"
#include<string>
#include<iosfwd>

namespace rengine {
    bool make_uci_move(Board& board, const std::string& move_text);
    int run_uci_loop(std::istream& input, std::ostream& output);
}

#endif //CPP_ENGINE_UCI_H
