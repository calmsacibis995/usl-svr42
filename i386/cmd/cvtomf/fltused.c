/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cvtomf:fltused.c	1.2"

/* Enhanced Application Compatibility Support */

long	_fltused = 0;

asm(".section	.note");
asm(".string	\"Converted OMF object(s), use XENIX semantics!\"");

/* End Enhanced Application Compatibility Support */
