#!/bin/sh
# t006_function_builtin.sh — "function" builtin stores and executes body

# function reads lines until "return" — needs a script file
tmpscript=$(mktemp /tmp/t006.XXXXXX.csh)
cat > "$tmpscript" << 'MCSH_SCRIPT'
function greet
echo hello
return
greet
MCSH_SCRIPT

out=$("$MCSH" -f "$tmpscript" 2>&1)
rm -f "$tmpscript"

case "$out" in
    hello) exit 0 ;;
    *)     echo "expected 'hello', got: $out"; exit 1 ;;
esac
