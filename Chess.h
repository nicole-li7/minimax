#ifndef CHESS_H
#define CHESS_H

#include "GameState.h"
#include <vector>

// 8x8 board, index 0=a8 (black back rank), 63=h1 (white back rank).
// Pieces: P=1 N=2 B=3 R=4 Q=5 K=6; positive=white, negative=black.
// Player 1=White, -1=Black.
// Move encoding: bits 0-5=from, 6-11=to, 12-15=flag.
// Flags: 0=normal 1=en-passant 2=castle-K 3=castle-Q 4=promo-Q 5=promo-R 6=promo-B 7=promo-N

class Chess : public GameState {
public:
    Chess();
    Chess(const Chess& o);

    std::vector<int> get_moves() const;
    GameState* apply_move(int move) const;
    int  evaluate() const;
    bool is_terminal() const;
    int  current_player() const;

    int  get_cell(int idx)   const;  // piece value; positive=white, negative=black
    int  get_last_from()     const;
    int  get_last_to()       const;
    bool is_in_check(int pl) const;
    int  winner()            const;

private:
    int board[64];
    int player;    // 1=white, -1=black
    int castling;  // bits: 0=WK 1=WQ 2=BK 3=BQ
    int ep_sq;     // en-passant target square, -1=none
    int last_from, last_to;

    bool is_attacked(int sq, int by) const;
    void gen_pseudo(std::vector<int>& out) const;
    bool is_legal(int mv) const;
};

#endif
