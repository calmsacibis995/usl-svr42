/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/glbvars.h	1.1"

/*
 * @(#)glbvars.h 1.2 89/03/10
 *
 */
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


/**************************** TYPEDEFS, ETC *************************************/

#define  f3_HTABCACHE       128         /* number of consecutive entries cached
    	    	    	    	    	   for the fonts' hashing tables        */
typedef struct findextype {
    char    *fontname;      /* font identification                              */
    FILE    *filep;         /* file pointer                                     */
    int32    lastaccess;    /* access count ("time") of most recent access      */
    float    fontbbox[4];   /* font bounding box                                */
    int32    globalvars,    /*  maximum                                         */
    	     globalsize,    /*  storage                                         */
    	     stacksize,     /*  requirements                                    */
    	     tempsize;      /*  for this font                                   */

    int32    statesize;     /* in bytes, header included                        */
    int32    preambledisp;  /* displacement, in bytes, of the font's preamble   */
    int32    htabdisp;      /* displacement, in bytes, of the hashing table     */
    int32    htabslog;
    int32    htabsize;      /* hashing table size; htabsize = 2^htabslog        */
    int32    htabcachebase; /* lowest entry cached (in htabcache[0])            */
    int32    htabcache[f3_HTABCACHE];       /* consecutive cached entries       */
    int32    sumtextbytes;  /* sum of the all the text bytes in the font file   */
    bool     encrypt;       /* record if encryption is on or not                */
}       f3_FIndexTYPE;

typedef struct pstatetype {
    f3_FIndexTYPE
    	    *fip;
    fract    pixratio;      /* pixel ratio (actual/ideal)                       */
    fract    a,b,c,d;       /* coefficients of the transformation font->raster, */
    int8     lshift;	    /*   divided by 2<<lshift */
    int32    actualsize;    /* may be slightly larger that fip->statesize in
                               order to account for all the space and not leave
                               useless holes                                    */
    int32	     globalhsize;   /* # of entries in the heap after preamble          */
    int32	     lastaccess;
    int32	     activecnt;
}   f3_PStateTYPE;

typedef struct f3_pair {
    VOID     (*func)();
    union {
        int32            i;
    	fract            f;
    	struct f3_pair  *p;
    }   arg;
}   f3_PairTYPE;

typedef union argtype {/* this union must be exactly identical to the previous one */
    int32    i;
    fract    f;
    char    *p;
}   f3_ArgTYPE;



#define	f3_STATESIZE(roots,total)       ( sizeof(f3_PStateTYPE) +       \
                                          sizeof(f3_PairTYPE)*(total) + \
                                          (roots) )

#define	f3_MINSTATE                     sizeof(f3_PStateTYPE)

/********************************************************************************/

typedef struct xstatetype {      /* model for all other states */
    bool                 (*restore)();
}   f3_XStateTYPE;

typedef struct xstate0type {      /* EXECUTION STATE - LEVEL 0 DATA
                       * Always saved first / restored last
                       */
    bool                 (*restore)();
    f3_PStateTYPE       *psp;
    VOID                 (*scanner)();
    f3_PairTYPE         *scanpairp;
    union {
        f3_PairTYPE     *blockbeginp;
        unsigned int32   proc_or_block;
    }   pbl;
}   f3_XState0TYPE;

typedef struct xstate1type {      /* EXECUTION STATE - LEVEL 1 DATA
                       * Saved/Restored when callto/retfrom $pre/$sym$proc level
                       */
    bool                 (*restore)();
    int32                localcnt;
    f3_PairTYPE         *localp;
}   f3_XState1TYPE;

typedef struct xstate2type {      /* EXECUTION STATE - LEVEL 2 DATA
    	    	       * Saved by CallSymbol; this is the only case in which
			 the input state (file position and encryption state,
			 mainly, but also fip for good measure, even when we
			 don't do calls across files) needs to be restored
			 upon SYMBOL completion. It is also the only way to
			 call f3_Execute recursively, thus the inclusion of
			 <xdonejmp>
    	    	       */
    bool    	    	 (*restore)();
    f3_FIndexTYPE       *fip;
    int32                fdisp;
    int32		 encryptstate;
    jmp_buf		 xdonejmp;
}   f3_XState2TYPE;

typedef struct xstate3type {      /* EXECUTION STATE - LEVEL 3 DATA
                       * Saved/Restored when calling/returning a new symbol
                       * (CALLSYM)
                       */
    bool                 (*restore)();
    bool                *constant;
    f3_PairTYPE         *globalb,
                        *globall,
                        *globalh;
}   f3_XState3TYPE;

/********************************************************************************/

int32                    f3_AccessCount;        /* an ever-increasing "time"    */

f3_PairTYPE              f3_ScanPair;

int32                    f3_ErrorCode;
jmp_buf                  f3_ErrorJmp;

char                    *f3_FNamesB,            /* Registered font names        */
                        *f3_FNamesL;

f3_XState0TYPE           f3_XState0;            /* Current state                */
f3_XState1TYPE           f3_XState1;
f3_XState2TYPE           f3_XState2;
f3_XState3TYPE           f3_XState3;

char                    *f3_XSaveB,             /* XState save stack            */
                        *f3_XSaveP,
                        *f3_XSaveL;

f3_PairTYPE             *f3_OStackB,            /* Operand stack                */
                        *f3_OStackP,
                        *f3_OStackL;

f3_PairTYPE             *f3_TempB,       /* Temporary area (local vars + heap)  */
                        *f3_TempL;
f3_PairTYPE		*f3_LocalH;

f3_FIndexTYPE           *f3_FIndexB,            /* Font indeces                 */
                        *f3_FIndexL;

f3_PStateTYPE           *f3_PStateB,            /* Preamble states              */
                        *f3_PStateL;

#define	PSP              f3_XState0.psp
#define	SCAN             f3_XState0.scanner
#define	SPP              f3_XState0.scanpairp
#define	PBL_LOOP         f3_XState0.pbl.blockbeginp
#define	SET_PBL_BLOCK    f3_XState0.pbl.proc_or_block = 0
#define	SET_PBL_PROC     f3_XState0.pbl.proc_or_block = 1
#define	IS_BLOCK         f3_XState0.pbl.proc_or_block == 0
#define	IS_PROC          f3_XState0.pbl.proc_or_block == 1
#define	IS_LOOP          f3_XState0.pbl.proc_or_block > 1

#define	LOCALCNT         f3_XState1.localcnt
#define	LOCALP           f3_XState1.localp
#define	LOCALH           f3_LocalH
#define	LOCALB           f3_TempB
#define	LOCALL           f3_TempL

#define	FIP              f3_XState2.fip
#define	BYTEFILEP        f3_XState2.fip->filep
#define	XDONEJMP	 f3_XState2.xdonejmp

#define	CONSTANT         f3_XState3.constant
#define	GLOBALB          f3_XState3.globalb
#define	GLOBALL          f3_XState3.globall
#define	GLOBALH          f3_XState3.globalh

#define	SP               f3_OStackP
#define	SB               f3_OStackB
#define	SL               f3_OStackL

#define	XP               f3_XSaveP
#define	XL               f3_XSaveL
#define	XB               f3_XSaveB

/********************************************************************************/



