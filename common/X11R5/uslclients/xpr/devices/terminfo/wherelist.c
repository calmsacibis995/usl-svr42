#ident	"@(#)xpr:devices/terminfo/wherelist.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#define	STREQU(A,B)	(strcmp((A), (B)) == 0)

/**
 ** wherelist() - RETURN POINTER TO ITEM IN LIST
 **/

char			**wherelist (item, list)
	register char		*item;
	register char		**list;
{
	if (!list || !*list)
		return (0);

	while (*list) {
		if (STREQU(*list, item))
			return (list);
		list++;
	}
	return (0);
}
