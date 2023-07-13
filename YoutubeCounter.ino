#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "font/Aurebesh_Bold80pt7b.h"
#include "counterWeb.h"

#ifdef __AVR__
#include <avr/power.h>
#endif
#define PIN 12
#define NUMPIXELS 2

TFT_eSPI tft = TFT_eSPI();
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500

unsigned long lastFetchTime = 0;
const unsigned long fetchInterval = 5 * 60 * 1000; // 5 minutes in milliseconds
int currentSubscriberCount;

void setup()
{
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif
    pixels.begin();
    pixels.clear();

    Serial.begin(115200);
    initWebServer();

    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    if (getSubscriberCount(currentSubscriberCount))
    {
        drawCenteredString(String(currentSubscriberCount), &Aurebesh_Bold80pt7b);
    }
}

void loop()
{
    server.handleClient();
    fetchSubscriberCountIfNeeded();

    // for (int i = 0; i < NUMPIXELS; i++)
    // {
    //     pixels.setPixelColor(i, pixels.Color(255, 0, 0));
    // }
    // pixels.show();
    // delay(DELAYVAL);
    // for (int i = 0; i < NUMPIXELS; i++)
    // {
    //     pixels.setPixelColor(i, pixels.Color(0, 0, 255));
    // }
    // pixels.show();
    // delay(DELAYVAL);
    // pixels.clear();
    // pixels.show();
    // delay(DELAYVAL);
}

void drawHTTPIndicator(uint32_t color)
{
    tft.fillRect(0, tft.height()-3, tft.width(), 3, color);
    tft.fillRect(75, tft.height()-6, 25, 3, color);
}

void drawCenteredString(const String &text, const GFXfont *f)
{
    // Set the desired font
    tft.setFreeFont(f);

    Serial.println(text);

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

void fetchSubscriberCountIfNeeded()
{
    unsigned long currentTime = millis();
    if (currentTime - lastFetchTime >= fetchInterval)
    {
        lastFetchTime = currentTime;
        int subscriberCount;
        if (getSubscriberCount(subscriberCount))
        {
            // Serial.println("Subscriber count: " + String(subscriberCount));
            if (subscriberCount != currentSubscriberCount)
            {
                currentSubscriberCount = subscriberCount;
                tft.fillScreen(TFT_BLACK);
                drawCenteredString(String(currentSubscriberCount), &Aurebesh_Bold80pt7b);
            }
        }
        else
        {
            Serial.println("Failed to get subscriber count.");
        }
    }
}

bool getSubscriberCount(int &subscriberCount)
{
    // Declare an object of class HTTPClient
    HTTPClient http;

    // Specify request destination
    String url = "https://youtube.googleapis.com/youtube/v3/channels?part=statistics";
    url += "&id=";
    url += CHANNEL_ID;
    url += "&fields=items/statistics/subscriberCount";
    url += "&key=";
    url += APIKEY;

    Serial.println(url);

    // Initialize the connection
    http.begin(url);

    // Send the request
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
