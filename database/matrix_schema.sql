
-- MATRIX HOME SOC database schema

CREATE TABLE IF NOT EXISTS devices (
    mac TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    ip TEXT,
    device_type TEXT,
    status TEXT NOT NULL DEFAULT 'OFFLINE',
    first_seen TEXT,
    last_seen TEXT,
    source TEXT NOT NULL DEFAULT 'DHCP',
    consecutive_misses INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT NOT NULL,
    level TEXT NOT NULL,
    message TEXT NOT NULL,
    mac TEXT,
    ip TEXT,
    source TEXT NOT NULL DEFAULT 'MATRIX',
    sid INTEGER,
    src_ip TEXT,
    dest_ip TEXT,
    dest_port INTEGER,
    proto TEXT,
    signature TEXT
);
