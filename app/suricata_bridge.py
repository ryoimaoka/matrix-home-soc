
#!/usr/bin/env python3

import os
import json
import time
import sqlite3
import logging
from pathlib import Path
from datetime import datetime, timezone

import paho.mqtt.client as mqtt


# ==========================================
# MATRIX HOME SOC - Suricata MQTT Bridge
# SQLite + MQTT + Deduplication + Threat History
# ==========================================

BASE = Path("/opt/matrix-home-soc")
CONFIG_FILE = BASE / "config/monitor.json"

EVE_FILE = "/var/log/suricata/eve.json"
DB_FILE = str(BASE / "data/matrix.db")

# Load MQTT configuration from the local configuration file.
with open(CONFIG_FILE, "r", encoding="utf-8") as f:
    CONFIG = json.load(f)

MQTT_HOST = CONFIG["mqtt_host"]
MQTT_PORT = int(CONFIG["mqtt_port"])
MQTT_USER = CONFIG["mqtt_user"]
MQTT_PASSWORD = CONFIG["mqtt_password"]

MQTT_TOPIC = "matrix/events"

# Threat History
MQTT_REQUEST_TOPIC = "matrix/requests/threats"
MQTT_HISTORY_TOPIC = "matrix/threats/history"
HISTORY_LIMIT = 10

IGNORE_SIDS = {
    2210020,
    2210029,
    2210045,
}

DEDUP_SECONDS = 30

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [SURICATA-BRIDGE] %(levelname)s: %(message)s"
)

logger = logging.getLogger("suricata-bridge")

client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="MATRIX-SURICATA-BRIDGE"
)

client.username_pw_set(MQTT_USER, MQTT_PASSWORD)

recent_events = {}


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        logger.info("MQTT connected")

        client.subscribe(MQTT_REQUEST_TOPIC, qos=0)
        logger.info("Subscribed: %s", MQTT_REQUEST_TOPIC)
    else:
        logger.error("MQTT connection failed: %s", reason_code)


def on_disconnect(client, userdata, disconnect_flags, reason_code, properties):
    logger.warning("MQTT disconnected: %s", reason_code)


def on_message(client, userdata, msg):
    if msg.topic != MQTT_REQUEST_TOPIC:
        return

    try:
        request = msg.payload.decode("utf-8").strip().upper()

        if request == "LATEST":
            logger.info("Threat history request received")
            publish_threat_history()

    except Exception:
        logger.exception("History request processing failed")


client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_message = on_message


def severity_to_level(severity):
    try:
        severity = int(severity)
    except (TypeError, ValueError):
        return "INFO"

    if severity <= 1:
        return "ALERT"
    elif severity == 2:
        return "WARNING"
    else:
        return "INFO"


def is_duplicate(event):
    alert = event.get("alert", {})

    key = (
        alert.get("signature_id"),
        event.get("src_ip"),
        event.get("dest_ip"),
        event.get("dest_port"),
        event.get("proto")
    )

    current = time.monotonic()
    previous = recent_events.get(key)

    expired = [
        k for k, timestamp in recent_events.items()
        if current - timestamp > DEDUP_SECONDS
    ]

    for k in expired:
        del recent_events[k]

    if previous is not None and current - previous < DEDUP_SECONDS:
        return True

    recent_events[key] = current
    return False


def save_to_database(event, level, sid):
    alert = event.get("alert", {})

    src_ip = event.get("src_ip")
    dest_ip = event.get("dest_ip")
    dest_port = event.get("dest_port")
    proto = event.get("proto")
    signature = alert.get("signature", "Unknown signature")

    timestamp = event.get("timestamp")
    if not timestamp:
        timestamp = datetime.now(timezone.utc).isoformat()

    try:
        sid_value = int(sid)
    except (TypeError, ValueError):
        sid_value = None

    try:
        port_value = int(dest_port) if dest_port is not None else None
    except (TypeError, ValueError):
        port_value = None

    message = f"SID{sid} {signature}"

    with sqlite3.connect(DB_FILE, timeout=10) as db:
        db.execute("""
            INSERT INTO events (
                timestamp,
                level,
                message,
                ip,
                source,
                sid,
                src_ip,
                dest_ip,
                dest_port,
                proto,
                signature
            )
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            timestamp,
            level,
            message,
            src_ip,
            "SURICATA",
            sid_value,
            src_ip,
            dest_ip,
            port_value,
            proto,
            signature
        ))

    logger.info(
        "DB saved: SID%s %s -> %s:%s",
        sid, src_ip, dest_ip, dest_port
    )


def publish_alert(event):
    alert = event.get("alert", {})
    sid = alert.get("signature_id", 0)

    try:
        sid_number = int(sid)
    except (TypeError, ValueError):
        sid_number = 0

    if sid_number in IGNORE_SIDS:
        return

    if is_duplicate(event):
        return

    severity = alert.get("severity", 3)
    level = severity_to_level(severity)

    proto = event.get("proto", "IP")
    dest_port = event.get("dest_port", "")

    # Save the event to SQLite.
    try:
        save_to_database(event, level, sid)
    except Exception:
        logger.exception("Database save failed for SID%s", sid)

    # Send real-time MQTT notification to M5Stack.
    detail = f"SID{sid} {proto}{dest_port}"
    payload = f"{level}|{detail}"

    if not client.is_connected():
        logger.warning(
            "MQTT offline. DB saved; notification skipped: %s",
            payload
        )
        return

    result = client.publish(
        MQTT_TOPIC,
        payload=payload,
        qos=0,
        retain=False
    )

    if result.rc == mqtt.MQTT_ERR_SUCCESS:
        logger.info("Published: %s", payload)
    else:
        logger.error("MQTT publish failed: %s", result.rc)


def publish_threat_history():
    """Retrieve and publish the latest Suricata threat events from SQLite."""

    try:
        with sqlite3.connect(DB_FILE, timeout=10) as db:
            db.row_factory = sqlite3.Row

            rows = db.execute("""
                SELECT
                    id,
                    timestamp,
                    level,
                    sid,
                    src_ip,
                    dest_ip,
                    dest_port,
                    proto,
                    signature
                FROM events
                WHERE source = 'SURICATA'
                ORDER BY id DESC
                LIMIT ?
            """, (HISTORY_LIMIT,)).fetchall()

        count = 0

        # Publish events individually.
        for row in rows:
            item = {
                "type": "event",
                "id": row["id"],
                "timestamp": row["timestamp"],
                "level": row["level"],
                "sid": row["sid"],
                "src_ip": row["src_ip"] or "",
                "dest_ip": row["dest_ip"] or "",
                "dest_port": row["dest_port"] or 0,
                "proto": row["proto"] or "",
                "signature": (row["signature"] or "")[:80]
            }

            payload = json.dumps(
                item,
                separators=(",", ":"),
                ensure_ascii=True
            )

            result = client.publish(
                MQTT_HISTORY_TOPIC,
                payload=payload,
                qos=0,
                retain=False
            )

            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                logger.error("History publish failed: %s", result.rc)
                return

            count += 1

        # Publish history completion notification.
        end_payload = json.dumps({
            "type": "end",
            "count": count
        }, separators=(",", ":"))

        client.publish(
            MQTT_HISTORY_TOPIC,
            payload=end_payload,
            qos=0,
            retain=False
        )

        logger.info("Threat history published: %s events", count)

    except Exception:
        logger.exception("Threat history database query failed")


def monitor_eve():
    logger.info("Monitoring: %s", EVE_FILE)

    file_handle = None
    current_inode = None

    while True:
        try:
            if not os.path.exists(EVE_FILE):
                time.sleep(2)
                continue

            stat = os.stat(EVE_FILE)

            if file_handle is None:
                file_handle = open(EVE_FILE, "r", encoding="utf-8")
                file_handle.seek(0, os.SEEK_END)
                current_inode = stat.st_ino
                logger.info("Started at end of existing EVE log")
                continue

            # Handle log rotation.
            if stat.st_ino != current_inode:
                logger.info("EVE log rotated")
                file_handle.close()
                file_handle = open(EVE_FILE, "r", encoding="utf-8")
                current_inode = stat.st_ino

            # Handle file truncation.
            elif stat.st_size < file_handle.tell():
                logger.info("EVE log truncated")
                file_handle.seek(0)

            line = file_handle.readline()

            if not line:
                time.sleep(0.2)
                continue

            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue

            if event.get("event_type") != "alert":
                continue

            publish_alert(event)

        except Exception:
            logger.exception("Monitor error")
            time.sleep(2)


def main():
    logger.info("Starting MATRIX Suricata Bridge")

    client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
    client.loop_start()

    time.sleep(1)

    monitor_eve()


if __name__ == "__main__":
    main()
