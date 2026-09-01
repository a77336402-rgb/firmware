/*
 * ============================================================
 *  GhostNet — packet_monitor.cpp
 *  Promiscuous Packet Sniffing, Handshake (EAPOL) Capture,
 *  Channel Analysis & Deauth Detection
 * ============================================================
 */

#include "packet_monitor.h"
#include "utils.h"
#include <WiFi.h>
#include <esp_wifi.h>

PacketMonitor packetMonitor;

static PacketStats g_stats;

PacketMonitor::PacketMonitor() {
  _running = false;
  _channel = 1;
  _lastHopMs = 0;
  _lastPpsMs = 0;
  _ppsCounter = 0;
  _deauthAlert = false;
  _historyIndex = 0;
  _capturingHandshake = false;
  _analyzingChannels = false;
  _analyzeChannel = 1;
  _analyzeStartMs = 0;
  _analyzePktCount = 0;
  memset(_ppsHistory, 0, sizeof(_ppsHistory));
  memset(&_handshake, 0, sizeof(_handshake));
  memset(_channelInfo, 0, sizeof(_channelInfo));
}

void PacketMonitor::promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* payload = pkt->payload;
  uint16_t len = pkt->rx_ctrl.sig_len;

  g_stats.total++;
  packetMonitor._ppsCounter++;

  if (type == WIFI_PKT_MGMT) {
    g_stats.mgmt++;
    uint8_t subtype = payload[0] & 0xF0;
    if (subtype == 0x80) g_stats.beacon++;
    else if (subtype == 0x40) g_stats.probe++;
    else if (subtype == 0xC0) {
      g_stats.deauth++;
      packetMonitor._deauthAlert = true;
    }
  } else if (type == WIFI_PKT_DATA) {
    g_stats.data++;
    // Check for EAPOL (802.1X key exchange frames)
    if (len >= 36) {
      // Look for LLC/SNAP 888e (EAPOL)
      for (int i = 24; i < min((int)len - 8, 48); i++) {
        if (payload[i] == 0x88 && payload[i+1] == 0x8E) {
          g_stats.eapol++;
          if (packetMonitor._capturingHandshake) {
            uint8_t eapolType = payload[i+3]; // 3 = Key
            if (eapolType == 0x03) {
              packetMonitor._handshake.gotM1 = true;
              packetMonitor._handshake.gotM2 = true;
              packetMonitor._handshake.complete = true;
            }
          }
          break;
        }
      }
    }
  } else if (type == WIFI_PKT_CTRL) {
    g_stats.ctrl++;
  }
}

void PacketMonitor::start() {
  resetStats();
  _running = true;
  _channel = 1;
  _lastHopMs = millis();
  _lastPpsMs = millis();
  _ppsCounter = 0;

  WiFi.mode(WIFI_MODE_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&PacketMonitor::promiscuousCallback);
  esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
}

void PacketMonitor::stop() {
  _running = false;
  esp_wifi_set_promiscuous(false);
}

void PacketMonitor::update() {
  if (!_running) return;

  unsigned long now = millis();

  // Channel hopping
  if (now - _lastHopMs >= CHANNEL_HOP_MS) {
    _lastHopMs = now;
    _channel = (_channel % MAX_CHANNELS) + 1;
    esp_wifi_set_channel(_channel, WIFI_SECOND_CHAN_NONE);
  }

  // PPS calculation
  if (now - _lastPpsMs >= 1000) {
    _lastPpsMs = now;
    g_stats.pps = _ppsCounter;
    _ppsCounter = 0;

    _ppsHistory[_historyIndex] = g_stats.pps;
    _historyIndex = (_historyIndex + 1) % PKT_HISTORY_LEN;
  }
}

bool PacketMonitor::isRunning() const { return _running; }
PacketStats PacketMonitor::getStats() const { return g_stats; }
void PacketMonitor::resetStats() {
  memset(&g_stats, 0, sizeof(g_stats));
  _ppsCounter = 0;
  _deauthAlert = false;
}

uint16_t* PacketMonitor::getPPSHistory() { return _ppsHistory; }
int PacketMonitor::getHistoryLen() const { return PKT_HISTORY_LEN; }
int PacketMonitor::getCurrentChannel() const { return _channel; }

uint32_t PacketMonitor::getDeauthCount() const { return g_stats.deauth; }
bool PacketMonitor::isDeauthAlert() const { return _deauthAlert; }
void PacketMonitor::clearDeauthAlert() { _deauthAlert = false; }

void PacketMonitor::startHandshakeCapture(const uint8_t* targetBSSID, const char* ssid) {
  _capturingHandshake = true;
  memset(&_handshake, 0, sizeof(_handshake));
  if (targetBSSID) memcpy(_handshake.bssid, targetBSSID, 6);
  if (ssid) strncpy(_handshake.ssid, ssid, 32);
  start();
}

void PacketMonitor::stopHandshakeCapture() {
  _capturingHandshake = false;
  stop();
}

bool PacketMonitor::isCapturingHandshake() const { return _capturingHandshake; }
HandshakeInfo PacketMonitor::getHandshakeInfo() const { return _handshake; }

void PacketMonitor::startChannelAnalysis() {
  _analyzingChannels = true;
  _analyzeChannel = 1;
  _analyzeStartMs = millis();
  memset(_channelInfo, 0, sizeof(_channelInfo));
  for (int i = 0; i < MAX_CHANNELS; i++) {
    _channelInfo[i].channel = i + 1;
  }
  start();
}

void PacketMonitor::stopChannelAnalysis() {
  _analyzingChannels = false;
  stop();
}

bool PacketMonitor::isAnalyzingChannels() const { return _analyzingChannels; }
ChannelInfo* PacketMonitor::getChannelInfo() { return _channelInfo; }

int PacketMonitor::getMostCongestedChannel() const {
  uint32_t maxPkts = 0;
  int ch = 1;
  for (int i = 0; i < MAX_CHANNELS; i++) {
    if (_channelInfo[i].packetCount > maxPkts) {
      maxPkts = _channelInfo[i].packetCount;
      ch = i + 1;
    }
  }
  return ch;
}

int PacketMonitor::getLeastCongestedChannel() const {
  uint32_t minPkts = 0xFFFFFFFF;
  int ch = 1;
  for (int i = 0; i < MAX_CHANNELS; i++) {
    if (_channelInfo[i].packetCount < minPkts) {
      minPkts = _channelInfo[i].packetCount;
      ch = i + 1;
    }
  }
  return ch;
}
