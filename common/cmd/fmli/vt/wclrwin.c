/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:vt/wclrwin.c	1.2.3.3"

#include	<curses.h>
#include	"wish.h"
#include	"vtdefs.h"
#include	"vt.h"
#include	"attrs.h"

void
wclrwin(vid)
vt_id vid;		      /* added parameter.  abs s13 */
{
	register struct vt	*v;

	if (vid >= 0)				/* abs s13 */
	    v = &VT_array[(int)vid]; 		/* abs s13 */
	else if (VT_curid >= 0)			/* abs s13 */
	    v = &VT_array[VT_curid];
	else					/* abs s13 */
	    return;				/* abs s13 */
	wclrtobot(v->win);
	v->flags |= VT_BDIRTY;
}
