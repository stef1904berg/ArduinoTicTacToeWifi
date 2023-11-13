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

void Board::drawTicTacToe() {
    this->clearFrame();

    int drawX = 4;

    for (int x = 0; x < 3; x++) {

        int drawY = 7;
        for (int y = 0; y < 3; y++) {
            int playerMove = this->playerMoves[x][y];

            this->drawPlayerMove(drawX, drawY, playerMove);

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
    }
}