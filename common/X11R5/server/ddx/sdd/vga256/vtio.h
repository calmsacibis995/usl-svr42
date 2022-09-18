/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/vtio.h	1.3"

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

#include "vtdefs.h"

#define BYTE	unsigned char
#define	SUCCESS	1
#define	FAIL	0

/*
 * structure used to initialize the V256 registers
 */
struct v256_regs 
{
	unsigned char	seqtab[NSEQ];
	unsigned char	miscreg;
	struct egainit	egatab;
};


/*
 * General adapter information such as size of display, pixels per inch, etc.
 */
struct at_disp_info 
{
	char	*entry;		/* string type, "ET4000" */
	char	*monitor;	/* monitor type, ex: "MULTISYNC" */
	int	vt_type;	/* defined type, VT_V256 */
	int	xpix;		/* pixels per scanline */
	int	ypix;		/* number of scanlines */
	BYTE	*vt_buf;	/* virtual address of screen memory */
	int	buf_size;	/* size of screen memory */
	int	map_size;	/* size of one plane of memory */
	int	ad_addr;	/* base register address for adapter */
	int	slbytes;	/* number of bytes in a scanline */
	struct v256_regs *regs;	/* v256 registers for mode */
	int	(*ext_init)();	/* called to initialize 'extended' modes */
	int	(*ext_rest)();	/* called to recover from 'extended' modes */
	int	(*ext_page)();	/* called to select a video page */
};

#define ErrorF printf
