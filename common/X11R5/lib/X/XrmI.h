/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XrmI.h	1.1"
/* $XConsortium: XrmI.h,v 1.7 91/04/23 18:25:52 rws Exp $ */
/*

Copyright 1990 by the Massachusetts Institute of Technology


*/

/*
 * Macros to abstract out reading the file, and getting its size.
 *
 * You may need to redefine these for various other operating systems. 
 */

#include	<X11/Xos.h>
#include        <sys/stat.h>                        

#define OpenFile(name) 		open((name), O_RDONLY)
#define CloseFile(fd)           close((fd))
#define ReadFile(fd,buf,size)	read((fd), (buf), (size))
#define GetSizeOfFile(name,size)                    \
{                                                   \
    struct stat status_buffer;                      \
    if ( (stat((name), &status_buffer)) == -1 )     \
	size = -1;                                  \
    else                                            \
	size = status_buffer.st_size;               \
}
