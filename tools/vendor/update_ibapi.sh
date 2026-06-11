#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
vendor_root="$repo_root/third_party/ibapi"
checksums="$vendor_root/SHA256SUMS"
expected_protobuf_macro="3021012"

usage() {
  cat <<USAGE
Usage:
  tools/vendor/update_ibapi.sh --verify-current
  tools/vendor/update_ibapi.sh --archive PATH --expected-sha256 SHA256 --apply

This script intentionally requires a reviewed archive checksum. It does not
perform blind network updates.
USAGE
}

verify_current() {
  if [[ ! -f "$checksums" ]]; then
    echo "Missing $checksums" >&2
    return 1
  fi
  (cd "$repo_root" && sha256sum -c "third_party/ibapi/SHA256SUMS")
}

find_ibjts_root() {
  local extracted="$1"
  local match
  match="$(find "$extracted" -type f -path '*/IBJts/API_VersionNum.txt' -print -quit)"
  if [[ -z "$match" ]]; then
    echo "Could not find IBJts/API_VersionNum.txt in archive" >&2
    return 1
  fi
  dirname "$match"
}

assert_clean_scope() {
  local disallowed
  disallowed="$(find "$vendor_root" -type f \( \
    -name '*.jar' -o \
    -name '*.class' -o \
    -name '*.java' -o \
    -name '*.py' -o \
    -name '*.proto' \
  \) -print)"
  if [[ -n "$disallowed" ]]; then
    echo "Disallowed files found in vendored IB API scope:" >&2
    echo "$disallowed" >&2
    return 1
  fi
}

assert_protobuf_stub_version() {
  local probe="$vendor_root/IBJts/source/cppclient/client/protobufUnix/ExecutionRequest.pb.h"
  if [[ ! -f "$probe" ]]; then
    echo "Missing protobuf probe file: $probe" >&2
    return 1
  fi
  if ! grep -Eq "${expected_protobuf_macro}[[:space:]]*< PROTOBUF_MIN_PROTOC_VERSION" "$probe"; then
    echo "Bundled IB protobuf stubs do not match expected protobuf 3.21.12 macro $expected_protobuf_macro" >&2
    return 1
  fi
}

write_checksums() {
  (cd "$repo_root" && find third_party/ibapi -type f \
    ! -path 'third_party/ibapi/SHA256SUMS' \
    -print0 | sort -z | xargs -0 sha256sum > third_party/ibapi/SHA256SUMS)
}

archive=""
expected_sha=""
apply_update=0
verify_only=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --verify-current)
      verify_only=1
      shift
      ;;
    --archive)
      archive="${2:-}"
      shift 2
      ;;
    --expected-sha256)
      expected_sha="${2:-}"
      shift 2
      ;;
    --apply)
      apply_update=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ "$verify_only" -eq 1 ]]; then
  verify_current
  assert_clean_scope
  assert_protobuf_stub_version
  exit 0
fi

if [[ -z "$archive" || -z "$expected_sha" || "$apply_update" -ne 1 ]]; then
  usage >&2
  exit 2
fi

if [[ ! -f "$archive" ]]; then
  echo "Archive not found: $archive" >&2
  exit 1
fi

actual_sha="$(sha256sum "$archive" | awk '{print $1}')"
if [[ "$actual_sha" != "$expected_sha" ]]; then
  echo "Archive checksum mismatch" >&2
  echo "expected: $expected_sha" >&2
  echo "actual:   $actual_sha" >&2
  exit 1
fi

tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

case "$archive" in
  *.zip)
    unzip -q "$archive" -d "$tmpdir"
    ;;
  *.tar|*.tar.gz|*.tgz)
    tar -xf "$archive" -C "$tmpdir"
    ;;
  *)
    echo "Unsupported archive type: $archive" >&2
    exit 1
    ;;
esac

ibjts_root="$(find_ibjts_root "$tmpdir")"
client_root="$ibjts_root/source/cppclient/client"
if [[ ! -d "$client_root" ]]; then
  echo "Missing source/cppclient/client in archive" >&2
  exit 1
fi

rm -rf "$vendor_root/IBJts"
mkdir -p "$vendor_root/IBJts/source/cppclient"
cp -a "$ibjts_root/API_VersionNum.txt" "$vendor_root/IBJts/"
if [[ -f "$ibjts_root/CMakeLists.txt" ]]; then
  cp -a "$ibjts_root/CMakeLists.txt" "$vendor_root/IBJts/"
fi
if [[ -f "$ibjts_root/source/cppclient/Intel_lib_build.txt" ]]; then
  cp -a "$ibjts_root/source/cppclient/Intel_lib_build.txt" "$vendor_root/IBJts/source/cppclient/"
fi
cp -a "$client_root" "$vendor_root/IBJts/source/cppclient/"

assert_clean_scope
assert_protobuf_stub_version
write_checksums

echo "IB API vendor tree updated. Review VENDOR.md, PATCHES.md, and run native builds/tests before committing."
