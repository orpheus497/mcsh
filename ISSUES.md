# mcsh — Consolidation Issue Log

Running log of bugs, obsolete code, and modernisation tasks noticed while
consolidating `tcsh` and `etcsh` into `mcsh`. Items are notes, not yet all
triaged or prioritised; they are burned down as polishing proceeds.

See `PLAN.md` for the full phased execution plan derived from this log.

---

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
  with interrupts disabled.
- **#117 / #121** (`sh.lex.c`, `sh.dol.c`) — Unicode regression: emoji/wide
  chars stripped from filenames and variable assignments since 6.24.14.
- **#93** (`tw.color.c`) — `ls-F` colour failures with `CLICOLOR_FORCE`.
- **#102 / #82** (`tcsh.man.in`) — Acute accent lintian warning; pipe
  workaround missing from man page.
- **`DrawGhost()`** — still writes directly to the terminal, bypassing the
  `Display`/`Vdisplay` virtual-display model. Stale ghost tails can appear on
  wide-character input or terminal resize. Full fix: integrate ghost rendering
  into the `Refresh()` pipeline.

### 4. Test suite

- `tests/` not yet initialised. Minimum suite required: startup file order,
  `$mcsh`/`$tcsh` variable correctness, unicode filename round-trip, expression
  overflow, job-count prompt, `cd -N` stack navigation.

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
- **Test suite (`tests/`)** — deferred.
- **Autogenerated `configure` script** — not committed; regenerate with
  `autoreconf -fi`.
