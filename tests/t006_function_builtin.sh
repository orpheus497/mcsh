#!/bin/sh
# t006_function_builtin.sh — "function" builtin stores and executes body

tmpscript=$(mktemp /tmp/t006.XXXXXX)
trap 'rm -f "$tmpscript"' EXIT INT TERM

cat > "$tmpscript" << 'MCSH_SCRIPT'
function greet
echo hello
return
greet
MCSH_SCRIPT

out=$("$MCSH" -f "$tmpscript" 2>&1)
rc=$?

if [ $rc -ne 0 ]; then
    echo "FAIL: mcsh exited with status $rc: $out"
    exit 1
fi

case "$out" in
    hello) exit 0 ;;
    *)     echo "expected 'hello', got: $out"; exit 1 ;;
esac
