#!/usr/bin/env bash
set -euo pipefail

broker="${1:-}"
source_dir="${2:-}"
build_dir="${3:-}"
config_path="${4:-}"

require_env() {
  local name="$1"
  if [[ -z "${!name:-}" ]]; then
    echo "SKIP: missing ${name}"
    exit 0
  fi
}

case "${broker}" in
  alpaca)
    require_env ALPACA_API_KEY
    require_env ALPACA_API_SECRET
    require_env ALPACA_PAPER_BASE_URL
    require_env ALPACA_STREAM_URL
    ;;
  ib)
    require_env IB_HOST
    require_env IB_PORT
    require_env IB_CLIENT_ID
    ;;
  binance)
    require_env BINANCE_API_KEY
    require_env BINANCE_SECRET_KEY
    require_env BINANCE_BASE_URL
    require_env BINANCE_STREAM_URL
    ;;
  *)
    echo "Unknown broker: ${broker}" >&2
    exit 1
    ;;
esac

live_exe="${build_dir}/bin/regimeflow_live"
if [[ ! -x "${live_exe}" ]]; then
  echo "Missing live binary: ${live_exe}" >&2
  exit 1
fi

log_file="$(mktemp)"
cleanup() {
  rm -f "${log_file}"
}
trap cleanup EXIT

python3 - "${live_exe}" "${config_path}" "${log_file}" <<'PY'
import os
import signal
import subprocess
import sys
import time

exe, config_path, log_path = sys.argv[1:]
with open(log_path, "w", encoding="utf-8") as log:
    proc = subprocess.Popen(
        [exe, "--config", config_path],
        stdout=log,
        stderr=subprocess.STDOUT,
        cwd=os.path.dirname(os.path.dirname(exe)),
        env=os.environ.copy(),
    )

    started = False
    deadline = time.time() + 10.0
    while time.time() < deadline:
        if proc.poll() is not None:
            break
        time.sleep(0.25)
        with open(log_path, "r", encoding="utf-8") as reader:
            content = reader.read()
        if "Live engine started (connected)" in content:
            started = True
            break

    if proc.poll() is None:
        proc.terminate()
        try:
            proc.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=5.0)

if not started:
    with open(log_path, "r", encoding="utf-8") as reader:
        sys.stdout.write(reader.read())
    sys.exit(1)
PY
