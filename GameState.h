#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>

class GameState {
public:
    virtual ~GameState() {}

    // Returns all legal moves from this position. Moves are ints (board index or encoded).
    virtual std::vector<int> get_moves() const = 0;

    // Returns a heap-allocated new state after applying the move. Caller owns the pointer.
    virtual GameState* apply_move(int move) const = 0;

    // Heuristic (or exact) score. Positive favors player 1, negative favors player -1.
    // Called at terminal nodes or when depth limit is reached.
    virtual int evaluate() const = 0;

    // True when the game is over (win, loss, or draw).
    virtual bool is_terminal() const = 0;

    // Returns 1 or -1 indicating whose turn it is.
    virtual int current_player() const = 0;
};

#endif
