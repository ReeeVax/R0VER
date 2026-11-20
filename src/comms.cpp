#include "comms.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_system.h>
#include <stdlib.h>
#include <string.h>

#include "battery.h"
#include "motors.h"

static const unsigned long COMMAND_TIMEOUT_MS = 1000;
static const char AP_SSID[] = "Rover-ESP32";
static const char CONTROL_PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
  <title>Rover controls</title>
  <style>
    :root { color-scheme: dark; font-family: system-ui, sans-serif; }
    body { margin: 0; min-height: 100vh; display: grid; place-items: center;
           background: #101820; color: #f3f6f8; }
    main { width: min(92vw, 390px); text-align: center; }
    h1 { font-size: 1.5rem; }
    p { color: #b7c5ce; }
    .pad { display: grid; grid-template-columns: repeat(3, 1fr);
           gap: 12px; margin: 24px auto; }
    button { min-height: 76px; border: 0; border-radius: 16px;
             background: #247ba0; color: white; font-size: 1.1rem;
             font-weight: 700; touch-action: none; user-select: none;
             -webkit-user-select: none; }
    button:active { background: #36a3ce; }
    .empty { visibility: hidden; }
    .stop { width: 100%; background: #c44536; }
    #status { min-height: 1.5em; }
  </style>
</head>
<body>
  <main>
    <h1>Rover controls</h1>
    <p>Press and hold a direction. Release to stop.</p>
    <div class="pad">
      <div class="empty"></div>
      <button data-left="150" data-right="150">▲</button>
      <div class="empty"></div>
      <button data-left="-130" data-right="130">◀</button>
      <div class="empty"></div>
      <button data-left="130" data-right="-130">▶</button>
      <div class="empty"></div>
      <button data-left="-150" data-right="-150">▼</button>
      <div class="empty"></div>
    </div>
    <button class="stop" id="stop">STOP</button>
    <p id="status" role="status">Ready</p>
  </main>
  <script>
    let active = null;
    let refreshTimer = null;
    let driveRequestQueued = false;
    let requestQueue = Promise.resolve();
    const status = document.getElementById('status');

    function enqueueRequest(makeRequest) {
      const request = requestQueue.then(makeRequest);
      requestQueue = request.catch(() => {});
      return request;
    }

    function sendDrive() {
      if (!active || driveRequestQueued) return;
      const command = active;
      driveRequestQueued = true;
      const body = new URLSearchParams({left: command.left, right: command.right});
      enqueueRequest(() => active === command ? fetch('/drive', {method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: body.toString(), cache: 'no-store'}) : null)
        .then(response => {
          if (!response) return;
          if (!response.ok) throw new Error('Drive rejected');
          if (active === command) status.textContent = 'Driving — release to stop';
        })
        .catch(() => {
          if (active === command) status.textContent = 'Command failed';
        })
        .then(() => { driveRequestQueued = false; });
    }

    function stop() {
      active = null;
      if (refreshTimer !== null) {
        clearInterval(refreshTimer);
        refreshTimer = null;
      }
      status.textContent = 'Stopping';
      enqueueRequest(() => fetch('/stop', {
        method: 'POST', cache: 'no-store', keepalive: true
      }))
        .then(() => { status.textContent = 'Stopped'; })
        .catch(() => { status.textContent = 'Stop sent; link unavailable'; });
    }

    document.querySelectorAll('button[data-left]').forEach(button => {
      button.addEventListener('pointerdown', event => {
        event.preventDefault();
        if (active) stop();
        active = {left: button.dataset.left, right: button.dataset.right};
        try { button.setPointerCapture(event.pointerId); } catch (_) {}
        sendDrive();
        refreshTimer = setInterval(sendDrive, 250);
      });
    });

    document.addEventListener('pointerup', stop);
    document.addEventListener('pointercancel', stop);
    document.addEventListener('lostpointercapture', stop);
    document.getElementById('stop').addEventListener('click', stop);
    window.addEventListener('blur', stop);
    document.addEventListener('visibilitychange', () => {
      if (document.hidden) stop();
    });
    window.addEventListener('pagehide', stop);
  </script>
</body>
</html>
)HTML";

static WebServer server(80);
static char commandBuffer[48];
static size_t commandLength = 0;
static bool commandOverflow = false;
static bool driveCommandActive = false;
static bool webServerStarted = false;
static unsigned long lastDriveCommandMs = 0;

static void stopDriveLease()
{
    stopMotors();
    driveCommandActive = false;
}

static bool parseSpeed(const char *text, int &speed)
{
    if (text == nullptr || *text == '\0')
    {
        return false;
    }

    char *end = nullptr;
    const long parsed = strtol(text, &end, 10);
    if (end == text || *end != '\0' || parsed < -255 || parsed > 255)
    {
        return false;
    }

    speed = static_cast<int>(parsed);
    return true;
}

static bool parseWebSpeed(const String &text, int &speed)
{
    return parseSpeed(text.c_str(), speed);
}

static bool requestOriginAllowed()
{
    const String expectedOrigin = String("http://") + WiFi.softAPIP().toString();
    return server.hasHeader("Origin") && server.header("Origin") == expectedOrigin;
}

static void handleRoot()
{
    server.send_P(200, "text/html; charset=utf-8", CONTROL_PAGE);
}

static void handleWebStop()
{
    // A stop is always safe to honor, even if a browser omits or fails Origin.
    stopDriveLease();
    if (!requestOriginAllowed())
    {
        server.send(403, "text/plain", "Stopped; browser origin rejected");
        return;
    }
    server.send(200, "text/plain", "Stopped");
}

static void handleWebDrive()
{
    if (!requestOriginAllowed())
    {
        stopDriveLease();
        server.send(403, "text/plain", "Browser origin rejected");
        return;
    }

    int leftSpeed = 0;
    int rightSpeed = 0;
    if (server.args() != 2 ||
        !server.hasArg("left") ||
        !server.hasArg("right") ||
        !parseWebSpeed(server.arg("left"), leftSpeed) ||
        !parseWebSpeed(server.arg("right"), rightSpeed))
    {
        stopDriveLease();
        server.send(400, "text/plain", "Use left and right speeds from -255 to 255");
        return;
    }

    if (!batteryIsReady())
    {
        stopDriveLease();
        server.send(503, "text/plain", "Drive disabled: INA260 unavailable");
        return;
    }

    setDrive(leftSpeed, rightSpeed);
    driveCommandActive = leftSpeed != 0 || rightSpeed != 0;
    lastDriveCommandMs = millis();
    server.send(200, "text/plain", "OK");
}

static void handleNotFound()
{
    if (server.uri() == "/drive")
    {
        stopDriveLease();
        server.send(405, "text/plain", "Use POST /drive");
        return;
    }
    if (server.uri() == "/stop")
    {
        handleWebStop();
        return;
    }
    server.send(404, "text/plain", "Not found");
}

static void startAccessPoint()
{
    char password[25];
    WiFi.mode(WIFI_AP);

    // Generate 96 bits of hexadecimal WPA2 passphrase entropy for this boot.
    snprintf(password, sizeof(password), "%08lx%08lx%08lx",
             static_cast<unsigned long>(esp_random()),
             static_cast<unsigned long>(esp_random()),
             static_cast<unsigned long>(esp_random()));

    if (!WiFi.softAP(AP_SSID, password))
    {
        Serial.println("Wi-Fi AP startup failed; USB serial control remains available.");
        return;
    }

    const char *headers[] = {"Origin"};
    server.collectHeaders(headers, 1);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/drive", HTTP_POST, handleWebDrive);
    server.on("/stop", HTTP_POST, handleWebStop);
    server.onNotFound(handleNotFound);
    server.begin();
    webServerStarted = true;

    Serial.println("Wi-Fi AP control is enabled.");
    Serial.print("AP SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP password (new each boot): ");
    Serial.println(password);
    Serial.print("Control page: http://");
    Serial.println(WiFi.softAPIP());
}

static void processSerialCommand(bool motorsPermitted)
{
    commandBuffer[commandLength] = '\0';
    char *command = strtok(commandBuffer, " \t\r");
    if (command == nullptr)
    {
        return;
    }

    if (strcmp(command, "S") == 0)
    {
        char *extra = strtok(nullptr, " \t\r");
        stopDriveLease();
        if (extra != nullptr)
        {
            Serial.println("ERR usage: S");
        }
        else
        {
            Serial.println("OK stopped");
        }
        return;
    }

    char *leftText = strtok(nullptr, " \t\r");
    char *rightText = strtok(nullptr, " \t\r");
    char *extra = strtok(nullptr, " \t\r");
    int leftSpeed = 0;
    int rightSpeed = 0;

    if (strcmp(command, "D") != 0 ||
        !parseSpeed(leftText, leftSpeed) ||
        !parseSpeed(rightText, rightSpeed) ||
        extra != nullptr)
    {
        stopDriveLease();
        Serial.println("ERR use D <left -255..255> <right -255..255> or S");
        return;
    }

    if (!motorsPermitted)
    {
        stopDriveLease();
        Serial.println("ERR drive disabled: INA260 unavailable");
        return;
    }

    setDrive(leftSpeed, rightSpeed);
    driveCommandActive = leftSpeed != 0 || rightSpeed != 0;
    lastDriveCommandMs = millis();
    Serial.println("OK drive");
}

void commsSetup()
{
    commandLength = 0;
    commandOverflow = false;
    driveCommandActive = false;
    webServerStarted = false;
    startAccessPoint();
}

void commsUpdate(bool motorsPermitted)
{
    if (webServerStarted)
    {
        server.handleClient();
    }

    if (!motorsPermitted)
    {
        stopDriveLease();
    }

    while (Serial.available() > 0)
    {
        const char incoming = static_cast<char>(Serial.read());
        if (incoming == '\n')
        {
            if (commandOverflow)
            {
                Serial.println("ERR line too long");
                stopDriveLease();
            }
            else
            {
                processSerialCommand(motorsPermitted);
            }

            commandLength = 0;
            commandOverflow = false;
        }
        else if (!commandOverflow && incoming != '\r')
        {
            if (commandLength + 1 < sizeof(commandBuffer))
            {
                commandBuffer[commandLength++] = incoming;
            }
            else
            {
                commandOverflow = true;
            }
        }
    }

    if (driveCommandActive &&
        static_cast<unsigned long>(millis() - lastDriveCommandMs) >= COMMAND_TIMEOUT_MS)
    {
        stopDriveLease();
        Serial.println("OK command timeout; stopped");
    }
}
