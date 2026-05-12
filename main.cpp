#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "msimg32.lib")

#include <windows.h>
#include <cstring>
#include <cstdio>
#include "TicTacToe.h"
#include "Othello.h"
#include "Gomoku.h"
#include "Chess.h"
#include "Minimax.h"

#define WM_AI_MOVE (WM_USER + 1)

// ---- window ----
static const int WIN_W = 500, WIN_H = 580;

// TicTacToe board (3x3, 120px cells)
static const int TTT_BX = 70, TTT_BY = 72, TTT_CS = 120;

// Othello board (8x8, 54px cells; 8*54=432, (500-432)/2=34)
static const int OTH_BX = 34, OTH_BY = 70, OTH_CS = 54;

// Gomoku board (15x15 intersections, 30px spacing; 14*30=420, (500-420)/2=40)
static const int GOM_BX = 40, GOM_BY = 55, GOM_CS = 30;

// Chess board (8x8, 55px cells; 8*55=440, (500-440)/2=30)
static const int CHE_BX = 30, CHE_BY = 58, CHE_CS = 55;

// ---- colors ----
static const COLORREF C_BG     = RGB(22,  22,  35);
static const COLORREF C_PANEL  = RGB(30,  30,  48);
static const COLORREF C_GRID   = RGB(60,  63,  84);
static const COLORREF C_HOVER  = RGB(40,  41,  58);
static const COLORREF C_X      = RGB(137, 180, 250);
static const COLORREF C_O      = RGB(243, 139, 168);
static const COLORREF C_WIN    = RGB(166, 227, 161);
static const COLORREF C_WIN_BG = RGB(35,  52,  40);
static const COLORREF C_TEXT   = RGB(205, 214, 244);
static const COLORREF C_DIM    = RGB(108, 112, 134);
static const COLORREF C_BTN    = RGB(52,  55,  76);
static const COLORREF C_BTNHOV = RGB(76,  80, 106);
static const COLORREF C_TITLE  = RGB(180, 190, 254);

static const COLORREF C_OB_BG    = RGB(25, 105, 25);
static const COLORREF C_OB_GRID  = RGB(10,  82, 10);
static const COLORREF C_OB_BLACK = RGB(20,  20,  28);
static const COLORREF C_OB_WHITE = RGB(228, 224, 210);
static const COLORREF C_OB_HINT  = RGB(55, 145, 55);
static const COLORREF C_OB_HOV   = RGB(45, 130, 45);

static const COLORREF C_CH_LIGHT  = RGB(240, 217, 181);
static const COLORREF C_CH_DARK   = RGB(181, 136,  99);
static const COLORREF C_CH_SEL_L  = RGB(246, 246, 130);
static const COLORREF C_CH_SEL_D  = RGB(186, 202,  68);
static const COLORREF C_CH_LAST_L = RGB(205, 210, 106);
static const COLORREF C_CH_LAST_D = RGB(170, 162,  58);
static const COLORREF C_CH_CHK_L  = RGB(255, 110, 110);
static const COLORREF C_CH_CHK_D  = RGB(200,  55,  55);
static const COLORREF C_CH_WP     = RGB(255, 255, 255);
static const COLORREF C_CH_BP     = RGB(25,  20,  20);
static const COLORREF C_CH_DOT    = RGB(90,  80,  70);

static const COLORREF C_GM_BG    = RGB(60,  46,  28);
static const COLORREF C_GM_GRID  = RGB(95,  73,  46);
static const COLORREF C_GM_STAR  = RGB(80,  60,  36);
static const COLORREF C_GM_BLACK = RGB(22,  22,  28);
static const COLORREF C_GM_WHITE = RGB(232, 228, 215);
static const COLORREF C_GM_WBDR  = RGB(150, 145, 132);

// ---- app state ----
enum AppState { MENU, CHOOSE_SIDE, PLAYING, GAME_OVER };
enum GameType { GAME_TTT, GAME_OTH, GAME_GOM, GAME_CHE };

struct App {
    AppState state;
    GameType game;
    TicTacToe ttt;
    Othello   oth;
    Gomoku    gom;
    Chess     che;
    int  human;       // 1=first/X/Black, -1=second/O/White
    int  hover_cell;  // -1=none
    int  hover_btn;   // context-dependent; 0=none
    int  result;      // 1, -1, or 0
    bool undo_taunt;
    int  chess_sel;              // selected square, -1=none
    std::vector<int> chess_lgl;  // legal moves from chess_sel
};

static App  g_app;
static HWND g_hwnd;
static HFONT g_fnt_title, g_fnt_status, g_fnt_btn, g_fnt_label, g_fnt_score, g_fnt_big, g_fnt_piece;

// ---- button rects ----
// Menu: 4 buttons, width=114, gap=8, margin=10
static const RECT R_M_TTT  = {10,  158, 124, 368};
static const RECT R_M_OTH  = {132, 158, 246, 368};
static const RECT R_M_GOM  = {254, 158, 368, 368};
static const RECT R_M_CHE  = {376, 158, 490, 368};

// Choose side
static const RECT R_CS_1   = {145, 215, 235, 305};
static const RECT R_CS_2   = {265, 215, 355, 305};

// Play again (per game, positioned below each board)
static const RECT R_AG_TTT = {170, 458, 330, 498};
static const RECT R_AG_OTH = {170, 518, 330, 558};
static const RECT R_AG_GOM = {170, 500, 330, 540};
static const RECT R_AG_CHE = {170, 518, 330, 558};

// Undo button (bottom-left, visible during PLAYING)
static const RECT R_UNDO = {14, 543, 134, 571};

// ---- helpers ----

static bool pt_in(RECT r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}
static RECT get_again_rect() {
    if (g_app.game == GAME_TTT) return R_AG_TTT;
    if (g_app.game == GAME_OTH) return R_AG_OTH;
    if (g_app.game == GAME_GOM) return R_AG_GOM;
    return R_AG_CHE;
}

static void fill_rc(HDC dc, RECT r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c); FillRect(dc, &r, b); DeleteObject(b);
}

static void rrect(HDC dc, int x1, int y1, int x2, int y2, int rx,
                  COLORREF fill, COLORREF bdr, int bw) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = bw>0 ? CreatePen(PS_SOLID,bw,bdr) : (HPEN)GetStockObject(NULL_PEN);
    HBRUSH ob = (HBRUSH)SelectObject(dc,br); HPEN op = (HPEN)SelectObject(dc,pn);
    RoundRect(dc, x1,y1,x2,y2, rx,rx);
    SelectObject(dc,ob); SelectObject(dc,op);
    DeleteObject(br); if(bw>0) DeleteObject(pn);
}

static void disc(HDC dc, int cx, int cy, int r, COLORREF fill) {
    HBRUSH br = CreateSolidBrush(fill);
    HPEN   pn = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH ob = (HBRUSH)SelectObject(dc,br); HPEN op = (HPEN)SelectObject(dc,pn);
    Ellipse(dc, cx-r,cy-r, cx+r,cy+r);
    SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(br);
}

// Board piece drawing (used on TTT game board, choose-side preview)
static void draw_x(HDC dc, RECT r, COLORREF col) {
    int pad=24;
    HPEN pn=CreatePen(PS_SOLID,10,col), op=(HPEN)SelectObject(dc,pn);
    MoveToEx(dc,r.left+pad,r.top+pad,NULL);    LineTo(dc,r.right-pad,r.bottom-pad);
    MoveToEx(dc,r.right-pad,r.top+pad,NULL);   LineTo(dc,r.left+pad,r.bottom-pad);
    SelectObject(dc,op); DeleteObject(pn);
}
static void draw_o(HDC dc, RECT r, COLORREF col) {
    int pad=16;
    HPEN pn=CreatePen(PS_SOLID,10,col); HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN op=(HPEN)SelectObject(dc,pn); HBRUSH ob=(HBRUSH)SelectObject(dc,nb);
    Ellipse(dc, r.left+pad,r.top+pad,r.right-pad,r.bottom-pad);
    SelectObject(dc,op); SelectObject(dc,ob); DeleteObject(pn);
}

// Small icons used in menu buttons
static void icon_x(HDC dc, int cx, int cy, COLORREF col) {
    HPEN pn=CreatePen(PS_SOLID,5,col), op=(HPEN)SelectObject(dc,pn);
    MoveToEx(dc,cx-17,cy-17,NULL); LineTo(dc,cx+17,cy+17);
    MoveToEx(dc,cx+17,cy-17,NULL); LineTo(dc,cx-17,cy+17);
    SelectObject(dc,op); DeleteObject(pn);
}
static void icon_o(HDC dc, int cx, int cy, COLORREF col) {
    HPEN pn=CreatePen(PS_SOLID,5,col); HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN op=(HPEN)SelectObject(dc,pn); HBRUSH ob=(HBRUSH)SelectObject(dc,nb);
    Ellipse(dc,cx-17,cy-17,cx+17,cy+17);
    SelectObject(dc,op); SelectObject(dc,ob); DeleteObject(pn);
}
static void icon_gom(HDC dc, int cx, int cy, bool hover) {
    // Mini 3x3 Gomoku board with a stone at center
    int hs=22;
    COLORREF bc = hover ? C_GM_BG  : RGB(38,29,18);
    COLORREF gc = hover ? C_GM_GRID : RGB(65,50,32);
    COLORREF sc = hover ? C_GM_BLACK : RGB(55,55,68);
    HBRUSH br=CreateSolidBrush(bc); RECT r={cx-hs,cy-hs,cx+hs,cy+hs};
    FillRect(dc,&r,br); DeleteObject(br);
    HPEN gp=CreatePen(PS_SOLID,1,gc), og=(HPEN)SelectObject(dc,gp);
    for (int i=-1;i<=1;i++) {
        MoveToEx(dc,cx+i*11,cy-hs+3,NULL); LineTo(dc,cx+i*11,cy+hs-3);
        MoveToEx(dc,cx-hs+3,cy+i*11,NULL); LineTo(dc,cx+hs-3,cy+i*11);
    }
    SelectObject(dc,og); DeleteObject(gp);
    disc(dc,cx,cy,9,sc);
}

static void text_cx(HDC dc, const char* s, int cx, int y, HFONT f, COLORREF col) {
    HFONT of=(HFONT)SelectObject(dc,f);
    SetTextColor(dc,col); SetBkMode(dc,TRANSPARENT);
    SIZE sz; GetTextExtentPoint32A(dc,s,(int)strlen(s),&sz);
    TextOutA(dc,cx-sz.cx/2,y,s,(int)strlen(s));
    SelectObject(dc,of);
}

// ---- board coordinate helpers ----

static RECT ttt_cell_rect(int i) {
    RECT r={TTT_BX+(i%3)*TTT_CS, TTT_BY+(i/3)*TTT_CS,0,0};
    r.right=r.left+TTT_CS; r.bottom=r.top+TTT_CS; return r;
}
static int ttt_from_pt(int x, int y) {
    int bx=x-TTT_BX,by=y-TTT_BY;
    if(bx<0||by<0||bx>=TTT_CS*3||by>=TTT_CS*3) return -1;
    return (by/TTT_CS)*3+(bx/TTT_CS);
}

static RECT oth_cell_rect(int i) {
    RECT r={OTH_BX+(i%8)*OTH_CS, OTH_BY+(i/8)*OTH_CS,0,0};
    r.right=r.left+OTH_CS; r.bottom=r.top+OTH_CS; return r;
}
static int oth_from_pt(int x, int y) {
    int bx=x-OTH_BX,by=y-OTH_BY;
    if(bx<0||by<0||bx>=OTH_CS*8||by>=OTH_CS*8) return -1;
    return (by/OTH_CS)*8+(bx/OTH_CS);
}

// Gomoku: snap click to nearest intersection within half-cell
static int gom_from_pt(int x, int y) {
    int margin = GOM_CS/2;
    if (x < GOM_BX-margin || x > GOM_BX+14*GOM_CS+margin ||
        y < GOM_BY-margin || y > GOM_BY+14*GOM_CS+margin) return -1;
    int col = (x - GOM_BX + margin) / GOM_CS;
    int row = (y - GOM_BY + margin) / GOM_CS;
    if (col<0) col=0; if (col>14) col=14;
    if (row<0) row=0; if (row>14) row=14;
    return row*15+col;
}

// ---- paint functions ----

static void paint_menu(HDC dc) {
    text_cx(dc,"MINIMAX GAMES",WIN_W/2, 55, g_fnt_title,  C_TITLE);
    text_cx(dc,"Choose a game",WIN_W/2, 118, g_fnt_status, C_DIM);

    // Tic Tac Toe
    bool th=(g_app.hover_btn==1);
    rrect(dc,R_M_TTT.left,R_M_TTT.top,R_M_TTT.right,R_M_TTT.bottom,16,th?C_BTNHOV:C_BTN,C_GRID,2);
    { int cx=(R_M_TTT.left+R_M_TTT.right)/2, my=(R_M_TTT.top+R_M_TTT.bottom)/2-22;
      icon_x(dc,cx-25,my,th?C_X:C_DIM); icon_o(dc,cx+25,my,th?C_O:C_DIM); }
    text_cx(dc,"Tic Tac Toe",(R_M_TTT.left+R_M_TTT.right)/2,R_M_TTT.bottom-44,g_fnt_btn,th?C_TEXT:C_DIM);

    // Othello
    bool oh=(g_app.hover_btn==2);
    rrect(dc,R_M_OTH.left,R_M_OTH.top,R_M_OTH.right,R_M_OTH.bottom,16,oh?C_BTNHOV:C_BTN,C_GRID,2);
    { int cx=(R_M_OTH.left+R_M_OTH.right)/2, my=(R_M_OTH.top+R_M_OTH.bottom)/2-22;
      disc(dc,cx-22,my,20,oh?C_OB_BLACK:RGB(55,55,68));
      disc(dc,cx+22,my,20,oh?C_OB_WHITE:RGB(130,126,118)); }
    text_cx(dc,"Othello",(R_M_OTH.left+R_M_OTH.right)/2,R_M_OTH.bottom-44,g_fnt_btn,oh?C_TEXT:C_DIM);

    // Gomoku
    bool gh=(g_app.hover_btn==3);
    rrect(dc,R_M_GOM.left,R_M_GOM.top,R_M_GOM.right,R_M_GOM.bottom,16,gh?C_BTNHOV:C_BTN,C_GRID,2);
    { int cx=(R_M_GOM.left+R_M_GOM.right)/2, my=(R_M_GOM.top+R_M_GOM.bottom)/2-22;
      icon_gom(dc,cx,my,gh); }
    text_cx(dc,"Gomoku",(R_M_GOM.left+R_M_GOM.right)/2,R_M_GOM.bottom-44,g_fnt_btn,gh?C_TEXT:C_DIM);

    // Chess
    bool ch=(g_app.hover_btn==4);
    rrect(dc,R_M_CHE.left,R_M_CHE.top,R_M_CHE.right,R_M_CHE.bottom,16,ch?C_BTNHOV:C_BTN,C_GRID,2);
    {   // Mini 4x4 chess board icon
        int cx=(R_M_CHE.left+R_M_CHE.right)/2, my=(R_M_CHE.top+R_M_CHE.bottom)/2-22;
        int ts=11;
        for(int row=0;row<4;row++) for(int col=0;col<4;col++){
            bool light=((row+col)%2==0);
            COLORREF sqc=ch?(light?RGB(210,190,150):RGB(150,110,75)):(light?RGB(130,120,100):RGB(90,70,50));
            RECT sr={cx-ts*2+col*ts,my-ts*2+row*ts,cx-ts*2+col*ts+ts,my-ts*2+row*ts+ts};
            fill_rc(dc,sr,sqc);
        }
        // Draw a white king symbol in the center using g_fnt_piece
        HFONT of=(HFONT)SelectObject(dc,g_fnt_piece);
        SetTextColor(dc,ch?RGB(255,255,240):RGB(160,150,130));
        SetBkMode(dc,TRANSPARENT);
        wchar_t wch[2]={0x2654,0}; // white king unicode
        SIZE sz; GetTextExtentPoint32W(dc,wch,1,&sz);
        TextOutW(dc,cx-sz.cx/2,my-ts*2-sz.cy/2+9,wch,1);
        SelectObject(dc,of);
    }
    text_cx(dc,"Chess",(R_M_CHE.left+R_M_CHE.right)/2,R_M_CHE.bottom-44,g_fnt_btn,ch?C_TEXT:C_DIM);
}

static void paint_choose_side(HDC dc) {
    bool ttt = (g_app.game == GAME_TTT);
    text_cx(dc, ttt?"Tic Tac Toe":g_app.game==GAME_OTH?"Othello":"Gomoku",
            WIN_W/2, 80, g_fnt_title, C_TITLE);
    text_cx(dc,"Choose your side", WIN_W/2, 150, g_fnt_status, C_DIM);

    bool h1=(g_app.hover_btn==1), h2=(g_app.hover_btn==2);
    rrect(dc,R_CS_1.left,R_CS_1.top,R_CS_1.right,R_CS_1.bottom,14,h1?C_BTNHOV:C_BTN,C_GRID,2);
    rrect(dc,R_CS_2.left,R_CS_2.top,R_CS_2.right,R_CS_2.bottom,14,h2?C_BTNHOV:C_BTN,C_GRID,2);

    if (ttt) {
        RECT r1=R_CS_1, r2=R_CS_2;
        draw_x(dc,r1,h1?C_X:C_DIM); draw_o(dc,r2,h2?C_O:C_DIM);
        text_cx(dc,"Go First",  (R_CS_1.left+R_CS_1.right)/2,R_CS_1.bottom+10,g_fnt_label,C_DIM);
        text_cx(dc,"Go Second", (R_CS_2.left+R_CS_2.right)/2,R_CS_2.bottom+10,g_fnt_label,C_DIM);
    } else {
        int cx1=(R_CS_1.left+R_CS_1.right)/2, cy1=(R_CS_1.top+R_CS_1.bottom)/2;
        int cx2=(R_CS_2.left+R_CS_2.right)/2, cy2=(R_CS_2.top+R_CS_2.bottom)/2;
        disc(dc,cx1,cy1,30,h1?C_GM_BLACK:RGB(55,55,68));
        disc(dc,cx2,cy2,30,h2?C_GM_WHITE:RGB(130,126,118));
        text_cx(dc,"Black (First)",  cx1,R_CS_1.bottom+10,g_fnt_label,C_DIM);
        text_cx(dc,"White (Second)", cx2,R_CS_2.bottom+10,g_fnt_label,C_DIM);
    }
}

static void paint_ttt(HDC dc) {
    const char* st;
    if (g_app.state==PLAYING)
        st=(g_app.ttt.current_player()==g_app.human)?"Your turn":"AI thinking...";
    else
        st=(g_app.result==g_app.human)?"You win!":(g_app.result==0)?"Draw!":"AI wins!";
    text_cx(dc,st,WIN_W/2,26,g_fnt_status,C_TEXT);

    rrect(dc,TTT_BX-10,TTT_BY-10,TTT_BX+TTT_CS*3+10,TTT_BY+TTT_CS*3+10,14,C_PANEL,C_GRID,1);

    bool wc[9]={false}; g_app.ttt.get_winning_cells(wc);
    for (int i=0;i<9;i++) {
        RECT r=ttt_cell_rect(i);
        RECT ri={r.left+3,r.top+3,r.right-3,r.bottom-3};
        if (wc[i])
            rrect(dc,ri.left,ri.top,ri.right,ri.bottom,10,C_WIN_BG,C_BG,0);
        else if (g_app.state==PLAYING&&i==g_app.hover_cell&&g_app.ttt.get_cell(i)==0)
            rrect(dc,ri.left,ri.top,ri.right,ri.bottom,10,C_HOVER,C_BG,0);
    }

    HPEN gp=CreatePen(PS_SOLID,2,C_GRID),ogp=(HPEN)SelectObject(dc,gp);
    for(int c=1;c<3;c++){int x=TTT_BX+c*TTT_CS;MoveToEx(dc,x,TTT_BY+14,NULL);LineTo(dc,x,TTT_BY+TTT_CS*3-14);}
    for(int r=1;r<3;r++){int y=TTT_BY+r*TTT_CS;MoveToEx(dc,TTT_BX+14,y,NULL);LineTo(dc,TTT_BX+TTT_CS*3-14,y);}
    SelectObject(dc,ogp); DeleteObject(gp);

    for(int i=0;i<9;i++){
        int v=g_app.ttt.get_cell(i); if(!v) continue;
        RECT r=ttt_cell_rect(i);
        if(v==1) draw_x(dc,r,wc[i]?C_WIN:C_X);
        else     draw_o(dc,r,wc[i]?C_WIN:C_O);
    }

    if (g_app.state==GAME_OVER){
        bool ah=(g_app.hover_btn==3);
        rrect(dc,R_AG_TTT.left,R_AG_TTT.top,R_AG_TTT.right,R_AG_TTT.bottom,10,ah?C_BTNHOV:C_BTN,C_GRID,2);
        text_cx(dc,"Play Again",(R_AG_TTT.left+R_AG_TTT.right)/2,R_AG_TTT.top+10,g_fnt_btn,C_TEXT);
    }
}

static void paint_oth(HDC dc) {
    int blk=g_app.oth.count(1), wht=g_app.oth.count(-1);
    char sbuf[64]; sprintf_s(sbuf,sizeof(sbuf),"Black: %d    White: %d",blk,wht);
    const char* st;
    if (g_app.state==PLAYING){
        int cp=g_app.oth.current_player();
        st=(cp==g_app.human)?(g_app.human==1?"Your turn (Black)":"Your turn (White)"):"AI thinking...";
    } else {
        st=(g_app.result==g_app.human)?"You win!":(g_app.result==0)?"Draw!":"AI wins!";
    }
    text_cx(dc,st,   WIN_W/2,14,g_fnt_status,C_TEXT);
    text_cx(dc,sbuf, WIN_W/2,43,g_fnt_score, C_DIM);

    RECT br={OTH_BX,OTH_BY,OTH_BX+OTH_CS*8,OTH_BY+OTH_CS*8}; fill_rc(dc,br,C_OB_BG);
    HPEN gp=CreatePen(PS_SOLID,1,C_OB_GRID),ogp=(HPEN)SelectObject(dc,gp);
    for(int i=1;i<8;i++){
        int x=OTH_BX+i*OTH_CS; MoveToEx(dc,x,OTH_BY,NULL); LineTo(dc,x,OTH_BY+OTH_CS*8);
        int y=OTH_BY+i*OTH_CS; MoveToEx(dc,OTH_BX,y,NULL); LineTo(dc,OTH_BX+OTH_CS*8,y);
    }
    SelectObject(dc,ogp); DeleteObject(gp);

    bool can_move[64]={false};
    if (g_app.state==PLAYING&&g_app.oth.current_player()==g_app.human)
        for(int i=0;i<64;i++) can_move[i]=g_app.oth.can_place(i);

    for(int i=0;i<64;i++){
        RECT r=oth_cell_rect(i);
        int cx=(r.left+r.right)/2, cy=(r.top+r.bottom)/2, pad=4;
        int v=g_app.oth.get_cell(i);
        if      (v== 1) disc(dc,cx,cy,OTH_CS/2-pad,C_OB_BLACK);
        else if (v==-1) disc(dc,cx,cy,OTH_CS/2-pad,C_OB_WHITE);
        else if (can_move[i]){
            COLORREF hc=(i==g_app.hover_cell)?C_OB_HOV:C_OB_HINT;
            disc(dc,cx,cy,OTH_CS/2-pad,hc);
            disc(dc,cx,cy,OTH_CS/2-pad-7,C_OB_BG);
        }
    }

    if (g_app.state==GAME_OVER){
        bool ah=(g_app.hover_btn==3);
        rrect(dc,R_AG_OTH.left,R_AG_OTH.top,R_AG_OTH.right,R_AG_OTH.bottom,10,ah?C_BTNHOV:C_BTN,C_GRID,2);
        text_cx(dc,"Play Again",(R_AG_OTH.left+R_AG_OTH.right)/2,R_AG_OTH.top+10,g_fnt_btn,C_TEXT);
    }
}

static void paint_gom(HDC dc) {
    const char* st;
    if (g_app.state==PLAYING){
        int cp=g_app.gom.current_player();
        st=(cp==g_app.human)?(g_app.human==1?"Your turn (Black)":"Your turn (White)"):"AI thinking...";
    } else {
        st=(g_app.result==g_app.human)?"You win!":(g_app.result==0)?"Draw!":"AI wins!";
    }
    text_cx(dc,st,WIN_W/2,18,g_fnt_status,C_TEXT);

    // Board background
    RECT br={GOM_BX-12,GOM_BY-12,GOM_BX+14*GOM_CS+12,GOM_BY+14*GOM_CS+12};
    fill_rc(dc,br,C_GM_BG);

    // Grid lines
    HPEN gp=CreatePen(PS_SOLID,1,C_GM_GRID),ogp=(HPEN)SelectObject(dc,gp);
    for(int i=0;i<15;i++){
        int x=GOM_BX+i*GOM_CS, y=GOM_BY+i*GOM_CS;
        MoveToEx(dc,x,GOM_BY,NULL);           LineTo(dc,x,GOM_BY+14*GOM_CS);
        MoveToEx(dc,GOM_BX,y,NULL);           LineTo(dc,GOM_BX+14*GOM_CS,y);
    }
    SelectObject(dc,ogp); DeleteObject(gp);

    // Star points (hoshi): center + 4 symmetric corner points
    static const int STARS[5][2]={{7,7},{3,3},{3,11},{11,3},{11,11}};
    for(int i=0;i<5;i++)
        disc(dc,GOM_BX+STARS[i][1]*GOM_CS,GOM_BY+STARS[i][0]*GOM_CS,3,C_GM_STAR);

    // Hover ghost stone
    if (g_app.state==PLAYING&&g_app.hover_cell>=0&&g_app.gom.get_cell(g_app.hover_cell)==0){
        int hc=g_app.hover_cell;
        int cx=GOM_BX+(hc%15)*GOM_CS, cy=GOM_BY+(hc/15)*GOM_CS;
        COLORREF ghost=(g_app.human==1)?RGB(60,60,78):RGB(195,190,176);
        disc(dc,cx,cy,GOM_CS/2-3,ghost);
    }

    // Winning cells
    bool wc[225]={false};
    g_app.gom.get_winning_cells(wc);

    // Stones
    int last_mv=g_app.gom.get_last_move();
    int sr=GOM_CS/2-3;  // stone radius
    for(int i=0;i<225;i++){
        int v=g_app.gom.get_cell(i); if(!v) continue;
        int cx=GOM_BX+(i%15)*GOM_CS, cy=GOM_BY+(i/15)*GOM_CS;

        if(wc[i]) disc(dc,cx,cy,sr+5,C_WIN);  // green halo for winning stones

        if(v==1){
            disc(dc,cx,cy,sr,C_GM_BLACK);
        } else {
            disc(dc,cx,cy,sr+1,C_GM_WBDR);  // thin border
            disc(dc,cx,cy,sr,  C_GM_WHITE);
        }

        // Last-move dot in contrasting color
        if(i==last_mv){
            COLORREF dot=(v==1)?RGB(210,55,55):RGB(55,55,210);
            disc(dc,cx,cy,4,dot);
        }
    }

    if (g_app.state==GAME_OVER){
        bool ah=(g_app.hover_btn==3);
        rrect(dc,R_AG_GOM.left,R_AG_GOM.top,R_AG_GOM.right,R_AG_GOM.bottom,10,ah?C_BTNHOV:C_BTN,C_GRID,2);
        text_cx(dc,"Play Again",(R_AG_GOM.left+R_AG_GOM.right)/2,R_AG_GOM.top+10,g_fnt_btn,C_TEXT);
    }
}

// Chess piece unicode: K=♔/♚ Q=♕/♛ R=♖/♜ B=♗/♝ N=♘/♞ P=♙/♟
static const wchar_t CHE_WPIECES[7]={0,0x2659,0x2658,0x2657,0x2656,0x2655,0x2654};
static const wchar_t CHE_BPIECES[7]={0,0x265F,0x265E,0x265D,0x265C,0x265B,0x265A};

static int che_sq_from_pt(int x,int y){
    int bx=x-CHE_BX,by=y-CHE_BY;
    if(bx<0||by<0||bx>=CHE_CS*8||by>=CHE_CS*8) return -1;
    return (by/CHE_CS)*8+(bx/CHE_CS);
}

static bool che_move_to_in_lgl(int to){
    for(int i=0;i<(int)g_app.chess_lgl.size();i++)
        if(((g_app.chess_lgl[i]>>6)&63)==to) return true;
    return false;
}

static void paint_chess(HDC dc){
    const char* st;
    if(g_app.state==PLAYING){
        int cp=g_app.che.current_player();
        if(cp==g_app.human)
            st=(g_app.human==1)?"Your turn (White)":"Your turn (Black)";
        else
            st="AI thinking...";
    } else {
        st=(g_app.result==g_app.human)?"You win!":(g_app.result==0)?"Draw!":"AI wins!";
    }
    text_cx(dc,st,WIN_W/2,22,g_fnt_status,C_TEXT);

    bool in_chk=g_app.che.is_in_check(g_app.che.current_player());
    int last_fr=g_app.che.get_last_from();
    int last_to=g_app.che.get_last_to();

    // Find current player's king square for check highlight
    int king_sq=-1;
    if(in_chk){
        int kp=g_app.che.current_player();
        for(int i=0;i<64;i++) if(g_app.che.get_cell(i)==kp*6){king_sq=i;break;}
    }

    // Draw squares
    for(int i=0;i<64;i++){
        int row=i/8,col2=i%8;
        bool light=((row+col2)%2==0);
        COLORREF sqc;
        if(i==king_sq)                           sqc=light?C_CH_CHK_L:C_CH_CHK_D;
        else if(i==g_app.chess_sel)              sqc=light?C_CH_SEL_L:C_CH_SEL_D;
        else if(i==last_fr||i==last_to)          sqc=light?C_CH_LAST_L:C_CH_LAST_D;
        else                                     sqc=light?C_CH_LIGHT:C_CH_DARK;
        int sx=CHE_BX+col2*CHE_CS,sy=CHE_BY+row*CHE_CS;
        RECT sr={sx,sy,sx+CHE_CS,sy+CHE_CS};
        fill_rc(dc,sr,sqc);

        // Legal move dot / ring
        if(g_app.state==PLAYING&&che_move_to_in_lgl(i)){
            int cx2=sx+CHE_CS/2,cy=sy+CHE_CS/2;
            if(g_app.che.get_cell(i)==0){
                disc(dc,cx2,cy,9,C_CH_DOT);
            } else {
                // Ring around enemy piece
                HPEN rp=CreatePen(PS_SOLID,3,C_CH_DOT);
                HBRUSH nb=(HBRUSH)GetStockObject(NULL_BRUSH);
                HPEN op=(HPEN)SelectObject(dc,rp);
                HBRUSH ob=(HBRUSH)SelectObject(dc,nb);
                Ellipse(dc,cx2-CHE_CS/2+3,cy-CHE_CS/2+3,cx2+CHE_CS/2-3,cy+CHE_CS/2-3);
                SelectObject(dc,op);SelectObject(dc,ob);DeleteObject(rp);
            }
        }
    }

    // Board border
    HPEN bp=CreatePen(PS_SOLID,2,C_GRID),obp=(HPEN)SelectObject(dc,bp);
    HBRUSH bnb=(HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH obbr=(HBRUSH)SelectObject(dc,bnb);
    Rectangle(dc,CHE_BX,CHE_BY,CHE_BX+CHE_CS*8,CHE_BY+CHE_CS*8);
    SelectObject(dc,obp);SelectObject(dc,obbr);DeleteObject(bp);

    // Draw pieces
    HFONT of2=(HFONT)SelectObject(dc,g_fnt_piece);
    SetBkMode(dc,TRANSPARENT);
    for(int i=0;i<64;i++){
        int v=g_app.che.get_cell(i); if(!v) continue;
        int row=i/8,col2=i%8;
        int cx2=CHE_BX+col2*CHE_CS+CHE_CS/2;
        int cy=CHE_BY+row*CHE_CS+CHE_CS/2;
        int pt=(v<0)?-v:v;
        wchar_t wch[2]={(v>0)?CHE_WPIECES[pt]:CHE_BPIECES[pt],0};
        // Shadow for visibility
        SetTextColor(dc,RGB(0,0,0));
        SIZE sz; GetTextExtentPoint32W(dc,wch,1,&sz);
        TextOutW(dc,cx2-sz.cx/2+1,cy-sz.cy/2+1,wch,1);
        // Piece
        SetTextColor(dc,(v>0)?C_CH_WP:C_CH_BP);
        TextOutW(dc,cx2-sz.cx/2,cy-sz.cy/2,wch,1);
    }
    SelectObject(dc,of2);

    if(g_app.state==GAME_OVER){
        bool ah=(g_app.hover_btn==3);
        rrect(dc,R_AG_CHE.left,R_AG_CHE.top,R_AG_CHE.right,R_AG_CHE.bottom,10,ah?C_BTNHOV:C_BTN,C_GRID,2);
        text_cx(dc,"Play Again",(R_AG_CHE.left+R_AG_CHE.right)/2,R_AG_CHE.top+10,g_fnt_btn,C_TEXT);
    }
}

static void paint(HDC dc) {
    RECT bg={0,0,WIN_W,WIN_H}; fill_rc(dc,bg,C_BG);
    switch(g_app.state){
    case MENU:        paint_menu(dc);        break;
    case CHOOSE_SIDE: paint_choose_side(dc); break;
    case PLAYING: case GAME_OVER:
        if      (g_app.game==GAME_TTT) paint_ttt(dc);
        else if (g_app.game==GAME_OTH) paint_oth(dc);
        else if (g_app.game==GAME_GOM) paint_gom(dc);
        else                           paint_chess(dc);
        break;
    }

    // Undo button — shown while playing
    if (g_app.state==PLAYING) {
        bool uh=(g_app.hover_btn==4);
        rrect(dc,R_UNDO.left,R_UNDO.top,R_UNDO.right,R_UNDO.bottom,8,uh?C_BTNHOV:C_BTN,C_GRID,2);
        text_cx(dc,"Undo",(R_UNDO.left+R_UNDO.right)/2,R_UNDO.top+6,g_fnt_btn,uh?C_TEXT:C_DIM);
    }

    // Taunt popup — semi-transparent fullscreen overlay + centered card
    if (g_app.undo_taunt) {
        HDC odc = CreateCompatibleDC(dc);
        HBITMAP obmp = CreateCompatibleBitmap(dc, WIN_W, WIN_H);
        HBITMAP oldbmp = (HBITMAP)SelectObject(odc, obmp);
        RECT fill = {0, 0, WIN_W, WIN_H}; fill_rc(odc, fill, RGB(0,0,0));
        BLENDFUNCTION bf = {AC_SRC_OVER, 0, 200, 0};
        AlphaBlend(dc, 0, 0, WIN_W, WIN_H, odc, 0, 0, WIN_W, WIN_H, bf);
        SelectObject(odc, oldbmp); DeleteObject(obmp); DeleteDC(odc);

        int cw=440, ch=180, cx=(WIN_W-cw)/2, cy=(WIN_H-ch)/2;
        rrect(dc, cx, cy, cx+cw, cy+ch, 24, C_PANEL, C_TITLE, 2);
        text_cx(dc, "im too good for undo", WIN_W/2, cy+55, g_fnt_big, C_O);
    }
}

static void on_paint(HWND hwnd) {
    PAINTSTRUCT ps; HDC hdc=BeginPaint(hwnd,&ps);
    HDC mdc=CreateCompatibleDC(hdc);
    HBITMAP bmp=CreateCompatibleBitmap(hdc,WIN_W,WIN_H);
    HBITMAP obmp=(HBITMAP)SelectObject(mdc,bmp);
    paint(mdc);
    BitBlt(hdc,0,0,WIN_W,WIN_H,mdc,0,0,SRCCOPY);
    SelectObject(mdc,obmp); DeleteObject(bmp); DeleteDC(mdc);
    EndPaint(hwnd,&ps);
}

// ---- game logic ----

static void do_ai_move_ttt() {
    int mv=Minimax::best_move(&g_app.ttt,9);
    if(mv>=0){TicTacToe* n=static_cast<TicTacToe*>(g_app.ttt.apply_move(mv));g_app.ttt=*n;delete n;}
    if(g_app.ttt.is_terminal()){g_app.state=GAME_OVER;g_app.result=g_app.ttt.winner();}
    InvalidateRect(g_hwnd,NULL,FALSE);
}

// Forward declaration needed because check_oth_turn and do_ai_move_oth call each other
static void check_oth_turn(HWND hwnd);

static void do_ai_move_oth(HWND hwnd) {
    int mv=Minimax::best_move(&g_app.oth,5);
    Othello* n=static_cast<Othello*>(g_app.oth.apply_move(mv>=0?mv:-1));
    g_app.oth=*n; delete n;
    check_oth_turn(hwnd);
}

static void check_oth_turn(HWND hwnd) {
    if(g_app.oth.is_terminal()){
        g_app.state=GAME_OVER;
        int b=g_app.oth.count(1),w=g_app.oth.count(-1);
        g_app.result=(b>w)?1:(w>b)?-1:0;
        InvalidateRect(hwnd,NULL,FALSE); return;
    }
    // Auto-pass if current player has no legal placements
    std::vector<int> moves=g_app.oth.get_moves();
    if(moves.size()==1&&moves[0]==-1){
        Othello* n=static_cast<Othello*>(g_app.oth.apply_move(-1));
        g_app.oth=*n; delete n;
    }
    InvalidateRect(hwnd,NULL,FALSE);
    if(g_app.oth.current_player()!=g_app.human){UpdateWindow(hwnd);PostMessage(hwnd,WM_AI_MOVE,0,0);}
}

static void check_gom_turn(HWND hwnd);

static void do_ai_move_gom(HWND hwnd) {
    int mv=Minimax::best_move(&g_app.gom,4);
    if(mv>=0){Gomoku* n=static_cast<Gomoku*>(g_app.gom.apply_move(mv));g_app.gom=*n;delete n;}
    check_gom_turn(hwnd);
}

static void check_gom_turn(HWND hwnd) {
    if(g_app.gom.is_terminal()){
        g_app.state=GAME_OVER; g_app.result=g_app.gom.winner();
        InvalidateRect(hwnd,NULL,FALSE); return;
    }
    InvalidateRect(hwnd,NULL,FALSE);
    if(g_app.gom.current_player()!=g_app.human){UpdateWindow(hwnd);PostMessage(hwnd,WM_AI_MOVE,0,0);}
}

static void check_che_turn(HWND hwnd);

static void do_ai_move_che(HWND hwnd){
    int mv=Minimax::best_move(&g_app.che,4);
    if(mv>=0){Chess* n=static_cast<Chess*>(g_app.che.apply_move(mv));g_app.che=*n;delete n;}
    check_che_turn(hwnd);
}

static void check_che_turn(HWND hwnd){
    if(g_app.che.is_terminal()){
        g_app.state=GAME_OVER; g_app.result=g_app.che.winner();
        InvalidateRect(hwnd,NULL,FALSE); return;
    }
    InvalidateRect(hwnd,NULL,FALSE);
    if(g_app.che.current_player()!=g_app.human){UpdateWindow(hwnd);PostMessage(hwnd,WM_AI_MOVE,0,0);}
}

static void start_game(HWND hwnd, GameType game, int human) {
    g_app.game=game; g_app.human=human;
    g_app.state=PLAYING; g_app.hover_cell=-1; g_app.hover_btn=0; g_app.result=0;
    g_app.undo_taunt=false; g_app.chess_sel=-1; g_app.chess_lgl.clear();
    g_app.ttt=TicTacToe(); g_app.oth=Othello(); g_app.gom=Gomoku(); g_app.che=Chess();
    InvalidateRect(hwnd,NULL,FALSE);
    if(human==-1){UpdateWindow(hwnd);PostMessage(hwnd,WM_AI_MOVE,0,0);}
}

// ---- input ----

static bool is_clickable(int x, int y) {
    switch(g_app.state){
    case MENU:
        return pt_in(R_M_TTT,x,y)||pt_in(R_M_OTH,x,y)||pt_in(R_M_GOM,x,y)||pt_in(R_M_CHE,x,y);
    case CHOOSE_SIDE:
        return pt_in(R_CS_1,x,y)||pt_in(R_CS_2,x,y);
    case PLAYING:
        if(pt_in(R_UNDO,x,y)) return true;
        if(g_app.game==GAME_TTT){int c=ttt_from_pt(x,y);return c>=0&&g_app.ttt.get_cell(c)==0&&g_app.ttt.current_player()==g_app.human;}
        if(g_app.game==GAME_OTH){int c=oth_from_pt(x,y);return c>=0&&g_app.oth.can_place(c)&&g_app.oth.current_player()==g_app.human;}
        if(g_app.game==GAME_GOM){int c=gom_from_pt(x,y);return c>=0&&g_app.gom.get_cell(c)==0&&g_app.gom.current_player()==g_app.human;}
        {   // Chess: clickable if human's turn and clicking board
            if(g_app.che.current_player()!=g_app.human) return false;
            int c=che_sq_from_pt(x,y); if(c<0) return false;
            int v=g_app.che.get_cell(c);
            // Clickable if: has a piece selected and this is a legal dest or own piece, OR no selection and own piece
            if(g_app.chess_sel>=0&&che_move_to_in_lgl(c)) return true;
            return (g_app.human==1?v>0:v<0);
        }
    case GAME_OVER:
        return pt_in(get_again_rect(),x,y);
    }
    return false;
}

static void update_hover(HWND hwnd, int x, int y) {
    int pc=g_app.hover_cell, pb=g_app.hover_btn;
    g_app.hover_cell=-1; g_app.hover_btn=0;
    switch(g_app.state){
    case MENU:
        if      (pt_in(R_M_TTT,x,y)) g_app.hover_btn=1;
        else if (pt_in(R_M_OTH,x,y)) g_app.hover_btn=2;
        else if (pt_in(R_M_GOM,x,y)) g_app.hover_btn=3;
        else if (pt_in(R_M_CHE,x,y)) g_app.hover_btn=4;
        break;
    case CHOOSE_SIDE:
        if      (pt_in(R_CS_1,x,y)) g_app.hover_btn=1;
        else if (pt_in(R_CS_2,x,y)) g_app.hover_btn=2;
        break;
    case PLAYING:
        if(pt_in(R_UNDO,x,y)){g_app.hover_btn=4; break;}
        if(g_app.game==GAME_TTT){int c=ttt_from_pt(x,y);if(c>=0&&g_app.ttt.get_cell(c)==0&&g_app.ttt.current_player()==g_app.human)g_app.hover_cell=c;}
        else if(g_app.game==GAME_OTH){int c=oth_from_pt(x,y);if(c>=0&&g_app.oth.can_place(c)&&g_app.oth.current_player()==g_app.human)g_app.hover_cell=c;}
        else if(g_app.game==GAME_GOM){int c=gom_from_pt(x,y);if(c>=0&&g_app.gom.get_cell(c)==0&&g_app.gom.current_player()==g_app.human)g_app.hover_cell=c;}
        else{int c=che_sq_from_pt(x,y);if(c>=0)g_app.hover_cell=c;}
        break;
    case GAME_OVER:
        if(pt_in(get_again_rect(),x,y)) g_app.hover_btn=3;
        break;
    }
    if(g_app.hover_cell!=pc||g_app.hover_btn!=pb) InvalidateRect(hwnd,NULL,FALSE);
}

static void on_click(HWND hwnd, int x, int y) {
    switch(g_app.state){
    case MENU:
        if      (pt_in(R_M_TTT,x,y)){g_app.game=GAME_TTT;g_app.state=CHOOSE_SIDE;g_app.hover_btn=0;InvalidateRect(hwnd,NULL,FALSE);}
        else if (pt_in(R_M_OTH,x,y)){g_app.game=GAME_OTH;g_app.state=CHOOSE_SIDE;g_app.hover_btn=0;InvalidateRect(hwnd,NULL,FALSE);}
        else if (pt_in(R_M_GOM,x,y)){g_app.game=GAME_GOM;g_app.state=CHOOSE_SIDE;g_app.hover_btn=0;InvalidateRect(hwnd,NULL,FALSE);}
        else if (pt_in(R_M_CHE,x,y)){g_app.game=GAME_CHE;g_app.state=CHOOSE_SIDE;g_app.hover_btn=0;InvalidateRect(hwnd,NULL,FALSE);}
        break;
    case CHOOSE_SIDE:
        if      (pt_in(R_CS_1,x,y)) start_game(hwnd,g_app.game, 1);
        else if (pt_in(R_CS_2,x,y)) start_game(hwnd,g_app.game,-1);
        break;
    case PLAYING:
        if(pt_in(R_UNDO,x,y)){
            g_app.undo_taunt=true;
            InvalidateRect(hwnd,NULL,FALSE);
            SetTimer(hwnd,1,2000,NULL);
            break;
        }
        if(g_app.game==GAME_TTT){
            int c=ttt_from_pt(x,y);
            if(c>=0&&g_app.ttt.get_cell(c)==0&&g_app.ttt.current_player()==g_app.human){
                TicTacToe* n=static_cast<TicTacToe*>(g_app.ttt.apply_move(c));
                g_app.ttt=*n; delete n;
                if(g_app.ttt.is_terminal()){g_app.state=GAME_OVER;g_app.result=g_app.ttt.winner();InvalidateRect(hwnd,NULL,FALSE);}
                else{InvalidateRect(hwnd,NULL,FALSE);UpdateWindow(hwnd);PostMessage(hwnd,WM_AI_MOVE,0,0);}
            }
        } else if(g_app.game==GAME_OTH){
            int c=oth_from_pt(x,y);
            if(c>=0&&g_app.oth.can_place(c)&&g_app.oth.current_player()==g_app.human){
                Othello* n=static_cast<Othello*>(g_app.oth.apply_move(c));
                g_app.oth=*n; delete n;
                check_oth_turn(hwnd);
            }
        } else if(g_app.game==GAME_GOM){
            int c=gom_from_pt(x,y);
            if(c>=0&&g_app.gom.get_cell(c)==0&&g_app.gom.current_player()==g_app.human){
                Gomoku* n=static_cast<Gomoku*>(g_app.gom.apply_move(c));
                g_app.gom=*n; delete n;
                check_gom_turn(hwnd);
            }
        } else {
            // Chess: click-click interface
            if(g_app.che.current_player()==g_app.human){
                int c=che_sq_from_pt(x,y); if(c<0) break;
                int v=g_app.che.get_cell(c);
                bool is_own=(g_app.human==1)?v>0:v<0;
                if(g_app.chess_sel>=0&&che_move_to_in_lgl(c)){
                    // Execute the move (auto-promote to queen)
                    int chosen=-1;
                    for(int i=0;i<(int)g_app.chess_lgl.size();i++){
                        int mv=g_app.chess_lgl[i];
                        if(((mv>>6)&63)==c){
                            if(chosen<0||(mv>>12&15)==4) chosen=mv;
                        }
                    }
                    if(chosen>=0){
                        Chess* n=static_cast<Chess*>(g_app.che.apply_move(chosen));
                        g_app.che=*n; delete n;
                        g_app.chess_sel=-1; g_app.chess_lgl.clear();
                        check_che_turn(hwnd);
                    }
                } else if(is_own){
                    // Select this piece
                    g_app.chess_sel=c;
                    g_app.chess_lgl.clear();
                    std::vector<int> all=g_app.che.get_moves();
                    for(int i=0;i<(int)all.size();i++)
                        if((all[i]&63)==c) g_app.chess_lgl.push_back(all[i]);
                    InvalidateRect(hwnd,NULL,FALSE);
                } else {
                    g_app.chess_sel=-1; g_app.chess_lgl.clear();
                    InvalidateRect(hwnd,NULL,FALSE);
                }
            }
        }
        break;
    case GAME_OVER:
        if(pt_in(get_again_rect(),x,y)){g_app.state=MENU;g_app.hover_btn=0;g_app.hover_cell=-1;InvalidateRect(hwnd,NULL,FALSE);}
        break;
    }
}

// ---- window procedure ----

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg){
    case WM_TIMER:
        KillTimer(hwnd,1);
        g_app.undo_taunt=false;
        InvalidateRect(hwnd,NULL,FALSE);
        return 0;
    case WM_AI_MOVE:
        if      (g_app.game==GAME_TTT) do_ai_move_ttt();
        else if (g_app.game==GAME_OTH) do_ai_move_oth(hwnd);
        else if (g_app.game==GAME_GOM) do_ai_move_gom(hwnd);
        else                           do_ai_move_che(hwnd);
        return 0;
    case WM_MOUSEMOVE:
        update_hover(hwnd,(int)(short)LOWORD(lp),(int)(short)HIWORD(lp)); return 0;
    case WM_LBUTTONDOWN:
        on_click(hwnd,(int)(short)LOWORD(lp),(int)(short)HIWORD(lp)); return 0;
    case WM_SETCURSOR:
        if(LOWORD(lp)==HTCLIENT){
            POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd,&pt);
            SetCursor(LoadCursor(NULL,is_clickable(pt.x,pt.y)?IDC_HAND:IDC_ARROW));
            return TRUE;
        } break;
    case WM_PAINT:      on_paint(hwnd); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DESTROY:    PostQuitMessage(0); return 0;
    }
    return DefWindowProcA(hwnd,msg,wp,lp);
}

// ---- entry point ----

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    g_fnt_title  = CreateFontA(44,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    g_fnt_status = CreateFontA(24,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    g_fnt_btn    = CreateFontA(20,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    g_fnt_label  = CreateFontA(16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    g_fnt_score  = CreateFontA(18,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    g_fnt_big    = CreateFontA(42,0,0,0,FW_BOLD,  0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_SWISS,"Segoe UI");
    g_fnt_piece  = CreateFontA(44,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,"Segoe UI Symbol");

    g_app.state=MENU; g_app.game=GAME_TTT; g_app.human=1;
    g_app.hover_cell=-1; g_app.hover_btn=0; g_app.result=0; g_app.undo_taunt=false;
    g_app.chess_sel=-1;

    WNDCLASSEXA wc={}; wc.cbSize=sizeof(wc); wc.style=CS_HREDRAW|CS_VREDRAW;
    wc.lpfnWndProc=WndProc; wc.hInstance=hInst;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW); wc.hbrBackground=NULL;
    wc.lpszClassName="MinimaxGames"; wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
    RegisterClassExA(&wc);

    RECT cr={0,0,WIN_W,WIN_H};
    AdjustWindowRect(&cr,WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
    g_hwnd=CreateWindowExA(0,"MinimaxGames","Minimax Games",
        WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT,CW_USEDEFAULT,cr.right-cr.left,cr.bottom-cr.top,
        NULL,NULL,hInst,NULL);
    ShowWindow(g_hwnd,nCmdShow); UpdateWindow(g_hwnd);

    MSG msg;
    while(GetMessageA(&msg,NULL,0,0)){TranslateMessage(&msg);DispatchMessageA(&msg);}

    DeleteObject(g_fnt_title); DeleteObject(g_fnt_status);
    DeleteObject(g_fnt_btn);   DeleteObject(g_fnt_label); DeleteObject(g_fnt_score);
    DeleteObject(g_fnt_big);   DeleteObject(g_fnt_piece);
    return (int)msg.wParam;
}
