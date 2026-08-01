#!/bin/sh
# t101_run_basic.sh - run compiles, executes, and propagates exit status

has_clang=0
if command -v clang >/dev/null 2>&1; then
    has_clang=1
elif command -v cc >/dev/null 2>&1 && cc --version 2>&1 | grep -qi clang; then
    has_clang=1
fi
if [ "$has_clang" -ne 1 ]; then
    echo "SKIP: no clang-family compiler in PATH"
    exit 77
fi

tmpdir=$(mktemp -d /tmp/t101.XXXXXX) || exit 1
trap 'rm -rf "$tmpdir"' EXIT INT TERM

cat > "$tmpdir/hello.c" <<'EOF_HELLO'
#include <stdio.h>
int main(void) {
    puts("hello");
    return 0;
}
EOF_HELLO

cat > "$tmpdir/exit42.c" <<'EOF_EXIT42'
int main(void) {
    return 42;
}
EOF_EXIT42

cat > "$tmpdir/args.c" <<'EOF_ARGS'
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 3)
        return 2;
    printf("%s %s\n", argv[1], argv[2]);
    return 0;
}
EOF_ARGS

mkdir -p "$tmpdir/proj"
cat > "$tmpdir/proj/util.h" <<'EOF_UTIL_H'
int add(int a, int b);
EOF_UTIL_H
cat > "$tmpdir/proj/util.c" <<'EOF_UTIL_C'
#include "util.h"
int add(int a, int b) { return a + b; }
EOF_UTIL_C
cat > "$tmpdir/proj/main.c" <<'EOF_MAIN_C'
#include <stdio.h>
#include "util.h"
int main(void) {
    printf("%d\n", add(2, 3));
    return 0;
}
EOF_MAIN_C

out=$("$MCSH" -f -c "set mcsh_cache_dir = $tmpdir/cache; run $tmpdir/hello.c" 2>&1)
status=$?
if [ $status -ne 0 ] || [ "$out" != "hello" ]; then
    printf "expected hello/0, got status=%d output=%s\n" "$status" "$out"
    exit 1
fi

out=$("$MCSH" -f -c "set mcsh_cache_dir = $tmpdir/cache; run $tmpdir/hello.c" 2>&1)
status=$?
if [ $status -ne 0 ] || [ "$out" != "hello" ]; then
    printf "expected cached hello/0, got status=%d output=%s\n" "$status" "$out"
    exit 1
fi

"$MCSH" -f -c "set mcsh_cache_dir = $tmpdir/cache; run $tmpdir/exit42.c >/dev/null" >/dev/null 2>&1
status=$?
if [ $status -ne 42 ]; then
    printf "expected exit status 42, got %d\n" "$status"
    exit 1
fi

out=$("$MCSH" -f -c "set mcsh_cache_dir = $tmpdir/cache; run $tmpdir/args.c one two" 2>&1)
status=$?
if [ $status -ne 0 ] || [ "$out" != "one two" ]; then
    printf "expected streamlined arg passthrough, got status=%d output=%s\n" "$status" "$out"
    exit 1
fi

out=$("$MCSH" -f -c "set mcsh_cache_dir = $tmpdir/cache; run $tmpdir/proj" 2>&1)
status=$?
if [ $status -ne 0 ] || [ "$out" != "5" ]; then
    printf "expected project build+run output 5, got status=%d output=%s\n" "$status" "$out"
    exit 1
fi

exit 0
