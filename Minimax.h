#ifndef MINIMAX_H
#define MINIMAX_H

#include "GameState.h"

class Minimax {
public:
    // Returns the best move for the current player using alpha-beta pruning.
    // max_depth: how many plies to search. Use a large value (e.g. 9) to solve completely.
    // Returns -1 if no moves are available.
    static int best_move(const GameState* state, int max_depth);

private:
    static const int POS_INF = 1000000;
    static const int NEG_INF = -1000000;

    static int alphabeta(const GameState* state, int depth, int alpha, int beta, bool maximizing);
};

#endif
