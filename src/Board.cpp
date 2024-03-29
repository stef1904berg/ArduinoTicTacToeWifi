//
// Created by Stef on 13/11/2023.
//

#include "Board.h"

void Board::setPlayerMove(int x, int y, int move) {
    this->playerMoves[y][x] = move;
}

int Board::getPlayerMove(int x, int y) {
    return this->playerMoves[y][x];
}

void Board::setPixel(int x, int y, byte on) {
    y = 7 - y;
    this->frame[y][x] = on;
}

void Board::turnPixelOn(int x, int y) {
    this->setPixel(x, y, 1);
}

void Board::turnPixelOff(int x, int y) {
    this->setPixel(x, y, 0);
}

void Board::clearFrame() {
    for (int x = 0; x < 12; x++) {
        for (int y = 0; y < 8; y++) {
            this->turnPixelOff(x, y);
        }
    }
}

void Board::drawTicTacToe(bool clear) {
    if (clear) { this->clearFrame(); }

    int drawX = 4;
    for (int x = 0; x < 3; x++) {

        int drawY = 7;
        for (int y = 0; y < 3; y++) {

            this->drawPlayerMove(drawX, drawY, this->playerMoves[x][y]);

            drawY -= 3;
        }
        drawX += 3;
    }
}

void Board::drawPlayerMove(int x, int y, int move) {
    switch (move) {
        case 0:
            this->turnPixelOff(x, y);
            this->turnPixelOff(x + 1, y);
            this->turnPixelOff(x, y - 1);
            this->turnPixelOff(x + 1, y - 1);
            break;
        case 1:
            this->turnPixelOn(x, y);
            this->turnPixelOff(x + 1, y);
            this->turnPixelOff(x, y - 1);
            this->turnPixelOn(x + 1, y - 1);
            break;
        case 2:
            this->turnPixelOn(x, y);
            this->turnPixelOn(x + 1, y);
            this->turnPixelOn(x, y - 1);
            this->turnPixelOn(x + 1, y - 1);
            break;
        case 3:
            this->turnPixelOff(x, y);
            this->turnPixelOff(x + 1, y);
            this->turnPixelOn(x, y - 1);
            this->turnPixelOff(x + 1, y - 1);
    }
}

void Board::loadFrame(byte newFrame[8][12]) {
    for (int x = 0; x < 12; ++x) {
        for (int y = 0; y < 8; ++y) {
            this->setPixel(x, y, newFrame[7 - y][x]);
        }
    }
}

bool Board::checkTicTacToeWin(int player) {
    // Check vertical wins
    for (int y = 0; y < 3; ++y) {
        if (this->playerMoves[y][0] == player && this->playerMoves[y][1] == player && this->playerMoves[y][2] == player) { return true; }
    }

    // Check horizontal wins
    for (int x = 0; x < 3; ++x) {
        if (this->playerMoves[0][x] == player && this->playerMoves[1][x] == player && this->playerMoves[2][x] == player) { return true; }
    }

    // Check 2 horizontal wins
    if (this->playerMoves[0][0] == player && this->playerMoves[1][1] == player && this->playerMoves[2][2] == player) { return true; }
    if (this->playerMoves[0][2] == player && this->playerMoves[1][1] == player && this->playerMoves[2][0] == player) { return true; }

    return false;
}

bool Board::checkTicTacToeDraw() {
    int playerMoves = 0;

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            int playerMove = this->playerMoves[x][y];
            if (playerMove == 1 || playerMove == 2) { playerMoves++; }
        }
    }

    return playerMoves >= 9;
}
