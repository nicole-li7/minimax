#include "Gomoku.h"
#include <cstring>

static const int DR[4] = {0, 1, 1,  1};
static const int DC[4] = {1, 0, 1, -1};

Gomoku::Gomoku() : player(1), last_move(-1) {
    memset(board, 0, sizeof(board));
}

Gomoku::Gomoku(const Gomoku& o) : player(o.player), last_move(o.last_move) {
    memcpy(board, o.board, sizeof(board));
}

int Gomoku::check_winner() const {
    for (int i = 0; i < 225; i++) {
        if (!board[i]) continue;
        int p = board[i], r = i/15, c = i%15;
        for (int d = 0; d < 4; d++) {
            int cnt = 1;
            for (int k = 1; k < 5; k++) {
                int nr = r + k*DR[d], nc = c + k*DC[d];
                if (nr<0||nr>=15||nc<0||nc>=15||board[nr*15+nc]!=p) break;
                cnt++;
            }
            if (cnt >= 5) return p;
        }
    }
    return 0;
}

std::vector<int> Gomoku::get_moves() const {
    if (check_winner()) return std::vector<int>();

    bool seen[225] = {false};
    std::vector<int> moves;
    bool any = false;

    for (int i = 0; i < 225; i++) {
        if (!board[i]) continue;
        any = true;
        int r = i/15, c = i%15;
        for (int dr = -2; dr <= 2; dr++) {
            for (int dc = -2; dc <= 2; dc++) {
                int nr = r+dr, nc = c+dc;
                if (nr<0||nr>=15||nc<0||nc>=15) continue;
                int ni = nr*15+nc;
                if (!board[ni] && !seen[ni]) { seen[ni]=true; moves.push_back(ni); }
            }
        }
    }

    if (!any) moves.push_back(7*15+7);  // opening: center
    return moves;
}

GameState* Gomoku::apply_move(int move) const {
    Gomoku* n = new Gomoku(*this);
    n->board[move] = player;
    n->player    = -player;
    n->last_move = move;
    return n;
}

// Score a window of 5: positive = good for Black (player 1), negative = good for White.
static int window_score(int cp, int co) {
    if (cp && co) return 0;          // window blocked by both — useless
    if (cp == 5) return  1000000;
    if (co == 5) return -1000000;
    if (cp == 4) return  10000;
    if (cp == 3) return    100;
    if (cp == 2) return     10;
    if (cp == 1) return      1;
    if (co == 4) return -10000;
    if (co == 3) return   -100;
    if (co == 2) return    -10;
    if (co == 1) return     -1;
    return 0;
}

int Gomoku::evaluate() const {
    int w = check_winner();
    if (w ==  1) return  1000000;
    if (w == -1) return -1000000;

    int score = 0;

    // Scan every window of 5 in all four directions
    for (int r = 0; r < 15; r++) {          // horizontal
        for (int c = 0; c <= 10; c++) {
            int cp=0, co=0;
            for (int k=0;k<5;k++){int v=board[r*15+c+k];if(v==1)cp++;else if(v==-1)co++;}
            score += window_score(cp,co);
        }
    }
    for (int c = 0; c < 15; c++) {          // vertical
        for (int r = 0; r <= 10; r++) {
            int cp=0, co=0;
            for (int k=0;k<5;k++){int v=board[(r+k)*15+c];if(v==1)cp++;else if(v==-1)co++;}
            score += window_score(cp,co);
        }
    }
    for (int r = 0; r <= 10; r++) {         // diagonal down-right
        for (int c = 0; c <= 10; c++) {
            int cp=0, co=0;
            for (int k=0;k<5;k++){int v=board[(r+k)*15+(c+k)];if(v==1)cp++;else if(v==-1)co++;}
            score += window_score(cp,co);
        }
    }
    for (int r = 0; r <= 10; r++) {         // diagonal down-left
        for (int c = 4; c < 15; c++) {
            int cp=0, co=0;
            for (int k=0;k<5;k++){int v=board[(r+k)*15+(c-k)];if(v==1)cp++;else if(v==-1)co++;}
            score += window_score(cp,co);
        }
    }

    return score;
}

bool Gomoku::is_terminal() const {
    if (check_winner()) return true;
    for (int i = 0; i < 225; i++) if (!board[i]) return false;
    return true;
}

int  Gomoku::current_player() const { return player; }
int  Gomoku::get_cell(int idx)  const { return board[idx]; }
int  Gomoku::get_last_move()    const { return last_move; }
int  Gomoku::winner()           const { return check_winner(); }

void Gomoku::get_winning_cells(bool cells[225]) const {
    memset(cells, 0, 225 * sizeof(bool));
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 15; c++) {
            int p = board[r*15+c]; if (!p) continue;
            for (int d = 0; d < 4; d++) {
                // Check if a 5-window starting here is all same player
                bool ok = true;
                for (int k = 1; k < 5; k++) {
                    int nr=r+k*DR[d], nc=c+k*DC[d];
                    if (nr<0||nr>=15||nc<0||nc>=15||board[nr*15+nc]!=p) { ok=false; break; }
                }
                if (ok) {
                    for (int k = 0; k < 5; k++)
                        cells[(r+k*DR[d])*15 + (c+k*DC[d])] = true;
                }
            }
        }
    }
}
