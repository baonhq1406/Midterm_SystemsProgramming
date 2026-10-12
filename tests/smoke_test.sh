#!/bin/sh
set -eu

PROGRAM="$(pwd)/myls"
TEST_DIR="$(mktemp -d)"
cleanup() { rm -rf "$TEST_DIR"; }
trap cleanup EXIT HUP INT TERM

mkdir -p "$TEST_DIR/subdir"
printf 'hello\n' > "$TEST_DIR/plain.txt"
printf 'hidden\n' > "$TEST_DIR/.hidden"
printf '#!/bin/sh\nexit 0\n' > "$TEST_DIR/runme"
chmod +x "$TEST_DIR/runme"
ln -s plain.txt "$TEST_DIR/link"

# Basic listing, hidden-file modes, long format, metadata and recursion.
"$PROGRAM" "$TEST_DIR" >/dev/null
"$PROGRAM" -a "$TEST_DIR" | grep -F '.hidden' >/dev/null
"$PROGRAM" -a "$TEST_DIR" | grep -Fx '.' >/dev/null
"$PROGRAM" -a "$TEST_DIR" | grep -Fx '..' >/dev/null
"$PROGRAM" -A "$TEST_DIR" | grep -F '.hidden' >/dev/null
if "$PROGRAM" -A "$TEST_DIR" | grep -Fx '.' >/dev/null; then
    echo "-A unexpectedly included ." >&2
    exit 1
fi
if "$PROGRAM" -A "$TEST_DIR" | grep -Fx '..' >/dev/null; then
    echo "-A unexpectedly included .." >&2
    exit 1
fi
if [ "$(id -u)" -ne 0 ]; then
    if "$PROGRAM" "$TEST_DIR" | grep -F '.hidden' >/dev/null; then
        echo "default listing unexpectedly included a hidden file" >&2
        exit 1
    fi
fi
"$PROGRAM" -l "$TEST_DIR" >/dev/null
"$PROGRAM" -l "$TEST_DIR/link" | grep -F ' -> plain.txt' >/dev/null
"$PROGRAM" -n "$TEST_DIR" >/dev/null
"$PROGRAM" -i -s "$TEST_DIR" >/dev/null
"$PROGRAM" -F "$TEST_DIR" | grep -F 'runme*' >/dev/null
"$PROGRAM" -R "$TEST_DIR" >/dev/null
"$PROGRAM" -d "$TEST_DIR" | grep -F "$TEST_DIR" >/dev/null
"$PROGRAM" -dR "$TEST_DIR" | grep -F "$TEST_DIR" >/dev/null
"$PROGRAM" -Rd "$TEST_DIR" | grep -F "$TEST_DIR" >/dev/null
"$PROGRAM" -q "$TEST_DIR" >/dev/null
"$PROGRAM" -w "$TEST_DIR" >/dev/null
"$PROGRAM" -t "$TEST_DIR" >/dev/null
"$PROGRAM" -S "$TEST_DIR" >/dev/null
"$PROGRAM" -h -l "$TEST_DIR" >/dev/null
"$PROGRAM" -k -s "$TEST_DIR" >/dev/null

# -s must not print a total when stdout is redirected.
REDIRECTED_OUTPUT="$TEST_DIR/subdir/redirected-output.txt"
"$PROGRAM" -s "$TEST_DIR" > "$REDIRECTED_OUTPUT"

if grep -q '^total ' "$REDIRECTED_OUTPUT"; then
    echo "-s printed total when stdout was redirected" >&2
    exit 1
fi

# -l must still print a total when stdout is redirected.
LONG_OUTPUT="$TEST_DIR/subdir/long-output.txt"
"$PROGRAM" -l "$TEST_DIR" > "$LONG_OUTPUT"

if ! grep -q '^total ' "$LONG_OUTPUT"; then
    echo "-l did not print the directory total" >&2
    exit 1
fi

echo "smoke tests passed"
