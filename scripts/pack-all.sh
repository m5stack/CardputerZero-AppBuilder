#!/usr/bin/env bash
#
# Build .deb for every example that has packaging/meta.env.
#
# Usage:
#   scripts/pack-all.sh [--skip-build] [--output-dir dist] [--only DIR1,DIR2,...]
#
# Exits non-zero if ANY example fails; prints a summary at the end.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SKIP_BUILD=""
OUTPUT_DIR="$REPO_ROOT/dist"
ONLY=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-build) SKIP_BUILD="--skip-build"; shift ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        --only) ONLY="$2"; shift 2 ;;
        -h|--help) grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown arg: $1" >&2; exit 2 ;;
    esac
done

mkdir -p "$OUTPUT_DIR"

declare -a TARGETS=()
if [[ -n "$ONLY" ]]; then
    IFS=',' read -r -a TARGETS <<< "$ONLY"
else
    while IFS= read -r meta; do
        TARGETS+=("$(dirname "$(dirname "$meta")")")
    done < <(find "$REPO_ROOT" -mindepth 3 -maxdepth 3 -path "*/packaging/meta.env" | sort)
fi

if [[ ${#TARGETS[@]} -eq 0 ]]; then
    echo "no packaging/meta.env found under $REPO_ROOT" >&2
    exit 1
fi

OK=()
FAIL=()
for d in "${TARGETS[@]}"; do
    d="${d%/}"
    [[ "$d" = /* ]] || d="$REPO_ROOT/$d"
    echo
    echo "##### $(basename "$d") #####"
    if "$SCRIPT_DIR/pack-deb.sh" "$d" $SKIP_BUILD --output-dir "$OUTPUT_DIR"; then
        OK+=("$(basename "$d")")
    else
        FAIL+=("$(basename "$d")")
    fi
done

echo
echo "=== summary ==="
echo "ok   : ${#OK[@]}  (${OK[*]:-})"
echo "fail : ${#FAIL[@]}  (${FAIL[*]:-})"
[[ ${#FAIL[@]} -eq 0 ]]
