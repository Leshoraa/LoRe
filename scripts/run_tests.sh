#!/usr/bin/env bash
# LoRe Host Unit Test Runner
# Compiles and executes algorithmic unit test suites on host Linux using g++.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
TEST_DIR="${ROOT_DIR}/tests/unit"
BIN_DIR="/tmp/lore_tests_bin"

mkdir -p "${BIN_DIR}"

echo "[TEST] Running LoRe Algorithmic Unit Test Suite"

PASSED=0
FAILED=0

run_test() {
    local test_name="$1"
    local src_file="${TEST_DIR}/${test_name}.cpp"
    local bin_file="${BIN_DIR}/${test_name}"
    local extra_srcs="${2:-}"

    echo -n "[RUN] ${test_name}... "
    if g++ -O2 -std=c++17 -Wall -Wextra -I"${ROOT_DIR}" "${src_file}" ${extra_srcs} -o "${bin_file}" 2>/tmp/compile_err.log; then
        if "${bin_file}" > /tmp/test_out.log 2>&1; then
            echo "PASS"
            PASSED=$((PASSED + 1))
        else
            echo "FAIL (execution)"
            cat /tmp/test_out.log
            FAILED=$((FAILED + 1))
        fi
    else
        echo "FAIL (compilation)"
        cat /tmp/compile_err.log
        FAILED=$((FAILED + 1))
    fi
}

run_test "test_minimum_jerk"
run_test "test_kalman_convergence" "${ROOT_DIR}/src/math/kalman_filter.cpp"
run_test "test_affective_langevin"
run_test "test_personality_circadian"
run_test "test_episodic_memory" "${ROOT_DIR}/src/ai/memory_engine.cpp"
run_test "test_brain_inference"
run_test "test_kinematics_feedforward"
run_test "test_ble_telemetry"
run_test "test_notification_parser"

echo "[TEST] Summary: ${PASSED} Passed, ${FAILED} Failed"

if [ ${FAILED} -ne 0 ]; then
    exit 1
fi
