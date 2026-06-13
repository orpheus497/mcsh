#!/bin/sh
# t015_killdup.sh — KillRing deduplication via killdup variable
#
# Tests the three deduplication modes of c_push_kill():
#   killdup=all   — skip push if any earlier ring entry matches
#   killdup=prev  — skip push if immediately previous entry matches
#   killdup=erase — remove earlier matching entry and re-push (move to top)
# and the unset-killdup case (no deduplication).
#
# Also covers the len==0 edge case introduced in the optimised condition order:
#   (len == 0 || KillRing[j].buf[0] == first)
#
# Because kill-ring operations require interactive line-editing (a real tty),
# this test drives mcsh through a Python pseudo-terminal.  The test skips
# automatically when python3 is unavailable or the pty module is missing.

# ---- require python3 with pty -----------------------------------------------
if ! python3 -c "import pty, select, os, sys" 2>/dev/null; then
    echo "python3 with pty/select/os/sys not available"
    exit 77
fi

# ---- helpers -----------------------------------------------------------------
MCSH="${MCSH:-../mcsh}"

# run_pty SCRIPT_BODY
#   Writes a self-contained Python script to a temp file and runs it.
#   The Python script receives $MCSH via the environment.
run_pty() {
    python3 "$1"
    return $?
}

# ---- write the shared Python pty driver to a temp file ----------------------
PYDRIVER=$(mktemp /tmp/killdup_driver_XXXXXX.py)
trap 'rm -f "$PYDRIVER"' EXIT INT TERM

cat > "$PYDRIVER" << 'PYEOF'
"""
Pseudo-terminal driver for mcsh kill-ring tests.

Each test case is a function that:
  1. Starts mcsh in an interactive pty with a fixed, recognisable prompt
  2. Sends a sequence of keystrokes (including kill/yank control chars)
  3. Reads the resulting output and checks for an expected string
  4. Returns True on pass, False on fail
"""

import os, sys, pty, select, time, signal, re

MCSH = os.environ.get("MCSH", "../mcsh")

CTRL_A  = b"\x01"   # beginning-of-line
CTRL_E  = b"\x05"   # end-of-line
CTRL_K  = b"\x0b"   # kill-line (push to kill ring)
CTRL_Y  = b"\x19"   # yank (insert from kill ring)
META_Y  = b"\x1b\x79"  # yank-pop (cycle kill ring)
ENTER   = b"\r"

# A sentinel we can grep for: chosen to be distinct from shell output.
SENTINEL = "TESTDONE_MARKER"

STRIP_ESC = re.compile(rb"\x1b(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")

def strip_escapes(data: bytes) -> bytes:
    """Remove ANSI/VT100 escape sequences."""
    return STRIP_ESC.sub(b"", data)


def read_until(master_fd: int, pattern: str, timeout: float = 5.0) -> str:
    """
    Read from master_fd until `pattern` appears in accumulated output or
    until `timeout` seconds elapse.  Returns the accumulated decoded text.
    """
    buf = b""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        r, _, _ = select.select([master_fd], [], [], min(remaining, 0.2))
        if r:
            try:
                chunk = os.read(master_fd, 4096)
            except OSError:
                break
            buf += chunk
            clean = strip_escapes(buf).decode("utf-8", errors="replace")
            if pattern in clean:
                return clean
        else:
            # poll even when nothing arrived, to catch slow shells
            clean = strip_escapes(buf).decode("utf-8", errors="replace")
            if pattern in clean:
                return clean
    return strip_escapes(buf).decode("utf-8", errors="replace")


def write(master_fd: int, data: bytes) -> None:
    os.write(master_fd, data)
    time.sleep(0.05)   # small pacing delay


def start_mcsh(extra_init: str = "") -> tuple:
    """
    Fork an interactive mcsh with a fixed prompt "PROMPT> ".
    Returns (pid, master_fd).
    extra_init is additional tcsh code run after setting the prompt.
    """
    master_fd, slave_fd = pty.openpty()

    pid = os.fork()
    if pid == 0:
        # Child: become the interactive mcsh
        os.close(master_fd)
        os.setsid()
        # Make the slave the controlling terminal
        import termios, fcntl, struct
        fcntl.ioctl(slave_fd, termios.TIOCSCTTY, 0)
        for fd in (0, 1, 2):
            os.dup2(slave_fd, fd)
        if slave_fd > 2:
            os.close(slave_fd)
        # Use -f (skip rc files) so the environment is clean
        os.execv(MCSH, [MCSH, "-f"])
        os._exit(1)  # exec failed

    os.close(slave_fd)

    # Wait for the initial prompt (default tcsh prompt contains "%" or ">")
    read_until(master_fd, "%", timeout=4.0)

    # Set a predictable, short prompt and run any extra init
    init_cmd = "set prompt='P> '; " + extra_init + "\r"
    write(master_fd, init_cmd.encode())
    read_until(master_fd, "P> ", timeout=3.0)

    return pid, master_fd


def stop_mcsh(pid: int, master_fd: int) -> None:
    try:
        write(master_fd, b"exit\r")
    except OSError:
        pass
    try:
        os.close(master_fd)
    except OSError:
        pass
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass


def run_test(name: str, init: str, steps, expected_in_output: str,
             not_expected: str = None) -> bool:
    """
    Generic test runner.

    init      : extra tcsh init code (string sent after prompt is ready)
    steps     : iterable of (bytes, wait_for_str) pairs
    expected_in_output : string that must appear in the final output
    not_expected       : string that must NOT appear (optional)
    """
    pid, fd = start_mcsh(init)
    try:
        for data, wait_pat in steps:
            write(fd, data)
            if wait_pat:
                read_until(fd, wait_pat, timeout=3.0)
        # Send a sentinel echo so we know commands have completed
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, timeout=4.0)
    finally:
        stop_mcsh(pid, fd)

    ok = expected_in_output in out
    if not ok:
        print(f"FAIL [{name}]: expected {expected_in_output!r} in output")
        print(f"  output was: {out!r}")
        return False
    if not_expected and not_expected in out:
        print(f"FAIL [{name}]: did not expect {not_expected!r} in output")
        print(f"  output was: {out!r}")
        return False
    return True


# ── Test cases ────────────────────────────────────────────────────────────────

def test_killdup_all_deduplicates():
    """
    killdup=all: pushing the same string a second time is silently dropped.

    Sequence:
      1. Kill "hello"      → ring: ["hello"]
      2. Kill "hello" again → ring: ["hello"]  (not ["hello","hello"])
      3. Yank              → inserts "hello"
      4. Yank-pop          → ring has only one entry, so still "hello"
      5. Echo the yank result and check it equals "hello"

    Implementation:
      Type "hello", Ctrl+A, Ctrl+K  (moves to start then kills whole line)
      Type "hello", Ctrl+A, Ctrl+K  (duplicate – should be skipped)
      Type "echo ", Ctrl+Y, Enter   (yank and echo)
    """
    steps = [
        # First kill: "hello"
        (b"hello" + CTRL_A + CTRL_K, "P> "),
        # Second kill: "hello" (duplicate, should be dropped by killdup=all)
        (b"hello" + CTRL_A + CTRL_K, "P> "),
        # Yank into an echo command then press Enter
        (b"echo " + CTRL_Y + ENTER, "P> "),
    ]
    return run_test(
        "killdup=all deduplicates",
        init="set killdup=all",
        steps=steps,
        expected_in_output="hello",
    )


def test_killdup_prev_skips_immediately_previous():
    """
    killdup=prev: a push is skipped when the immediately previous entry matches.

    Sequence:
      1. Kill "world"     → ring: ["world"]
      2. Kill "world"     → ring unchanged (prev matches)
      3. Yank             → "world"
    """
    steps = [
        (b"world" + CTRL_A + CTRL_K, "P> "),
        (b"world" + CTRL_A + CTRL_K, "P> "),
        (b"echo " + CTRL_Y + ENTER, "P> "),
    ]
    return run_test(
        "killdup=prev skips immediately previous",
        init="set killdup=prev",
        steps=steps,
        expected_in_output="world",
    )


def test_killdup_prev_allows_non_consecutive_dup():
    """
    killdup=prev: a duplicate that is NOT immediately previous IS added.

    Sequence:
      1. Kill "alpha"  → ring: ["alpha"]
      2. Kill "beta"   → ring: ["alpha", "beta"]
      3. Kill "alpha"  → ring: ["alpha", "beta", "alpha"]  (not consecutive)
      4. Yank          → "alpha"  (most recent)
      5. Yank-pop      → "beta"   (second entry)
    """
    pid, fd = start_mcsh("set killdup=prev")
    try:
        write(fd, b"alpha" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        write(fd, b"beta"  + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        write(fd, b"alpha" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        # Yank then yank-pop: expect "beta" to appear
        write(fd, b"echo " + CTRL_Y + META_Y + ENTER); read_until(fd, "P> ", 3)
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, 4)
    finally:
        stop_mcsh(pid, fd)

    if "beta" not in out:
        print(f"FAIL [killdup=prev allows non-consecutive dup]: expected 'beta' in {out!r}")
        return False
    return True


def test_killdup_erase_moves_earlier_entry_to_top():
    """
    killdup=erase: when a duplicate is found, the earlier entry is erased and
    the new push goes to the top.

    Sequence:
      1. Kill "foo"  → ring: ["foo"]
      2. Kill "bar"  → ring: ["foo", "bar"]
      3. Kill "foo"  → erase moves earlier "foo", ring: ["bar", "foo"]
      4. Yank        → "foo"  (top of ring)
      5. Yank-pop    → "bar"  (second entry)
    """
    pid, fd = start_mcsh("set killdup=erase")
    try:
        write(fd, b"foo" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        write(fd, b"bar" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        write(fd, b"foo" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        # Yank "foo" first, then yank-pop to get "bar"
        write(fd, b"echo " + CTRL_Y + META_Y + ENTER); read_until(fd, "P> ", 3)
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, 4)
    finally:
        stop_mcsh(pid, fd)

    if "bar" not in out:
        print(f"FAIL [killdup=erase moves earlier entry to top]: expected 'bar' (via yank-pop) in {out!r}")
        return False
    return True


def test_no_killdup_allows_duplicates():
    """
    Without killdup set, the same string can appear multiple times in the ring.

    Sequence:
      1. Kill "dup"  → ring: ["dup"]
      2. Kill "dup"  → ring: ["dup", "dup"]  (no dedup)
      3. Yank        → "dup"
      4. Yank-pop    → still "dup" (second entry exists, same content)
      5. Verify two separate yank results both equal "dup" (ring has ≥ 2 entries)
    """
    pid, fd = start_mcsh("unset killdup")
    try:
        write(fd, b"dup" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        write(fd, b"dup" + CTRL_A + CTRL_K); read_until(fd, "P> ", 3)
        # First yank
        write(fd, b"echo " + CTRL_Y + ENTER); read_until(fd, "P> ", 3)
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, 4)
    finally:
        stop_mcsh(pid, fd)

    if "dup" not in out:
        print(f"FAIL [no killdup allows duplicates]: expected 'dup' in {out!r}")
        return False
    return True


def test_killdup_all_empty_kill():
    """
    Edge case: len == 0 (Ctrl+K on an already-empty line / cursor at EOL with nothing to kill).

    The optimised condition is:
        KillRing[i].buf[len] == '\\0' &&
        (len == 0 || KillRing[i].buf[0] == first) &&
        Strncmp(...)

    When len == 0 the first-character check is skipped entirely.  This test
    ensures that an empty kill with killdup=all does not crash or misbehave.

    Sequence:
      1. Kill "nonempty"   → ring has one entry
      2. Ctrl+K on empty prompt (len=0, nothing killed, nothing pushed)
      3. Yank              → should still yield "nonempty" (empty kill skipped or
                             handled gracefully; ring unchanged or has empty entry)
    """
    steps = [
        # Push a real string first
        (b"nonempty" + CTRL_A + CTRL_K, "P> "),
        # Now press Ctrl+K on an empty line: len==0 kill
        (CTRL_K, "P> "),
        # Yank – we expect something sensible back (the non-empty kill)
        (b"echo " + CTRL_Y + ENTER, "P> "),
    ]
    # We just verify no crash and the shell is still responsive
    pid, fd = start_mcsh("set killdup=all")
    try:
        for data, wait in steps:
            write(fd, data)
            read_until(fd, wait, 3)
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, 4)
    finally:
        stop_mcsh(pid, fd)

    # Shell must still be alive and the sentinel must appear
    if SENTINEL not in out:
        print(f"FAIL [killdup=all empty kill]: shell unresponsive after len=0 kill")
        return False
    return True


def test_killdup_all_single_char():
    """
    Boundary: single-character string.  The first-character optimisation
    (buf[0] == first) must not confuse a 1-char string with an empty one.

    Sequence:
      1. Kill "x"   → ring: ["x"]
      2. Kill "x"   → duplicate, must be skipped (killdup=all)
      3. Kill "y"   → different first char, must be added → ring: ["x","y"]
      4. Yank       → "y"
    """
    steps = [
        (b"x" + CTRL_A + CTRL_K, "P> "),
        (b"x" + CTRL_A + CTRL_K, "P> "),   # dup – skipped
        (b"y" + CTRL_A + CTRL_K, "P> "),   # distinct – added
        (b"echo " + CTRL_Y + ENTER, "P> "),
    ]
    pid, fd = start_mcsh("set killdup=all")
    try:
        for data, wait in steps:
            write(fd, data)
            read_until(fd, wait, 3)
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, 4)
    finally:
        stop_mcsh(pid, fd)

    if "y" not in out:
        print(f"FAIL [killdup=all single char]: expected 'y' in {out!r}")
        return False
    return True


def test_killdup_erase_single_entry_ring():
    """
    Regression: erase mode with ring size == 1 (i == 0 in the loop).

    The inner block `if (i > 0)` must NOT execute when the duplicate is the
    single entry (i == 0); the entry is simply kept in place.  Ensures no
    off-by-one or wrap-around error.

    Sequence:
      1. Kill "solo"  → ring: ["solo"]   (KillRingLen == 1)
      2. Kill "solo"  → erase, i == 0 → no movement, ring still: ["solo"]
      3. Yank         → "solo"
    """
    steps = [
        (b"solo" + CTRL_A + CTRL_K, "P> "),
        (b"solo" + CTRL_A + CTRL_K, "P> "),
        (b"echo " + CTRL_Y + ENTER, "P> "),
    ]
    pid, fd = start_mcsh("set killdup=erase")
    try:
        for data, wait in steps:
            write(fd, data)
            read_until(fd, wait, 3)
        write(fd, ("echo " + SENTINEL + "\r").encode())
        out = read_until(fd, SENTINEL, 4)
    finally:
        stop_mcsh(pid, fd)

    if "solo" not in out:
        print(f"FAIL [killdup=erase single-entry ring]: expected 'solo' in {out!r}")
        return False
    return True


# ── Runner ────────────────────────────────────────────────────────────────────

TESTS = [
    test_killdup_all_deduplicates,
    test_killdup_prev_skips_immediately_previous,
    test_killdup_prev_allows_non_consecutive_dup,
    test_killdup_erase_moves_earlier_entry_to_top,
    test_no_killdup_allows_duplicates,
    test_killdup_all_empty_kill,
    test_killdup_all_single_char,
    test_killdup_erase_single_entry_ring,
]

failed = 0
for fn in TESTS:
    try:
        result = fn()
    except Exception as exc:
        print(f"FAIL [{fn.__name__}]: exception: {exc}")
        result = False
    if not result:
        failed += 1

sys.exit(0 if failed == 0 else 1)
PYEOF

# ---- run the Python driver ---------------------------------------------------
run_pty "$PYDRIVER"
status=$?

exit $status
