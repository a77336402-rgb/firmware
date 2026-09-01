/*
 * ============================================================
 *  GhostNet — ESP32-S3 WiFi/BLE & HID Security Analysis Tool
 *  Main Arduino Sketch File (GhostNet.ino)
 * ============================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>

#include "config.h"
#include "utils.h"
#include "display_ui.h"
#include "wifi_module.h"
#include "ble_module.h"
#include "packet_monitor.h"
#include "evil_portal.h"
#include "network_attacks.h"
#include "badusb.h"

// ── Module Instances ────────────────────────────────────────
DisplayUI      ui;
BleModule      bleModule;
EvilPortal     evilPortal;
NetworkAttacks netAttacks;
BadUSB         badUsb;

// ── State Machine ──────────────────────────────────────────
AppState currentAppState = STATE_BOOT;
int menuSelectedIndex   = 0;
int menuScrollOffset    = 0;
int subListSelected     = 0;
int subListScroll       = 0;
int beaconModeSelected  = 0;
int bleSpamModeSelected = 0;
int settingsSelected    = 0;
int settingsScroll      = 0;

// ── Button Debounce State ──────────────────────────────────
unsigned long lastBtnCheckMs = 0;
unsigned long btnSelectPressTime = 0;
bool btnSelectWasPressed = false;

// ── Forward Declarations ───────────────────────────────────
void handleButtons();
void updateUI();
void handleSerialCLI();

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  Serial.println(F("\n[+] GhostNet ESP32-S3 Initializing..."));

  // Button pins setup
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  // Initialize OLED Display
  if (ui.init()) {
    ui.showBootScreen();
  }

  // Initialize Modules
  wifiModule.init();
  bleModule.init();

  currentAppState = STATE_MENU;
  Serial.println(F("[+] GhostNet System Ready."));
}

void loop() {
  handleButtons();
  handleSerialCLI();

  // Background active workers
  if (wifiModule.isDeauthRunning()) {
    wifiModule.sendDeauthFrame();
  }
  if (wifiModule.isBeaconRunning()) {
    wifiModule.sendBeaconFrame();
  }
  if (bleModule.isSpamming()) {
    bleModule.sendSpamPacket();
  }
  if (packetMonitor.isRunning()) {
    packetMonitor.update();
  }
  if (evilPortal.isRunning()) {
    evilPortal.handleClient();
  }
  if (netAttacks.isPortScanRunning()) {
    netAttacks.scanNextPort();
  }
  if (netAttacks.isDHCPRunning()) {
    netAttacks.sendDHCPDiscover();
  }
  if (badUsb.isRunning()) {
    badUsb.executeNextLine();
  }

  // Refresh display at target FPS
  static unsigned long lastFrameMs = 0;
  if (millis() - lastFrameMs >= (1000 / DISPLAY_FPS)) {
    lastFrameMs = millis();
    updateUI();
  }
}

// ── Button Handling ─────────────────────────────────────────
void handleButtons() {
  if (millis() - lastBtnCheckMs < 20) return;
  lastBtnCheckMs = millis();

  bool upPressed     = (digitalRead(BTN_UP) == LOW);
  bool downPressed   = (digitalRead(BTN_DOWN) == LOW);
  bool selectPressed = (digitalRead(BTN_SELECT) == LOW);

  // UP Button
  static bool lastUp = false;
  if (upPressed && !lastUp) {
    if (currentAppState == STATE_MENU) {
      if (menuSelectedIndex > 0) menuSelectedIndex--;
      if (menuSelectedIndex < menuScrollOffset) menuScrollOffset = menuSelectedIndex;
    } else if (currentAppState == STATE_WIFI_SCAN || currentAppState == STATE_BLE_SCAN || currentAppState == STATE_DEAUTH_SELECT) {
      if (subListSelected > 0) subListSelected--;
      if (subListSelected < subListScroll) subListScroll = subListSelected;
    } else if (currentAppState == STATE_BEACON_SELECT) {
      if (beaconModeSelected > 0) beaconModeSelected--;
    } else if (currentAppState == STATE_BLE_SPAM_SELECT) {
      if (bleSpamModeSelected > 0) bleSpamModeSelected--;
    } else if (currentAppState == STATE_SETTINGS) {
      if (settingsSelected > 0) settingsSelected--;
      if (settingsSelected < settingsScroll) settingsScroll = settingsSelected;
    }
  }
  lastUp = upPressed;

  // DOWN Button
  static bool lastDown = false;
  if (downPressed && !lastDown) {
    if (currentAppState == STATE_MENU) {
      if (menuSelectedIndex < MENU_TOTAL_ITEMS - 1) menuSelectedIndex++;
      if (menuSelectedIndex >= menuScrollOffset + VISIBLE_MENU_ITEMS) menuScrollOffset = menuSelectedIndex - VISIBLE_MENU_ITEMS + 1;
    } else if (currentAppState == STATE_WIFI_SCAN) {
      if (subListSelected < wifiModule.getNetworkCount() - 1) subListSelected++;
      if (subListSelected >= subListScroll + VISIBLE_MENU_ITEMS) subListScroll = subListSelected - VISIBLE_MENU_ITEMS + 1;
    } else if (currentAppState == STATE_BLE_SCAN) {
      if (subListSelected < bleModule.getDeviceCount() - 1) subListSelected++;
      if (subListSelected >= subListScroll + VISIBLE_MENU_ITEMS) subListScroll = subListSelected - VISIBLE_MENU_ITEMS + 1;
    } else if (currentAppState == STATE_DEAUTH_SELECT) {
      if (subListSelected < wifiModule.getNetworkCount() - 1) subListSelected++;
      if (subListSelected >= subListScroll + VISIBLE_MENU_ITEMS) subListScroll = subListSelected - VISIBLE_MENU_ITEMS + 1;
    } else if (currentAppState == STATE_BEACON_SELECT) {
      if (beaconModeSelected < BEACON_MODE_COUNT - 1) beaconModeSelected++;
    } else if (currentAppState == STATE_BLE_SPAM_SELECT) {
      if (bleSpamModeSelected < BLE_SPAM_MODE_COUNT - 1) bleSpamModeSelected++;
    } else if (currentAppState == STATE_SETTINGS) {
      if (settingsSelected < 3) settingsSelected++;  // 4 settings items (0-3)
      if (settingsSelected >= settingsScroll + VISIBLE_MENU_ITEMS) settingsScroll = settingsSelected - VISIBLE_MENU_ITEMS + 1;
    }
  }
  lastDown = downPressed;

  // SELECT Button (Click & Long-Press Detection)
  if (selectPressed) {
    if (!btnSelectWasPressed) {
      btnSelectWasPressed = true;
      btnSelectPressTime = millis();
    } else if (millis() - btnSelectPressTime >= LONG_PRESS_MS) {
      // Long press -> Back to menu
      btnSelectWasPressed = false;
      if (currentAppState != STATE_MENU) {
        // Stop any running operations
        wifiModule.stopDeauth();
        wifiModule.stopBeaconSpam();
        wifiModule.stopProbeSniffer();
        wifiModule.stopCredSniffer();
        wifiModule.stopAPClone();
        bleModule.stopScan();
        bleModule.stopSpam();
        bleModule.stopAirTagSpoof();
        packetMonitor.stop();
        evilPortal.stop();
        netAttacks.stopPortScan();
        netAttacks.stopDHCPStarvation();
        badUsb.stopExecution();
        currentAppState = STATE_MENU;
        delay(200);
      }
    }
  } else {
    if (btnSelectWasPressed) {
      unsigned long duration = millis() - btnSelectPressTime;
      btnSelectWasPressed = false;

      if (duration < LONG_PRESS_MS) {
        // Short Click Action
        if (currentAppState == STATE_MENU) {
          switch (menuSelectedIndex) {
            case MENU_WIFI_SCAN:
              currentAppState = STATE_WIFI_SCAN;
              subListSelected = 0; subListScroll = 0;
              wifiModule.scanNetworks();
              break;
            case MENU_BLE_SCAN:
              currentAppState = STATE_BLE_SCAN;
              subListSelected = 0; subListScroll = 0;
              bleModule.startScan();
              break;
            case MENU_PACKET_MON:
              currentAppState = STATE_PACKET_MONITOR;
              packetMonitor.start();
              break;
            case MENU_DEAUTH:
              currentAppState = STATE_DEAUTH_SELECT;
              subListSelected = 0; subListScroll = 0;
              if (wifiModule.getNetworkCount() == 0) wifiModule.scanNetworks();
              break;
            case MENU_BEACON_SPAM:
              currentAppState = STATE_BEACON_SELECT;
              beaconModeSelected = 0;
              break;
            case MENU_EVIL_PORTAL:
              currentAppState = STATE_EVIL_PORTAL_RUNNING;
              evilPortal.start(PORTAL_SSID);
              break;
            case MENU_PROBE_SNIFF:
              currentAppState = STATE_PROBE_SNIFFER;
              wifiModule.startProbeSniffer();
              break;
            case MENU_HANDSHAKE:
              currentAppState = STATE_HANDSHAKE_CAPTURE;
              packetMonitor.startHandshakeCapture(nullptr, "Any");
              break;
            case MENU_BLE_SPAM:
              currentAppState = STATE_BLE_SPAM_SELECT;
              bleSpamModeSelected = 0;
              break;
            case MENU_AP_CLONE:
              currentAppState = STATE_AP_CLONE_RUNNING;
              wifiModule.startAPClone(0);
              break;
            case MENU_DEAUTH_DET:
              currentAppState = STATE_DEAUTH_DETECTOR;
              packetMonitor.start();
              break;
            case MENU_PORT_SCAN:
              currentAppState = STATE_PORT_SCANNER_RUNNING;
              netAttacks.startPortScan(IPAddress(192, 168, 4, 1));
              break;
            case MENU_DEVICE_INFO:
              currentAppState = STATE_DEVICE_INFO;
              break;
            case MENU_SETTINGS:
              currentAppState = STATE_SETTINGS;
              settingsSelected = 0;
              settingsScroll = 0;
              break;
          }
        } else if (currentAppState == STATE_WIFI_SCAN) {
          if (wifiModule.getNetworkCount() > 0) currentAppState = STATE_WIFI_SCAN_DETAIL;
        } else if (currentAppState == STATE_BLE_SCAN) {
          if (bleModule.getDeviceCount() > 0) currentAppState = STATE_BLE_SCAN_DETAIL;
        } else if (currentAppState == STATE_DEAUTH_SELECT) {
          if (wifiModule.getNetworkCount() > 0) {
            wifiModule.startDeauth(subListSelected);
            currentAppState = STATE_DEAUTH_RUNNING;
          }
        } else if (currentAppState == STATE_BEACON_SELECT) {
          wifiModule.startBeaconSpam((BeaconMode)beaconModeSelected);
          currentAppState = STATE_BEACON_RUNNING;
        } else if (currentAppState == STATE_BLE_SPAM_SELECT) {
          bleModule.startSpam((BLESpamMode)bleSpamModeSelected);
          currentAppState = STATE_BLE_SPAM_RUNNING;
        } else if (currentAppState == STATE_DEAUTH_RUNNING || currentAppState == STATE_BEACON_RUNNING ||
                   currentAppState == STATE_PACKET_MONITOR || currentAppState == STATE_EVIL_PORTAL_RUNNING ||
                   currentAppState == STATE_BLE_SPAM_RUNNING || currentAppState == STATE_DEAUTH_DETECTOR) {
          // Stop and return
          wifiModule.stopDeauth();
          wifiModule.stopBeaconSpam();
          bleModule.stopSpam();
          packetMonitor.stop();
          evilPortal.stop();
          currentAppState = STATE_MENU;
        }
      }
    }
  }
}

// ── UI Rendering State Machine ──────────────────────────────
void updateUI() {
  switch (currentAppState) {
    case STATE_MENU:
      ui.drawMainMenu(menuSelectedIndex, menuScrollOffset);
      break;

    case STATE_WIFI_SCAN:
      ui.drawWifiScanScreen(wifiModule.getNetworks(), wifiModule.getNetworkCount(), subListSelected, subListScroll);
      break;

    case STATE_WIFI_SCAN_DETAIL:
      if (wifiModule.getNetwork(subListSelected)) {
        ui.drawWifiDetailScreen(*wifiModule.getNetwork(subListSelected));
      }
      break;

    case STATE_BLE_SCAN:
      ui.drawBleScanScreen(bleModule.getDevices(), bleModule.getDeviceCount(), subListSelected, subListScroll);
      break;

    case STATE_BLE_SCAN_DETAIL:
      if (bleModule.getDevice(subListSelected)) {
        ui.drawBleDetailScreen(*bleModule.getDevice(subListSelected));
      }
      break;

    case STATE_PACKET_MONITOR:
      ui.drawPacketMonitor(packetMonitor.getStats(), packetMonitor.getPPSHistory(), packetMonitor.getHistoryLen(), packetMonitor.getCurrentChannel());
      break;

    case STATE_DEAUTH_SELECT:
      ui.drawDeauthSelect(wifiModule.getNetworks(), wifiModule.getNetworkCount(), subListSelected, subListScroll);
      break;

    case STATE_DEAUTH_RUNNING: {
      NetworkInfo* target = wifiModule.getNetwork(wifiModule.getDeauthTargetIndex());
      if (target) {
        ui.drawDeauthRunning(*target, wifiModule.getDeauthCount(), millis());
      }
      break;
    }

    case STATE_BEACON_SELECT:
      ui.drawBeaconSelect(beaconModeSelected);
      break;

    case STATE_BEACON_RUNNING:
      ui.drawBeaconRunning(wifiModule.getBeaconMode(), wifiModule.getBeaconCount(), millis());
      break;

    case STATE_EVIL_PORTAL_RUNNING:
      ui.drawEvilPortalRunning(evilPortal.getSSID(), evilPortal.getClientCount(), evilPortal.getCreds(), evilPortal.getCredCount(), 0);
      break;

    case STATE_DEAUTH_DETECTOR:
      ui.drawDeauthDetector(packetMonitor.getDeauthCount(), packetMonitor.isDeauthAlert(), packetMonitor.getCurrentChannel());
      break;

    case STATE_PROBE_SNIFFER:
      ui.drawProbeSniffer(wifiModule.getProbes(), wifiModule.getProbeCount(), subListScroll);
      break;

    case STATE_HANDSHAKE_CAPTURE:
      ui.drawHandshakeCapture(packetMonitor.getHandshakeInfo());
      break;

    case STATE_BLE_SPAM_RUNNING:
      ui.drawBleSpamRunning(bleModule.getSpamMode(), bleModule.getSpamCount(), millis());
      break;

    case STATE_AP_CLONE_RUNNING:
      ui.drawAPCloneRunning(wifiModule.getAPCloneSSID(), wifiModule.getAPCloneClients());
      break;

    case STATE_PORT_SCANNER_RUNNING:
      ui.drawPortScannerRunning(netAttacks.getTargetIP(), netAttacks.getScannedPortCount(), netAttacks.getTotalPortsToScan(), netAttacks.getOpenPorts(), netAttacks.getOpenPortCount());
      break;

    case STATE_DEVICE_INFO:
      ui.drawDeviceInfo();
      break;

    case STATE_SETTINGS:
      ui.drawSettings(settingsSelected, settingsScroll);
      break;

    default:
      break;
  }
}

// ── Serial CLI for Remote Control ───────────────────────────
void handleSerialCLI() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.equalsIgnoreCase("help")) {
    Serial.println(F("\n=== GhostNet Serial CLI ==="));
    Serial.println(F("scan wifi       - Scan WiFi networks"));
    Serial.println(F("scan ble        - Scan BLE devices"));
    Serial.println(F("deauth <index>  - Launch deauth on AP index"));
    Serial.println(F("beacon <mode>   - Start beacon spam (0=rand, 1=funny, 2=rickroll)"));
    Serial.println(F("portal <ssid>   - Launch evil portal"));
    Serial.println(F("blespam <mode>  - Start BLE spam (0=Apple, 1=Android, 4=All)"));
    Serial.println(F("badusb <index>  - Execute BadUSB payload (0=Notepad, 1=Rickroll)"));
    Serial.println(F("stop            - Stop active attacks/monitors"));
    Serial.println(F("info            - Show device stats\n"));
  } else if (cmd.equalsIgnoreCase("scan wifi")) {
    Serial.println(F("[+] Scanning WiFi..."));
    int count = wifiModule.scanNetworks();
    Serial.printf("[+] Found %d APs:\n", count);
    for (int i = 0; i < count; i++) {
      NetworkInfo* net = wifiModule.getNetwork(i);
      Serial.printf(" [%02d] %-20s (CH:%02d, RSSI:%03d dBm, BSSID:%s)\n", i, net->ssid, net->channel, net->rssi, macToString(net->bssid).c_str());
    }
  } else if (cmd.equalsIgnoreCase("scan ble")) {
    Serial.println(F("[+] Scanning BLE..."));
    bleModule.startScan();
    Serial.printf("[+] Found %d BLE devices\n", bleModule.getDeviceCount());
  } else if (cmd.startsWith("deauth ")) {
    int idx = cmd.substring(7).toInt();
    wifiModule.startDeauth(idx);
    currentAppState = STATE_DEAUTH_RUNNING;
    Serial.printf("[+] Deauth launched on AP index %d\n", idx);
  } else if (cmd.startsWith("beacon ")) {
    int mode = cmd.substring(7).toInt();
    wifiModule.startBeaconSpam((BeaconMode)mode);
    currentAppState = STATE_BEACON_RUNNING;
    Serial.println(F("[+] Beacon spam started"));
  } else if (cmd.startsWith("portal ")) {
    String ssid = cmd.substring(7);
    evilPortal.start(ssid.c_str());
    currentAppState = STATE_EVIL_PORTAL_RUNNING;
    Serial.printf("[+] Evil portal started with SSID: %s\n", ssid.c_str());
  } else if (cmd.startsWith("badusb ")) {
    int idx = cmd.substring(7).toInt();
    badUsb.selectPayload(idx);
    badUsb.startExecution();
    Serial.printf("[+] Executing BadUSB payload %d...\n", idx);
  } else if (cmd.equalsIgnoreCase("stop")) {
    wifiModule.stopDeauth();
    wifiModule.stopBeaconSpam();
    bleModule.stopSpam();
    packetMonitor.stop();
    evilPortal.stop();
    badUsb.stopExecution();
    currentAppState = STATE_MENU;
    Serial.println(F("[+] All tasks stopped. Returned to menu."));
  } else if (cmd.equalsIgnoreCase("info")) {
    Serial.printf("[+] Heap Free: %u bytes, Uptime: %lu ms\n", esp_get_free_heap_size(), millis());
  }
}
