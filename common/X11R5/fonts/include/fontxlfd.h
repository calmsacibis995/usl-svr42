/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontinc:include/fontxlfd.h	1.2"
/*

 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _FONTXLFD_H_
#define _FONTXLFD_H_


typedef struct _FontScalable {
    int         pixel,
                point,
                x,
                y,
                width;
}           FontScalableRec, *FontScalablePtr;

extern Bool FontParseXLFDName();

#define FONT_XLFD_REPLACE_NONE	0
#define FONT_XLFD_REPLACE_STAR	1
#define FONT_XLFD_REPLACE_ZERO	2
#define FONT_XLFD_REPLACE_VALUE	3

#define         MAXPOINT        65535
#define         MAXPIXEL        6553

#endif				/* _FONTXLFD_H_ */
