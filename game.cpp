#include "game.h"
#include "user_interface.h"

// Game class
Game::Game() {
  // White player always starts
  currentTurn = WHITE_PLAYER;

  // Game on!
  isGameFinished = false;

  // Initial board settings
  memcpy(board, initialBoard, sizeof(char) * 8 * 8);

  // Castling is allowed (to each side) until the player moves the king or the
  // rook
  isCastlingKingSideAllowed[WHITE_PLAYER] = true;
  isCastlingKingSideAllowed[BLACK_PLAYER] = true;

  isCastlingQueenSideAllowed[WHITE_PLAYER] = true;
  isCastlingQueenSideAllowed[BLACK_PLAYER] = true;
}

Game::~Game() {
  whiteCaptured.clear();
  blackCaptured.clear();
  rounds.clear();
}

void Game::movePiece(Position present, Position future,
                     Chess::EnPassant *enPassant, Chess::Castling *castling,
                     Chess::Promotion *promotion) {
  // Get the piece to be moved
  char chPiece = getPieceAtPosition(present);

  // Is the destination square occupied?
  char chCapturedPiece = getPieceAtPosition(future);

  // So, was a piece captured in this move?
  if (0x20 != chCapturedPiece) {
    if (WHITE_PIECE == getPieceColor(chCapturedPiece)) {
      // A white piece was captured
      whiteCaptured.push_back(chCapturedPiece);
    } else {
      // A black piece was captured
      blackCaptured.push_back(chCapturedPiece);
    }
  } else if (enPassant->isApplied) {
    char chCapturedEP = getPieceAtPosition(enPassant->PawnCaptured.row,
                                           enPassant->PawnCaptured.column);

    if (WHITE_PIECE == getPieceColor(chCapturedEP)) {
      // A white piece was captured
      whiteCaptured.push_back(chCapturedEP);
    } else {
      // A black piece was captured
      blackCaptured.push_back(chCapturedEP);
    }

    // Now, remove the captured pawn
    board[enPassant->PawnCaptured.row][enPassant->PawnCaptured.column] =
        EMPTY_SQUARE;
  }

  // Remove piece from present position
  board[present.row][present.column] = EMPTY_SQUARE;

  // Move piece to new position
  if (promotion->isApplied) {
    board[future.row][future.column] = promotion->pieceAfter;
  } else {
    board[future.row][future.column] = chPiece;
  }

  // Was it a castling move?
  if (castling->isApplied) {
    // The king was already move, but we still have to move the rook to 'jump'
    // the king
    char chPiece = getPieceAtPosition(castling->rookBefore.row,
                                      castling->rookBefore.column);

    // Remove the rook from present position
    board[castling->rookBefore.row][castling->rookBefore.column] =
        EMPTY_SQUARE;

    // 'Jump' into to new position
    board[castling->rookAfter.row][castling->rookAfter.column] =
        chPiece;
  }

  // Castling requirements
  if ('K' == toupper(chPiece)) {
    // After the king has moved once, no more castling allowed
    isCastlingKingSideAllowed[getCurrentTurn()] = false;
    isCastlingQueenSideAllowed[getCurrentTurn()] = false;
  } else if ('R' == toupper(chPiece)) {
    // If the rook moved from column 'A', no more castling allowed on the queen
    // side
    if (0 == present.column) {
      isCastlingQueenSideAllowed[getCurrentTurn()] = false;
    }

    // If the rook moved from column 'A', no more castling allowed on the queen
    // side
    else if (7 == present.column) {
      isCastlingKingSideAllowed[getCurrentTurn()] = false;
    }
  }

  // Change turns
  changeTurns();
}

bool Game::castlingAllowed(Side side, int color) {
  if (QUEEN_SIDE == side) {
    return isCastlingQueenSideAllowed[color];
  } else // if ( KING_SIDE == side )
  {
    return isCastlingKingSideAllowed[color];
  }
}

char Game::getPieceAtPosition(int row, int column) {
  return board[row][column];
}

char Game::getPieceAtPosition(Position pos) {
  return board[pos.row][pos.column];
}

char Game::getPiece_considerMove(int row, int column,
                                 IntendedMove *intendedMove) {
  char chPiece;

  // If there is no intended move, just return the current position of the board
  if (nullptr == intendedMove) {
    chPiece = getPieceAtPosition(row, column);
  } else {
    // In this case, we are trying to understand what WOULD happen if the move
    // was made, so we consider a move that has not been made yet
    if (intendedMove->from.row == row && intendedMove->from.column == column) {
      // The piece wants to move from that square, so it would be empty
      chPiece = EMPTY_SQUARE;
    } else if (intendedMove->to.row == row &&
               intendedMove->to.column == column) {
      // The piece wants to move to that square, so return the piece
      chPiece = intendedMove->piece;
    } else {
      chPiece = getPieceAtPosition(row, column);
    }
  }

  return chPiece;
}

Chess::UnderAttack Game::isUnderAttack(int row, int column, int color,
                                       IntendedMove *intendedMove) {
  UnderAttack attack = {false};

  // a) Direction: HORIZONTAL
  {
    // Check all the way to the right
    for (int i = column + 1; i < 8; i++) {
      char chPieceFound = getPiece_considerMove(row, i, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook to the right, so the piece is in jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = row;
        attack.attacker[attack.numberOfAttackers - 1].position.column = i;
        attack.attacker[attack.numberOfAttackers - 1].direction = HORIZONTAL;
        break;
      } else {
        // There is a piece that does not attack horizontally, so no problem
        break;
      }
    }

    // Check all the way to the left
    for (int i = column - 1; i >= 0; i--) {
      char chPieceFound = getPiece_considerMove(row, i, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook to the right, so the piece is in jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = row;
        attack.attacker[attack.numberOfAttackers - 1].position.column = i;
        attack.attacker[attack.numberOfAttackers - 1].direction = HORIZONTAL;
        break;
      } else {
        // There is a piece that does not attack horizontally, so no problem
        break;
      }
    }
  }

  // b) Direction: VERTICAL
  {
    // Check all the way up
    for (int i = row + 1; i < 8; i++) {
      char chPieceFound = getPiece_considerMove(i, column, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook to the right, so the piece is in jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = column;
        attack.attacker[attack.numberOfAttackers - 1].direction = VERTICAL;
        break;
      } else {
        // There is a piece that does not attack vertically, so no problem
        break;
      }
    }

    // Check all the way down
    for (int i = row - 1; i >= 0; i--) {
      char chPieceFound = getPiece_considerMove(i, column, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook to the right, so the piece is in jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = column;
        attack.attacker[attack.numberOfAttackers - 1].direction = VERTICAL;
        break;
      } else {
        // There is a piece that does not attack vertically, so no problem
        break;
      }
    }
  }

  // c) Direction: DIAGONAL
  {
    // Check the diagonal up-right
    for (int i = row + 1, j = column + 1; i < 8 && j < 8; i++, j++) {
      char chPieceFound = getPiece_considerMove(i, j, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'P') && (i == row + 1) &&
                 (j == column + 1) && (color == WHITE_PIECE)) {
        // A pawn only puts another piece in jeopardy if it's (diagonally) right
        // next to it
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the piece is in
        // jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else {
        // There is a piece that does not attack diagonally, so no problem
        break;
      }
    }

    // Check the diagonal up-left
    for (int i = row + 1, j = column - 1; i < 8 && j > 0; i++, j--) {
      char chPieceFound = getPiece_considerMove(i, j, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'P') && (i == row + 1) &&
                 (j == column - 1) && (color == WHITE_PIECE)) {
        // A pawn only puts another piece in jeopardy if it's (diagonally) right
        // next to it
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the piece is in
        // jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else {
        // There is a piece that does not attack diagonally, so no problem
        break;
      }
    }

    // Check the diagonal down-right
    for (int i = row - 1, j = column + 1; i > 0 && j < 8; i--, j++) {
      char chPieceFound = getPiece_considerMove(i, j, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'P') && (i == row - 1) &&
                 (j == column + 1) && (color == BLACK_PIECE)) {
        // A pawn only puts another piece in jeopardy if it's (diagonally) right
        // next to it
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the piece is in
        // jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else {
        // There is a piece that does not attack diagonally, so no problem
        break;
      }
    }

    // Check the diagonal down-left
    for (int i = row - 1, j = column - 1; i > 0 && j > 0; i--, j--) {
      char chPieceFound = getPiece_considerMove(i, j, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        break;
      } else if ((toupper(chPieceFound) == 'P') && (i == row - 1) &&
                 (j == column - 1) && (color == BLACK_PIECE)) {
        // A pawn only puts another piece in jeopardy if it's (diagonally) right
        // next to it
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the piece is in
        // jeopardy
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = i;
        attack.attacker[attack.numberOfAttackers - 1].position.column = j;
        attack.attacker[attack.numberOfAttackers - 1].direction = DIAGONAL;
        break;
      } else {
        // There is a piece that does not attack diagonally, so no problem
        break;
      }
    }
  }

  // d) Direction: L_SHAPED
  {
    // Check if the piece is put in jeopardy by a knigh

    Position knight_moves[8] = {{1, -2},  {2, -1},  {2, 1},  {1, 2},
                                {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    for (auto &knight_move : knight_moves) {
      int iRowToTest = row + knight_move.row;
      int iColumnToTest = column + knight_move.column;

      if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 ||
          iColumnToTest > 7) {
        // This square does not even exist, so no need to test
        continue;
      }

      char chPieceFound =
          getPiece_considerMove(iRowToTest, iColumnToTest, intendedMove);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color, so no problem
        continue;
      } else if ((toupper(chPieceFound) == 'N')) {
        attack.isUnderAttack = true;
        attack.numberOfAttackers += 1;

        attack.attacker[attack.numberOfAttackers - 1].position.row = iRowToTest;
        attack.attacker[attack.numberOfAttackers - 1].position.column = iColumnToTest;
        break;
      }
    }
  }

  return attack;
}

bool Game::isReachable(int row, int column, int color) {
  bool bReachable = false;

  // a) Direction: HORIZONTAL
  {
    // Check all the way to the right
    for (int i = column + 1; i < 8; i++) {
      char chPieceFound = getPieceAtPosition(row, i);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook to the right, so the square is reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move horizontally
        break;
      }
    }

    // Check all the way to the left
    for (int i = column - 1; i >= 0; i--) {
      char chPieceFound = getPieceAtPosition(row, i);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook to the left, so the square is reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move horizontally
        break;
      }
    }
  }

  // b) Direction: VERTICAL
  {
    // Check all the way up
    for (int i = row + 1; i < 8; i++) {
      char chPieceFound = getPieceAtPosition(i, column);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'P') &&
                 (getPieceColor(chPieceFound) == BLACK_PIECE) &&
                 (i == row + 1)) {
        // There is a pawn one square up, so the square is reachable
        bReachable = true;
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook on the way up, so the square is reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move vertically
        break;
      }
    }

    // Check all the way down
    for (int i = row - 1; i >= 0; i--) {
      char chPieceFound = getPieceAtPosition(i, column);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'P') &&
                 (getPieceColor(chPieceFound) == WHITE_PIECE) &&
                 (i == row - 1)) {
        // There is a pawn one square down, so the square is reachable
        bReachable = true;
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'R')) {
        // There is a queen or a rook on the way down, so the square is
        // reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move vertically
        break;
      }
    }
  }

  // c) Direction: DIAGONAL
  {
    // Check the diagonal up-right
    for (int i = row + 1, j = column + 1; i < 8 && j < 8; i++, j++) {
      char chPieceFound = getPieceAtPosition(i, j);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the square is
        // reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move diagonally
        break;
      }
    }

    // Check the diagonal up-left
    for (int i = row + 1, j = column - 1; i < 8 && j > 0; i++, j--) {
      char chPieceFound = getPieceAtPosition(i, j);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the square is
        // reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move diagonally
        break;
      }
    }

    // Check the diagonal down-right
    for (int i = row - 1, j = column + 1; i > 0 && j < 8; i--, j++) {
      char chPieceFound = getPieceAtPosition(i, j);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the square is
        // reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move diagonally
        break;
      }
    }

    // Check the diagonal down-left
    for (int i = row - 1, j = column - 1; i > 0 && j > 0; i--, j--) {
      char chPieceFound = getPieceAtPosition(i, j);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        break;
      } else if ((toupper(chPieceFound) == 'Q') ||
                 (toupper(chPieceFound) == 'B')) {
        // There is a queen or a bishop in that direction, so the square is
        // reachable
        bReachable = true;
        break;
      } else {
        // There is a piece that does not move diagonally
        break;
      }
    }
  }

  // d) Direction: L_SHAPED
  {
    // Check if the piece is put in jeopardy by a knigh

    Position knight_moves[8] = {{1, -2},  {2, -1},  {2, 1},  {1, 2},
                                {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
    for (auto &knight_move : knight_moves) {
      int iRowToTest = row + knight_move.row;
      int iColumnToTest = column + knight_move.column;

      if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 ||
          iColumnToTest > 7) {
        // This square does not even exist, so no need to test
        continue;
      }

      char chPieceFound = getPieceAtPosition(iRowToTest, iColumnToTest);
      if (EMPTY_SQUARE == chPieceFound) {
        // This square is empty, move on
        continue;
      }

      if (color == getPieceColor(chPieceFound)) {
        // This is a piece of the same color
        continue;
      } else if ((toupper(chPieceFound) == 'N')) {
        bReachable = true;
        break;
      }
    }
  }

  return bReachable;
}

bool Game::isSquareOccupied(int row, int column) {
  bool bOccupied = false;

  if (0x20 != getPieceAtPosition(row, column)) {
    bOccupied = true;
  }

  return bOccupied;
}

bool Game::isPathFree(Position startingPos, Position finishingPos,
                      int direction) {
  bool bFree = false;

  switch (direction) {
  case Chess::HORIZONTAL: {
    // If it is a horizontal move, we can assume the startingPos.row ==
    // finishingPos.row If the piece wants to move from column 0 to column 7,
    // we must check if columns 1-6 are free
    if (startingPos.column == finishingPos.column) {
      cout << "Error. Movement is horizontal but column is the same\n";
    }

    // Moving to the right
    else if (startingPos.column < finishingPos.column) {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = startingPos.column + 1; i < finishingPos.column; i++) {
        if (isSquareOccupied(startingPos.row, i)) {
          bFree = false;
          cout << "Horizontal path to the right is not clear!\n";
        }
      }
    }

    // Moving to the left
    else // if (startingPos.column > finishingPos.column)
    {
      // Setting bFree as initially true, only inside the cases, guarantees that
      // the path is checked
      bFree = true;

      for (int i = startingPos.column - 1; i > finishingPos.column; i--) {
        if (isSquareOccupied(startingPos.row, i)) {
          bFree = false;
          cout << "Horizontal path to the left is not clear!\n";
        }
      }
    }
  } break;

  case Chess::VERTICAL: {
    // If it is a vertical move, we can assume the startingPos.column ==
    // finishingPos.column If the piece wants to move from column 0 to column
    // 7, we must check if columns 1-6 are free
    if (startingPos.row == finishingPos.row) {
      cout << "Error. Movement is vertical but row is the same\n";
      throw std::runtime_error(
          "Error. Movement is vertical but row is the same");
    }

    // Moving up
    else if (startingPos.row < finishingPos.row) {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = startingPos.row + 1; i < finishingPos.row; i++) {
        if (isSquareOccupied(i, startingPos.column)) {
          bFree = false;
          cout << "Vertical path up is not clear!\n";
        }
      }
    }

    // Moving down
    else // if (startingPos.column > finishingPos.row)
    {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = startingPos.row - 1; i > finishingPos.row; i--) {
        if (isSquareOccupied(i, startingPos.column)) {
          bFree = false;
          cout << "Vertical path down is not clear!\n";
        }
      }
    }
  } break;

  case Chess::DIAGONAL: {
    // Moving up and right
    if ((finishingPos.row > startingPos.row) &&
        (finishingPos.column > startingPos.column)) {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isSquareOccupied(startingPos.row + i, startingPos.column + i)) {
          bFree = false;
          cout << "Diagonal path up-right is not clear!\n";
        }
      }
    }

    // Moving up and left
    else if ((finishingPos.row > startingPos.row) &&
             (finishingPos.column < startingPos.column)) {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isSquareOccupied(startingPos.row + i, startingPos.column - i)) {
          bFree = false;
          cout << "Diagonal path up-left is not clear!\n";
        }
      }
    }

    // Moving down and right
    else if ((finishingPos.row < startingPos.row) &&
             (finishingPos.column > startingPos.column)) {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isSquareOccupied(startingPos.row - i, startingPos.column + i)) {
          bFree = false;
          cout << "Diagonal path down-right is not clear!\n";
        }
      }
    }

    // Moving down and left
    else if ((finishingPos.row < startingPos.row) &&
             (finishingPos.column < startingPos.column)) {
      // Settting bFree as initially true, only inside the cases, guarantees
      // that the path is checked
      bFree = true;

      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isSquareOccupied(startingPos.row - i, startingPos.column - i)) {
          bFree = false;
          cout << "Diagonal path down-left is not clear!\n";
        }
      }
    }

    else {
      throw std::runtime_error("Error. Diagonal move not allowed");
    }
  } break;
  }

  return bFree;
}

bool Game::canBeBlocked(Position startingPos, Position finishingPos,
                        int direction) {
  bool bBlocked = false;

  Chess::UnderAttack blocker = {0};

  switch (direction) {
  case Chess::HORIZONTAL: {
    // If it is a horizontal move, we can assume the startingPos.row ==
    // finishingPos.row If the piece wants to move from column 0 to column 7,
    // we must check if columns 1-6 are free
    if (startingPos.column == finishingPos.column) {
      cout << "Error. Movement is horizontal but column is the same\n";
    }

    // Moving to the right
    else if (startingPos.column < finishingPos.column) {
      for (int i = startingPos.column + 1; i < finishingPos.column; i++) {
        if (isReachable(startingPos.row, i, getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }

    // Moving to the left
    else // if (startingPos.column > finishingPos.column)
    {
      for (int i = startingPos.column - 1; i > finishingPos.column; i--) {
        if (isReachable(startingPos.row, i, getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }
  } break;

  case Chess::VERTICAL: {
    // If it is a vertical move, we can assume the startingPos.column ==
    // finishingPos.column If the piece wants to move from column 0 to column
    // 7, we must check if columns 1-6 are free
    if (startingPos.row == finishingPos.row) {
      cout << "Error. Movement is vertical but row is the same\n";
      throw std::runtime_error(
          "Error. Movement is vertical but row is the same");
    }

    // Moving up
    else if (startingPos.row < finishingPos.row) {
      for (int i = startingPos.row + 1; i < finishingPos.row; i++) {
        if (isReachable(i, startingPos.column, getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }

    // Moving down
    else // if (startingPos.column > finishingPos.row)
    {
      for (int i = startingPos.row - 1; i > finishingPos.row; i--) {
        if (isReachable(i, startingPos.column, getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }
  } break;

  case Chess::DIAGONAL: {
    // Moving up and right
    if ((finishingPos.row > startingPos.row) &&
        (finishingPos.column > startingPos.column)) {
      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isReachable(startingPos.row + i, startingPos.column + i,
                        getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }

    // Moving up and left
    else if ((finishingPos.row > startingPos.row) &&
             (finishingPos.column < startingPos.column)) {
      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isReachable(startingPos.row + i, startingPos.column - i,
                        getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }

    // Moving down and right
    else if ((finishingPos.row < startingPos.row) &&
             (finishingPos.column > startingPos.column)) {
      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isReachable(startingPos.row - i, startingPos.column + i,
                        getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }

    // Moving down and left
    else if ((finishingPos.row < startingPos.row) &&
             (finishingPos.column < startingPos.column)) {
      for (int i = 1; i < abs(finishingPos.row - startingPos.row); i++) {
        if (isReachable(startingPos.row - i, startingPos.column - i,
                        getOpponentColor())) {
          // Some piece can block the way
          bBlocked = true;
        }
      }
    }

    else {
      cout << "Error. Diagonal move not allowed\n";
      throw std::runtime_error("Error. Diagonal move not allowed");
    }
  } break;
  }

  return bBlocked;
}

bool Game::isCheckMate() {
  bool bCheckmate = false;

  // 1. First of all, it the king in check?
  if (!playerKingInCheck()) {
    return false;
  }

  // 2. Can the king move the other square?
  Chess::Position king_moves[8] = {{1, -1}, {1, 0},  {1, 1},   {0, 1},
                                   {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}};

  Chess::Position king = findKing(getCurrentTurn());

  for (auto &king_move : king_moves) {
    int iRowToTest = king.row + king_move.row;
    int iColumnToTest = king.column + king_move.column;

    if (iRowToTest < 0 || iRowToTest > 7 || iColumnToTest < 0 ||
        iColumnToTest > 7) {
      // This square does not even exist, so no need to test
      continue;
    }

    if (EMPTY_SQUARE != getPieceAtPosition(iRowToTest, iColumnToTest)) {
      // That square is not empty, so no need to test
      continue;
    }

    Chess::IntendedMove intended_move;
    intended_move.piece = getPieceAtPosition(king.row, king.column);
    intended_move.from.row = king.row;
    intended_move.from.column = king.column;
    intended_move.to.row = iRowToTest;
    intended_move.to.column = iColumnToTest;

    // Now, for every possible move of the king, check if it would be in
    // jeopardy Since the move has already been made,
    // currentGame->getCurrentTurn() now will return the next player's color.
    // And it is in fact this king that we want to check for jeopardy
    Chess::UnderAttack king_moved = isUnderAttack(
        iRowToTest, iColumnToTest, getCurrentTurn(), &intended_move);

    if (!king_moved.isUnderAttack) {
      // This means there is at least one position when the king would not be in
      // jeopardy, so that's not a checkmate
      return false;
    }
  }

  // 3. Can the attacker be taken or another piece get into the way?
  Chess::UnderAttack king_attacked =
      isUnderAttack(king.row, king.column, getCurrentTurn());
  if (1 == king_attacked.numberOfAttackers) {
    // Can the attacker be taken?
    Chess::UnderAttack king_attacker = {0};
    king_attacker = isUnderAttack(king_attacked.attacker[0].position.row,
                                  king_attacked.attacker[0].position.column,
                                  getOpponentColor());

    if (king_attacker.isUnderAttack) {
      // This means that the attacker can be taken, so it's not a checkmate
      return false;
    } else {
      // Last resort: can any piece get in between the attacker and the king?
      char chAttacker =
          getPieceAtPosition(king_attacked.attacker[0].position.row,
                             king_attacked.attacker[0].position.column);

      switch (toupper(chAttacker)) {
      case 'P':
      case 'N': {
        // If it's a pawn, there's no space in between the attacker and the king
        // If it's a knight, it doesn't matter because the knight can 'jump'
        // So, this is checkmate
        bCheckmate = true;
      } break;

      case 'B':
      case 'R': {
        if (!canBeBlocked(king_attacked.attacker[0].position, king,
                          Chess::DIAGONAL)) {
          // If no piece can get in the way, it's a checkmate
          bCheckmate = true;
        }
      } break;

      case 'Q': {
        if (!canBeBlocked(king_attacked.attacker[0].position, king,
                          king_attacked.attacker[0].direction)) {
          // If no piece can get in the way, it's a checkmate
          bCheckmate = true;
        }
      } break;

      default: {
        throw std::runtime_error("!!!!Should not reach here. Invalid piece");
      } break;
      }
    }
  } else {
    // If there is more than one attacker and we reached this point, it's a
    // checkmate
    bCheckmate = true;
  }

  // If the game has ended, store in the class variable
  isGameFinished = bCheckmate;

  return bCheckmate;
}

bool Game::isKingInCheck(int color, IntendedMove *intendedMove) {
  bool bCheck = false;

  Position king = {0};

  // Must check if the intended move is to move the king itself
  if (nullptr != intendedMove && 'K' == toupper(intendedMove->piece)) {
    king.row = intendedMove->to.row;
    king.column = intendedMove->to.column;
  } else {
    king = findKing(color);
  }

  UnderAttack king_attacked =
      isUnderAttack(king.row, king.column, color, intendedMove);

  if (king_attacked.isUnderAttack) {
    bCheck = true;
  }

  return bCheck;
}

bool Game::playerKingInCheck(IntendedMove *intendedMove) {
  return isKingInCheck(getCurrentTurn(), intendedMove);
}

bool Game::wouldKingBeInCheck(char piece, Position present, Position future,
                              EnPassant *enPassant) {
  IntendedMove intended_move;

  intended_move.piece = piece;
  intended_move.from.row = present.row;
  intended_move.from.column = present.column;
  intended_move.to.row = future.row;
  intended_move.to.column = future.column;

  return playerKingInCheck(&intended_move);
}

Chess::Position Game::findKing(int iColor) {
  char chToLook = (WHITE_PIECE == iColor) ? 'K' : 'k';
  Position king = {0};

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (chToLook == getPieceAtPosition(i, j)) {
        king.row = i;
        king.column = j;
      }
    }
  }

  return king;
}

void Game::changeTurns() {
  if (WHITE_PLAYER == currentTurn) {
    currentTurn = BLACK_PLAYER;
  } else {
    currentTurn = WHITE_PLAYER;
  }
}

bool Game::isFinished() const { return isGameFinished; }

int Game::getCurrentTurn() const { return currentTurn; }

int Game::getOpponentColor() const {
  int iColor;

  if (Chess::WHITE_PLAYER == getCurrentTurn()) {
    iColor = Chess::BLACK_PLAYER;
  } else {
    iColor = Chess::WHITE_PLAYER;
  }

  return iColor;
}

void Game::parseMove(string move, Position *form, Position *to,
                     char *promoted) {
  form->column = move[0];
  form->row = move[1];
  to->column = move[3];
  to->row = move[4];

  // Convert columns from ['A'-'H'] to [0x00-0x07]
  form->column = form->column - 'A';
  to->column = to->column - 'A';

  // Convert row from ['1'-'8'] to [0x00-0x07]
  form->row = form->row - '1';
  to->row = to->row - '1';

  if (promoted != nullptr) {
    if (move[5] == '=') {
      *promoted = move[6];
    } else {
      *promoted = EMPTY_SQUARE;
    }
  }
}

void Game::logMove(std::string &toRecord) {
  if (toRecord.length() == 5) {
    toRecord += "  ";
  }

  if (WHITE_PLAYER == getCurrentTurn()) {
    // If this was a white player move, create a new round and leave the
    // blackMove empty
    Round round;
    round.whiteMove = toRecord;
    round.blackMove = "";

    rounds.push_back(round);
  } else {
    // If this was a blackMove, just update the last Round
    Round round = rounds[rounds.size() - 1];
    round.blackMove = toRecord;

    // Remove the last round and put it back, now with the black move
    rounds.pop_back();
    rounds.push_back(round);
  }
}

string Game::getLastMove() {
  string last_move;

  // Who did the last move?
  if (BLACK_PLAYER == getCurrentTurn()) {
    // If it's black's turn now, white had the last move
    last_move = rounds[rounds.size() - 1].whiteMove;
  } else {
    // Last move was black's
    last_move = rounds[rounds.size() - 1].blackMove;
  }

  return last_move;
}
bool Game::isMoveValid(Game*currentGame, Chess::Position present, Chess::Position future,
                       Chess::EnPassant *enPassant, Chess::Castling *castling,
                       Chess::Promotion *promotion) {
  bool isValid = false;

  char chPiece = currentGame->getPieceAtPosition(present.row, present.column);

  // Is the piece  allowed to move in that direction?
  switch (toupper(chPiece)) {
  case 'P': {
    // Wants to move forward
    if (future.column == present.column) {
      // Simple move forward
      if ((Chess::isWhitePiece(chPiece) && future.row == present.row + 1) ||
          (Chess::isBlackPiece(chPiece) && future.row == present.row - 1)) {
        if (EMPTY_SQUARE ==
            currentGame->getPieceAtPosition(future.row, future.column)) {
          isValid = true;
        }
      }

      // Double move forward
      else if ((Chess::isWhitePiece(chPiece) &&
                future.row == present.row + 2) ||
               (Chess::isBlackPiece(chPiece) &&
                future.row == present.row - 2)) {
        // This is only allowed if the pawn is in its original place
        if (Chess::isWhitePiece(chPiece)) {
          if (EMPTY_SQUARE == currentGame->getPieceAtPosition(
                                  future.row - 1, future.column) &&
              EMPTY_SQUARE ==
                  currentGame->getPieceAtPosition(
                      future.row, future.column) &&
              1 == present.row) {
            isValid = true;
          }
        } else // if ( isBlackPiece(piece) )
        {
          if (EMPTY_SQUARE == currentGame->getPieceAtPosition(
                                  future.row + 1, future.column) &&
              EMPTY_SQUARE ==
                  currentGame->getPieceAtPosition(
                      future.row, future.column) &&
              6 == present.row) {
            isValid = true;
          }
        }
      } else {
        // This is invalid
        return false;
      }
    }

    // The "en passant" move
    else if ((Chess::isWhitePiece(chPiece) && 4 == present.row &&
              5 == future.row && 1 == abs(future.column - present.column)) ||
             (Chess::isBlackPiece(chPiece) && 3 == present.row &&
              2 == future.row && 1 == abs(future.column - present.column))) {
      // It is only valid if last move of the opponent was a double move forward
      // by a pawn on a adjacent column
      string last_move = currentGame->getLastMove();

      // Parse the line
      Chess::Position LastMoveFrom;
      Chess::Position LastMoveTo;
      currentGame->parseMove(last_move, &LastMoveFrom, &LastMoveTo);

      // First of all, was it a pawn?
      char chLstMvPiece =
          currentGame->getPieceAtPosition(LastMoveTo.row, LastMoveTo.column);

      if (toupper(chLstMvPiece) != 'P') {
        return false;
      }

      // Did the pawn have a double move forward and was it an adjacent column?
      if (2 == abs(LastMoveTo.row - LastMoveFrom.row) &&
          1 == abs(LastMoveFrom.column - present.column)) {
        cout << "En passant move!\n";
        isValid = true;

        enPassant->isApplied = true;
        enPassant->PawnCaptured.row = LastMoveTo.row;
        enPassant->PawnCaptured.column = LastMoveTo.column;
      }
    }

    // Wants to capture a piece
    else if (1 == abs(future.column - present.column)) {
      if ((Chess::isWhitePiece(chPiece) && future.row == present.row + 1) ||
          (Chess::isBlackPiece(chPiece) && future.row == present.row - 1)) {
        // Only allowed if there is something to be captured in the square
        if (EMPTY_SQUARE !=
            currentGame->getPieceAtPosition(future.row, future.column)) {
          isValid = true;
          cout << "Pawn captured a piece!\n";
        }
      }
    } else {
      // This is invalid
      return false;
    }

    // If a pawn reaches its eight rank, it must be promoted to another piece
    if ((Chess::isWhitePiece(chPiece) && 7 == future.row) ||
        (Chess::isBlackPiece(chPiece) && 0 == future.row)) {
      cout << "Pawn must be promoted!\n";
      promotion->isApplied = true;
    }
  } break;

  case 'R': {
    // Horizontal move
    if ((future.row == present.row) && (future.column != present.column)) {
      // Check if there are no pieces on the way
      if (currentGame->isPathFree(present, future, Chess::HORIZONTAL)) {
        isValid = true;
      }
    }
    // Vertical move
    else if ((future.row != present.row) &&
             (future.column == present.column)) {
      // Check if there are no pieces on the way
      if (currentGame->isPathFree(present, future, Chess::VERTICAL)) {
        isValid = true;
      }
    }
  } break;

  case 'N': {
    if ((2 == abs(future.row - present.row)) &&
        (1 == abs(future.column - present.column))) {
      isValid = true;
    }

    else if ((1 == abs(future.row - present.row)) &&
             (2 == abs(future.column - present.column))) {
      isValid = true;
    }
  } break;

  case 'B': {
    // Diagonal move
    if (abs(future.row - present.row) ==
        abs(future.column - present.column)) {
      // Check if there are no pieces on the way
      if (currentGame->isPathFree(present, future, Chess::DIAGONAL)) {
        isValid = true;
      }
    }
  } break;

  case 'Q': {
    // Horizontal move
    if ((future.row == present.row) && (future.column != present.column)) {
      // Check if there are no pieces on the way
      if (currentGame->isPathFree(present, future, Chess::HORIZONTAL)) {
        isValid = true;
      }
    }
    // Vertical move
    else if ((future.row != present.row) &&
             (future.column == present.column)) {
      // Check if there are no pieces on the way
      if (currentGame->isPathFree(present, future, Chess::VERTICAL)) {
        isValid = true;
      }
    }

    // Diagonal move
    else if (abs(future.row - present.row) ==
             abs(future.column - present.column)) {
      // Check if there are no pieces on the way
      if (currentGame->isPathFree(present, future, Chess::DIAGONAL)) {
        isValid = true;
      }
    }
  } break;

  case 'K': {
    // Horizontal move by 1
    if ((future.row == present.row) &&
        (1 == abs(future.column - present.column))) {
      isValid = true;
    }

    // Vertical move by 1
    else if ((future.column == present.column) &&
             (1 == abs(future.row - present.row))) {
      isValid = true;
    }

    // Diagonal move by 1
    else if ((1 == abs(future.row - present.row)) &&
             (1 == abs(future.column - present.column))) {
      isValid = true;
    }

    // Castling
    else if ((future.row == present.row) &&
             (2 == abs(future.column - present.column))) {
      // Castling is only allowed in these circunstances:

      // 1. King is not in check
      if (currentGame->playerKingInCheck()) {
        return false;
      }

      // 2. No pieces in between the king and the rook
      if (!currentGame->isPathFree(present, future, Chess::HORIZONTAL)) {
        return false;
      }

      // 3. King and rook must not have moved yet;
      // 4. King must not pass through a square that is attacked by an enemy
      // piece
      if (future.column > present.column) {
        // if future.column is greather, it means king side
        if (!currentGame->castlingAllowed(Chess::Side::KING_SIDE,
                                          Chess::getPieceColor(chPiece))) {
          createNextMessage("Castling to the king side is not allowed.\n");
          return false;
        } else {
          // Check if the square that the king skips is not under attack
          Chess::UnderAttack square_skipped = currentGame->isUnderAttack(present.row, present.column + 1, currentGame->getCurrentTurn());
          if (!square_skipped.isUnderAttack) {
            // Fill the castling structure
            castling->isApplied = true;

            // Present position of the rook
            castling->rookBefore.row = present.row;
            castling->rookBefore.column = present.column + 3;

            // Future position of the rook
            castling->rookAfter.row = future.row;
            castling->rookAfter.column =
                present.column + 1; // future.column -1

            isValid = true;
          }
        }
      } else {
        // if present.column is greater, it means queen side
        if (!currentGame->castlingAllowed(Chess::Side::QUEEN_SIDE,
                                          Chess::getPieceColor(chPiece))) {
          createNextMessage("Castling to the queen side is not allowed.\n");
          return false;
        } else {
          // Check if the square that the king skips is not attacked
          Chess::UnderAttack square_skipped = currentGame->isUnderAttack(present.row, present.column - 1, currentGame->getCurrentTurn());
          if (!square_skipped.isUnderAttack) {
            // Fill the castling structure
            castling->isApplied = true;

            // Present position of the rook
            castling->rookBefore.row = present.row;
            castling->rookBefore.column = present.column - 4;

            // Future position of the rook
            castling->rookAfter.row = future.row;
            castling->rookAfter.column =
                present.column - 1; // future.column +1

            isValid = true;
          }
        }
      }
    }
  } break;

  default: {
    cout << "!!!!Should not reach here. Invalid piece: " << char(chPiece)
         << "\n\n\n";
  } break;
  }

  // If it is a move in an invalid direction, do not even bother to check the
  // rest
  if (!isValid) {
    cout << "Piece is not allowed to move to that square\n";
    return false;
  }

  // Is there another piece of the same color on the destination square?
  if (currentGame->isSquareOccupied(future.row, future.column)) {
    char chAuxPiece =
        currentGame->getPieceAtPosition(future.row, future.column);
    if (Chess::getPieceColor(chPiece) == Chess::getPieceColor(chAuxPiece)) {
      cout << "Position is already taken by a piece of the same color\n";
      return false;
    }
  }

  // Would the king be in check after the move?
  if (currentGame->wouldKingBeInCheck(chPiece, present, future, enPassant)) {
    cout << "Move would put player's king in check\n";
    return false;
  }

  return isValid;
}

void Game::makeMove(Game *current_game, Chess::Position present, Chess::Position future,
                    Chess::EnPassant *S_enPassant, Chess::Castling *S_castling,
                    Chess::Promotion *S_promotion) {
  char chPiece =
      current_game->getPieceAtPosition(present.row, present.column);

  // Captured a piece?
  if (current_game->isSquareOccupied(future.row, future.column)) {
    char chAuxPiece =
        current_game->getPieceAtPosition(future.row, future.column);

    if (Chess::getPieceColor(chPiece) != Chess::getPieceColor(chAuxPiece)) {
      createNextMessage(Chess::describePiece(chAuxPiece) + " captured!\n");
    } else {
      cout << "Error. We should not be making this move\n";
      throw std::runtime_error("Error. We should not be making this move");
    }
  } else if (S_enPassant->isApplied) {
    createNextMessage("Pawn captured by \"en passant\" move!\n");
  }

  if (S_castling->isApplied) {
    createNextMessage("Castling applied!\n");
  }

  current_game->movePiece(present, future, S_enPassant, S_castling,
                          S_promotion);
}

void Game::movePiece(Game *current_game) {
  std::string to_record;

  // Get user input for the piece they want to move
  cout << "Choose piece to be moved. (example: A1 or b2): ";

  std::string move_from;
  getline(cin, move_from);

  if (move_from.length() > 2) {
    createNextMessage("You should type only two characters (column and row)\n");
    return;
  }

  Chess::Position present;
  present.column = move_from[0];
  present.row = move_from[1];

  present.column = toupper(present.column);

  if (present.column < 'A' || present.column > 'H') {
    createNextMessage("Invalid column.\n");
    return;
  }

  if (present.row < '0' || present.row > '8') {
    createNextMessage("Invalid row.\n");
    return;
  }

  // Put in the string to be logged
  to_record += present.column;
  to_record += present.row;
  to_record += "-";

  // Convert column from ['A'-'H'] to [0x00-0x07]
  present.column = present.column - 'A';

  // Convert row from ['1'-'8'] to [0x00-0x07]
  present.row = present.row - '1';

  char chPiece =
      current_game->getPieceAtPosition(present.row, present.column);
  cout << "Piece is " << char(chPiece) << "\n";

  if (0x20 == chPiece) {
    createNextMessage("You picked an EMPTY square.\n");
    return;
  }

  if (Chess::WHITE_PIECE == current_game->getCurrentTurn()) {
    if (!Chess::isWhitePiece(chPiece)) {
      createNextMessage("It is WHITE's turn and you picked a BLACK piece\n");
      return;
    }
  } else {
    if (!Chess::isBlackPiece(chPiece)) {
      createNextMessage("It is BLACK's turn and you picked a WHITE piece\n");
      return;
    }
  }

  // Get user input for the square to move to
  cout << "Move to: ";
  std::string move_to;
  getline(cin, move_to);

  if (move_to.length() > 2) {
    createNextMessage("You should type only two characters (column and row)\n");
    return;
  }

  Chess::Position future;
  future.column = move_to[0];
  future.row = move_to[1];

  future.column = toupper(future.column);

  if (future.column < 'A' || future.column > 'H') {
    createNextMessage("Invalid column.\n");
    return;
  }

  if (future.row < '0' || future.row > '8') {
    createNextMessage("Invalid row.\n");
    return;
  }

  // Put in the string to be logged
  to_record += future.column;
  to_record += future.row;

  // Convert columns from ['A'-'H'] to [0x00-0x07]
  future.column = future.column - 'A';

  // Convert row from ['1'-'8'] to [0x00-0x07]
  future.row = future.row - '1';

  // Check if it is not the exact same square
  if (future.row == present.row && future.column == present.column) {
    createNextMessage("[Invalid] You picked the same square!\n");
    return;
  }

  // Is that move allowed?
  Chess::EnPassant S_enPassant = {false};
  Chess::Castling S_castling = {false};
  Chess::Promotion S_promotion = {false};

  if (!Game::isMoveValid(current_game, present, future, &S_enPassant, &S_castling, &S_promotion)) {
    createNextMessage("[Invalid] Piece can not move to that square!\n");
    return;
  }

  // Promotion: user most choose a piece to
  // replace the pawn
  if (S_promotion.isApplied) {
    cout << "Promote to (Q, R, N, B): ";
    std::string piece;
    getline(cin, piece);

    if (piece.length() > 1) {
      createNextMessage("You should type only one character (Q, R, N or B)\n");
      return;
    }

    char chPromoted = toupper(piece[0]);

    if (chPromoted != 'Q' && chPromoted != 'R' && chPromoted != 'N' &&
        chPromoted != 'B') {
      createNextMessage("Invalid character.\n");
      return;
    }

    current_game->getPieceAtPosition(present.row, present.column);

    if (Chess::WHITE_PLAYER == current_game->getCurrentTurn()) {
      S_promotion.pieceAfter = toupper(chPromoted);
    } else {
      S_promotion.pieceAfter = tolower(chPromoted);
    }

    to_record += '=';
    to_record += toupper(chPromoted); // always log with a capital letter
  }

  // Log the move: do it prior to making the move
  // because we need the getCurrentTurn()
  current_game->logMove(to_record);

  // Make the move
  makeMove(current_game, present, future, &S_enPassant, &S_castling, &S_promotion);

  // Check if this move we just did put the opponent's king in check
  // Keep in mind that player turn has already changed
  if (current_game->playerKingInCheck()) {
    if (current_game->isCheckMate()) {
      if (Chess::WHITE_PLAYER == current_game->getCurrentTurn()) {
        appendToNextMessage("Checkmate! Black wins the game!\n");
      } else {
        appendToNextMessage("Checkmate! White wins the game!\n");
      }
    } else {
      // Add to the string with '+=' because it's possible that
      // there is already one message (e.g., piece captured)
      if (Chess::WHITE_PLAYER == current_game->getCurrentTurn()) {
        appendToNextMessage("White king is in check!\n");
      } else {
        appendToNextMessage("Black king is in check!\n");
      }
    }
  }
}
