/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5util:misc/dlsym.c	1.1"
/*
 * Stub interface to dynamic linker routines
 * that SunOS uses but didn't ship with 4.1.
 *
 * The C library routine wcstombs in SunOS 4.1 tries to dynamically
 * load some routines using the dlsym interface, described in dlsym(3x).
 * Unfortunately SunOS 4.1 does not include the necessary library, libdl.
 *
 * The R5 Xlib uses wcstombs.  If you link dynamcally, your program can
 * run even with the unresolved reference to dlsym.  However, if you
 * link statically, you will encounter this bug.  One workaround
 * is to include these stub routines when you link.
 */

void *dlopen()
{
    return 0;
}

void *dlsym()
{
    return 0;
}

int dlclose()
{
    return -1;
}
