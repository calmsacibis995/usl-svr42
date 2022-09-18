/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/scanners.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)scanners.c 1.6 89/05/23";
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
#include    	"f3codes.h"

#include    	"glbvars.h"
#include    	"xstack.h"
#include    	"error.h"
#include    	"array.h"
#include    	"scanners.h"


VOID	f3_Number(argp)
f3_ArgTYPE  	 *argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	SP->func = f3_Number;
    	SP->arg.f = argp->f;
    	SP++;
}

VOID	f3_Array(argp)
f3_ArgTYPE  	*argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	SP->func = f3_Array;
    	SP->arg.p = (f3_PairTYPE *)argp->p;
    	SP++;
}

VOID	f3_ProcBlock(argp)
f3_ArgTYPE  	    *argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	SP->func = f3_ProcBlock;
    	SP->arg.p = (f3_PairTYPE *)argp->p;
    	SP++;
}

VOID	f3_SimpleBlock(argp)
f3_ArgTYPE	      *argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	SP->func = f3_SimpleBlock;
    	SP->arg.p = (f3_PairTYPE *)argp->p;
    	SP++;
}

VOID	f3_GlobalAddress(argp)
f3_ArgTYPE		*argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	SP->func = f3_GlobalAddress;
    	SP->arg.p = (f3_PairTYPE *)argp->p;
    	SP++;
}

VOID	f3_LocalAddress(argp)
f3_ArgTYPE  	       *argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	SP->func = f3_LocalAddress;
    	SP->arg.p = (f3_PairTYPE *)argp->p;
    	SP++;
}

VOID	f3_LocalDisp(argp)
f3_ArgTYPE	    *argp;
{    	DCHECK(S_OFLO,SP>=SL);
    	DCHECK(LADDX_LO, argp->i < 0);
    	DCHECK(LADDX_HI, argp->i >=LOCALCNT);
    	SP->func = f3_LocalAddress;
    	SP->arg.p = LOCALP - argp->i - 1;
    	SP++;
}

/***************************************************************************************/

VOID	f3_ExecProcBlock(argp)
f3_ArgTYPE	    	*argp;
{
    	register f3_PairTYPE *h;

    	h = (f3_PairTYPE *)argp->p;
    	XSAVE0;
    	XSAVE1;	    /* locals */

    	SCAN =	    f3_ScanPairs;
    	SPP =	    h;	    /* before 1st pair */
    	SET_PBL_PROC;
    	LOCALCNT = h->arg.i;
    	LOCALP += LOCALCNT;
}

VOID	f3_ExecSimpleBlock(argp)
f3_ArgTYPE	    	  *argp;
{
    	register f3_PairTYPE *h;

    	h = (f3_PairTYPE *)argp->p;
    	XSAVE0;

    	SCAN =	    f3_ScanPairs;
    	SPP = 	    h-1;    /* before 1st pair */
    	SET_PBL_BLOCK;
}


VOID   	f3_EndBlock()
{
    	if (IS_LOOP) {
    	    SPP = PBL_LOOP;
    	} else {
    	    if (SCAN==f3_ScanBytes) PSP->activecnt--;
    	    XRESTORE;
    	}
}

/***********************************************************************************/

VOID	f3_GlobalReference(argp)
f3_ArgTYPE	    	  *argp;
{	f3_PairTYPE 	 aux;
	f3_PairTYPE 	*address;

    	address = (f3_PairTYPE *)argp->p;
    	DCHECK(GADDX_LO,address <  GLOBALB);
    	DCHECK(GADDX_HI,address >= GLOBALL);

    	aux = *address;
    	if (aux.func == f3_ProcBlock)	aux.func = f3_ExecProcBlock;
    	if (aux.func == f3_SimpleBlock)	aux.func = f3_ExecSimpleBlock;
    	aux.func(&aux.arg);
}

VOID   	f3_LocalReference(argp)
f3_ArgTYPE	    	  *argp;
{
    	    	 f3_PairTYPE 	*address;
/*   	register f3_PairTYPE 	 aux;	*/
    	    	 int32	    	 disp;

    	disp = argp->i;
    	DCHECK(LADDX_LO,disp <  0);
    	DCHECK(LADDX_HI,disp >= LOCALCNT);

    	address = LOCALP - disp - 1;
	if      (address->func == f3_Number)
			f3_Number(&(address->arg));
	else if (address->func == f3_Array)
			f3_Array(&(address->arg));
	else if (address->func == f3_ProcBlock)
			f3_ExecProcBlock(&(address->arg));
	else if (address->func == f3_SimpleBlock)
			f3_ExecSimpleBlock(&(address->arg));
	else
			(address->func)(&(address->arg));
/*    	aux = *address;
    	if (aux.func == f3_ProcBlock)	aux.func = f3_ExecProcBlock;
    	if (aux.func == f3_SimpleBlock)	aux.func = f3_ExecSimpleBlock;
    	aux.func(&aux.arg);	*/
}

/***********************************************************************************/

VOID   	f3_ScanBytes()
{
    register int32           bcode;
             int32           address;
             bool            global,quoted;
    register int32           size;
    register f3_PairTYPE    *auxp,*auxq;

    bcode = f3_FontReadBINT(BYTEFILEP);

    if (f3_BYTE_IE_OR_SA(bcode)==f3_SA) {	/* short forms of address reference
    	    	    	    	    	    	   and quoted address */
        quoted =    f3_BYTE_AR_OR_QA(bcode)==f3_QA;
        address =   f3_BYTE_ADDX_BITS(bcode);
        global =    f3_ADDX_IS_GLOBAL(address);
        if (quoted) {                                       /* quoted addresses */
            if (global) {
                SPP->func =   f3_GlobalAddress;
                SPP->arg.p =  GLOBALB + address;
            } else {
                SPP->func =   f3_LocalDisp;
                SPP->arg.i =  f3_ADDX_LOCAL_DISP(address);
            }
        } else {                                          /* address references */
            if (global) {
                if (*(CONSTANT + address)) {
                    *SPP =   *(GLOBALB + address);
                    if (SPP->func == f3_ProcBlock)
                        SPP->func = f3_ExecProcBlock;
                    if (SPP->func == f3_SimpleBlock)
                        SPP->func = f3_ExecSimpleBlock;
                } else {
                    SPP->func =   f3_GlobalReference;
                    SPP->arg.p =  GLOBALB + address;
                }
            } else {
                SPP->func =   f3_LocalReference;
                SPP->arg.i =  f3_ADDX_LOCAL_DISP(address);
            }
        }
    } else {
        if (bcode <= f3_VSINT_MAX) {
            SPP->func =  f3_Number;
            SPP->arg.f = fracti(bcode);
        } else {
            switch ((int)bcode) {
    	    	case f3_BINT_ESC:
                    SPP->func = f3_Number;
                    SPP->arg.f = fracti(f3_FontReadBINT(BYTEFILEP));
                    break;
    	    	case f3_WINT_ESC:
                    SPP->func = f3_Number;
                    SPP->arg.f = fracti(f3_FontReadWINT(BYTEFILEP));
                    break;
    	    	case f3_SREAL_ESC:
                    SPP->func = f3_Number;
                    SPP->arg.f = f3_FontReadSREAL(BYTEFILEP);
                    break;
    	    	case f3_FREAL_ESC:
                    SPP->func = f3_Number;
                    SPP->arg.f = f3_FontReadFREAL(BYTEFILEP);
                    break;
    	    	case f3_BIARRAY_ESC:
                    size = f3_FontReadWINT(BYTEFILEP);
                    SPP->func = f3_Array;
                    SPP->arg.p = auxp = f3_MakeTempArray(size);
                    while (size--) {
                        auxp->func = f3_Number;
                        auxp->arg.f = fracti(f3_FontReadBINT(BYTEFILEP));
                        auxp++;
                    }
                    break;
    	    	case f3_WIARRAY_ESC:
                    size = f3_FontReadWINT(BYTEFILEP);
                    SPP->func = f3_Array;
                    SPP->arg.p = auxp = f3_MakeTempArray(size);
                    while (size--) {
                        auxp->func = f3_Number;
                        auxp->arg.f = fracti(f3_FontReadWINT(BYTEFILEP));
                        auxp++;
                    }
                    break;
    	    	case f3_SRARRAY_ESC:
                    size = f3_FontReadWINT(BYTEFILEP);
                    SPP->func = f3_Array;
                    SPP->arg.p = auxp = f3_MakeTempArray(size);
                    while (size--) {
                        auxp->func = f3_Number;
                        auxp->arg.f = f3_FontReadSREAL(BYTEFILEP);
                        auxp++;
                    }
                    break;
    	    	case f3_FRARRAY_ESC:
                    size = f3_FontReadWINT(BYTEFILEP);
                    SPP->func = f3_Array;
                    SPP->arg.p = auxp = f3_MakeTempArray(size);
                    while (size--) {
                        auxp->func =   f3_Number;
                        auxp->arg.f = f3_FontReadFREAL(BYTEFILEP);
                        auxp++;
                    }
                    break;

    	    	case f3_GAR_ESC:
                    address = f3_FontReadBINT(BYTEFILEP);
                    if (*(CONSTANT + address)) {
                        *SPP =   *(GLOBALB + address);
                        if (SPP->func == f3_ProcBlock)
                            SPP->func = f3_ExecProcBlock;
                        if (SPP->func == f3_SimpleBlock)
                            SPP->func = f3_ExecSimpleBlock;
                    } else {
                        SPP->func =  f3_GlobalReference;
                        SPP->arg.p = GLOBALB + address;
                    }
                    break;
    	    	case f3_LAR_ESC:
                    SPP->func =   f3_LocalReference;
                    SPP->arg.i = f3_FontReadBINT(BYTEFILEP);
                    break;
    	    	case f3_GQA_ESC:
                    address = f3_FontReadBINT(BYTEFILEP);
                    SPP->func  = f3_GlobalAddress;
                    SPP->arg.p = GLOBALB + address;
                    break;
    	    	case f3_LQA_ESC:
                    SPP->func  = f3_LocalDisp;
                    SPP->arg.i = f3_FontReadBINT(BYTEFILEP);
                    break;

    	    	case f3_BEGINOP:
                    auxp = SP;
                    bcode = f3_FontReadBINT(BYTEFILEP);
                    if (!f3_BYTE_SIMPLEBLOCK(bcode)) {
                        SP->func  = NULL;
                        SP->arg.i = bcode;
                        SP++;
                    }
                    do {
                        f3_ScanBytes();
			if (SPP->func==f3_Array) {
			    SPP->arg.p = f3_MakeArrayPerm(SPP->arg.p);
			}
                        *SP = *SPP;
                    } while ((SP++)->func!=f3_EndBlock);
                    size = SP - auxp;
                    if (f3_BYTE_SIMPLEBLOCK(bcode))
                        SPP->func = f3_SimpleBlock;
                    else
                        SPP->func = f3_ProcBlock;
                    SPP->arg.p = auxp = f3_MakePermArray(size);
                    SP -= size;
                    auxq = SP;
                    while (size--)
                        *auxp++ = *auxq++;
                    break;
    	    	case f3_ENDOP:
                    SPP->func = f3_EndBlock;
                    break;
            }
        }
    }
}

VOID	f3_ScanPairs()	    { SPP++; }

VOID	f3_ScanDone()	    { /*f3_XDone = TRUE; longjmp(f3_XDoneJmp,1);
				*/ longjmp(XDONEJMP,1); }

#ifdef DEBUG
int32 	SDisplay,SDepth,ALength;
char	StackImage[2000],*SIP;



    	SETSDISPLAY(s,d,l)
int32 	s,d,l;
{   	SDisplay = s;
    	SDepth = d;
    	ALength = l;
    	SIP = StackImage; *SIP = 0;
}


#define    	CATPRINTF(f,d)			\
{   	char	aux[100];			\
						\
    	sprintf(aux,f,d);			\
    	strcpy(SIP,aux); SIP += strlen(aux);	\
    	if (SIP >= (StackImage + 2000))		\
    	    SIP = StackImage + 1999;		\
    	*SIP = 0;				\
}


    	CATADDX(a)
f3_PairTYPE	*a;
{
    	if      (a>=GLOBALB && a<GLOBALL) { CATPRINTF(" (GVAR %3ld)",(long)(a-GLOBALB)); }
    	else if (a>=GLOBALL )	    	  { CATPRINTF(" (GHEAP)",0); } /* true, only because G > L always... */
    	else if (a>=LOCALB  && a<LOCALP)  { CATPRINTF(" (LVAR %3ld)",(long)(LOCALP-a-1)); }
    	else if (a>=LOCALH  && a<LOCALL)  { CATPRINTF(" (LHEAP)",0); }
    	else	    	    	          { CATPRINTF(" (???)",0); }
}
    	

    	DSTACK()
{   	f3_PairTYPE *sp;
    	int32 	     depth;

    	SIP = StackImage;

    	if (SDisplay)	{ CATPRINTF("%ld entries deep\n",(long)(SP-SB)) }
    	else	    	{ CATPRINTF("[%ld] ",(long)(SP-SB)) }
    	depth = 0;
    	for (sp=SP-1; sp>=SB; sp--) {
    	    depth++;
    	    if (depth>SDepth) {
    	    	CATPRINTF("...\n",0);
    	    	break;
    	    }
    	    if        (sp->func==f3_Number) {	CATPRINTF("N %10.4f",floatfr(sp->arg.f));
    	    } else if (sp->func==f3_Array) {	CATPRINTF("A 0x%lx",sp->arg.p);
    	    	    	    	    	    	if (SDisplay) CATADDX(sp->arg.p);
    	    	    	    	    	    	if (SDisplay>1) {
    	    	    	    	    	    	    int	i,size;
    	    	    	    	    	    	    size = f3_ArraySize(sp->arg.p);
    	    	    	    	    	    	    if (size>ALength) size=ALength;
    	    	    	    	    	    	    CATPRINTF(" (",0);
    	    	    	    	    	    	    for (i=0; i<size; i++)
    	    	    	    	    	    	    	CATPRINTF(" %10.4f",
							  floatfr((sp->arg.p+i)->arg.f));
    	    	    	    	    	    	    if (size!=f3_ArraySize(sp->arg.p))
    	    	    	    	    	    	    	CATPRINTF(" ...",0);
    	    	    	    	    	    	    CATPRINTF(")",0);
    	    	    	    	    	    	}
    	    } else if (sp->func==f3_ProcBlock) {
    	    	    	    	    	    	CATPRINTF("P 0x%lx",sp->arg.p);
    	    	    	    	    	    	if (SDisplay) CATADDX(sp->arg.p);
    	    } else if (sp->func==f3_SimpleBlock) {
    	    	    	    	    	    	CATPRINTF("B 0x%lx",sp->arg.p);
    	    	    	    	    	    	if (SDisplay) CATADDX(sp->arg.p);
    	    } else if (sp->func==f3_GlobalAddress) {
    	    	    	    	    	    	CATPRINTF("G 0x%lx",sp->arg.p);
    	    	    	    	    	    	if (SDisplay) CATADDX(sp->arg.p);
    	    } else if (sp->func==f3_LocalAddress) {
    	    	    	    	    	    	CATPRINTF("L 0x%lx",sp->arg.p);
    	    	    	    	    	    	if (SDisplay) CATADDX(sp->arg.p);
    	    } else {	   	    		CATPRINTF("? 0x%lx",sp->func);
    	    }

    	    if (SDisplay)   { CATPRINTF("\n",0); }
    	    else    	    { CATPRINTF(" * ",0); }
    	}
}
#endif
