/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xdmcp:WC32.c	1.2"
/*
 * $XConsortium: WC32.c,v 1.2 91/01/23 22:14:40 gildea Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <X11/Xos.h>
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

int
XdmcpWriteCARD32 (buffer, value)
    XdmcpBufferPtr  buffer;
    CARD32	    value;
{
    if (!XdmcpWriteCARD8 (buffer, value >> 24))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, (value >> 16) & 0xff))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, (value >> 8) & 0xff))
	return FALSE;
    if (!XdmcpWriteCARD8 (buffer, value & 0xff))
	return FALSE;
    return TRUE;
}
