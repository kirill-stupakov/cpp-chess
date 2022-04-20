#include "chess.h"
#include "includes.h"
#include "user_interface.h"

int Chess::getPieceColor(char piece) {
  if (isupper(piece)) {
    return WHITE_PIECE;
  } else {
    return BLACK_PIECE;
  }
}

bool Chess::isWhitePiece(char piece) {
  return getPieceColor(piece) == Chess::WHITE_PIECE;
}

bool Chess::isBlackPiece(char piece) {
  return getPieceColor(piece) == Chess::BLACK_PIECE;
}

std::string Chess::describePiece(char piece) {
  std::string description;

  if (isWhitePiece(piece)) {
    description += "White ";
  } else {
    description += "Black ";
  }

  switch (toupper(piece)) {
  case 'P': {
    description += "pawn";
  } break;

  case 'N': {
    description += "knight";
  } break;

  case 'B': {
    description += "bishop";
  } break;

  case 'R': {
    description += "rook";
  } break;

  case 'Q': {
    description += "queen";
  } break;

  default: {
    description += "unknown piece";
  } break;
  }

  return description;
}
