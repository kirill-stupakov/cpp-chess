#pragma once
#include "includes.h"
#include "chess.h"

class Game : Chess {
public:
  Game();
  ~Game();

  void movePiece(Position present, Position future,
                 Chess::EnPassant *enPassant, Chess::Castling *castling,
                 Chess::Promotion *promotion);

  bool castlingAllowed(Side side, int color);

  char getPieceAtPosition(int row, int column);

  char getPieceAtPosition(Position pos);

  char getPiece_considerMove(int row, int column,
                             IntendedMove *intendedMove = nullptr);

  UnderAttack isUnderAttack(int row, int column, int color,
                            IntendedMove *intendedMove = nullptr);

  bool isReachable(int row, int column, int color);

  bool isSquareOccupied(int row, int column);

  bool isPathFree(Position startingPos, Position finishingPos, int direction);

  bool canBeBlocked(Position startingPos, Position finishingPos, int direction);

  bool isCheckMate();

  bool isKingInCheck(int color, IntendedMove *intendedMove = nullptr);

  bool playerKingInCheck(IntendedMove *intendedMove = nullptr);

  bool wouldKingBeInCheck(char piece, Position present, Position future,
                          EnPassant *enPassant);

  Position findKing(int iColor);

  void changeTurns();

  bool isFinished() const;

  int getCurrentTurn() const;

  int getOpponentColor() const;

  static void parseMove(string move, Position *form, Position *to,
                        char *promoted = nullptr);

  void logMove(std::string &toRecord);

  string getLastMove();

  static bool isMoveValid(Game*currentGame, Chess::Position present, Chess::Position future,
                          Chess::EnPassant *enPassant, Chess::Castling *castling,
                          Chess::Promotion *promotion);

  static void movePiece(Game *current_game);

  static void makeMove(Game *current_game, Position present, Position future,
                       EnPassant *S_enPassant, Castling *S_castling,
                       Promotion *S_promotion);

  // Save all the moves
  struct Round {
    string whiteMove;
    string blackMove;
  };

  // std::deque<std::string> moves;
  std::deque<Round> rounds;

  // Save the captured pieces
  std::vector<char> whiteCaptured;
  std::vector<char> blackCaptured;

private:
  // Represent the pieces in the board
  char board[8][8]{};

  // Castling requirements
  bool isCastlingKingSideAllowed[2]{};
  bool isCastlingQueenSideAllowed[2]{};

  // Holds the current turn
  int currentTurn;

  // Has the game finished already?
  bool isGameFinished;
};
