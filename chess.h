#pragma once
#include "includes.h"

class Chess {
public:
  static int getPieceColor(char piece);

  static bool isWhitePiece(char piece);

  static bool isBlackPiece(char piece);

  static std::string describePiece(char piece);

  enum PieceColor { WHITE_PIECE = 0, BLACK_PIECE = 1 };

  enum Player { WHITE_PLAYER = 0, BLACK_PLAYER = 1 };

  enum Side { QUEEN_SIDE = 2, KING_SIDE = 3 };

  enum Direction { HORIZONTAL = 0, VERTICAL, DIAGONAL, L_SHAPE };

  struct Position {
    int row;
    int column;
  };

  struct EnPassant {
    bool isApplied;
    Position PawnCaptured;
  };

  struct Castling {
    bool isApplied;
    Position rookBefore;
    Position rookAfter;
  };

  struct Promotion {
    bool isApplied;
    char pieceAfter;
  };

  struct IntendedMove {
    char piece;
    Position from;
    Position to;
  };

  struct Attacker {
    Position position;
    Direction direction;
  };

  struct UnderAttack {
    bool isUnderAttack;
    int numberOfAttackers;
    Attacker attacker[9]; // maximum theoretical number of attackers
  };

  const char initialBoard[8][8] = {
      {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'},
      {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
      {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
      {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
      {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
      {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
      {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
      {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
  };
};
