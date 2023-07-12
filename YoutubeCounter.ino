#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include "font/FLIPclockblack80pt7b.h"

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
    drawCenteredString("9999", &FLIPclockblack80pt7b);
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

void drawCenteredString(const String &text, const GFXfont *f)
{
    // Set the desired font
    tft.setFreeFont(f);

    // Get the width and height of the display
    int displayWidth = tft.width();
    int displayHeight = tft.height();

    // Calculate the width of the text string
    int textWidth = tft.textWidth(text);
    int textHeight = 160; // tft.fontHeight();

    // Calculate the position to center the text
    int cursorX = (displayWidth - textWidth) / 2;
    int cursorY = (displayHeight - textHeight) / 2; 

    // Draw the text
    tft.drawString(text, cursorX, cursorY);
}
