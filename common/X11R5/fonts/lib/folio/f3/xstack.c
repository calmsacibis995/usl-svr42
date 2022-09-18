/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/xstack.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)xstack.c 1.4 89/05/25";
#endif
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include    	<setjmp.h>

#include        "cdefs.h"
#include	"frmath.h"
#include	"f3io.h"
#include    	"glbvars.h"
#include    	"error.h"
#include    	"xstack.h"

#ifdef DEBUG
char	XSImage[100];
int 	XSI;
#endif

#ifdef PERF
extern  unsigned char	*f3_CurFontP;
extern	unsigned char	 f3_Font[];
extern	bool	 	 f3_MemSeek();
#endif /*PERF*/

VOID	f3_XSave0()
{
#ifdef DEBUG
if (XP==XL)
    	XSI=0;
XSImage[XSI++] = '0';
XSImage[XSI] = 0;
#endif
    	XP -= sizeof(f3_XState0TYPE);
    	DCHECK(X_OFLO,XP<XB);

    	*(f3_XState0TYPE *)XP = f3_XState0;
}

VOID	f3_XSave1()
{
#ifdef DEBUG
if (XP==XL) XSI=0;
XSImage[XSI++] = '1';
XSImage[XSI] = 0;
#endif
    	XP -= sizeof(f3_XState1TYPE);
    	DCHECK(X_OFLO,XP<XB);

    	*(f3_XState1TYPE *)XP = f3_XState1;
}

VOID	f3_XSave2()
{
#ifdef DEBUG
if (XP==XL) XSI=0;
XSImage[XSI++] = '2';
XSImage[XSI] = 0;
#endif
    	XP -= sizeof(f3_XState2TYPE);
    	DCHECK(X_OFLO,XP<XB);
#ifndef PERF
	f3_XState2.fdisp = ftell(f3_XState2.fip->filep); /* aka BYTEFILEP */
#else
	f3_XState2.fdisp = ((long)f3_CurFontP - (long)f3_Font) / sizeof( char );
#endif  /*PERF*/
	f3_XState2.encryptstate = f3_EncryptState;

    	*(f3_XState2TYPE *)XP = f3_XState2;
}

VOID	f3_XSave3()
{
#ifdef DEBUG
if (XP==XL) XSI=0;
XSImage[XSI++] = '3';
XSImage[XSI] = 0;
#endif
    	XP -= sizeof(f3_XState3TYPE);
    	DCHECK(X_OFLO,XP<XB);

    	*(f3_XState3TYPE *)XP = f3_XState3;
}

bool	f3_XRestore0()
{
#ifdef DEBUG
XSI--;
XSImage[XSI] = 0;
#endif
    	f3_XState0 = *(f3_XState0TYPE *)XP;
    	XP += sizeof(f3_XState0TYPE);
    	DCHECK(X_UFLO,XP > XL);			/* compare w/ max   may 5/19 */

    	return(FALSE);
}

bool	f3_XRestore1()
{
#ifdef DEBUG
XSI--;
XSImage[XSI] = 0;
#endif
    	f3_XState1 = *(f3_XState1TYPE *)XP;
    	XP += sizeof(f3_XState1TYPE);
    	DCHECK(X_UFLO,XP > XL);

    	return(TRUE);
}

bool	f3_XRestore2()
{
#ifdef DEBUG
XSI--;
XSImage[XSI] = 0;
#endif
    	f3_XState2 = *(f3_XState2TYPE *)XP;
    	XP += sizeof(f3_XState2TYPE);
    	DCHECK(X_UFLO,XP > XL);

#ifndef PERF
    	fseek(f3_XState2.fip->filep,f3_XState2.fdisp,(int32)0);
#else
    	f3_MemSeek(f3_XState2.fdisp,(int32)0);
#endif  /*PERF*/
	f3_EncryptState = f3_XState2.encryptstate;

    	return(TRUE);
}

bool	f3_XRestore3()
{
#ifdef DEBUG
XSI--;
XSImage[XSI] = 0;
#endif
    	f3_XState3 = *(f3_XState3TYPE *)XP;
    	XP += sizeof(f3_XState3TYPE);
    	DCHECK(X_UFLO,XP > XL);

    	return(TRUE);
}

VOID	f3_XRestore() {	while( ((f3_XStateTYPE *)XP)->restore()	); }

