//
// Created by stef on 11/13/2023.
//

#include "Arduino.h";

#ifndef ARDUINOTICTACTOEWIFI_BOARD_H
#define ARDUINOTICTACTOEWIFI_BOARD_H

class Board {

private:
    int playerMoves[3][3];

public:
    byte frame[8][12];

    void setPlayerMove(int x, int y, int move) {
        playerMoves[y][x] = move;
    }

    int getPlayerMove(int x, int y) {
        return this->playerMoves[y][x];
    }

    void setPixel(int x, int y, byte on) {
        y = 7 - y;
        this->frame[y][x] = on;
    }

    void turnPixelOn(int x, int y) {
        this->setPixel(x, y, 1);
    }

    void turnPixelOff(int x, int y) {
        this->setPixel(x, y, 0);
    }

    void clearFrame() {
        for (int x = 0; x < 12; x++) {
            for (int y = 0; y < 8; y++) {
                turnPixelOff(x, y);
            }
        }
    }

    void drawTicTacToe() {
        this->clearFrame();

        int drawX = 5;

        for (int x = 0; x < 3; x++) {

            int drawY = 7;
            for (int y = 0; y < 3; y++) {
                int playerMove = playerMoves[x][y];

                drawPlayerMove(drawX, drawY, playerMove);

                drawY -= 3;
            }
            drawX += 3;
        }

    }

    void drawPlayerMove(int x, int y, int move) {
        switch (move) {
            case 1:
                this->turnPixelOn(x, y);
                this->turnPixelOff(x - 1, y);
                this->turnPixelOff(x, y - 1);
                this->turnPixelOn(x - 1, y - 1);
                break;
            case 2:
                this->turnPixelOn(x, y);
                this->turnPixelOn(x - 1, y);
                this->turnPixelOn(x, y - 1);
                this->turnPixelOn(x - 1, y - 1);
        }
    }
};


#endif //ARDUINOTICTACTOEWIFI_BOARD_H
