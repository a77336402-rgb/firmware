/*
 * ============================================================
 *  GhostNet — display_ui.h
 *  OLED display driver, menu rendering, all UI screens
 * ============================================================
 */

#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// Color constants compatibility
#ifndef SSD1306_WHITE
#define SSD1306_WHITE SH110X_WHITE
#define SSD1306_BLACK SH110X_BLACK
#define SSD1306_INVERSE SH110X_INVERSE
#endif

// ── Forward declarations for data types from other modules ─
struct NetworkInfo;
struct BLEDeviceInfo;
struct PacketStats;
struct CapturedCred;
struct ProbeInfo;
struct HandshakeInfo;
struct OpenPort;

// ── DisplayUI Class ────────────────────────────────────────
class DisplayUI {
public:
  DisplayUI();

  bool     init();
  void     clear();
  void     render();               // Push buffer to screen

  // ── Boot & Splash ──
  void     showBootScreen();       // Animated boot with logo + progress bar

  // ── Main Menu ──
  void     drawMainMenu(int selectedIndex, int scrollOffset);

  // ── Status Bar & Footer ──
  void     drawStatusBar(const char* title);
  void     drawFooter(const char* left, const char* right);

  // ── WiFi Screens ──
  void     drawWifiScanScreen(NetworkInfo* nets, int count, int selected, int scroll);
  void     drawWifiDetailScreen(const NetworkInfo& net);

  // ── BLE Screens ──
  void     drawBleScanScreen(BLEDeviceInfo* devs, int count, int selected, int scroll);
  void     drawBleDetailScreen(const BLEDeviceInfo& dev);

  // ── Packet Monitor ──
  void     drawPacketMonitor(const PacketStats& stats, uint16_t* history, int histLen, int channel);

  // ── Deauth ──
  void     drawDeauthSelect(NetworkInfo* nets, int count, int selected, int scroll);
  void     drawDeauthRunning(const NetworkInfo& target, uint32_t framesSent, unsigned long elapsed);

  // ── Beacon Spam ──
  void     drawBeaconSelect(int selectedMode);
  void     drawBeaconRunning(BeaconMode mode, uint32_t beaconsSent, unsigned long elapsed);

  // ── Evil Portal ──
  void     drawEvilPortalRunning(const char* ssid, int clients, CapturedCred* creds, int credCount, int scroll);

  // ── Deauth Detector ──
  void     drawDeauthDetector(uint32_t deauthCount, bool alertActive, int channel);

  // ── Device Info ──
  void     drawDeviceInfo();

  // ── Settings ──
  void     drawSettings(int selected, int scroll);

  // ── Probe Sniffer ──
  void     drawProbeSniffer(ProbeInfo* probes, int count, int scroll);

  // ── Handshake Capture ──
  void     drawHandshakeCapture(const HandshakeInfo& hs);

  // ── BLE Spam ──
  void     drawBleSpamRunning(BLESpamMode mode, uint32_t count, unsigned long elapsed);

  // ── AP Clone ──
  void     drawAPCloneRunning(const char* ssid, int clients);

  // ── Port Scanner ──
  void     drawPortScannerRunning(IPAddress target, int scanned, int total, OpenPort* ports, int openCount);

  // ── Utility Drawing ──
  void     drawSignalBars(int x, int y, int bars);       // 0-4
  void     drawProgressBar(int x, int y, int w, int h, int percent);
  void     drawScrollbar(int y, int h, int totalItems, int visibleItems, int scrollOffset);
  void     showAlert(const char* line1, const char* line2, const char* line3);
  void     drawCenteredText(const char* text, int y);

  // ── Bold text (double-strike at x and x+1) ──
  template<typename T>
  void printBold(T value) {
    int16_t cx = _display.getCursorX();
    int16_t cy = _display.getCursorY();
    _display.print(value);
    int16_t nx = _display.getCursorX();
    int16_t ny = _display.getCursorY();
    _display.setCursor(cx + 1, cy);
    _display.print(value);
    _display.setCursor(nx, ny);
  }

  Adafruit_SH1106G* getDisplay() { return &_display; }

private:
  Adafruit_SH1106G _display;

  // ── Menu icon bitmaps ──
  static const uint8_t ICON_WIFI[];
  static const uint8_t ICON_BLE[];
  static const uint8_t ICON_PACKET[];
  static const uint8_t ICON_DEAUTH[];
  static const uint8_t ICON_BEACON[];
  static const uint8_t ICON_PORTAL[];
  static const uint8_t ICON_SHIELD[];
  static const uint8_t ICON_INFO[];
  static const uint8_t ICON_SETTINGS[];
  static const uint8_t LOGO_GHOST[];

  const uint8_t* _menuIcons[MENU_TOTAL_ITEMS];
  const char*    _menuLabels[MENU_TOTAL_ITEMS];
};

#endif // DISPLAY_UI_H
