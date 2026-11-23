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

Documentation, Raspberry Pi source code, M5Stack firmware, configuration examples, and database schemas will be organized in this repository.

## Documentation

Installation instructions, system architecture, configuration details, operation procedures, and recovery procedures will be added as the project documentation is completed.

## Security Notice

This project is intended for educational, experimental, and personal home network monitoring purposes.

Before publishing configuration files or screenshots, remove credentials, private IP addresses, MAC addresses, device identifiers, and other sensitive information.

## License

License information will be added after the project contents and publication scope have been reviewed.
