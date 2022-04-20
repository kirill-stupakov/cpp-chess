#include "includes.h"
#include "game.h"
#include "user_interface.h"

Game *currentGame = nullptr;

void newGame() {
  delete currentGame;
  currentGame = new Game();
}

int main() {
  bool gameContinues = true;

  // Clear screen and print the logo
  clearScreen();
  printLogo();

  string input;

  while (gameContinues) {
    printMessage();
    printMenu();

    // Get input from user
    cout << "Type here: ";
    getline(cin, input);

    if (input.length() != 1) {
      cout << "Invalid option. Type one letter only\n\n";
      continue;
    }

    try {
      switch (input[0]) {
      case 'N':
      case 'n': {
        newGame();
        clearScreen();
        printLogo();
        printSituation(*currentGame);
        printBoard(*currentGame);
      } break;

      case 'M':
      case 'm': {
        if (nullptr != currentGame) {
          if (currentGame->isFinished()) {
            cout << "This game has already finished!\n";
          } else {
            Game::movePiece(currentGame);
            printLogo();
            printSituation(*currentGame);
            printBoard(*currentGame);
          }
        } else {
          cout << "No game running!\n";
        }

      } break;

      case 'Q':
      case 'q': {
        gameContinues = false;
      } break;

      default: {
        cout << "Option does not exist\n\n";
      } break;
      }

    } catch (const char *s) {
    }
  }

  return 0;
}
