/*
 * ed.syntax.h: Interactive syntax highlighting for mcsh.
 *
 * Provides a parallel byte array SyntaxColor[INBUFSIZE] whose every entry
 * holds a SynToken value for the corresponding InputBuf character.
 * syntax_colorize() rescans the input buffer on every buffer mutation when
 * `set syntax` is active.  The render pipeline in ed.refresh.c / ed.screen.c
 * consults SyntaxColor[] at draw time via the VcolorDisplay / ColorDisplay
 * parallel arrays (Option B: full virtual-display integration).
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

/*
 * SynToken — per-character syntactic category.
 * Values fit in a uint8_t (0–255).  The renderer maps each value to an
 * ANSI SGR color code pair (foreground + attributes).
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
    SYN__MAX     = 12
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

#endif /* _h_ed_syntax */
