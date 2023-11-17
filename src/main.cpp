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

// Host
// 10: broadcast open room + send handshake accept message
// 11: finished handshakes
// 12: waiting for move
// 13: playing move
// 14: won game
// 15: lost game

// Client
// 20: listening for open rooms + send handshake request
// 21: finished handshakes
// 22: waiting for move
// 23: playing move
// 24: won game
// 25: lost game
int gameState = 0;
bool hostGame = false;
int movePosX = 0;
int movePosY = 0;

// Game host variables
bool handshakeSent = false;
String clientId;

// Client variables
String hostId;

// Button variables;
int acceptButtonState;
int moveButtonState;
int lastAcceptButtonState = LOW;
int lastMoveButtonState = LOW;

// Timers
unsigned long previousMillis = 0;
const long broadcastHostInterval = 10000;

String localId;

void updateButtonStates();

void publishMessage(const String &message, const String &topic);

String randomString(int length);

void onMqttMessage(int messageSize);

String getValue(String data, char separator, int index);

void nextMovePosition();

void setup() {
    Serial.begin(115200);

    // Set random seed (not good, but it's better than nothing)
    int rSeed = analogRead(0) + (analogRead(0) * 1000) + (analogRead(0) * 1000000);
    randomSeed(rSeed);

    localId = randomString(10);
    Serial.print("Client identifier: ");
    Serial.println(localId);

    // Setup input buttons
    pinMode(ACCEPT_BUTTON_PIN, INPUT_PULLUP); // Accept button
    pinMode(MOVE_OPTION_BUTTON_PIN, INPUT_PULLUP); // Move option button

    matrix.begin();

    mqtt.setUsernamePassword(MQTT_USERNAME, MQTT_PASSWORD);
    mqtt.onMessage(onMqttMessage);

    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        board.loadFrame(NO_WIFI_SCREEN);
        matrix.renderBitmap(board.frame, 8, 12);
        while (true);
    }
}

void loop() {
    updateButtonStates();

    mqtt.poll();

    // Startup, connecting to internet
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

        Serial.print("Client IP address: ");
        Serial.println(WiFi.localIP());
        gameState = 1;
    }

    // Connecting to MQTT server
    if (gameState == 1) {

        if (!mqtt.connect(MQTT_ADDRESS)) {
            Serial.print("MQTT connection failed! Error code = ");
            Serial.println(mqtt.connectError());
            while (true);
        }

        Serial.println("Connected to MQTT server");
        publishMessage("connected", "device/" + localId + "/status");

        gameState = 2;
    }

    // User host of join decision
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
                publishMessage("hosting", "device/" + localId + "/status"); // no use for this yet
                mqtt.subscribe(topicPrefix + "/rooms/" + localId + "/handshake");
                gameState = 10;
            } else {
                publishMessage("joining", "device/" + localId + "/status"); // no use for this yet
                mqtt.subscribe(topicPrefix + "/rooms/+/status");
                gameState = 20;
            }
            board.clearFrame();
        }
    }

    // broadcast open room + send handshake accept message
    if (gameState == 10) {
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= broadcastHostInterval) {
            previousMillis = currentMillis;

            publishMessage("open", "rooms/" + localId + "/status");

            board.loadFrame(WIFI_CONNECTED_SCREEN);
        } else {
            board.clearFrame();
        }
    }

    // Host: finished handshakes
    if (gameState == 11) {
        Serial.println("Host handshake complete");
        publishMessage("connected", "games/" + localId + clientId + "/host");

        bool hostFirstMove = random(0, 2);
        publishMessage(hostFirstMove ? "hostmove" : "clientmove", "games/" + localId + clientId + "/status");
        gameState = 12;
    }

    // Client: finished handshakes
    if (gameState == 21) {
        Serial.println("Client handshake complete");
        publishMessage("connected", "games/" + hostId + localId + "/client");
        gameState = 22;

    }

    // waiting for client move
    if (gameState == 12) {

    }

    // waiting for host move
    if (gameState == 22) {

    }

    // playing host/client move
    if (gameState == 13 || gameState == 23) {
        if (board.getPlayerMove(movePosY, movePosX) == 0) {
            board.setPlayerMove(movePosY, movePosX, 3);
        }

        if (moveButtonState == LOW && lastMoveButtonState == HIGH) {

            if (board.getPlayerMove(movePosY, movePosX) == 3 || board.getPlayerMove(movePosY, movePosX) == 0) {
                board.setPlayerMove(movePosY, movePosX, 0);
            }

            nextMovePosition();
            while (board.getPlayerMove(movePosY, movePosX) == 1 || board.getPlayerMove(movePosY, movePosX) == 2) {
                nextMovePosition();
            }

            board.setPlayerMove(movePosY, movePosX, 3);

        }

        if (acceptButtonState == LOW && lastAcceptButtonState == HIGH) {
            board.setPlayerMove(movePosY, movePosX, hostGame ? 2 : 1);

            nextMovePosition();
            int iterations = 0;
            while (board.getPlayerMove(movePosY, movePosX) == 1 || board.getPlayerMove(movePosY, movePosX) == 2) {
                nextMovePosition();
                iterations++;
                if (iterations > 7) { break; }
            }

            String boardData = "";
            for (int x = 0; x < 3; ++x) {
                for (int y = 0; y < 3; ++y) {
                    boardData += board.getPlayerMove(x, y);
                }
            }
            boardData.replace('3', '0');

            String roomId;
            if (hostGame) {
                roomId = localId + clientId;
            } else {
                roomId = hostId + localId;
            }
            publishMessage(boardData, "games/" + roomId + "/board");
            publishMessage(hostGame ? "hostmove" : "clientmove", "games/" + roomId + "/status");

            if (board.checkTicTacToeWin(hostGame ? 2 : 1)) {
                publishMessage(hostGame ? "hostwin" : "clientwin", "games/" + roomId + "/status");
                Serial.print(hostGame ? "Host" : "Client");
                Serial.println(" has won the game");
                gameState = hostGame ? 24 : 14;
            } else {
                gameState = hostGame ? 22 : 12;
            }
        }

        board.drawTicTacToe(false);
    }

    // Show win screen
    if (gameState == 14 || gameState == 24) {
        board.loadFrame(WIN_SCREEN);
    }

    // Show lose screen
    if (gameState == 15 || gameState == 25) {
        board.loadFrame(LOSE_SCREEN);
    }

    lastAcceptButtonState = acceptButtonState;
    lastMoveButtonState = moveButtonState;

    matrix.renderBitmap(board.frame, 8, 12);
}

void updateButtonStates() {
    acceptButtonState = digitalRead(7);
    moveButtonState = digitalRead(6);
}

void publishMessage(const String &message, const String &topic) {
    mqtt.beginMessage(topicPrefix + "/" + topic);
    mqtt.print(message);
    mqtt.endMessage();
}

String randomString(int length) {
    const char possible[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char rString[length];
    for (int i = 0; i < length; ++i) {
        rString[i] = possible[random(0, 52)];
    }
    return rString;
}

void onMqttMessage(int messageSize) {
    String message;
    String messageTopic = mqtt.messageTopic();

    while (mqtt.available()) {
        message += (char) mqtt.read();
    }

    String firstPart = getValue(messageTopic, '/', 1);
    if (firstPart == "rooms") {
        String roomId = getValue(messageTopic, '/', 2);

        String secondPart = getValue(messageTopic, '/', 3);
        if (secondPart == "status" && message == "open") {
            // Client
            // Respond to open room message
            // rooms/<room>/handshake = localId
            publishMessage(localId, "rooms/" + roomId + "/handshake");
            mqtt.subscribe(topicPrefix + "/rooms/" + roomId + "/" + localId);

        } else if (secondPart == "handshake" && roomId == localId && hostGame && !handshakeSent) {
            // Host
            // Response to client room handshake request
            // rooms/<room>/<client id> = accept
            clientId = message;
            mqtt.unsubscribe(topicPrefix + "/rooms/" + localId + "/handshake");
            publishMessage("accept", "rooms/" + roomId + "/" + message);
            mqtt.subscribe(topicPrefix + "/games/" + localId + clientId + "/board");
            mqtt.subscribe(topicPrefix + "/games/" + localId + clientId + "/status");
            handshakeSent = true;

            gameState = 11;
        } else if (secondPart == localId && message == "accept") {
            // Client
            // Handle handshake accept message
            hostId = roomId;
            mqtt.unsubscribe(topicPrefix + "/rooms/+/status");
            mqtt.unsubscribe(topicPrefix + "/rooms/" + roomId + "/" + localId);
            mqtt.subscribe(topicPrefix + "/games/" + hostId + localId + "/board");
            mqtt.subscribe(topicPrefix + "/games/" + hostId + localId + "/status");

            gameState = 21;
        }
    }

    if (firstPart == "games") {
        String gameId = getValue(messageTopic, '/', 2);
        String secondPart = getValue(messageTopic, '/', 3);

        if (secondPart == "board") {
            int x = 0;
            int y = 0;
            for (int i = 0; i < message.length(); ++i) {
                board.setPlayerMove(x, y, message[i] - '0');

                y++;
                if (y >= 3) {
                    y = 0;
                    x++;
                }

                board.drawTicTacToe(false);
            }
        }

        if (secondPart == "status") {

            // Set host to moving
            if (message == "hostmove" && hostGame) {
                gameState = 13;
            }

            // Set client to moving
            if (message == "clientmove" && !hostGame) {
                gameState = 23;
            }

            if (message == "clientwin") {
                if (hostGame) {
                    gameState = 25;
                } else {
                    gameState = 24;
                }
            }

            if (message == "hostwin") {
                if (hostGame) {
                    gameState = 24;
                } else {
                    gameState = 25;
                }
            }

        }
    }
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void nextMovePosition() {
    movePosX++;
    if (movePosX >= 3) {
        movePosX = 0;
        movePosY++;
    }
    if (movePosY >= 3) { movePosY = 0; }
}