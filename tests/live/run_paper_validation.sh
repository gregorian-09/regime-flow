#!/usr/bin/env bash
set -euo pipefail

broker="${1:-}"
source_dir="${2:-}"
build_dir="${3:-}"
config_path="${4:-}"
mode="${VALIDATION_MODE:-submit_cancel_reconcile}"
quantity="${VALIDATION_QUANTITY:-}"
limit_price="${VALIDATION_LIMIT_PRICE:-}"

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

validator="${build_dir}/bin/regimeflow_live_validate"
if [[ ! -x "${validator}" ]]; then
  echo "Missing validation binary: ${validator}" >&2
  exit 1
fi

cmd=("${validator}" "--config" "${config_path}" "--mode" "${mode}")
if [[ -n "${quantity}" ]]; then
  cmd+=("--quantity" "${quantity}")
fi
if [[ -n "${limit_price}" ]]; then
  cmd+=("--limit-price" "${limit_price}")
fi

"${cmd[@]}"
