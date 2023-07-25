#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "font/Aurebesh_Bold80pt7b.h"
#include "font/Aurebesh_Bold40pt7b.h"
#include "font/Aurebesh_Bold30pt7b.h"
#include "counterWeb.h"
#include "mp3tf16p.h"
#include "scheduler.h"
#include "interface.h"
#include "darth_vader_helmet.h"

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
int soundGainingSubscriber[] = {2, 3, 4, 6, 11};
int darthVaderBreathingSound = 12;
int soundGainingSubscriberCount = 5;
int soundLoosingSubscriber = 3;
int soundTwoPlusSubscriber = 9;
int volumeChangeFeedback = 5;
int test = 1;

// Rotary Encoder
#define Rotary_Clock 27
#define Rotary_Data 26
#define Rotary_PushButton 14
#define PushButton_Debounce 200
#define Rotary_Debounce 50
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
const GFXfont *aurebeshText = &Aurebesh_Bold30pt7b;
const uint32_t counterColor = TFT_WHITE;

// Youtube Statistics
int currentViewCount = -1;
int currentSubscriberCount = -1;
enum Subscriber_Status
{
    UNKNNOWN,
    LOOSING,
    GAINING,
};
Subscriber_Status currentSubscriberStatus = UNKNNOWN;

// Interface activated by the Rotary Encoder
InterfaceMode interface;

// Task Scheduler
TaskScheduler scheduler;

// Preferences to store values into non-volatile memory
Preferences prefs;
const char *volumePreference = "volume";
const char *channelIdPreference = "channel_id";
const char *apiKeyPreference = "api_key";

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

    prefs.begin("youtube");
    currentVolume = prefs.getInt(volumePreference, 15);

    drawCenteredHorizontalText("Connect", 80, aurebeshText, TFT_DARKGREY);
    drawCenteredHorizontalText("Wifi", 160, aurebeshText, TFT_DARKGREY);
    initWebServer();
    getYoutubeStatistics(currentSubscriberCount);
    playDarthVadedBreathing();

    scheduler.addTask(showRandomRoundPixels, 10000L);
    scheduler.addTask(fetchSubscriberCount, 300000L);
    scheduler.addTask(playDarthVadedBreathing, 3600000L);
    // scheduler.addTask(showRainbow, 950000L);
}

void loop()
{
    server.handleClient();
    scheduler.runTasks();
    readInterfaceThroughRotaryEncoder();

    if (readRotaryPushButton())
    {
        currentSubscriberStatus = UNKNNOWN;
        showCurrentSubscriberStatus();
    }

    if (interface.checkReset())
    {
        showSubscriberCount();
    }
}

// Increment the volume
void incrementVolume()
{
    currentVolume++;
    if (currentVolume > 30)
    {
        currentVolume = 0;
    }
    prefs.putInt(volumePreference, currentVolume);
}

// Decrement the volume
void decrementVolume()
{
    currentVolume--;
    if (currentVolume < 0)
    {
        currentVolume = 30;
    }
    prefs.putInt(volumePreference, currentVolume);
}

void readInterfaceThroughRotaryEncoder(void)
{
    Rotary_Status rotary = readRotaryEncoder();
    if (rotary != NO_STATUS)
    {

        switch (rotary)
        {
        case CLOCKWISE:
            interface.nextMode();

            break;
        case COUNTERCLOCKWISE:
            interface.prevMode();

            break;
        }
        switch (interface.getMode())
        {
        case NORMAL:
            showSubscriberCount();
            break;
        case VIEWCOUNT:
            showViewCount();
            break;
        case VOLUME:
            clearScreen();
            drawTactical();
            drawCenteredHorizontalText("Volume", 80, aurebeshText, TFT_DARKGREY);
            drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, counterColor);
            while (!interface.checkReset() && !readRotaryPushButton())
            {
                rotary = readRotaryEncoder();
                switch (rotary)
                {
                case CLOCKWISE:
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, TFT_BLACK);
                    incrementVolume();
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, counterColor);
                    mp3.playTrackNumber(volumeChangeFeedback, currentVolume, false);
                    interface.resetTime();
                    break;
                case COUNTERCLOCKWISE:
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, TFT_BLACK);
                    decrementVolume();
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, counterColor);
                    mp3.playTrackNumber(volumeChangeFeedback, currentVolume, false);
                    interface.resetTime();
                    break;
                }
            }
            showSubscriberCount();
            break;
        }
    }
}

void drawTactical()
{
    // Draw 6 vertical lines spaced evenly
    int lineSpacing = tft.width() / 7; // Divide by 7 to get 6 spaces
    for (int i = 1; i <= 6; i++)
    {
        tft.drawLine(i * lineSpacing, 0, i * lineSpacing, tft.height(), TFT_DARKGREEN);
    }

    // Draw 4 concentric circles to the left of the screen
    for (int i = 1; i <= 4; i++)
    {
        tft.drawCircle(50, tft.height() / 2, i * 50, TFT_DARKGREEN); // Radius is i * 75 to get a maximum radius of 300
    }

    // Draw a large ellipse, where one end is on the right and going off the screen on the left
    tft.drawEllipse(tft.width() / 4, tft.height() / 2, tft.width(), tft.height() / 2, TFT_DARKGREEN);

    // Draw two ellipses of this kind
    tft.drawEllipse(tft.width() / 4, tft.height() / 2, tft.width() / 2, tft.height() / 4, TFT_DARKGREEN);
    tft.drawEllipse(tft.width() / 4, tft.height() / 2, tft.width() / 3, tft.height() / 8, TFT_DARKGREEN);
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

void showSubscriberCount(void)
{
    clearScreen();
    drawTactical();
    drawCenteredScreenText(String(currentSubscriberCount), aurebeshCounter, counterColor);
}
void showViewCount(void)
{
    clearScreen();
    drawTactical();
    drawCenteredHorizontalText("ViewCount", 80, aurebeshText, TFT_DARKGREY);
    drawCenteredHorizontalText(String(currentViewCount), 160, aurebeshText, counterColor);
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

void showLoosingSubscriberPixels(void)
{
    unsigned long c = millis();

    while (millis() - c < 4000)
    {
        for (int i = 0; i < pixels.numPixels(); i++)
        {
            int red = map(i, 0, pixels.numPixels(), 255, 0);
            int green = map(i, 0, pixels.numPixels(), 0, 255);
            int blue = 0;
            pixels.setPixelColor(i, pixels.Color(red, green, blue));
        }
        pixels.show();
        delay(50);

        for (int i = 0; i < pixels.numPixels(); i++)
        {
            int red = map(i, 0, pixels.numPixels(), 0, 255);
            int green = map(i, 0, pixels.numPixels(), 255, 0);
            int blue = 0;
            pixels.setPixelColor(i, pixels.Color(red, green, blue));
        }
        pixels.show();
        delay(50);
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
    showCurrentSubscriberStatus();
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
    clearScreen();
    tft.drawBitmap((tft.width() - 344) / 2, 0, darth_vader_helmet, 344, 320, TFT_RED);

    mp3.playTrackNumber(darthVaderBreathingSound, currentVolume, false);
    delay(500);
    for (int i = 0; i < 2; i++)
    {
        // Inspiration
        pause = 1096 / 255;
        for (int i = 0; i <= 255; i++)
        {
            pixels.setBrightness(i);
            setColorAllRoundPixels(pixels.Color(255, 0, 0));
            pixels.setPixelColor(looseSubscriberPixel, pixels.Color(255, 0, 0));
            pixels.setPixelColor(gainSubscriberPixel, pixels.Color(255, 0, 0));
            pixels.show();
            delay(pause);
        }
        // Expiration
        pause = 1550 / 255;
        for (int i = 255; i >= 0; i--)
        {
            pixels.setBrightness(i);
            setColorAllRoundPixels(pixels.Color(255, 0, 0));
            pixels.setPixelColor(looseSubscriberPixel, pixels.Color(255, 0, 0));
            pixels.setPixelColor(gainSubscriberPixel, pixels.Color(255, 0, 0));
            pixels.show();
            delay(pause);
        }
    }
    pixels.setBrightness(255);
    pixels.clear();
    pixels.show();
    clearScreen();
    showSubscriberCount();
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
    int lineSpacing = tft.width() / 7; // Divide by 7 to get 6 spaces
    for (int i = 1; i <= 6; i++)
    {
        tft.fillRoundRect((i - 1) * lineSpacing + 10, tft.height() - 10, random(20, 40), 10, 10, color);
    }
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

void fetchSubscriberCount()
{
    int subscriberCount;
    if (getYoutubeStatistics(subscriberCount))
    {
        applyNewSubscriberCount(subscriberCount);
    }
    else
    {
        Serial.println("Failed to get subscriber count.");
    }
}

void applyNewSubscriberCount(int newSubscriberCount)
{
    int sound;
    if (newSubscriberCount != currentSubscriberCount)
    {
        if (newSubscriberCount > currentSubscriberCount)
        {
            if (newSubscriberCount - currentSubscriberCount > 1)
            {
                sound = soundTwoPlusSubscriber;
            }
            else
            {
                sound = soundGainingSubscriber[random(soundGainingSubscriberCount)];
            }
            currentSubscriberStatus = GAINING;
            currentSubscriberCount = newSubscriberCount;
        }
        else
        {
            sound = soundLoosingSubscriber;
            currentSubscriberStatus = LOOSING;
            currentSubscriberCount = newSubscriberCount;
        }
        mp3.playTrackNumber(sound, currentVolume, false);
        if (currentSubscriberStatus == GAINING)
        {
            showRainbow();
        }
        else
        {
            showLoosingSubscriberPixels();
        }
        showSubscriberCount();
        showCurrentSubscriberStatus();
    }
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

bool getYoutubeStatistics(int &subscriberCount)
{
    HTTPClient http;

    String url = "https://youtube.googleapis.com/youtube/v3/channels?part=statistics";
    url += "&id=";
    url += prefs.getString(channelIdPreference);
    url += "&fields=items/statistics/subscriberCount,items/statistics/viewCount";
    url += "&key=";
    url += prefs.getString(apiKeyPreference);

    http.begin(url);
    int httpResponseCode = http.GET();

    // If the request was successful, httpResponseCode will be a number > 0
    if (httpResponseCode > 0)
    {
        String response = http.getString();
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);

        // Check for errors in deserialization
        if (error)
        {
            Serial.println(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            http.end();
            drawHTTPIndicator(TFT_RED);
            return false;
        }

        JsonObject root = doc.as<JsonObject>();

        // Check if the JSON response contains the expected keys
        if (root.containsKey("items") && root["items"].as<JsonArray>().size() > 0 &&
            root["items"][0].as<JsonObject>().containsKey("statistics") &&
            root["items"][0]["statistics"].as<JsonObject>().containsKey("subscriberCount"))
        {
            subscriberCount = root["items"][0]["statistics"]["subscriberCount"].as<int>();
            currentViewCount = root["items"][0]["statistics"]["viewCount"].as<int>();
            http.end();
            drawHTTPIndicator(TFT_GREEN);
            return true;
        }
        else
        {
            Serial.println("JSON response did not contain expected keys.");
            http.end();
            drawHTTPIndicator(TFT_RED);
            return false;
        }
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
