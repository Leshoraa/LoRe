#!/usr/bin/env bash
# ==============================================================================
# LoRe Firmware Flash Script
# Target: ESP32-S3 SuperMini on /dev/ttyACM0
# ==============================================================================

set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PORT="${1:-/dev/ttyACM0}"
FQBN="esp32:esp32:esp32s3:PSRAM=disabled,PartitionScheme=min_spiffs"

echo "[FLASH] Uploading LoRe firmware to ${PORT} (${FQBN})..."
arduino-cli upload -p "${PORT}" -b "${FQBN}" "${SKETCH_DIR}/LoRe.ino"

echo "[FLASH] Upload complete."
