# MATRIX HOME SOC Installation Guide

This document describes the installation and configuration of MATRIX HOME SOC on a Raspberry Pi 3.

The system integrates Nmap, Suricata IDS, Eclipse Mosquitto, SQLite, Python monitoring services, and an M5Stack Core2 dashboard.

## 1. System Requirements

### Hardware

- Raspberry Pi 3 Model B
- M5Stack Core2
- Wi-Fi network
- PC for initial configuration and firmware upload

### Software

- Ubuntu Server 22.04 LTS
- Python 3
- Nmap
- Suricata IDS
- Eclipse Mosquitto
- SQLite
- Arduino IDE

## 2. Operating System Preparation

Install Ubuntu Server 22.04 LTS on the Raspberry Pi.

Connect the Raspberry Pi to the local network and access it through SSH.

Update the package index and installed packages:

```bash
sudo apt update
sudo apt upgrade -y
```

Install the required system packages:

```bash
sudo apt install -y \
  python3 \
  python3-venv \
  python3-pip \
  nmap \
  sqlite3 \
  mosquitto \
  mosquitto-clients \
  suricata
```

## 3. Project Directory

Create the project directory structure:

```bash
sudo mkdir -p /opt/matrix-home-soc/{app,data,logs,config}
```

The project uses the following directory layout:

```text
/opt/matrix-home-soc/
├── app/
│   ├── matrix_monitor.py
│   ├── matrix_traffic.py
│   └── suricata_bridge.py
├── config/
│   └── monitor.json
├── data/
├── logs/
└── venv/
```

Copy the application files from the repository into the `app` directory.

## 4. Python Environment

Create a Python virtual environment:

```bash
sudo python3 -m venv /opt/matrix-home-soc/venv
```

Install the required Python dependency:

```bash
sudo /opt/matrix-home-soc/venv/bin/pip install paho-mqtt==2.2.0
```

## 5. Configuration

Create the local configuration file:

```bash
sudo nano /opt/matrix-home-soc/config/monitor.json
```

Use the following example and adjust the values for your network:

```json
{
  "network": "192.168.1.0/24",
  "scan_interval": 300,
  "mqtt_host": "127.0.0.1",
  "mqtt_port": 1883,
  "mqtt_user": "matrix",
  "mqtt_password": "CHANGE_ME",
  "interface": "wlan0"
}
```

Replace the example network, MQTT credentials, and network interface with the appropriate values for your environment.

Do not publish the actual `monitor.json` file or credentials.

## 6. MQTT Broker

MATRIX HOME SOC uses Eclipse Mosquitto as its MQTT message broker.

Configure Mosquitto with authenticated access.

Create a password file:

```bash
sudo mosquitto_passwd -c /etc/mosquitto/passwd matrix
```

Configure the Mosquitto listener to use the password file and disable anonymous access.

Example configuration:

```text
listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Restart Mosquitto:

```bash
sudo systemctl restart mosquitto
sudo systemctl enable mosquitto
```

The MQTT credentials must match the application and M5Stack configuration.

## 7. Suricata IDS

Suricata is used for network intrusion detection.

Configure Suricata for the network interface used by the Raspberry Pi.

The example configuration is available at:

```text
config/suricata.yaml.example
```

Review the following settings for your environment:

- HOME_NET
- Network interface
- AF_PACKET configuration
- EVE JSON logging
- Rule configuration

Use `suricata-update` to obtain and update detection rules.

Suricata writes events to its EVE JSON log, which is consumed by the MATRIX HOME SOC Suricata bridge.

The example configuration is provided as a reference and may require adjustments for the installed Suricata version and network environment.

## 8. Database Initialization

MATRIX HOME SOC uses SQLite to store device inventory and security events.

The database schema is provided at:

```text
database/matrix_schema.sql
```

Initialize the database:

```bash
sqlite3 /opt/matrix-home-soc/data/matrix.db \
  < database/matrix_schema.sql
```

## 9. systemd Services

MATRIX HOME SOC uses systemd to manage its background services.

The repository includes the following service definitions:

| Service | Description |
|---|---|
| matrix-home-soc | Network discovery and monitoring |
| matrix-suricata-bridge | Suricata event processing |
| matrix-traffic | Network traffic monitoring |

The service files are available in:

```text
systemd/
```

Copy the service files into `/etc/systemd/system/` and review their paths and execution settings for your installation.

Reload systemd configuration:

```bash
sudo systemctl daemon-reload
```

Enable and start the services:

```bash
sudo systemctl enable --now matrix-home-soc
sudo systemctl enable --now matrix-suricata-bridge
sudo systemctl enable --now matrix-traffic
```

Check service status:

```bash
sudo systemctl status matrix-home-soc
sudo systemctl status matrix-suricata-bridge
sudo systemctl status matrix-traffic
```

## 10. M5Stack Core2

The M5Stack Core2 acts as the dedicated monitoring dashboard.

Firmware source code:

```text
m5stack/matrix_home_soc/matrix_home_soc.ino
```

Copy `secrets.example.h` to `secrets.h` and configure the local Wi-Fi and MQTT credentials.

Install the required Arduino libraries:

- M5Unified
- PubSubClient

Compile and upload the firmware using Arduino IDE with the M5Stack Core2 board selected.

Refer to the M5Stack Core2 Setup section in the main README for additional details.

## 11. MQTT Topics

The system uses the following MQTT topics:

| Topic | Purpose |
|---|---|
| `matrix/status` | System status |
| `matrix/events` | Network events |
| `matrix/inventory` | Device inventory |
| `matrix/requests/threats` | Threat history requests |
| `matrix/threats/history` | Threat history |
| `matrix/traffic` | Network traffic statistics |

## 12. Monitoring Scope and Limitations

The Raspberry Pi operates as a Wi-Fi-connected monitoring device.

The current implementation monitors traffic visible to its own network interface. It does not capture all unicast communications between other devices on the home LAN.

Full LAN-wide traffic visibility requires additional network infrastructure, such as switch port mirroring or an appropriately positioned network sensor.

## 13. Security Considerations

- Use authenticated MQTT access.
- Do not publish passwords or private configuration files.
- Protect the Raspberry Pi and its SSH access.
- Restrict access to MQTT and management services.
- Keep the operating system and detection rules updated.
- Avoid publishing private IP addresses, MAC addresses, and household device identifiers.

## 14. Project Repository

Source code, configuration examples, database schemas, service definitions, and M5Stack firmware are available in this repository.

Repository:

https://github.com/ryoimaoka/matrix-home-soc
