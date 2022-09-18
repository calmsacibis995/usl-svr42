/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/bdfint.h	1.1"
/*

 * $XConsortium: bdfint.h,v 1.2 91/07/17 20:42:48 rws Exp $
 *
 * Copyright 1990 Massachusetts Institute of Technology
 *
 *
 *
 */

#ifndef BDFINT_H
#define BDFINT_H

#define bdfIsPrefix(buf,str)	(!strncmp((char *)buf,str,strlen(str)))
#define	bdfStrEqual(s1,s2)	(!strcmp(s1,s2))

#define	BDF_GENPROPS	6
#define NullProperty	((FontPropPtr)0)

/*
 * This structure holds some properties we need to generate if they aren't
 * specified in the BDF file and some other values read from the file
 * that we'll need to calculate them.  We need to keep track of whether
 * or not we've read them.
 */
typedef struct BDFSTAT {
    int         linenum;
    char       *fileName;
    char        fontName[MAXFONTNAMELEN];
    float       pointSize;
    int         resolution_x;
    int         resolution_y;
    int         digitCount;
    int         digitWidths;
    int         exHeight;

    FontPropPtr fontProp;
    FontPropPtr pointSizeProp;
    FontPropPtr resolutionXProp;
    FontPropPtr resolutionYProp;
    FontPropPtr resolutionProp;
    FontPropPtr xHeightProp;
    FontPropPtr weightProp;
    FontPropPtr quadWidthProp;
    BOOL        haveFontAscent;
    BOOL        haveFontDescent;
    BOOL        haveDefaultCh;
}           bdfFileState;

extern unsigned char *bdfGetLine();

extern void bdfError();
extern void bdfWarning();
extern Atom bdfForceMakeAtom();
extern Atom bdfGetPropertyValue();
extern unsigned char bdfHexByte();

#endif				/* BDFINT_H */
