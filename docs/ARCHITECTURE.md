# MATRIX HOME SOC Architecture

## 1. Overview

MATRIX HOME SOC is a self-hosted home network security monitoring system built with a Raspberry Pi 3 and an M5Stack Core2.

The system integrates network discovery, intrusion detection, event processing, database storage, network traffic monitoring, and hardware-based visualization.

The architecture is designed around lightweight open-source components that can operate on a resource-constrained Raspberry Pi.

## 2. High-Level Architecture

The system consists of four main layers:

1. Network monitoring
2. Event processing and storage
3. MQTT communication
4. Hardware visualization

### Architecture Diagram

```text
                  HOME NETWORK
                       |
                       |
              +------------------+
              |   Wi-Fi Router   |
              +------------------+
                       |
                       |
              +------------------+
              |  Raspberry Pi 3  |
              |                  |
              |  Nmap            |
              |  Suricata IDS    |
              |  Python Services |
              |  SQLite          |
              |  Mosquitto       |
              +------------------+
                       |
                       |
                  MQTT / Wi-Fi
                       |
                       |
              +------------------+
              |  M5Stack Core2   |
              |                  |
              |  8-Page HUD      |
              |  Dashboard       |
              +------------------+
```

## 3. Hardware Components

### Raspberry Pi 3 Model B

The Raspberry Pi acts as the central monitoring server.

Its responsibilities include:

- Network device discovery
- Intrusion detection
- Security event processing
- Database storage
- Network traffic measurement
- MQTT message distribution
- Background service management

### M5Stack Core2

The M5Stack Core2 acts as a dedicated hardware dashboard.

It receives monitoring information through MQTT and displays it across eight pages.

The dashboard uses a Matrix-inspired black and neon-green interface.

## 4. Software Components

| Component | Responsibility |
|---|---|
| Nmap | Network device discovery |
| Suricata IDS | Intrusion detection |
| Suricata Bridge | EVE JSON event processing |
| Python Monitor | Device discovery and monitoring |
| Traffic Monitor | Network interface traffic measurement |
| SQLite | Event and inventory storage |
| Eclipse Mosquitto | MQTT message broker |
| systemd | Service lifecycle management |
| M5Stack Firmware | Hardware dashboard |

## 5. Network Discovery Flow

Nmap periodically scans the configured home network.

The monitoring application processes the scan results and maintains the device inventory.

The inventory information is published to MQTT.

```text
Home Network
     |
     v
   Nmap
     |
     v
Python Monitoring Service
     |
     v
Device Inventory
     |
     v
MQTT: matrix/inventory
     |
     v
M5Stack Core2
```

The configured scan interval is 300 seconds.

## 6. Intrusion Detection Flow

Suricata analyzes traffic visible to the Raspberry Pi network interface.

Detected events are written to the EVE JSON log.

The Suricata bridge reads the event log, processes relevant records, and stores event information in SQLite.

The bridge also publishes event information through MQTT for dashboard display.

```text
Network Traffic
      |
      v
Suricata IDS
      |
      v
EVE JSON Log
      |
      v
Suricata Bridge
      |
      +------------------+
      |                  |
      v                  v
    SQLite             MQTT
                         |
                         v
                   M5Stack Core2
```

Security event records include information such as:

- Event source
- Signature ID (SID)
- Source IP address
- Destination IP address
- Destination port
- Protocol
- Signature description

The dashboard displays recent threat history, with a configured history limit of 10 entries.

## 7. Traffic Monitoring Flow

The traffic monitoring service reads network interface statistics from the Linux system.

It calculates received and transmitted traffic rates and maintains cumulative traffic totals.

The resulting statistics are published through MQTT.

```text
Linux Network Interface
          |
          v
  Interface Statistics
          |
          v
  Traffic Monitoring
          |
          v
   MQTT: matrix/traffic
          |
          v
    M5Stack Core2
```

The monitoring interval is approximately 2 seconds.

## 8. MQTT Communication Architecture

Eclipse Mosquitto provides message distribution between the Raspberry Pi services and the M5Stack Core2.

The following topics are used:

| MQTT Topic | Function |
|---|---|
| `matrix/status` | System status |
| `matrix/events` | Network and security events |
| `matrix/inventory` | Device inventory |
| `matrix/requests/threats` | Threat history requests |
| `matrix/threats/history` | Threat history responses |
| `matrix/traffic` | Network traffic statistics |

MQTT communication uses authenticated access.

The Raspberry Pi hosts the broker, while the M5Stack Core2 connects as an MQTT client over Wi-Fi.

## 9. Database Architecture

SQLite provides local persistent storage.

The database stores device inventory and security event information.

The schema is maintained in:

```text
database/matrix_schema.sql
```

The database is located on the Raspberry Pi at:

```text
/opt/matrix-home-soc/data/matrix.db
```

The database allows monitoring information to be retained locally without requiring an external database server.

## 10. Service Architecture

The system uses systemd to manage background processes.

| Service | Function |
|---|---|
| `matrix-home-soc` | Network discovery and device monitoring |
| `matrix-suricata-bridge` | Suricata event processing |
| `matrix-traffic` | Network traffic monitoring |
| `mosquitto` | MQTT message broker |
| `suricata` | Intrusion detection engine |

The MATRIX HOME SOC application services are configured for automatic startup and service recovery.

## 11. Resource Considerations

The system is designed to operate on a Raspberry Pi 3 with limited memory and processing resources.

Resource management includes:

- Python virtual environment isolation
- Lightweight SQLite storage
- Dedicated systemd services
- Suricata memory tuning
- Swap space configuration

These measures help maintain stable operation on resource-constrained hardware.

## 12. Monitoring Scope and Limitations

The Raspberry Pi connects to the home network through Wi-Fi.

Suricata can analyze traffic visible to its configured network interface.

However, ordinary Wi-Fi connectivity does not provide complete visibility into all unicast communications between other devices on the home LAN.

For full LAN-wide traffic monitoring, additional network infrastructure is required, such as:

- Switch port mirroring
- A network TAP
- An appropriately positioned network sensor

These capabilities are outside the scope of the current implementation.

## 13. Design Principles

MATRIX HOME SOC follows these design principles:

- Use open-source technologies.
- Keep the architecture lightweight.
- Separate monitoring, event processing, and visualization.
- Store relevant security events locally.
- Use MQTT for communication between components.
- Provide a dedicated hardware monitoring interface.
- Maintain automatic service startup and recovery.

## 14. Repository

The complete project source code, firmware, configuration examples, database schema, and documentation are maintained in:

https://github.com/ryoimaoka/matrix-home-soc
