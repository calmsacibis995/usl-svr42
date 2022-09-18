/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/array.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)array.c 1.3 89/05/23";
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
#include    	"glbvars.h"
#include    	"error.h"
#include    	"scanners.h"
#include    	"array.h"


f3_PairTYPE
    	*f3_MakeTempArray(size)
register int32	size;
{   	/* allocs <size+1> entries on the heap (the first one init to size) and
    	   returns a pointer to the second */
    	register f3_PairTYPE	*p,*q;

    	p = LOCALH -= (size+1);
    	DCHECK(L_OFLO,LOCALP>=LOCALH);
    	p->arg.i = size;
    	p++;
    	q = p;
    	while (size--) {
    	    q->func = f3_Number;
    	    q->arg.f = 0;
    	    q++;
    	}
    	return(p);
}

f3_PairTYPE
    	*f3_DupArray(h)
register f3_PairTYPE	*h;
{   	register int32	    	 size;
    	register f3_PairTYPE	*p,*q;

    	size = (h-1)->arg.i;
    	p = h-1;
    	q = LOCALH -= (size+1);
    	DCHECK(L_OFLO,LOCALP>=LOCALH);
    	*q++ = *p++;	/* copy header */
    	while (size-- > 0) {
    	    *q++ = *p++;
    	}

    	return(LOCALH+1);
}


f3_PairTYPE
    	*f3_MakePermArray(size)
register int32	size;
{   	/* allocs <size+1> entries on the global heap (the first one init to size)
	   and returns a pointer to the second */

    	GLOBALH -= (size+1);
    	DCHECK(G_OFLO,GLOBALH<GLOBALL);

    	return(GLOBALH+1);
}

f3_PairTYPE
	*f3_MakeArrayPerm(h)
register f3_PairTYPE	*h;
{   	register int32 	     	 size;
    	register f3_PairTYPE	*p,*q;

    	/* is it already permanent? */
    	if (h>GLOBALH)	    return(h);	/* relies on the fact that the local area is
    	    	    	    	    	   lower than any global */

    	/* no; make space */
    	size = (h-1)->arg.i;
    	GLOBALH -= (size+1);
    	DCHECK(G_OFLO,GLOBALH<GLOBALL);
    	p = h-1;
    	q = GLOBALH;
    	*q++ = *p++;	/* copy header */
    	while (size-- > 0) {
    	    *q++ = *p++;
    	}

    	return(GLOBALH+1);
}



