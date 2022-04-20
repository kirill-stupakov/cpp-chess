#include "user_interface.h"
#include "includes.h"

string next_message;

void createNextMessage(string message) { next_message = message; }

void appendToNextMessage(string message) { next_message += message; }
void clearScreen() { system("clear"); }

void printLogo() { cout << "    ===============| CHESS |==============\n"; }

void printMenu() { cout << "Commands: (N)ew game \t(M)ove \t(Q)uit \n"; }

void printMessage() {
  cout << next_message << endl;
  next_message = "";
}

void printLine(int line, int color1, int color2, Game &game) {
  int cell = 7;
  for (int subLine = 0; subLine < cell / 2; subLine++) {
    for (int pair = 0; pair < 4; pair++) {
      for (int subColumn = 0; subColumn < cell; subColumn++) {
        if (subLine == 1 && subColumn == 3) {
          cout << char(game.getPieceAtPosition(line, pair * 2) != 0x20
                           ? game.getPieceAtPosition(line, pair * 2)
                           : color1);
        } else {
          cout << char(color1);
        }
      }

      for (int subColumn = 0; subColumn < cell; subColumn++) {
        if (subLine == 1 && subColumn == 3) {
          cout << char(game.getPieceAtPosition(line, pair * 2 + 1) != 0x20
                           ? game.getPieceAtPosition(line, pair * 2 + 1)
                           : color2);
        } else {
          cout << char(color2);
        }
      }
    }

    if (1 == subLine) {
      cout << "   " << line + 1;
    }
    cout << "\n";
  }
}

void printSituation(Game &game) {
  if (!game.rounds.empty()) {
    cout << "Last moves:\n";

    int moves = game.rounds.size();
    int toShow = moves >= 5 ? 5 : moves;

    string space;
    while (toShow--) {
      if (moves < 10) {
        space = " ";
      }

      cout << space << moves << " ...... "
           << game.rounds[moves - 1].whiteMove.c_str() << " | "
           << game.rounds[moves - 1].blackMove.c_str() << "\n";
      moves--;
    }

    cout << "\n";
  }

  if (!game.whiteCaptured.empty() || !game.blackCaptured.empty()) {
    cout << "---------------------------------------------\n";
    cout << "WHITE captured: ";
    for (char i : game.whiteCaptured) {
      cout << char(i) << " ";
    }
    cout << "\n";

    cout << "black captured: ";
    for (char i : game.blackCaptured) {
      cout << char(i) << " ";
    }
    cout << "\n---------------------------------------------\n";
  }

  cout << "Current turn: "
       << (game.getCurrentTurn() == Chess::WHITE_PIECE ? "WHITE (upper case)"
                                                       : "BLACK (lower case)")
       << "\n\n";
}

void printBoard(Game &game) {
  cout << "   A      B      C      D      E      F      G      H\n\n";

  for (int link = 7; link >= 0; link--) {
    if (link % 2 == 0) {
      printLine(link, BLACK_SQUARE, WHITE_SQUARE, game);
    }

    else {
      printLine(link, WHITE_SQUARE, BLACK_SQUARE, game);
    }
  }
}