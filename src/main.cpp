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

// 10: broadcast open room + send handshake accept message
// 11: finished handshakes

// 20: listening for open rooms + send handshake request
// 21: finished handshakes
int gameState = 0;
bool hostGame = false;

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
void publishMessage(const String& message, const String& topic);
String randomString(int length);
void onMqttMessage(int messageSize);
String getValue(String data, char separator, int index);

void setup() {
    Serial.begin(115200);

    // Set random seed (not good, but it's better than nothing
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

        publishMessage("connected", "device/" + localId + "/status");

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
                publishMessage("hosting", "device/" + localId + "/status");
                mqtt.subscribe(topicPrefix + "/rooms/" + localId + "/handshake");
                gameState = 10;
            } else {
                publishMessage("joining", "device/" + localId + "/status");
                mqtt.subscribe(topicPrefix + "/rooms/+/status");
                gameState = 20;
            }
            board.clearFrame();
        }
    }

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

    if (gameState == 11) {
        Serial.println("Host handshake complete");
        publishMessage("connected", "/game/" + localId + clientId + "/client");
        while (true);
    }

    if (gameState == 21) {
        Serial.println("Client handshake complete");
        publishMessage("connected", "/game/" + hostId + localId + "/client");
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

void publishMessage(const String& message, const String& topic) {
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

    Serial.println(messageTopic);
    String firstPart = getValue(messageTopic, '/', 1);
    if (firstPart == "rooms") {
        String roomId = getValue(messageTopic, '/', 2);

        String secondPart = getValue(messageTopic, '/', 3);
        if (secondPart == "status" && message == "open") {
            // Respond to open room message
            // rooms/<room>/handshake = client id
            publishMessage(localId, "rooms/" + roomId + "/handshake");
            mqtt.subscribe(topicPrefix + "/" + roomId + "/" + localId);

        } else if (secondPart == "handshake" && roomId == localId && hostGame && !handshakeSent) {
            // Response to client room handshake request
            // rooms/<room>/<client id> = accept
            publishMessage("accept", "rooms/" + roomId + "/" + message);
            handshakeSent = true;
            gameState = 11;
        } else if (secondPart == localId && message == "accept") {
            hostId = roomId;
            gameState == 21;
        }
    }
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;

    for(int i=0; i<=maxIndex && found<=index; i++){
        if(data.charAt(i)==separator || i==maxIndex){
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }

    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}