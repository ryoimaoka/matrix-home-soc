
#!/usr/bin/env python3

import json
import sqlite3
import subprocess
import time
import xml.etree.ElementTree as ET
from datetime import datetime
from pathlib import Path

import paho.mqtt.client as mqtt


BASE = Path("/opt/matrix-home-soc")
CONFIG_FILE = BASE / "config/monitor.json"
DB_FILE = BASE / "data/matrix.db"

OFFLINE_THRESHOLD = 3

with open(CONFIG_FILE, "r", encoding="utf-8") as f:
    CONFIG = json.load(f)

NETWORK = CONFIG["network"]
SCAN_INTERVAL = int(CONFIG["scan_interval"])

# Optional initial device inventory.
# Add environment-specific devices locally if required.
# Never commit personal MAC addresses or IP addresses.

SEED_DEVICES = []


def now():
    return datetime.now().astimezone().isoformat(timespec="seconds")


def log(message):
    print(f"[{now()}] {message}", flush=True)


def connect_database():
    db = sqlite3.connect(DB_FILE)

    db.execute("""
        CREATE TABLE IF NOT EXISTS devices (
            mac TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            ip TEXT,
            device_type TEXT,
            status TEXT NOT NULL DEFAULT 'OFFLINE',
            first_seen TEXT,
            last_seen TEXT,
            source TEXT NOT NULL DEFAULT 'DHCP'
        )
    """)

    db.execute("""
        CREATE TABLE IF NOT EXISTS events (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            level TEXT NOT NULL,
            message TEXT NOT NULL,
            mac TEXT,
            ip TEXT
        )
    """)

    columns = {
        row[1] for row in db.execute(
            "PRAGMA table_info(devices)"
        ).fetchall()
    }

    if "consecutive_misses" not in columns:
        db.execute("""
            ALTER TABLE devices
            ADD COLUMN consecutive_misses INTEGER NOT NULL DEFAULT 0
        """)
        log("Database migration: consecutive_misses added")

    db.commit()
    return db


def seed_database(db):
    for name, mac, ip, device_type in SEED_DEVICES:
        db.execute("""
            INSERT OR IGNORE INTO devices
            (mac, name, ip, device_type, status, source)
            VALUES (?, ?, ?, ?, 'OFFLINE', 'DHCP')
        """, (mac, name, ip, device_type))

    db.commit()
    log(f"Device inventory initialized: {len(SEED_DEVICES)} devices")


def save_event(db, level, message, mac=None, ip=None):
    timestamp = now()

    db.execute("""
        INSERT INTO events (timestamp, level, message, mac, ip)
        VALUES (?, ?, ?, ?, ?)
    """, (timestamp, level, message, mac, ip))

    db.commit()
    log(f"{level}: {message}")

    if mqtt_client.is_connected():
        mqtt_client.publish(
            "matrix/events",
            f"{level}|{message}",
            qos=1
        )


def scan_network(db):
    log(f"Starting network scan: {NETWORK}")

    result = subprocess.run(
        ["nmap", "-sn", "-n", "-oX", "-", NETWORK],
        capture_output=True,
        text=True,
        timeout=120
    )

    if result.returncode != 0:
        log(f"Nmap error: {result.stderr.strip()}")
        return

    root = ET.fromstring(result.stdout)
    observed = {}

    for host in root.findall("host"):
        status = host.find("status")

        if status is None or status.get("state") != "up":
            continue

        ip = None
        mac = None

        for address in host.findall("address"):
            if address.get("addrtype") == "ipv4":
                ip = address.get("addr")
            elif address.get("addrtype") == "mac":
                mac = address.get("addr", "").upper()

        if ip:
            if mac:
                key = mac
            else:
                existing = db.execute(
                    "SELECT mac FROM devices WHERE ip = ?",
                    (ip,)
                ).fetchone()

                key = existing[0] if existing else f"IP:{ip}"

            observed[key] = ip

    timestamp = now()

    # Update responding devices to ONLINE.
    for key, ip in observed.items():

        row = db.execute("""
            SELECT mac, name, status, last_seen
            FROM devices
            WHERE mac = ?
        """, (key,)).fetchone()

        if row is None:
            db.execute("""
                INSERT INTO devices
                (mac, name, ip, device_type, status,
                 first_seen, last_seen, source, consecutive_misses)
                VALUES (?, 'Unknown', ?, 'Unknown', 'ONLINE',
                        ?, ?, 'NMAP', 0)
            """, (key, ip, timestamp, timestamp))

            save_event(
                db, "WARNING", f"NEW DEVICE {ip}", key, ip
            )

        else:
            old_status = row[2]
            previous_last_seen = row[3]

            db.execute("""
                UPDATE devices
                SET ip = ?,
                    status = 'ONLINE',
                    last_seen = ?,
                    consecutive_misses = 0
                WHERE mac = ?
            """, (ip, timestamp, key))

            if old_status == "OFFLINE" and previous_last_seen:
                save_event(
                    db, "INFO", f"DEVICE ONLINE {ip}", key, ip
                )

    # Update consecutive missed scan counts.
    rows = db.execute("""
        SELECT mac, ip, status, consecutive_misses
        FROM devices
    """).fetchall()

    for mac, ip, old_status, misses in rows:

        if mac in observed:
            continue

        if old_status == "OFFLINE":
            continue

        misses += 1

        if misses >= OFFLINE_THRESHOLD:
            db.execute("""
                UPDATE devices
                SET status = 'OFFLINE',
                    consecutive_misses = ?
                WHERE mac = ?
            """, (misses, mac))

            save_event(
                db,
                "INFO",
                f"DEVICE OFFLINE {ip} "
                f"({OFFLINE_THRESHOLD} consecutive misses)",
                mac,
                ip
            )

        else:
            db.execute("""
                UPDATE devices
                SET consecutive_misses = ?
                WHERE mac = ?
            """, (misses, mac))

            log(
                f"NO RESPONSE {ip}: "
                f"{misses}/{OFFLINE_THRESHOLD} "
                f"(keeping {old_status})"
            )

    db.commit()

    # Calculate inventory statistics from database state.
    online_count = db.execute("""
        SELECT COUNT(*) FROM devices
        WHERE status = 'ONLINE'
    """).fetchone()[0]

    offline_count = db.execute("""
        SELECT COUNT(*) FROM devices
        WHERE status = 'OFFLINE'
    """).fetchone()[0]

    total_count = db.execute("""
        SELECT COUNT(*) FROM devices
    """).fetchone()[0]

    # Count currently ONLINE devices discovered by Nmap.
    unknown_count = db.execute("""
        SELECT COUNT(*) FROM devices
        WHERE source = 'NMAP'
          AND status = 'ONLINE'
    """).fetchone()[0]

    inventory = {
        "online": online_count,
        "offline": offline_count,
        "registered": total_count,
        "unknown": unknown_count,
        "timestamp": timestamp
    }

    if mqtt_client.is_connected():
        mqtt_client.publish(
            "matrix/inventory",
            json.dumps(inventory),
            qos=1
        )

    log(
        f"Scan complete: ONLINE={online_count}, "
        f"OFFLINE={offline_count}, "
        f"REGISTERED={total_count}, "
        f"UNKNOWN={unknown_count}"
    )


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        log("MQTT connected")
        client.publish(
            "matrix/status",
            "RASPBERRY PI ONLINE",
            qos=1
        )
    else:
        log(f"MQTT connection failed: {reason_code}")


mqtt_client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="matrix-home-soc-monitor"
)

mqtt_client.username_pw_set(
    CONFIG["mqtt_user"],
    CONFIG["mqtt_password"]
)

mqtt_client.on_connect = on_connect


def main():
    db = connect_database()
    seed_database(db)

    try:
        mqtt_client.connect(
            CONFIG["mqtt_host"],
            int(CONFIG["mqtt_port"]),
            keepalive=60
        )
        mqtt_client.loop_start()

    except Exception as e:
        log(f"MQTT initial connection error: {e}")

    try:
        while True:
            try:
                scan_network(db)
            except Exception as e:
                log(f"Scan exception: {e}")

            time.sleep(SCAN_INTERVAL)

    except KeyboardInterrupt:
        log("Monitor stopped")

    finally:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
        db.close()


if __name__ == "__main__":
    main()
