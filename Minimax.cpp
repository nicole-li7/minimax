#include "Minimax.h"
#include <vector>
#include <algorithm>

// Out-of-class definitions required for static const members in C++03
const int Minimax::POS_INF;
const int Minimax::NEG_INF;

struct ScoredMove {
    int score;
    int move;
};

static bool cmp_desc(const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; }
static bool cmp_asc (const ScoredMove& a, const ScoredMove& b) { return a.score < b.score; }

// Score each move with a depth-0 evaluation and return moves sorted best-first.
static std::vector<int> order_moves(const GameState* state,
                                    const std::vector<int>& moves,
                                    bool maximizing)
{
    std::vector<ScoredMove> scored;
    for (int i = 0; i < (int)moves.size(); i++) {
        GameState* next = state->apply_move(moves[i]);
        ScoredMove sm = { next->evaluate(), moves[i] };
        delete next;
        scored.push_back(sm);
    }
    if (maximizing)
        std::sort(scored.begin(), scored.end(), cmp_desc);
    else
        std::sort(scored.begin(), scored.end(), cmp_asc);

    std::vector<int> ordered;
    for (int i = 0; i < (int)scored.size(); i++)
        ordered.push_back(scored[i].move);
    return ordered;
}

int Minimax::alphabeta(const GameState* state, int depth, int alpha, int beta, bool maximizing) {
    if (state->is_terminal() || depth == 0)
        return state->evaluate();

    std::vector<int> moves = order_moves(state, state->get_moves(), maximizing);

    if (maximizing) {
        int best = NEG_INF;
        for (int i = 0; i < (int)moves.size(); i++) {
            GameState* next = state->apply_move(moves[i]);
            int score = alphabeta(next, depth - 1, alpha, beta, false);
            delete next;
            if (score > best) best = score;
            if (score > alpha) alpha = score;
            if (beta <= alpha) break;
        }
        return best;
    } else {
        int best = POS_INF;
        for (int i = 0; i < (int)moves.size(); i++) {
            GameState* next = state->apply_move(moves[i]);
            int score = alphabeta(next, depth - 1, alpha, beta, true);
            delete next;
            if (score < best) best = score;
            if (score < beta) beta = score;
            if (beta <= alpha) break;
        }
        return best;
    }
}

int Minimax::best_move(const GameState* state, int max_depth) {
    std::vector<int> moves = state->get_moves();
    if (moves.empty()) return -1;

    bool maximizing = (state->current_player() == 1);
    std::vector<int> ordered = order_moves(state, moves, maximizing);
    int best_score = maximizing ? NEG_INF : POS_INF;
    int best = ordered[0];

    for (int i = 0; i < (int)ordered.size(); i++) {
        GameState* next = state->apply_move(ordered[i]);
        int score = alphabeta(next, max_depth - 1, NEG_INF, POS_INF, !maximizing);
        delete next;

        if (maximizing && score > best_score) { best_score = score; best = ordered[i]; }
        else if (!maximizing && score < best_score) { best_score = score; best = ordered[i]; }
    }

    return best;
}
