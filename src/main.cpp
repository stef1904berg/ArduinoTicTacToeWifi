#include <Arduino.h>
#include <Arduino_LED_Matrix.h>
#include <Board.h>
#include <BoardScreens.h>
#include <WiFiS3.h>
#include <ArduinoMqttClient.h>

const int ACCEPT_BUTTON_PIN = 7;
const int MOVE_OPTION_BUTTON_PIN = 6;

ArduinoLEDMatrix matrix;
Board board;

WiFiClient wifiClient;
MqttClient mqtt(wifiClient);

// 0: startup, connecting to internet
// 1: connecting to MQTT Server
// 2: user host or join decision


// 10: setup game hosting

// 20: setup game joining
int gameState = 0;
int hostGame = 0;

String MQTT_ADDRESS;

// Button variables;
int acceptButtonState;
int moveButtonState;
int lastAcceptButtonState = LOW;
int lastMoveButtonState = LOW;


void updateButtonStates();

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

    if (mqtt.connected()) {
        mqtt.poll();
    }

    if (gameState == 0) {
        int status = WiFi.begin("IoTatelierF2144", "IoTatelier");

        // Wait until
        if (status == WL_CONNECTED) {
            while (WiFi.gatewayIP() == "0.0.0.0");
        } else {
            board.loadFrame(NO_WIFI_SCREEN);
            matrix.renderBitmap(board.frame, 8, 12);
            Serial.println("Couldn't connect to network.");
            while (true);
        }

        board.loadFrame(WIFI_CONNECTED_SCREEN);
        matrix.renderBitmap(board.frame, 8, 12);

        MQTT_ADDRESS = WiFi.gatewayIP().toString();
        Serial.println(MQTT_ADDRESS);
        Serial.println(WiFi.localIP());
        gameState = 1;
    }

    if (gameState == 1) {

        if (!mqtt.connect(MQTT_ADDRESS.c_str())) {
            Serial.print("MQTT connection failed! Error code = ");
            Serial.println(mqtt.connectError());
            while (true);
        }

        mqtt.beginMessage("stefvandenberg/device/" + WiFi.localIP().toString() + "/status");
        mqtt.print("connected");
        mqtt.endMessage();

        gameState = 2;
    }


    if (gameState == 2) {
        if (moveButtonState == LOW && lastMoveButtonState == HIGH) {
            hostGame = !hostGame;
        }

        if (hostGame) {
            board.loadFrame(HOST_GAME_SCREEN);
        } else {
            board.loadFrame(JOIN_GAME_SCREEN);
        }

        if (acceptButtonState == LOW && lastAcceptButtonState == HIGH) {
            mqtt.beginMessage("stefvandenberg/device/" + WiFi.localIP().toString() + "/status");
            if (hostGame) {
                mqtt.print("hosting");
                gameState = 10;
            } else {
                mqtt.print("joining");
                gameState = 20;
            }
            mqtt.endMessage();
            board.clearFrame();
        }
    }

    if (gameState == 10) {
//        mqtt.
    }


    lastAcceptButtonState = acceptButtonState;
    lastMoveButtonState = moveButtonState;

    matrix.renderBitmap(board.frame, 8, 12);
}

void updateButtonStates() {
    acceptButtonState = digitalRead(7);
    moveButtonState = digitalRead(6);
}