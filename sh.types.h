/* sh.types.h: Minimal type definitions for mcsh.
 *
 * Previously this file contained a large per-platform typedef thicket for
 * systems without <stdint.h> or proper POSIX <sys/types.h>.  C99 is now
 * the assumed floor for every supported target, so all of those ifdefs
 * have been retired.
 *
 * What remains:
 *   ptr_t   — generic void pointer alias used throughout the codebase.
 *   ioctl_t — third-argument type for ioctl(2) calls; void * on POSIX.
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
#ifndef _h_sh_types
#define _h_sh_types

/*
 * Minix does not provide caddr_t in <sys/types.h>.
 */
#if defined(_MINIX) && !defined(_MINIX_VMD)
typedef char * caddr_t;
#endif

#ifndef _PTR_T
# define _PTR_T
typedef void * ptr_t;
#endif /* _PTR_T */

#ifndef _IOCTL_T
# define _IOCTL_T
typedef void * ioctl_t;		/* Third arg of ioctl */
#endif /* _IOCTL_T */

#endif /* _h_sh_types */
