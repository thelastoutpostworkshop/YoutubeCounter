// Web Functions
//
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include "secrets.h"

const char *hostName = "youtube";

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

WebServer server(80);

// Define a command structure
struct Command
{
    const char *endpoint;
    void (*handler)();
};

// List of Commands
void handleHello(void);
void handleUpdate(void);
void handleShowRainbow(void);
void handleDarthVaderBreathing(void);
void handleDemoPlusOneSubscriber(void);
void handleDemoMinusOneSubscriber(void);
void handleDemoPlusTwoSubscriber(void);

// List of external functions
void showRainbow();
void playDarthVadedBreathing(void);
void applyNewSubscriberCount(int);
extern int currentSubscriberCount;

const char *updateCommand = "/update";
const char *showRainbowCommand = "/show_rainbow";
const char *playDarthVaderBreathingCommand = "/darth_vader";
const char *demoPlusOneSubscriberCommand = "/demo_plus_one_subscriber";
const char *demoMinusOneSubscriberCommand = "/demo_minus_one_subscriber";
const char *demoPlusTwoSubscriberCommand = "/demo_plus_two_subscriber";

Command commands[] = {
    {"/", handleHello},
    {updateCommand, handleUpdate},
    {showRainbowCommand, handleShowRainbow},
    {playDarthVaderBreathingCommand, handleDarthVaderBreathing},
    {demoPlusOneSubscriberCommand, handleDemoPlusOneSubscriber},
    {demoMinusOneSubscriberCommand, handleDemoMinusOneSubscriber},
    {demoPlusTwoSubscriberCommand, handleDemoPlusTwoSubscriber},
    //... add more commands as needed
};

String commandsList(void)
{
    String commandList = "<style>\
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
                          </style>\
                          Available commands:<br/>";
    for (Command &cmd : commands)
    {
        if (cmd.endpoint == updateCommand)
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
            commandList += cmd.endpoint;
            commandList += "\"; })'>";
        }

        commandList += cmd.endpoint;
        commandList += "</button><br/>";
    }
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
    String body = "<p style='font-size: 24px;'>Host: " + String(hostName) + "</p>" +
                  "<p style='font-size: 24px;'>IP address: " + WiFi.localIP().toString() + "</p>" +
                  "<p style='font-size: 24px;'>Connected to: " + String(ssid) + "</p>" +
                  "<p style='font-size: 24px;'><br/>" + commandsList() + "</p>";
    server.send(200, "text/html", body);
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
    showRainbow();
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

void setupCommands(void)
{
    // Register the command handlers with the web server
    for (Command &cmd : commands)
    {
        server.on(cmd.endpoint, HTTP_GET, cmd.handler);
    }

    server.onNotFound(handleNotFound);

    /*handling uploading firmware file */
    server.on(
        "/upload", HTTP_POST, []()
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
