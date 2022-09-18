/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/si/mizerarc.c	1.2"

/*
 *	Copyright (c) 1991 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990, 1991 INTERACTIVE Systems Corporation
 *	All rights reserved.
 */

/************************************************************
Copyright 1989 by The Massachusetts Institute of Technology
Author:  Bob Scheifler, MIT X Consortium
********************************************************/
/* $XConsortium: mizerarc.c,v 5.24 89/11/25 12:33:17 rws Exp $ */

/* Derived from:
 * "Algorithm for drawing ellipses or hyperbolae with a digital plotter"
 * by M. L. V. Pitteway
 * The Computer Journal, November 1967, Volume 10, Number 3, pp. 282-289
 */

#include <math.h>
#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "gcstruct.h"
#include "pixmapstr.h"
/* SI: start */
#include "regionstr.h"
#include "windowstr.h"
#include "si.h"			/* SI (change to mi.h?) */
#include "sidep.h"
#include "mizerarc.h"		/* SI (change to sizerarc.h?) */
/* SI: end */

#define FULLCIRCLE (360 * 64)
#define OCTANT (45 * 64)
#define QUADRANT (90 * 64)
#define HALFCIRCLE (180 * 64)
#define QUADRANT3 (270 * 64)

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

/* int siGCPrivateIndex = 0;*/	/* SI */

/* SI (the following defines are surrounded by notdef) */
#ifdef notdef
/*
 *	These are the macros that came from MIT.  They seem to cause
 *	a problem with the V.3.2 compiler that results in a floating
 *	exception.  Since the 387 appears to be accurate at the
 *	four axes anyway, I'm just going to call the sin/cos functions
 *	directly.
 */
#define Dsin(d) ((d) == 0 ? 0.0 : ((d) == QUADRANT ? 1.0 : \
                 ((d) == HALFCIRCLE ? 0.0 : \
                 ((d) == QUADRANT3 ? -1.0 : sin((double)d*(M_PI/11520.0))))))

#define Dcos(d) ((d) == 0 ? 1.0 : ((d) == QUADRANT ? 0.0 : \
		 ((d) == HALFCIRCLE ? -1.0 : \
		 ((d) == QUADRANT3 ? 0.0 : cos((double)d*(M_PI/11520.0))))))
#endif

/* SI: start */
#define Dsin(d) sin((double)d*(M_PI/11520.0))
#define Dcos(d) cos((double)d*(M_PI/11520.0))
/* SI: end */

typedef struct {
    DDXPointRec startPt;
    DDXPointRec endPt;
    int dashIndex;
    int dashOffset;
    int dashIndexInit;
    int dashOffsetInit;
} DashInfo;

static miZeroArcPtRec oob = {65536, 65536, 0};

/*
 * (x - l)^2 / (W/2)^2  + (y + H/2)^2 / (H/2)^2 = 1
 *
 * where l is either 0 or .5
 *
 * alpha = 4(W^2)
 * beta = 4(H^2)
 * gamma = 0
 * u = 2(W^2)H
 * v = 4(H^2)l
 * k = -4(H^2)(l^2)
 *
 */

Bool
miZeroArcSetup(arc, info, ok360)
    register xArc *arc;
    register miZeroArcRec *info;
    Bool ok360;
{
    int l;
    int angle1, angle2;
    int startseg, endseg;
    int startAngle, endAngle;
    int i, overlap;
    miZeroArcPtRec start, end;
    double tmp1, tmp2;		/*CSAM, resolve weird problems.
                                         grep CSAMnotes for the reason*/

    l = arc->width & 1;
    if (arc->width == arc->height)
    {
	info->alpha = 4;
	info->beta = 4;
	info->k1 = -8;
	info->k3 = -16;
	info->b = 12;
	info->a = (arc->width << 2) - 12;
	info->d = 17 - (arc->width << 1);
	if (l)
	{
	    info->b -= 4;
	    info->a += 4;
	    info->d -= 7;
	}
    }
    else if (!arc->width || !arc->height)
    {
	info->alpha = 0;
	info->beta = 0;
	info->k1 = 0;
	info->k3 = 0;
	info->a = -arc->height;
	info->b = 0;
	info->d = -1;
    }
    else
    {
	/* initial conditions */
	info->alpha = (arc->width * arc->width) << 2;
	info->beta = (arc->height * arc->height) << 2;
	info->k1 = info->beta << 1;
	info->k3 = info->k1 + (info->alpha << 1);
	info->b = l ? 0 : -info->beta;
	info->a = info->alpha * arc->height;
	info->d = info->b - (info->a >> 1) - (info->alpha >> 2);
	if (l)
	    info->d -= info->beta >> 2;
	info->a -= info->b;
	/* take first step, d < 0 always */
	info->b -= info->k1;
	info->a += info->k1;
	info->d += info->b;
	/* octant change, b < 0 always */
	info->k1 = -info->k1;
	info->k3 = -info->k3;
	info->b = -info->b;
	info->d = info->b - info->a - info->d;
	info->a = info->a - (info->b << 1);
    }
    info->dx = 1;
    info->dy = 0;
    info->w = (CARD16)(arc->width + 1) >> 1;		/* ANSI */
    info->h = arc->height >> 1;
    info->xorg = arc->x + (arc->width >> 1);
    info->yorg = arc->y;
    info->xorgo = info->xorg + l;
    info->yorgo = info->yorg + arc->height;
    if (!arc->width && !arc->height)
    {
	info->x = 0;
	info->y = 0;
	info->initialMask = 1;
	info->start = oob;
	info->end = oob;
	return FALSE;
    }
    if (!arc->width && arc->height)
    {
	info->x = 0;
	info->y = 1;
    }
    else
    {
	info->x = 1;
	info->y = 0;
    }
    angle1 = arc->angle1;
    angle2 = arc->angle2;
    if (angle2 > FULLCIRCLE)
	angle2 = FULLCIRCLE;
    else if (angle2 < -FULLCIRCLE)
	angle2 = -FULLCIRCLE;
    if (angle2 < 0)
    {
	startAngle = angle1 + angle2;
	endAngle = angle1;
    }
    else
    {
	startAngle = angle1;
	endAngle = angle1 + angle2;
    }
    if (startAngle < 0)
	startAngle = FULLCIRCLE - (-startAngle) % FULLCIRCLE;
    if (startAngle >= FULLCIRCLE)
	startAngle = startAngle % FULLCIRCLE;
    if (endAngle < 0)
	endAngle = FULLCIRCLE - (-endAngle) % FULLCIRCLE;
    if (endAngle >= FULLCIRCLE)
	endAngle = endAngle % FULLCIRCLE;
    info->startAngle = startAngle;
    info->endAngle = endAngle;
    if (ok360 && (startAngle == endAngle) && arc->angle2 &&
	arc->width && arc->height)
    {
	info->initialMask = 0xf;
	info->start = oob;
	info->end = oob;
	return TRUE;
    }
    startseg = startAngle / OCTANT;
    if (!arc->height || (((startseg + 1) & 2) && arc->width))
    {
/*CSAM, the original code causes floating exception.
	I wrote a small test that contains 200 following lines.
	It also caused floating exception. Stan and I suspected
	this has something to do with math chip. Fortunately, I find
	the workaround, so I didn't get the bottom of this from the
	language folks. Someone may want to do that.
	I also checked the C4, the results were identical
CSAMnotes*/
#ifdef notdef	/*CSAM, fgrep CSAMnotes for the reasons*/
	start.x = Dcos(startAngle) * ((arc->width + 1) / 2.0);
#else
     	start.x = (tmp1=Dcos(startAngle)) * (tmp2=(double)((CARD16)(arc->width + 1)) / 2.0);  /*ANSI*/
#endif
	if (start.x < 0)
	    start.x = -start.x;
	start.y = -1;
    }
    else
    {
#ifdef notdef	/*CSAM, fgrep CSAMnotes for the reasons*/
	start.y = Dsin(startAngle) * (arc->height / 2.0);
#else
	start.y = (tmp1=Dsin(startAngle)) * (tmp2=(double)arc->height / 2.0);
/*ANSI*/
#endif
	if (start.y < 0)
	    start.y = -start.y;
	start.y = info->h - start.y;
	start.x = 65536;
    }
    endseg = endAngle / OCTANT;
    if (!arc->height || (((endseg + 1) & 2) && arc->width))
    {
#ifdef notdef	/*CSAM, fgrep CSAMnotes for the reasons*/
	end.x = Dcos(endAngle) * ((arc->width + 1) / 2.0);
#else
	end.x = (tmp1=Dcos(endAngle)) * (tmp2=(double)((CARD16)(arc->width + 1)) / 2.0);   /*ANSI*/
#endif
	if (end.x < 0)
	    end.x = -end.x;
	end.y = -1;
    }
    else
    {
#ifdef notdef	/*CSAM, fgrep CSAMnotes for the reasons*/
	end.y = Dsin(endAngle) * (arc->height / 2.0);
#else
	end.y = (tmp1=Dsin(endAngle)) * (tmp2=(double)arc->height / 2.0);
/*ANSI*/
#endif
	if (end.y < 0)
	    end.y = -end.y;
	end.y = info->h - end.y;
	end.x = 65536;
    }
    info->firstx = start.x;
    info->firsty = start.y;
    info->initialMask = 0;
    overlap = arc->angle2 && (endAngle <= startAngle);
    for (i = 0; i < 4; i++)
    {
	if (overlap ?
	    ((i * QUADRANT <= endAngle) || ((i + 1) * QUADRANT > startAngle)) :
	    ((i * QUADRANT <= endAngle) && ((i + 1) * QUADRANT > startAngle)))
	    info->initialMask |= (1 << i);
    }
    start.mask = info->initialMask;
    end.mask = info->initialMask;
    startseg >>= 1;
    endseg >>= 1;
    overlap = overlap && (endseg == startseg);
    if (start.x != end.x || start.y != end.y || !overlap)
    {
	if (startseg & 1)
	{
	    if (!overlap)
		info->initialMask &= ~(1 << startseg);
	    if (start.x > end.x || start.y > end.y)
		end.mask &= ~(1 << startseg);
	}
	else
	{
	    start.mask &= ~(1 << startseg);
	    if (((start.x < end.x || start.y < end.y) ||
		 (start.x == end.x && start.y == end.y && (endseg & 1))) &&
		!overlap)
		end.mask &= ~(1 << startseg);
	}
	if (endseg & 1)
	{
	    end.mask &= ~(1 << endseg);
	    if (((start.x > end.x || start.y > end.y) ||
		 (start.x == end.x && start.y == end.y && !(startseg & 1))) &&
		!overlap)
		start.mask &= ~(1 << endseg);
	}
	else
	{
	    if (!overlap)
		info->initialMask &= ~(1 << endseg);
	    if (start.x < end.x || start.y < end.y)
		start.mask &= ~(1 << endseg);
	}
    }
    if (startseg & 1)
    {
	info->start = start;
	info->end = oob;
    }
    else
    {
	info->end = start;
	info->start = oob;
    }
    if (endseg & 1)
    {
	info->altend = end;
	info->altstart = oob;
    }
    else
    {
	info->altstart = end;
	info->altend = oob;
    }
    if (info->altstart.x < info->start.x || info->altstart.y < info->start.y)
    {
	miZeroArcPtRec tmp;
 	tmp = info->altstart;
	info->altstart = info->start;
	info->start = tmp;
    }
    if (info->altend.x < info->end.x || info->altend.y < info->end.y)
    {
	miZeroArcPtRec tmp;
 	tmp = info->altend;
	info->altend = info->end;
	info->end = tmp;
    }
    if (!info->start.x || !info->start.y)
    {
	info->initialMask = info->start.mask;
	info->start = info->altstart;
    }
    return FALSE;
}

#define Pixelate(xval,yval) \
    { \
	pts->x = xval; \
	pts->y = yval; \
	pts++; \
    }

#define DoPix(idx,xval,yval) if (mask & (1 << idx)) Pixelate(xval, yval);

DDXPointPtr
miZeroArcPts(arc, pts)
    xArc *arc;
    register DDXPointPtr pts;
{
    miZeroArcRec info;
    register int x, y, a, b, d, mask;
    register int k1, k3, dx, dy;
    Bool do360;

    do360 = miZeroArcSetup(arc, &info, TRUE);
    MIARCSETUP();
    mask = info.initialMask;
    if (!(arc->width & 1))
    {
	DoPix(1, info.xorgo, info.yorg);
	DoPix(3, info.xorgo, info.yorgo);
    }
    if (!info.end.x || !info.end.y)
    {
	mask = info.end.mask;
	info.end = info.altend;
    }
    if (do360 && (arc->width == arc->height) && !(arc->width & 1))
    {
	int yorgh = info.yorg + info.h;
	int xorghp = info.xorg + info.h;
	int xorghn = info.xorg - info.h;

	while (1)
	{
	    Pixelate(info.xorg + x, info.yorg + y);
	    Pixelate(info.xorg - x, info.yorg + y);
	    Pixelate(info.xorg - x, info.yorgo - y);
	    Pixelate(info.xorg + x, info.yorgo - y);
	    if (a < 0)
		break;
	    Pixelate(xorghp - y, yorgh - x);
	    Pixelate(xorghn + y, yorgh - x);
	    Pixelate(xorghn + y, yorgh + x);
	    Pixelate(xorghp - y, yorgh + x);
	    MIARCCIRCLESTEP(;);
	}
	if (x > 1 && pts[-1].x == pts[-5].x && pts[-1].y == pts[-5].y)
	    pts -= 4;
	x = info.w;
	y = info.h;
    }
    else if (do360)
    {
	while (y < info.h || x < info.w)
	{
	    MIARCOCTANTSHIFT(;);
	    Pixelate(info.xorg + x, info.yorg + y);
	    Pixelate(info.xorgo - x, info.yorg + y);
	    Pixelate(info.xorgo - x, info.yorgo - y);
	    Pixelate(info.xorg + x, info.yorgo - y);
	    MIARCSTEP(;,;);
	}
    }
    else
    {
	while (y < info.h || x < info.w)
	{
	    MIARCOCTANTSHIFT(;);
	    if ((x == info.start.x) || (y == info.start.y))
	    {
		mask = info.start.mask;
		info.start = info.altstart;
	    }
	    DoPix(0, info.xorg + x, info.yorg + y);
	    DoPix(1, info.xorgo - x, info.yorg + y);
	    DoPix(2, info.xorgo - x, info.yorgo - y);
	    DoPix(3, info.xorg + x, info.yorgo - y);
	    if ((x == info.end.x) || (y == info.end.y))
	    {
		mask = info.end.mask;
		info.end = info.altend;
	    }
	    MIARCSTEP(;,;);
	}
    }
    if ((x == info.start.x) || (y == info.start.y))
	mask = info.start.mask;
    DoPix(0, info.xorg + x, info.yorg + y);
    DoPix(2, info.xorgo - x, info.yorgo - y);
    if (arc->height & 1)
    {
	DoPix(1, info.xorgo - x, info.yorg + y);
	DoPix(3, info.xorg + x, info.yorgo - y);
    }
    return pts;
}

#undef DoPix
#define DoPix(idx,xval,yval) \
    if (mask & (1 << idx)) \
    { \
	arcPts[idx]->x = xval; \
	arcPts[idx]->y = yval; \
	arcPts[idx]++; \
    }

static void
miZeroArcDashPts(pGC, arc, dinfo, points, maxPts, evenPts, oddPts)
    GCPtr pGC;
    xArc *arc;
    DashInfo *dinfo;
    int maxPts;
    register DDXPointPtr points, *evenPts, *oddPts;
{
    miZeroArcRec info;
    register int x, y, a, b, d, mask;
    register int k1, k3, dx, dy;
    int dashRemaining;
    DDXPointPtr arcPts[4];
    DDXPointPtr startPts[5], endPts[5];
    int deltas[5];
    DDXPointPtr startPt, pt, lastPt, pts;
    int i, j, delta, ptsdelta, seg, startseg;

    for (i = 0; i < 4; i++)
	arcPts[i] = points + (i * maxPts);
    (void)miZeroArcSetup(arc, &info, FALSE);
    MIARCSETUP();
    mask = info.initialMask;
    startseg = info.startAngle / QUADRANT;
    startPt = arcPts[startseg];
    if (!(arc->width & 1))
    {
	DoPix(1, info.xorgo, info.yorg);
	DoPix(3, info.xorgo, info.yorgo);
    }
    if (!info.end.x || !info.end.y)
    {
	mask = info.end.mask;
	info.end = info.altend;
    }
    while (y < info.h || x < info.w)
    {
	MIARCOCTANTSHIFT(;);
	if ((x == info.firstx) || (y == info.firsty))
	    startPt = arcPts[startseg];
	if ((x == info.start.x) || (y == info.start.y))
	{
	    mask = info.start.mask;
	    info.start = info.altstart;
	}
	DoPix(0, info.xorg + x, info.yorg + y);
	DoPix(1, info.xorgo - x, info.yorg + y);
	DoPix(2, info.xorgo - x, info.yorgo - y);
	DoPix(3, info.xorg + x, info.yorgo - y);
	if ((x == info.end.x) || (y == info.end.y))
	{
	    mask = info.end.mask;
	    info.end = info.altend;
	}
	MIARCSTEP(;,;);
    }
    if ((x == info.firstx) || (y == info.firsty))
	startPt = arcPts[startseg];
    if ((x == info.start.x) || (y == info.start.y))
	mask = info.start.mask;
    DoPix(0, info.xorg + x, info.yorg + y);
    DoPix(2, info.xorgo - x, info.yorgo - y);
    if (arc->height & 1)
    {
	DoPix(1, info.xorgo - x, info.yorg + y);
	DoPix(3, info.xorg + x, info.yorgo - y);
    }
    for (i = 0; i < 4; i++)
    {
	seg = (startseg + i) & 3;
	pt = points + (seg * maxPts);
	if (seg & 1)
	{
	    startPts[i] = pt;
	    endPts[i] = arcPts[seg];
	    deltas[i] = 1;
	}
	else
	{
	    startPts[i] = arcPts[seg] - 1;
	    endPts[i] = pt - 1;
	    deltas[i] = -1;
	}
    }
    startPts[4] = startPts[0];
    endPts[4] = startPt;
    startPts[0] = startPt;
    if (startseg & 1)
    {
	if (startPts[4] != endPts[4])
	    endPts[4]--;
	deltas[4] = 1;
    }
    else
    {
	if (startPts[0] > startPts[4])
	    startPts[0]--;
	if (startPts[4] < endPts[4])
	    endPts[4]--;
	deltas[4] = -1;
    }
    if (arc->angle2 < 0)
    {
	DDXPointPtr tmps, tmpe;
	int tmpd;

	tmpd = deltas[0];
	tmps = startPts[0] - tmpd;
	tmpe = endPts[0] - tmpd;
	startPts[0] = endPts[4] - deltas[4];
	endPts[0] = startPts[4] - deltas[4];
	deltas[0] = -deltas[4];
	startPts[4] = tmpe;
	endPts[4] = tmps;
	deltas[4] = -tmpd;
	tmpd = deltas[1];
	tmps = startPts[1] - tmpd;
	tmpe = endPts[1] - tmpd;
	startPts[1] = endPts[3] - deltas[3];
	endPts[1] = startPts[3] - deltas[3];
	deltas[1] = -deltas[3];
	startPts[3] = tmpe;
	endPts[3] = tmps;
	deltas[3] = -tmpd;
	tmps = startPts[2] - deltas[2];
	startPts[2] = endPts[2] - deltas[2];
	endPts[2] = tmps;
	deltas[2] = -deltas[2];
    }
    for (i = 0; startPts[i] == endPts[i]; i++)
	;
    pt = startPts[i];
    for (j = 4; startPts[j] == endPts[i]; j--)
	;
    lastPt = endPts[j] - deltas[j];
    if ((pt->x == dinfo->endPt.x) && (pt->y == dinfo->endPt.y))
    {
	startPts[i] += deltas[i];
    }
    else
    {
	dinfo->dashIndex = dinfo->dashIndexInit;
	dinfo->dashOffset = dinfo->dashOffsetInit;
    }
    if ((lastPt->x == dinfo->startPt.x) && (lastPt->y == dinfo->startPt.y) &&
	(lastPt != pt))
	endPts[j] = pt;
    dinfo->startPt = *pt;
    dinfo->endPt = *lastPt;
    dashRemaining = pGC->dash[dinfo->dashIndex] - dinfo->dashOffset;
    for (i = 0; i < 5; i++)
    {
	pt = startPts[i];
	lastPt = endPts[i];
	delta = deltas[i];
	while (pt != lastPt)
	{
	    if (dinfo->dashIndex & 1)
	    {
		pts = *oddPts;
		ptsdelta = -1;
	    }
	    else
	    {
		pts = *evenPts;
		ptsdelta = 1;
	    }
	    while ((pt != lastPt) && --dashRemaining >= 0)
	    {
		*pts = *pt;
		pts += ptsdelta;
		pt += delta;
	    }
	    if (dinfo->dashIndex & 1)
		*oddPts = pts;
	    else
		*evenPts = pts;
	    if (dashRemaining <= 0)
	    {
		if (++(dinfo->dashIndex) == pGC->numInDashList)
		    dinfo->dashIndex = 0;
		dashRemaining = pGC->dash[dinfo->dashIndex];
	    }
	}
    }
    dinfo->dashOffset = pGC->dash[dinfo->dashIndex] - dashRemaining;
}

void
miZeroPolyArc(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    int maxPts = 0;
    register int n, maxw;
    register xArc *arc;
    register int i;
    DDXPointPtr points, pts, oddPts;
    register DDXPointPtr pt;
    int numPts;
    Bool direct_arc, direct_pts;	/* SI */
    Bool dospans;
    int *widths;
    unsigned long fgPixel = pGC->fgPixel;
    DashInfo dinfo;
    RegionPtr 	prgnDst;
    register BoxPtr	pbox;
    int 		nbox;

    prgnDst = ((siPrivGC *)(pGC->devPrivates[siGCPrivateIndex].ptr))->pCompositeClip;

    for (arc = parcs, i = narcs; --i >= 0; arc++)
    {
	if (!miCanZeroArc(arc))
	    miPolyArc(pDraw, pGC, 1, arc);
	else
	{
	    if (arc->width > arc->height)
		n = arc->width + (arc->height >> 1);
	    else
		n = arc->height + (arc->width >> 1);
	    if (n > maxPts)
		maxPts = n;
	}
    }
    if (!maxPts)
	return;
    numPts = maxPts << 2;
    dospans = (pGC->fillStyle != FillSolid);

    if (dospans)
    {
	widths = (int *)ALLOCATE_LOCAL(sizeof(int) * numPts);
	if (!widths)
	    return;
	maxw = 0;
    }
    if (pGC->lineStyle != LineSolid)
    {
	numPts <<= 1;
	dinfo.startPt.x = parcs->x - 1;
	dinfo.startPt.y = parcs->y - 1;
	dinfo.endPt = dinfo.startPt;
	dinfo.dashIndexInit = 0;
	dinfo.dashOffsetInit = 0;
	miStepDash((int)pGC->dashOffset, &dinfo.dashIndexInit,
		   (unsigned char *) pGC->dash, (int)pGC->numInDashList,
		   &dinfo.dashOffsetInit);
    }
    points = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * numPts);
    if (!points)
    {
	if (dospans)
	{
	    DEALLOCATE_LOCAL(widths);
	}
	return;
    }

    direct_arc = si_hasdrawarc && (pGC->lineStyle == LineSolid) && 
	         (pDraw->type == DRAWABLE_WINDOW);
    if (direct_arc)
	si_PrepareGS(pGC);

    for (arc = parcs, i = narcs; --i >= 0; arc++)
    {
	if (miCanZeroArc(arc))
	{
	    /* SI: start */
	    /*
	    * Try to draw the arc using an sdd function
	    */
	    if (direct_arc) {
		if (si_hascliplist(SIavail_drawarc)) {
		    if (si_Drawarc(arc->x + pDraw->x,
				   arc->y + pDraw->y,
				   arc->width, arc->height,
				   arc->angle1, arc->angle2) == SI_SUCCEED)
			continue;
		}
		else {
		    pbox = REGION_RECTS(prgnDst);
		    nbox = REGION_NUM_RECTS(prgnDst);

		    while (--nbox >= 0) {
			si_setdrawarcclip(pbox->x1, pbox->y1, 
					  pbox->x2-1, pbox->y2-1);
			if (si_Drawarc(arc->x + pDraw->x,
				       arc->y + pDraw->y,
				       arc->width, arc->height,
				       arc->angle1, arc->angle2) == SI_FAIL) {
			    si_setdrawarcclip(0, 0, si_getscanlinelen-1,
					      si_getscanlinecnt-1);
			    break;
			}
			pbox++;
		    }
		    if (nbox < 0)
			continue;
		}
	    }

	    /*
	     * See if we can use the direct sdd point plotting.  To do
	     * this, we can't need to clip, and we must be able to draw
	     * with points, not spans.
	     */
	    direct_pts = 0;
	    if (!dospans && pDraw->type == DRAWABLE_WINDOW && si_hasplotpoint)
            {
		int 		x1, y1, x2, y2;

		x1 = arc->x + pDraw->x;		/* SI (R4) */
		y1 = arc->y + pDraw->y;		/* SI (R4) */
		x2 = x1 + arc->width;
		y2 = y1 + arc->height;

		pbox = REGION_RECTS(prgnDst);
		nbox = REGION_NUM_RECTS(prgnDst);

		if (nbox == 0)
	        {	/* nothing to draw */
		    if (dospans)
		    {
			DEALLOCATE_LOCAL(widths);
		    }
		    DEALLOCATE_LOCAL(points);
		    return;
		}

		direct_pts = 1;
		for (; nbox--; pbox++)
                {
			if (x1 < pbox->x1 || x2 >= pbox->x2 ||
			    y1 < pbox->y1 || y2 >= pbox->y2) 
				direct_pts = 0;
		}
	
		if (direct_pts)
                {
			arc->x = x1;
			arc->y = y1;
			si_PrepareGS(pGC);
		}
	    }
	    /* SI: end */

	    if (pGC->lineStyle == LineSolid)
		pts = miZeroArcPts(arc, points);
	    else
	    {
		pts = points;
		oddPts = &points[(numPts >> 1) - 1];
		miZeroArcDashPts(pGC, arc, &dinfo,
				 oddPts + 1, maxPts, &pts, &oddPts);
	    }
	    n = pts - points;
	    if (!dospans) {
		if (n && (!direct_pts || (si_plotpoints(n, points) == SI_FAIL)))
		    (*pGC->ops->PolyPoint)(pDraw, pGC,CoordModeOrigin,n,points);
	    }
	    else
	    {
		if (n > maxw)
		{
		    while (maxw < n)
			widths[maxw++] = 1;
		}
		/* SI: start (added test for DRAWABLE_WINDOW as well) */
		if (pGC->miTranslate && (pDraw->type == DRAWABLE_WINDOW))
		/* SI: end */
		{
		    for (pt = points; pt != pts; pt++)
		    {
			pt->x += pDraw->x;
			pt->y += pDraw->y;
		    }
		}
		(*pGC->ops->FillSpans)(pDraw, pGC, n, points, widths, FALSE);
	    }
	    if (pGC->lineStyle != LineDoubleDash)
		continue;
	    if ((pGC->fillStyle == FillSolid) ||
		(pGC->fillStyle == FillStippled))
	    {
		DoChangeGC(pGC, GCForeground, (XID *)&pGC->bgPixel, 0);
		ValidateGC(pDraw, pGC);
		/* SI: start */
		if (direct_pts)
                {
			si_PrepareGS(pGC);
		}
		/* SI: end */
	    }
	    pts = &points[numPts >> 1];
	    oddPts++;
	    n = pts - oddPts;
	    if (!dospans) {
		if (n && (!direct_pts || (si_plotpoints(n, oddPts) == SI_FAIL)))
		    (*pGC->ops->PolyPoint)(pDraw, pGC,CoordModeOrigin,n,oddPts);
	    }
	    else {
		if (n > maxw)
		{
		    while (maxw < n)
			widths[maxw++] = 1;
		}
		/* SI: start (added test for DRAWABLE_WINDOW as well) */
		if (pGC->miTranslate && (pDraw->type == DRAWABLE_WINDOW))
		/* SI: end */
		{
		    for (pt = oddPts; pt != pts; pt++)
		    {
			pt->x += pDraw->x;
			pt->y += pDraw->y;
		    }
		}
		(*pGC->ops->FillSpans)(pDraw, pGC, n, oddPts, widths, FALSE);
	    }
	    if ((pGC->fillStyle == FillSolid) ||
		(pGC->fillStyle == FillStippled))
	    {
		DoChangeGC(pGC, GCForeground, (XID *)&fgPixel, 0);
		ValidateGC(pDraw, pGC);
		/* SI: start */
		if (direct_pts)
			si_PrepareGS(pGC);
		/* SI: end */
	    }
	}
    }
    if (dospans)
    {
	DEALLOCATE_LOCAL(widths);
    }
    DEALLOCATE_LOCAL(points);
}
