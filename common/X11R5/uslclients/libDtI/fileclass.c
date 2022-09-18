/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:fileclass.c	1.4"
#endif

#include <X11/Intrinsic.h>
#include "DtI.h"

/*
 * This file contains routines to maintain an array of file class structures.
 */


/*
 * Allocate an entry for a new class.
 */
DmFclassPtr
DmNewFileClass(key)
void *key;	/* ptr to FmodeKey or FnameKey */
{
	DmFclassPtr fcp;

	if (fcp = (DmFclassPtr)calloc(1, sizeof(DmFclassRec)))
		fcp->key  = key;
	return(fcp);
}

