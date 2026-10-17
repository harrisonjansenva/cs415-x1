#!/usr/bin/env bash

# Targets configuration
TARGET_CDN="http://example.com/"
TARGET_UTAH="http://mirrors.xmission.com/debian/README"
TARGET_CALIFORNIA="http://mirrors.ocf.berkeley.edu/debian/README"
TARGET_NEWYORK="http://mirrors.rit.edu/debian/README"
TARGET_GERMANY="http://ftp.fau.de/debian/README"
TARGET_JAPAN="http://ftp.jaist.ac.jp/debian/README"

DOWNLOAD_UTAH="http://mirrors.xmission.com/debian/dists/trixie/main/binary-amd64/Packages.gz"
DOWNLOAD_JAPAN="http://ftp.jaist.ac.jp/debian/dists/trixie/main/binary-amd64/Packages.gz"

PUBLIC_IP_URL="http://checkip.amazonaws.com/"
DNS_TRACE_HOST="ftp.jaist.ac.jp"

# Validate vantage point argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [laptop|onyx|codespace]" >&2
    exit 1
fi

VANTAGE="$1"
case "$VANTAGE" in
    laptop|onyx|codespace)
        ;;
    *)
        echo "Error: Invalid vantage point '$VANTAGE'. Must be laptop, onyx, or codespace." >&2
        echo "Usage: $0 [laptop|onyx|codespace]" >&2
        exit 1
        ;;
esac

# Build first
make || {
    echo "Error: Build failed" >&2
    exit 1
}

OUTFILE="measurements-${VANTAGE}.txt"

# Run measurements and tee to screen and OUTFILE
(
    SECONDS=0
    FAILED=0

    echo "=== x1 measure.sh  vantage=${VANTAGE} ==="
    echo "date      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "hostname  $(hostname)"
    echo "uname     $(uname -a)"
    echo "commit    $(git rev-parse --short HEAD 2>/dev/null || echo 'unknown')"

    if command -v ip >/dev/null 2>&1; then
        ip -4 addr
    else
        # macOS fallback
        ifconfig
    fi
    echo ""

    # Helper function to run a step
    run_target() {
        local name="$1"
        shift
        echo "=== ${name}  $(date -u +%H:%M:%S) ==="
        "$@"
        local status=$?
        if [ $status -ne 0 ]; then
            echo "FAILED: exit status ${status}"
            FAILED=$((FAILED + 1))
        fi
        echo ""
    }

    # 1. Public address
    echo "=== public address  $(date -u +%H:%M:%S) ==="
    TMP_IP=$(mktemp)
    ./pageload -4 -o "$TMP_IP" "$PUBLIC_IP_URL"
    PUB_STATUS=$?
    if [ $PUB_STATUS -ne 0 ]; then
        echo "FAILED: exit status ${PUB_STATUS}"
        FAILED=$((FAILED + 1))
    else
        cat "$TMP_IP"
    fi
    rm -f "$TMP_IP"
    echo ""

    # 2. Latency targets (-4 -n 5)
    run_target "latency cdn" ./pageload -4 -n 5 "$TARGET_CDN"
    run_target "latency utah" ./pageload -4 -n 5 "$TARGET_UTAH"
    run_target "latency california" ./pageload -4 -n 5 "$TARGET_CALIFORNIA"
    run_target "latency new york" ./pageload -4 -n 5 "$TARGET_NEWYORK"
    run_target "latency germany" ./pageload -4 -n 5 "$TARGET_GERMANY"
    run_target "latency japan" ./pageload -4 -n 5 "$TARGET_JAPAN"

    # 3. Downloads (-4 -n 3)
    run_target "download utah" ./pageload -4 -n 3 "$DOWNLOAD_UTAH"
    run_target "download japan" ./pageload -4 -n 3 "$DOWNLOAD_JAPAN"

    # 4. Two at once against Utah mirror
    echo "=== two at once  $(date -u +%H:%M:%S) ==="
    TMP_TWO_1=$(mktemp)
    TMP_TWO_2=$(mktemp)
    ./pageload -4 "$TARGET_UTAH" > "$TMP_TWO_1" 2>&1 &
    PID1=$!
    ./pageload -4 "$TARGET_UTAH" > "$TMP_TWO_2" 2>&1 &
    PID2=$!

    wait $PID1
    STATUS1=$?
    wait $PID2
    STATUS2=$?

    cat "$TMP_TWO_1"
    if [ $STATUS1 -ne 0 ]; then
        echo "Instance 1 failed with status $STATUS1"
        FAILED=$((FAILED + 1))
    fi

    cat "$TMP_TWO_2"
    if [ $STATUS2 -ne 0 ]; then
        echo "Instance 2 failed with status $STATUS2"
        FAILED=$((FAILED + 1))
    fi
    rm -f "$TMP_TWO_1" "$TMP_TWO_2"
    echo ""

    # 5. DNS trace
    echo "=== dns trace  $(date -u +%H:%M:%S) ==="
    if command -v dig >/dev/null 2>&1; then
        dig +trace "$DNS_TRACE_HOST" || echo "Notice: dig +trace failed or was blocked by local network"
    else
        echo "Notice: dig command is not installed"
    fi
    echo ""

    # Summary
    echo "=== done in ${SECONDS} s, ${FAILED} failed ==="

) 2>&1 | tee "$OUTFILE"
