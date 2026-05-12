#include "Othello.h"
#include <cstring>

static const int DR[8] = {-1,-1,-1, 0, 0, 1, 1, 1};
static const int DC[8] = {-1, 0, 1,-1, 1,-1, 0, 1};

// Classic positional weight table: corners are very valuable,
// cells adjacent to unowned corners are dangerous.
static const int WEIGHTS[64] = {
    120,-20, 20,  5,  5, 20,-20,120,
    -20,-40, -5, -5, -5, -5,-40,-20,
     20, -5, 15,  3,  3, 15, -5, 20,
      5, -5,  3,  3,  3,  3, -5,  5,
      5, -5,  3,  3,  3,  3, -5,  5,
     20, -5, 15,  3,  3, 15, -5, 20,
    -20,-40, -5, -5, -5, -5,-40,-20,
    120,-20, 20,  5,  5, 20,-20,120
};

Othello::Othello() : player(1) {
    memset(board, 0, sizeof(board));
    board[27] = -1;  // (3,3) = White
    board[28] =  1;  // (3,4) = Black
    board[35] =  1;  // (4,3) = Black
    board[36] = -1;  // (4,4) = White
}

Othello::Othello(const Othello& o) : player(o.player) {
    memcpy(board, o.board, sizeof(board));
}

bool Othello::is_valid_move(int idx, int pl) const {
    if (board[idx] != 0) return false;
    int r0 = idx / 8, c0 = idx % 8;
    for (int d = 0; d < 8; d++) {
        int r = r0 + DR[d], c = c0 + DC[d];
        bool found_opp = false;
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            int v = board[r*8 + c];
            if      (v == -pl) { found_opp = true; }
            else if (v ==  pl) { if (found_opp) return true; break; }
            else               { break; }
            r += DR[d]; c += DC[d];
        }
    }
    return false;
}

bool Othello::has_any_move(int pl) const {
    for (int i = 0; i < 64; i++)
        if (board[i] == 0 && is_valid_move(i, pl))
            return true;
    return false;
}

std::vector<int> Othello::get_moves() const {
    std::vector<int> m;
    for (int i = 0; i < 64; i++)
        if (is_valid_move(i, player))
            m.push_back(i);
    // If no legal placements, pass (only if opponent can still move)
    if (m.empty() && has_any_move(-player))
        m.push_back(-1);
    return m;
}

GameState* Othello::apply_move(int move) const {
    Othello* n = new Othello(*this);
    if (move == -1) {
        n->player = -player;
        return n;
    }
    n->board[move] = player;
    int r0 = move / 8, c0 = move % 8;
    for (int d = 0; d < 8; d++) {
        int r = r0 + DR[d], c = c0 + DC[d];
        int fl[8], nf = 0;
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            int v = n->board[r*8 + c];
            if      (v == -player) { fl[nf++] = r*8 + c; }
            else if (v ==  player) { for (int i = 0; i < nf; i++) n->board[fl[i]] = player; break; }
            else                   { break; }
            r += DR[d]; c += DC[d];
        }
    }
    n->player = -player;
    return n;
}

bool Othello::is_terminal() const {
    return !has_any_move(1) && !has_any_move(-1);
}

int Othello::evaluate() const {
    if (is_terminal()) {
        int b = count(1), w = count(-1);
        return (b > w) ? 10000 : (w > b) ? -10000 : 0;
    }
    int score = 0;
    for (int i = 0; i < 64; i++)
        score += board[i] * WEIGHTS[i];
    return score;
}

int  Othello::current_player() const { return player; }
int  Othello::get_cell(int idx) const { return board[idx]; }
bool Othello::can_place(int idx) const { return is_valid_move(idx, player); }

int Othello::count(int pl) const {
    int n = 0;
    for (int i = 0; i < 64; i++) if (board[i] == pl) n++;
    return n;
}
