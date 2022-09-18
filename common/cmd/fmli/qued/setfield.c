/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:qued/setfield.c	1.10.3.3"

#include <stdio.h>
#include <curses.h>
/* #include <malloc.h>  abs s19 */
#include "token.h"
#include "winp.h"
#include "fmacs.h"
#include "terror.h"
#include "attrs.h"
#include "wish.h"		/* abs s16 */

#define FSIZE(x)	(x->rows * (x->cols + 1))

setfieldflags(fld, flags)
register ifield *fld;
register int flags;
{
    fld->flags = (flags & I_CHANGEABLE) | (fld->flags & ~(I_CHANGEABLE));
    if (fld->flags & I_INVISIBLE)
    {
	if (fld->value)
	    free(fld->value);	/* abs */
	if ((fld->value = (char *) malloc(FSIZE(fld))) == NULL)
	    fatal(NOMEM, "");
	fld->valptr = fld->value;
	*fld->value = EOS;           /* abs s16 */  
    }
    fld->fieldattr = (fld->flags & I_FILL ? Attr_underline: Attr_normal);
}

