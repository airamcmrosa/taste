#!/usr/bin/env bash
# Usage: scripts/run-tests.sh <T-NN>
# Builds tests/test_<TNN>.cpp against tests/mock, runs it, writes .test-results/<T-NN>.json
# (CLAUDE.md section 2.1). Exit code 0 only when status is pass.
set -u

if [ $# -ne 1 ]; then
  echo "usage: $0 <T-NN>" >&2
  exit 2
fi

TASK="$1"
cd "$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)" || exit 2

# REQ IDs covered per task. Extend here when a task adds tests.
case "$TASK" in
  T-03)
    REQS="REQ-INP-01 REQ-INP-02 REQ-INP-03 REQ-LED-02 REQ-LED-03 REQ-LED-04 REQ-HAP-01 REQ-HAP-02 REQ-HAP-03 REQ-HAP-04 REQ-HAP-05 REQ-FW-01 REQ-FW-03 REQ-PWR-01 REQ-PWR-04"
    ;;
  T-04)
    # TODO(T-04): REQ-FW-02 REQ-FW-04 REQ-INP-03
    REQS="REQ-FW-02 REQ-FW-04 REQ-INP-03"
    ;;
  T-05)
    # TODO(T-05): REQ-SAF-01 REQ-SAF-02 REQ-REL-02
    REQS="REQ-SAF-01 REQ-SAF-02 REQ-REL-02"
    ;;
  *)
    echo "run-tests.sh: unknown task '$TASK' (add it to the case statement)" >&2
    exit 2
    ;;
esac

SRC="tests/test_${TASK//-/}.cpp"
if [ ! -f "$SRC" ]; then
  echo "run-tests.sh: missing test file $SRC" >&2
  exit 2
fi

mkdir -p .test-results
OUT=".test-results/${TASK}.json"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/paladar-test.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

TESTS=0
FAILED=1
STATUS="fail"

echo "== building $SRC =="
if g++ -std=c++17 -Wall -Wextra -Werror -I tests/mock "$SRC" -o "$TMP/test" 2>&1; then
  echo "== running =="
  "$TMP/test" 2>&1 | tee "$TMP/out.txt"
  RC=${PIPESTATUS[0]}
  LINE="$(grep -E '^RESULT tests=[0-9]+ failed=[0-9]+' "$TMP/out.txt" | tail -n 1)"
  if [ -n "$LINE" ]; then
    TESTS="$(echo "$LINE" | sed -E 's/.*tests=([0-9]+).*/\1/')"
    FAILED="$(echo "$LINE" | sed -E 's/.*failed=([0-9]+).*/\1/')"
    if [ "$RC" -eq 0 ] && [ "$FAILED" -eq 0 ] && [ "$TESTS" -gt 0 ]; then
      STATUS="pass"
    else
      STATUS="fail"
      [ "$FAILED" -eq 0 ] && FAILED=1
    fi
  else
    echo "run-tests.sh: no RESULT line (test crashed?)" >&2
    TESTS=0; FAILED=1; STATUS="fail"
  fi
else
  echo "run-tests.sh: compile failed" >&2
  TESTS=0; FAILED=1; STATUS="fail"
fi

# AVR compile gate: sketch must build for the Nano with no warnings from the sketch itself
# (core/library warnings are ignored). Skipped when arduino-cli is not installed.
if [ "$STATUS" = "pass" ] && command -v arduino-cli >/dev/null 2>&1; then
  echo "== compiling firmware/paladar for arduino:avr:nano =="
  if arduino-cli compile --fqbn arduino:avr:nano --warnings all --build-path "$TMP/avr" firmware/paladar >"$TMP/avr.txt" 2>&1; then
    if grep -E 'firmware/paladar/.*(warning|error)' "$TMP/avr.txt"; then
      echo "run-tests.sh: AVR build has sketch warnings" >&2
      STATUS="fail"; FAILED=$((FAILED + 1))
    else
      tail -n 2 "$TMP/avr.txt"
    fi
  else
    cat "$TMP/avr.txt" >&2
    STATUS="fail"; FAILED=$((FAILED + 1))
  fi
  TESTS=$((TESTS + 1))
fi

REQ_JSON=""
for r in $REQS; do
  [ -n "$REQ_JSON" ] && REQ_JSON="$REQ_JSON,"
  REQ_JSON="$REQ_JSON\"$r\""
done

TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
printf '{"task":"%s","status":"%s","reqs":[%s],"tests":%s,"failed":%s,"timestamp":"%s"}\n' \
  "$TASK" "$STATUS" "$REQ_JSON" "$TESTS" "$FAILED" "$TS" > "$OUT"

echo "== $STATUS: tests=$TESTS failed=$FAILED -> $OUT =="
[ "$STATUS" = "pass" ]
