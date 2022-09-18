/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/opcntrl.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)opcntrl.c 1.2 89/03/10";
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
#include    	"f3io.h"

#include    	"glbvars.h"
#include    	"error.h"
#include    	"xstack.h"
#include    	"scanners.h"
#include    	"opcntrl.h"

#ifdef DEBUG
extern	char	StackImage[];
#endif

VOID	f3_NoOp(ap) f3_ArgTYPE *ap;
{
#ifdef DEBUG
    	if      (SP< SB)    printf("STACK UFLO %d\n",SP-SB);
    	else if (SP==SB)    printf("STACK EMPTY\n");
    	else {	DSTACK();   printf(StackImage); }
#endif
}

VOID	f3_If(ap) f3_ArgTYPE *ap;
{
    	register f3_PairTYPE	*thenp,*condp;

    	thenp = --SP;
    	condp = --SP;
    	DCHECK(S_UFLO,SP<SB);

    	DCHECK(INV_OPERAND,condp->func!=f3_Number);
    	DCHECK(INV_OPERAND,thenp->func!=f3_SimpleBlock);
    	if (condp->arg.f<FRHALF)  return;

    	XSAVE0;

    	SCAN =	    f3_ScanPairs;
    	SPP = 	    thenp->arg.p-1;    /* before 1st pair */
    	SET_PBL_BLOCK;
}

VOID	f3_IfElse(ap) f3_ArgTYPE *ap;
{
    	register f3_PairTYPE	*thenp,
    	    	    	    	*elsep,
    	    	    	    	*condp,
    	    	    	    	*h;

    	elsep = --SP;
    	thenp = --SP;
    	condp = --SP;
    	DCHECK(S_UFLO,SP<SB);

    	DCHECK(INV_OPERAND,condp->func!=f3_Number);
    	DCHECK(INV_OPERAND,thenp->func != f3_SimpleBlock);
    	DCHECK(INV_OPERAND,elsep->func != f3_SimpleBlock);

    	if (condp->arg.f>=FRHALF)	h = thenp->arg.p;
    	else	    	    		h = elsep->arg.p;

    	XSAVE0;

    	SCAN =	    f3_ScanPairs;
    	SPP = 	    h-1;    /* before 1st pair */
    	SET_PBL_BLOCK;
}


VOID	f3_Forever(ap) f3_ArgTYPE *ap;
{    	register f3_PairTYPE	*loopp;

    	loopp = --SP;
    	DCHECK(S_UFLO,SP<SB);
    	DCHECK(INV_OPERAND,loopp->func != f3_SimpleBlock);

    	XSAVE0;
    	SCAN =	    	    f3_ScanPairs;
    	PBL_LOOP = SPP =    loopp->arg.p-1;    /* before 1st pair */
}

VOID	f3_Break(ap) f3_ArgTYPE *ap;
{    	bool	enough;

    	do {
    	    /* we assume we can identify a "symbol"
    	       level (which is unlikely to be exited
    	       by a break, true) by the scanner */
    	    if (SCAN==f3_ScanBytes) PSP->activecnt--;
    	    enough = IS_LOOP || IS_PROC;
    	    XRESTORE;
    	} while(!enough);
}

VOID	f3_Return(ap) f3_ArgTYPE *ap;
{    	bool	enough;

    	do {
    	    /* we assume we can identify a "symbol"
    	       level by the scanner */
    	    if (SCAN==f3_ScanBytes) PSP->activecnt--;
    	    enough = IS_PROC;
    	    XRESTORE;
    	} while(!enough);
}

extern	f3_PStateTYPE	*f3_FindPState();    /* ... */

VOID	f3_CallSymbol(ap) f3_ArgTYPE *ap;
{
    	f3_PairTYPE    	*ccodep,*transp;
	int32		 ccode;
    	f3_PStateTYPE	*psp;

    	ccodep =   --SP;
    	transp =   --SP;
    	DCHECK(S_UFLO,SP<SB);
    	DCHECK(INV_OPERAND,ccodep->func!=f3_Number);
    	DCHECK(INV_OPERAND,transp->func!=f3_Array);
	ccode =  (int32)(ccodep->arg.f); /* we want to regard this fract as an int */
	transp = transp->arg.p;

    	XSAVE0;
    	XSAVE1;
	XSAVE2;	/* this is the only case when we save State2 */
    	XSAVE3;

    	psp = f3_FindPState(	FIP,
				PSP->pixratio,
		   	    	(transp  )->arg.f,
    	    	    	    	(transp+1)->arg.f,
    	    	    	    	(transp+2)->arg.f,
    	    	    	    	(transp+3)->arg.f  );

    	CHECK(NO_SYMBOL,FALSE==f3_SeekSymbol(FIP,ccode));

    	PSP = 	     psp;
    	SCAN =	     f3_ScanBytes;
    	SPP =	    &f3_ScanPair;
    	SET_PBL_PROC;
    	CONSTANT =  (char *)(psp + 1);
    	GLOBALB =   (f3_PairTYPE *)(CONSTANT + FIP->globalvars);
    	GLOBALL =   GLOBALB + FIP->globalvars;
        GLOBALH =   GLOBALB + FIP->globalsize - psp->globalhsize;
    	LOCALCNT =  f3_FontReadBINT(FIP->filep);
    	LOCALP +=   LOCALCNT;
}


