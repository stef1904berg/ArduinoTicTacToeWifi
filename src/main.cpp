#include <Arduino.h>
#include <Arduino_LED_Matrix.h>
#include <Board.h>
#include <BoardScreens.h>
#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include <secrets.h>

const int ACCEPT_BUTTON_PIN = 7;
const int MOVE_OPTION_BUTTON_PIN = 6;

const String topicPrefix = MQTT_TOPIC_PREFIX;

ArduinoLEDMatrix matrix;
Board board;

WiFiClient wifiClient;
MqttClient mqtt(wifiClient);

// 0: startup, connecting to internet
// 1: connecting to MQTT Server
// 2: user host or join decision

// 10: setup game hosting + listen for handshake request
// 11: send handshake response

// 20: listening for open rooms + send handshake request
// 21: listen for handshake response
int gameState = 0;
int hostGame = 0;

// Button variables;
int acceptButtonState;
int moveButtonState;
int lastAcceptButtonState = LOW;
int lastMoveButtonState = LOW;

// Timers
unsigned long previousMillis = 0;
const long broadcastHostInterval = 1000;

String clientIdentifier;

void updateButtonStates();
void publishMessage(const String& message, const String& topic);
String randomString(int length);

void setup() {
    Serial.begin(115200);

    // Set random seed (not good, but it's better than nothing
    int rSeed = analogRead(0) + (analogRead(0) * 1000) + (analogRead(0) * 1000000);
    randomSeed(rSeed);

    clientIdentifier = randomString(10);
    Serial.print("Client identifier: ");
    Serial.println(clientIdentifier);

    // Setup input buttons
    pinMode(ACCEPT_BUTTON_PIN, INPUT_PULLUP); // Accept button
    pinMode(MOVE_OPTION_BUTTON_PIN, INPUT_PULLUP); // Move option button

    matrix.begin();

    mqtt.setUsernamePassword(MQTT_USERNAME, MQTT_PASSWORD);

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
        int status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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

        Serial.print("Client IP address: ");
        Serial.println(WiFi.localIP());
        gameState = 1;
    }

    if (gameState == 1) {

        if (!mqtt.connect(MQTT_ADDRESS)) {
            Serial.print("MQTT connection failed! Error code = ");
            Serial.println(mqtt.connectError());
            while (true);
        }

        publishMessage("connected", "device/" + WiFi.localIP().toString() + "/status");
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
            if (hostGame) {
                publishMessage("hosting", "device/" + WiFi.localIP().toString() + "/status");
                gameState = 10;
            } else {
                publishMessage("joining", "device/" + WiFi.localIP().toString() + "/status");
                gameState = 20;
            }
            mqtt.endMessage();
            board.clearFrame();
        }
    }

    if (gameState == 10) {
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= broadcastHostInterval) {
            previousMillis = currentMillis;

            publishMessage("open", "rooms/" + clientIdentifier + "/status");

            board.loadFrame(WIFI_CONNECTED_SCREEN);
        } else {
            board.clearFrame();
        }
    }

    lastAcceptButtonState = acceptButtonState;
    lastMoveButtonState = moveButtonState;

    matrix.renderBitmap(board.frame, 8, 12);
}

void updateButtonStates() {
    acceptButtonState = digitalRead(7);
    moveButtonState = digitalRead(6);
}

void publishMessage(const String& message, const String& topic) {
    mqtt.beginMessage(topicPrefix + "/" + topic);
    mqtt.print(message);
    mqtt.endMessage();
}

String randomString(int length) {
    const char possible[] = "abcdefghijklmnopqrstuvwxyz";
    char rString[length];
    for (int i = 0; i < length; ++i) {
        rString[i] = possible[random(0, 26)];
    }
    return rString;
}
