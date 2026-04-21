# mcsh — Consolidation Issue Log

Running log of bugs, obsolete code, and modernisation tasks noticed while
consolidating `tcsh` and `etcsh` into `mcsh`. Items are notes, not yet
triaged or prioritised; they will be burned down as polishing proceeds.

See `PLAN.md` for the full phased execution plan derived from this log.

---

## Completed work (2026-04-21)

### Phase 9 — Native interactive syntax highlighting ✓

`set syntax` activates per-keystroke colour highlighting in the interactive
command line editor. The implementation follows Option B: full virtual-display
pipeline integration (no raw ESC bypass).

**Architecture — end-to-end:**

1. `syntax_colorize()` (`ed.syntax.c`) runs after every keystroke dispatch in
   `ed.inputl.c`. Single-pass state machine over `InputBuf[0..LastChar)` emits
   a `SynToken` byte into `SyntaxColor[]` for every input character.
2. `Draw(cp, …)` (`ed.refresh.c`) reads `SyntaxColor[cp - InputBuf]` and sets
   the `vcurrent_color` global (defaults to `SYN_NORMAL` for prompt characters).
3. `Vdraw(c, width)` packs `vcurrent_color` into the upper bits of each display
   `Char` via `SYN_PACK(c, vcurrent_color)` and writes the packed value into
   `Vdisplay[v][h]` directly — no separate parallel colour arrays are used.
4. `update_line()` diffs `Vdisplay` against `Display` per cell; because the
   token is part of the `Char`, colour-only changes are detected automatically
   without any display poisoning.
5. `so_write()` (`ed.screen.c`) extracts the token with `SYN_TOK(cell)` and
   the glyph with `SYN_GLYPH(cell)` per character, then calls
   `SetSGRColor(token)` before output and resets with `SetSGRColor(-1)` at the
   end of each write.
6. `SetSGRColor(int fg)` tracks `cur_sgr` to suppress redundant SGR emissions.
   Emits `ESC[1;{code}m` (bold) or `ESC[{code}m` or `ESC[0m` via `putpure()`.
   When emitting a full reset (`ESC[0m`) it also sets `cur_atr = 0` so terminal
   attribute state stays consistent.  `StopHighlight()` clears `cur_sgr` after
   emitting `T_me` so subsequent `SetSGRColor()` calls are not incorrectly
   skipped.

**Token types and default colours:**

| Token | Colour |
|-------|--------|
| `SYN_KEYWORD` | Bold cyan (36) |
| `SYN_BUILTIN` | Bold green (32) |
| `SYN_CMD_OK` | Green (32) |
| `SYN_CMD_BAD` | Bold red (31) |
| `SYN_OPERATOR` | Yellow (33) |
| `SYN_VARIABLE` | Magenta (35) |
| `SYN_DQUOTE` | Yellow (33) |
| `SYN_SQUOTE` | Yellow (33) |
| `SYN_BACKTICK` | Cyan (36) |
| `SYN_COMMENT` | Bright black (90) |
| `SYN_ERROR` | Bold red (31) — unmatched quote |

**Files changed / added:**

- `ed.syntax.h` — new: `SynToken` enum, `SynColor` struct, `SynPalette[]`,
  `SyntaxColor[]` array, `syntax_colorize()`, `syntax_clear()`,
  `syntax_cache_clear()` declarations; `SYN_PACK`/`SYN_TOK`/`SYN_GLYPH` macros
  for token bit-packing into display `Char` values.
- `ed.syntax.c` — new: tokeniser, LRU command cache (`CMD_CACHE_SIZE=32`),
  `cmd_on_path()` via `stat(2)` + `access(2)` + `$PATH` walk;
  `syntax_cache_clear()` invalidates the cache on PATH/cwd changes.
- `ed.h` — added `vcurrent_color` external; no separate `VcolorDisplay` /
  `ColorDisplay` arrays (tokens are packed directly into `Vdisplay`/`Display`
  `Char` values and extracted with `SYN_TOK()`/`SYN_GLYPH()`).
- `ed.screen.c` — `SetSGRColor()` new static function; `so_write()` extracts
  token with `SYN_TOK()` / glyph with `SYN_GLYPH()` per character;
  `SetAttributes()` resets `cur_sgr` when clearing all attributes;
  `StopHighlight()` clears `cur_sgr` so subsequent `SetSGRColor()` calls
  are not skipped.
- `ed.refresh.c` — `Draw()` sets `vcurrent_color`; `Vdraw()` packs token into
  `Vdisplay` via `SYN_PACK()`; syntax is recolored inside `Repair()` and the
  `CC_REFRESH` path so mutations from completion/expansion are always reflected.
- `ed.inputl.c` — `syntax_colorize()` called inside `Repair()` (covers all
  buffer-mutation switch cases) and before `Refresh()` in the `CC_REFRESH` path.
- `ed.decls.h` — extern declarations for `syntax_colorize()`, `syntax_clear()`,
  `syntax_cache_clear()`.
- `sh.set.c` — `update_vars()` calls `syntax_colorize()`/`syntax_clear()` on
  `set`/`unset syntax`; `dounset` path calls `syntax_clear()`; PATH change also
  calls `syntax_cache_clear()`.
- `tc.const.c` / `tc.const.h` — `STRsyntax[]` constant.
- `Makefile.in` — `ed.syntax.${SUF}` added to `EDOBJS`; `ed.syntax.h` listed
  as a prerequisite for `ed.syntax.${SUF}`, `ed.screen.${SUF}`, and
  `ed.inputl.${SUF}`.
- `dot.mcshrc` — `set syntax` added after `set color`.

### Phase 8 — Code review fixes (PR3, Gemini + CodeRabbit) ✓
- **`configure.ac` TCSH_BASELINE_VERSION:** Replaced hardcoded `"TCSH_VERSION"` literal
  with the actual M4 macro expansion `TCSH_VERSION` so `config.h` stays in sync
  with the single-source version definition in `configure.ac`.
- **`configure.ac` PACKAGE_PATCHLEVEL normalization:** Changed from `printf '%d'` to
  `sed 's/^0*//; s/^$/0/'` to strip leading zeros without invoking numeric
  parsing — prevents invalid C integer literals like `08` or `09` in `patchlevel.h`.
- **`sh.func.c` doif type safety:** Changed local variable `i` from `int` to
  `tcsh_number_t` in `doif()` so wide expression results (values > `INT_MAX`)
  are not silently truncated before the truth/false branch decision.
- **`vms.termcap.c` octal escapes:** Added `case '4':` through `case '7':` to the
  backslash-escape switch; continuation digit validation now checks `<= '7'`
  so the octal parse is strict and correct for all legal octal digits.
- **All prior code review items (noted below) were already addressed by the
  previous session on 2026-04-20.**

### Phase 7b — Native features (2026-04-21) ✓
- **Fish-style predictive autocomplete:** `predict_from_history()` in `ed.chared.c`
  scans `Histlist` for a prefix match and fills `GhostBuf`; `DrawGhost()` in
  `ed.refresh.c` renders the ghost text dimmed after the cursor and repositions
  via backspaces. `e_predict_accept` (Right-Arrow) copies `GhostBuf` into
  the input buffer. Ghost text is cleared on any non-insert command.
- **Native git branch prompt escapes `%g` / `%G`:** `git_get_info()` in
  `tc.prompt.c` walks upward from `$cwd` looking for `.git/HEAD`; result is
  cached per-CWD pointer. `%g` expands to the branch name; `%G` appends the
  operation state (`MERGING`, `REBASING-i`, `REBASING`, `AM`, `CHERRY-PICKING`,
  `REVERTING`, `BISECTING`) when applicable.
- **`dot.mcshrc` rewrite:** Reference start-up file fully rewritten to mirror
  `.tcshrc` section structure. Adds `set color`, `rprompt='%S%G%s'`,
  `histfile=~/.mcsh_history`, `histdup=erase`, `set symlinks=chase`,
  full keybinding set, programmable completions, and a complete alias block.
  No starship dependency.

### Phase 4b — Additional bug fixes (2026-04-20) ✓
- **`acaux/install-sh` name patterns:** All three `case` statements that protect
  against problematic names (`-`, `=`, `(`, `)`, `!`) updated to use `[=\(\)!]*`
  (with `*` suffix) so multi-character names beginning with those characters
  are caught, not just single-character names.
- **`m4/lib-prefix.m4` comment typo:** `dn;` on line 153 corrected to `dnl` so
  the comment is properly discarded by M4 and not emitted into `configure`.
- **`m4/po.m4` C# DLL cleanup:** Generated Makefile rule error handler now
  removes `$frobbedlang/$(DOMAIN).resources.dll` (the actual target) instead
  of `$frobbedlang.msg` (the source).
- **`m4/po.m4` GETTEXT_MACRO_VERSION:** Updated from `0.22` to `0.23` to match
  the file header.
- **`ed.defns.c` NLS catalog ID collision:** `predict-accept` command uses
  catalog ID `CSAVS(3, 124, …)` (was 122, which collides with `newline-and-hold`).
- **`ed.refresh.c` DrawGhost stale rendering (partial mitigation):** `DrawGhost()` now tracks
  `prev_ghost_cols` and `prev_ghost_start_h` statically; erases the previous ghost overlay with
  spaces and backspaces before drawing the new one. This reduces staleness but is a partial fix
  only — `DrawGhost()` still bypasses `Display`/`Vdisplay` and writes directly to the terminal;
  the root cause remains because clearing is not applied through the virtual-display pipeline
  used by `Refresh()`. Full integration into `Display`/`Vdisplay` is deferred.
- **`ed.inputl.c` GhostBuf clear redraw:** Ghost text is cleared and `Refresh()`
  called only when `GhostBuf` was non-empty, avoiding spurious redraws.
- **`ed.chared.c` e_predict_accept NUL terminator:** NUL written after the copy
  loop; bounds check against `InputLim` preserved.
- **`ed.chared.c` predict_from_history strip:** `'\n'` and `'\r'` stripped from
  ghost text so no trailing newline contaminates the ghost overlay.
- **`dch-template.in`:** Distribution changed from `unstable` to `UNRELEASED`;
  placeholder changelog line replaced with real release notes covering all
  Phase 1–7 deliverables.
- **`alacritty.toml`:** pywal `import` line commented out (not active); `program`
  set to `"mcsh"` (resolved via `PATH`); colour theming delegated to a
  machine-local override file.
- **`vms.termcap.c` tgetnum/tgetflag/tgetstr OOB:** Each colon-scan loop
  (`while (*cp && *cp != ':')`) was already guarded by `if (!*cp) return(-1)`
  before the inner loop; confirmed correct.
- **`vms.termcap.c` sscanf + strcmp:** Both `sscanf` calls changed to use
  `%[^|:]` scanset (stops at `|` or `:`) and `strcmp` for exact name match.
- **`vms.termcap.c` fgets overflow:** Continuation loop now computes
  `remaining = sizeof(bp) - bplen - 1` and passes that to `fgets`; checked
  for NULL return.
- **`vms.termcap.c` tgoto bounds:** Static `ret[]` enlarged to 64 bytes with
  `rend` pointer; `%d` format uses `snprintf` into a temp buffer then
  `memcpy` with bounds check; `+` and `%` cases check `rp < rend`.
- **`sh.func.c` doif (previous session):** `int i` → `tcsh_number_t i` (also
  noted above as PR3 fix; initial correction made in this session).
- **`sh.sem.c` Dfix gating:** `Dfix()` is skipped for expression-evaluating
  builtins (`doif`, `dowhile`, `dotest`, `dolet`, `doexit`) so operand
  expansion stays lazy and `TEXP_IGNORE` short-circuit works correctly.
- **`sh.exp.c` exp6 TEXP_NOGLOB guard:** The fallback return in `exp6()` already
  reads `ignore & TEXP_NOGLOB ? Strsave(cp) : globone(cp, G_APPEND)`, keeping
  expansion deferred under `TEXP_IGNORE`; confirmed correct, no change needed.
- **`sh.set.c` getn empty string:** The `if (*cp == '\0') return 0;` fast path
  is intentional — it handles ignored expression arms (short-circuited RHS
  of `&&`/`||`). Documented with inline comment; no behavioural change.

---

## Completed work (2026-04-20)

### Phase 5 — Feature enhancements from upstream PRs ✓
- **PR #89 — Interactive comments (`#`):** `#` now acts as a comment character in
  interactive mode. `sh.parse.c` `syn0()` strips comment tokens when `intty`;
  `sh.lex.c` `word()` `case '#':` reads to newline when interactive; `sh.h` adds
  `extern char *pchrs`; `sh.c` sets `pchrs = ";&\n#"` when editing is active,
  `pchrs = ";&\n"` otherwise.
- **PR #107 — Expression short-circuit:** `$?a && "$a" != ""` no longer throws when
  `a` is unset. `sh.sem.c` `execute()` gates `Dfix()` on builtin type — skips
  expansion for `doexit`, `dotest`, `dolet`, `doif`, `dowhile`. `QUOTES` macro
  moved to `sh.h`; `sh.misc.c` gains `blkcmp()`, `blkcmpfree()`,
  `blkcmp_cleanup()`; `sh.decls.h` updated with extern declarations.
  `sh.func.c` `xechoit()` guarded the same way.
- **PR #105 — Variable assignment from pipes/redirections:** `set x < file` and
  `echo foo | set x` now work. `sh.set.c` `doset()` detects pipe/redirect via
  `c->t_dlef || !isatty(OLDSTD)` and reads stdin when active. `sh.func.c`
  `dosetenv()` gains the same pipe-read path for `setenv VAR`.
- **PR #77 — `function` builtin:** Named function definitions are available.
  `sh.func.c` `dofunction()` implements definition and call dispatch;
  `sh.init.c` registers `function` in the builtins table.
- **Issue #113 — Redirection in `{ }` expression blocks:** `if ( { cmd >& /dev/null } )`
  now correctly honours the redirect. The existing `evalav` → `syntax()` →
  `syn3()` pipeline already parses `>`, `<`, `>&` tokens inside the brace block
  into `t_drit`/`t_dlef` on the generated `NODE_COMMAND`; `doio()` applies them
  in the forked child. No separate fix was required — the code path was already
  correct.

### Phase 2 — VMS / Windows / dead platform purge ✓
- `vms.termcap.c`, `termcap.vms`, `system/vms` — deleted.
- All `#ifdef _VMS_POSIX` / `#ifdef __VMS` blocks removed from every `.c`/`.h`.
- All `#ifdef WINNT_NATIVE` / `#ifdef _WIN_NT` blocks removed from every `.c`/`.h` via `unifdef` + Python pass.
- `system/win32`, `system/uwin`, `system/emx` — deleted.
- `Imakefile`, `imake.config` — deleted.
- `system/` pruned to active POSIX platforms only; 50+ defunct entries removed.
- `configure.ac` dead platform branches (ultrix, dgux, hpux7, cray, convex, apollo, SCO, BS2000, tekXD88, sunos3/4, sysV68/88, etc.) removed.
- `Makefile.in` VMS/OS2/dead-platform comment stanzas removed.

### Phase 1 — Branding sweep (deferred items) ✓
- `sh.c`: `tcshstr[]` → `mcshstr[]`; detection logic now recognises `mcsh` and `tcsh` binary names equally; `$SHELL` check extended to `/mcsh`.
- `csh-mode.el`: header updated to include `mcsh`.
- `complete.mcsh` created alongside `complete.tcsh`.

### Phase 3 — Source hygiene ✓
- `tc.alloc.c`: bundled Caltech allocator permanently disabled; `SYSMALLOC` forced at top of file; system allocator always used.
- `tc.vers.c`: `SYSMALLOC`/`SMSTR` option string entry removed.
- `sh.types.h`: 396-line platform typedef thicket collapsed to 60 lines using `<stdint.h>`/`<stddef.h>` as floor; only `ptr_t`, `ioctl_t`, and Minix `caddr_t` remain.
- `sh.h`: hpux ANSI-mode K&R `bfunc_t` workaround removed; clean C99 prototype retained.

### Phase 6 — Build system ✓
- `configure.ac`: `AC_SEARCH_LIBS([crypt], [crypt xcrypt])` — fixes #99 (undefined `crypt` on modern glibc with `libxcrypt`).
- `configure.ac`: `AC_CHECK_FUNC([glob], ...)` probe added for libc `glob(3)`.
- `configure.ac`: note added to prefer autoconf ≥ 2.72.

### Phase 4 — Bug fixes (partial) ✓
- `tc.prompt.c` `%j`: now counts only live job leaders (`p_procid == p_jobid` && `PRUNNING|PSTOPPED`), not all proclist entries. Fixes upstream #110.
- `sh.set.c` `getn()`: rewrote to use `strtoll` with base detection (decimal/octal/hex) and proper overflow/errno checking. Fixes upstream #101 (`@ x = (1 << 63)` overflow). `configure.ac` now probes for `strtoll`; `strtol` fallback used when unavailable.
- `sh.exp.c` `exp3a`: shift operations now use `unsigned long long` arithmetic to avoid signed-integer UB; shift range clamped to `CHAR_BIT * sizeof(tcsh_number_t)` bits. Companion fix for #101.
- `sh.lex.c`: garbled `#endif /* ! && !__CYGWIN__ */` comments corrected to `/* !defined(__CYGWIN__) */`.
- `sh.dir.c`: garbled `#else /* ! */` / `#endif /* */` comments on the Cygwin branch corrected.

---

## Remaining open items

## 1. Identity / branding — deferred cosmetic sweep

Core rebrand is complete. The following are documentation-only items:

- The body of `tcsh.man.in` still contains thousands of descriptive
  `tcsh` references that document shell features inherited from tcsh.
  These should be audited to disambiguate "the shell" (→ `mcsh`/`.Nm`)
  from "the tcsh-compat surface" (keep as `tcsh`).
- NLS catalogues in `nls/` may need regeneration via `catgen` if any
  message strings embed the package name; spot-check done, none found.

## 2. Source-hygiene items still open

- `gethost.c` ships a generated `host.defs` parser rather than using
  `/etc/hosts` or `getaddrinfo(3)`; the generated table is rarely in
  sync with reality on modern systems.
- `glob.c` ships its own globbing rather than using libc `glob(3)` —
  worth delegating to libc where available.
- `ed.screen.c` contains several large `#ifdef` ladders that reference
  obsolete terminal types; prune to curses/terminfo only.
- `tc.os.c` has hundreds of `#ifdef _AIX`, `#ifdef sun`, etc. Many of
  those vendors / variants are gone — audit and simplify.
- The NLS catalogues in `nls/` are machine-generated via `catgen`; check
  that regenerating them still works with modern `gencat`.

## 3. Bugs / warnings already flagged upstream (carry forward)

- Open upstream CVEs against tcsh (none outstanding at time of import);
  track the Astron advisory list so we pull in security fixes.
- Known warning spew with modern GCC (`-Wdeprecated-non-prototype`,
  `-Wimplicit-function-declaration`) in `ed.screen.c`, `tw.parse.c`,
  `sh.exp.c`. Clean up when touching.
- `tests/testsuite.at` needs to be re-audited after the autoconf
  version gets bumped in `configure.ac`; some macros are deprecated
  under autoconf ≥ 2.72.
- `DrawGhost()` in `ed.refresh.c` emits raw ANSI SGR sequences and
  writes directly to the terminal, bypassing the virtual-display model
  (`Display`/`Vdisplay`). This means stale ghost tails can appear on
  wide-character input or terminal resize. Full fix requires integrating
  ghost rendering into the `Refresh()` virtual-display pipeline.

## 4. Scope of this consolidation push

Present on the branch:

- All top-level program source: `sh.*.c/h`, `ed.*.c/h`, `tc.*.c/h`,
  `tw.*.c/h`, `glob.c/h`, `dotlock.c/h`, `mi.*`, `ma.setp.c`,
  `gethost.c`, plus `host.defs`, `pathnames.h`,
  `snames.h`, `config_f.h`, `patchlevel.h.in`.
- Modern autotools build: `configure.ac`, `Makefile.in`, `aclocal.m4`,
  `config.h.in`, `atlocal.in`, `acaux/`, `m4/`, `build/`.
- Support: `tcsh.man.in`, `complete.tcsh`, `complete.mcsh`, `csh-mode.el`,
  `glob.3`, `eight-bit.me`, `dot.login`, `dot.tcshrc`, `dot.mcshrc`,
  `src.desc`.
- Full NLS tree: `nls/` (all catalogues and `Makefile.in`).
- Platform fragments: `system/` (pruned to active POSIX compile-time configs;
  defunct entries removed in Phase 2).

Explicitly deferred / excluded by scope decision:

- **Native Windows support (`win32/`)** — dropped.
- **Test suite (`tests/`)** — deferred to a follow-up commit.
- **Autogenerated `configure` script** — not committed; regenerate
  with `autoreconf -fi` from `configure.ac` + `acaux/` + `m4/`.
