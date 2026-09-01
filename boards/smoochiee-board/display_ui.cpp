/*
 * ============================================================
 *  GhostNet — display_ui.cpp
 *  OLED display driver, menu rendering, all screen layouts
 * ============================================================
 */

#include "display_ui.h"
#include "wifi_module.h"
#include "ble_module.h"
#include "packet_monitor.h"
#include "evil_portal.h"
#include "utils.h"
#include <Wire.h>
#include <esp_system.h>
#include <WiFi.h>

// ── 8x8 pixel icon bitmaps (stored in PROGMEM) ────────────
// Each icon is 8x8 = 8 bytes

const uint8_t DisplayUI::ICON_WIFI[] PROGMEM = {
  0b00000000,
  0b01111110,
  0b10000001,
  0b00111100,
  0b01000010,
  0b00011000,
  0b00100100,
  0b00011000
};

const uint8_t DisplayUI::ICON_BLE[] PROGMEM = {
  0b00010000,
  0b00010100,
  0b00011010,
  0b01011100,
  0b01011100,
  0b00011010,
  0b00010100,
  0b00010000
};

const uint8_t DisplayUI::ICON_PACKET[] PROGMEM = {
  0b11111111,
  0b10000001,
  0b10111101,
  0b10100101,
  0b10100101,
  0b10111101,
  0b10000001,
  0b11111111
};

const uint8_t DisplayUI::ICON_DEAUTH[] PROGMEM = {
  0b00011000,
  0b00100100,
  0b01011010,
  0b10011001,
  0b10011001,
  0b01011010,
  0b00100100,
  0b00011000
};

const uint8_t DisplayUI::ICON_BEACON[] PROGMEM = {
  0b00010000,
  0b00111000,
  0b01111100,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00111000
};

const uint8_t DisplayUI::ICON_PORTAL[] PROGMEM = {
  0b00111100,
  0b01000010,
  0b10011001,
  0b10100101,
  0b10100101,
  0b10011001,
  0b01000010,
  0b00111100
};

const uint8_t DisplayUI::ICON_SHIELD[] PROGMEM = {
  0b00111100,
  0b01111110,
  0b11111111,
  0b11111111,
  0b11111111,
  0b01111110,
  0b00111100,
  0b00011000
};

const uint8_t DisplayUI::ICON_INFO[] PROGMEM = {
  0b00111100,
  0b01000010,
  0b01011010,
  0b01000010,
  0b01011010,
  0b01011010,
  0b01000010,
  0b00111100
};

const uint8_t DisplayUI::ICON_SETTINGS[] PROGMEM = {
  0b00100100,
  0b01111110,
  0b11011011,
  0b11111111,
  0b11111111,
  0b11011011,
  0b01111110,
  0b00100100
};

// 16x16 ghost logo for boot screen
const uint8_t DisplayUI::LOGO_GHOST[] PROGMEM = {
  0b00000111, 0b11100000,
  0b00011111, 0b11111000,
  0b00111111, 0b11111100,
  0b01111111, 0b11111110,
  0b01110011, 0b10011110,
  0b01110011, 0b10011110,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11111111, 0b11111111,
  0b11101110, 0b11101111,
  0b11000100, 0b01000111,
  0b10000000, 0b00000011,
  0b00000000, 0b00000000,
};


#include "network_attacks.h"

// ── Constructor ────────────────────────────────────────────
DisplayUI::DisplayUI()
  : _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
{
  _menuIcons[0]  = ICON_WIFI;
  _menuIcons[1]  = ICON_BLE;
  _menuIcons[2]  = ICON_PACKET;
  _menuIcons[3]  = ICON_DEAUTH;
  _menuIcons[4]  = ICON_BEACON;
  _menuIcons[5]  = ICON_PORTAL;
  _menuIcons[6]  = ICON_PACKET;
  _menuIcons[7]  = ICON_SHIELD;
  _menuIcons[8]  = ICON_BLE;
  _menuIcons[9]  = ICON_WIFI;
  _menuIcons[10] = ICON_SHIELD;
  _menuIcons[11] = ICON_SETTINGS;
  _menuIcons[12] = ICON_INFO;
  _menuIcons[13] = ICON_SETTINGS;

  _menuLabels[0]  = "WiFi Scanner";
  _menuLabels[1]  = "BLE Scanner";
  _menuLabels[2]  = "Packet Monitor";
  _menuLabels[3]  = "Deauth Attack";
  _menuLabels[4]  = "Beacon Spam";
  _menuLabels[5]  = "Evil Portal";
  _menuLabels[6]  = "Probe Sniffer";
  _menuLabels[7]  = "WPA Handshake";
  _menuLabels[8]  = "BLE Spam";
  _menuLabels[9]  = "Rogue AP Clone";
  _menuLabels[10] = "Deauth Detect";
  _menuLabels[11] = "Port Scanner";
  _menuLabels[12] = "Device Info";
  _menuLabels[13] = "Settings";
}

// ── Init ───────────────────────────────────────────────────
bool DisplayUI::init() {
  Serial.printf("[DISPLAY] Starting I2C on SDA: GPIO %d, SCL: GPIO %d\n", I2C_SDA, I2C_SCL);
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(I2C_FREQUENCY);
  delay(100);

  // Scan I2C for OLED Address
  uint8_t foundAddr = 0;
  Wire.beginTransmission(OLED_ADDR_PRIMARY);
  if (Wire.endTransmission() == 0) {
    foundAddr = OLED_ADDR_PRIMARY;
    Serial.printf("[DISPLAY] Found OLED at 0x%02X\n", foundAddr);
  } else {
    Wire.beginTransmission(OLED_ADDR_ALT);
    if (Wire.endTransmission() == 0) {
      foundAddr = OLED_ADDR_ALT;
      Serial.printf("[DISPLAY] Found OLED at 0x%02X\n", foundAddr);
    }
  }

  if (foundAddr == 0) {
    foundAddr = OLED_ADDR_PRIMARY; // Fallback
    Serial.println(F("[DISPLAY] Warning: No ACK received, defaulting to 0x3C"));
  }

  // Initialize SH1106 display controller
  if (!_display.begin(foundAddr, true)) {
    Serial.println(F("[DISPLAY] SH1106 allocation/init FAILED! Retrying at 100kHz..."));
    Wire.setClock(100000);
    delay(50);
    if (!_display.begin(foundAddr, true)) {
      Serial.println(F("[DISPLAY] SH1106 init failed completely."));
      return false;
    }
  }

  // Clear display buffer and push to hardware RAM immediately (wipes noise/dots)
  _display.clearDisplay();
  _display.display();
  delay(50);

  _display.setTextColor(SH110X_WHITE);
  _display.setTextSize(1);
  _display.setTextWrap(false);
  
  Serial.println(F("[DISPLAY] SH1106 OLED Initialized Successfully."));
  return true;
}

void DisplayUI::clear() { _display.clearDisplay(); }
void DisplayUI::render() { _display.display(); }

// ── Centered text helper ───────────────────────────────────
void DisplayUI::drawCenteredText(const char* text, int y) {
  int16_t x1, y1;
  uint16_t w, h;
  _display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  _display.setCursor((SCREEN_WIDTH - w) / 2, y);
  printBold(text);
}

// ── Boot Screen ────────────────────────────────────────────
void DisplayUI::showBootScreen() {
  for (int progress = 0; progress <= 100; progress += 4) {
    _display.clearDisplay();

    // Ghost logo centered at top
    _display.drawBitmap(56, 4, LOGO_GHOST, 16, 16, SSD1306_WHITE);

    // Title
    _display.setTextSize(1);
    drawCenteredText(GHOSTNET_NAME, 24);

    // Version
    char verBuf[20];
    snprintf(verBuf, sizeof(verBuf), "v%s", GHOSTNET_VERSION);
    drawCenteredText(verBuf, 34);

    // Progress bar
    drawProgressBar(14, 48, 100, 8, progress);

    // Loading text
    const char* loadTexts[] = {"Initializing...", "Loading WiFi...", "Loading BLE...", "Starting UI...", "Ready!"};
    int idx = progress / 25;
    if (idx > 4) idx = 4;
    drawCenteredText(loadTexts[idx], 58);

    _display.display();
    delay(30);
  }
  delay(400);
}

// ── Status Bar ─────────────────────────────────────────────
void DisplayUI::drawStatusBar(const char* title) {
  // Background bar
  _display.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_H, SSD1306_WHITE);
  _display.setTextColor(SSD1306_BLACK);
  _display.setTextSize(1);
  _display.setCursor(2, 1);
  printBold(title);

  // Free heap on right side (compact)
  uint32_t freeHeap = esp_get_free_heap_size() / 1024;
  char heapBuf[10];
  snprintf(heapBuf, sizeof(heapBuf), "%luK", (unsigned long)freeHeap);
  int16_t x1, y1;
  uint16_t w, h;
  _display.getTextBounds(heapBuf, 0, 0, &x1, &y1, &w, &h);
  _display.setCursor(SCREEN_WIDTH - w - 2, 1);
  printBold(heapBuf);

  _display.setTextColor(SSD1306_WHITE);  // Reset
}

// ── Footer ─────────────────────────────────────────────────
void DisplayUI::drawFooter(const char* left, const char* right) {
  int y = SCREEN_HEIGHT - FOOTER_H;
  _display.drawLine(0, y, SCREEN_WIDTH - 1, y, SSD1306_WHITE);
  _display.setTextSize(1);
  _display.setCursor(2, y + 2);
  printBold(left);
  if (right) {
    int16_t x1, y1;
    uint16_t w, h;
    _display.getTextBounds(right, 0, 0, &x1, &y1, &w, &h);
    _display.setCursor(SCREEN_WIDTH - w - 2, y + 2);
    printBold(right);
  }
}

// ── Main Menu ──────────────────────────────────────────────
void DisplayUI::drawMainMenu(int selectedIndex, int scrollOffset) {
  _display.clearDisplay();
  drawStatusBar("GhostNet");

  int startY = CONTENT_Y + 1;
  for (int i = 0; i < VISIBLE_MENU_ITEMS; i++) {
    int idx = scrollOffset + i;
    if (idx >= MENU_TOTAL_ITEMS) break;

    int itemY = startY + i * MENU_ITEM_H;

    // Highlight selected item
    if (idx == selectedIndex) {
      _display.fillRoundRect(0, itemY, SCREEN_WIDTH - 4, MENU_ITEM_H - 1, 2, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    // Icon
    _display.drawBitmap(3, itemY + 1, _menuIcons[idx], 8, 8, 
                        idx == selectedIndex ? SSD1306_BLACK : SSD1306_WHITE);

    // Label
    _display.setCursor(14, itemY + 1);
    printBold(_menuLabels[idx]);

    _display.setTextColor(SSD1306_WHITE);  // Reset
  }

  // Scrollbar
  drawScrollbar(CONTENT_Y, CONTENT_H, MENU_TOTAL_ITEMS, VISIBLE_MENU_ITEMS, scrollOffset);

  drawFooter("\x18\x19:Nav", "\x07:Sel");
  _display.display();
}

// ── WiFi Scan Screen ───────────────────────────────────────
void DisplayUI::drawWifiScanScreen(NetworkInfo* nets, int count, int selected, int scroll) {
  _display.clearDisplay();

  char titleBuf[22];
  snprintf(titleBuf, sizeof(titleBuf), "WiFi [%d]", count);
  drawStatusBar(titleBuf);

  if (count == 0) {
    drawCenteredText("Scanning...", 30);
    drawFooter("", "Hold:Back");
    _display.display();
    return;
  }

  int startY = CONTENT_Y + 1;
  int visibleItems = VISIBLE_MENU_ITEMS;

  for (int i = 0; i < visibleItems; i++) {
    int idx = scroll + i;
    if (idx >= count) break;

    int itemY = startY + i * MENU_ITEM_H;

    if (idx == selected) {
      _display.fillRoundRect(0, itemY, SCREEN_WIDTH - 4, MENU_ITEM_H - 1, 2, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    // SSID (truncated to 12 chars)
    _display.setCursor(2, itemY + 1);
    char ssidBuf[13];
    strncpy(ssidBuf, nets[idx].ssid, 12);
    ssidBuf[12] = '\0';
    printBold(ssidBuf);

    // RSSI
    char rssiBuf[6];
    snprintf(rssiBuf, sizeof(rssiBuf), "%d", nets[idx].rssi);
    _display.setCursor(80, itemY + 1);
    printBold(rssiBuf);

    // Signal bars
    int bars = getSignalBars(nets[idx].rssi);
    drawSignalBars(105, itemY + 1, bars);

    // Lock icon for encrypted
    if (nets[idx].encType != 0) {  // WIFI_AUTH_OPEN = 0
      _display.setCursor(120, itemY + 1);
      printBold("*");
    }

    _display.setTextColor(SSD1306_WHITE);
  }

  drawScrollbar(CONTENT_Y, CONTENT_H, count, visibleItems, scroll);
  drawFooter("\x18\x19:Nav", "\x07:Det");
  _display.display();
}

// ── WiFi Detail Screen ─────────────────────────────────────
void DisplayUI::drawWifiDetailScreen(const NetworkInfo& net) {
  _display.clearDisplay();
  drawStatusBar("AP Detail");

  int y = CONTENT_Y + 2;

  _display.setCursor(0, y);
  printBold("SSID:");
  _display.setCursor(32, y);
  char ssidTrunc[18];
  strncpy(ssidTrunc, net.ssid, 17);
  ssidTrunc[17] = '\0';
  printBold(ssidTrunc);
  y += 9;

  _display.setCursor(0, y);
  printBold("BSID:");
  _display.setCursor(32, y);
  printBold(macToString(net.bssid));
  y += 9;

  _display.setCursor(0, y);
  printBold("RSSI:");
  _display.setCursor(32, y);
  char rssiBuf[16];
  snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm (%d%%)", net.rssi, rssiToPercent(net.rssi));
  printBold(rssiBuf);
  y += 9;

  _display.setCursor(0, y);
  printBold("CH: ");
  printBold(net.channel);
  printBold("  Enc: ");
  printBold(encTypeToString(net.encType));

  drawFooter("Hold:Back", "");
  _display.display();
}

// ── BLE Scan Screen ────────────────────────────────────────
void DisplayUI::drawBleScanScreen(BLEDeviceInfo* devs, int count, int selected, int scroll) {
  _display.clearDisplay();

  char titleBuf[22];
  snprintf(titleBuf, sizeof(titleBuf), "BLE [%d]", count);
  drawStatusBar(titleBuf);

  if (count == 0) {
    drawCenteredText("Scanning...", 30);
    drawFooter("", "Hold:Back");
    _display.display();
    return;
  }

  int startY = CONTENT_Y + 1;
  int visibleItems = VISIBLE_MENU_ITEMS;

  for (int i = 0; i < visibleItems; i++) {
    int idx = scroll + i;
    if (idx >= count) break;

    int itemY = startY + i * MENU_ITEM_H;

    if (idx == selected) {
      _display.fillRoundRect(0, itemY, SCREEN_WIDTH - 4, MENU_ITEM_H - 1, 2, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    // Alert icon for suspicious devices
    if (devs[idx].isAlert) {
      _display.setCursor(1, itemY + 1);
      printBold("!");
    }

    // Name (truncated)
    _display.setCursor(devs[idx].isAlert ? 9 : 2, itemY + 1);
    char nameBuf[13];
    const char* displayName = strlen(devs[idx].name) > 0 ? devs[idx].name : devs[idx].deviceType;
    strncpy(nameBuf, displayName, 12);
    nameBuf[12] = '\0';
    printBold(nameBuf);

    // RSSI
    char rssiBuf[6];
    snprintf(rssiBuf, sizeof(rssiBuf), "%d", devs[idx].rssi);
    _display.setCursor(90, itemY + 1);
    printBold(rssiBuf);

    // Signal bars
    drawSignalBars(115, itemY + 1, getSignalBars(devs[idx].rssi));

    _display.setTextColor(SSD1306_WHITE);
  }

  drawScrollbar(CONTENT_Y, CONTENT_H, count, visibleItems, scroll);
  drawFooter("\x18\x19:Nav", "\x07:Det");
  _display.display();
}

// ── BLE Detail Screen ──────────────────────────────────────
void DisplayUI::drawBleDetailScreen(const BLEDeviceInfo& dev) {
  _display.clearDisplay();
  drawStatusBar("BLE Detail");

  int y = CONTENT_Y + 2;

  _display.setCursor(0, y);
  printBold("Name:");
  _display.setCursor(32, y);
  printBold(strlen(dev.name) > 0 ? dev.name : "(unknown)");
  y += 9;

  _display.setCursor(0, y);
  printBold("MAC:");
  _display.setCursor(28, y);
  printBold(dev.address);
  y += 9;

  _display.setCursor(0, y);
  printBold("RSSI: ");
  printBold(dev.rssi);
  printBold(" dBm");
  y += 9;

  _display.setCursor(0, y);
  printBold("Type: ");
  printBold(dev.deviceType);
  if (dev.isAlert) {
    printBold(" [!]");
  }

  drawFooter("Hold:Back", "");
  _display.display();
}

// ── Packet Monitor Screen ──────────────────────────────────
void DisplayUI::drawPacketMonitor(const PacketStats& stats, uint16_t* history, int histLen, int channel) {
  _display.clearDisplay();

  char titleBuf[22];
  snprintf(titleBuf, sizeof(titleBuf), "PktMon CH:%d", channel);
  drawStatusBar(titleBuf);

  // ── Bar graph of PPS history ──
  int graphX = 2;
  int graphY = CONTENT_Y + 1;
  int graphW = SCREEN_WIDTH - 6;
  int graphH = PKT_GRAPH_H;

  // Find max for scaling
  uint16_t maxVal = 1;
  for (int i = 0; i < histLen; i++) {
    if (history[i] > maxVal) maxVal = history[i];
  }

  // Draw bars
  int barW = max(1, graphW / histLen);
  for (int i = 0; i < histLen && i * barW < graphW; i++) {
    int barH = map(history[i], 0, maxVal, 0, graphH);
    if (barH > 0) {
      _display.fillRect(graphX + i * barW, graphY + graphH - barH, barW - 1, barH, SSD1306_WHITE);
    }
  }

  // Border
  _display.drawRect(graphX - 1, graphY - 1, graphW + 2, graphH + 2, SSD1306_WHITE);

  // ── Stats below graph ──
  int statY = graphY + graphH + 3;
  _display.setTextSize(1);

  _display.setCursor(0, statY);
  char statBuf[22];
  snprintf(statBuf, sizeof(statBuf), "PPS:%lu T:%lu", (unsigned long)stats.pps, (unsigned long)stats.total);
  printBold(statBuf);

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── Deauth Select Screen ──────────────────────────────────
void DisplayUI::drawDeauthSelect(NetworkInfo* nets, int count, int selected, int scroll) {
  _display.clearDisplay();

  char titleBuf[22];
  snprintf(titleBuf, sizeof(titleBuf), "Deauth [%d]", count);
  drawStatusBar(titleBuf);

  if (count == 0) {
    drawCenteredText("Scan WiFi first!", 28);
    drawCenteredText("Go to WiFi Scanner", 38);
    drawFooter("Hold:Back", "");
    _display.display();
    return;
  }

  int startY = CONTENT_Y + 1;
  for (int i = 0; i < VISIBLE_MENU_ITEMS; i++) {
    int idx = scroll + i;
    if (idx >= count) break;

    int itemY = startY + i * MENU_ITEM_H;

    if (idx == selected) {
      _display.fillRoundRect(0, itemY, SCREEN_WIDTH - 4, MENU_ITEM_H - 1, 2, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    _display.setCursor(2, itemY + 1);
    char ssidBuf[16];
    strncpy(ssidBuf, nets[idx].ssid, 15);
    ssidBuf[15] = '\0';
    printBold(ssidBuf);

    char chBuf[6];
    snprintf(chBuf, sizeof(chBuf), "CH%d", nets[idx].channel);
    _display.setCursor(100, itemY + 1);
    printBold(chBuf);

    _display.setTextColor(SSD1306_WHITE);
  }

  drawScrollbar(CONTENT_Y, CONTENT_H, count, VISIBLE_MENU_ITEMS, scroll);
  drawFooter("\x18\x19:Nav", "\x07:Atk");
  _display.display();
}

// ── Deauth Running Screen ─────────────────────────────────
void DisplayUI::drawDeauthRunning(const NetworkInfo& target, uint32_t framesSent, unsigned long elapsed) {
  _display.clearDisplay();
  drawStatusBar("! DEAUTH !");

  int y = CONTENT_Y + 2;

  _display.setCursor(0, y);
  printBold("Target: ");
  char ssidBuf[14];
  strncpy(ssidBuf, target.ssid, 13);
  ssidBuf[13] = '\0';
  printBold(ssidBuf);
  y += 10;

  _display.setCursor(0, y);
  printBold("BSSID: ");
  printBold(macToString(target.bssid));
  y += 10;

  _display.setCursor(0, y);
  printBold("Frames: ");
  printBold(framesSent);
  y += 10;

  _display.setCursor(0, y);
  printBold("Time: ");
  printBold(formatUptime(elapsed));

  // Animated indicator
  int dotCount = (millis() / 300) % 4;
  _display.setCursor(110, CONTENT_Y + 2);
  for (int i = 0; i < dotCount; i++) printBold(".");

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── Beacon Spam Select ────────────────────────────────────
void DisplayUI::drawBeaconSelect(int selectedMode) {
  _display.clearDisplay();
  drawStatusBar("Beacon Spam");

  const char* modes[] = {"Random SSIDs", "Funny SSIDs", "Rickroll", "Custom"};
  int startY = CONTENT_Y + 2;

  for (int i = 0; i < BEACON_MODE_COUNT; i++) {
    int itemY = startY + i * MENU_ITEM_H;
    if (i == selectedMode) {
      _display.fillRoundRect(0, itemY, SCREEN_WIDTH - 4, MENU_ITEM_H - 1, 2, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }
    _display.setCursor(4, itemY + 1);
    printBold(modes[i]);
    _display.setTextColor(SSD1306_WHITE);
  }

  drawFooter("\x18\x19:Nav", "\x07:Start");
  _display.display();
}

// ── Beacon Running Screen ─────────────────────────────────
void DisplayUI::drawBeaconRunning(BeaconMode mode, uint32_t beaconsSent, unsigned long elapsed) {
  _display.clearDisplay();
  drawStatusBar("! BEACON !");

  const char* modeNames[] = {"Random", "Funny", "Rickroll", "Custom"};

  int y = CONTENT_Y + 4;

  _display.setCursor(0, y);
  printBold("Mode: ");
  printBold(modeNames[mode]);
  y += 12;

  _display.setCursor(0, y);
  printBold("Beacons: ");
  printBold(beaconsSent);
  y += 12;

  _display.setCursor(0, y);
  printBold("Time: ");
  printBold(formatUptime(elapsed));

  // Animated broadcast icon
  int frame = (millis() / 200) % 3;
  int cx = 110, cy = CONTENT_Y + 8;
  _display.fillCircle(cx, cy, 2, SSD1306_WHITE);
  if (frame >= 1) _display.drawCircle(cx, cy, 5, SSD1306_WHITE);
  if (frame >= 2) _display.drawCircle(cx, cy, 8, SSD1306_WHITE);

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── Evil Portal Running ───────────────────────────────────
void DisplayUI::drawEvilPortalRunning(const char* ssid, int clients, CapturedCred* creds, int credCount, int scroll) {
  _display.clearDisplay();
  drawStatusBar("Evil Portal");

  int y = CONTENT_Y + 2;

  _display.setCursor(0, y);
  printBold("AP: ");
  printBold(ssid);
  y += 10;

  _display.setCursor(0, y);
  printBold("Clients: ");
  printBold(clients);
  printBold("  Creds: ");
  printBold(credCount);
  y += 10;

  // Show latest credentials
  if (credCount > 0) {
    _display.drawLine(0, y, SCREEN_WIDTH - 1, y, SSD1306_WHITE);
    y += 2;
    int startIdx = max(0, credCount - 2);  // Show last 2
    for (int i = startIdx; i < credCount && y < SCREEN_HEIGHT - FOOTER_H; i++) {
      _display.setCursor(0, y);
      char credBuf[22];
      snprintf(credBuf, sizeof(credBuf), "%s:%s", creds[i].username, creds[i].password);
      credBuf[21] = '\0';
      printBold(credBuf);
      y += 9;
    }
  }

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── Deauth Detector ───────────────────────────────────────
void DisplayUI::drawDeauthDetector(uint32_t deauthCount, bool alertActive, int channel) {
  _display.clearDisplay();

  if (alertActive) {
    // Flashing alert
    bool flash = (millis() / 300) % 2;
    if (flash) {
      _display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    }
    drawStatusBar("!! ALERT !!");
  } else {
    drawStatusBar("Deauth Det.");
  }

  int y = CONTENT_Y + 4;

  _display.setCursor(0, y);
  printBold("Monitoring CH: ");
  printBold(channel);
  y += 12;

  _display.setCursor(0, y);
  printBold("Deauth Pkts: ");
  printBold(deauthCount);
  y += 12;

  _display.setCursor(0, y);
  if (alertActive) {
    printBold("!!! ATTACK DETECTED !!!");
  } else if (deauthCount > 0) {
    printBold("Status: Deauths seen");
  } else {
    printBold("Status: Clean");
  }

  _display.setTextColor(SSD1306_WHITE);  // Reset in case of alert flash

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── Device Info Screen ────────────────────────────────────
void DisplayUI::drawDeviceInfo() {
  _display.clearDisplay();
  drawStatusBar("Device Info");

  int y = CONTENT_Y + 2;

  // Chip model
  _display.setCursor(0, y);
  printBold("Chip: ESP32-S3");
  y += 9;

  // MAC
  _display.setCursor(0, y);
  printBold("MAC: ");
  printBold(WiFi.macAddress());
  y += 9;

  // Free heap
  _display.setCursor(0, y);
  printBold("Heap: ");
  printBold(formatBytes(esp_get_free_heap_size()));
  y += 9;

  // Uptime
  _display.setCursor(0, y);
  printBold("Up: ");
  printBold(formatUptime(millis()));
  y += 9;

  // CPU freq
  _display.setCursor(0, y);
  printBold("CPU: ");
  printBold(getCpuFrequencyMhz());
  printBold(" MHz");

  drawFooter("Hold:Back", "");
  _display.display();
}

// ── Settings Screen ───────────────────────────────────────
void DisplayUI::drawSettings(int selected, int scroll) {
  _display.clearDisplay();
  drawStatusBar("Settings");

  const char* settingLabels[] = {"Brightness: Max", "WiFi CH: Auto", "BLE Scan: 5s", "About..."};
  int settingCount = 4;

  int startY = CONTENT_Y + 2;
  for (int i = 0; i < VISIBLE_MENU_ITEMS && i < settingCount; i++) {
    int idx = scroll + i;
    if (idx >= settingCount) break;

    int itemY = startY + i * MENU_ITEM_H;

    if (idx == selected) {
      _display.fillRoundRect(0, itemY, SCREEN_WIDTH - 4, MENU_ITEM_H - 1, 2, SSD1306_WHITE);
      _display.setTextColor(SSD1306_BLACK);
    } else {
      _display.setTextColor(SSD1306_WHITE);
    }

    _display.setCursor(4, itemY + 1);
    printBold(settingLabels[idx]);
    _display.setTextColor(SSD1306_WHITE);
  }

  drawFooter("\x18\x19:Nav", "Hold:Back");
  _display.display();
}

// ── Signal Bars (4 bars max) ──────────────────────────────
void DisplayUI::drawSignalBars(int x, int y, int bars) {
  for (int i = 0; i < 4; i++) {
    int barH = 2 + i * 2;  // Heights: 2, 4, 6, 8
    int barX = x + i * 3;
    int barY = y + (8 - barH);
    if (i < bars) {
      _display.fillRect(barX, barY, 2, barH, SSD1306_WHITE);
    } else {
      _display.drawRect(barX, barY, 2, barH, SSD1306_WHITE);
    }
  }
}

// ── Progress Bar ──────────────────────────────────────────
void DisplayUI::drawProgressBar(int x, int y, int w, int h, int percent) {
  _display.drawRoundRect(x, y, w, h, 2, SSD1306_WHITE);
  int fillW = map(percent, 0, 100, 0, w - 4);
  if (fillW > 0) {
    _display.fillRoundRect(x + 2, y + 2, fillW, h - 4, 1, SSD1306_WHITE);
  }
}

// ── Scrollbar ─────────────────────────────────────────────
void DisplayUI::drawScrollbar(int y, int h, int totalItems, int visibleItems, int scrollOffset) {
  if (totalItems <= visibleItems) return;  // No scrollbar needed

  int sbX = SCREEN_WIDTH - 2;
  int sbH = max(4, h * visibleItems / totalItems);
  int sbY = y + (h - sbH) * scrollOffset / (totalItems - visibleItems);

  _display.drawLine(sbX, y, sbX, y + h - 1, SSD1306_WHITE);
  _display.fillRect(sbX - 1, sbY, 3, sbH, SSD1306_WHITE);
}

// ── Alert Box ─────────────────────────────────────────────
void DisplayUI::showAlert(const char* line1, const char* line2, const char* line3) {
  _display.clearDisplay();

  // Bordered box
  _display.drawRoundRect(4, 4, 120, 56, 4, SSD1306_WHITE);

  if (line1) drawCenteredText(line1, 14);
  if (line2) drawCenteredText(line2, 28);
  if (line3) drawCenteredText(line3, 42);

  _display.display();
}

// ── Probe Sniffer Screen ──────────────────────────────────
void DisplayUI::drawProbeSniffer(ProbeInfo* probes, int count, int scroll) {
  _display.clearDisplay();
  char titleBuf[22];
  snprintf(titleBuf, sizeof(titleBuf), "Probes [%d]", count);
  drawStatusBar(titleBuf);

  if (count == 0) {
    drawCenteredText("Listening...", 30);
    drawFooter("", "Hold:Back");
    _display.display();
    return;
  }

  int startY = CONTENT_Y + 1;
  for (int i = 0; i < VISIBLE_MENU_ITEMS; i++) {
    int idx = scroll + i;
    if (idx >= count) break;
    int itemY = startY + i * MENU_ITEM_H;

    _display.setCursor(2, itemY + 1);
    char ssidBuf[14];
    strncpy(ssidBuf, probes[idx].ssid, 13);
    ssidBuf[13] = '\0';
    printBold(ssidBuf);

    char rssiBuf[6];
    snprintf(rssiBuf, sizeof(rssiBuf), "%d", (int)probes[idx].rssi);
    _display.setCursor(95, itemY + 1);
    printBold(rssiBuf);
  }

  drawScrollbar(CONTENT_Y, CONTENT_H, count, VISIBLE_MENU_ITEMS, scroll);
  drawFooter("Hold:Back", "");
  _display.display();
}

// ── Handshake Capture Screen ──────────────────────────────
void DisplayUI::drawHandshakeCapture(const HandshakeInfo& hs) {
  _display.clearDisplay();
  drawStatusBar("EAPOL Handshake");

  int y = CONTENT_Y + 4;
  _display.setCursor(0, y);
  printBold("Target: ");
  printBold(strlen(hs.ssid) > 0 ? hs.ssid : "Any AP");
  y += 11;

  _display.setCursor(0, y);
  printBold("M1:["); printBold(hs.gotM1 ? "X" : " "); printBold("] ");
  printBold("M2:["); printBold(hs.gotM2 ? "X" : " "); printBold("] ");
  printBold("M3:["); printBold(hs.gotM3 ? "X" : " "); printBold("]");
  y += 11;

  _display.setCursor(0, y);
  if (hs.complete) {
    printBold(">> CAPTURED! <<");
  } else {
    printBold("Waiting for auth...");
  }

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── BLE Spam Screen ───────────────────────────────────────
void DisplayUI::drawBleSpamRunning(BLESpamMode mode, uint32_t count, unsigned long elapsed) {
  _display.clearDisplay();
  drawStatusBar("! BLE SPAM !");

  const char* modeNames[] = {"Apple", "Android", "Windows", "Samsung", "All Platforms"};
  int y = CONTENT_Y + 4;

  _display.setCursor(0, y);
  printBold("Target: ");
  printBold(modeNames[mode]);
  y += 12;

  _display.setCursor(0, y);
  printBold("Packets: ");
  printBold(count);
  y += 12;

  _display.setCursor(0, y);
  printBold("Time: ");
  printBold(formatUptime(elapsed));

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── AP Clone Screen ───────────────────────────────────────
void DisplayUI::drawAPCloneRunning(const char* ssid, int clients) {
  _display.clearDisplay();
  drawStatusBar("Rogue AP Clone");

  int y = CONTENT_Y + 6;
  _display.setCursor(0, y);
  printBold("SSID: ");
  printBold(ssid);
  y += 12;

  _display.setCursor(0, y);
  printBold("Clients Connected: ");
  printBold(clients);
  y += 12;

  _display.setCursor(0, y);
  printBold("IP: 192.168.4.1");

  drawFooter("\x07:Stop", "");
  _display.display();
}

// ── Port Scanner Screen ───────────────────────────────────
void DisplayUI::drawPortScannerRunning(IPAddress target, int scanned, int total, OpenPort* ports, int openCount) {
  _display.clearDisplay();
  drawStatusBar("Port Scanner");

  int y = CONTENT_Y + 2;
  _display.setCursor(0, y);
  printBold("Target: ");
  printBold(target);
  y += 10;

  _display.setCursor(0, y);
  printBold("Progress: ");
  printBold(scanned); printBold("/"); printBold(total);
  y += 10;

  _display.setCursor(0, y);
  printBold("Open: ");
  printBold(openCount);
  if (openCount > 0) {
    printBold(" (");
    printBold(ports[openCount - 1].port);
    printBold(")");
  }

  drawFooter("\x07:Stop", "");
  _display.display();
}
