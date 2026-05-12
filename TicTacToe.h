#ifndef TICTACTOE_H
#define TICTACTOE_H

#include "GameState.h"
#include <vector>

// Board cells are indexed 0-8, left-to-right, top-to-bottom:
//   0 | 1 | 2
//   ---------
//   3 | 4 | 5
//   ---------
//   6 | 7 | 8
//
// Player 1 = X (maximizer), Player -1 = O (minimizer).
// X always goes first.

class TicTacToe : public GameState {
public:
    TicTacToe();
    TicTacToe(const TicTacToe& other);

    std::vector<int> get_moves() const;
    GameState* apply_move(int move) const;
    int evaluate() const;
    bool is_terminal() const;
    int current_player() const;

    void print() const;

    // Returns 1 if X won, -1 if O won, 0 otherwise.
    int winner() const;

    // Returns the value at a board cell: 1=X, -1=O, 0=empty.
    int get_cell(int idx) const;

    // Fills cells[9] with true for each cell that is part of a winning line.
    void get_winning_cells(bool cells[9]) const;

private:
    int board[9];
    int player;  // 1 or -1

    int check_winner() const;
};

#endif
