#!/bin/sh
# t100_run_validation.sh - validation errors for run builtin

tmpdir=$(mktemp -d /tmp/t100.XXXXXX) || exit 1
trap 'rm -rf "$tmpdir"' EXIT INT TERM

cat > "$tmpdir/script.mcsh" <<'MCSH_SCRIPT'
echo hello
MCSH_SCRIPT

cat > "$tmpdir/hello.c" <<'EOF_HELLO'
int main(void) { return 0; }
EOF_HELLO

out=$("$MCSH" -f -c "run $tmpdir/script.mcsh" 2>&1)
status=$?
if [ $status -eq 0 ]; then
    printf 'expected non-zero status for .mcsh input, got 0\n'
    exit 1
fi
printf '%s\n' "$out" | grep -F "'run' is for C files only." >/dev/null 2>&1 || {
    printf "expected .mcsh guard message, got: %s\n" "$out"
    exit 1
}

out=$("$MCSH" -f -c "run $tmpdir/missing.c" 2>&1)
status=$?
if [ $status -eq 0 ]; then
    printf 'expected non-zero status for missing file, got 0\n'
    exit 1
fi
printf '%s\n' "$out" | grep -F "file not found" >/dev/null 2>&1 || {
    printf "expected missing-file message, got: %s\n" "$out"
    exit 1
}

if command -v gcc >/dev/null 2>&1; then
    out=$("$MCSH" -f -c "set mcsh_cc = gcc; run $tmpdir/hello.c" 2>&1)
    status=$?
    if [ $status -eq 0 ]; then
        printf 'expected non-zero status for non-clang mcsh_cc, got 0\n'
        exit 1
    fi
    printf '%s\n' "$out" | grep -F "not a clang-family compiler" >/dev/null 2>&1 || {
        printf "expected clang-family enforcement message, got: %s\n" "$out"
        exit 1
    }
fi

exit 0
