#include "TicTacToe.h"
#include <iostream>
#include <cstring>

static const int WINNING_LINES[8][3] = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},  // rows
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},  // cols
    {0, 4, 8}, {2, 4, 6}               // diagonals
};

TicTacToe::TicTacToe() : player(1) {
    memset(board, 0, sizeof(board));
}

TicTacToe::TicTacToe(const TicTacToe& other) : player(other.player) {
    memcpy(board, other.board, sizeof(board));
}

int TicTacToe::check_winner() const {
    for (int i = 0; i < 8; i++) {
        int a = WINNING_LINES[i][0];
        int b = WINNING_LINES[i][1];
        int c = WINNING_LINES[i][2];
        if (board[a] != 0 && board[a] == board[b] && board[b] == board[c]) {
            return board[a];
        }
    }
    return 0;
}

std::vector<int> TicTacToe::get_moves() const {
    std::vector<int> moves;
    if (check_winner() != 0) return moves;  // game already over
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) moves.push_back(i);
    }
    return moves;
}

GameState* TicTacToe::apply_move(int move) const {
    TicTacToe* next = new TicTacToe(*this);
    next->board[move] = player;
    next->player = -player;
    return next;
}

int TicTacToe::evaluate() const {
    int w = check_winner();
    if (w == 1)  return 100;
    if (w == -1) return -100;
    return 0;
}

bool TicTacToe::is_terminal() const {
    if (check_winner() != 0) return true;
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) return false;
    }
    return true;  // board full, draw
}

int TicTacToe::current_player() const {
    return player;
}

int TicTacToe::winner() const {
    return check_winner();
}

int TicTacToe::get_cell(int idx) const {
    return board[idx];
}

void TicTacToe::get_winning_cells(bool cells[9]) const {
    memset(cells, 0, 9 * sizeof(bool));
    for (int i = 0; i < 8; i++) {
        int a = WINNING_LINES[i][0];
        int b = WINNING_LINES[i][1];
        int c = WINNING_LINES[i][2];
        if (board[a] != 0 && board[a] == board[b] && board[b] == board[c]) {
            cells[a] = cells[b] = cells[c] = true;
        }
    }
}

void TicTacToe::print() const {
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int cell = board[row * 3 + col];
            if      (cell ==  1) std::cout << " X ";
            else if (cell == -1) std::cout << " O ";
            else                 std::cout << " " << (row * 3 + col + 1) << " ";

            if (col < 2) std::cout << "|";
        }
        std::cout << "\n";
        if (row < 2) std::cout << "---|---|---\n";
    }
    std::cout << "\n";
}
