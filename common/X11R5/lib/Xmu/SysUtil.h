/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:SysUtil.h	1.2"
/* $XConsortium: SysUtil.h,v 1.3 91/07/22 23:46:12 converse Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

#ifndef _SYSUTIL_H_
#define _SYSUTIL_H_

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

extern int XmuGetHostname(
#if NeedFunctionPrototypes
    char*	/* buf_return */,
    int		/* maxlen */
#endif
);

_XFUNCPROTOEND

#endif /* _SYSUTIL_H_ */
