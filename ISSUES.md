# mcsh — Consolidation Issue Log

Running log of bugs, obsolete code, and modernisation tasks noticed while
consolidating `tcsh` and `etcsh` into `mcsh`. Items are notes, not yet all
triaged or prioritised; they are burned down as polishing proceeds.

See `PLAN.md` for the full phased execution plan derived from this log.

---

## Completed work (2026-04-21, round 5 — PR #4 Copilot review fixes)

### PR #4 Copilot inline review comments resolved ✓

- **`sh.file.c` UTF16_STRING typo** — The preprocessor guard in `compare()`
  was `!defined(UTF16_STRING)` (singular) while the codebase defines
  `UTF16_STRINGS` (plural). The typo meant the guard was always true, so
  `wcscoll()` was called even on `UTF16_STRINGS` builds where `Char` is
  `wint_t` (not `wchar_t`), risking UB. Fixed to `!defined(UTF16_STRINGS)`.
  On the surviving `WIDE_STRINGS && !UTF16_STRINGS` branch `Char` is now
  guaranteed to be `wchar_t`, so the cast simplifies to the correct
  `*(const wchar_t *const *)` without the `(void*)` detour.

- **`config_f.h` `UTF16_STRINGS` over-broad condition** — Changing
  `SIZEOF_WCHAR_T < 4` to `<= 4` inadvertently activated `UTF16_STRINGS`
  on all 32-bit `wchar_t` platforms (Linux, macOS, FreeBSD, most POSIX
  systems), flipping `Char` to `wint_t` and routing `Str*` operations
  through UTF-16 surrogate-pair wrappers. `UTF16_STRINGS` is only correct
  on systems where `wchar_t` is genuinely 16-bit (Windows, some embedded
  targets). Fixed to `SIZEOF_WCHAR_T == 2` per Copilot's suggestion.

---

## Completed work (2026-04-21, round 4 — upstream carry-forward sweep)

### Upstream tcsh-org/tcsh bug fixes applied ✓

A full sweep of all open and recently closed issues/PRs in the upstream
[tcsh-org/tcsh](https://github.com/tcsh-org/tcsh) repository was performed.
The following were applied:

- **#116** (`sh.file.c`) — 32-bit `wcscoll` type mismatch: `Char *` is
  `unsigned int *` on i686, but `wcscoll` expects `const wchar_t *`
  (`const long int *` on 32-bit). Guard corrected to `!defined(UTF16_STRINGS)`;
  cast simplified to `*(const wchar_t *const *)` (safe because on that branch
  `Char == wchar_t`).

- **#115** (`config_f.h`) — Shift-JIS `UTF16_STRINGS` condition: changed from
  `< 4` to `== 2` so `UTF16_STRINGS` only fires on genuinely 16-bit `wchar_t`
  targets. The previous `<= 4` was too broad and incorrectly activated the
  UTF-16 code path on all 32-bit POSIX systems.

- **#115** (`sh.h`) — `defined(CODESET)` guard removed from
  `AUTOSET_KANJI`. `CODESET` is an enum constant (not a macro) in some
  NLS environments, so `#if defined(CODESET)` silently evaluates false.
  Fixed by removing the `&& defined(CODESET)` clause; the remaining
  guards (`KANJI`, `WIDE_STRINGS`, `HAVE_NL_LANGINFO`) are sufficient.

Items **already resolved** in mcsh prior to this sweep (confirmed):

- **#103** (`nls/Makefile.in`) — Greek locale uses `el` (correct ISO 639-1)
  not `gr`. Already correct.
- **#104** (`Makefile.in`, `configure.ac`) — Cross-build `*_FOR_BUILD`
  flags for `gethost`. Already applied.
- **#99** (`configure.ac`) — `undefined reference to 'crypt'` on glibc.
  Fixed via `AC_SEARCH_LIBS` (Phase 6).
- **#101** (`sh.exp.c`) — Signed integer overflow in expression evaluation.
  Fixed (Phase 4).
- **#110** (`tc.prompt.c`) — `%j` job-count prompt overcounts. Fixed
  (Phase 4).
- **#98** — History merging. Already in tcsh 6.24.x baseline.
- **#97** — Incremental nice priority. Already in tcsh 6.24.x baseline.

Items **not applied** (rejected upstream or out of scope):

- **#118** (`sh.dol.c`) — FIONREAD-less portable solution. PR was closed
  by upstream maintainer as penalising all platforms for a minority case.
  Not applied.
- **#114** (`sh.lex.c`) — Shift-JIS backslash byte 165: environment-
  specific runtime `strcmp(getenv("LANG"), "ja_JP.SJIS")` approach is
  fragile and was closed without merge upstream. Not applied.

Items **still open upstream and tracked in mcsh** (see "Remaining open items"):

- **#119** — `unshare --user --pid` hang (critical)
- **#93** — `ls-F` colour with `CLICOLOR_FORCE` (low)
- **#102 / #82** — Acute accent lintian; man page pipe workaround (low)
- **#123** — Syntax improvement/alias multi-line (feature request; tracking)
- **#113** — Redirect in `{ }` expression blocks (resolved in mcsh, Phase 5)

---

## Completed work (2026-04-21, round 3 — PR3 CodeRabbit round-2 review fixes)

### Phase 8 (round 3) — CodeRabbit PR3 review fixes ✓

- **`configure.ac` TCSH_BASELINE_VERSION macro expansion:** `AC_DEFINE_UNQUOTED`
  previously passed `["TCSH_VERSION"]` (extra M4 quoting brackets) which emitted
  the literal identifier `TCSH_VERSION` into `config.h` rather than the version
  string. Fixed: value is now `[TCSH_VERSION]` so M4 expands the macro and
  config.h correctly emits `#define TCSH_BASELINE_VERSION "6.24.13"`.

- **`sh.sem.c` Dfix skip reverted:** A reviewer suggestion to skip `Dfix()` for
  expression-evaluating builtins was applied but caused `$?VAR` and other
  variable references inside `if` conditions to never be expanded, producing
  "No match" errors at runtime. Reverted to the original unconditional `Dfix()`
  call; the lazy-evaluation concern is a pre-existing upstream tcsh behaviour
  that requires a deeper refactor outside the scope of this PR.

## Completed work (2026-04-21, round 2 — PR3 final fixes + pushd/popd)

### Phase 8 (round 2) — Copilot review fixes ✓

- **`configure.ac` TCSH_BASELINE_VERSION:** `AC_DEFINE_UNQUOTED` now wraps the
  value as `["TCSH_VERSION"]` (a quoted string literal) so `config.h` emits
  `#define TCSH_BASELINE_VERSION "6.24.13"` — a valid C string — rather than
  the bare identifier `TCSH_VERSION` which would be undefined.

- **`tc.prompt.c` git cache marker-mtime independence:** The previous code
  compared every state-marker file's mtime against `git_head_mtime`, so a live
  `MERGE_HEAD` (whose mtime is unrelated to `.git/HEAD`) always differed and
  forced a full git-info refresh on every single prompt render while in merge
  state. Fixed: HEAD mtime tracked in `git_head_mtime`; max mtime of all
  state-marker files (`MERGE_HEAD`, `CHERRY_PICK_HEAD`, `REBASE_HEAD`,
  `rebase-merge/head-name`) tracked separately in `git_marker_mtime`. Both are
  compared and updated independently.

- **`ed.screen.c` `SetSGRColor` SGR desync:** When `sc->fg == 0` (default
  colour, no bold), the code emitted `ESC[0m` which resets **all** SGR
  attributes (including underline, standout) while `cur_atr` was not cleared,
  causing the editor's attribute tracking to drift. Fixed: emits `ESC[22;39m`
  (cancel bold, reset fg only) and clears the `BOLD` bit in `cur_atr`.

- **`ed.refresh.c` `DrawGhost` SGR desync:** `DrawGhost()` reset with `ESC[0m`
  after writing ghost text. On the `RefPlusOne` incremental path this could
  clobber real attribute state without `cur_atr` being updated. Fixed: emits
  `ESC[22;39m` to undo dim/bold and reset fg only.

- **`ed.inputl.c` double `Refresh()` with `set syntax`:** `CC_NORM` + `set
  syntax` used to promote the return code to `CC_REFRESH`, causing a full
  `Refresh()` after every command that already refreshed internally (e.g.
  `e_insert`). Fixed: calls `syntax_colorize()` directly without altering the
  return value.

  > **Correction (Round 11):** the double refresh was real, but removing the
  > promotion removed the only thing that ever *rendered* the colours. From
  > this change until Round 11, `set syntax` produced no visible highlighting
  > at all while typing — `syntax_colorize()` ran after `e_insert()` had
  > already painted the character, and nothing redrew. Colours only appeared
  > after an unrelated full redraw. Fixed properly in Round 11 below, still at
  > one paint per keystroke.

### Phase 9 (extension) — zsh-style pushd/popd tree navigation ✓

- **`dirs -v` arrow marker:** The current directory (index 0) is now marked
  with `→` in the vertical display, making the current stack position
  immediately visible.

- **pushd/popd default to tree display:** After every `pushd`, `popd`, or
  `cd +N` / `cd -N` navigation, the directory stack is shown in the numbered
  vertical format (equivalent to `dirs -v`) rather than the previous flat
  horizontal output. Explicit format flags (`-p`, `-l`, `-n`) override this.

- **`cd -N` navigation:** Jumps to stack entry N counted from the bottom of
  the stack (oldest entry), mirroring zsh's `cd -N` semantics. A pre-scan in
  `dochngd()` detects numeric `-N` args before `skipargs()` so they are never
  rejected as unknown flags. Existing `cd +N` (forward from current) unchanged.

- **`dfind()` extended:** Now handles both `+N` (from top) and `-N` (from
  bottom) patterns.

---

## Completed work (2026-04-21, round 1)

### Phase 9 — Native interactive syntax highlighting ✓

`set syntax` activates per-keystroke colour highlighting in the interactive
command line editor. The implementation uses full virtual-display pipeline
integration (no raw ESC bypass).

**Architecture — end-to-end:**

1. `syntax_colorize()` (`ed.syntax.c`) runs after every keystroke dispatch in
   `ed.inputl.c`. Single-pass state machine over `InputBuf[0..LastChar)` emits
   a `SynToken` byte into `SyntaxColor[]` for every input character.
2. `Draw(cp, …)` (`ed.refresh.c`) reads `SyntaxColor[cp - InputBuf]` and sets
   the `vcurrent_color` global (defaults to `SYN_NORMAL` for prompt characters).
3. `Vdraw(c, width)` packs `vcurrent_color` into the upper bits of each display
   `Char` via `SYN_PACK(c, vcurrent_color)` and writes the packed value into
   `Vdisplay[v][h]` directly.
4. `update_line()` diffs `Vdisplay` against `Display` per cell; colour-only
   changes are detected automatically because the token is part of the `Char`.
5. `so_write()` (`ed.screen.c`) extracts the token with `SYN_TOK(cell)` and
   the glyph with `SYN_GLYPH(cell)` per character, then calls
   `SetSGRColor(token)` before output and `SetSGRColor(-1)` at line end.
6. `SetSGRColor(int fg)` tracks `cur_sgr` to suppress redundant SGR emissions.
   Emits `ESC[1;{code}m` (bold) or `ESC[{code}m` (colour only) or
   `ESC[22;39m` (reset fg/bold without clobbering other attributes) via
   `putpure()`. Updates `cur_atr` to stay consistent.

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
  `syntax_cache_clear()` declarations; `SYN_PACK`/`SYN_TOK`/`SYN_GLYPH`
  macros for token bit-packing into display `Char` values.
- `ed.syntax.c` — new: tokeniser, LRU command cache (`CMD_CACHE_SIZE=32`),
  `cmd_on_path()` via `stat(2)` + `access(2)` + `$PATH` walk.
- `ed.h` — added `vcurrent_color` external.
- `ed.screen.c` — `SetSGRColor()` emits targeted SGR, tracks `cur_sgr`/`cur_atr`.
- `ed.refresh.c` — `Draw()` sets `vcurrent_color`; `Vdraw()` packs via `SYN_PACK()`.
- `ed.inputl.c` — `syntax_colorize()` called on `CC_NORM` and `CC_REFRESH` paths.
- `sh.set.c` — `update_vars()` calls `syntax_colorize()`/`syntax_clear()` on
  `set`/`unset syntax`; PATH change calls `syntax_cache_clear()`.
- `tc.const.c` / `tc.const.h` — `STRsyntax[]` constant.
- `Makefile.in` — `ed.syntax.${SUF}` added to `EDOBJS`.
- `dot.mcshrc` — `set syntax` added after `set color`.

### Phase 8 (round 1) — Code review fixes (PR3, Gemini + CodeRabbit) ✓

- **`configure.ac` PACKAGE_PATCHLEVEL normalisation:** Changed from
  `printf '%d'` to `sed 's/^0*//; s/^$/0/'` to strip leading zeros without
  invoking numeric parsing — prevents invalid C integer literals like `08`/`09`.
- **`sh.func.c` doif type safety:** `doif()` local variable `i` widened from
  `int` to `tcsh_number_t` so wide expression results are not silently truncated.
- **`vms.termcap.c` octal escapes:** Added `case '4':` through `case '7':`;
  continuation digit validation checks `<= '7'`.
- **`ed.defns.c` NLS catalog collision:** `predict-accept` uses ID 124
  (was 122, colliding with `newline-and-hold`).
- **`ed.chared.c` e_predict_accept:** NUL written after the copy loop; bounds
  checked against `InputLim`.
- **`ed.chared.c` predict_from_history:** Trailing `\n`/`\r` stripped from
  ghost text; INBUFSIZE bound respected.
- **`ed.refresh.c` DrawGhost erase logic:** Erases previous ghost (spaces +
  backspaces) only when `Cursor == LastChar` to avoid overwriting real input.
- **`ed.inputl.c` GhostBuf clear:** `Refresh()` called only when `GhostBuf`
  was non-empty, avoiding spurious redraws.
- **`vms.termcap.c` sscanf + strcmp:** `sscanf` uses `%[^|:]` scanset;
  `strcmp` for exact name match.
- **`vms.termcap.c` fgets overflow:** Continuation loop computes remaining
  capacity and passes it to `fgets`.
- **`vms.termcap.c` tgoto bounds:** Static buffer 64 bytes; `%d` via
  `snprintf`; all write positions bounds-checked.
- **`vms.termcap.c` `case '\\'`:** Corrected from invalid `case '\':`.
- **`vms.termcap.c` sizeof(bp):** Capacity calculation corrected from pointer
  size to 1024 (the actual caller buffer size).
- **`sh.sem.c` Dfix gating:** `Dfix()` skipped for expression-evaluating
  builtins (`doif`, `dowhile`, `dotest`, `dolet`, `doexit`).
- **`m4/lib-prefix.m4`:** `dn;` → `dnl`.
- **`m4/po.m4` DLL cleanup:** Error handler removes the actual DLL target.
- **`m4/po.m4` GETTEXT_MACRO_VERSION:** Updated to 0.23.
- **`acaux/install-sh` name patterns:** Case patterns use `*` suffix.
- **`dch-template.in`:** Distribution `unstable` → `UNRELEASED`.
- **`alacritty.toml`:** `program = "mcsh"` via PATH; pywal import commented out.
- **`dot.mcshrc`:** Home/End bindings corrected; GPU vars gated behind local
  override; interactive-only block guarded by `$?prompt`; `set time` moved to
  end; `VIMINIT` used instead of `VIMINFO`.

---

## Completed work (2026-04-21, phase 7b)

### Native features ✓

- **Fish-style predictive autocomplete:** `predict_from_history()` in
  `ed.chared.c` scans `Histlist` for a prefix match and fills `GhostBuf`;
  `DrawGhost()` in `ed.refresh.c` renders the ghost text dimmed after the
  cursor. `e_predict_accept` (Right-Arrow) copies `GhostBuf` into the input
  buffer.
- **Native git branch prompt escapes `%g` / `%G`:** `git_get_info()` in
  `tc.prompt.c` walks upward from `$cwd` looking for `.git/HEAD`; cached
  per-CWD pointer. `%g` = branch name; `%G` = branch + operation state.
- **`dot.mcshrc` rewrite:** Reference start-up file fully rewritten with
  interactive guard, `set syntax`, `set color`, `rprompt='%S%G%s'`, full
  keybinding set, programmable completions, alias block, `set time` coloured
  format, root guard, and local-override sourcing.

---

## Completed work (2026-04-20)

### Phase 5 — Feature enhancements from upstream PRs ✓

- **PR #89 — Interactive comments (`#`):** `#` now acts as a comment character
  in interactive mode.
- **PR #107 — Expression short-circuit:** `$?a && "$a" != ""` no longer throws
  when `a` is unset.
- **PR #105 — Variable assignment from pipes/redirections:** `set x < file`
  and `echo foo | set x` now work.
- **PR #77 — `function` builtin:** Named function definitions available.
- **Issue #113 — Redirection in `{ }` expression blocks:** Works correctly;
  code path was already correct, confirmed by audit.

### Phase 2 — VMS / Windows / dead platform purge ✓

- All Windows (`#ifdef WINNT_NATIVE`) and VMS (`#ifdef __VMS`) blocks removed.
- `vms.termcap.c` retained and repurposed as a portable POSIX termcap shim.
- `system/` pruned to active POSIX platforms; 50+ defunct entries removed.
- `configure.ac` dead platform branches removed.

### Phase 1 — Branding sweep ✓

- `sh.c`: `tcshstr[]` → `mcshstr[]`; `$mcsh` and `$tcsh` both set.
- `complete.mcsh` created alongside `complete.tcsh`.

### Phase 3 — Source hygiene ✓

- `tc.alloc.c`: bundled allocator disabled; system allocator always used.
- `sh.types.h`: collapsed to 60 lines using `<stdint.h>`/`<stddef.h>`.

### Phase 6 — Build system ✓

- `configure.ac`: `AC_SEARCH_LIBS([crypt], …)`, `AC_CHECK_FUNC([glob], …)`.

### Phase 4 — Bug fixes (partial) ✓

- `%j` prompt, `getn()` overflow, shift UB, `sh.lex.c` comment garbling fixed.

---

## Remaining open items

### 1. Identity / branding — deferred cosmetic sweep

- `tcsh.man.in` body text: "the shell" references should become `.Nm`/`mcsh`;
  tcsh-compat-surface references should stay `tcsh`. Also needs new sections
  for `set syntax`, `%g`/`%G`, `cd -N`, zsh-style `pushd`/`popd` display.
- NLS catalogues: spot-check for package-name embeds; regeneration via `catgen`
  not yet validated with modern `gencat`.

### 2. Source-hygiene items still open

- `gethost.c` ships a generated `host.defs` parser rather than `getaddrinfo(3)`.
- `glob.c` ships its own globbing; should delegate to libc `glob(3)` where available.
- `ed.screen.c` still has large `#ifdef` ladders for obsolete terminal types.
- `tc.os.c` has dead `#ifdef _AIX`, `#ifdef sun`, etc. vendor blocks.
- NLS catalogues in `nls/`: check that `catgen` + `gencat` still work cleanly.

### 3. Known bugs / upstream carry-forwards

- **#119** (`sh.proc.c`) — `unshare --user --pid` hang. Fork retry loop sleeps
  with interrupts disabled. Fix: use `SIGALRM`-based timeout or `nanosleep`
  with signal unblocking.
- **#93** (`tw.color.c`) — `ls-F` colour failures with `CLICOLOR_FORCE`,
  `LSCOLORS`, `LS_COLORS`. Audit colour detection and environment-variable
  precedence.
- **#102 / #82** (`tcsh.man.in`) — Acute accent lintian warning; pipe
  workaround missing from man page.
- **#123** (feature request) — Alias/function multi-line definition: user
  requests third-quote type or here-doc alias support for multi-line complex
  aliases. Tracked for Phase 5 follow-up.
- **`DrawGhost()`** — still writes directly to the terminal, bypassing the
  `Display`/`Vdisplay` virtual-display model. Stale ghost tails can appear on
  wide-character input or terminal resize. Full fix: integrate ghost rendering
  into the `Refresh()` pipeline.

### 5. Scope of this consolidation push

Present on the branch:

- All top-level program source: `sh.*.c/h`, `ed.*.c/h`, `tc.*.c/h`,
  `tw.*.c/h`, `glob.c/h`, `dotlock.c/h`, `mi.*`, `ma.setp.c`, `gethost.c`,
  plus `host.defs`, `pathnames.h`, `snames.h`, `config_f.h`, `patchlevel.h.in`.
- `ed.syntax.c`, `ed.syntax.h` — new native syntax highlighting engine.
- Modern autotools build: `configure.ac`, `Makefile.in`, `aclocal.m4`,
  `config.h.in`, `atlocal.in`, `acaux/`, `m4/`.
- Support: `tcsh.man.in`, `complete.tcsh`, `complete.mcsh`, `csh-mode.el`,
  `glob.3`, `eight-bit.me`, `dot.login`, `dot.tcshrc`, `dot.mcshrc`,
  `src.desc`.
- Full NLS tree: `nls/` (all catalogues and `Makefile.in`).
- Platform fragments: `system/` (pruned to active POSIX configs).

Explicitly deferred / excluded:

- **Native Windows support** — dropped.
- **Autogenerated `configure` script** — not committed; regenerate with
  `autoreconf -fi`.

---

## Round 6 — Review response: three flagged weaknesses (dev4) (2026-04-21)

Addresses all findings from the deep-dive analysis (paste_1 / paste_2) and
Gemini PR #5 inline comments.

### 1. Short-circuit evaluation (`sh.dol.c`)

**Root cause (confirmed):** `Dfix()` in `sh.sem.c` expands all `$` tokens
*before* `doif` calls `expr()`. The expression evaluator in `sh.exp.c` already
implements correct `TEXP_IGNORE` short-circuit at the `exp0`/`exp1` level, but
`Dfix` runs unconditionally before evaluation, so `"$a"` in
`if ($?a && "$a" != "")` threw `ERR_UNDVAR` before `&&` could suppress it.

**Fix (`sh.dol.c:643`):** In `Dgetdol()`, when a variable is unset and not
found in the environment, instead of calling `udvar()` (which throws
`ERR_UNDVAR`), set `dolp = STRNULL` and jump to `eatbrac`. This makes unset
`$varname` silently expand to `""` — matching bash/zsh double-quote semantics
— so the expression evaluator receives `"" != ""` (false) rather than dying.
`$?varname` continues to work correctly via the existing `bitset` path.

**Test:** `t003_shortcircuit.sh` — `unset a; if ($?a && "$a" != "") echo yes`
must produce no output and exit 0.

### 2. Unicode regression (`sh.lex.c`, `sh.dol.c`) — fixed

**Scope:** Inherited from tcsh 6.24.14. Byte-vs-character length confusion in
the wide-string expansion path caused multi-byte characters (emoji, CJK, Latin
Extended) to be dropped or corrupted during filename glob expansion and variable
assignment. Affected any locale where `MB_CUR_MAX > 1`.

**Affected upstream issues:** tcsh #117, #121.

**Root cause:** Two `mbtowc` accumulation loops compared the partial-byte count
against `MB_LEN_MAX` (compile-time worst case across all locales, 16 on glibc)
instead of `MB_CUR_MAX` (runtime maximum for the current locale, 4 for UTF-8).
When `mbtowc` returned `-1` for a stray invalid byte, the loop continued reading
up to 15 additional bytes of lookahead before giving up, swallowing valid
multi-byte sequences that immediately followed.

**Fix (two-line change):**
- `sh.lex.c` `wide_read()` — `(partial - i) < MB_LEN_MAX` → `(partial - i) <
  (size_t)MB_CUR_MAX`. Covers script-file reads, stdin pipes, and backquote
  command substitution.
- `sh.dol.c` `Dgetdol()` `$<` accumulation loop — `cbp < MB_LEN_MAX` → `cbp <
  (size_t)MB_CUR_MAX`. Covers the `$<` line-read primitive.

The corrected pattern matches the existing reference implementation at
`ed.inputl.c:814`. Buffer declarations (`char cbuf[MB_LEN_MAX]`) are unchanged
because they must size for the worst case across all platforms.

**Tests:** `tests/t009_unicode_vars.sh` through `tests/t014_unicode_script_source.sh`
cover variable round-trip, `$%` character count, glob expansion, `$<` stdin read,
backquote substitution, invalid-byte recovery, and sourced-script Unicode.

### 3. Test suite — initial suite created (`tests/`)

`tests/` directory created with 8 regression scripts and a `Makefile`:

| Script | What it tests |
|--------|---------------|
| `t001_vars.sh` | `$mcsh` and `$tcsh` are set on startup |
| `t002_overflow.sh` | `@ x = (1 << 31)` yields `2147483648` (unsigned left-shift) |
| `t003_shortcircuit.sh` | `$?a && "$a" != ""` is silent when `$a` unset |
| `t004_pipe_to_var.sh` | `echo foo \| set x` assigns `x=foo` |
| `t005_cd_stack.sh` | `pushd`/`cd -1` navigates directory stack correctly |
| `t006_function_builtin.sh` | `function` builtin stores and executes body |
| `t007_arith_rsh.sh` | `@ x = (-8 >> 1)` yields `-4` (signed right-shift) |
| `t008_unset_modifiers.sh` | `${unset:h}` and `$#unset` don't error when var is unset |

Run with: `make -C tests MCSH=./mcsh check`

### 4. Gemini PR #5 inline comment — `cache_store()` goto removed (`ed.syntax.c`)

Rewrote `cache_store()` with two explicit loops: first pass finds an empty
slot; second pass (only if needed) scans for the LRU victim. No `goto`. Logic
and semantics are identical; the victim variable is initialised to `-1` so the
first pass's early-break is the only way it gets set to a valid index before
the second pass.

### 5. Gemini PR #5 inline comment — magic number `2` replaced (`tc.prompt.c`)

Added `#define GIT_POLL_INTERVAL 2` near the top of the file and replaced the
literal `2` in the throttle check with the named constant.

> **Correction (Round 10):** only the first half of this actually landed. The
> `#define` was added, but the throttle check kept its hard-coded `int
> poll_interval = 2;` — the constant was dead code for the entire life of this
> entry. Genuinely fixed in Round 10 below, along with validation of the
> `$GIT_POLL_INTERVAL` environment override.

---

## Round 7 — PR #5 Copilot + Gemini review response (2026-04-21)

### 1. `sh.dol.c` — unset variable modifier handling fixed

**Copilot + Gemini finding:** The unset-variable expansion path jumped directly to
`eatbrac` without calling `fixDolMod()`, causing `${unset:h}` and similar
modifier expressions to crash with "Missing }" because the `:h` was left in
the input stream. Also, `$#unset` (dimen) and `$%unset` (length) did not return
a sensible value.

**Fix:** Call `fixDolMod()` before branching to `eatbrac`, consume modifiers
properly, and return `0` for both `$#unset` and `$%unset` (consistent with
treating an unset variable as empty/zero-length).  The comment now correctly
states this applies to all variable expansions, not only double-quoted ones.

**Test:** `t008_unset_modifiers.sh` — `${unset:h}` must not error; `$#unset` must yield `0`.

### 2. `tests/run_tests.sh` — portability hardening

**Copilot findings:**
- Header comment incorrectly stated scripts "print PASS or FAIL"; they actually
  exit 0/non-zero with optional failure output.
- Glob `t*.sh` could iterate the literal pattern on a `/bin/sh` with no
  matching files; now guarded with `set -- t*.sh; [ -e "$1" ] || exit`.
- `echo "$result"` with arbitrary content is non-portable (leading `-n` or
  backslash sequences); replaced with `printf '%s\n' "$result"` throughout.

### 3. `tests/t006_function_builtin.sh` — mktemp portability

**Copilot finding:** `mktemp /tmp/t006.XXXXXX.csh` fails on BSD/macOS because
`mktemp` requires the template to end with X characters (suffixes after the X
block are rejected). Removed `.csh` suffix — the shell interpreter is set by
the heredoc content, not the filename.

### 4. `tests/Makefile` — was already present

The `tests/Makefile` was created in Round 6 and supports `make check` and
`make MCSH=/path/to/mcsh check`. All documentation references to
`tests/run_tests.sh` are therefore accurate.

### 5. `ed.syntax.c` — syntax highlighting improvements

- Fixed `in_table()`: removed unused loop variable `i` (loop is pointer-based).
- Fixed `ST_VARIABLE` state machine: `$$`, `$!`, `$<` are single-character
  special variables and now correctly transition to `ST_NORMAL` after being
  coloured.  Previously the redundant inner check re-tested `buf[i]` (same as
  `ch`) causing `$$` to sometimes stay in variable state.
- Extended redirection operator colouring to cover `>!`, `>>!`, `>|`, `>>&`
  (noclobber-override and append-noclobber forms).

### 6. `ed.chared.c` — command- and file-aware predictive autocomplete

`predict_from_history()` now falls through to two additional predictors when
no history match is found:

- **`predict_file()`** — fires when the current word starts with `/`, `./`, or
  `~/`.  Splits the word into directory + basename prefix, does a single
  `opendir()` scan, and sets GhostBuf to the unique suffix.  Directories get a
  trailing `/`.  Ambiguous or no matches produce no ghost.
- **`predict_cmd()`** — fires when the word is at the command position in the
  input line (no prior non-space characters, or immediately after `;`/`|`/`&`).
  Scans all `$PATH` directories for a uniquely matching executable and sets
  GhostBuf to the suffix.

Priority: history > file path > command name.  Existing Tab completion is
unchanged; the new predictors are ghost-text only (accept with right-arrow /
Ctrl-F).

### 7. `tests/t008_unset_modifiers.sh` — new regression test

Covers the `${unset:h}` modifier fix and `$#unset` == 0 behaviour.

---

## Round 9 — PR #5 review response part 2 (Apr 2026)

### 1. `ed.syntax.c` — redirection coloring tightened

**CodeRabbit finding:** Redirection continuation loop was over-broad for `<`.
Fixed to only allow `!` and `|` when the opening operator is `>`. Both still
accept `&`, `-`, `>`, and `<`.

### 2. `ed.chared.c` — predictive completion enhancements

- **Caching:** Added caching for `predict_file()` and `predict_cmd()` to avoid
  redundant filesystem scans on every keystroke. `f_cache` tracks directory
  mtime; `c_cache` tracks the `$PATH` string.
- **User Toggle:** All predictive logic gated behind `set predict` shell
  variable.
- **`~user` Expansion:** `predict_file()` now supports `~user/` expansion via
  `getpwnam()`.
- **Empty PATH components:** `predict_cmd()` now treats empty components in
  `$PATH` as the current directory (`.`), matching `cmd_on_path()`.

### 3. Test suite hardening

- **`tests/run_tests.sh`:** Now recognizes exit code `77` as `SKIP` and reports
  it in the summary. Fixed a literal newline in an error message.
- **`tests/lib_locale.sh`:** Updated to use portable ERE (`grep -E`) and exit
  code `77` for skips.
- **`tests/t006_function_builtin.sh`:** Added cleanup `trap` and exit status
  verification.
- **`tests/t008_unset_modifiers.sh`:** Escaped `$` in failure message and
  switched to portable `grep -E`.

---

## Round 10 — git prompt correctness pass (2026-08-15)

An inspection of the `%g` / `%G` implementation, driven by a pty harness
against real repositories, found that most of the feature was inert outside a
repository root. Six defects, all in `tc.prompt.c` unless noted.

### 1. Cache staleness watched the wrong directory *(critical)*

`git_get_info()` walks **up** from `$cwd` to find the repository, but the
staleness check built its `stat()` paths as `$cwd/.git/HEAD` and
`$cwd/.git/MERGE_HEAD` — always relative to the *current* directory.

In any subdirectory that path does not exist, so `stat()` failed,
`git_head_mtime` stayed `0`, the stored value was also `0`, and `need_refresh`
was never set. The branch name froze at whatever it was when the directory was
entered and never updated again. The same root cause broke linked worktrees
**even at their root**, because there `.git` is a file and `$cwd/.git/HEAD` is
never a valid path.

Reproduced before the fix — the branch was switched between samples:

```
[REPO ROOT]     ['brand_new_branch', ...]   correct
[SUBDIRECTORY]  ['main', 'main', 'main']    stuck
WORKTREE ROOT   ['wtbranch', ...]           stuck
```

**Fix:** `git_get_info()` already resolves the real git directory — it was
discarding it. It now reports it through a `gitdirout` parameter, and the new
`git_stat_mtimes()` helper watches *that* directory. Subdirectories, linked
worktrees, submodules and bare repositories are all fixed by the same change.

### 2. Staleness watch list did not cover every reported state

The marker list omitted `REVERT_HEAD` and `BISECT_LOG`, so entering or leaving
a revert or a bisect changed no watched mtime and went unnoticed whenever
`HEAD` itself did not change. The list now has one entry per state
`git_get_info()` can report, plus the `rebase-merge` and `rebase-apply`
directories, since a state can begin or end without any watched *file*'s mtime
changing.

### 3. Double `fclose()` on the linked-worktree path

When `.git` was a file, `gf` was closed after parsing the `gitdir:` line, but
control then fell through to a second `fclose(gf)` if the resolved path
exceeded `MAXPATHLEN`. The `if (gf)` guard tested a pointer that was never
cleared. Restructured so the handle is closed exactly once on every path.

### 4. Detached HEAD printed the full 40-character object name

`xsnprintf(branch, branchsz, "%.7s", path)` did not truncate. The cause is in
`tc.printf.c`: at the flags stage a `.` immediately following `%` is consumed
as a zero-pad flag, so the `7` is then parsed as a *field width* and the
precision branch never sees its `.`. `"%.7s"` silently means `"%07s"` — pad to
seven, never truncate.

Truncation is now explicit via `memcpy()` and a new `GIT_SHORT_SHA_LEN`
constant, with a comment recording the `xsnprintf()` limitation.

**The underlying `tc.printf.c` defect is left in place deliberately** — it
affects every format string in the shell and warrants its own change. An audit
of the current uses found no other live victim: `tw.color.c`'s `"%.2d"` is
correct by coincidence (`%.2d` and `%02d` agree for integers) and
`sh.func.c`'s `"%-13.13s"` is correct because its `.` does not directly follow
the `%`.

### 5. `GIT_POLL_INTERVAL` was dead, unvalidated and mis-throttled

Three separate problems: the `#define` added in Round 6 was never actually
used (see the correction on that entry); the `$GIT_POLL_INTERVAL` environment
override was parsed with unchecked `atoi()`, so any typo silently meant `0`
("poll on every prompt"); and `git_last_stattime` was never set on the refresh
path, so the first staleness poll always fired regardless of the configured
interval.

Parsing now goes through `git_poll_interval()` using `strtol()` with full
`errno`, trailing-garbage and range checking, falling back to the compiled-in
default on anything malformed. The throttle window is restarted on refresh.
Verified with `GIT_POLL_INTERVAL=10`: the prompt holds the cached branch at
+0.5s and +3s after a branch switch, and updates at +12s.

### 6. Cache key was a pointer comparison

`git_oldcwd != gcwd` compared a stored `Char *` against the variable table's
current pointer, and held a pointer that `cd` frees. It behaved correctly in
testing, but depended on the allocator never handing back a recycled block
with different contents. `git_oldcwd` is now a `char` buffer compared with
`strcmp()`, which removes the dangling-pointer class of bug outright.

### Verification

All states exercised from a **subdirectory**, which none of them reached
before:

```
clean               %G = 'main'
during merge        %G = 'main|MERGING'
during cherry-pick  %G = 'main|CHERRY-PICKING'
during revert       %G = 'main|REVERTING'
during bisect       %G = 'main|BISECTING'
during rebase -i    %G = 'other|REBASING-i'
detached HEAD       %G = 'f372482|DETACHED'
```

Each returns to the plain branch name after the corresponding `--abort` /
`reset`. Worktree branch switches now track, and `%g` remains empty outside a
repository.

### Also in this round

- **`%G` reports `DETACHED`.** A detached `HEAD` previously showed a bare
  object name, indistinguishable from a branch literally named `f372482`. It
  is reported only when no more specific operation is in progress, since
  rebase and bisect both detach.
- **`tests/t015_dotmcshrc_ls_colors.sh` fixed.** It asserted on
  `echo $CLICOLOR:$LSCOLORS`. In csh a `:` directly after a variable name
  introduces a modifier (`:h`, `:t`, …), so this is a syntax error — correct
  csh behaviour, not a shell bug. Confirmed against a plain `set a=1; set b=2;
  echo $a:$b` with no rc file loaded. Switched to the brace-delimited form.
  The suite had been red on this, which is why it was not caught earlier;
  it is now **17 passed, 0 failed**.
- **`sh.h`** — `CHAR_EOF` was a plain `(-2)` compared against `eChar`, which is
  unsigned `wint_t` in the wide-character build. Now cast to `eChar`.
- **`ed.chared.c`** — reindented a block where an unguarded statement was
  indented as though it were guarded by the preceding `if`. Logic unchanged.
- **Warnings.** The tree now builds clean under `-Wall -Wextra` (0 warnings
  across `sh.*.c`, `tc.*.c`, `ed.*.c`, `tw.*.c`, `glob.c`, `dotlock.c`). Note
  that the default build does **not** pass `-Wall`, so this is not yet
  enforced by anything.
- **Repository hygiene.** Removed five tracked files that should never have
  been committed: `test2` and `test3` (ELF binaries), `tc.prompt.c.orig` (a
  stale copy of a file under active development — a hazard for `grep`/`sed`
  sweeps), `fix_truncation.patch` (a stale fragment against code this round
  replaced; its intent, truncation checking in the marker loop, is preserved
  in `git_stat_mtimes()`), and `strncpy_analysis.md` (scratch analysis).
  `.gitignore` gained `*.orig`, `*.rej` and `test[0-9]` to prevent recurrence.
- **Documentation.** `%g` / `%G` documented properly in `tcsh.man.in`
  (including every reported state and `$GIT_POLL_INTERVAL`), and `README.md`
  and `dot.mcshrc` brought in line.

### Known remaining gaps

Not addressed in this round, and not regressions:

- `tc.printf.c` precision parsing (item 4) — the general fix.
- No dirty/staged indicator, ahead/behind counts, or stash indicator.
- No way to disable the feature; the `stat()` traffic happens whether or not
  the configured prompt actually uses `%g` or `%G`.
- No automated test coverage for the git escapes. The pty harness used to
  verify this round lives outside the tree; the suite is still shell-level
  only.

---

## Round 11 — highlighting render pipeline + git staleness exactness (2026-08-17)

A pty-driven investigation of why interactive highlighting felt far less
capable than comparable shells. The engine turned out to be sound; almost
nothing it produced was reaching the screen.

### 1. Syntax colours were computed but never drawn while typing *(critical)*

Measured against the built binary — same buffer, the only difference being a
forced redraw:

```
after typing 'if ls notarealcmd "str" $HOME # note'  ->  no colour at all
same line, then ^L                                   ->  'if' bold cyan,
                                                         '"str"' yellow,
                                                         '$HOME' magenta,
                                                         '# note' grey
```

`e_insert()` (`ed.chared.c`) paints a single inserted character through
`RefPlusOne()` and returns `CC_NORM`. Only afterwards did `Inputl()` call
`syntax_colorize()`, and nothing redrew. The character was therefore painted
*before* its colour was known, and the freshly computed `SyntaxColor[]` sat
unread until an unrelated full redraw — `^L`, history recall, completion,
resize — happened to repaint the line.

This was introduced deliberately as an optimisation (Round 2, listed in
`README.md` as "eliminating the double `Refresh()` per keystroke"). The
double refresh was real, but removing the promotion removed the only thing
that rendered the colours.

**Fix:** the `CC_NORM` path now colourises *and* repaints, and `e_insert()`
skips its one-character fast path while `set syntax` is active. That path
draws raw and cannot recolour characters already on screen — which a single
keystroke routinely requires, since typing a quote opens a string and typing
a final letter completes a command name. There is still exactly one paint per
keystroke, so the original optimisation's intent is preserved.

### 2. Highlighting was silently dead on modern terminals

```
TERM=xterm-256color  YES     TERM=alacritty    no  <-- silently dead
TERM=screen          YES     TERM=xterm-kitty  no  <-- silently dead
TERM=linux           YES     TERM=foot         no  <-- silently dead
```

Round 8 gated all SGR emission on `T_CanColor`, derived solely from the
termcap `Co` capability. A *missing* terminfo entry is not evidence of a
monochrome terminal, though: entries for alacritty, kitty, foot and wezterm
are routinely absent in minimal containers, on servers, and over `ssh` to
older hosts. Those users lost highlighting entirely, with no diagnostic.

**Fix:** `TermCanColor()` in `ed.screen.c` keeps `Co` as authoritative when
present, then falls back to a non-empty `$COLORTERM`, a `color` substring in
`$TERM`, and a list of known colour-capable emulator names. Genuinely
monochrome terminals still resolve to no colour:

```
alacritty YES   xterm-kitty YES   foot YES   wezterm YES
vt100 no        dumb no           vt100 + COLORTERM=truecolor YES
```

`settc Co <n>` still overrides explicitly, so the capability can be forced
either way, and `echotc color` still reports the result.

### 3. Aliases and shell functions rendered as "command not found"

The classifier consulted keywords, builtins and `$PATH` — but neither
aliases nor functions, both of which shadow `$PATH`. Verified against the
shipped `dot.mcshrc`, which enables `set syntax` **and** defines 15 aliases:

```
alias 'll' -> BOLD RED "command not found"      alias 'pd' -> BOLD RED
alias 'g'  -> BOLD RED                          alias 'cclean' -> BOLD RED
alias '..' -> BOLD RED
```

The default configuration marked every one of its own aliases as broken.

**Fix:** `classify_command()` in `ed.syntax.c` now resolves a command-position
word the way the shell actually would — keywords, builtins, functions,
aliases, then `$PATH` — using read-only `adrof1()` lookups against the
existing `functions` and `aliases` tables. Two tokens were added,
`SYN_ALIAS` (bold blue) and `SYN_FUNCTION` (bold magenta), so the three
kinds stay distinguishable rather than collapsing into "builtin".

Functions are probed **before** aliases: declaring a function also installs
an alias shim (`name -> (function name !*)`) that dispatches to it, so an
alias lookup alone reported every function as a plain alias.

The change also collapsed three duplicated copies of the classification
ladder into the single helper.

### 4. Git HEAD staleness was compared at one-second granularity

A defect in Round 10's own work. `st_mtime` counts whole seconds, so two
HEAD writes inside the same second left the prompt permanently stale.
Demonstrated by forcing the collision:

```
cached on 'main', HEAD mtime 1787007041
switched to 'raceb', mtime forced back to 1787007041
  after 2s:  %g = 'main'   (actual: raceb)     <-- before
  after 2s:  %g = 'raceb'  (actual: raceb)     <-- after
```

Real triggers: scripted `git checkout a && git checkout b`, TUI git clients
(lazygit, tig, magit), and rebase stepping through commits.

**Fix:** `git_stat_mtimes()` became `git_read_state()`, which compares the
literal contents of `HEAD` instead of its mtime. `HEAD` is a ~41 byte file,
so the read costs about what the `stat()` did and is exact. The operation
markers stay on mtime — they are only probed for existence, and a same-second
create/delete pair still moves `HEAD` or the branch name.

### Also in this round

- **`README.md`** documented the `function` builtin as
  `function name { body }`. That form does not work — it fails with
  "Undeclared function". The working syntax is `function name`, the body,
  terminated by `return`. Corrected, and the alias-shim behaviour noted.
- **`tests/t017`** now runs every invocation with `COLORTERM` explicitly
  unset. Without that it would pass or fail depending on which terminal the
  developer happened to run it from, now that `COLORTERM` feeds the
  capability fallback.
- Man page, `README.md` and `dot.mcshrc` updated for the new tokens, the
  classification order, the colour-capability fallback and the HEAD
  comparison change.

Suite: 17 passed, 0 failed. Zero warnings under `-Wall -Wextra`.

### Known remaining gaps

Unchanged from Round 10 and still open, in rough priority order:

- **Arguments are not highlighted at all** — flags, existing vs non-existent
  paths, and globs all render plain. This is the largest remaining coverage
  gap and the one most visible next to other shells.
- The second command after `sudo`, `env`, `nohup`, `time` or `xargs` is not
  classified; only the first word of a pipeline segment is.
- No highlighting for assignments (`=`), history references (`!!`, `!$`),
  `~` expansion, `$argv[1]` subscripts or `$x:h` modifiers; set and unset
  variables look identical.
- `cmd_on_path()` re-implements `$PATH` search rather than reusing the
  shell's own hash table (`xhash`), so it can disagree with what would
  actually run; and `wordbuf[wi] = (char)(buf[...] & CHAR)` truncates a wide
  character to its low byte, so non-ASCII command names are looked up
  corrupted.
- `SYN_MASK` (`0xF0000000`) is numerically identical to `INVALID_BYTE` and
  contains `QUOTE`. `ed.syntax.h` documents the `QUOTE` assumption but not
  `INVALID_BYTE`, which `GetNextChar()` produces. No input reaching the
  display could be made to carry it — the input layer rejects those bytes
  first — so this is undocumented fragility rather than a live bug.
- Every `so_write()` chunk brackets its output with `ESC[22;39m` ... `ESC[0m`,
  a full reset, including around the prompt; the second cell of a
  double-width character emits a spurious reset of its own.
- Git still reports identity (branch, state) rather than status: no dirty
  flag, staged/unstaged counts, untracked indicator, ahead/behind, stash
  count or last-commit age. Measured costs for the design decision:
  C `stat()` over 536 tracked files 0.52 ms; `git status --porcelain`
  fork+exec 5.8 ms. Both are affordable behind the existing 2 s cache.

---

## Round 12 — full-line highlighting and repository status (2026-08-18)

Closes the two largest gaps identified in Round 11: highlighting stopped at
the command word, and the git escapes reported identity rather than status.

### 1. Arguments, wrappers and expansions are highlighted

Previously everything after the command word rendered plain. Now:

```
ls -laF /etc/passwd /nope/zz *.c
  'ls'           cmd-ok        '-laF'        OPTION
  '/etc/passwd'  PATH          ' /nope/zz '  plain
  '*.c'          glob

sudo ls -l /etc      -> 'sudo' cmd-ok, 'ls' cmd-ok, '-l' OPTION, '/etc' PATH
sudo -E ls           -> 'sudo' cmd-ok, '-E' OPTION, 'ls' cmd-ok
set x = 5            -> 'set' BUILTIN, '=' operator
echo !! !$ != 3      -> '!!' '!$' expansions, '!=' operator
echo $argv[1] $HOME:h-> both coloured as complete variable references
```

Design notes:

- **Conservative by construction.** A word is coloured only when it is
  unambiguously an option, a glob, or a name that exists on disk, and only
  words that look like filenames (carrying a `/` or a leading `~`) are
  probed. A non-existent path stays plain rather than being flagged: the
  shell cannot know whether an argument was meant to be a filename.
- **Wrapper commands** (`sudo`, `doas`, `env`, `nohup`, `nice`, `time`,
  `command`, `exec`, `xargs`, …) keep the following word in command
  position. Only the genuine head of a pipeline segment may render as
  "command not found" — after a wrapper the parse is a guess (`sudo -u root
  ls` puts `root` in command position), so an unresolved word there is left
  plain rather than shown as an error.
- **`=` is only an operator outside a word**, so `set x = 5` highlights
  while `--opt=value` stays a single argument.
- **`!` disambiguated**: `!=` is the inequality operator, everything else in
  the `!!` / `!$` / `!n` / `!string` family is an expansion.

Two tokens were added, `SYN_OPTION` and `SYN_PATH`, which fills the 4-bit
token field exactly (16/16). The remaining categories reuse existing tokens
rather than demanding a wider field.

**Because the field is now full there is no spare value for a range clamp to
catch**, so the invariant that `QUOTE` (`0x80000000`) and `INVALID_BYTE`
(`0xF0000000`, numerically identical to `SYN_MASK`) must never reach the
display is now written down in `ed.syntax.h`, along with why it currently
holds and what would break it.

### 2. Two long-standing lookup bugs, found while refactoring

The three duplicated word-flush sites collapsed into one `flush_word()`,
which exposed both:

- **Wide characters were truncated.** `wordbuf[wi] = (char)(buf[i] & CHAR)`
  narrowed a wide character to its low byte, so any command or path
  containing a non-ASCII character was looked up under a corrupted name and
  always classified as not found. Now encoded with `one_wctomb()`.
- **Relative paths containing a slash never resolved.** `cmd_on_path()`
  treated only a leading `/` or `.` as a direct file reference, so
  `build/tool` was appended to each `$PATH` entry and never found. Anything
  carrying a `/` is now checked directly. Verified: `sub/prog` in the cwd
  now classifies as a valid command.

### 3. Command cache was not invalidated on `cd`

The cache keys on the bare word, so entries for relative names
(`./configure`, `build/tool`) are only valid in the directory they were
resolved in. Only a `$path` change cleared it; `dnewcwd()` now does too.

### 4. SGR emission tightened

`SYN_NORMAL` mapped to palette entry 0, which emitted `ESC[22;39m`. Every
uncoloured run — the prompt, plain arguments — was therefore bracketed by a
needless set/reset pair on each write. It now maps to "no colour", so plain
text emits nothing:

```
prompt redraw, syntax OFF: ESC[1;32mu@h ESC[0m:[ ESC[1;31m0 ESC[0m] #
prompt redraw, syntax ON : ESC[1;32mu@h ESC[0m:[ ESC[1;31m0 ESC[0m] #
```

Byte-identical. The trailing cell of a double-width character is also
skipped rather than asked for its colour, which used to reset mid-character.

### 5. Repository status: `%v` and `%V`

`%g`/`%G` report identity. `%v` reports state, `%V` is both:

```
clean                          main
1 modified tracked file        main *1
2 modified                     main *2
  (same from a subdirectory)   main *2
stashed, tree clean            main $1
1 local commit not pushed      main ^
after push                     main
after `git add`                main          (staged is not reported - see below)
tracked file deleted           main *1
during a merge conflict        main|MERGING !1 ^
after merge --abort            main ^
```

Indicators: `*n` modified tracked files, `!n` unmerged paths, `$n` stash
entries, `^` local commits the upstream does not have.

**How, without spawning git.** `.git/index` is parsed (versions 2 and 3) and
each entry compared against an `lstat()` of the working-tree file — the same
stat comparison git's own fast path makes, using the stat data the index
already caches. Conflicts come from the index stage bits, stashes from the
stash reflog line count, and upstream divergence from `branch.<name>.remote`
in `config` plus a ref comparison that honours `packed-refs`.

**What is deliberately not reported**, because it cannot be derived without
walking the object store or evaluating `.gitignore`: changes staged relative
to `HEAD`, untracked files, and ahead/behind *counts*. These are left absent
rather than approximated — a status indicator that is sometimes wrong is
worse than one that is missing. Reporting them accurately means either
implementing zlib/packfile reading or spawning `git status --porcelain`
(measured 5.8 ms, affordable behind the existing cache); that remains an
open design choice, not an oversight.

**Cost.** One `lstat()` per tracked path, measured ~0.5 ms over 536 files,
at most once per `GIT_POLL_INTERVAL`, and **only for prompts that actually
use `%v` or `%V`** — a prompt using just `%g` never pays for it. Index
version 4 (opt-in path compression) is not parsed; the scan reports unknown
and no indicator is shown rather than risking a misread. Submodule entries
are skipped, since evaluating them means walking another repository.

Sub-fixes made during this work:

- A conflicted path appears once per stage in the index, so a single
  conflicted file first reported `!3`. Runs of stages for one path are now
  counted once.
- Status staleness was initially keyed on the index mtime, which is wrong:
  editing a tracked file in the working tree never touches `.git/index`, so
  the indicators froze in a long-lived shell. Confirmed, then changed to
  rescan on the poll interval. Verified live in one session, clean →
  modified → clean.
- `git_get_info()` now also reports the worktree root, which the index scan
  needs — index paths are relative to it, and for a linked worktree it is
  not the parent of the git directory.

### Also

- `dot.mcshrc` switches its right prompt from `%G` to `%V`.
- Man page, `README.md` and `dot.mcshrc` document the new escapes, including
  what is and is not reported and why.

Suite: 17 passed, 0 failed. Zero warnings under `-Wall -Wextra`. Full git
battery re-verified: subdirectories, linked worktrees, every operation
state, detached HEAD, and the poll throttle.

### Known remaining gaps

- Staged-vs-`HEAD`, untracked files and ahead/behind counts (above).
- `cmd_on_path()` still re-implements `$PATH` search rather than reusing the
  shell's `xhash` table. Deliberate: `xhash` is a bloom-filter-style bit
  table whose false positives would colour a non-existent command green, and
  the LRU cache already removes the syscall cost. Documented rather than
  changed.
- Variables are not checked for being set; `$typo` and `$HOME` look alike.
  Skipped as too false-positive-prone to be worth it.
- Index version 4 is unparsed.
