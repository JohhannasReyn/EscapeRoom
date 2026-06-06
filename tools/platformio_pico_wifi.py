Import("env")

from pathlib import Path


def parse_env_file(path):
    values = {}

    if not path.exists():
        raise RuntimeError(f"Missing shared Pico WiFi config: {path}")

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()

        if not line or line.startswith("#"):
            continue

        if "=" not in line:
            continue

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip('"').strip("'")
        values[key] = value

    return values


project_dir = Path(env.subst("$PROJECT_DIR")).resolve()
repo_root = project_dir.parent
config = parse_env_file(repo_root / "pico-wifi.env")

required = ["WIFI_SSID", "WIFI_PASS", "MQTT_BROKER", "MQTT_BROKER_PORT"]
missing = [key for key in required if not config.get(key)]

if missing:
    raise RuntimeError(f"Missing required Pico WiFi config values: {', '.join(missing)}")

defines = [
    ("WIFI_SSID", env.StringifyMacro(config["WIFI_SSID"])),
    ("WIFI_PASS", env.StringifyMacro(config["WIFI_PASS"])),
    ("MQTT_BROKER", env.StringifyMacro(config["MQTT_BROKER"])),
    ("MQTT_BROKER_PORT", int(config["MQTT_BROKER_PORT"])),
]

fallback = config.get("MQTT_BROKER_FALLBACK", "")
if fallback:
    defines.append(("MQTT_BROKER_FALLBACK", env.StringifyMacro(fallback)))

env.Append(CPPDEFINES=defines)
