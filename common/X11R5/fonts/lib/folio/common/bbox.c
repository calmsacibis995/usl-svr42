/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:common/bbox.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)bbox.c 1.2 89/03/10";
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

/*---------------------------------------------------------------
    	BOUNDING BOXES
---------------------------------------------------------------*/

/* 
    filling in a bounding box <*b> given the lox <x>, loy <y>,
    x dimension <xsz> and y dimension <ysz>
*/

void	bbox_dFill(b, x, y, xsz, ysz)
bbox_dBBox  *b;
double	x, y, xsz, ysz;
{
    b->lox = x;
    b->loy = y;
    b->hix = x + xsz;
    b->hiy = y + ysz;
}


void	bbox_iFill(b,x,y,xsz,ysz)
bbox_iBBox  *b;
int32	x, y, xsz, ysz;
{
    b->lox = x;
    b->loy = y;
    b->hix = x + xsz;
    b->hiy = y + ysz;
}

/*-------------------------------------------------------------*/

/* 
    expanding a bounding box <*b> to include a point <*p>
*/

void	bbox_dExpandToIncludePoint(b, p)
bbox_dBBox  *b;
pair_dXY    *p;
{
    if ( p->x > b->hix )
    	b->hix = p->x;
    else if ( p->x < b->lox )
    	b->lox = p->x;
    if ( p->y > b->hiy )
    	b->hiy = p->y;
    else if ( p->y < b->loy )
    	b->loy = p->y;

}

void	bbox_iExpandToIncludePoint(b, p)
bbox_iBBox  *b;
pair_iXY    *p;
{
    if ( p->x > b->hix )
    	b->hix = p->x;
    else if ( p->x < b->lox )
    	b->lox = p->x;
    if ( p->y > b->hiy )
    	b->hiy = p->y;
    else if ( p->y < b->loy )
    	b->loy = p->y;

}

/*-------------------------------------------------------------*/

/* 
    displace bbox <*b1> by a pair XY <*v> to get new bbox <*b2> 
*/

void	bbox_dDisplace(b2, b1, v)
bbox_dBBox  *b2, *b1;
pair_dXY    *v;
{
    b2->lox = b1->lox + v->x;
    b2->loy = b1->loy + v->y;
    b2->hix = b1->hix + v->x;
    b2->hiy = b1->hiy + v->y;
}

void	bbox_iDisplace(b2, b1, v)
bbox_iBBox  *b2, *b1;
pair_iXY    *v;
{
    b2->lox = b1->lox + v->x;
    b2->loy = b1->loy + v->y;
    b2->hix = b1->hix + v->x;
    b2->hiy = b1->hiy + v->y;
}

/*-------------------------------------------------------------*/

/*
    scale bbox <*b1> by a scale factor <s> to get new bbox <*b2> 
*/
    
void	bbox_dScale(b2, b1, s)
bbox_dBBox  *b2, *b1;
double	s;
{
    b2->lox = b1->lox * s;
    b2->loy = b1->loy * s;
    b2->hix = b1->hix * s;
    b2->hiy = b1->hiy * s;
}

void	bbox_iScale(b2, b1, s)
bbox_iBBox  *b2, *b1;
int32	s;
{
    b2->lox = b1->lox * s;
    b2->loy = b1->loy * s;
    b2->hix = b1->hix * s;
    b2->hiy = b1->hiy * s;
}

/*-------------------------------------------------------------*/

/*
    enlarge/scale  bbox <*b1> by a value <d>  to get new bbox <*b2> 
*/

void	bbox_dEnlarge(b2, b1, d)
bbox_dBBox  *b2, *b1;
double	d;
{
    b2->lox = b1->lox - d;
    b2->loy = b1->loy - d;
    b2->hix = b1->hix + d;
    b2->hiy = b1->hiy + d;
}

void	bbox_iEnlarge(b2, b1, d)
bbox_iBBox  *b2, *b1;
int32	    d;
{
    b2->lox = b1->lox - d;
    b2->loy = b1->loy - d;
    b2->hix = b1->hix + d;
    b2->hiy = b1->hiy + d;
}

void	bbox_dShrink(b2, b1, d)
bbox_dBBox  *b2, *b1;
double	d;
{
    b2->lox = b1->lox + d;
    b2->loy = b1->loy + d;
    b2->hix = b1->hix - d;
    b2->hiy = b1->hiy - d;
}

void	bbox_iShrink(b2, b1, d)
bbox_iBBox  *b2, *b1;
int32	d;
{
    b2->lox = b1->lox + d;
    b2->loy = b1->loy + d;
    b2->hix = b1->hix - d;
    b2->hiy = b1->hiy - d;
}

/*-------------------------------------------------------------*/

/*
    union of  bboxes <*b1> and <*b2> get new bbox <*b3> 
*/


void	bbox_dUnion(b3, b1, b2)
bbox_dBBox  *b3, *b2, *b1;
{
    b3->lox = min(b1->lox, b2->lox);
    b3->loy = min(b1->loy, b2->loy);
    b3->hix = max(b1->hix, b2->hix);
    b3->hiy = max(b1->hiy, b2->hiy);
}


void	bbox_iUnion(b3, b1, b2)
bbox_iBBox  *b3, *b2, *b1;
{
    b3->loy = min(b1->loy, b2->loy);
    b3->lox = min(b1->lox, b2->lox);
    b3->hix = max(b1->hix, b2->hix);
    b3->hiy = max(b1->hiy, b2->hiy);
}


/*-------------------------------------------------------------*/

/*
    intersection of  bboxes <*b1> and <*b2> get new bbox <*b3> 
*/


void	bbox_dIntersect(b3, b1, b2)
bbox_dBBox  *b3, *b2, *b1;
{
    b3->lox = max(b1->lox, b2->lox);
    b3->loy = max(b1->loy, b2->loy);
    b3->hix = min(b1->hix, b2->hix);
    b3->hiy = min(b1->hiy, b2->hiy);
}


void	bbox_iIntersect(b3, b1, b2)
bbox_iBBox  *b3, *b2, *b1;
{
    b3->lox = max(b1->lox, b2->lox);
    b3->loy = max(b1->loy, b2->loy);
    b3->hix = min(b1->hix, b2->hix);
    b3->hiy = min(b1->hiy, b2->hiy);
}

/*-------------------------------------------------------------*/

/*
    gridfitting of an double bbox <*db> to integer bbox <*ib> 
*/

void	bbox_GridFit(ib, db)
bbox_iBBox  *ib;
bbox_dBBox  *db;
{
    ib->lox = math_Floor(db->lox);
    ib->loy = math_Floor(db->loy);
    ib->hix = math_Ceiling(db->hix);
    ib->hiy = math_Ceiling(db->hiy);
}

/*
    gridfitting of an fixed point bbox <*fixb> to integer bbox <*ib> 
    using <f> bits of fraction
*/

void	bbox_GridFitFixedPoint(ib, fixb, f)
bbox_iBBox  *ib, *fixb;
int32	    f;
{
    if ( f != 0 ) {
    	ib->lox = math_FloorFixedPoint(fixb->lox, f);
    	ib->loy = math_FloorFixedPoint(fixb->loy, f);
    	ib->hix = math_CeilingFixedPoint(fixb->hix, f);
    	ib->hiy = math_CeilingFixedPoint(fixb->hiy, f);
    	ib->lox >>= f;
    	ib->loy >>= f;
    	ib->hix >>= f;
    	ib->hiy >>= f;
    } else {
    	ib->lox = fixb->lox;
    	ib->loy = fixb->loy;
    	ib->hix = fixb->hix;
    	ib->hiy = fixb->hiy;
    }
}

/*-------------------------------------------------------------*/

/* 
    transform a bounding box <*b1> to bbox <*b2> using
    a transformation <*t>
*/

void	bbox_Transform(b2, b1, t)
bbox_dBBox  	*b2, *b1;
trans_dTrans	*t;
{
    pair_dXY	p1, p2, p3, p4;
    bbox_dBBox	b3, b4;

    p1.x = b1->lox;
    p1.y = b1->loy;
    p2.x = p1.x;
    p2.y = b1->hiy;
    p3.x = b1->hix;
    p3.y = p2.y;
    p4.x = p3.x;
    p4.y = b1->loy;
    trans_Apply(t, &p1);
    trans_Apply(t, &p2);
    trans_Apply(t, &p3);
    trans_Apply(t, &p4);
    bbox_FillFrom2Points(&b3, &p1, &p2);
    bbox_FillFrom2Points(&b4, &p3, &p4);
    bbox_dUnion(b2, &b3, &b4);
}


void	bbox_TransformFloating(b2, b1, t)
bbox_dBBox  	*b2, *b1;
trans_dTrans	*t;
{
    pair_dXY	p1, p2, p3, p4;
    bbox_dBBox	b3, b4;

    p1.x = b1->lox;
    p1.y = b1->loy;
    p2.x = p1.x;
    p2.y = b1->hiy;
    p3.x = b1->hix;
    p3.y = p2.y;
    p4.x = p3.x;
    p4.y = b1->loy;
    trans_ApplyFloating(t, &p1);
    trans_ApplyFloating(t, &p2);
    trans_ApplyFloating(t, &p3);
    trans_ApplyFloating(t, &p4);
    bbox_FillFrom2Points(&b3, &p1, &p2);
    bbox_FillFrom2Points(&b4, &p3, &p4);
    bbox_dUnion(b2, &b3, &b4);
}



