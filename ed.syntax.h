/*
 * ed.syntax.h: Interactive syntax highlighting for mcsh.
 *
 * Provides a parallel byte array SyntaxColor[INBUFSIZE] whose every entry
 * holds a SynToken value for the corresponding InputBuf character.
 * syntax_colorize() rescans the input buffer on every buffer mutation when
 * `set syntax` is active.  The render pipeline in ed.refresh.c / ed.screen.c
 * reads syntax tokens packed into Vdisplay / Display Char values via
 * SYN_TOK() / SYN_GLYPH() (Option B: full virtual-display integration).
 */
/*-
 * Copyright (c) 2026 The mcsh Contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _h_ed_syntax
#define _h_ed_syntax

#include <stdint.h>

/*
 * Syntax token packing into Vdisplay Char values.
 *
 * Char is 32-bit (WIDE_STRINGS).  ATTRIBUTES uses 0x0F000000.
 * Bits 0xF0000000 are unused in the display path (QUOTE/0x80000000 is
 * only set by the lexer, never written into Vdisplay/Display).
 * We store the 4-bit SynToken in those bits so update_line()'s glyph
 * diff (*o == *n) naturally detects colour-only changes without any
 * separate parallel arrays or display poisoning.
 *
 * For SHORT_STRINGS / narrow-Char builds (sizeof(Char) < 4) bit-packing
 * is disabled: SYN_PACK is a no-op, SYN_TOK always returns SYN_NORMAL,
 * and SYN_GLYPH is the identity function so the build is still correct
 * (syntax colours are simply not shown on narrow builds).
 *
 * INVARIANT — the 0xF0000000 field must be free in the display path.
 *
 * It is not free everywhere: QUOTE is 0x80000000 and INVALID_BYTE is
 * 0xF0000000, i.e. exactly SYN_MASK.  Neither may reach Vdisplay/Display.
 *   - QUOTE is set only by the lexer, which never writes to the display.
 *   - INVALID_BYTE is produced by GetNextChar() (ed.inputl.c) for bytes that
 *     do not decode in the current locale, but such bytes are rejected before
 *     insertion, so they never reach InputBuf and therefore never reach
 *     Vdraw().
 * All 16 token values are now assigned, so a stray high-bit character would
 * no longer be caught by a range clamp: it would silently render as some
 * other token.  Anything that starts letting undecodable bytes into InputBuf
 * must either strip SYN_MASK on the way in or move to a parallel array.
 */
#if defined(WIDE_STRINGS) || (defined(SIZEOF_CHAR_T) && SIZEOF_CHAR_T >= 4) || \
    (!defined(SHORT_STRINGS) && !defined(KANJI))
# define SYN_SHIFT	28			/* bit position of token field */
# define SYN_MASK	((Char)0xF0000000U)	/* mask for token field */
/* Pack token t into display Char c */
# define SYN_PACK(c, t)	(((c) & ~SYN_MASK) | (((Char)(t)) << SYN_SHIFT))
/* Extract token from display Char c */
# define SYN_TOK(c)	(((unsigned)(c) >> SYN_SHIFT) & 0xF)
/* Strip token bits to get the raw glyph for terminal output */
# define SYN_GLYPH(c)	((c) & ~SYN_MASK)
#else
/* Narrow-Char fallback: disable bit-packing, syntax colours not available */
# define SYN_MASK	0
# define SYN_PACK(c, t)	(c)
# define SYN_TOK(c)	0
# define SYN_GLYPH(c)	(c)
#endif

/*
 * SYN_GHOST — per-cell flag marking a predictive-autocomplete cell.
 *
 * Ghost text is not part of InputBuf, so it cannot be described by the
 * SyntaxColor[] side array and it has no SynToken of its own: the 4-bit
 * token field is full (SYN__MAX == 16).  It needs one bit, and it needs it
 * *in the cell*, because update_line() decides what to repaint by comparing
 * Display[] against Vdisplay[] cell for cell.  Marking ghostness anywhere
 * else would leave a cell that keeps its glyph but changes its nature -
 * typing exactly the character that was predicted - looking unchanged to the
 * differ, so the dim attribute would stay on screen over real input.
 *
 * Bit assignment, per Char layout in sh.h:
 *
 *   WIDE_STRINGS   QUOTE 0x80000000, SYN_MASK 0xF0000000, ATTRIBUTES
 *                  0x0F000000, CHAR 0x00FFFFFF.  Every bit outside CHAR is
 *                  taken, so the flag lives at the top of the CHAR field, at
 *                  bit 23 (0x00800000).  No character can collide with it:
 *                  the Unicode and ISO/IEC 10646 code space ends at
 *                  U+10FFFF, which is 0x10FFFF, and 0x10FFFF has bit 23
 *                  clear (bit 23 is 0x800000, larger than 0x10FFFF), so no
 *                  code point sets it.
 *
 *   SHORT_STRINGS  QUOTE 0100000, ATTRIBUTES 0074000, LITERAL 04000,
 *                  CHAR 0377.  Bits 0001400 are unassigned; take 0000400.
 *
 *   8-bit build    Char has no spare bit at all; SYN_GHOST is 0 and the
 *                  renderer draws no ghost text (undimmed ghost text is
 *                  indistinguishable from real input, which is worse than
 *                  none).
 *
 * Two cell values in the display path have bit 23 set for reasons of their
 * own and must never be mistaken for ghost cells:
 *
 *   CHAR_DBWIDTH  == LITERAL|(LITERAL-1) == 0x01FFFFFF, the continuation
 *                   column of a double-width character.
 *   LITERAL cells  carry a litptr index in the low bits.
 *
 * Both have LITERAL set and ghost cells never do, which is what
 * SYN_IS_GHOST() tests.
 */
#if defined(WIDE_STRINGS) || (defined(SIZEOF_CHAR_T) && SIZEOF_CHAR_T >= 4) || \
    (!defined(SHORT_STRINGS) && !defined(KANJI))
# define SYN_GHOST	((Char)0x00800000U)
#elif defined(SHORT_STRINGS)
# define SYN_GHOST	((Char)0000400)
#else
# define SYN_GHOST	((Char)0)
#endif

/* Is this display cell ghost text?  See the bit-assignment note above. */
#define SYN_IS_GHOST(c)	(SYN_GHOST != 0 && ((c) & SYN_GHOST) != 0 && \
			 (SYN_GLYPH(c) & LITERAL) == 0)
/* Strip the ghost flag; the result is what goes to the terminal. */
#define SYN_UNGHOST(c)	((c) & ~SYN_GHOST)

/*
 * SynToken — per-character syntactic category.
 * Values 0-15 fit in the 4-bit token field above.
 */
typedef enum {
    SYN_NORMAL   = 0,	/* uncoloured / default terminal colour */
    SYN_KEYWORD  = 1,	/* language keyword: if while foreach … */
    SYN_BUILTIN  = 2,	/* shell built-in: echo set alias cd … */
    SYN_CMD_OK   = 3,	/* first word — found on $PATH */
    SYN_CMD_BAD  = 4,	/* first word — NOT found on $PATH */
    SYN_OPERATOR = 5,	/* |  ;  &&  ||  &  >  <  >>  >& */
    SYN_VARIABLE = 6,	/* $var  $?var  ${var}  $#var */
    SYN_DQUOTE   = 7,	/* "…" double-quoted string */
    SYN_SQUOTE   = 8,	/* '…' single-quoted string */
    SYN_BACKTICK = 9,	/* `…` command substitution */
    SYN_COMMENT  = 10,	/* # to end-of-line */
    SYN_ERROR    = 11,	/* unmatched quote / bracket */
    SYN_ALIAS    = 12,	/* first word — a defined alias */
    SYN_FUNCTION = 13,	/* first word — a defined shell function */
    SYN_OPTION   = 14,	/* argument beginning with '-' */
    SYN_PATH     = 15,	/* argument naming an existing file or directory */
    SYN__MAX     = 16	/* == 1 << 4: the token field is now exactly full */
} SynToken;

/*
 * SGR colour specification for one token type.
 * fg: ANSI foreground code (30-37, 90-97, or 0 for default).
 * bold: non-zero ⇒ prepend SGR bold (1).
 */
typedef struct {
    int  fg;
    int  bold;
} SynColor;

/*
 * Default colour palette — matches the plan doc.
 * Callers may override via the SynPalette[] array.
 */
extern SynColor SynPalette[SYN__MAX];

/*
 * SyntaxColor[i] holds the SynToken for InputBuf[i].
 * Kept in sync with the buffer length; bytes past LastChar are SYN_NORMAL.
 */
extern uint8_t SyntaxColor[INBUFSIZE];

/*
 * Rescan InputBuf[0..LastChar) and rebuild SyntaxColor[].
 * Safe to call on every keystroke: O(n) in line length, no allocation,
 * no shell state mutation, no stderror().
 */
extern void syntax_colorize(void);

/*
 * Clear the entire SyntaxColor array (all SYN_NORMAL).
 * Called when `set syntax` is unset or the shell is not in input mode.
 */
extern void syntax_clear(void);

/*
 * Invalidate the command-lookup cache.
 * Call when PATH or the current working directory changes so stale
 * cmd_on_path() results are not returned for newly installed or
 * shadowed executables.
 */
extern void syntax_cache_clear(void);

#endif /* _h_ed_syntax */
