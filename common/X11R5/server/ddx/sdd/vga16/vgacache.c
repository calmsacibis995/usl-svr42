/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga16:vga16/vgacache.c	1.4"

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

#include "X.h"
#include "Xmd.h"
#include "sidep.h"
#include "sys/types.h"
#ifndef SVR4
#include "sys/at_ansi.h"
#endif
#include "sys/kd.h"
#include "vtio.h"
#include "vga.h"

extern struct at_disp_info vt_info;


/*
 *	vga_cache_alloc (buf, type)
 *			allocates cache memory and returns the pointer to
 *			cache memory in buf->Bptr.
 *
 *	Input:
 *		SIbitmap	buf
 *		SIint32		type	-- SHORTTERM_MEM or LONGTERM_MEM
 */
SIBool
vga_cache_alloc (buf, type)
SIbitmap	*buf;
SIint32		type;
{
	int	size;

	DBENTRY("vga_cache_alloc()");

	size = (((buf->Bwidth + 7) >> 3) + 1) * vt_info.planes;
	if ( (buf->Bptr = (SIArray)malloc (buf->Bheight * size)) != NULL) {
		buf->Btype = XY_PIXMAP;
	}
}	

/*
 *	vga_cache_free (buf)
 *			frees a previously allocated cache memory.
 *			ie: buf->Bptr
 *
 *	Input:
 *		SIbitmap	*buf	-- pointer to the memory
 *
 * 3/27/91: This does not match the SI SPEC; RESOLVE THIS SOON.
 */
SIBool
vga_cache_free (buf)
SIbitmap	*buf;
{
	DBENTRY("vga_cache_free()");

	if (buf->Bptr)
		free (buf->Bptr);
}

/* THE FOLLOWING TWO ROUTINES ARE NOT NEEDED FOR VGA SDD */

/*
 *	vga_cache_lock (buf)
 *			lock cached memory
 *
 *	Input:
 *		SIbitmap	*buf
 *
SIBool
vga_cache_lock (buf)
SIbitmap	*buf;
{
	DBENTRY("vga_cache_lock()");
}
*/

/*
 *	vga_cache_unlock (buf)
 *			unlock cached memory
 *
 *	Input:
 *		SIbitmap	*buf
 *
SIBool
vga_cache_unlock (buf)
SIbitmap	*buf;
{
	DBENTRY("vga_cache_unlock()");
}
*/
