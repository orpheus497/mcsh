#!/bin/sh
# t021_sysinfo_freebsd.sh — the FreeBSD sources behind `sysinfo' (tc.fetch.c).
#
# Each row the panel reads from FreeBSD's own interfaces is held against the
# tool FreeBSD itself provides for the same fact: sysctl(8), swapinfo(8),
# mount(8), kenv(1) and pkg(8).  A row whose source this machine does not
# have - no swap device, no battery, no SMBIOS - must be absent, not guessed.
# Skipped on any other system.

[ "$(uname -s)" = FreeBSD ] || { echo "not FreeBSD"; exit 77; }

fail=0
CFG=$(mktemp -d "${TMPDIR:-/tmp}/t021.XXXXXX") || exit 1
trap 'rm -rf "$CFG"' EXIT INT TERM
mkdir -p "$CFG/mcsh"
printf 'logo = none\npalette = off\n' > "$CFG/mcsh/sysinfo.conf"
panel=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1)
row() { printf '%s\n' "$panel" | sed -n "s/^$1  *//p"; }
bad() { printf '%s\n' "$*"; fail=1; }

# A byte count as fetch_size() renders it: binary units, one decimal digit,
# rounded towards zero.
size() {
    awk -v b="$1" 'BEGIN {
        split("B KiB MiB GiB TiB PiB", u, " "); s = 1; i = 1
        while (i < 6 && b / s >= 1024) { s *= 1024; i++ }
        if (i == 1) printf "%d B\n", b
        else printf "%d.%d %s\n", int(b / s), int((b % s) * 10 / s), u[i]
    }'
}
total_of() { printf '%s\n' "$1" | sed -n 's/.* \/ \(.*\) ([0-9]*%).*/\1/p'; }

# Memory: present, its total hw.physmem, and below 100% - with no
# _SC_AVPHYS_PAGES in FreeBSD's sysconf(), it used to read "X / X (100%)".
mem=$(row Memory)
case "$mem" in
    '')        bad "no Memory row" ;;
    *'(100%)') bad "the Memory row reports all memory in use: [$mem]" ;;
esac
want=$(size "$(sysctl -n hw.physmem)")
[ -z "$mem" ] || [ "$(total_of "$mem")" = "$want" ] ||
    bad "the Memory total is [$(total_of "$mem")], hw.physmem is $want"

# Swap: the devices swapinfo(8) lists, or no row at all.
kb=$(swapinfo -k | awk '$1 ~ /^\/dev\// { t += $2 } END { print t + 0 }')
swap=$(row Swap)
if [ "$kb" -gt 0 ]; then
    want=$(size $((kb * 1024)))
    [ "$(total_of "$swap")" = "$want" ] ||
        bad "the Swap total is [$(total_of "$swap")], swapinfo says $want"
else
    [ -z "$swap" ] || bad "a Swap row with no swap device: [$swap]"
fi

# Uptime and Load: both present; Load is vm.loadavg, read again once if the
# averages moved between the two reads.
[ -n "$(row Uptime)" ] || bad "no Uptime row"
for try in 1 2; do
    want=$(sysctl -n vm.loadavg | tr -d '{}' | sed 's/^ *//; s/ *$//')
    load=$(XDG_CONFIG_HOME="$CFG" "$MCSH" -f -c 'sysinfo -n' 2>&1 |
           sed -n 's/^Load  *//p')
    [ "$load" = "$want" ] && break
done
[ "$load" = "$want" ] || bad "Load is [$load], vm.loadavg is [$want]"

# CPU: named by hw.model; clocked at the top of dev.cpu.0.freq_levels where
# cpufreq(4) publishes one, else at hw.clockrate.
cpu=$(row CPU)
model=$(sysctl -n hw.model | sed 's/ @ .*//')
case "$cpu" in
    "$model"*) ;;
    *) bad "the CPU row [$cpu] does not begin with hw.model [$model]" ;;
esac
mhz=$(sysctl -n dev.cpu.0.freq_levels 2>/dev/null |
      tr ' ' '\n' | cut -d/ -f1 | sort -n | tail -1)
[ -n "$mhz" ] || mhz=$(sysctl -n hw.clockrate 2>/dev/null)
if [ -n "$mhz" ] && [ "$mhz" -ge 10 ]; then
    want=$(printf '%d.%02d GHz' $((mhz / 1000)) $(((mhz % 1000) / 10)))
    case "$cpu" in
        *" @ $want") ;;
        *) bad "the CPU row [$cpu] does not end with @ $want" ;;
    esac
fi

# Disk: the type of the filesystem on /, as mount(8) names it.
want=$(mount -p | awk '$2 == "/" { t = $3 } END { print t }')
case "$(row 'Disk (\/)')" in
    *" - $want") ;;
    *) bad "the Disk row [$(row 'Disk (\/)')] does not end with - $want" ;;
esac

# Host: the SMBIOS strings the loader put in the kernel environment.
product=$(kenv -q smbios.system.product 2>/dev/null)
version=$(kenv -q smbios.system.version 2>/dev/null)
if [ -n "$product" ]; then
    want=$product${version:+ $version}
    [ "$(row Host)" = "$want" ] || bad "Host is [$(row Host)], kenv says [$want]"
else
    [ -z "$(row Host)" ] || bad "a Host row with no SMBIOS strings: [$(row Host)]"
fi

# Battery: acpi_battery(4)'s charge, or no row without a battery.
units=$(sysctl -n hw.acpi.battery.units 2>/dev/null || echo 0)
life=$(sysctl -n hw.acpi.battery.life 2>/dev/null || echo -1)
if [ "$units" -gt 0 ] && [ "$life" -ge 0 ] && [ "$life" -le 100 ]; then
    case "$(row Battery)" in
        "$life%"*) ;;
        *) bad "Battery is [$(row Battery)], hw.acpi.battery.life is $life" ;;
    esac
else
    [ -z "$(row Battery)" ] || bad "a Battery row with no battery: [$(row Battery)]"
fi

# Terminal: the parent walk now reads kern.proc.pid, so an ancestor named
# like an emulator is found by name.
cp /bin/sh "$CFG/foot"
term=$(env -u TERM_PROGRAM -u TERM_PROGRAM_VERSION XDG_CONFIG_HOME="$CFG" \
       "$CFG/foot" -c "\"$MCSH\" -f -c 'sysinfo -n'; :" 2>&1 |
       sed -n 's/^Terminal  *//p')
[ "$term" = foot ] || bad "under a parent named foot, Terminal is [$term]"

# Packages, when built with SQLite: pkg / ports / manual, held against
# pkg(8) itself.  pkg annotates what it installed from a repository with
# "repository"; ports builds have no such tag; "manual" is a command in
# /usr/local/bin that pkg which(8) cannot find an owner for.
case $("$MCSH" -f -c 'echo $version' 2>/dev/null) in
*sqlite*)
    if pkg -N >/dev/null 2>&1 && [ -f /var/db/pkg/local.sqlite ] &&
       total=$(pkg query '%n' 2>/dev/null | wc -l | tr -d ' '); then
        repo=$(pkg query '%n %At' 2>/dev/null |
               awk '$2 == "repository" { print $1 }' | sort -u | wc -l |
               tr -d ' ')
        ports=$((total - repo))
        manual=0
        for f in /usr/local/bin/*; do
            [ -f "$f" ] && [ -x "$f" ] || continue
            pkg which -q "$f" >/dev/null 2>&1 || manual=$((manual + 1))
        done
        pk=$(row Packages)
        for want in "$repo (pkg)" "$ports (ports)" "$manual (manual)"; do
            case "$want" in 0\ *) continue ;; esac
            case "$pk" in
                *"$want"*) ;;
                *) bad "Packages is [$pk], expected it to include $want" ;;
            esac
        done
    fi ;;
esac

exit $fail
