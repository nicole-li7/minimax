#ifndef OTHELLO_H
#define OTHELLO_H

#include "GameState.h"
#include <vector>

// 8x8 board, cells indexed 0-63 (row-major, top-left = 0).
// Player 1 = Black (goes first), Player -1 = White.
// Starting position: Black at (3,4)(4,3), White at (3,3)(4,4).
// move == -1 means "pass" (used when current player has no legal moves).

class Othello : public GameState {
public:
    Othello();
    Othello(const Othello& other);

    std::vector<int> get_moves() const;
    GameState* apply_move(int move) const;
    int evaluate() const;
    bool is_terminal() const;
    int current_player() const;

    int  get_cell(int idx) const;    // 1=Black, -1=White, 0=empty
    int  count(int player) const;
    bool can_place(int idx) const;   // can current player legally place here?

private:
    int board[64];
    int player;  // 1 or -1

    bool is_valid_move(int idx, int pl) const;
    bool has_any_move(int pl) const;
};

#endif
