# Shared helper sourced by the t009..t014 Unicode regression tests.
#
# Sets $utf8_locale to a UTF-8 locale that is actually installed on the
# host (preferring en_US.UTF-8, then C.UTF-8, then any UTF-8 entry from
# `locale -a`).  If no UTF-8 locale is available, prints a SKIP message
# and exits 0 so the caller (and the test runner) treats the test as
# skipped rather than failed.

# Try preferred locales first (en_US.UTF-8, then C.UTF-8), allowing
# optional @modifier suffixes (e.g. en_US.UTF-8@euro).  Fall back to
# any UTF-8 locale reported by `locale -a`.
utf8_locale=$(locale -a 2>/dev/null | grep -i '^en_US\.UTF-\?8\(@.*\)\?$' | head -n 1)
if [ -z "$utf8_locale" ]; then
    utf8_locale=$(locale -a 2>/dev/null | grep -i '^C\.UTF-\?8\(@.*\)\?$' | head -n 1)
fi
if [ -z "$utf8_locale" ]; then
    utf8_locale=$(locale -a 2>/dev/null | grep -i 'UTF-\?8' | head -n 1)
fi
if [ -z "$utf8_locale" ]; then
    echo "SKIP: no UTF-8 locale available"
    exit 0
fi
