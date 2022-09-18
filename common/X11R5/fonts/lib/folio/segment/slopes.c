/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:segment/slopes.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)slopes.c 1.2 89/03/10";
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


#include "cdefs.h"
#include "common.h"
#include "slopes.h"


int32	slope_Quadrant( ps )
pair_iXY    *ps;
{
    /*
    	Quadrants and their boundaries (8 total) are numbered from 0 in the
    	order in which they are encountered when moving counterclockwise
    	from the positive X axis. The origin is considered to be in quadrant
    	0.
    */
    Rint32   x,y;
    x = ps->x; y = ps->y;
    if (x==0) {
    	if (y>0) return(2);
    	if (y==0) return(0);
    	return(6);
    }
    if (x>0) {
    	if (y>0) return(1);
    	if (y==0) return(0);
    	return(7);
    } 
    /* if (x<0) */ {
    	if (y>0) return(3);
    	if (y==0) return(4);	
    	return(5);
    }
}

/*
    Given a point <*p> returns the operation type that can be used to
    map the point to the first quadrant
*/
int32	slope_FindOpToQuad1( p )
pair_iXY	*p;
{
    switch ((int)(slope_Quadrant(p))) {
    	case 0:
    	case 1: return(slope_NullOp);
    	case 2:
    	case 3: return(slope_R90);
    	case 4:
    	case 5: return(slope_R180); 
    	case 6:
    	case 7: return(slope_R270);
    }
}

/*-----------------------------------------------------------------*/
/*
    Given a point <*p> returns the octant number:
    Octants and boundaries between them (16 total) are numbered
    from 0 in the ordered in which they are encountered as one
    circles counterclockwise starting with the positive X axis.
*/

int32	slope_Octant( p )
pair_iXY	*p;
{
    Rint32   x,y;
    x = p->x; y = p->y;
    if (x==0) {
    	if (y>0) return(4);
    	if (y<0) return(12);
    	/*if (y==0)*/ return(0);
    }
    if (x>0) {
    	if (y>0) {
    	    if (x>y) return(1);
    	    if (x<y) return(3);
    	    /*if (x==y)*/ return(2);
    	}
    	if (y<0) {
    	    y = -y;
    	    if (x<y) return(13);
    	    if (x>y) return(15);
    	    /*if (x==y)*/ return(14);
    	}
    	/*if (y==0)*/ return(0);
    } 
    /* if (x<0) */ {
    	x = -x;
    	if (y>0) {
    	    if (x>y) return(7);
    	    if (x<y) return(5);
    	    /*if (x==y)*/ return(6);
    	}
    	if (y<0) {
    	    y = -y;
    	    if (x>y) return(9);
    	    if (x<y) return(11);
    	    /*if (x==y)*/ return(10);
    	}
    	/*if (y==0)*/ return(8);
    } 
}

/*
    Given a point <*p> returns the operation type that can be used to
    map the point to the first octant.
*/
int32	slope_FindOpToOct1( p )
pair_iXY	*p;
{
    switch ((int)(slope_Octant(p))) {
    	case 0:
    	case 1: return(slope_NullOp);
    	case 2:
    	case 3: return(slope_F11);
    	case 4:
    	case 5: return(slope_R90); 
    	case 6:
    	case 7: return(slope_R90F11);
    	case 8:
    	case 9: return(slope_R180);
    	case 10:
    	case 11: return(slope_R180F11);
    	case 12:
    	case 13: return(slope_R270);
    	case 14:
    	case 15: return(slope_R270F11);
    }
}


/*-------------------------------------------------------------------*/


/*
    Given a point <*p> and a operation maps the point accordingly
*/
void	slope_ApplyOp ( p, op )
pair_iXY    *p;
int32	    op;
{
    int32   tmp;
    switch ((int)op) {
    	case slope_NullOp: 
    	    	    break;
    	case slope_R90:
    	    	    tmp = p->x; p->x = p->y; p->y = -tmp;
    	    	    break;
    	case slope_R180:
    	    	    p->x = -p->x; p->y = -p->y;
    	    	    break;
    	case slope_R270:
    	    	    tmp = p->x; p->x = -p->y; p->y = tmp;
    	    	    break;
    	case slope_F11:
    	    	    tmp = p->x; p->x = p->y; p->y = tmp;
    	    	    break;
    	case slope_R90F11:
    	    	    p->x = -p->x;
    	    	    break;
    	case slope_R180F11:
    	    	    tmp = p->x; p->x = -p->y; p->y = -tmp;
    	    	    break;
    	case slope_R270F11:
    	    	    p->y = -p->y;
    	    	    break;
    }
}
    	
