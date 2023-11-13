#include <Arduino.h>
#include <Arduino_LED_Matrix.h>
#include <Board.h>
#include <BoardScreens.h>
#include <WiFiS3.h>

const int ACCEPT_BUTTON_PIN = 7;
const int MOVE_OPTION_BUTTON_PIN = 6;

ArduinoLEDMatrix matrix;
Board board;

// 0: user host or join decision

// 10: starting AP for client join
// 11: client can join
// 12: client joined game

// 20: looking for AP to connect to
// 21: joined AP
// 22: connection established with host
int gameState = 0;
int hostGame = 0;

// Button variables;
int acceptButtonState;
int moveButtonState;
int lastAcceptButtonState = LOW;
int lastMoveButtonState = LOW;


void updateButtonStates();
void scanForNetworks();

void setup() {
    Serial.begin(115200);

    // Setup input buttons
    pinMode(ACCEPT_BUTTON_PIN, INPUT_PULLUP); // Accept button
    pinMode(MOVE_OPTION_BUTTON_PIN, INPUT_PULLUP); // Move option button

    matrix.begin();

    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        board.loadFrame(NO_WIFI_SCREEN);
        matrix.renderBitmap(board.frame, 8, 12);
        while (true);
    }
}

void loop() {
    updateButtonStates();



    if (gameState == 0) {
        if (moveButtonState == LOW && lastMoveButtonState == HIGH) {
            hostGame = !hostGame;
        }

        if (hostGame) {
            board.loadFrame(HOST_GAME_SCREEN);
        } else {
            board.loadFrame(JOIN_GAME_SCREEN);
        }

        if (acceptButtonState == LOW && lastAcceptButtonState == HIGH) {
            if (hostGame) {
                gameState = 10; // host gameState
            } else {
                gameState = 20; // join gameState
            }
        }

    }

    if (gameState == 10) {
        Serial.println("Starting AP.");
        while (true);
    }

    if (gameState == 20) {
        Serial.println("Scanning for networks.");
        scanForNetworks();
    }

    if (gameState == 21) {
        Serial.println("Connected to ap!");
        while (true);
    }


    lastAcceptButtonState = acceptButtonState;
    lastMoveButtonState = moveButtonState;

    matrix.renderBitmap(board.frame, 8, 12);
}

void updateButtonStates() {
    acceptButtonState = digitalRead(7);
    moveButtonState = digitalRead(6);
}

void scanForNetworks() {
    int numSsids = WiFi.scanNetworks();
    if (numSsids == -1) {
        board.loadFrame(NO_WIFI_SCREEN);
        matrix.renderBitmap(board.frame, 8, 12);
        Serial.println("Couldn't get a WiFi connection");
        while (true);
    }


    for (int network = 0; network < numSsids; ++network) {
        if (WiFi.encryptionType(network) == ENC_TYPE_NONE && strncmp(WiFi.SSID(network),"ttt_", 4) == 0) {
            Serial.print("Connecting to network: ");
            Serial.print(WiFi.SSID(network));
            Serial.println(".");
            int status = WiFi.begin(WiFi.SSID(network));

            // Wait until
            if (status == WL_CONNECTED) {
                while (WiFi.gatewayIP() == "0.0.0.0");
            } else {
                board.loadFrame(NO_WIFI_SCREEN);
                matrix.renderBitmap(board.frame, 8, 12);
                Serial.println("Couldn't connect to network.");
                while (true);
            }

            Serial.println(WiFi.gatewayIP());
            gameState = 21;
        }
    }

}