# MATRIX HOME SOC Operations Guide

This document explains the operation and monitoring features of MATRIX HOME SOC.

MATRIX HOME SOC uses a Raspberry Pi 3 as its monitoring server and an M5Stack Core2 as a dedicated security dashboard.

## 1. System Overview

MATRIX HOME SOC integrates the following components:

| Component | Role |
|---|---|
| Nmap | Network device discovery |
| Suricata IDS | Network intrusion detection |
| Eclipse Mosquitto | MQTT message broker |
| SQLite | Device and security event storage |
| Python services | Monitoring and event processing |
| M5Stack Core2 | Real-time monitoring dashboard |
| systemd | Background service management |

The Raspberry Pi performs monitoring and event processing. The M5Stack Core2 displays the collected information through MQTT.

## 2. M5Stack Core2 Dashboard

The M5Stack Core2 provides eight monitoring pages.

### HOME

Displays an overview of the monitoring system.

This page provides a quick view of the system's current status.

### NETWORK

Displays network-related information collected by the monitoring system.

Use this page to review the network monitoring status.

### THREATS

Displays security events detected and processed by the system.

Security events are collected from Suricata through the Suricata bridge.

### DEVICES

Displays discovered network devices.

Device discovery and inventory management are performed using Nmap.

The inventory is refreshed periodically. The configured inventory freshness period is 15 minutes.

### TRAFFIC

Displays network interface traffic statistics.

The Raspberry Pi traffic monitoring service measures received and transmitted traffic and publishes the results through MQTT.

Traffic measurements are periodically refreshed. The dashboard uses a 10-second timeout for traffic data freshness.

### SYSTEM

Displays information about the monitoring server and system operation.

Use this page to review the server's operational status.

### POWER

Displays power-related information available to the M5Stack Core2.

### SETTINGS

Provides dashboard settings and navigation-related controls.

## 3. Dashboard Navigation

The dashboard uses a Matrix-inspired black and neon-green HUD design.

The interface runs in landscape orientation at 320 × 240 resolution.

The eight pages are organized into navigation groups:

| Navigation Group | Pages |
|---|---|
| Group 1 | HOME / NETWORK / THREATS / MORE |
| Group 2 | DEVICES / TRAFFIC / SYSTEM / MORE |
| Group 3 | POWER / SETTINGS / HOME / BACK |

The display uses M5Canvas double-buffered rendering to reduce visible drawing flicker.

## 4. Network Device Discovery

Nmap is used to discover devices on the configured network.

The monitoring service periodically scans the configured network and maintains a device inventory.

The inventory is published through MQTT using:

```text
matrix/inventory
```

The dashboard uses this information to display discovered devices.

The configured scan interval is 300 seconds.

## 5. Intrusion Detection and Threat Events

Suricata monitors traffic visible to the configured network interface.

Detected events are written to the Suricata EVE JSON log.

The Suricata bridge processes these events and stores relevant information in SQLite.

The event information includes:

- Event source
- Suricata signature ID (SID)
- Source IP address
- Destination IP address
- Destination port
- Protocol
- Signature description

The processed events are made available to the dashboard through MQTT.

Related topics include:

```text
matrix/events
matrix/requests/threats
matrix/threats/history
```

The dashboard displays recent threat history. The configured history limit is 10 entries.

## 6. Network Traffic Monitoring

The traffic monitoring service measures network interface statistics using the Linux network interface counters.

The service collects:

- Received traffic
- Transmitted traffic
- Traffic rate
- Cumulative traffic totals

The measurements are published through:

```text
matrix/traffic
```

The M5Stack Core2 uses this information to visualize network traffic.

The traffic monitoring interval is approximately 2 seconds.

## 7. MQTT Communication

MQTT provides communication between the Raspberry Pi services and the M5Stack Core2.

The system uses Eclipse Mosquitto as its message broker.

| MQTT Topic | Description |
|---|---|
| `matrix/status` | System status |
| `matrix/events` | Network and security events |
| `matrix/inventory` | Discovered device inventory |
| `matrix/requests/threats` | Threat history requests |
| `matrix/threats/history` | Threat history responses |
| `matrix/traffic` | Network traffic statistics |

The MQTT broker uses authenticated access.

## 8. Background Services

MATRIX HOME SOC uses systemd to manage its monitoring services.

| Service | Responsibility |
|---|---|
| `matrix-home-soc` | Network discovery and device monitoring |
| `matrix-suricata-bridge` | Suricata event processing and database integration |
| `matrix-traffic` | Network traffic measurement and MQTT publishing |
| `mosquitto` | MQTT message broker |
| `suricata` | Intrusion detection engine |

### Check Service Status

```bash
sudo systemctl status matrix-home-soc
sudo systemctl status matrix-suricata-bridge
sudo systemctl status matrix-traffic
sudo systemctl status mosquitto
sudo systemctl status suricata
```

### Restart a Service

For example, restart the network monitoring service:

```bash
sudo systemctl restart matrix-home-soc
```

### View Service Logs

```bash
sudo journalctl -u matrix-home-soc -n 50
sudo journalctl -u matrix-suricata-bridge -n 50
sudo journalctl -u matrix-traffic -n 50
```

### Check Enabled Services

```bash
systemctl is-enabled matrix-home-soc
systemctl is-enabled matrix-suricata-bridge
systemctl is-enabled matrix-traffic
```

The services are configured to start automatically through systemd.

## 9. Database

SQLite stores device inventory and security event information.

The database is located at:

```text
/opt/matrix-home-soc/data/matrix.db
```

The database schema is provided in:

```text
database/matrix_schema.sql
```

## 10. Monitoring Scope and Limitations

The Raspberry Pi operates as a Wi-Fi-connected monitoring device.

The system monitors traffic visible to its own network interface.

It does not provide complete visibility into all unicast communications between other devices on the home LAN.

Full LAN-wide monitoring would require additional network infrastructure, such as switch port mirroring or an appropriately positioned network sensor.

## 11. Operational Notes

- Keep the Raspberry Pi connected to the local network.
- Ensure Mosquitto is running for MQTT communication.
- Ensure Suricata is running for intrusion detection.
- Check systemd service status if monitoring information stops updating.
- Protect MQTT credentials and local configuration files.
- Do not publish private IP addresses, MAC addresses, or household device identifiers.

## 12. Project Repository

Source code, configuration examples, database schemas, service definitions, installation instructions, and M5Stack firmware are available in the project repository.

https://github.com/ryoimaoka/matrix-home-soc
