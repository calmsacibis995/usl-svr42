#ident	"@(#)libfolio1.2:common/trans.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static char sccsid[] = "@(#)trans.c 1.3 89/05/25";
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


#include    "cdefs.h"
#include    "common.h"


/*----------------------------------------------------------------*/

/*
    Construction of transformation matrices
*/


trans_dTrans trans_Identity = {1.0,0.0,0.0,1.0,0.0,0.0};


void	trans_MakeTranslation (dx,dy,tm)
double	    	dx,dy;
trans_dTrans  	*tm;
{
    *tm = trans_Identity;
    tm->dx = dx; tm->dy = dy;
}


void	trans_MakeScale (sx,sy,tm)
double	    	sx,sy;
trans_dTrans  	*tm;
{
    *tm = trans_Identity;
    tm->a = sx; tm->d = sy;
}


void	trans_MakeRotation (ang,tm)
double	    	ang;
trans_dTrans	*tm;
{
    double   sn,cs;
    sn = math_Sin(ang);
    cs = math_Cos(ang);
    tm->a = tm->d = cs;
    tm->b = -sn; tm->c = sn;
    tm->dx = tm->dy  = 0.0;
}


/*----------------------------------------------------------------*/

/*
    Operations on transformation matrices
*/


void	trans_Multiply (m1,m2,m3)
trans_dTrans  *m1,*m2,*m3;
{
    /*
    	Performs m3 = m1*m2 in such a way that m3 may be one of the
    	operands and things will still work out correctly.
    */
    trans_dTrans  tmp;
    tmp.a = ((m1->a)*(m2->a)) + ((m1->b)*(m2->c));
    tmp.b = ((m1->a)*(m2->b)) + ((m1->b)*(m2->d));
    tmp.c = ((m1->c)*(m2->a)) + ((m1->d)*(m2->c));
    tmp.d = ((m1->c)*(m2->b)) + ((m1->d)*(m2->d));
    tmp.dx = ((m1->dx)*(m2->a)) + ((m1->dy)*(m2->c)) + (m2->dx);
    tmp.dy = ((m1->dx)*(m2->b)) + ((m1->dy)*(m2->d)) + (m2->dy);
    *m3 = tmp;
}


bool	trans_Invert(cmt,res)
trans_dTrans  *cmt,*res;
{
    double   den;
    den = ((cmt->a)*(cmt->d)) - ((cmt->b)*(cmt->c));
    if ( den == 0 ) {
    	*res = trans_Identity;
    	return(false);
    }
    res->a = cmt->d / den;
    res->b = - (cmt->c / den);
    res->c = - (cmt->b / den);
    res->d = cmt->a / den;
    res->dx = (((cmt->dx)*(cmt->d)) - ((cmt->dy)*(cmt->c))) / den;
    res->dy = (((cmt->dy)*(cmt->a)) - ((cmt->dx)*(cmt->b))) / den;
    return(true);
}


void	trans_Concatenate (mt,cmt)
trans_dTrans  *mt,*cmt;
{
    trans_dTrans  tmp;
    tmp.a = ((mt->a)*(cmt->a)) + ((mt->b)*(cmt->c));
    tmp.b = ((mt->a)*(cmt->b)) + ((mt->b)*(cmt->d));
    tmp.c = ((mt->c)*(cmt->a)) + ((mt->d)*(cmt->c));
    tmp.d = ((mt->c)*(cmt->b)) + ((mt->d)*(cmt->d));
    tmp.dx = ((mt->dx)*(cmt->a)) + ((mt->dy)*(cmt->c)) + (cmt->dx);
    tmp.dy = ((mt->dx)*(cmt->b)) + ((mt->dy)*(cmt->d)) + (cmt->dy);
    *cmt = tmp;
}


void	trans_Translate(x,y,cmt)
double	    	x,y;
trans_dTrans 	*cmt;
{
    cmt->dx += (cmt->a)*x + (cmt->c)*y;
    cmt->dy += (cmt->b)*x + (cmt->d)*y;
}


void	trans_Scale(sx,sy,cmt)
double	    	sx,sy;
trans_dTrans	*cmt;
{
    cmt->a *= sx;
    cmt->b *= sx;
    cmt->c *= sy;
    cmt->d *= sy;
}


void	trans_Rotate(ang,cmt)
double	    	ang;
trans_dTrans	*cmt;
{
    double   sn,cs;
    trans_dTrans  tmp;
    sn = sin(ang);
    cs = cos(ang);
    tmp.a = (cs*(cmt->a)) + (sn*(cmt->c));
    tmp.b = (cs*(cmt->b)) + (sn*(cmt->d));
    tmp.c = -(sn*(cmt->a)) + (cs*(cmt->c));
    tmp.d = -(sn*(cmt->b)) + (cs*(cmt->d));
    tmp.dx = cmt->dx; tmp.dy = cmt->dy;
    *cmt = tmp;
}


void	trans_GridFit(cmt)
trans_dTrans	*cmt;
{
    cmt->dx = math_iRound(cmt->dx);
    cmt->dy = math_iRound(cmt->dy);
}


/*----------------------------------------------------------------*/

/*
    Operations of transformation matrices on points and distances
*/


void	trans_Apply(cmt,p)
trans_dTrans	*cmt;
pair_dXY    	*p;
{
    pair_dXY p1;
    p1 = *p;
    p->x = ((cmt->a)*(p1.x))+((cmt->c)*(p1.y))+(cmt->dx);
    p->y = ((cmt->b)*(p1.x))+((cmt->d)*(p1.y))+(cmt->dy);
}


void	trans_ApplyFloating(cmt,p)
trans_dTrans	*cmt;
pair_dXY    	*p;
{
    pair_dXY p1;
    p1 = *p;
    p->x = ((cmt->a)*(p1.x))+((cmt->c)*(p1.y));
    p->y = ((cmt->b)*(p1.x))+((cmt->d)*(p1.y));
}


void	trans_ApplyDisplacement(cmt,p)
trans_dTrans  	*cmt;
pair_dXY    	*p;
{
    pair_dXY p1;
    p1 = *p;
    p->x += cmt->dx;
    p->y += cmt->dy;
}


void	trans_dTofr(frmt,dmt)
trans_frTrans  	*frmt;
trans_dTrans  	*dmt;
{
	frmt->a = math_iRound(fractf(dmt->a));
	frmt->b = math_iRound(fractf(dmt->b));
	frmt->c = math_iRound(fractf(dmt->c));
	frmt->d = math_iRound(fractf(dmt->d));
	frmt->dx = math_iRound(fractf(dmt->dx));
	frmt->dy = math_iRound(fractf(dmt->dy));
}


void	trans_frTod(dmt,frmt)
trans_dTrans  	*dmt;
trans_frTrans  	*frmt;
{
	dmt->a = (double)(floatfr(frmt->a));
	dmt->b = (double)(floatfr(frmt->b));
	dmt->c = (double)(floatfr(frmt->c));
	dmt->d = (double)(floatfr(frmt->d));
	dmt->dx = (double)(floatfr(frmt->dx));
	dmt->dy = (double)(floatfr(frmt->dy));
}
