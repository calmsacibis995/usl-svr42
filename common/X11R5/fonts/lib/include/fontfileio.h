/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:lib/include/fontfileio.h	1.1"
/*
 * $XConsortium: fontfileio.h,v 1.1 91/09/07 11:59:55 keith Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include    <bufio.h>

typedef BufFilePtr  FontFilePtr;

#define FontFileGetc(f)	    BufFileGet(f)
#define FontFilePutc(c,f)   BufFilePut(c,f)
#define FontFileRead(f,b,n) BufFileRead(f,b,n)
#define FontFileWrite(f,b,n)	BufFileWrite(f,b,n)
#define FontFileSkip(f,n)   (BufFileSkip (f, n) != BUFFILEEOF)
#define FontFileSeek(f,n)   (BufFileSeek (f,n,0) != BUFFILEEOF)

#define FontFileEOF	BUFFILEEOF

extern FontFilePtr  FontFileOpen ();
extern FontFilePtr  FontFileOpenWrite ();
extern FontFilePtr  FontFileOpenFd ();
extern FontFilePtr  FontFileOpenWriteFd ();
extern int	    FontFileClose ();
