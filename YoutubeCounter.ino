#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include "font/FLIPclockblack80pt7b.h"""

#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN 12
#define NUMPIXELS 2

TFT_eSPI tft = TFT_eSPI();
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

void setup()
{
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif
    pixels.begin();
    pixels.clear();

    Serial.begin(115200); // This line mandatory for using the display prototyping library, change the baud rate if needed

    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(&FLIPclockblack80pt7b);
    tft.drawString("9999", 60, 80);
}

void loop()
{

    for (int i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    }
    pixels.show();
    delay(DELAYVAL);
    for (int i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    }
    pixels.show();
    delay(DELAYVAL);
    pixels.clear();
    pixels.show();
    delay(DELAYVAL);
}