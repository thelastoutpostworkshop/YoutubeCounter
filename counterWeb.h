// Web server Functions
//
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "secrets.h"

WebServer server(80);

// Web server host name
const char *hostName = "youtube";

// CSS for the web application
const char *css = "<style>\
                          .loading {\
                            background-color: #f3f3f3;\
                            cursor: not-allowed;\
                          }\
                          .spinner {\
                            display: inline-block;\
                            width: 40px;\
                            height: 40px;\
                            border: 5px solid rgba(0,0,0,.2);\
                            border-top-color: #008000;\
                            border-radius: 100%;\
                            animation: spin 1s linear infinite;\
                          }\
                          @keyframes spin {\
                            to { transform: rotate(360deg); }\
                          }\
                          .column {\
                            width: 50%;\
                            padding: 10px;\
                            box-sizing: border-box;\
                          }\
                          .separator {\
                            width: 2px;\
                            background: #ddd;\
                            height: 100%;\
                            position: absolute;\
                            left: 50%;\
                            top: 0;\
                          }\
                          .left { float: left; }\
                          .right { float: right; }\
                          h1 { text-align: center; }\
                    .label {\
                      font-weight: bold;\
                      font-size: 25px;\
                    }\
                    input[type=\"text\"] {\
                      background-color: #f3f3f3;\
                      border: 1px solid #ccc;\
                      padding: 10px;\
                      width: 50%;\
                      box-sizing: border-box;\
                      font-size: 25px;\
                    }\
                    input[type=\"submit\"] {\
                      background-color: #008800;\
                      color: white;\
                      border: none;\
                      padding: 10px;\
                      text-transform: uppercase;\
                      cursor: pointer;\
                      font-size: 25px;\
                      margin-top: 10px;\
                      width: 50%;\
                    }\
                    input[type=\"submit\"]:hover {\
                      background-color: #006600;\
                    }\
                          button {\
                            font-size: 40px;\
                            padding: 15px;\
                            width: 90%;\
                            box-sizing: border-box;\
                            margin: 20px 5%;\
                            border-radius: 25px;\
                            transition: background-color 0.3s, transform 0.3s;\
                          }\
                          button:hover {\
                            background-color: #ddd;\
                          }\
                          button:active {\
                            transform: scale(0.95);\
                          }\
                          </style>";

const char *htmlPageUpdate =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<style>"
    "body {"
    "    font-size: 40px;" /* Change the size as needed */
    "}"
    "input[type=submit], input[type=file] {"
    "    font-size: 40px;"    /* Change the size as needed */
    "    padding: 10px 20px;" /* Change the padding as needed */
    "}"
    "progress {"
    "    width: 100%;"
    "    height: 100px;" /* Increase the height as needed */
    "}"
    "</style>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<progress id='prg' value='0' max='100'></progress>"
    "<script>"
    "$('form').submit(function(e){"
    "    e.preventDefault();"
    "    var form = $('#upload_form')[0];"
    "    var data = new FormData(form);"
    "    $.ajax({"
    "        url: '/upload',"
    "        type: 'POST',"
    "        data: data,"
    "        contentType: false,"
    "        processData:false,"
    "        xhr: function() {"
    "            var xhr = new window.XMLHttpRequest();"
    "            xhr.upload.addEventListener('progress', function(evt) {"
    "                if (evt.lengthComputable) {"
    "                    var per = evt.loaded / evt.total;"
    "                    $('#prg').val(Math.round(per*100));"
    "                }"
    "            }, false);"
    "            return xhr;"
    "        },"
    "        success:function(d, s) {"
    "            console.log('success!')"
    "        },"
    "        error: function (a, b, c) {"
    "        }"
    "    });"
    "});"
    "</script>";

// Structure to store commands available on the web application
struct Command
{
    const char *name;
    const char *endpoint;
    void (*handler)();
};

// List of Commands available on the web application
void handleHello(void);
void handleUpdate(void);
void handleShowRainbow(void);
void handleShowFastRandom(void);
void handleDarthVaderBreathing(void);
void handleDemoPlusOneSubscriber(void);
void handleDemoMinusOneSubscriber(void);
void handleDemoPlusTwoSubscriber(void);
void handleConfigureYoutubeSettings(void);

// List of external functions and variables needed by the web application
void showRainbowPixels();
void playDarthVadedBreathing(void);
void applyNewSubscriberCount(int);
void showFastRandomPixels(void);
extern int currentSubscriberCount;
extern Preferences prefs;
extern const char *channelIdPreference;
extern const char *apiKeyPreference;

// Fetch commands name, route and handling function to be called
Command fetchCommands[] = {
    {"Home", "/", handleHello},
    {"Rainbow", "/show_rainbow", handleShowRainbow},
    {"Fast Random", "/show_fastrandom", handleShowFastRandom},
    {"Darth Vader Breathing", "/darth_vader", handleDarthVaderBreathing},
    {"Demo Add 1 Subscriber", "/demo_plus_one_subscriber", handleDemoPlusOneSubscriber},
    {"Demo Remove 1 Subscriber", "/demo_minus_one_subscriber", handleDemoMinusOneSubscriber},
    {"Demo Add 2 Subscriber", "/demo_plus_two_subscriber", handleDemoPlusTwoSubscriber},
};

// Post commands name, route and handling function to be called
Command postCommands[] = {
    {"Upload code", "/update", handleUpdate},
    {"Youtube settings", "/youtube_settings", handleConfigureYoutubeSettings},
};

// Functions to handle post data by the forms on the web application
const char *uploadEndpoint = "/upload";
const char *saveYoutubeSettingsEndpoint = "/save_youtube_settings";

// Build a list of commands available to display on the web application
String commandsList(void)
{
    String commandList = css;

    commandList += "<div class='separator'></div>\
                          <div class='column left'>\
                          <h1>Commands</h1>";
    for (Command &cmd : fetchCommands)
    {
        if (cmd.endpoint == "/update")
        {
            commandList += "<button style='font-size: 40px; padding: 15px; width: 90%; box-sizing: border-box; margin: 20px 5%; border-radius: 25px;' onclick=\"location.href='";
            commandList += cmd.endpoint;
            commandList += "'\">";
        }
        else
        {
            commandList += "<button id='";
            commandList += cmd.endpoint;
            commandList += "' style='font-size: 40px; padding: 15px; width: 90%; box-sizing: border-box; margin: 20px 5%; border-radius: 25px;' onclick='this.classList.add(\"loading\"); this.innerHTML=\"<div class=spinner></div>\"; fetch(\"";
            commandList += cmd.endpoint;
            commandList += "\").then(() => { this.classList.remove(\"loading\"); this.textContent=\"";
            commandList += cmd.name;
            commandList += "\"; })'>";
        }

        commandList += cmd.name;
        commandList += "</button><br/>";
    }
    commandList += "</div>\
                    <div class='column right'>\
                    <h1>Settings Configuration</h1>";
    for (Command &cmd : postCommands)
    {

        commandList += "<button style='font-size: 40px; padding: 15px; width: 90%; box-sizing: border-box; margin: 20px 5%; border-radius: 25px;' onclick=\"location.href='";
        commandList += cmd.endpoint;
        commandList += "'\">";

        commandList += cmd.name;
        commandList += "</button><br/>";
    }
    commandList += "</div>";

    return commandList;
}

void sendPlainText(const char *fmt, ...)
{
    char buf[256]; // Buffer to hold the formatted string. Adjust the size as needed.
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    server.send(200, "text/plain", buf);
}

void handleHello(void)
{
    String body = "<div style='border: 2px solid #008800; background-color: #e6f7e6; padding: 10px; border-radius: 10px; margin: 10px; font-size: 24px;'>\
                   URL: " +
                  String(hostName) + ".local<br/>" +
                  "IP address: " + WiFi.localIP().toString() + "<br/>" +
                  "Connected to: " + String(ssid) +
                  "</div>" + commandsList();
    server.send(200, "text/html", body);
}

void handleConfigureYoutubeSettings(void)
{
    server.sendHeader("Connection", "close");

    String html = css;
    html += "<form action='";
    html += saveYoutubeSettingsEndpoint;
    html += "' method=\"post\">\
                       <label class='label'>Channel ID:</label><br>\
                       <input type=\"text\" name=\"channel_id\" size=\"50\">\
                       <br>\
                       <label class='label'>API Key:</label><br>\
                       <input type=\"text\" name=\"api_key\" size=\"50\">\
                       <br><br>\
                       <input type=\"submit\" value=\"Submit\">\
                     </form>";
    server.send(200, "text/html", html);
}

void handleDemoPlusOneSubscriber(void)
{
    applyNewSubscriberCount(currentSubscriberCount + 1);
    server.send(200);
}
void handleDemoMinusOneSubscriber(void)
{
    applyNewSubscriberCount(currentSubscriberCount - 1);
    server.send(200);
}
void handleDemoPlusTwoSubscriber(void)
{
    applyNewSubscriberCount(currentSubscriberCount + 2);
    server.send(200);
}
void handleShowRainbow(void)
{
    showRainbowPixels();
    server.send(200);
}
void handleShowFastRandom(void)
{
    showFastRandomPixels();
    server.send(200);
}
void handleDarthVaderBreathing(void)
{
    playDarthVadedBreathing();
    server.send(200);
}

void handleUpdate(void)
{
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", htmlPageUpdate);
}

void handleNotFound()
{
    String message = "Command Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

// Setup of routes for handling requests coming from the web application
void setupCommands(void)
{
    // Register the command handlers with the web server
    for (Command &cmd : fetchCommands)
    {
        server.on(cmd.endpoint, HTTP_GET, cmd.handler);
    }
    for (Command &cmd : postCommands)
    {
        server.on(cmd.endpoint, HTTP_GET, cmd.handler);
    }

    server.onNotFound(handleNotFound);

    /*handling uploading firmware file */
    server.on(
        uploadEndpoint, HTTP_POST, []()
        {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
        []()
        {
            HTTPUpload &upload = server.upload();
            if (upload.status == UPLOAD_FILE_START)
            {
                Serial.printf("Update: %s\n", upload.filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                { // start with max available size
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE)
            {
                /* flashing firmware to ESP*/
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                }
                else
                {
                    Update.printError(Serial);
                }
            }
        });

    // Handling configuration of youtube settings
    server.on(saveYoutubeSettingsEndpoint, HTTP_POST, []()
              {
    if (server.hasArg("channel_id") && server.hasArg("api_key")) {
      String channelId = server.arg("channel_id");
      String apiKey = server.arg("api_key");

      prefs.putString(channelIdPreference,channelId);
      prefs.putString(apiKeyPreference,apiKey);
      
      server.send(200, "text/plain", "Data stored successfully.");
    } else {
      server.send(400, "text/plain", "Missing data in request.");
    } });
}

void initWebServer()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin(hostName))
    {
        Serial.println("MDNS responder started");
    }

    setupCommands();

    server.begin();
    Serial.println("HTTP server started");
}
