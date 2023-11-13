#include <Arduino.h>
#include <Arduino_LED_Matrix.h>
#include <Board.h>

ArduinoLEDMatrix matrix;
Board board;

void setup() {
//    Serial.begin(115200);

    matrix.begin();

    board.drawTicTacToe();

    matrix.renderBitmap(board.frame, 8, 12);

}

void loop() {

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            board.setPlayerMove(x, y, random(0,3));
        }
    }

    board.drawTicTacToe();
    matrix.renderBitmap(board.frame, 8, 12);
    delay(100);
}