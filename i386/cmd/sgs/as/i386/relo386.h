/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:i386/relo386.h	1.2"
/* relo386.h */

/* Routines to handle i386-style relocations. */

#ifdef __STDC__
void relocaddr(Eval *, Uchar *, Section *);
void relocpcrel(Eval *, Uchar *, Section *);
#else
void relocaddr();
void relocpcrel();
#endif
