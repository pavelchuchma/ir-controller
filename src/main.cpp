#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPTelnet.h>
#include <IRremote.hpp> 
#include "credentials.h"

ESPTelnet telnet;
const int telnetPort = 23;
const short int LED_PIN = 2; // GPIO2 is the built-in LED pin on ESP-01
const short int INPUT_PIN = 3; // GPIO3 is the RX pin on ESP-01
short ledState = 0;


void initializeWiFi() {
    Serial.println("\nconnecting WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint16_t connectWait = 240;
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        digitalWrite(LED_PIN, ledState = !ledState);
        Serial.printf(".");
        if (!connectWait--) {
            Serial.printf("Failed to connect, restarting!\n");
            ESP.restart();
        }
    }

    Serial.printf("\nWiFi connected, IP address: ");
    digitalWrite(LED_PIN, 0);

    Serial.println(WiFi.localIP());
    if (!MDNS.begin("esp01")) {
        Serial.println("Error setting up MDNS responder!");
    }
    Serial.println("mDNS responder started");
}

void onTelnetConnect(String ip) {
    Serial.printf("- Telnet: %s connected\n", ip.c_str());
    telnet.println("\nWelcome " + telnet.getIP());
}

void onTelnetInput(String str) {
    if (str == "bye") {
        telnet.println("> disconnecting you...");
        telnet.disconnectClient();
    } else if (str == "r") {
        IrSender.sendKaseikyo_Denon(0x514, 0x0, 3);
        telnet.println("> sending Kaseikyo Denon command: AVR ON");
    } else if (str == "p1") {
        IrSender.sendNEC(0x32, 0x2, 3);
        telnet.println("> sending NEC command Projector ON");
    } else if (str == "p2") {
        IrSender.sendNEC(0x32, 0x2E, 3);
        telnet.println("> sending NEC command Projector OFF");
    } else {
        telnet.println(str);
    }
}

void setupTelnet() {
    telnet.onConnect(onTelnetConnect);
    telnet.onInputReceived(onTelnetInput);

    Serial.print("Telnet: ");
    if (telnet.begin(telnetPort)) {
        Serial.println("running on port " + String(telnetPort));
    } else {
        Serial.println("error. Will reboot...");
        ESP.restart();
    }
}

void setupIR() {
    IrReceiver.begin(INPUT_PIN, false);
    IrSender.begin(LED_PIN, false, 0);
    telnet.print("Ready to receive IR signals of protocols: ");
    printActiveIRProtocols(&Serial);
}

void setup() {
    Serial.begin(460800, SERIAL_8N1, SERIAL_TX_ONLY, 1, false);
    pinMode(LED_PIN, OUTPUT);
    pinMode(INPUT_PIN, INPUT);

    initializeWiFi();

    setupTelnet();

    setupIR();
    Serial.println("IR receiver ready");
}


void loopIR() {
    /*
     * Check if received data is available and if yes, try to decode it.
     * Decoded result is in the IrReceiver.decodedIRData structure.
     *
     * E.g. command is in IrReceiver.decodedIRData.command
     * address is in command is in IrReceiver.decodedIRData.address
     * and up to 32 bit raw data in IrReceiver.decodedIRData.decodedRawData
     */
    if (IrReceiver.decode()) {
        /*
         * Print a summary of received data
         */
        if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
            Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
            // We have an unknown protocol here, print extended info
            IrReceiver.printIRResultRawFormatted(&Serial, true);

            IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
        } else {
            IrReceiver.resume(); // Early enable receiving of the next IR frame

            // IrReceiver.printIRResultShort(&Serial);
            IrReceiver.printIRSendUsage(&Serial);
        }

        /*
         * Finally, check the received data and perform actions according to the received command
         */
        if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT) {
            telnet.println(F("Repeat received. Here you can repeat the same action as before."));
        } else {
            if (IrReceiver.decodedIRData.command == 0x10) {
                telnet.println(F("Received command 0x10."));
                // do something
            } else if (IrReceiver.decodedIRData.command == 0x11) {
                telnet.println(F("Received command 0x11."));
                // do something else
            }
        }
    }
}

void loop() {
    telnet.loop();
    MDNS.update();
    loopIR();
}
