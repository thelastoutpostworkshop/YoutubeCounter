#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "font/Aurebesh_Bold80pt7b.h"
#include "font/Aurebesh_Bold40pt7b.h"
#include "counterWeb.h"
#include "mp3tf16p.h"
#include "scheduler.h"
#include "tactical.h"

#ifdef __AVR__
#include <avr/power.h>
#endif

// Neopixels
#define PIXELSPIN 12
#define PIXELSCOUNT 8
uint16_t roundPixels[] = {1, 2, 3, 4, 5, 6};
const int roundPixelsCount = 6;
const int looseSubscriberPixel = 7;
const int gainSubscriberPixel = 0;

Adafruit_NeoPixel pixels(PIXELSCOUNT, PIXELSPIN, NEO_GRB + NEO_KHZ800);

// Display
TFT_eSPI tft = TFT_eSPI();

// MP3 Player
#define RXPIN 16
#define TXPIN 17
MP3Player mp3(RXPIN, TXPIN);
int currentVolume = 15;
int soundStartup = 12;
int soundGainingSubscriber[] = {2, 3, 4, 6, 11};
int soundGainingSubscriberCount = 5;
int soundLoosingSubscriber = 3;
int soundTwoPlusSubscriber = 9;
int test = 1;

// Rotary Encoder
#define Rotary_Clock 27
#define Rotary_Data 26
#define Rotary_PushButton 14
#define PushButton_Debounce 200
#define Rotary_Debounce 5
enum Rotary_Status
{
    NO_STATUS,
    CLOCKWISE,
    COUNTERCLOCKWISE
};

int rotary_lastStateClock; // Store the PREVIOUS status of the clock pin (HIGH or LOW)
unsigned long rotary_lastTimeButtonPress = 0;
unsigned long rotary_lastTurn = 0;

// Fonts
const GFXfont *aurebeshCounter = &Aurebesh_Bold80pt7b;
const GFXfont *aurebeshText = &Aurebesh_Bold40pt7b;
const uint32_t counterColor = TFT_WHITE;

int currentSubscriberCount;
enum Subscriber_Status
{
    UNKNNOWN,
    LOOSING,
    GAINING,
};
Subscriber_Status currentSubscriberStatus = UNKNNOWN;

TaskScheduler scheduler;

void setup()
{
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif
    Serial.begin(115200);
    randomSeed(analogRead(0));

    initPixels();
    initDisplay();
    initRotaryEncoder();
    mp3.initialize();
    mp3.playTrackNumber(soundStartup, currentVolume, false);

    drawCenteredHorizontalText("Connect", 80, aurebeshText, TFT_YELLOW);
    drawCenteredHorizontalText("Wifi", 160, aurebeshText, TFT_YELLOW);
    initWebServer();

    clearScreen();

    // // Define a grayscale color map from white to black.
    // uint16_t cmap[256];
    // for (int i = 0; i < 256; i++)
    // {
    //     uint8_t gray = 255 - i;             // invert the grayscale value
    //     uint16_t r = gray >> 3;             // shift to 5 bit value
    //     uint16_t g = gray >> 2;             // shift to 6 bit value
    //     uint16_t b = gray >> 3;             // shift to 5 bit value
    //     cmap[i] = (r << 11) | (g << 5) | b; // Color in 565 format
    // }

    // tft.pushImage(0, 0, 480, 320, image_data_tactical,true,cmap);

    if (getSubscriberCount(currentSubscriberCount))
    {
        drawCenteredScreenText(String(currentSubscriberCount), aurebeshCounter, counterColor);
    }

    scheduler.addTask(showRandomRoundPixels, 10000L);
    scheduler.addTask(fetchSubscriberCountIfNeeded, 300000L);
    // scheduler.addTask(playDarthVadedBreathing, 3600000L);
    // scheduler.addTask(showRainbow, 900000L);
}

void loop()
{
    server.handleClient();
    scheduler.runTasks();

    if (readRotaryPushButton())
    {
        mp3.playTrackNumber(test, currentVolume, false);
        test += 1;
    }

    Rotary_Status rotary = readRotaryEncoder();
    if (rotary != NO_STATUS)
    {
        switch (rotary)
        {
        case CLOCKWISE:
            for (int i = 0; i < PIXELSCOUNT; i++)
            {
                pixels.setPixelColor(i, pixels.Color(0, 255, 0));
            }
            pixels.show();
            break;
        case COUNTERCLOCKWISE:
            for (int i = 0; i < PIXELSCOUNT; i++)
            {
                pixels.setPixelColor(i, pixels.Color(0, 0, 255));
            }
            pixels.show();
            break;
        }
    }
}

void initDisplay(void)
{
    tft.begin();
    tft.setRotation(3);
    clearScreen();
}

void initPixels(void)
{
    pixels.begin();
    pixels.clear();
    pixels.show();
}

void showRainbow(void)
{
    for (int i = 1; i < 50; i++)
    {
        pixels.rainbow(0, i);
        pixels.show();
        delay(100);
    }
    pixels.clear();
    pixels.show();
}

void showRandomRoundPixels(void)
{
    // Determine the number of pixels to change (up to 3)
    int numPixelsToChange = min(3, roundPixelsCount);

    // Clear all pixels in roundPixels
    for (int i = 0; i < roundPixelsCount; i++)
    {
        pixels.setPixelColor(roundPixels[i], pixels.Color(0, 0, 0));
    }

    // Show the cleared pixels
    pixels.show();

    // Change color of up to 3 random pixels
    for (int i = 0; i < numPixelsToChange; i++)
    {
        // Get a random index within the roundPixels array
        int index = random(roundPixelsCount);
        // Set the color of the pixel at the random index
        pixels.setPixelColor(roundPixels[index], pixels.Color(0, 0, 32));
    }

    // Show the newly colored pixels
    pixels.show();
}

void clearScreen(void)
{
    tft.fillScreen(TFT_BLACK);
}

void initRotaryEncoder(void)
{
    pinMode(Rotary_Clock, INPUT_PULLUP);
    pinMode(Rotary_Data, INPUT_PULLUP);
    pinMode(Rotary_PushButton, INPUT_PULLUP);
    rotary_lastStateClock = digitalRead(Rotary_Clock);
}

void playDarthVadedBreathing(void)
{
    uint32_t pause;
    mp3.playTrackNumber(soundStartup, currentVolume, false);
    delay(500);
    for (int i = 0; i < 2; i++)
    {
        // Inspiration
        pause = 1096 / 255;
        for (int i = 0; i <= 255; i++)
        {
            pixels.setBrightness(i);
            setColorAllRoundPixels(pixels.Color(255, 0, 0));
            pixels.show();
            delay(pause);
        }
        // Expiration
        pause = 1550 / 255;
        for (int i = 255; i >= 0; i--)
        {
            pixels.setBrightness(i);
            setColorAllRoundPixels(pixels.Color(255, 0, 0));
            pixels.show();
            delay(pause);
        }
    }
    pixels.setBrightness(255);
}

void setColorAllRoundPixels(uint32_t color)
{
    for (int i = 0; i < roundPixelsCount; i++)
    {
        pixels.setPixelColor(roundPixels[i], color);
    }
}

boolean readRotaryPushButton(void)
{
    int buttonValue = digitalRead(Rotary_PushButton);
    if (buttonValue == LOW)
    {
        if (millis() - rotary_lastTimeButtonPress > PushButton_Debounce)
        {
            Serial.println("Button pressed!");
            // Remember last button press event
            rotary_lastTimeButtonPress = millis();
            return true;
        }
    }
    return false;
}

Rotary_Status readRotaryEncoder(void)
{
    Rotary_Status rotaryRead = NO_STATUS;
    // Read the current state of CLK
    int clockValue = digitalRead(Rotary_Clock);

    // If last and current state of Rotary_Clock are different, then "pulse occurred"
    // React to only 1 state change to avoid double count
    if (clockValue != rotary_lastStateClock && clockValue == 1 && (millis() - rotary_lastTurn > Rotary_Debounce))
    {
        // If the Rotary_Data state is different than the Rotary_Clock state then
        // the encoder is rotating "CCW" so we decrement
        int data = digitalRead(Rotary_Data);
        if (data != clockValue)
        {
            rotaryRead = CLOCKWISE;
        }
        else
        {

            rotaryRead = COUNTERCLOCKWISE;
        }
        rotary_lastTurn = millis();
    }
    rotary_lastStateClock = clockValue;
    return rotaryRead;
}

void drawHTTPIndicator(uint32_t color)
{
    tft.fillRect(0, tft.height() - 3, tft.width(), 3, color);
    tft.fillRect(50, tft.height() - 6, tft.width() - 100, 3, color);
}

void drawCenteredScreenText(const String &text, const GFXfont *f, uint32_t color)
{
    tft.setFreeFont(f);

    Serial.println(text);

    int displayWidth = tft.width();
    int displayHeight = tft.height();

    int textWidth = tft.textWidth(text);
    int textHeight = 160; // tft.fontHeight();

    int cursorX = (displayWidth - textWidth) / 2;
    int cursorY = (displayHeight - textHeight) / 2;

    tft.setTextColor(color);
    tft.drawString(text, cursorX, cursorY);
}
void drawCenteredHorizontalText(const String &text, int line, const GFXfont *f, uint32_t color)
{
    tft.setFreeFont(f);

    Serial.println(text);

    int displayWidth = tft.width();

    int textWidth = tft.textWidth(text);

    int cursorX = (displayWidth - textWidth) / 2;

    tft.setTextColor(color);
    tft.drawString(text, cursorX, line);
}

void fetchSubscriberCountIfNeeded()
{
    int sound;

    int subscriberCount;
    pixels.setPixelColor(gainSubscriberPixel, pixels.Color(0, 0, 255));
    pixels.show();
    if (getSubscriberCount(subscriberCount))
    {
        if (subscriberCount != currentSubscriberCount)
        {
            if (subscriberCount > currentSubscriberCount)
            {
                if (subscriberCount - currentSubscriberCount > 1)
                {
                    sound = soundTwoPlusSubscriber;
                }
                else
                {
                    sound = soundGainingSubscriber[(soundGainingSubscriberCount)];
                }
                currentSubscriberStatus = GAINING;
            }
            else
            {
                sound = soundLoosingSubscriber;
                currentSubscriberStatus = LOOSING;
            }
            mp3.playTrackNumber(sound, currentVolume, false);
            drawCenteredScreenText(String(currentSubscriberCount), aurebeshCounter, TFT_BLACK);
            currentSubscriberCount = subscriberCount;
            drawCenteredScreenText(String(currentSubscriberCount), aurebeshCounter, counterColor);
            showCurrentSubscriberStatus();
        }
    }
    else
    {
        Serial.println("Failed to get subscriber count.");
    }
    pixels.setPixelColor(gainSubscriberPixel, pixels.Color(0, 0, 0));
    pixels.show();
}

void showCurrentSubscriberStatus(void)
{
    pixels.setPixelColor(gainSubscriberPixel, pixels.Color(0, 0, 0));
    pixels.setPixelColor(looseSubscriberPixel, pixels.Color(0, 0, 0));

    switch (currentSubscriberStatus)
    {
    case GAINING:
        pixels.setPixelColor(gainSubscriberPixel, pixels.Color(0, 255, 0));
        break;

    case LOOSING:
        pixels.setPixelColor(looseSubscriberPixel, pixels.Color(255, 0, 0));
        /* code */
        break;
    }
    pixels.show();
}

bool getSubscriberCount(int &subscriberCount)
{
    HTTPClient http;

    String url = "https://youtube.googleapis.com/youtube/v3/channels?part=statistics";
    url += "&id=";
    url += CHANNEL_ID;
    url += "&fields=items/statistics/subscriberCount";
    url += "&key=";
    url += APIKEY;

    http.begin(url);
    int httpResponseCode = http.GET();

    // If the request was successful, httpResponseCode will be a number > 0
    if (httpResponseCode > 0)
    {
        String response = http.getString();
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        subscriberCount = doc["items"][0]["statistics"]["subscriberCount"].as<int>();
        // Serial.println(httpResponseCode); // Print HTTP return code
        // Serial.println(response);         // Print request response payload
        // Serial.println(subscriberCount);  // Print subscriber count
        http.end();
        drawHTTPIndicator(TFT_GREEN);
        return true;
    }
    else
    {
        Serial.print("Error on sending request: ");
        Serial.println(httpResponseCode);
        http.end();
        drawHTTPIndicator(TFT_RED);
        return false;
    }
}
