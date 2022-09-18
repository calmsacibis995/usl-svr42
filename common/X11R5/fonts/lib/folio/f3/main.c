#ident	"@(#)libfolio1.2:f3/main.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static char sccsid[] = "@(#)main.c 1.16 89/07/11";
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
#include    	<setjmp.h>
#include	<fcntl.h>

#include        "cdefs.h"
#include        <math.h>
#include	"frmath.h"

#include    	"f3.h"
#include    	"f3io.h"
#include	"common.h"

#include    	"arc.h"
#include    	"path.h"
#include    	"f3path.h"

#include    	"rname.h"
#include    	"error.h"
#include    	"glbvars.h"
#include    	"xstack.h"
#include    	"scanners.h"

#include    	"opcntrl.h"
#include    	"oparith.h"
#include    	"opgraph.h"
#include    	"opother.h"
#include    	"array.h"

f3_PathTYPE 	     f3_CurrentPath;

bool	    	     f3_GridFitting;
bool	    	     f3_ControlDust;
float		     f3_Factor1024;

path_handler	     f3_handler;

bool	    	     f3_Valid;

int32	    	     f3_DustControl;

f3_FIndexTYPE	    *f3_LastFIndexP;

trans_frTrans	     f3_AutoRel;
bbox_iBBox  	     f3_BBox;
pair_frXY    	     f3_BoxDisp;
pair_frXY            f3_LastPoint;


f3_PairTYPE 	    *f3_SaveSP;
f3_PairTYPE 	    *f3_CurveP,*f3_CurveL,
    	    	    *f3_PointP,*f3_PointB,*f3_PointL;

#define	MAX4(v1,v2,v3,v4)	(max(max(v1,v2),max(v3,v4)))
#define	MIN4(v1,v2,v3,v4)	(min(min(v1,v2),min(v3,v4)))

#ifdef PERF
#define 	    f3_MAXFONTFILESIZE		150000
unsigned char	    f3_Font[f3_MAXFONTFILESIZE];
int32		    f3_FontFileSize;
unsigned char	    *f3_CurFontP;

void    f3_ReadIntoMem( fp )
FILE	*fp;
{
    unsigned char	*fontp, *maxfontp;
    int32		c;

    fontp    = f3_Font;
    maxfontp = f3_Font + f3_MAXFONTFILESIZE;
    while ( (c = getc( fp )) != EOF ) {
	*fontp = c;
	if ( ++fontp >= maxfontp ) {
	    fprintf( stderr, "Font file longer than %d\n", f3_MAXFONTFILESIZE );
	    exit();
	}
    }
    f3_FontFileSize = ((long)fontp - (long)f3_Font) / sizeof( char );
    f3_CurFontP     = f3_Font;
}

int32	f3_MemSeek( disp, type )
int32	disp;
int32	type;
{
    int32	index;

    index = ((long)f3_CurFontP - (long)f3_Font) / sizeof( char );
    if ( type == 0 )
	index = disp;
    if ( type == 1 )
	index += disp;
    if ( type == 2 )
	index = f3_FontFileSize + disp;

    if ( index < 0 ||
	 index > f3_FontFileSize ) {
	return( -1 );
    }

    f3_CurFontP = f3_Font + index;
    return( 0 );
}
#endif /*PERF*/

#define		     MAX_FRACT_MOD 2048	/* the fonts should be able to handle this */


/*  	----------------    	*/
bool	f3_SetFont(name)
char	*name;
/*  	----------------    	*/
{
    	f3_CurrentPath.name = f3_RegisterName(name);
    	return(f3_CurrentPath.name != NULL);
}

/*  	----------------   	*/
void	f3_SetSize(size)
float	size;
/*  	----------------   	*/
{
    	f3_CurrentPath.size = size;
}

/*  	----------------   	*/
void	f3_SetCode(code)
int 	code;
/*  	----------------   	*/
{
    	f3_CurrentPath.code = code;
}

/*  	--------------------	*/
void	f3_SetGridFitting(gridfit,controldust)
bool	gridfit;
bool	controldust;
/*  	--------------------	*/
{
    	f3_GridFitting = gridfit;
    	f3_ControlDust = controldust;
}

/*  	----------------    */
int32 	f3_GetPathSize()
/*  	----------------    */
{
    	return(sizeof(f3_PathTYPE));
}

/*  	------------------  	*/
void	f3_SavePath(destp)
f3_PathTYPE *destp;
/*  	------------------  	*/
{
    	*destp = f3_CurrentPath;
}

/*  	-----------------------	    */
void	f3_RestorePath(sourcep)
f3_PathTYPE *sourcep;
/*  	-----------------------	    */
{
    	f3_CurrentPath = *sourcep;
}



/*  	-------------------------  	*/
f3_FIndexTYPE
	*f3_FindFIndex(target)
register char	*target;
/*  	-------------------------  	*/
{   register f3_FIndexTYPE  *ip,*lrup;
    register bool	      hit;
    int32                     magicNumber;
    int32                     curpos, hdrpos;

    /* find the index if already there (hit), or (!hit) the entry to be replaced,
       ie, the one least recently used */
    hit = FALSE;
    for (ip = lrup = f3_FIndexB; ip < f3_FIndexL; ip++) {
        if (ip->fontname) if (hit = (0==strcmp(ip->fontname,target))) break;
        if (ip->lastaccess < lrup->lastaccess)  lrup = ip;
    }

    if (hit) {  /* target found */
        if (f3_LastFIndexP!=ip) {	/* ...but not current */
            if (f3_LastFIndexP) {
#ifdef JHG
	fprintf(stderr,"closing \n",f3_LastFIndexP->filep);
#endif
                fclose(f3_LastFIndexP->filep);
				f3_LastFIndexP->filep = NULL;
				f3_LastFIndexP = NULL;
			}
#ifdef MSDOS
            ip->filep = fopen(target,"rb");     /* DOS: b means binary... */
            CHECK(INV_FONT,ip->filep==NULL);
#else
            ip->filep = fopen(target,"r");
#ifdef JHG
fprintf(stderr,"open target=%s\n",target);
#endif
            CHECK(INV_FONT,ip->filep==NULL);
            fcntl(fileno(ip->filep), F_SETFD);  /* close on exec */
#endif
#ifdef PERF
	    f3_ReadIntoMem( ip->filep );
#endif /*PERF*/
        }
    }
    else {	    /* target not found; load it in place of <*lrup> */
        if (lrup->filep) {
            register f3_PStateTYPE	*sp,*sp2;

#ifdef JHG
	fprintf(stderr,"closing\n", lrup->filep);
#endif
            fclose(lrup->filep);
            lrup->fontname = NULL;
			lrup->filep = NULL;

            /* remove any cached preamble state */
            for ( sp = f3_PStateB;
                  sp < f3_PStateL;
                  sp = (f3_PStateTYPE *)((char *)sp + sp->actualsize) ) {
                if (sp->fip == lrup)    sp->fip = NULL;
            }

/*#ifdef NOTDEF*/
            /* coalesce contiguous empty regions */
            for ( sp = f3_PStateB;
                  sp < f3_PStateL;
                  sp = (f3_PStateTYPE *)((char *)sp + sp->actualsize) ) {
                if (sp->fip!=NULL)
                    continue;
                sp2 = sp;
                do {
                    sp2 = (f3_PStateTYPE *)((char *)sp2 + sp2->actualsize);
                } while (sp2 < f3_PStateL && sp2->fip==NULL);
                sp->actualsize = (int32)((char *)sp2 - (char *)sp);	/* DOS */
            }
/*#endif*/
        }
        if (f3_LastFIndexP) {
#ifdef JHG
	fprintf(stderr,"close \n",f3_LastFIndexP->filep);
#endif
            fclose(f3_LastFIndexP->filep);
			f3_LastFIndexP->filep = NULL;
			f3_LastFIndexP = NULL;
		}
#ifdef MSDOS
        lrup->filep = fopen(target,"rb");       /* DOS */
        CHECK(NO_FONT,lrup->filep==NULL);
#else
#ifdef JHG
fprintf(stderr,"open2 target=%s\n",target);
#endif
        lrup->filep = fopen(target,"r");
        CHECK(NO_FONT,lrup->filep==NULL);
        fcntl(fileno(lrup->filep), F_SETFD);  /* close on exec */
#endif
#ifdef PERF
	f3_ReadIntoMem( lrup->filep );
#endif /*PERF*/
                                                            /* magic number     */
        magicNumber =         f3_FontReadLINTNE(lrup->filep);
	lrup->encrypt = ( magicNumber == f3_EMAGICNUMBER ) ?
				TRUE : FALSE;
        hdrpos = f3_FontReadLINTNE(lrup->filep);
#ifndef PERF
        curpos = ftell(lrup->filep);                    /* read the text         */
#else
	curpos = ((long)f3_CurFontP - (long)f3_Font) / sizeof( char );
#endif  /*PERF*/
        lrup->sumtextbytes = 0;
        do {
            lrup->sumtextbytes += f3_FontReadBINTNE(lrup->filep);
        } while ( ++curpos < hdrpos );

        lrup->fontbbox[0] = floatfr(f3_FontReadFREALNE(lrup->filep));
        lrup->fontbbox[1] = floatfr(f3_FontReadFREALNE(lrup->filep));
        lrup->fontbbox[2] = floatfr(f3_FontReadFREALNE(lrup->filep));
        lrup->fontbbox[3] = floatfr(f3_FontReadFREALNE(lrup->filep));

        lrup->globalvars =			/* permanent roots  */
	lrup->globalsize =    (f3_FontReadLINTNE(lrup->filep)+3)&0xFFFFFFFC;
                                                /* permanent array+proc space   */
        lrup->globalsize +=   f3_FontReadLINTNE(lrup->filep);
                                                /* temporary roots  */
        lrup->tempsize =      f3_FontReadLINTNE(lrup->filep);
                                                /* temporary array+proc space   */
        lrup->tempsize +=     f3_FontReadLINTNE(lrup->filep);
                                                /* stack entries    */
        lrup->stacksize =     f3_FontReadLINTNE(lrup->filep);
        lrup->statesize =	  f3_STATESIZE(lrup->globalvars,lrup->globalsize);
        lrup->htabdisp =      f3_FontReadLINTNE(lrup->filep);
        lrup->preambledisp =  f3_FontReadLINTNE(lrup->filep);
        lrup->htabslog =      f3_FontReadLINTNE(lrup->filep);
        lrup->htabsize =      ((int32)1) << lrup->htabslog;
        f3_FontReadLINTNE(lrup->filep);                     /* for future use  */
        f3_FontReadLINTNE(lrup->filep);
        f3_FontReadLINTNE(lrup->filep);
        f3_FontReadLINTNE(lrup->filep);
        f3_FontReadLINTNE(lrup->filep);
        f3_FontReadLINTNE(lrup->filep);

        lrup->fontname =      target;
        lrup->lastaccess =    f3_AccessCount++;
        lrup->htabcachebase = -f3_HTABCACHE;    	    /* ie, no part is mapped */
        ip = lrup;

    }

    if ( ip->encrypt )
    	f3_SetFontEncryption( TRUE );
    else
    	f3_SetFontEncryption( FALSE );
   
    f3_LastFIndexP = ip;
    return(ip);
}

/*  	--------------------	*/
VOID	f3_SeekPreamble(fip)
f3_FIndexTYPE   *fip;
/*  	--------------------	*/
{
    	fip->lastaccess = f3_AccessCount++;
#ifndef PERF
    	CHECK(INV_FONT,0!=fseek(fip->filep,fip->preambledisp,(int32)0));
#else
    	CHECK(INV_FONT,0!=f3_MemSeek(fip->preambledisp,(int32)0));
#endif  /*PERF*/
        f3_Seed( fip->sumtextbytes + fip->preambledisp );

}

/*  	-------------------------	*/
bool	f3_SeekSymbol(fip,target)
register f3_FIndexTYPE	*fip;
register int32 	    	 target;
/*  	-------------------------	*/
{   	    	 int32 	hashindex,
    	    	    	cacheindex;
    	register int32 	disp;
    	    	 int32	code;
    	register bool	found;


    	fip->lastaccess = f3_AccessCount++;

    	hashindex = target & (fip->htabsize - 1);
    	cacheindex = hashindex - fip->htabcachebase;
    	if (cacheindex < 0 || cacheindex >= f3_HTABCACHE) {
    	    int32	    i;
    	    int32	    readcount;

    	    /* remap the cached window so that it is centered around
    	       the target hashing index, but don't misuse the window */
    	    fip->htabcachebase = hashindex - (f3_HTABCACHE/2);
    	    if (fip->htabcachebase + f3_HTABCACHE > fip->htabsize)
    	    	fip->htabcachebase = fip->htabsize - f3_HTABCACHE;
    	    if (fip->htabcachebase < 0)
    	    	fip->htabcachebase = 0;

#ifndef PERF
    	    if (0!=fseek(fip->filep,fip->htabdisp + 4*fip->htabcachebase,(int32)0))
#else
    	    if (0!=f3_MemSeek(fip->htabdisp + 4*fip->htabcachebase,(int32)0))
#endif /*PERF*/
    	    	return(FALSE);
    	    readcount = min(fip->htabsize,f3_HTABCACHE);
    	    for (i = 0; i < readcount; i++)
                fip->htabcache[i] = f3_FontReadLINTNE(fip->filep);

    	    cacheindex = hashindex - fip->htabcachebase;
    	}
    	disp = fip->htabcache[cacheindex];
    	found = FALSE;
    	while (disp != 0 && ! found) {
#ifndef PERF
    	    if(0!=fseek(fip->filep,disp,(int32)0))
#else
    	    if(0!=f3_MemSeek(disp,(int32)0))
#endif /*PERF*/
    	    	return(FALSE);
    	    if ((code = f3_FontReadLINTNE(fip->filep)) == target) {
    	    	found =  TRUE;
    	    }
    	    disp = f3_FontReadLINTNE(fip->filep);
    	}
#ifndef PERF
        disp = ftell(fip->filep);
#else
	disp = ((long)f3_CurFontP - (long)f3_Font) / sizeof( char );
#endif  /*PERF*/
        f3_Seed( fip->sumtextbytes + disp );
    	return(found);
}

/*  	--------------------    	*/
VOID	f3_HeapSort(array,n)
int32   *array;
int32    n;
/*  	--------------------    	*/
{
    /* from Numerical Recipes by Press, Flannery, Teukolsky and Vetterling */
    int32   l,i,j,k, val;

    if ( n == 1 ) return;    /* so that we can handle 1 symbol. may 5/18/89  */

    l = n/2+1;
    k = n;

    while (TRUE) {
    	if (l > 1) {
    	    l--;
    	    val=array[l-1];
    	} else {
    	    val = array[k-1];
    	    array[k-1] = array[0];
    	    k--;
    	    if (k==1) {
    	    	array[0] = val;
    	    	break;
    	    }
    	}
    	i = l;
    	j = l+l;
    	while (j <= k) {
    	    if (j < k) {
    	    	if (array[j-1] < array[j])
    	    	    j++;
    	    }
    	    if (val<array[j-1]) {
    	    	array[i-1] = array[j-1];
    	    	i = j;
    	    	j += j;
    	    } else
    	    	j = k + 1;
    	}
    	array[i-1] = val;
    }
}

/*  	---------------------------------    	*/
int32	f3_GetSymbolCodes(codes,maxcount)
int32	*codes;
int32	 maxcount;
/*  	---------------------------------    	*/
{
    	f3_FIndexTYPE   *fip;
    	int32	    	disp,*codep,htabcount,symcount,pos;
    	bool	    	done;

    	/* read file header*/
    	fip = f3_FindFIndex(f3_CurrentPath.name);
    	
    	pos = f3_HTABCACHE/2;
    	symcount = 0;
    	codep = codes;
    	done = FALSE;
    	
    	while (((pos - f3_HTABCACHE/2) < fip->htabsize) && !done) {
    	    f3_SeekSymbol(fip,pos); 	/* trick to read part of hash table */
    	    htabcount = min(fip->htabsize - fip->htabcachebase,f3_HTABCACHE);
    	    pos += f3_HTABCACHE;
    	    while ((htabcount-- > 0) && !done) {
    	    	disp = fip->htabcache[htabcount];
    	    	while (disp && !done) {
#ifndef PERF
    	    	    if (0!=fseek(fip->filep,disp,(int32)0))
#else
    	    	    if (0!=f3_MemSeek(disp,(int32)0))
#endif  /*PERF*/
    	    	    	return(0);
    	    	    *codep++ = f3_FontReadLINTNE(fip->filep);
    	    	    disp = f3_FontReadLINTNE(fip->filep);
    	    	    if (symcount++ >= maxcount) {
    	    	    	done = TRUE;
    	    	    }
    	    	}
    	    }
    	}
    	if (symcount)
    	    f3_HeapSort(codes,symcount);
    	return(symcount);
}


#ifdef DEBUG
bool 	df3_DStack;
#endif

/*  	------------	    */
VOID	f3_Execute()
/*  	------------	    */
{
    	f3_ErrorCode = 0;
	if (setjmp(XDONEJMP))	return;
    	for (;;) {
    	    SCAN();
    	    SPP->func(&SPP->arg);
#ifdef DEBUG
    	    if (df3_DStack) DSTACK();
#endif
    	}
}



/*  	 ----------------------------------- 	*/
f3_PStateTYPE
    	*f3_FindPState(fip,pixratio,a,b,c,d)
register f3_FIndexTYPE	    	*fip;
	 fract	    	    	 pixratio;
    	 fract	    	    	 a,b,c,d;
/*  	 ----------------------------------- 	*/
{   	register f3_PStateTYPE	*sp,*emptyp;
    	    	 f3_PStateTYPE	*sp2;
    	    	 f3_PairTYPE 	*arrayp,*gp,*localh;
    	    	 char	    	*cp;

    	/* find the cached state and return it, or find a suitable hole if there is one */
    	emptyp = NULL;
    	for (	sp = f3_PStateB;
    	    	sp < f3_PStateL;
    	    	sp = (f3_PStateTYPE *)((char *)sp + sp->actualsize) ) {
    	    if (    sp->fip == fip  	    	&&
    	    	    sp->pixratio == pixratio 	&&
		    sp->lshift == f3_handler.LShift &&
    	    	    sp->a == a  	    	&&
    	    	    sp->b == b  	    	&&
    	    	    sp->c == c  	    	&&
    	    	    sp->d == d  	) {	sp->lastaccess = f3_AccessCount++;
    	    	    	    	    	    	sp->activecnt++;
    	    	    	    	    	    	return(sp); }

    	    if (    	emptyp == NULL	    	    	    /* no hole found so far */
    	    	    &&	sp->fip == NULL	    	    	    /* <sp> is a hole */
    	    	    &&	sp->actualsize >= fip->statesize    /* ... big enough */
    	       ) emptyp = sp;
    	}

    	/* being here means the state wasn't cached, so a new one will be made;
    	   there may or may not be space for it already available */
    	if (emptyp == NULL) {
    	    int32	    stateskilled,bestsk;
    	    int32	    newestkilled,bestnk;
    	    int32	    sizefound,bestsf;

    	    /* not enough space readily available; space will be made by destroying
    	       one or more existing states, but no remaining state will be moved, so
    	       the new state will begin at the same address as another -current or
    	       previous- state; we decide which is the best place to put the new
    	       state based on the number of states that need to be destroyed (the
    	       fewer, the better) and the time they were last used (the longer, the
    	       better)
    	       NOTE: this process is not guaranted to succeed, even if the space
    	       required for the new preamble state is less than the total space
    	       allocated for the area; this is due to the fact that multiple preamble
    	       states may be required simultaneously (composite symbols) so that
    	       "active" preamble states cannot be destroyed which, in turn, leads to
    	       possible memory segmentation problems */

    	    CHECK(NO_SPACE, fip->statesize >
                            (int32)((char *)f3_PStateL - (char *)f3_PStateB));

    	    bestsk = 10000; bestnk = f3_AccessCount;
    	    for (
    	    	    sp = f3_PStateB;
    	    	    (int32)((char *)f3_PStateL - (char *)sp) >= fip->statesize;
    	    	     sp = (f3_PStateTYPE *)((char *)sp + sp->actualsize) ) {
    	    	stateskilled = 0;
    	    	newestkilled = 0;
    	    	sizefound = 0;
    	    	sp2 = sp;
    	    	while (	(sizefound < fip->statesize) && (sp2->activecnt == 0) ) {
    	    	    sizefound += sp2->actualsize;
    	    	    if (sp2->fip) {
    	    	    	stateskilled++;
    	    	    	if (sp2->lastaccess > newestkilled)
			    newestkilled = sp2->lastaccess;
    	    	    }
    	    	    sp2 = (f3_PStateTYPE *)((char *)sp2 + sp2->actualsize);
    	    	}
    	    	if (sizefound < fip->statesize) {
		    /* an active state was found before enough space
    	    	       had been collected, so <sp2> cannot be the
    	    	       place we are looking for; the state following
    	    	       <sp2> is the first place where it makes sense
    	    	       to continue the search */
    	    	    sp = sp2;
    	    	    continue;
    	    	}

    	    	/* a possible place found; check if it is the best */
    	    	if ((stateskilled < bestsk)   ||
    	    	    (stateskilled == bestsk && newestkilled < bestnk)) {
    	    	    bestsk = stateskilled;
    	    	    bestnk = newestkilled;
    	    	    bestsf = sizefound;
    	    	    emptyp = sp;
    	    	}
    	    }

    	    /* search exhausted; have we found anything? */
    	    CHECK(NO_SPACE,emptyp==NULL);

    	    /* yes */
    	    emptyp->fip = NULL;
    	    emptyp->actualsize = bestsf;
    	}

    	emptyp->fip =	    fip;
    	emptyp->pixratio =  pixratio;
    	emptyp->a = 	    a;
    	emptyp->b = 	    b;
    	emptyp->c = 	    c;
    	emptyp->d = 	    d;
	emptyp->lshift =    f3_handler.LShift;
    	if (emptyp->actualsize > fip->statesize + f3_MINSTATE) {
    	    sp2 = (f3_PStateTYPE *)((char *)emptyp + fip->statesize);
    	    sp2->fip = NULL;
    	    sp2->actualsize = emptyp->actualsize - fip->statesize;
	    sp2->lastaccess = 0;
	    sp2->activecnt = 0;
    	    emptyp->actualsize = fip->statesize;
    	}

    	f3_SeekPreamble(fip);

#ifdef EXPERIMENT
    	XSAVE0;
    	XSAVE1;
	/* we never save State2 (file and encryption state);
	   the only action that can invalidate it is CallSymbol
	   (opcontrol) because it repositions the file */
    	XSAVE3;
#endif

    	SCAN = 	    f3_ScanDone;
    	XSAVE0;

        PSP        = emptyp;
        FIP        = fip;
        SCAN       = f3_ScanBytes;
        SPP        = &f3_ScanPair;
        SET_PBL_PROC;
        CONSTANT   = (char *)(emptyp + 1);
        GLOBALB    = (f3_PairTYPE *)(CONSTANT + fip->globalvars);
        GLOBALL    = GLOBALB + fip->globalvars;
        GLOBALH    = GLOBALB + fip->globalsize;
        LOCALCNT   = f3_FontReadBINT(FIP->filep);
        LOCALP    += LOCALCNT;

    	for (gp = GLOBALB; gp < GLOBALL; gp++)	    	     gp->func = f3_NoOp;
    	(GLOBALB + f3_ADDADDX)->func = f3_Add;
    	(GLOBALB + f3_SUBADDX)->func = f3_Sub;
    	(GLOBALB + f3_MULADDX)->func = f3_Mul;
    	(GLOBALB + f3_DIVADDX)->func = f3_Div;
    	(GLOBALB + f3_SQRTADDX)->func = f3_Sqrt;
    	(GLOBALB + f3_NEGADDX)->func = f3_Neg;
    	(GLOBALB + f3_ABSADDX)->func = f3_Abs;
    	(GLOBALB + f3_MINADDX)->func = f3_Min;
    	(GLOBALB + f3_MAXADDX)->func = f3_Max;
    	(GLOBALB + f3_CEILADDX)->func = f3_Ceil;
    	(GLOBALB + f3_FLOORADDX)->func = f3_Floor;
    	(GLOBALB + f3_TRUNCADDX)->func = f3_Trunc;
    	(GLOBALB + f3_ROUNDADDX)->func = f3_Round;
    	(GLOBALB + f3_SINADDX)->func = f3_Sin;
    	(GLOBALB + f3_COSADDX)->func = f3_Cos;
    	(GLOBALB + f3_TANADDX)->func = f3_Tan;
    	(GLOBALB + f3_ASINADDX)->func = f3_ASin;
    	(GLOBALB + f3_ACOSADDX)->func = f3_ACos;
    	(GLOBALB + f3_ATAN2ADDX)->func = f3_ATan2;
    	(GLOBALB + f3_GTADDX)->func = f3_GT;
    	(GLOBALB + f3_GEADDX)->func = f3_GE;
    	(GLOBALB + f3_EQADDX)->func = f3_EQ;
    	(GLOBALB + f3_NEADDX)->func = f3_NE;
    	(GLOBALB + f3_LTADDX)->func = f3_LT;
    	(GLOBALB + f3_LEADDX)->func = f3_LE;
    	(GLOBALB + f3_IFADDX)->func = f3_If;
    	(GLOBALB + f3_IFELSEADDX)->func = f3_IfElse;
    	(GLOBALB + f3_FOREVERADDX)->func = f3_Forever;
    	(GLOBALB + f3_BREAKADDX)->func = f3_Break;
    	(GLOBALB + f3_RETURNADDX)->func = f3_Return;
    	(GLOBALB + f3_DUPADDX)->func = f3_Dup;
    	(GLOBALB + f3_CONSTANTADDX)->func = f3_Constant;
    	(GLOBALB + f3_ASSIGNADDX)->func = f3_Assign;
    	(GLOBALB + f3_VALUEOFADDX)->func = f3_ValueOf;
    	(GLOBALB + f3_ARRAYADDX)->func = f3_MakeArray;
    	(GLOBALB + f3_INDEXADDX)->func = f3_Index;
    	(GLOBALB + f3_CATADDX)->func = f3_Cat;
    	(GLOBALB + f3_SUBARRAYADDX)->func = f3_SubArray;
    	(GLOBALB + f3_TRANSFORMADDX)->func = f3_Transform;
    	(GLOBALB + f3_PIXRATIOADDX)->func = f3_PixRatio;
    	(GLOBALB + f3_POPADDX)->func = f3_Pop;
    	(GLOBALB + f3_SIZEOFADDX)->func = f3_Sizeof;
    	(GLOBALB + f3_STACKDEPTHADDX)->func = f3_StackDepth;
    	(GLOBALB + f3_ROLLADDX)->func = f3_Roll;
    	(GLOBALB + f3_BOXADDX)->func = f3_Box;
    	(GLOBALB + f3_CALLSYMADDX)->func = f3_CallSymbol;
    	(GLOBALB + f3_EXCHANGEADDX)->func = f3_Exchange;
    	(GLOBALB + f3_SLOPEADDX)->func = f3_Slope;
    	(GLOBALB + f3_INVERTADDX)->func = f3_Invert;
    	for (	cp = CONSTANT, gp = GLOBALB;
    	    	cp < (char *)GLOBALB;
    	    	cp++, gp++  )	    *cp = (gp->func!=f3_NoOp);

    	arrayp = f3_MakeTempArray((int32)4);
    	arrayp[0].func =
    	arrayp[1].func =
    	arrayp[2].func =
    	arrayp[3].func = f3_Number;
    	arrayp[0].arg.f = a;
    	arrayp[1].arg.f = b;
    	arrayp[2].arg.f = c;
    	arrayp[3].arg.f = d;
    	SP->func = f3_Array;
    	SP->arg.p = arrayp;
    	SP++;

	localh = LOCALH; /* the preamble cannot leave anything in the local heap, ergo */
    	f3_Execute();
	LOCALH = localh;

    	emptyp->globalhsize = GLOBALB + fip->globalsize - GLOBALH;
    	emptyp->lastaccess = f3_AccessCount++;
    	emptyp->activecnt = 1;

#ifdef EXPERIMENT
    	XRESTORE;
#endif

    	return(emptyp);
}

/*-------------------------------------------------------------------------*/

void	f3_GetPoint(pp)
pair_frXY    *pp;
{
	if (f3_PointP >= f3_PointL)
		f3_PointP = f3_PointB;
    	pp->x = (f3_PointP++)->arg.f;
    	pp->y = (f3_PointP++)->arg.f;
	if (f3_Factor1024!=1) {
	    float	aux;

	    aux = floatfr(pp->x);
	    aux /= f3_Factor1024;
	    pp->x = fractf(aux);
	    aux = floatfr(pp->y);
	    aux /= f3_Factor1024;
	    pp->y = fractf(aux);
	}
}

int32 	f3_GetCurve()
{    	if (f3_CurveP >= f3_CurveL) return(f3_NOTACURVE);
    	else	    	    	    return(floorfr((f3_CurveP++)->arg.f));
}

fract	f3_GetSharpnessSquared()
{    	fract	n,d;

    	n = (f3_CurveP++)->arg.f;
    	d = (f3_CurveP++)->arg.f;

    	return(frsq(frdiv(n,d)));
}

/*-------------------------------------------------------------------------*/


/*  	-----------------   	*/
bool 	f3_NextSubpath( )
/*  	-----------------   	*/
{
    	if (! f3_Valid)	return(FALSE);
    	if (SP <= SB) 	return(FALSE);

    	SP--;
    	DCHECK(INV_FONT,    SP->func!=f3_Array);
    	f3_CurveP = SP->arg.p;
    	f3_CurveL = f3_CurveP + f3_ArraySize(SP->arg.p);
    	SP--;
    	DCHECK(INV_FONT,    SP->func!=f3_Array);
    	f3_PointP = f3_PointB = SP->arg.p;
	f3_PointL = f3_PointB + f3_ArraySize(f3_PointB);

    	f3_GetPoint(&f3_LastPoint);

    	return(TRUE);
}


/*  	----------------------    */
bool    f3_NextArc(arcp)
register arc_frSegment 	*arcp;
/*  	----------------------    */
{    	register int32 	curve;

    	if (! f3_Valid)	return(FALSE);

     	curve = f3_GetCurve();
    	if        (curve == f3_LINETOKEN) {
    	    arcp->type = arc_LINETYPE;
    	    arcp->data.line.from = f3_LastPoint;
    	    f3_GetPoint(&(arcp->data.line.to));
    	    f3_LastPoint = arcp->data.line.to;
    	} else if (curve == f3_CONICTOKEN) {
    	    arcp->type = arc_CONICTYPE;
    	    arcp->data.conic.a = f3_LastPoint;
    	    f3_GetPoint(&(arcp->data.conic.b));
    	    f3_GetPoint(&(arcp->data.conic.c));
    	    arcp->data.conic.sh2 = f3_GetSharpnessSquared();
    	    f3_LastPoint = arcp->data.conic.c;
    	} else if (curve == f3_BEZIERTOKEN) {
    	    arcp->type = arc_BEZIERTYPE;
    	    arcp->data.bezier.a = f3_LastPoint;
    	    f3_GetPoint(&(arcp->data.bezier.b));
    	    f3_GetPoint(&(arcp->data.bezier.c));
    	    f3_GetPoint(&(arcp->data.bezier.d));
    	    f3_LastPoint = arcp->data.bezier.d;
    	} else {
    	    return(FALSE);
    	}

    	return(TRUE);
}

/*  	--------------------------------  	*/
bool	f3_InitTraversal(trans,pixratio)
register trans_dTrans	*trans;
    	 float	    	 pixratio;
/*  	--------------------------------  	*/
{
    	float	    	     tmod,oldtmod;
	float		     tmod2;
	int32		     itmod;
    	char	    	    *fontname;
    	float 	    	     fsize;
    	int32 	    	     ccode;
    	f3_FIndexTYPE       *fip;
    	f3_PStateTYPE	    *psp;
    	f3_PairTYPE 	    *pp;
	fract		     fra,frb,frc,frd;


    	f3_Valid = TRUE;

    	ccode =	    f3_CurrentPath.code;
    	fontname =  f3_CurrentPath.name;
    	fsize =	    f3_CurrentPath.size;

/*
    	tmod = sqrt(trans->a*trans->d - trans->c*trans->b);
    	f3_DustControl = (tmod > 0)? path_RIGHTFDUST : path_LEFTFDUST;
    	if (tmod<0) 	tmod = -tmod;
    	tmod = trans->a*trans->d - trans->c*trans->b;
	if ( tmod < 0 ) {
	    tmod = sqrt( -tmod );
    	    f3_DustControl = path_LEFTFDUST;
	}
	else {
    	    tmod = sqrt( tmod );
    	    f3_DustControl = path_RIGHTFDUST;
	}
*/
	oldtmod = trans->a*trans->d - trans->c*trans->b;
	f3_DustControl = (oldtmod > 0)? path_RIGHTFDUST : path_LEFTFDUST;
	tmod = max(math_Abs(trans->a),math_Abs(trans->b));
        tmod2 = max(math_Abs(trans->c),math_Abs(trans->d));
	tmod = max(tmod, tmod2);
   	tmod *= fsize;
	itmod = tmod;


	f3_Factor1024 = 1;
	if (!f3_GridFitting) {
	    f3_Factor1024 = 1024/tmod;
	    if (f3_Factor1024 < 1)	f3_Factor1024 = 1.0;
	    fsize *= f3_Factor1024;
	    tmod  *= f3_Factor1024;
	    itmod = tmod;
	}
	f3_handler.LShift = 0;
	while (itmod >= MAX_FRACT_MOD) {
		itmod >>= 1;
		f3_handler.LShift++;
	}
	fsize /= (((int32)1)<<f3_handler.LShift);

	fra = fractf(fsize*trans->a);
	frb = fractf(fsize*trans->b);
	frc = fractf(fsize*trans->c);
	frd = fractf(fsize*trans->d);

    	if (!f3_ControlDust) {
    	    f3_DustControl = path_NOFDUST;
    	}

    	f3_ErrorCode = 0;
    	setjmp(f3_ErrorJmp);
    	/* here we should check for a NO_SPACE condition and, if
    	    possible, free space and retry */
    	if (f3_ErrorCode != 0)	{ f3_Valid = FALSE; return(FALSE); }


	/* nothing exist at this point */
    	XP = XL;
    	SP = SB;	/* XXXX */
    	LOCALP = LOCALB;
    	LOCALH = LOCALL;

    	fip = f3_FindFIndex(fontname);
    	psp = f3_FindPState(	fip,	fractf(pixratio),
    	    	    	    	    	fra,
    	    	    	    	    	frb,
    	    	    	    	    	frc,
    	    	    	    	    	frd   );
    	SP = SB;	/* XXXX */

    	LOCALP = LOCALB;
    	LOCALH = LOCALL;

    	CHECK(NO_SYMBOL,FALSE==f3_SeekSymbol(fip,ccode));

        SCAN       = f3_ScanDone;
        XSAVE0;
        PSP        = psp;
        FIP        = fip;
        SCAN       = f3_ScanBytes;
        SPP        = &f3_ScanPair;
        SET_PBL_PROC;
        CONSTANT   = (char *)(psp + 1);
        GLOBALB    = (f3_PairTYPE *)(CONSTANT + fip->globalvars);
        GLOBALL    = GLOBALB + fip->globalvars;
        GLOBALH    = GLOBALB + fip->globalsize - psp->globalhsize;

        LOCALCNT   = f3_FontReadBINT(FIP->filep);
        LOCALP    += LOCALCNT;

    	f3_Execute();

    	/* now the stack should contain:
    	 *
    	 *  	TOP 	array: advance vector
    	 *
    	 *  	    	array: displacement to contours' origin
    	 *
    	 *  	    	array: bounding box [minx miny maxx maxy]
    	 *
    	 *  	    	array: curves types + sharpness |
    	 *  	    	    	    	    	    	| +
    	 *  	    	array: control points	    	|
    	 *
	 *
    	 * or for a space character
	 *
    	 *  	TOP 	array: advance vector
    	 */

    	DCHECK(S_UFLO, (SP < (SB+5)) && (SP > (SB+1)));

    	/* raster advance */
    	SP--;
    	DCHECK(INV_FONT,    SP->func!=f3_Array);
    	pp = SP->arg.p;
    	DCHECK(INV_FONT,    f3_ArraySize(pp) < 2);
    	f3_AutoRel.a = fractf(trans->a);
    	f3_AutoRel.b = fractf(trans->b);
    	f3_AutoRel.c = fractf(trans->c);
    	f3_AutoRel.d = fractf(trans->d);

    	f3_AutoRel.dx =	( pp   ->arg.f);
    	f3_AutoRel.dy =	((pp+1)->arg.f);
	if (f3_Factor1024!=1) {
	    float	aux;

	    aux = floatfr(f3_AutoRel.dx);
	    aux /= f3_Factor1024;
	    f3_AutoRel.dx = fractf(aux);
	    aux = floatfr(f3_AutoRel.dy);
	    aux /= f3_Factor1024;
	    f3_AutoRel.dy = fractf(aux);
	}

    	if ( SP == SB ) {	/* this is a space character; make a null
				   bounding box and box displacement and
				   set <SaveSP> pointing to SP=SB */
	    f3_BBox.lox = f3_BBox.loy = f3_BBox.hix = f3_BBox.hiy = 0;
	    f3_BoxDisp.x = f3_BoxDisp.y = 0;
    	    f3_SaveSP = SP;

    	    return( TRUE );
	}

	/* a normal (ie, non space) character */

    	SP--;		    	/* set box displacement */
    	DCHECK(INV_FONT,    SP->func!=f3_Array);
    	pp = SP->arg.p;
    	DCHECK(INV_FONT,    f3_ArraySize(pp) < 2);
    	f3_BoxDisp.x =	( pp   ->arg.f);
    	f3_BoxDisp.y =	((pp+1)->arg.f);

    	SP--;		    	/* set bounding box */
    	DCHECK(INV_FONT,    SP->func!=f3_Array);
    	pp = SP->arg.p;
    	DCHECK(INV_FONT,    f3_ArraySize(pp) < 4);

#define floorfr_shifted(fr,s)	\
	(((fr) + (((int32)1) << (16-1-(s)))) >> (16 - (s)))
#define ceilingfr_shifted(fr,s)	\
	(((fr) + (((int32)1) << (16 - (s))) - 1) >> (16 - (s)))
   	f3_BBox.lox =	floorfr_shifted(   pp   ->arg.f,f3_handler.LShift);
    	f3_BBox.loy =	floorfr_shifted(  (pp+1)->arg.f,f3_handler.LShift);
    	f3_BBox.hix =	ceilingfr_shifted((pp+2)->arg.f,f3_handler.LShift);
    	f3_BBox.hiy =	ceilingfr_shifted((pp+3)->arg.f,f3_handler.LShift);

	f3_SaveSP = SP;		/* set SaveSP pointing to SP, just above the
				   first subpath */

	if (f3_Factor1024!=1) {	/* scale box and box displacement */
	    float	aux;

	    aux = floatfr(f3_BoxDisp.x);
	    aux /= f3_Factor1024;
	    f3_BoxDisp.x = fractf(aux);
	    aux = floatfr(f3_BoxDisp.y);
	    aux /= f3_Factor1024;
	    f3_BoxDisp.y = fractf(aux);
	    aux = floatfr(f3_BBox.lox);
	    aux /= f3_Factor1024;
	    f3_BBox.lox = fractf(aux);
	    aux = floatfr(f3_BBox.loy);
	    aux /= f3_Factor1024;
	    f3_BBox.loy = fractf(aux);
	    aux = floatfr(f3_BBox.hix);
	    aux /= f3_Factor1024;
	    f3_BBox.hix = fractf(aux);
	    aux = floatfr(f3_BBox.hiy);
	    aux /= f3_Factor1024;
	    f3_BBox.hiy = fractf(aux);
	}

    	return(TRUE);
}

/*  	---------------------	    */
void    f3_RestartTraversal()
/*  	---------------------	    */
{
    	if (!f3_Valid)	return;

    	SP = f3_SaveSP;
}

/*	---------------------------	*/
bool	f3_GetFontBBox(trans,bboxp)
trans_dTrans	*trans;
bbox_dBBox  	*bboxp;
/*	---------------------------	*/
{
    	register f3_FIndexTYPE	*fip;
	register trans_dTrans	 ltrans;
	float			 v1,v2,v3,v4;
	float		r, r1;

    	f3_Valid = TRUE;

    	f3_ErrorCode = 0;
    	setjmp(f3_ErrorJmp);
    	if (f3_ErrorCode != 0)	{ f3_Valid = FALSE; return(FALSE); }

    	fip = f3_FindFIndex(f3_CurrentPath.name);

	ltrans = *trans;
	ltrans.a *= f3_CurrentPath.size;
	ltrans.b *= f3_CurrentPath.size;
	ltrans.c *= f3_CurrentPath.size;
	ltrans.d *= f3_CurrentPath.size;

#define	LOX	fip->fontbbox[0]
#define	LOY	fip->fontbbox[1]
#define	HIX	fip->fontbbox[2]
#define	HIY	fip->fontbbox[3]
#define	MAX4(v1,v2,v3,v4)	(max(max(v1,v2),max(v3,v4)))
#define	MIN4(v1,v2,v3,v4)	(min(min(v1,v2),min(v3,v4)))

	v1 = LOX * ltrans.a + LOY*ltrans.c;
	v2 = LOX * ltrans.a + HIY*ltrans.c;
	v3 = HIX * ltrans.a + HIY*ltrans.c;
	v4 = HIX * ltrans.a + LOY*ltrans.c;
	r = min(v1,v2);
	r1 = min(v3,v4);
	bboxp->lox = min(r,r1); 
	/*bboxp->lox = MIN4(v1,v2,v3,v4);*/
	r = max(v1,v2);
	r1 = max(v3,v4);
	bboxp->hix = max(r,r1);
	/*bboxp->hix = MAX4(v1,v2,v3,v4);*/

	v1 = LOX * ltrans.b + LOY*ltrans.d;
	v2 = LOX * ltrans.b + HIY*ltrans.d;
	v3 = HIX * ltrans.b + HIY*ltrans.d;
	v4 = HIX * ltrans.b + LOY*ltrans.d;
	r = min(v1,v2);
	r1 = min(v3,v4);
	bboxp->lox = min(r,r1); 
	/*bboxp->loy = MIN4(v1,v2,v3,v4);*/
	r = max(v1,v2);
	r1 = max(v3,v4);
	bboxp->hix = max(r,r1);
	/*bboxp->hix = MAX4(v1,v2,v3,v4);*/

	return(TRUE);
}


/*  	-----------------   	*/
void	f3_GetBBox(bboxp)
bbox_iBBox  	*bboxp;
/*  	-----------------   	*/
{
    	*bboxp = f3_BBox;
}

/*  	-----------------   	*/
void	f3_GetDisp(dispp)
pair_frXY    	*dispp;
/*  	-----------------   	*/
{
    	*dispp = f3_BoxDisp;
}

/*  	----------------------	    */
void	f3_GetAutoRelative(tp)
trans_frTrans	*tp;
/*  	----------------------	    */
{
    	*tp = f3_AutoRel;
}

/*  	-------------------   	*/
int32	f3_GetDustControl()
/*  	-------------------   	*/
{
    	return(f3_DustControl);
}

extern	char *getenv();


/*  	 ------------------------------------------------------------------------	    */
bool	 f3_PathInitialize(	storeb,storel,
    	    	    	    	regfnspace,
    	    	    	    	xsavespace,
    	    	    	    	ostackentries,
    	    	    	    	tempentries,
    	    	    	    	fontindeces
    	 )
int32 	*storeb,*storel;    /* base and limit of the storage area */
int32 	 regfnspace;	    /* size (bytes) of the font name registration area */
int32 	 xsavespace; 	    /* size (bytes) of the execution state save stack */
int32 	 ostackentries;	    /* number of entries in the operand stack */
int32 	 tempentries;	    /* number of entries in the local area */
int32 	 fontindeces;	    /* max number of font index entries */
/*  	 ------------------------------------------------------------------------	    */
{   	f3_FIndexTYPE	*ip;

#ifdef DEBUG
    	char	*vp;
    	SETSDISPLAY(2,100,100);
    	vp = getenv("DSTACK");
    	df3_DStack = vp != 0;
#endif

    	f3_AccessCount = 0;

    	f3_GridFitting = TRUE;
	f3_ControlDust = TRUE;

    	f3_FNamesB =	(char *)storeb;
    	f3_FNamesL =	f3_FNamesB + regfnspace;
    	f3_XSaveB = 	(char *)f3_FNamesL;
    	f3_XSaveL = 	f3_XSaveB + xsavespace;
    	f3_OStackB =	(f3_PairTYPE *)f3_XSaveL;
    	f3_OStackL =	f3_OStackB + ostackentries;
    	f3_TempB =  	(f3_PairTYPE *)f3_OStackL;
    	f3_TempL =  	f3_TempB + tempentries;
    	f3_FIndexB =	(f3_FIndexTYPE *)f3_TempL;
    	f3_FIndexL =	f3_FIndexB + fontindeces;
    	f3_PStateB = 	(f3_PStateTYPE *)f3_FIndexL;
    	f3_PStateL = 	(f3_PStateTYPE *)storel;

    	if ((char *)f3_PStateL < (char *)f3_PStateB + f3_MINSTATE)    return(FALSE);

    	f3_LastFIndexP = NULL;
    	for (ip = f3_FIndexB; ip < f3_FIndexL; ip++) {
    	    ip->fontname = 	NULL;
    	    ip->filep = 	NULL;
    	    ip->lastaccess =    -1;
    	}
    	f3_PStateB->fip = NULL;
    	f3_PStateB->actualsize = (int32)((char *)f3_PStateL - (char *)f3_PStateB);
    	f3_PStateB->lastaccess = -1;

    	f3_XState0.restore = f3_XRestore0;
    	f3_XState1.restore = f3_XRestore1;
    	f3_XState2.restore = f3_XRestore2;
    	f3_XState3.restore = f3_XRestore3;

    	f3_handler.InitTraversal =  	f3_InitTraversal;
    	f3_handler.RestartTraversal =	f3_RestartTraversal;
    	f3_handler.NextSubpath =    	f3_NextSubpath;
    	f3_handler.NextArc =	    	f3_NextArc;
    	f3_handler.SavePath =	    	f3_SavePath;
    	f3_handler.RestorePath =    	f3_RestorePath;
    	f3_handler.GetBBox =	    	f3_GetBBox;
    	f3_handler.GetDisp =	    	f3_GetDisp;
    	f3_handler.GetAutoRelative =	f3_GetAutoRelative;
    	f3_handler.GetPathSize =    	f3_GetPathSize;
    	f3_handler.GetDustControl = 	f3_GetDustControl;
    	f3_CurrentPath.handler =    	&f3_handler;

    	return(TRUE);
}
