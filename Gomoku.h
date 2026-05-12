#ifndef GOMOKU_H
#define GOMOKU_H

#include "GameState.h"
#include <vector>

// 15x15 board, cells indexed 0-224 (row-major, top-left = 0).
// Player 1 = Black (goes first), Player -1 = White.
// Win: 5 or more consecutive stones in any direction.
// Move generation only considers cells within 2 of any placed stone,
// keeping the branching factor manageable for alpha-beta.

class Gomoku : public GameState {
public:
    Gomoku();
    Gomoku(const Gomoku& other);

    std::vector<int> get_moves() const;
    GameState* apply_move(int move) const;
    int evaluate() const;
    bool is_terminal() const;
    int current_player() const;

    int  get_cell(int idx) const;       // 1=Black, -1=White, 0=empty
    int  get_last_move() const;         // -1 if no moves played yet
    int  winner() const;
    void get_winning_cells(bool cells[225]) const;

private:
    int board[225];
    int player;
    int last_move;

    int check_winner() const;
};

#endif
