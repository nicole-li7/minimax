#include "Chess.h"
#include <cstring>

static const int P=1,N=2,B=3,R=4,Q=5,K=6;

static int mk(int fr,int to,int fl=0){return fr|(to<<6)|(fl<<12);}
static int mfr(int m){return m&63;}
static int mto(int m){return(m>>6)&63;}
static int mfl(int m){return(m>>12)&15;}

enum{MN=0,ME=1,MCK=2,MCQ=3,MPQ=4,MPR=5,MPB=6,MPN=7};

static int pcol(int p){return p>0?1:p<0?-1:0;}
static int ptyp(int p){return p<0?-p:p;}

Chess::Chess():player(1),castling(15),ep_sq(-1),last_from(-1),last_to(-1){
    memset(board,0,sizeof(board));
    board[0]=-R;board[1]=-N;board[2]=-B;board[3]=-Q;
    board[4]=-K;board[5]=-B;board[6]=-N;board[7]=-R;
    for(int i=8;i<16;i++)  board[i]=-P;
    for(int i=48;i<56;i++) board[i]= P;
    board[56]=R;board[57]=N;board[58]=B;board[59]=Q;
    board[60]=K;board[61]=B;board[62]=N;board[63]=R;
}

Chess::Chess(const Chess& o):player(o.player),castling(o.castling),
    ep_sq(o.ep_sq),last_from(o.last_from),last_to(o.last_to){
    memcpy(board,o.board,sizeof(board));
}

bool Chess::is_attacked(int sq,int by) const {
    int r=sq/8,c=sq%8;
    // Pawns
    if(by==1){
        if(r+1<8&&c>0  &&board[(r+1)*8+(c-1)]==P) return true;
        if(r+1<8&&c<7  &&board[(r+1)*8+(c+1)]==P) return true;
    } else {
        if(r>0  &&c>0  &&board[(r-1)*8+(c-1)]==-P) return true;
        if(r>0  &&c<7  &&board[(r-1)*8+(c+1)]==-P) return true;
    }
    // Knights
    static const int KR[8]={-2,-2,-1,-1,1,1,2,2};
    static const int KC[8]={-1,1,-2,2,-2,2,-1,1};
    for(int d=0;d<8;d++){
        int nr=r+KR[d],nc=c+KC[d];
        if(nr>=0&&nr<8&&nc>=0&&nc<8&&board[nr*8+nc]==by*N) return true;
    }
    // Diagonals (bishop / queen)
    static const int BR[4]={-1,-1,1,1};
    static const int BC[4]={-1,1,-1,1};
    for(int d=0;d<4;d++){
        int nr=r+BR[d],nc=c+BC[d];
        while(nr>=0&&nr<8&&nc>=0&&nc<8){
            int v=board[nr*8+nc];
            if(v){if(v==by*B||v==by*Q)return true;break;}
            nr+=BR[d];nc+=BC[d];
        }
    }
    // Orthogonals (rook / queen)
    static const int RR[4]={-1,1,0,0};
    static const int RC[4]={0,0,-1,1};
    for(int d=0;d<4;d++){
        int nr=r+RR[d],nc=c+RC[d];
        while(nr>=0&&nr<8&&nc>=0&&nc<8){
            int v=board[nr*8+nc];
            if(v){if(v==by*R||v==by*Q)return true;break;}
            nr+=RR[d];nc+=RC[d];
        }
    }
    // King
    for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++){
        if(!dr&&!dc) continue;
        int nr=r+dr,nc=c+dc;
        if(nr>=0&&nr<8&&nc>=0&&nc<8&&board[nr*8+nc]==by*K) return true;
    }
    return false;
}

void Chess::gen_pseudo(std::vector<int>& out) const {
    for(int sq=0;sq<64;sq++){
        int p=board[sq]; if(pcol(p)!=player) continue;
        int pt=ptyp(p),r=sq/8,c=sq%8;

        if(pt==P){
            int dir=(player==1)?-1:1;
            int sr=(player==1)?6:1;
            int pr=(player==1)?0:7;
            int fwd=sq+dir*8;
            if(fwd>=0&&fwd<64&&!board[fwd]){
                if(fwd/8==pr){
                    out.push_back(mk(sq,fwd,MPQ));
                    out.push_back(mk(sq,fwd,MPN));
                } else {
                    out.push_back(mk(sq,fwd,MN));
                    if(r==sr){int f2=sq+dir*16;if(!board[f2])out.push_back(mk(sq,f2,MN));}
                }
            }
            for(int dc=-1;dc<=1;dc+=2){
                int nc=c+dc; if(nc<0||nc>=8) continue;
                int csq=(r+dir)*8+nc;
                if(csq<0||csq>=64) continue;
                if(pcol(board[csq])==-player){
                    if(csq/8==pr){out.push_back(mk(sq,csq,MPQ));out.push_back(mk(sq,csq,MPN));}
                    else          out.push_back(mk(sq,csq,MN));
                }
                if(csq==ep_sq) out.push_back(mk(sq,csq,ME));
            }
        } else if(pt==N){
            static const int KR[8]={-2,-2,-1,-1,1,1,2,2};
            static const int KC[8]={-1,1,-2,2,-2,2,-1,1};
            for(int d=0;d<8;d++){
                int nr=r+KR[d],nc=c+KC[d];
                if(nr>=0&&nr<8&&nc>=0&&nc<8&&pcol(board[nr*8+nc])!=player)
                    out.push_back(mk(sq,nr*8+nc,MN));
            }
        } else if(pt==B||pt==R||pt==Q){
            // dirs 0-3: diagonals, 4-7: orthogonals
            static const int ADR[8]={-1,-1,1,1,-1,1,0,0};
            static const int ADC[8]={-1,1,-1,1,0,0,-1,1};
            int s=(pt==R)?4:0, e=(pt==B)?4:8;
            for(int d=s;d<e;d++){
                int nr=r+ADR[d],nc=c+ADC[d];
                while(nr>=0&&nr<8&&nc>=0&&nc<8){
                    int vc=pcol(board[nr*8+nc]);
                    if(vc==player) break;
                    out.push_back(mk(sq,nr*8+nc,MN));
                    if(vc==-player) break;
                    nr+=ADR[d]; nc+=ADC[d];
                }
            }
        } else { // King
            for(int dr=-1;dr<=1;dr++) for(int dc=-1;dc<=1;dc++){
                if(!dr&&!dc) continue;
                int nr=r+dr,nc=c+dc;
                if(nr>=0&&nr<8&&nc>=0&&nc<8&&pcol(board[nr*8+nc])!=player)
                    out.push_back(mk(sq,nr*8+nc,MN));
            }
            // Castling
            int opp=-player;
            if(player==1&&sq==60){
                if((castling&1)&&!board[61]&&!board[62]&&board[63]==R&&
                   !is_attacked(60,opp)&&!is_attacked(61,opp)&&!is_attacked(62,opp))
                    out.push_back(mk(60,62,MCK));
                if((castling&2)&&!board[59]&&!board[58]&&!board[57]&&board[56]==R&&
                   !is_attacked(60,opp)&&!is_attacked(59,opp)&&!is_attacked(58,opp))
                    out.push_back(mk(60,58,MCQ));
            } else if(player==-1&&sq==4){
                if((castling&4)&&!board[5]&&!board[6]&&board[7]==-R&&
                   !is_attacked(4,opp)&&!is_attacked(5,opp)&&!is_attacked(6,opp))
                    out.push_back(mk(4,6,MCK));
                if((castling&8)&&!board[3]&&!board[2]&&!board[1]&&board[0]==-R&&
                   !is_attacked(4,opp)&&!is_attacked(3,opp)&&!is_attacked(2,opp))
                    out.push_back(mk(4,2,MCQ));
            }
        }
    }
}

bool Chess::is_legal(int mv) const {
    Chess* n=static_cast<Chess*>(apply_move(mv));
    int ksq=-1;
    for(int i=0;i<64;i++) if(n->board[i]==player*K){ksq=i;break;}
    bool bad=(ksq<0)||n->is_attacked(ksq,-player);
    delete n;
    return !bad;
}

std::vector<int> Chess::get_moves() const {
    std::vector<int> pseudo;
    gen_pseudo(pseudo);
    std::vector<int> legal;
    for(int i=0;i<(int)pseudo.size();i++)
        if(is_legal(pseudo[i])) legal.push_back(pseudo[i]);
    return legal;
}

GameState* Chess::apply_move(int mv) const {
    Chess* n=new Chess(*this);
    int fr=mfr(mv),to=mto(mv),fl=mfl(mv);
    int pc=n->board[fr],pt=ptyp(pc);
    n->ep_sq=-1; n->last_from=fr; n->last_to=to;
    n->board[to]=pc; n->board[fr]=0;

    switch(fl){
    case ME:  n->board[fr/8*8+to%8]=0; break;
    case MCK:
        if(player==1){n->board[63]=0;n->board[61]= R;}
        else         {n->board[7] =0;n->board[5] =-R;}
        break;
    case MCQ:
        if(player==1){n->board[56]=0;n->board[59]= R;}
        else         {n->board[0] =0;n->board[3] =-R;}
        break;
    case MPQ: n->board[to]=player*Q; break;
    case MPR: n->board[to]=player*R; break;
    case MPB: n->board[to]=player*B; break;
    case MPN: n->board[to]=player*N; break;
    }

    // En passant target for double pawn push
    if(pt==P&&(to-fr==16||fr-to==16)) n->ep_sq=(fr+to)/2;

    // Update castling rights
    if(pt==K){ if(player==1) n->castling&=~3; else n->castling&=~12; }
    if(fr==63||to==63) n->castling&=~1;
    if(fr==56||to==56) n->castling&=~2;
    if(fr==7 ||to==7 ) n->castling&=~4;
    if(fr==0 ||to==0 ) n->castling&=~8;

    n->player=-player;
    return n;
}

bool Chess::is_in_check(int pl) const {
    for(int i=0;i<64;i++)
        if(board[i]==pl*K) return is_attacked(i,-pl);
    return false;
}

bool Chess::is_terminal() const { return get_moves().empty(); }
int  Chess::current_player() const { return player; }
int  Chess::get_cell(int i)  const { return board[i]; }
int  Chess::get_last_from()  const { return last_from; }
int  Chess::get_last_to()    const { return last_to; }

int Chess::winner() const {
    if(is_terminal()&&is_in_check(player)) return -player;
    return 0;
}

// ---- Evaluation ----

static const int MAT[7]={0,100,320,330,500,900,20000};

static const int PST_P[64]={
     0, 0, 0, 0, 0, 0, 0, 0,
    50,50,50,50,50,50,50,50,
    10,10,20,30,30,20,10,10,
     5, 5,10,25,25,10, 5, 5,
     0, 0, 0,20,20, 0, 0, 0,
     5,-5,-10, 0, 0,-10,-5, 5,
     5,10,10,-20,-20,10,10, 5,
     0, 0, 0, 0, 0, 0, 0, 0
};
static const int PST_N[64]={
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};
static const int PST_B[64]={
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};
static const int PST_R[64]={
     0, 0, 0, 0, 0, 0, 0, 0,
     5,10,10,10,10,10,10, 5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
    -5, 0, 0, 0, 0, 0, 0,-5,
     0, 0, 0, 5, 5, 0, 0, 0
};
static const int PST_Q[64]={
    -20,-10,-10,-5,-5,-10,-10,-20,
    -10,  0,  0, 0, 0,  0,  0,-10,
    -10,  0,  5, 5, 5,  5,  0,-10,
     -5,  0,  5, 5, 5,  5,  0, -5,
      0,  0,  5, 5, 5,  5,  0, -5,
    -10,  5,  5, 5, 5,  5,  0,-10,
    -10,  0,  5, 0, 0,  0,  0,-10,
    -20,-10,-10,-5,-5,-10,-10,-20
};
static const int PST_K[64]={
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

static const int* PST_TBL[7]={NULL,PST_P,PST_N,PST_B,PST_R,PST_Q,PST_K};

int Chess::evaluate() const {
    std::vector<int> moves=get_moves();
    if(moves.empty())
        return is_in_check(player)?((player==1)?-1000000:1000000):0;
    int score=0;
    for(int i=0;i<64;i++){
        int v=board[i]; if(!v) continue;
        int c=pcol(v),pt=ptyp(v);
        int pi=(c==1)?i:(7-i/8)*8+(i%8);
        score+=c*(MAT[pt]+PST_TBL[pt][pi]);
    }
    return score;
}
