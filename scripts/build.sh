#!/usr/bin/env bash
# ==============================================================================
# LoRe Firmware Compilation Script
# Target: ESP32-S3 SuperMini (Dual-Core Xtensa LX7 @ 240 MHz)
# Configuration: PSRAM Disabled, 4MB Flash, Minimal SPIFFS (1.9MB App / 128KB SPIFFS)
# ==============================================================================

set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FQBN="esp32:esp32:esp32s3:PSRAM=disabled,PartitionScheme=min_spiffs"

echo "[BUILD] Compiling LoRe firmware for ${FQBN}..."
arduino-cli compile --clean -b "${FQBN}" "${SKETCH_DIR}/LoRe.ino"

echo "[BUILD] LoRe compilation successful."
