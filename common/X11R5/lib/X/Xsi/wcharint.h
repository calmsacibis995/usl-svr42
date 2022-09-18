/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/wcharint.h	1.1"
/*
 * $XConsortium: wcharint.h,v 1.11 91/06/26 13:54:07 rws Exp $
 */

/*
 * Copyright 1990, 1991 by OMRON Corporation, NTT Software Corporation,
 *                      and Nippon Telegraph and Telephone Corporation
 * Copyright 1991 by the Massachusetts Institute of Technology
 *
 *
 * OMRON, NTT SOFTWARE, NTT, AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * 
 *	Author: Li Yuhong	OMRON Corporation
 *   
 */

#ifndef	_MCHARINT_H_
#define _MCHARINT_H_

#include <ctype.h>
#ifndef _WCHARXLIB_
#include "wchar.h"
#endif

#ifndef WNULL
typedef	unsigned long wchar;	/* must be unsigned 4-byte type, ISO10646 */

#define WNULL	0
#endif

#define LATIN1_GROUP	0x20	/* Latin 1 Group# */
#define LATIN1_PLANE	0x20	/* Latin 1 Plane# */
#define LATIN1_ROW	0x20	/* Latin 1 Row#   */

#define SCRIPT1		0xffffff00
#define SCRIPT2		0xffff0000
#define CONTROLSCRIPT	0x00ffffff
#define LATINSCRIPT	0x20202000
#define HIRAGANASCRIPT  0x20202f00
#define KATAKANASCRIPT  0x20202f80
#define HANZISCRIPT	0x20308080
#define KANJISCRIPT	0x20408080
#define HANJASCRIPT	0x20508080
#define CLPADPADPAD	0x00808080
#define WCHARSPACE	0x20202020

#define PrivateZone	0x20203400	/* Private Use Zone */
#define PrivateZoneLast	0x20203b00	/* Last row of Private Use Zone */

#define PrivatePlane	0x20e00000	/* Private Use Plane */
#define PrivatePlaneLast 0x20ff0000	/* Last row of Private Use Plane */
#define PrivatePlaneLastI11 (PrivatePlaneLast | 0x8080)
#define PrivatePlaneLastI10 (PrivatePlaneLast | 0x8000)

#define	wgetgroup(wc)	((unsigned char)(((wc) >> 24) & 0x000000ff))
#define	wgetplane(wc)	((unsigned char)(((wc) >> 16) & 0x000000ff))
#define	wgetrow(wc)	((unsigned char)(((wc) >>  8) & 0x000000ff))
#define	wgetcell(wc)	((unsigned char)((wc) & 0x000000ff))


#define _Xiswcntrl(wc)   ( ((wc)==WNULL) || \
			   (((wc)&CONTROLSCRIPT)==CLPADPADPAD \
			    && iscntrl(wgetgroup(wc)&0x7f)) )

#define _Xatowc(c)	( ((c)=='\0') ? WNULL: \
			 (iscntrl((c)&0x7f)) ? (((wchar)(c)<<24)|CLPADPADPAD): \
						  (LATINSCRIPT|(c)) )
#define _Xwctoa(wc)	( _Xiswcntrl(wc) ? wgetgroup(wc): \
			 (((wc)&SCRIPT1)==LATINSCRIPT) ? wgetcell(wc): \
						(-1) )

#endif /* _MCHARINT_H_ */
