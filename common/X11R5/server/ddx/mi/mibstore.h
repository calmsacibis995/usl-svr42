/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/mi/mibstore.h	1.1"
/*-
 * mibstore.h --
 *	Header file for users of the MI backing-store scheme.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *	"$XConsortium: mibstore.h,v 5.0 89/06/09 15:00:32 keith Exp $ SPRITE (Berkeley)"
 */

#ifndef _MIBSTORE_H
#define _MIBSTORE_H

typedef struct {
    void	    (*SaveAreas)();
    void	    (*RestoreAreas)();
    void	    (*SetClipmaskRgn)();
    PixmapPtr	    (*GetImagePixmap)();
    PixmapPtr	    (*GetSpansPixmap)();
} miBSFuncRec, *miBSFuncPtr;

extern void miInitBackingStore();

#endif /* _MIBSTORE_H */
