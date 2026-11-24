# MATRIX HOME SOC

**A Raspberry Pi 3 and M5Stack Core2-based home network security monitoring system.**

MATRIX HOME SOC is a self-hosted home security monitoring project that integrates network device discovery, intrusion detection, event collection, traffic monitoring, and real-time visualization.

The system uses a Raspberry Pi 3 as its monitoring server and an M5Stack Core2 as a dedicated security dashboard.

## Project Overview

MATRIX HOME SOC combines several open-source technologies to provide visibility into network devices, security events, and network traffic.

### Key Features

- **Network Discovery:** Automatic device discovery and inventory management using Nmap.
- **Intrusion Detection:** Network threat detection using Suricata IDS.
- **Event Collection:** Security event processing and storage using SQLite.
- **MQTT Integration:** Communication between the Raspberry Pi and M5Stack Core2.
- **Traffic Monitoring:** Network interface traffic measurement and visualization.
- **Threat History:** Storage and display of recent security events.
- **Hardware Dashboard:** Eight-page real-time monitoring interface on M5Stack Core2.
- **Service Management:** Automatic startup and service recovery using systemd.

## System Architecture

| Component | Technology |
|---|---|
| Monitoring Server | Raspberry Pi 3 Model B |
| Operating System | Ubuntu Server 22.04 LTS |
| Network Discovery | Nmap |
| Intrusion Detection | Suricata IDS |
| Message Broker | Eclipse Mosquitto |
| Communication Protocol | MQTT |
| Event Database | SQLite |
| Backend | Python |
| Hardware Dashboard | M5Stack Core2 |
| Firmware | Arduino Framework |

## Dashboard

The M5Stack Core2 provides an eight-page monitoring interface.

| Page | Description |
|---|---|
| HOME | Overall system status |
| NETWORK | Network information |
| THREATS | Security event monitoring |
| DEVICES | Discovered network devices |
| TRAFFIC | Network traffic visualization |
| SYSTEM | Monitoring server status |
| POWER | Device power information |
| SETTINGS | Dashboard settings |

## Architecture and Monitoring Scope

The Raspberry Pi operates as a Wi-Fi-connected monitoring device.

The current implementation monitors traffic visible to its own network interface and does not provide complete visibility into all unicast communications between other devices on the home LAN.

Full LAN-wide monitoring would require additional network infrastructure, such as switch port mirroring or an appropriately positioned network sensor.

## Project Goals

- Build a practical home network security monitoring environment.
- Integrate open-source security technologies.
- Visualize security information on a standalone hardware dashboard.
- Maintain security event history.
- Provide reproducible installation and configuration documentation.

## Repository Structure

Documentation, Raspberry Pi source code, M5Stack firmware, configuration examples, and database schemas are organized in this repository.

## Documentation

Detailed documentation is available in the `docs/` directory.

| Document | Description |
|---|---|
| [Installation Guide](docs/INSTALLATION.md) | Installation and configuration procedures |
| [Operations Guide](docs/OPERATIONS.md) | Dashboard operation and system monitoring |
| [Architecture](docs/ARCHITECTURE.md) | System architecture and data flow |
| [Security Guide](docs/SECURITY.md) | Security configuration and operational considerations |

For an overview of the project and M5Stack Core2 setup, see this README.

## Security Notice

This project is intended for educational, experimental, and personal home network monitoring purposes.

Before publishing configuration files or screenshots, remove credentials, private IP addresses, MAC addresses, device identifiers, and other sensitive information.

## License

License information will be added after the project contents and publication scope have been reviewed.

## M5Stack Core2 Setup

### Hardware

- M5Stack Core2
- Raspberry Pi 3 running MATRIX HOME SOC

### Requirements

- Arduino IDE
- M5Stack board support
- M5Unified library
- PubSubClient library

### 1. Open the Firmware

Open the following sketch in Arduino IDE:

```text
m5stack/matrix_home_soc/matrix_home_soc.ino
```

### 2. Configure Credentials

Copy `secrets.example.h` to `secrets.h` in the same directory.

Edit `secrets.h` with your own Wi-Fi and MQTT settings.

```cpp
#define MATRIX_WIFI_SSID "YOUR_WIFI_SSID"
#define MATRIX_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define MATRIX_MQTT_SERVER "YOUR_RASPBERRY_PI_IP"
#define MATRIX_MQTT_PORT 1883
#define MATRIX_MQTT_USER "YOUR_MQTT_USER"
#define MATRIX_MQTT_PASSWORD "YOUR_MQTT_PASSWORD"
```

The MQTT settings must match the Mosquitto configuration on the Raspberry Pi.

**Important:** Never commit `secrets.h` to GitHub. Only `secrets.example.h` should be published.

### 3. Install Required Libraries

Install the following libraries using Arduino IDE Library Manager:

- M5Unified
- PubSubClient

### 4. Select Board and Upload

1. Connect M5Stack Core2 to your PC via USB.
2. Select the M5Stack Core2 board.
3. Select the corresponding COM port.
4. Open `matrix_home_soc.ino`.
5. Compile and upload the firmware.

### 5. MQTT Connection

After uploading, the M5Stack Core2 connects to the configured Wi-Fi network and communicates with the Raspberry Pi through MQTT.

The firmware uses the following MQTT topics:

| Topic | Purpose |
|---|---|
| `matrix/status` | System status |
| `matrix/events` | Network events |
| `matrix/inventory` | Device inventory |
| `matrix/requests/threats` | Threat history requests |
| `matrix/threats/history` | Threat history |
| `matrix/traffic` | Network traffic statistics |

### Display Pages

The M5Stack Core2 provides eight monitoring pages:

- HOME
- NETWORK
- THREATS
- SYSTEM
- DEVICES
- TRAFFIC
- POWER
- SETTINGS

The interface uses a Matrix-inspired black and neon-green HUD design with double-buffered rendering.

### Notes

The M5Stack communicates with the Raspberry Pi over Wi-Fi and MQTT.

The current Raspberry Pi Wi-Fi monitoring configuration does not capture all peer-to-peer traffic between other devices on the home LAN. Full LAN-wide traffic monitoring requires additional network infrastructure such as port mirroring.
