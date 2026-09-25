# lib_pty.sh - shared helper: build tests/ptydrive and run a program under it.
#
# Sourced, not executed.  Defines:
#   pty_setup            - compile the driver; returns 1 when it cannot be
#                          built or run, in which case the caller should exit
#                          77 (skip) rather than fail.
#   pty_run COLS ROWS    - read keystrokes on stdin, write the raw terminal
#                          byte stream to stdout.
#   pty_cleanup          - remove the scratch directory.
#
# $PTY_DIR holds the scratch directory while a test is running.

PTY_DIR=
PTY_BIN=

pty_setup() {
    src=${PTY_SRC:-./ptydrive.c}
    [ -f "$src" ] || return 1
    cc=${CC:-cc}
    command -v "$cc" >/dev/null 2>&1 || return 1

    PTY_DIR=$(mktemp -d 2>/dev/null) || return 1
    PTY_BIN=$PTY_DIR/ptydrive
    "$cc" -o "$PTY_BIN" "$src" >/dev/null 2>&1 || return 1

    # A container or a build chroot may have no /dev/pts at all; find out now
    # so the caller can skip rather than report a spurious failure.
    : | "$PTY_BIN" -c 20 -r 5 -s 50 /bin/echo pty >/dev/null 2>&1
    [ $? -eq 77 ] && return 1
    return 0
}

pty_run() {
    "$PTY_BIN" -c "$1" -r "$2" ${3:+-d "$3"} "$MCSH" -f -i
}

pty_cleanup() {
    [ -n "$PTY_DIR" ] && rm -rf "$PTY_DIR"
    PTY_DIR=
}
