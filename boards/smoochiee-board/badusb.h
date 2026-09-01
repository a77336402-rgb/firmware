/*
 * ============================================================
 *  GhostNet — badusb.h
 *  BadUSB: DuckyScript parser + USB HID keyboard injection
 *  Uses ESP32-S3 native USB OTG as HID keyboard
 * ============================================================
 */

#ifndef BADUSB_H
#define BADUSB_H

#include <Arduino.h>
#include "config.h"

// ── Max payload limits ─────────────────────────────────────
#define MAX_PAYLOAD_LINES    200
#define MAX_LINE_LENGTH      128
#define MAX_PAYLOADS         10

// ── Payload info ───────────────────────────────────────────
struct PayloadInfo {
  char     name[24];
  char     description[48];
  int      lineCount;
};

// ── BadUSB Class ───────────────────────────────────────────
class BadUSB {
public:
  BadUSB();

  void init();

  // ── Payload management ──
  int          getPayloadCount() const;
  PayloadInfo* getPayload(int index);
  PayloadInfo* getPayloads();
  void         selectPayload(int index);
  int          getSelectedPayload() const;

  // ── Execution ──
  void     startExecution();
  void     stopExecution();
  void     executeNextLine();
  bool     isRunning() const;
  bool     isComplete() const;
  int      getCurrentLine() const;
  int      getTotalLines() const;
  float    getProgress() const;
  const char* getStatusText() const;

  // ── DuckyScript parser ──
  void     parseLine(const char* line);

  // ── HID Keyboard helpers ──
  void     typeString(const char* str);
  void     pressKey(uint8_t key);
  void     pressModifierCombo(uint8_t modifier, uint8_t key);
  void     releaseAll();

private:
  bool         _running;
  bool         _complete;
  bool         _usbStarted;
  int          _selectedPayload;
  int          _currentLine;
  char         _statusText[32];
  unsigned long _delayUntil;
  int          _repeatCount;
  char         _lastCommand[MAX_LINE_LENGTH];

  PayloadInfo  _payloads[MAX_PAYLOADS];
  int          _payloadCount;

  // ── Built-in payloads (stored in PROGMEM) ──
  void     _loadBuiltinPayloads();
  void     _executePayloadLine(int payloadIdx, int lineIdx);

  // ── DuckyScript command handlers ──
  void     _handleSTRING(const char* text);
  void     _handleDELAY(const char* arg);
  void     _handleKEY(const char* keyName);
  void     _handleCOMBO(const char* line);       // e.g. "GUI r", "CTRL ALT DELETE"
  void     _handleREPEAT(const char* arg);
  void     _handleLED(const char* arg);

  // ── Key name → HID keycode mapping ──
  uint8_t  _nameToKeycode(const char* name);
  uint8_t  _nameToModifier(const char* name);
  bool     _isModifier(const char* name);

  // ── Built-in payload scripts ──
  static const char* PAYLOAD_HELLO[];
  static const int   PAYLOAD_HELLO_LEN;
  static const char* PAYLOAD_REVERSE_SHELL[];
  static const int   PAYLOAD_REVERSE_SHELL_LEN;
  static const char* PAYLOAD_WIFI_PASS[];
  static const int   PAYLOAD_WIFI_PASS_LEN;
  static const char* PAYLOAD_RICK_ROLL[];
  static const int   PAYLOAD_RICK_ROLL_LEN;
  static const char* PAYLOAD_DISABLE_DEFENDER[];
  static const int   PAYLOAD_DISABLE_DEFENDER_LEN;
};

#endif // BADUSB_H
