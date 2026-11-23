
#!/usr/bin/env python3

import json
import time
from pathlib import Path

import paho.mqtt.client as mqtt

BASE = Path("/opt/matrix-home-soc")
CONFIG_FILE = BASE / "config/monitor.json"
INTERVAL = 2

with open(CONFIG_FILE, "r", encoding="utf-8") as f:
    CONFIG = json.load(f)

# Use the configured network interface.
# Defaults to wlan0 when not specified.
INTERFACE = CONFIG.get("interface", "wlan0")

SYSFS = Path("/sys/class/net") / INTERFACE / "statistics"


def read_bytes(direction):
    return int((SYSFS / f"{direction}_bytes").read_text().strip())


client = mqtt.Client(
    callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
    client_id="matrix-home-soc-traffic"
)

client.username_pw_set(
    CONFIG["mqtt_user"],
    CONFIG["mqtt_password"]
)


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print("Traffic monitor MQTT connected", flush=True)
    else:
        print(f"MQTT connection failed: {reason_code}", flush=True)


client.on_connect = on_connect


def main():
    client.connect(
        CONFIG["mqtt_host"],
        int(CONFIG["mqtt_port"]),
        keepalive=60
    )
    client.loop_start()

    rx_prev = read_bytes("rx")
    tx_prev = read_bytes("tx")
    rx_total = rx_prev
    tx_total = tx_prev
    prev_time = time.monotonic()

    print(f"Monitoring interface: {INTERFACE}", flush=True)

    try:
        while True:
            time.sleep(INTERVAL)

            current_time = time.monotonic()
            elapsed = current_time - prev_time

            rx_now = read_bytes("rx")
            tx_now = read_bytes("tx")

            rx_rate = max(0, rx_now - rx_prev) * 8 / elapsed / 1000000
            tx_rate = max(0, tx_now - tx_prev) * 8 / elapsed / 1000000

            rx_total = rx_now
            tx_total = tx_now

            payload = {
                "interface": INTERFACE,
                "rx_mbps": round(rx_rate, 3),
                "tx_mbps": round(tx_rate, 3),
                "rx_total_mb": round(rx_total / 1048576, 2),
                "tx_total_mb": round(tx_total / 1048576, 2),
                "timestamp": time.time()
            }

            if client.is_connected():
                client.publish(
                    "matrix/traffic",
                    json.dumps(payload),
                    qos=0
                )

            rx_prev = rx_now
            tx_prev = tx_now
            prev_time = current_time

    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()
