# MATRIX HOME SOC Security Guide

## 1. Purpose

This document describes the security considerations and operational precautions for MATRIX HOME SOC.

MATRIX HOME SOC is a self-hosted home network security monitoring system built with a Raspberry Pi 3 and an M5Stack Core2.

It provides network visibility, intrusion detection, event collection, and monitoring visualization.

It is intended for educational, experimental, and personal home network monitoring purposes.

## 2. Security Architecture

The system uses several security-related components:

| Component | Security Role |
|---|---|
| Suricata IDS | Network intrusion detection |
| Nmap | Network device discovery |
| Eclipse Mosquitto | Authenticated MQTT messaging |
| SQLite | Local event and inventory storage |
| systemd | Background service management |
| Raspberry Pi | Central monitoring server |
| M5Stack Core2 | Monitoring dashboard |

These components provide monitoring and detection capabilities. They do not constitute a complete network security or prevention solution.

## 3. Credential Management

Credentials must be configured locally and must not be committed to the public repository.

Sensitive configuration includes:

- Wi-Fi SSID and password
- MQTT username and password
- Local configuration files
- Private keys and tokens

### M5Stack Credentials

The firmware uses:

```text
m5stack/matrix_home_soc/secrets.example.h
```

Copy this template to `secrets.h` and enter the local credentials.

Only `secrets.example.h` should be committed to GitHub.

The local `secrets.h` file must remain excluded through `.gitignore`.

### Raspberry Pi Configuration

The local monitoring configuration is stored in:

```text
/opt/matrix-home-soc/config/monitor.json
```

Do not publish this file if it contains actual credentials or private network information.

Use the example configuration in the repository as a template.

## 4. MQTT Security

MATRIX HOME SOC uses Eclipse Mosquitto for communication between the Raspberry Pi services and the M5Stack Core2.

The MQTT broker is configured to require authentication.

Recommended precautions:

- Disable anonymous access.
- Use a strong, unique MQTT password.
- Restrict access to the MQTT broker to trusted devices.
- Do not expose MQTT port 1883 directly to the public Internet.
- Protect local MQTT credential files.

The example configuration uses port 1883. This is ordinary MQTT and does not provide transport encryption by itself.

For networks where MQTT traffic may be exposed to untrusted parties, consider TLS configuration.

## 5. Raspberry Pi Security

The Raspberry Pi is the central monitoring server and should be protected against unauthorized access.

Recommended precautions:

- Use strong authentication for SSH.
- Keep Ubuntu and installed packages updated.
- Restrict access to management services.
- Avoid exposing unnecessary services to the Internet.
- Protect local configuration and database files.
- Review system logs and service status periodically.

## 6. Network Monitoring Limitations

The Raspberry Pi operates as a Wi-Fi-connected monitoring device.

The current implementation analyzes traffic visible to its own network interface.

It does not provide complete visibility into all unicast communications between other devices on the home LAN.

Full LAN-wide monitoring requires additional network infrastructure, such as switch port mirroring, a network TAP, or an appropriately positioned network sensor.

This limitation should be considered when interpreting monitoring results.

## 7. Intrusion Detection Limitations

Suricata provides intrusion detection based on traffic visible to the configured interface and the detection rules in use.

Detection results depend on:

- Network visibility
- Detection rule coverage
- Rule configuration
- Protocol and traffic characteristics
- Available system resources

An absence of alerts does not prove that a network is free from malicious activity.

Suricata IDS should be treated as a monitoring and detection component, not as a replacement for a firewall or endpoint protection.

## 8. Database and Log Protection

MATRIX HOME SOC stores monitoring information locally.

Relevant data includes:

- Device inventory
- Security event records
- Source and destination IP addresses
- Protocol and port information
- Suricata event signatures

These records may reveal information about the home network.

Protect the database and log files from unauthorized access.

Do not publish actual database files, logs, packet captures, or household device information.

## 9. GitHub Publication Safety

Before publishing or updating repository contents, check for:

- Actual passwords and credentials
- Private IP addresses
- MAC addresses
- Device names and identifiers
- Personal information
- Private keys and tokens
- Local database and log files

Use example configuration files with placeholder values.

If credentials are accidentally published, changing or deleting the visible file alone is not sufficient. Rotate the exposed credentials and review the repository history.

## 10. Operational Security Checklist

- [ ] MQTT anonymous access is disabled.
- [ ] MQTT uses a strong password.
- [ ] Actual credentials are excluded from GitHub.
- [ ] `secrets.h` is excluded through `.gitignore`.
- [ ] Local configuration files are protected.
- [ ] SSH access is protected.
- [ ] Unnecessary external access is avoided.
- [ ] Operating system and detection rules are maintained.
- [ ] Database and log files are not publicly exposed.
- [ ] Monitoring limitations are understood.

## 11. Disclaimer

MATRIX HOME SOC is a personal home network security monitoring project.

It is not intended to guarantee the detection of every attack or provide complete visibility into all network communications.

The system should be used as one part of a broader security approach.

Users are responsible for configuring and operating the system securely in their own environments.

## 12. Repository

Project source code, configuration examples, firmware, and documentation:

https://github.com/ryoimaoka/matrix-home-soc
