#include <Adafruit_NeoPixel.h>  // Install with the library manager in the Arduino IDE - Tested with version 1.12.3
#include <HTTPClient.h>         // Included with Espressif ESP32 Core - Tested with version 3.0.7
#include <ArduinoJson.h>        // Install with the library manager in the Arduino IDE - Tested with version 7.2.1
#include <Preferences.h>        // Included with Espressif ESP32 Core - Tested with version 3.0.7
#include <SimpleRotary.h>       // Install with the library manager in the Arduino IDE - Tested with version 1.1.3
#include "counterWeb.h"
#include <TFT_eSPI.h>           // Install with the library manager in the Arduino IDE - Tested with version 2.5.43 Driver to use : ILI9488
#include "mp3tf16p.h"
#include "scheduler.h"
#include "interface.h"
#include "darth_vader_helmet.h"
#include "font/Aurebesh_Bold80pt7b.h"
#include "font/Aurebesh_Bold30pt7b.h"
#include "font/hansolov355pt7b.h"

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
int soundGainingSubscriber[] = {2, 4, 6, 11}; //Choosen randomly for more fun!
int darthVaderBreathingSound = 12;
int soundGainingSubscriberCount = 4;
int soundLoosingSubscriber = 3;
int soundTwoPlusSubscriber = 9;
int volumeChangeFeedback = 5;

// Rotary Encoder
#define ROTARY_PIN_A 27
#define ROTARY_PIN_B 26
#define ROTARY_PUSH_BUTTON 14
SimpleRotary rotary(ROTARY_PIN_A, ROTARY_PIN_B, ROTARY_PUSH_BUTTON);

// Fonts
const GFXfont *aurebeshCounter = &hansolov355pt7b;
const GFXfont *aurebeshText = &Aurebesh_Bold30pt7b;

// Colors
const uint32_t counterColor = TFT_WHITE;

// Youtube Statistics
int currentViewCount = -1;
int currentSubscriberCount = -1;
enum Subscriber_Status
{
    NOCHANGE,
    LOOSING,
    GAINING,
};
Subscriber_Status currentSubscriberStatus = NOCHANGE;

// Interface activated by the Rotary Encoder
InterfaceMode interface;

// Task Scheduler called in the Loop function
TaskScheduler scheduler;

// Preferences to store values into non-volatile memory
Preferences prefs;
const char *volumePreference = "volume";
const char *channelIdPreference = "channel_id";
const char *apiKeyPreference = "api_key";


void setup()
{
    Serial.begin(115200);

    initPixels();
    initDisplay();
    mp3.initialize();

    prefs.begin("youtube");
    currentVolume = prefs.getInt(volumePreference, 15);

    drawCenteredHorizontalText("Connect", 80, aurebeshText, TFT_DARKGREY);
    drawCenteredHorizontalText("Wifi", 160, aurebeshText, TFT_DARKGREY);
    initWebServer();
    getYoutubeStatistics(currentSubscriberCount);
    playDarthVadedBreathing();

    // Scheduled tasks in milliseconds
    scheduler.addTask(showRandomRoundPixels, 10000L);
    scheduler.addTask(fetchSubscriberCount, 300000L);
    scheduler.addTask(playDarthVadedBreathing, 3600000L);
    scheduler.addTask(showFastRandomPixels, 950000L);

    // Browser request will be handle by a separate task
    xTaskCreatePinnedToCore(
        handleBrowserCalls,   /* Function to implement the task */
        "handleBrowserCalls", /* Name of the task */
        10000,        /* Stack size in words */
        NULL,         /* Task input parameter */
        1,            /* Priority of the task */
        NULL,         /* Task handle. */
        1);           /* Core where the task should run */
}

void loop()
{
    scheduler.runTasks();
    readInterfaceThroughRotaryEncoder();

    if (rotary.push() == 1)
    {
        // User has acknowledge the change in subscriber count
        currentSubscriberStatus = NOCHANGE;
        interface.setModeNormal();
        showCurrentSubscriberStatus();
        showSubscriberCount();
    }

    if (interface.checkReset())
    {
        showSubscriberCount();
    }
}

// Generates a true random value between minVal (inclusive) and maxVal (exclusive) using ESP32's hardware RNG.
uint32_t espRandomInRange(uint32_t minVal, uint32_t maxVal) {
    return (esp_random() % (maxVal - minVal)) + minVal;
}

// This runs on core 0 for responsiveness
void handleBrowserCalls(void * parameter)
{
    for(;;)
    {
        server.handleClient();
        delay(1); // allow other tasks to run
    }
}

// Interface functions
//

void incrementVolume()
{
    currentVolume++;
    if (currentVolume > 30)
    {
        currentVolume = 0;
    }
    prefs.putInt(volumePreference, currentVolume);
}

void decrementVolume()
{
    currentVolume--;
    if (currentVolume < 0)
    {
        currentVolume = 30;
    }
    prefs.putInt(volumePreference, currentVolume);
}

// Show the interface when the Rotary encoder is used
void readInterfaceThroughRotaryEncoder(void)
{
    int rotary_Value = rotary.rotate();
    if (rotary_Value != 0)
    {
        switch (rotary_Value)
        {
        case 1:
            interface.nextMode();
            break;
        case 2:
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
            while (!interface.checkReset() && rotary.push() == 0)
            {
                rotary_Value = rotary.rotate();
                switch (rotary_Value)
                {
                case 1:
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, TFT_BLACK);
                    incrementVolume();
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, counterColor);
                    mp3.playTrackNumber(volumeChangeFeedback, currentVolume, false);
                    interface.resetTime();
                    break;
                case 2:
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, TFT_BLACK);
                    decrementVolume();
                    drawCenteredHorizontalText(String(currentVolume), 160, aurebeshText, counterColor);
                    mp3.playTrackNumber(volumeChangeFeedback, currentVolume, false);
                    interface.resetTime();
                    break;
                }
            }
            interface.setModeNormal();
            showSubscriberCount();
            break;
        }
    }
}

// Display Functions
//
void initDisplay(void)
{
    tft.begin();
    tft.setRotation(3);
    clearScreen();
}

void drawTactical()
{
    int lineSpacing = tft.width() / 7; // Divide by 7 to get 6 spaces
    for (int i = 1; i <= 6; i++)
    {
        tft.drawLine(i * lineSpacing, 0, i * lineSpacing, tft.height(), TFT_DARKGREEN);
    }

    for (int i = 1; i <= 4; i++)
    {
        tft.drawCircle(50, tft.height() / 2, i * 50, TFT_DARKGREEN); // Radius is i * 75 to get a maximum radius of 300
    }

    tft.drawEllipse(tft.width() / 4, tft.height() / 2, tft.width(), tft.height() / 2, TFT_DARKGREEN);

    tft.drawEllipse(tft.width() / 4, tft.height() / 2, tft.width() / 2, tft.height() / 4, TFT_DARKGREEN);
    tft.drawEllipse(tft.width() / 4, tft.height() / 2, tft.width() / 3, tft.height() / 8, TFT_DARKGREEN);
}

void showViewCount(void)
{
    clearScreen();
    drawTactical();
    drawCenteredHorizontalText("ViewCount", 80, aurebeshText, TFT_DARKGREY);
    drawCenteredHorizontalText(String(currentViewCount), 160, aurebeshText, counterColor);
}

void drawHTTPIndicator(uint32_t color)
{
    int lineSpacing = tft.width() / 7; // Divide by 7 to get 6 spaces
    for (int i = 1; i <= 6; i++)
    {
        tft.fillRect((i - 1) * lineSpacing + 10, tft.height() - 10, espRandomInRange(15, 30), 10, color);
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

// Neopixels functions
//
void initPixels(void)
{
    pixels.begin();
    pixels.clear();
    pixels.show();
}

void clearScreen(void)
{
    tft.fillScreen(TFT_BLACK);
}

void showSubscriberCount(void)
{
    clearScreen();
    drawTactical();

    String displayCount;

    // Format subscriber count
    if (currentSubscriberCount >= 10000) {
        float formattedCount = currentSubscriberCount / 1000.0; // Convert to thousands
        displayCount = String(formattedCount, 1) + "K"; // 1 decimal place + "K"
    } else {
        displayCount = String(currentSubscriberCount); // Use the plain number for smaller counts
    }

    drawCenteredScreenText(displayCount, aurebeshCounter, counterColor);
}

void showRainbowPixels(void)
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
    for (int i = 0; i < pixels.numPixels(); i++)
    {
        pixels.setPixelColor(i, pixels.Color(255, 165, 0));
    }
    pixels.show();

    unsigned long c = millis();

    while (millis() - c < 4000)
    {
        delay(50);
        pixels.setBrightness(espRandomInRange(0,255));
        pixels.show();
    }

    pixels.clear();
    pixels.show();
}

void showFastRandomPixels(void)
{
    unsigned long c = millis();
    while (millis() - c < 10000)
    {
        showRandomRoundPixels();
        delay(200);
    }
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
        int index = espRandomInRange(0,roundPixelsCount);
        // Set the color of the pixel at the random index
        pixels.setPixelColor(roundPixels[index], pixels.Color(0, 0, 32));
    }

    // Show the newly colored pixels
    pixels.show();
    showCurrentSubscriberStatus();
}

void setColorAllRoundPixels(uint32_t color)
{
    for (int i = 0; i < roundPixelsCount; i++)
    {
        pixels.setPixelColor(roundPixels[i], color);
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

// MP3 Functions
//
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

// Youtube Statistics functions
//
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
            if (newSubscriberCount - currentSubscriberCount > 1 && currentSubscriberCount < 1000)
            {
                sound = soundTwoPlusSubscriber;
            }
            else
            {
                sound = soundGainingSubscriber[espRandomInRange(0,soundGainingSubscriberCount)];
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
            showRainbowPixels();
        }
        else
        {
            showLoosingSubscriberPixels();
        }
        showSubscriberCount();
        showCurrentSubscriberStatus();
    }
}

// Google Youtube API calls
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
