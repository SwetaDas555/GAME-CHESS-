#include <iostream>
#include <string>
#include <vector>
#include <cctype>   // For isupper, tolower
#include <cmath>    // For abs
#include <cstdlib>  // For system()
#include <limits>   // Required for numeric_limits

using namespace std;

// --- Configuration ---
const int BOARD_SIZE = 8; // Board dimensions are 8x8
const bool USE_UNICODE_SYMBOLS = true;
const bool USE_ANSI_COLORS = true;

// --- ANSI Color Codes ---
const string ANSI_RESET = "\033[0m";
const string ANSI_BG_LIGHT = "\033[47m";
const string ANSI_BG_DARK = "\033[100m";
const string ANSI_FG_BLACK = "\033[30m";
const string ANSI_FG_WHITE = "\033[97m";

// --- Helper Functions ---

string getPieceVisual(char piece) {
    if (USE_UNICODE_SYMBOLS) {
        switch (piece) {
            case 'P': return u8"\u2659"; case 'p': return u8"\u265F"; case 'R': return u8"\u2656"; case 'r': return u8"\u265C";
            case 'N': return u8"\u2658"; case 'n': return u8"\u265E"; case 'B': return u8"\u2657"; case 'b': return u8"\u265D";
            case 'Q': return u8"\u2655"; case 'q': return u8"\u265B"; case 'K': return u8"\u2654"; case 'k': return u8"\u265A";
            default: return " "; // Use space for empty with Unicode
        }
    } else {
        if (piece == ' ' || piece == '.') return "."; // Use dot for empty with ASCII
        string s(1, piece); return s;
    }
}

bool isWithinBounds(int r, int c) { return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE; }

// --- ChessGame Class ---
class ChessGame {
private:
    // Board is defined as 8x8
    char board[BOARD_SIZE][BOARD_SIZE];
    bool isWhiteTurn;
    int whiteKingRow, whiteKingCol;
    int blackKingRow, blackKingCol;
    string lastMoveNotation = "N/A";
    vector<char> whiteCaptured;
    vector<char> blackCaptured;

    // --- Basic Helpers ---
    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }
    bool notationToIndex(const string& n, int& r, int& c) const { if(n.length()!=2) return false; char f=tolower(n[0]); char rnk=n[1]; if(f<'a'||f>'h'||rnk<'1'||rnk>'8') return false; c=f-'a'; r='8'-rnk; return isWithinBounds(r,c); }
    string indexToNotation(int r, int c) const { if(!isWithinBounds(r,c)) return "??"; char f='a'+c; char rnk='8'-r; string s=""; s+=f; s+=rnk; return s; }
    char getPieceAt(int r, int c) const { if(!isWithinBounds(r,c)) return ' '; return board[r][c]; }
    bool isPieceWhite(char p) const { return p!='.'&&p!=' '&&isupper(p); }
    bool isPieceBlack(char p) const { return p!='.'&&p!=' '&&islower(p); }

    // --- Attack & Check Logic ---
    bool isSquareAttacked(int r, int c, bool attackerIsWhite) const {
        char attackingPawn = attackerIsWhite ? 'P' : 'p'; char attackingRook = attackerIsWhite ? 'R' : 'r';
        char attackingKnight = attackerIsWhite ? 'N' : 'n'; char attackingBishop = attackerIsWhite ? 'B' : 'b';
        char attackingQueen = attackerIsWhite ? 'Q' : 'q'; char attackingKing = attackerIsWhite ? 'K' : 'k';
        int pawnAttackSourceDir = attackerIsWhite ? 1 : -1;
        if (isWithinBounds(r + pawnAttackSourceDir, c - 1) && getPieceAt(r + pawnAttackSourceDir, c - 1) == attackingPawn) return true;
        if (isWithinBounds(r + pawnAttackSourceDir, c + 1) && getPieceAt(r + pawnAttackSourceDir, c + 1) == attackingPawn) return true;
        int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for(const auto& m:knightMoves) if(isWithinBounds(r+m[0],c+m[1])&&getPieceAt(r+m[0],c+m[1])==attackingKnight) return true;
        int rookDirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for(const auto& d:rookDirs) for(int i=1;;++i){int nr=r+i*d[0],nc=c+i*d[1]; if(!isWithinBounds(nr,nc))break; char pc=getPieceAt(nr,nc); if(pc!='.'){if(pc==attackingRook||pc==attackingQueen)return true; break;}}
        int bishopDirs[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
        for(const auto& d:bishopDirs) for(int i=1;;++i){int nr=r+i*d[0],nc=c+i*d[1]; if(!isWithinBounds(nr,nc))break; char pc=getPieceAt(nr,nc); if(pc!='.'){if(pc==attackingBishop||pc==attackingQueen)return true; break;}}
        for(int dr=-1;dr<=1;++dr)for(int dc=-1;dc<=1;++dc){if(dr==0&&dc==0)continue; if(isWithinBounds(r+dr,c+dc)&&getPieceAt(r+dr,c+dc)==attackingKing)return true;}
        return false;
     }
    bool moveLeavesKingInCheck(int startR, int startC, int endR, int endC) {
        char piece=board[startR][startC]; char target=board[endR][endC]; board[endR][endC]=piece; board[startR][startC]='.';
        int origKR=-1,origKC=-1; if(tolower(piece)=='k'){if(isWhiteTurn){origKR=whiteKingRow;origKC=whiteKingCol;whiteKingRow=endR;whiteKingCol=endC;}else{origKR=blackKingRow;origKC=blackKingCol;blackKingRow=endR;blackKingCol=endC;}}
        bool inCheck=isKingInCheck(isWhiteTurn); board[startR][startC]=piece; board[endR][endC]=target;
        if(origKR!=-1){if(isWhiteTurn){whiteKingRow=origKR;whiteKingCol=origKC;}else{blackKingRow=origKR;blackKingCol=origKC;}} return inCheck;
     }
    bool isKingInCheck(bool checkWhiteKing) const {
        int kr=checkWhiteKing?whiteKingRow:blackKingRow; int kc=checkWhiteKing?whiteKingCol:blackKingCol; return isSquareAttacked(kr,kc,!checkWhiteKing);
     }

    // --- Move Validation Logic ---
    bool isMoveValid(int startR, int startC, int endR, int endC, string& errorMsg) {
        errorMsg = ""; if(!isWithinBounds(startR,startC)||!isWithinBounds(endR,endC)){errorMsg="Coords out of bounds."; return false;}
        char piece=getPieceAt(startR,startC); if(piece=='.'){errorMsg="No piece at start."; return false;}
        if(isPieceWhite(piece)!=isWhiteTurn){errorMsg="Not your turn for that piece."; return false;}
        char target=getPieceAt(endR,endC); if(target!='.'&&(isPieceWhite(target)==isWhiteTurn)){errorMsg="Cannot capture own piece."; return false;}
        if(startR==endR&&startC==endC){errorMsg="Start=End."; return false;}
        bool valid=false; switch(tolower(piece)){case 'p':valid=isValidPawnMove(startR,startC,endR,endC,target); break; case 'r':valid=isValidRookMove(startR,startC,endR,endC); break; case 'n':valid=isValidKnightMove(startR,startC,endR,endC); break; case 'b':valid=isValidBishopMove(startR,startC,endR,endC); break; case 'q':valid=isValidQueenMove(startR,startC,endR,endC); break; case 'k':valid=isValidKingMove(startR,startC,endR,endC); break; default: errorMsg="Unknown piece."; return false;}
        if(!valid){errorMsg="Invalid move pattern for "+string(1,piece)+"."; return false;}
        if(moveLeavesKingInCheck(startR,startC,endR,endC)){errorMsg="Move leaves King in check."; return false;} return true;
     }
    bool isValidPawnMove(int sr, int sc, int er, int ec, char target) const {char p=getPieceAt(sr,sc); int dir=isPieceWhite(p)?-1:1; int start=isPieceWhite(p)?6:1; if(sc==ec&&er==sr+dir&&target=='.')return true; if(sc==ec&&sr==start&&er==sr+2*dir&&target=='.'&&getPieceAt(sr+dir,sc)=='.')return true; if(abs(sc-ec)==1&&er==sr+dir&&target!='.')return true; return false;}
    bool isValidRookMove(int sr, int sc, int er, int ec) const {if(sr!=er&&sc!=ec)return false; int stepR=(er>sr)?1:((er<sr)?-1:0); int stepC=(ec>sc)?1:((ec<sc)?-1:0); int cr=sr+stepR; int cc=sc+stepC; while(cr!=er||cc!=ec){if(getPieceAt(cr,cc)!='.')return false; cr+=stepR; cc+=stepC;} return true;}
    bool isValidKnightMove(int sr, int sc, int er, int ec) const {int dr=abs(sr-er); int dc=abs(sc-ec); return (dr==2&&dc==1)||(dr==1&&dc==2);}
    bool isValidBishopMove(int sr, int sc, int er, int ec) const {if(abs(sr-er)!=abs(sc-ec))return false; int stepR=(er>sr)?1:-1; int stepC=(ec>sc)?1:-1; int cr=sr+stepR; int cc=sc+stepC; while(cr!=er||cc!=ec){if(getPieceAt(cr,cc)!='.')return false; cr+=stepR; cc+=stepC;} return true;}
    bool isValidQueenMove(int sr, int sc, int er, int ec) const {return isValidRookMove(sr,sc,er,ec)||isValidBishopMove(sr,sc,er,ec);}
    bool isValidKingMove(int sr, int sc, int er, int ec) const {int dr=abs(sr-er); int dc=abs(sc-ec); return dr<=1&&dc<=1;}

public:
    ChessGame() : isWhiteTurn(true) {
        initializeBoard();
    }

    void initializeBoard() {
        // Initializes the 8x8 board
        board[0][0]='r'; board[0][1]='n'; board[0][2]='b'; board[0][3]='q'; board[0][4]='k'; board[0][5]='b'; board[0][6]='n'; board[0][7]='r'; // Rank 8
        for(int j=0;j<BOARD_SIZE;++j) board[1][j]='p'; // Rank 7
        for(int i=2;i<6;++i) for(int j=0;j<BOARD_SIZE;++j) board[i][j]='.'; // Ranks 6,5,4,3
        for(int j=0;j<BOARD_SIZE;++j) board[6][j]='P'; // Rank 2
        board[7][0]='R'; board[7][1]='N'; board[7][2]='B'; board[7][3]='Q'; board[7][4]='K'; board[7][5]='B'; board[7][6]='N'; board[7][7]='R'; // Rank 1
        whiteKingRow=7; whiteKingCol=4; blackKingRow=0; blackKingCol=4;
        whiteCaptured.clear(); blackCaptured.clear();
        lastMoveNotation="N/A"; isWhiteTurn=true;
     }

    // This function prints the 8x8 board based on the board array
    void printBoard() {
        cout << "   Captured by White: "; for (char p : whiteCaptured) cout << getPieceVisual(p) << " "; cout << endl;
        cout << "     +--------------------------------+" << endl;

        // This loop iterates 8 times (i=0 to 7), printing ranks 8 down to 1
        for (int i = 0; i < BOARD_SIZE; ++i) {
            cout << "   " << (8 - i) << " |"; // Rank number
            // This loop iterates 8 times (j=0 to 7), printing files a to h
            for (int j = 0; j < BOARD_SIZE; ++j) {
                string bg_color = "", fg_color = "";
                if (USE_ANSI_COLORS) {
                    bool isLight = (i + j) % 2 == 0;
                    bg_color = isLight ? ANSI_BG_LIGHT : ANSI_BG_DARK;
                    fg_color = isLight ? ANSI_FG_BLACK : ANSI_FG_WHITE;
                    cout << bg_color << fg_color;
                }
                cout << " " << getPieceVisual(board[i][j]) << "  "; // 4-char spacing
                if (USE_ANSI_COLORS) cout << ANSI_RESET;
            }
            cout << "| " << (8 - i);

            // Side Info
            if (i == 0) cout << "    Last Move: " << lastMoveNotation;
            if (i == 2) {
                cout << "    " << (isWhiteTurn ? ">>> White's Turn" : ">>> Black's Turn");
                if (isKingInCheck(isWhiteTurn)) { cout << " (CHECK!)"; }
            }
            if (i == 4) cout << "    Enter move below";
            if (i == 5) cout << "    (e.g., e2e4)";
            cout << endl;
        } // End row loop

        cout << "     +--------------------------------+" << endl;
        cout << "       a   b   c   d   e   f   g   h" << endl; // File letters
        cout << "   Captured by Black: "; for (char p : blackCaptured) cout << getPieceVisual(p) << " "; cout << endl;

        // Legend
        cout << "----------- Legend -----------" << endl;
        if (USE_UNICODE_SYMBOLS) { cout << " White: P"<<getPieceVisual('P')<<" R"<<getPieceVisual('R')<<" N"<<getPieceVisual('N')<<" B"<<getPieceVisual('B')<<" Q"<<getPieceVisual('Q')<<" K"<<getPieceVisual('K') << endl << " Black: p"<<getPieceVisual('p')<<" r"<<getPieceVisual('r')<<" n"<<getPieceVisual('n')<<" b"<<getPieceVisual('b')<<" q"<<getPieceVisual('q')<<" k"<<getPieceVisual('k') << endl; }
        else { cout << " White: P=Pawn R=Rook N=Knight B=Bishop Q=Queen K=King" << endl << " Black: p=Pawn r=Rook n=Knight b=Bishop q=Queen k=King" << endl; }
        cout << "   " << (USE_UNICODE_SYMBOLS ? "' '" : ".") << " = Empty Square" << endl;
        cout << "-----------------------------" << endl;
    }

    void makeMove(int startR, int startC, int endR, int endC) {
        char pieceMoved=board[startR][startC]; char capturedPiece=board[endR][endC];
        if(capturedPiece!='.'){ if(isPieceWhite(capturedPiece)) blackCaptured.push_back(capturedPiece); else whiteCaptured.push_back(capturedPiece); }
        board[endR][endC]=pieceMoved; board[startR][startC]='.';
        if(tolower(pieceMoved)=='k'){ if(isWhiteTurn){whiteKingRow=endR;whiteKingCol=endC;} else {blackKingRow=endR;blackKingCol=endC;} }
        lastMoveNotation=indexToNotation(startR,startC); lastMoveNotation+=(capturedPiece!='.')?"x":"-"; lastMoveNotation+=indexToNotation(endR,endC);
        bool opponentInCheck=isKingInCheck(!isWhiteTurn); if(opponentInCheck) lastMoveNotation+="+"; isWhiteTurn=!isWhiteTurn;
     }

    void play() {
         string input; string errorMsg = "";
         while (true) {
             clearScreen(); // Clear before printing
             printBoard(); // Print the 8x8 board

             cout << endl << "*** If board seems cut off, make terminal window TALLER! ***" << endl;

             if (!errorMsg.empty()) { cout << " (!) Invalid Move: " << errorMsg << endl; errorMsg = ""; }

             cout << " Enter move (e.g. e2e4), 'resign', or 'exit': "; cin >> input;

             if (input == "exit") { cout << " Exiting game." << endl; break; }
             if (input == "resign") { cout << (isWhiteTurn ? "White" : "Black") << " resigns. " << (!isWhiteTurn ? "White" : "Black") << " wins!" << endl; break; }
             if (input.length() != 4) {
                 errorMsg = "Input must be 4 chars (e.g., e2e4).";
                 cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n'); continue;
             }
             int startR, startC, endR, endC; string startN=input.substr(0,2); string endN=input.substr(2,2);
             if (!notationToIndex(startN, startR, startC)) { errorMsg = "Invalid start: '" + startN + "'."; continue; }
             if (!notationToIndex(endN, endR, endC)) { errorMsg = "Invalid end: '" + endN + "'."; continue; }
             if (isMoveValid(startR, startC, endR, endC, errorMsg)) { makeMove(startR, startC, endR, endC); }
         }
     }

}; // End ChessGame Class


void printInstructions() {
     cout << "============================== HOW TO PLAY ==============================" << endl;
     cout << " Objective: Checkmate the opponent's King." << endl << /* ... condensed ... */ endl;
     cout << " Commands:" << endl << "            - <move> (e.g., e2e4): Make a move." << endl << "            - resign: Forfeit the game." << endl << "            - exit: Quit the program." << endl;
     cout << "=========================================================================" << endl << endl;
     cout << "*** NOTE: If the board display looks cut off, please make your ***" << endl;
     cout << "***       terminal/console window TALLER before starting!      ***" << endl << endl;
     cout << "Press Enter to start the game...";
     // Removed cin.get(), just using ignore might be sufficient
     cin.ignore(numeric_limits<streamsize>::max(), '\n');
}


int main() {
    #ifdef _WIN32
        system("chcp 65001 > nul");
    #endif
    printInstructions();
    ChessGame game;
    game.play();
    return 0;
}