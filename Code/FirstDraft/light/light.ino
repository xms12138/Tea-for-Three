#include <Adafruit_NeoPixel.h>

#define LED_PIN 6
#define LED_COUNT 8
#define FSR_PIN A0

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {

  strip.begin();
  strip.setBrightness(255);

  pinMode(FSR_PIN, INPUT_PULLUP);   // 使用内部上拉
}

void loop() {

  int value = analogRead(FSR_PIN);

  if (value < 800)   // 按下时数值会下降
  {
    for(int i = 0; i < LED_COUNT; i++)
      strip.setPixelColor(i, strip.Color(0,255,0));
  }
  else
  {
    for(int i = 0; i < LED_COUNT; i++)
      strip.setPixelColor(i, strip.Color(0,0,0));
  }

  strip.show();

  delay(20);
}