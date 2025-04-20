#include <iostream>
#include <string>
#include <vector>
#include <cctype>   // For isupper, tolower
#include <cmath>    // For abs
#include <cstdlib>  // For system(), rand(), srand()
#include <limits>   // Required for numeric_limits
#include <algorithm> // For std::max_element, std::shuffle
#include <vector>    // For storing moves
#include <ctime>     // For srand(time(0))
#include <thread>    // For sleep
#include <chrono>    // For sleep_for


using namespace std;

// --- Configuration ---
const int BOARD_SIZE = 8; // Board dimensions are 8x8
const bool USE_UNICODE_SYMBOLS = true;
const bool USE_ANSI_COLORS = true;
const int AI_THINKING_MS = 500;
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

// --- Structure to represent a move ---
struct Move {
    int startR, startC, endR, endC;
    int score = 0;

    // Default constructor
    Move(int sr = -1, int sc = -1, int er = -1, int ec = -1, int s = 0)
        : startR(sr), startC(sc), endR(er), endC(ec), score(s) {}
};


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
        int pawnAttackSourceDir = attackerIsWhite ? 1 : -1; // Direction FROM which pawn attacks
        // Pawn attacks
        if (isWithinBounds(r + pawnAttackSourceDir, c - 1) && getPieceAt(r + pawnAttackSourceDir, c - 1) == attackingPawn) return true;
        if (isWithinBounds(r + pawnAttackSourceDir, c + 1) && getPieceAt(r + pawnAttackSourceDir, c + 1) == attackingPawn) return true;
        // Knight attacks
        int knightMoves[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for(const auto& m:knightMoves) if(isWithinBounds(r+m[0],c+m[1])&&getPieceAt(r+m[0],c+m[1])==attackingKnight) return true;
        // Rook/Queen (straight) attacks
        int rookDirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        for(const auto& d:rookDirs) for(int i=1;;++i){int nr=r+i*d[0],nc=c+i*d[1]; if(!isWithinBounds(nr,nc))break; char pc=getPieceAt(nr,nc); if(pc!='.'&& pc != ' '){if(pc==attackingRook||pc==attackingQueen)return true; break;} /* Stop if blocked */}
        // Bishop/Queen (diagonal) attacks
        int bishopDirs[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
        for(const auto& d:bishopDirs) for(int i=1;;++i){int nr=r+i*d[0],nc=c+i*d[1]; if(!isWithinBounds(nr,nc))break; char pc=getPieceAt(nr,nc); if(pc!='.' && pc != ' '){if(pc==attackingBishop||pc==attackingQueen)return true; break;}/* Stop if blocked */}
        // King attacks
        for(int dr=-1;dr<=1;++dr)for(int dc=-1;dc<=1;++dc){if(dr==0&&dc==0)continue; if(isWithinBounds(r+dr,c+dc)&&getPieceAt(r+dr,c+dc)==attackingKing)return true;}
        return false;
     }

    bool moveLeavesKingInCheck(int startR, int startC, int endR, int endC) {
        char piece = board[startR][startC];
        char target = board[endR][endC];
        board[endR][endC] = piece;
        board[startR][startC] = '.';

        int origKR = -1, origKC = -1;
        if (tolower(piece) == 'k') {
            if (isWhiteTurn) {
                origKR = whiteKingRow; origKC = whiteKingCol;
                whiteKingRow = endR; whiteKingCol = endC;
            } else {
                origKR = blackKingRow; origKC = blackKingCol;
                blackKingRow = endR; blackKingCol = endC;
            }
        }

        // Check if the current player's king is now in check
        bool inCheck = isKingInCheck(isWhiteTurn);

        // Undo the temporary move
        board[startR][startC] = piece;
        board[endR][endC] = target;

        // Restore king position if it was moved
        if (origKR != -1) {
            if (isWhiteTurn) {
                whiteKingRow = origKR; whiteKingCol = origKC;
            } else {
                blackKingRow = origKR; blackKingCol = origKC;
            }
        }
        return inCheck;
     }

    bool isKingInCheck(bool checkWhiteKing) const {
        int kr = checkWhiteKing ? whiteKingRow : blackKingRow;
        int kc = checkWhiteKing ? whiteKingCol : blackKingCol;
        // King is in check if the square it's on is attacked by the opponent
        return isSquareAttacked(kr, kc, !checkWhiteKing);
     }

    // --- Move Validation Logic ---
    // Overload without error message for internal/AI use
    bool isMoveValid(int startR, int startC, int endR, int endC) {
        string dummyError;
        return isMoveValid(startR, startC, endR, endC, dummyError);
    }

    // Original validation logic with error message
    bool isMoveValid(int startR, int startC, int endR, int endC, string& errorMsg) {
        errorMsg = "";
        if (!isWithinBounds(startR, startC) || !isWithinBounds(endR, endC)) {
            errorMsg = "Coordinates out of bounds."; return false;
        }
        char piece = getPieceAt(startR, startC);
        if (piece == '.' || piece == ' ') {
            errorMsg = "No piece at starting square " + indexToNotation(startR, startC) + "."; return false;
        }
        // Check if the piece belongs to the current player
        if ((isWhiteTurn && !isPieceWhite(piece)) || (!isWhiteTurn && !isPieceBlack(piece))) {
            errorMsg = "It's not that piece's turn (" + string(1,piece) + " at " + indexToNotation(startR,startC)+")."; return false;
        }

        char target = getPieceAt(endR, endC);
        // Check if capturing own piece
        if (target != '.' && target != ' ' && ((isWhiteTurn && isPieceWhite(target)) || (!isWhiteTurn && isPieceBlack(target)))) {
            errorMsg = "Cannot capture your own piece at " + indexToNotation(endR, endC) + "."; return false;
        }

        if (startR == endR && startC == endC) {
            errorMsg = "Start and end square cannot be the same."; return false;
        }

        // Validate piece-specific movement rules
        bool validPattern = false;
        switch (tolower(piece)) {
            case 'p': validPattern = isValidPawnMove(startR, startC, endR, endC, target); break;
            case 'r': validPattern = isValidRookMove(startR, startC, endR, endC); break;
            case 'n': validPattern = isValidKnightMove(startR, startC, endR, endC); break;
            case 'b': validPattern = isValidBishopMove(startR, startC, endR, endC); break;
            case 'q': validPattern = isValidQueenMove(startR, startC, endR, endC); break;
            case 'k': validPattern = isValidKingMove(startR, startC, endR, endC); break;
            default:  errorMsg = "Unknown piece type."; return false; // Should not happen
        }

        if (!validPattern) {
            errorMsg = "Invalid move pattern for " + string(1, piece) + " from " + indexToNotation(startR, startC) + " to " + indexToNotation(endR, endC) + ".";
            return false;
        }

        // Check if the move leaves the king in check (most crucial check)
        if (moveLeavesKingInCheck(startR, startC, endR, endC)) {
            errorMsg = "Move leaves your king in check.";
            return false;
        }

        // Add Castling/En Passant logic here if implementing them

        return true; // If all checks pass
     }
    // --- Piece Specific Move Logic (Mostly unchanged, ensure use '.' for empty) ---
    bool isValidPawnMove(int sr, int sc, int er, int ec, char target) const {char p=getPieceAt(sr,sc); int dir=isPieceWhite(p)?-1:1; int start=isPieceWhite(p)?6:1; // Check forward 1 square
        if(sc==ec&&er==sr+dir&&getPieceAt(er,ec)=='.')return true; // Check forward 2 squares from start
        if(sc==ec&&sr==start&&er==sr+2*dir&&getPieceAt(er,ec)=='.'&&getPieceAt(sr+dir,sc)=='.')return true; // Check diagonal capture
        if(abs(sc-ec)==1&&er==sr+dir&&(target!='.'&&target!=' '))return true; // Add En Passant check here if needed
        return false;}
    bool isValidRookMove(int sr, int sc, int er, int ec) const {if(sr!=er&&sc!=ec)return false; int stepR=(er>sr)?1:((er<sr)?-1:0); int stepC=(ec>sc)?1:((ec<sc)?-1:0); int cr=sr+stepR; int cc=sc+stepC; while(cr!=er||cc!=ec){if(getPieceAt(cr,cc)!='.')return false; cr+=stepR; cc+=stepC;} return true;}
    bool isValidKnightMove(int sr, int sc, int er, int ec) const {int dr=abs(sr-er); int dc=abs(sc-ec); return (dr==2&&dc==1)||(dr==1&&dc==2);}
    bool isValidBishopMove(int sr, int sc, int er, int ec) const {if(abs(sr-er)!=abs(sc-ec))return false; int stepR=(er>sr)?1:-1; int stepC=(ec>sc)?1:-1; int cr=sr+stepR; int cc=sc+stepC; while(cr!=er||cc!=ec){if(getPieceAt(cr,cc)!='.')return false; cr+=stepR; cc+=stepC;} return true;}
    bool isValidQueenMove(int sr, int sc, int er, int ec) const {return isValidRookMove(sr,sc,er,ec)||isValidBishopMove(sr,sc,er,ec);}
    bool isValidKingMove(int sr, int sc, int er, int ec) const {int dr=abs(sr-er); int dc=abs(sc-ec); return dr<=1&&dc<=1; /*Add castling check here*/}

    // --- AI Specific Logic ---

    // Get the value of a piece (for capture priority)
    int getPieceValue(char piece) const {
        switch (tolower(piece)) {
            case 'p': return 10;
            case 'n': return 30;
            case 'b': return 30;
            case 'r': return 50;
            case 'q': return 90;
            case 'k': return 900; // King value is high, but capture isn't the goal
            default: return 0;
        }
    }

    // Generate all valid moves for the current player
    vector<Move> generateValidMoves() {
        vector<Move> validMoves;
        char piece;
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                piece = getPieceAt(r, c);
                // Check if it's a piece belonging to the current player
                if (piece != '.' && piece != ' ' && ((isWhiteTurn && isPieceWhite(piece)) || (!isWhiteTurn && isPieceBlack(piece)))) {
                    // Try moving this piece to every other square
                    for (int er = 0; er < BOARD_SIZE; ++er) {
                        for (int ec = 0; ec < BOARD_SIZE; ++ec) {
                            if (isMoveValid(r, c, er, ec)) { // Use the internal validation
                                Move currentMove(r, c, er, ec);
                                // Score the move (simple capture scoring)
                                char targetPiece = getPieceAt(er, ec);
                                if (targetPiece != '.' && targetPiece != ' ') {
                                    currentMove.score = getPieceValue(targetPiece);
                                } else {
                                    currentMove.score = 1; // Give non-captures a small score
                                }
                                validMoves.push_back(currentMove);
                            }
                        }
                    }
                }
            }
        }
        return validMoves;
    }

    // AI makes its move (returns true if a move was made, false if no moves possible)
    bool makeAIMove() {
        vector<Move> validMoves = generateValidMoves();

        if (validMoves.empty()) {
            return false; // No legal moves - game over (checkmate or stalemate)
        }

        // Find the best move (highest score)
        int bestScore = -1;
        for(const auto& move : validMoves) {
             if (move.score > bestScore) {
                 bestScore = move.score;
             }
        }

        // Collect all moves with the best score
        vector<Move> bestMoves;
        for (const auto& move : validMoves) {
            if (move.score == bestScore) {
                bestMoves.push_back(move);
            }
        }

        // Choose randomly among the best moves
        int chosenIndex = rand() % bestMoves.size();
        Move chosenMove = bestMoves[chosenIndex];

        // Make the chosen move
        makeMove(chosenMove.startR, chosenMove.startC, chosenMove.endR, chosenMove.endC);
        return true;
    }

public:
    ChessGame() : isWhiteTurn(true) {
        initializeBoard();
        srand(time(0)); // Seed random number generator for AI
    }

    void initializeBoard() {
        // Initializes the 8x8 board (use '.' for empty)
        board[0][0]='r'; board[0][1]='n'; board[0][2]='b'; board[0][3]='q'; board[0][4]='k'; board[0][5]='b'; board[0][6]='n'; board[0][7]='r'; // Rank 8
        for(int j=0;j<BOARD_SIZE;++j) board[1][j]='p'; // Rank 7
        for(int i=2;i<6;++i) for(int j=0;j<BOARD_SIZE;++j) board[i][j]='.'; // Ranks 6,5,4,3 are empty
        for(int j=0;j<BOARD_SIZE;++j) board[6][j]='P'; // Rank 2
        board[7][0]='R'; board[7][1]='N'; board[7][2]='B'; board[7][3]='Q'; board[7][4]='K'; board[7][5]='B'; board[7][6]='N'; board[7][7]='R'; // Rank 1
        whiteKingRow=7; whiteKingCol=4; blackKingRow=0; blackKingCol=4;
        whiteCaptured.clear(); blackCaptured.clear();
        lastMoveNotation="N/A"; isWhiteTurn=true;
     }

    // This function prints the 8x8 board based on the board array
    void printBoard() {
        clearScreen();

        cout << "   Captured by White: "; for (char p : whiteCaptured) cout << getPieceVisual(p) << " "; cout << endl;
        cout << "     +--------------------------------+" << endl;

        for (int i = 0; i < BOARD_SIZE; ++i) {
            cout << "   " << (8 - i) << " |";
            // This loop iterates 8 times (j=0 to 7), printing files a to h
            for (int j = 0; j < BOARD_SIZE; ++j) {
                string bg_color = "", fg_color = "";
                if (USE_ANSI_COLORS) {
                    bool isLight = (i + j) % 2 == 0;
                    bg_color = isLight ? ANSI_BG_LIGHT : ANSI_BG_DARK;
                    // Use black text on light squares, white text on dark squares
                    fg_color = isLight ? ANSI_FG_BLACK : ANSI_FG_WHITE;
                    cout << bg_color << fg_color;
                }
                // Ensure consistent spacing
                 string pieceStr = getPieceVisual(board[i][j]);
                 string padding_before = " ";
                 string padding_after = (pieceStr.length() > 1 || pieceStr == " ") ? " " : "  ";
                 cout << padding_before << pieceStr << padding_after;

                if (USE_ANSI_COLORS) cout << ANSI_RESET;
            }
            cout << "| " << (8 - i);

            if (i == 0) cout << "    Last Move: " << lastMoveNotation;
            if (i == 2) {
                cout << "    " << (isWhiteTurn ? ">>> White's Turn (You)" : ">>> Black's Turn (AI)");
                if (isKingInCheck(isWhiteTurn)) { cout << " (CHECK!)"; }
            }
            if (i == 4 && isWhiteTurn) cout << "    Enter move below";
            if (i == 5 && isWhiteTurn) cout << "    (e.g., e2e4)";
            if (i == 4 && !isWhiteTurn) cout << "    AI is thinking...";
            cout << endl;
        } // End row loop

        cout << "     +--------------------------------+" << endl;
        cout << "       a   b   c   d   e   f   g   h" << endl; // File letters
        cout << "   Captured by Black: "; for (char p : blackCaptured) cout << getPieceVisual(p) << " "; cout << endl;

        // Legend (Unchanged)
        cout << "----------- Legend -----------" << endl;
        if (USE_UNICODE_SYMBOLS) { cout << " White: P"<<getPieceVisual('P')<<" R"<<getPieceVisual('R')<<" N"<<getPieceVisual('N')<<" B"<<getPieceVisual('B')<<" Q"<<getPieceVisual('Q')<<" K"<<getPieceVisual('K') << endl << " Black: p"<<getPieceVisual('p')<<" r"<<getPieceVisual('r')<<" n"<<getPieceVisual('n')<<" b"<<getPieceVisual('b')<<" q"<<getPieceVisual('q')<<" k"<<getPieceVisual('k') << endl; }
        else { cout << " White: P=Pawn R=Rook N=Knight B=Bishop Q=Queen K=King" << endl << " Black: p=Pawn r=Rook n=Knight b=Bishop q=Queen k=King" << endl; }
        cout << "   " << (USE_UNICODE_SYMBOLS ? "' '" : ".") << " = Empty Square" << endl;
        cout << "-----------------------------" << endl;
    }

    // Performs the move actions on the board
    void makeMove(int startR, int startC, int endR, int endC) {
        char pieceMoved = board[startR][startC];
        char capturedPiece = board[endR][endC];

        // Record capture
        if (capturedPiece != '.' && capturedPiece != ' ') {
            if (isPieceWhite(capturedPiece)) { // Black captured White
                blackCaptured.push_back(capturedPiece);
            } else {
                whiteCaptured.push_back(capturedPiece);
            }
        }

        // Move piece
        board[endR][endC] = pieceMoved;
        board[startR][startC] = '.'; // Mark origin as empty

        // Update King's position if King moved
        if (tolower(pieceMoved) == 'k') {
            if (isWhiteTurn) {
                whiteKingRow = endR; whiteKingCol = endC;
            } else {
                blackKingRow = endR; blackKingCol = endC;
            }
        }

        // Update last move notation
        lastMoveNotation = indexToNotation(startR, startC);
        lastMoveNotation += (capturedPiece != '.' && capturedPiece != ' ') ? "x" : "-"; // Capture notation
        lastMoveNotation += indexToNotation(endR, endC);

        // Check if the move puts the *opponent* in check
        bool opponentInCheck = isKingInCheck(!isWhiteTurn);
        if (opponentInCheck) {
            lastMoveNotation += "+"; // Check notation
        }


        // Switch turns
        isWhiteTurn = !isWhiteTurn;
     }


    // Main game loop
    void play() {
         string input; string errorMsg = "";
         bool gameOver = false;

         while (!gameOver) {
             printBoard(); // Print the board at the start of the turn


             if (!errorMsg.empty()) {
                 cout << " (!) Invalid Move: " << errorMsg << endl;
                 errorMsg = ""; // Clear error after displaying
             }

             // Check for game end conditions *before* asking for move
             // (Check if the current player has any valid moves)
             vector<Move> availableMoves = generateValidMoves();
             if (availableMoves.empty()) {
                 if (isKingInCheck(isWhiteTurn)) {
                     cout << "CHECKMATE! " << (isWhiteTurn ? "Black (AI)" : "White (You)") << " wins!" << endl;
                 } else {
                     cout << "STALEMATE! It's a draw." << endl;
                 }
                 gameOver = true;
                 break;
             }


             if (isWhiteTurn) { // Human Player's Turn
                 cout << " Enter move (e.g. e2e4), 'resign', or 'exit': ";
                 cin >> input;

                 if (input == "exit") {
                     cout << " Exiting game." << endl;
                     gameOver = true;
                     break;
                 }
                 if (input == "resign") {
                     cout << "White resigns. Black (AI) wins!" << endl;
                     gameOver = true;
                     break;
                 }
                 if (input.length() != 4) {
                     errorMsg = "Input must be 4 chars (e.g., e2e4).";
                     cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                     continue;
                 }

                 int startR, startC, endR, endC;
                 string startN = input.substr(0, 2);
                 string endN = input.substr(2, 2);

                 if (!notationToIndex(startN, startR, startC)) {
                     errorMsg = "Invalid start square notation: '" + startN + "'.";
                     continue;
                 }
                 if (!notationToIndex(endN, endR, endC)) {
                     errorMsg = "Invalid end square notation: '" + endN + "'.";
                     continue;
                 }

                 if (isMoveValid(startR, startC, endR, endC, errorMsg)) {
                     makeMove(startR, startC, endR, endC);
                 } else {
                     cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                     continue;
                 }

             } else { // AI Player's Turn (Black)
                if (AI_THINKING_MS > 0) {
                     this_thread::sleep_for(chrono::milliseconds(AI_THINKING_MS));
                }

                if (!makeAIMove()) {
                    // This case should be caught by the check at the start of the loop,
                    // but we keep it as a safeguard. makeAIMove itself returns bool.
                     if (isKingInCheck(false)) { // Check if Black King is in check
                         cout << "CHECKMATE! White (You) wins!" << endl;
                     } else {
                         cout << "STALEMATE! It's a draw." << endl;
                     }
                     gameOver = true;
                     break;
                }
                 // Turn is switched inside makeMove called by makeAIMove
             }
         } // End game loop


         if (gameOver) {
             printBoard();
             cout << "Game Over." << endl;
         }
     }

};


void printInstructions() {
     cout << "============================== HOW TO PLAY ==============================" << endl;
     cout << " Objective: Checkmate the opponent's King." << endl;
     cout << " You play as White. The AI plays as Black." << endl << endl;
     cout << " Input Format: Use algebraic notation (e.g., 'e2e4' moves the" << endl;
     cout << "               piece at e2 to e4)." << endl << endl;
     cout << " Commands:" << endl;
     cout << "            - <move> (e.g., e2e4): Make a move." << endl;
     cout << "            - resign: Forfeit the game." << endl;
     cout << "            - exit: Quit the program." << endl;
     cout << "=========================================================================" << endl << endl;
     cout << "*** NOTE: If the board display looks cut off or misaligned, please ***" << endl;
     cout << "***       make your terminal/console window TALLER and WIDER!      ***" << endl << endl;
     cout << "Press Enter to start the game...";
     // Wait for user to press Enter
     cin.ignore(numeric_limits<streamsize>::max(), '\n');
     // If the previous cin.ignore was removed, add one here
     // cin.get(); // Or use cin.ignore again if preferred
}

int main() {
    // Enable UTF-8 output on Windows
    #ifdef _WIN32
        system("chcp 65001 > null");
    #endif

    printInstructions();

    ChessGame game;
    game.play();

    cout << "Press Enter to exit." << endl;
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear potential leftover input
    cin.get(); // Wait before closing console window

    return 0;
}
