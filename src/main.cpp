#include <Arduino.h>
#include <Arduino_LED_Matrix.h>
#include "Board.h"

ArduinoLEDMatrix matrix;
Board board;

void setup() {
    matrix.begin();

    board.setPlayerMove(0, 0, 1);
    board.setPlayerMove(1, 0, 2);
    board.setPlayerMove(2, 0, 2);
    board.setPlayerMove(1, 1, 1);
    board.setPlayerMove(2, 2, 1);

    board.drawTicTacToe();

    matrix.renderBitmap(board.frame, 8, 12);

}

void loop() {

}