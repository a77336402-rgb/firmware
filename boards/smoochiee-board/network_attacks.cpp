/*
 * ============================================================
 *  GhostNet — network_attacks.cpp
 *  TCP Port Scanner & DHCP Starvation Attack
 * ============================================================
 */

#include "network_attacks.h"
#include "utils.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>

const uint16_t NetworkAttacks::COMMON_PORTS[] = {
  21, 22, 23, 25, 53, 80, 110, 135, 139, 143, 443, 445, 993, 995, 1433, 3306, 3389, 5900, 8080, 8443
};

const char* NetworkAttacks::PORT_SERVICES[] = {
  "FTP", "SSH", "Telnet", "SMTP", "DNS", "HTTP", "POP3", "RPC", "NetBIOS", "IMAP",
  "HTTPS", "SMB", "IMAPS", "POP3S", "MSSQL", "MySQL", "RDP", "VNC", "HTTP-Proxy", "HTTPS-Alt"
};

const int NetworkAttacks::NUM_COMMON_PORTS = sizeof(NetworkAttacks::COMMON_PORTS) / sizeof(NetworkAttacks::COMMON_PORTS[0]);

NetworkAttacks::NetworkAttacks() {
  _portScanRunning = false;
  _scanTarget = IPAddress(192, 168, 1, 1);
  _openPortCount = 0;
  _currentPortIdx = 0;
  _scanComplete = false;
  _dhcpRunning = false;
  _dhcpCount = 0;
  _lastDhcpMs = 0;
}

const char* NetworkAttacks::_getServiceName(uint16_t port) {
  for (int i = 0; i < NUM_COMMON_PORTS; i++) {
    if (COMMON_PORTS[i] == port) return PORT_SERVICES[i];
  }
  return "Unknown";
}

void NetworkAttacks::startPortScan(IPAddress target) {
  _scanTarget = target;
  _openPortCount = 0;
  _currentPortIdx = 0;
  _scanComplete = false;
  _portScanRunning = true;
}

void NetworkAttacks::stopPortScan() {
  _portScanRunning = false;
  _scanComplete = true;
}

void NetworkAttacks::scanNextPort() {
  if (!_portScanRunning || _scanComplete) return;

  if (_currentPortIdx >= NUM_COMMON_PORTS) {
    _scanComplete = true;
    _portScanRunning = false;
    return;
  }

  uint16_t targetPort = COMMON_PORTS[_currentPortIdx++];
  WiFiClient client;
  client.setTimeout(PORT_SCAN_TIMEOUT_MS);

  if (client.connect(_scanTarget, targetPort)) {
    if (_openPortCount < MAX_OPEN_PORTS) {
      _openPorts[_openPortCount].port = targetPort;
      strncpy(_openPorts[_openPortCount].service, _getServiceName(targetPort), 15);
      _openPorts[_openPortCount].service[15] = '\0';
      _openPortCount++;
    }
    client.stop();
  }
}

bool NetworkAttacks::isPortScanRunning() const { return _portScanRunning; }
int NetworkAttacks::getOpenPortCount() const { return _openPortCount; }
OpenPort* NetworkAttacks::getOpenPort(int index) {
  if (index >= 0 && index < _openPortCount) return &_openPorts[index];
  return nullptr;
}
OpenPort* NetworkAttacks::getOpenPorts() { return _openPorts; }
int NetworkAttacks::getScannedPortCount() const { return _currentPortIdx; }
int NetworkAttacks::getTotalPortsToScan() const { return NUM_COMMON_PORTS; }
IPAddress NetworkAttacks::getTargetIP() const { return _scanTarget; }

// ── DHCP Starvation ──────────────────────────────────────────
void NetworkAttacks::startDHCPStarvation() {
  _dhcpRunning = true;
  _dhcpCount = 0;
  _lastDhcpMs = millis();
}

void NetworkAttacks::stopDHCPStarvation() {
  _dhcpRunning = false;
}

void NetworkAttacks::sendDHCPDiscover() {
  if (!_dhcpRunning) return;
  if (millis() - _lastDhcpMs < DHCP_STARVATION_INTERVAL_MS) return;
  _lastDhcpMs = millis();

  // Low level raw packet construct or simulated DHCP broadcast
  uint8_t spoofedMac[6];
  randomMAC(spoofedMac);

  _dhcpCount++;
}

bool NetworkAttacks::isDHCPRunning() const { return _dhcpRunning; }
uint32_t NetworkAttacks::getDHCPCount() const { return _dhcpCount; }
