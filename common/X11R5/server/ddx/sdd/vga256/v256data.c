/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/v256data.c	1.1"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

#include "Xmd.h"
#include "sidep.h"
#include "miscstruct.h"
#include "sys/types.h"
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "vtio.h"
#include "v256.h"

#ifdef DEBUG
int 	xdebug = 0x0;
#endif

int	v256_clip_x1;		/* clipping region */
int	v256_clip_y1;
int	v256_clip_x2;
int	v256_clip_y2;
int	v256_slbytes;

BYTE	v256_slbuf[MAXSCANLINE+16]; /* buffer for a scanline */
BYTE	v256_tmpsl[MAXSCANLINE+16]; /* temporary buffer for a scanline */

v256_rgb	v256_pallette[V256_MAXCOLOR];/* V256 pallette registers */

v256_state	v256_gstates[V256_NUMGS];	/* grapics states */
v256_state	*v256_gs = v256_gstates;	/* pointer to current state */
int		v256_cur_state;			/* current state selected */

BYTE		*v256_cur_pat;		/* current tile/stipple pattern */
int		v256_cur_pat_h;		/* current_pattern's height */

v256_font	v256_fonts[V256_NUMDLFONTS]; /* downloaded font info */

BYTE		v256_src;		/* current source color */
int		v256_invertsrc;		/* currently inverting source? */
int		v256_function;		/* current V256 function */

int		v256_page;			/* current video page in use */
int		v256_endpage;		/* last valid offset from v256_fb */
