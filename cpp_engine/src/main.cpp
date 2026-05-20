#include <cassert>
#include "rengine/board.h"
#include "rengine/types.h"

int main() {
    rengine::Board board;
    rengine::clear(board);
    assert(rengine::is_empty(board));
}