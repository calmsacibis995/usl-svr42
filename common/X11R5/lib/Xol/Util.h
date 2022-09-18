/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:Util.h	1.1"
#endif

/*
 * Util.h
 *
 */

#ifndef _Util_h
#define _Util_h

#define ABS(x)               ((x) < 0 ? -(x) : (x))
#define NABS(x)              ((x) > 0 ? -(x) : (x))

#define MAX(x,y)             ((x) > (y) ? (x) : (y))
#define MIN(x,y)             ((x) < (y) ? (x) : (y))

#endif
