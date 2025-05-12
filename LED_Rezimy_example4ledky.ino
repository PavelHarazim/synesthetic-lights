
#include <WiFi.h>
#include <WiFiUdp.h>
#include <FastLED.h>

// ==== NASTAVENÍ SÍTĚ ====
const char* ssid = "TVA_WIFI_SIT";
const char* password = "TVOJE_HESLO";

const int localPort = 9000;  // UDP port

// ==== LED NASTAVENÍ ====
#define LED_PIN     5
#define NUM_LEDS    4
CRGB leds[NUM_LEDS];

// ==== UDP ====
WiFiUDP udp;
char incomingPacket[255];

// ==== REŽIMY ====
enum Mode { AUTO, MIDI, MANUAL };
Mode currentMode = AUTO;

// ==== Časovač pro AUTO beat ====
unsigned long lastAutoBeat = 0;
const unsigned long beatInterval = 500; // 120 BPM

// ==== Barva pro MANUAL ====
CRGB manualColor = CRGB::Blue;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Připojení k WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" WiFi připojeno!");
  Serial.println(WiFi.localIP());

  udp.begin(localPort);
  Serial.printf("UDP naslouchá na portu %d\n", localPort);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

void loop() {
  // AUTO režim beat
  if (currentMode == AUTO && millis() - lastAutoBeat > beatInterval) {
    lastAutoBeat = millis();
    blinkLEDs(CRGB::White);
  }

  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(incomingPacket, 255);
    if (len > 0) incomingPacket[len] = 0;

    String msg = String(incomingPacket);
    Serial.println("Zpráva: " + msg);

    // Přepnutí režimu
    if (msg.startsWith("/mode")) {
      if (msg.indexOf("auto") != -1) currentMode = AUTO;
      else if (msg.indexOf("midi") != -1) currentMode = MIDI;
      else if (msg.indexOf("manual") != -1) currentMode = MANUAL;
      Serial.print("Režim přepnut na: "); Serial.println(currentMode);
    }

    // MIDI zpráva
    if (currentMode == MIDI && msg.indexOf("/note") != -1) {
      blinkLEDs(CRGB::Red);
    }

    // MANUAL barva (např. /color 0 255 0)
    if (currentMode == MANUAL && msg.startsWith("/color")) {
      int r, g, b;
      sscanf(incomingPacket, "/color %d %d %d", &r, &g, &b);
      manualColor = CRGB(r, g, b);
      fill_solid(leds, NUM_LEDS, manualColor);
      FastLED.show();
    }

    // Obecný beat
    if (msg.indexOf("beat") != -1 && currentMode != AUTO) {
      blinkLEDs(CRGB::White);
    }
  }
}

void blinkLEDs(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  delay(80);
  FastLED.clear();
  FastLED.show();
}
