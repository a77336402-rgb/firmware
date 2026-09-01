/*
 * ============================================================
 *  GhostNet — ESP32-S3 WiFi/BLE Security Analysis Tool
 *  config.h — Hardware pin definitions & compile-time settings
 *  *** COMPLETE FEATURE SET ***
 * ============================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

// ── Firmware ────────────────────────────────────────────────
#define GHOSTNET_VERSION   "2.0.0"
#define GHOSTNET_NAME      "GhostNet"
#define GHOSTNET_AUTHOR    "GhostNet Team"

// ── OLED Display (SSD1306 / SH1106 128x64 I2C) ─────────────
#define SCREEN_WIDTH       128
#define SCREEN_HEIGHT      64
#define OLED_RESET         -1      // No reset pin

// OLED I2C Pins (ESP32-S3):
#define I2C_SDA            8       // OLED SDA -> GPIO 8  (GPIO 19/20 reserved for USB HID)
#define I2C_SCL            9       // OLED SCL -> GPIO 9

#define OLED_ADDR_PRIMARY  0x3C    // Primary I2C address
#define OLED_ADDR_ALT      0x3D    // Alternative I2C address
#define I2C_FREQUENCY      400000  // 400kHz I2C Fast Speed

// ── ESP32-S3 Dual USB Type-C Port Mapping ──────────────────
// 1. "COM / UART" Type-C Port: Connected to CP2102/CH343 -> Used for Code Uploading & Serial CLI.
// 2. "USB" Type-C Port: Connected directly to ESP32-S3 internal USB PHY (GPIO 19 D-, GPIO 20 D+)
//    -> Used for BadUSB / DuckyScript HID Keyboard injection! (Do not wire OLED to GPIO 19/20).

// ── Navigation Buttons ─────────────────────────────────────
#define BTN_UP             4       // GPIO for UP button
#define BTN_DOWN           5       // GPIO for DOWN button
#define BTN_SELECT         6       // GPIO for SELECT button
#define DEBOUNCE_MS        180     // Button debounce time (ms)
#define LONG_PRESS_MS      700     // Long-press threshold (ms)

// ── Display Layout ─────────────────────────────────────────
#define STATUS_BAR_H       10      // Top status bar height
#define FOOTER_H           10      // Bottom footer height
#define CONTENT_Y          (STATUS_BAR_H + 1)
#define CONTENT_H          (SCREEN_HEIGHT - STATUS_BAR_H - FOOTER_H - 2)
#define MENU_ITEM_H        10      // Height per menu item
#define VISIBLE_MENU_ITEMS 4       // Items visible in menu scroll window
#define MENU_TOTAL_ITEMS   14      // Total main-menu entries (expanded)

// ── WiFi ────────────────────────────────────────────────────
#define MAX_NETWORKS       50      // Max APs stored from scan
#define MAX_CHANNELS       13      // 1-13 (region dependent)
#define CHANNEL_HOP_MS     300     // Channel-hop interval (ms)
#define DEAUTH_INTERVAL_MS 5       // Delay between deauth frames
#define DEAUTH_REASON      7       // Reason code: Class 3 frame

// ── Beacon Spam ────────────────────────────────────────────
#define BEACON_INTERVAL_MS 1       // Delay between beacons
#define MAX_SSID_LEN       32      // Max SSID length

// ── BLE ─────────────────────────────────────────────────────
#define BLE_SCAN_TIME      5       // BLE scan window (seconds)
#define MAX_BLE_DEVICES    30      // Max BLE devices stored

// ── Packet Monitor ─────────────────────────────────────────
#define PKT_HISTORY_LEN    60      // Number of bars in graph
#define PKT_GRAPH_H        26      // Graph area height (px)

// ── Evil Portal ────────────────────────────────────────────
#define PORTAL_SSID        "Free WiFi"
#define PORTAL_CHANNEL     6
#define MAX_CREDENTIALS    20      // Max captured credentials
#define DNS_PORT           53
#define HTTP_PORT          80

// ── Probe Sniffer ──────────────────────────────────────────
#define MAX_PROBES         50      // Max stored probe requests
#define MAX_PROBE_SSID_LEN 33      // SSID + null

// ── DHCP Starvation ────────────────────────────────────────
#define DHCP_STARVATION_INTERVAL_MS 50  // Delay between DHCP discovers

// ── Port Scanner ───────────────────────────────────────────
#define PORT_SCAN_TIMEOUT_MS  200  // TCP connect timeout per port
#define MAX_OPEN_PORTS        30   // Max stored open ports
#define COMMON_PORTS_COUNT    20   // Number of common ports to scan

// ── Credential Sniffing ────────────────────────────────────
#define MAX_SNIFFED_CREDS   10     // Max captured credentials from traffic

// ── Misc ────────────────────────────────────────────────────
#define DISPLAY_FPS        15      // Target display refresh rate
#define SERIAL_BAUD        115200

// ── App States ─────────────────────────────────────────────
enum AppState {
  STATE_BOOT,
  STATE_MENU,
  // WiFi
  STATE_WIFI_SCAN,
  STATE_WIFI_SCAN_DETAIL,
  STATE_DEAUTH_SELECT,
  STATE_DEAUTH_RUNNING,
  STATE_BEACON_SELECT,
  STATE_BEACON_RUNNING,
  STATE_EVIL_PORTAL_RUNNING,
  STATE_AP_CLONE_SELECT,
  STATE_AP_CLONE_RUNNING,
  // Monitoring / Capture
  STATE_PACKET_MONITOR,
  STATE_PROBE_SNIFFER,
  STATE_DEAUTH_DETECTOR,
  STATE_HANDSHAKE_CAPTURE,
  STATE_CRED_SNIFFER,
  // BLE
  STATE_BLE_SCAN,
  STATE_BLE_SCAN_DETAIL,
  STATE_BLE_SPAM_SELECT,
  STATE_BLE_SPAM_RUNNING,
  // Network Tools
  STATE_PORT_SCANNER,
  STATE_PORT_SCANNER_RUNNING,
  STATE_DHCP_STARVATION,
  STATE_CHANNEL_ANALYZER,
  // Info / Settings
  STATE_DEVICE_INFO,
  STATE_SETTINGS,
};

// ── Menu IDs ───────────────────────────────────────────────
enum MenuItem {
  MENU_WIFI_SCAN = 0,
  MENU_BLE_SCAN,
  MENU_PACKET_MON,
  MENU_DEAUTH,
  MENU_BEACON_SPAM,
  MENU_EVIL_PORTAL,
  MENU_PROBE_SNIFF,
  MENU_HANDSHAKE,
  MENU_BLE_SPAM,
  MENU_AP_CLONE,
  MENU_DEAUTH_DET,
  MENU_PORT_SCAN,
  MENU_DEVICE_INFO,
  MENU_SETTINGS,
};

// ── Beacon Spam Modes ──────────────────────────────────────
enum BeaconMode {
  BEACON_RANDOM = 0,
  BEACON_FUNNY,
  BEACON_RICKROLL,
  BEACON_CUSTOM,
  BEACON_MODE_COUNT,
};

// ── BLE Spam Modes ─────────────────────────────────────────
enum BLESpamMode {
  BLE_SPAM_APPLE = 0,
  BLE_SPAM_ANDROID,
  BLE_SPAM_WINDOWS,
  BLE_SPAM_SAMSUNG,
  BLE_SPAM_ALL,
  BLE_SPAM_MODE_COUNT,
};

#endif // CONFIG_H
