#pragma once
#include "game.h"

#define WHITE_SQUARE '.'
#define BLACK_SQUARE ' '
#define EMPTY_SQUARE ' '

void createNextMessage(string message);
void appendToNextMessage(string message);
void clearScreen();
void printLogo();
void printMenu();
void printMessage();
void printLine(int line, int color1, int color2, Game &game);
void printSituation(Game &game);
void printBoard(Game &game);