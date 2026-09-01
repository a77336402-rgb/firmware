/*
 * ============================================================
 *  GhostNet — network_attacks.h
 *  Port scanner, DHCP starvation
 * ============================================================
 */

#ifndef NETWORK_ATTACKS_H
#define NETWORK_ATTACKS_H

#include <Arduino.h>
#include "config.h"

// ── Data Structures ────────────────────────────────────────
struct OpenPort {
  uint16_t port;
  char     service[16];
};

// ── NetworkAttacks Class ───────────────────────────────────
class NetworkAttacks {
public:
  NetworkAttacks();

  // ── Port Scanner ──
  void     startPortScan(IPAddress target);
  void     stopPortScan();
  void     scanNextPort();
  bool     isPortScanRunning() const;
  int      getOpenPortCount() const;
  OpenPort* getOpenPort(int index);
  OpenPort* getOpenPorts();
  int      getScannedPortCount() const;
  int      getTotalPortsToScan() const;
  IPAddress getTargetIP() const;

  // ── DHCP Starvation ──
  void     startDHCPStarvation();
  void     stopDHCPStarvation();
  void     sendDHCPDiscover();
  bool     isDHCPRunning() const;
  uint32_t getDHCPCount() const;

private:
  // Port scanner state
  bool       _portScanRunning;
  IPAddress  _scanTarget;
  OpenPort   _openPorts[MAX_OPEN_PORTS];
  int        _openPortCount;
  int        _currentPortIdx;
  bool       _scanComplete;

  // DHCP state
  bool       _dhcpRunning;
  uint32_t   _dhcpCount;
  unsigned long _lastDhcpMs;

  // Common ports to scan
  static const uint16_t COMMON_PORTS[];
  static const char*    PORT_SERVICES[];
  static const int      NUM_COMMON_PORTS;

  const char* _getServiceName(uint16_t port);
};

#endif // NETWORK_ATTACKS_H
