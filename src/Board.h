//
// Created by Stef on 13/11/2023.
//

#include <Arduino.h>

#ifndef ARDUINOTICTACTOEWIFI_BOARD_H
#define ARDUINOTICTACTOEWIFI_BOARD_H



class Board {

private:
    typedef byte (&led_frame)[8][12];

    int playerMoves[3][3];


public:
    byte frame[8][12];

    void setPlayerMove(int x, int y, int move);

    int getPlayerMove(int x, int y);

    void setPixel(int x, int y, byte on);

    void turnPixelOn(int x, int y);

    void turnPixelOff(int x, int y);

    void clearFrame();

    void drawTicTacToe();

    void drawPlayerMove(int x, int y, int move);

    void loadFrame(byte newFrame[8][12]);
};


#endif //ARDUINOTICTACTOEWIFI_BOARD_H
