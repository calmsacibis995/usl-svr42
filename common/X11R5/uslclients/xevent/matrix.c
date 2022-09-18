/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xevent:matrix.c	1.1"
#endif
/*
 matrix.c (C source file)
	Acc: 575326481 Fri Mar 25 15:54:41 1988
	Mod: 575321187 Fri Mar 25 14:26:27 1988
	Sta: 575570328 Mon Mar 28 11:38:48 1988
	Owner: 2011
	Group: 1985
	Permissions: 644
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/
/************************************************************************

	Copyright 1987 by AT&T
	All Rights Reserved

	author:
		Ross Hilbert
		AT&T 12/07/87
************************************************************************/

#include "matrix.h"

void GetMatrixDimensions (p, x, y)
MatrixAttributes * p;
int * x;
int * y;
{
	GetCellOrigin (p, p->rows, p->cols, x, y);
}

void GetCellOrigin (p, row, col, x, y)
MatrixAttributes * p;
int row, col;
int * x;
int * y;
{
	*x = (p->width + p->pad) * col + p->pad;
	*y = (p->height + p->pad) * row + p->pad;
}

void GetCellIndex (p, row, col, index)
MatrixAttributes * p;
int row, col;
int * index;
{
	if (p->order)
		*index = row * p->cols + col;
	else
		*index = col * p->rows + row;
}

void GetCellAddress (p, index, row, col)
MatrixAttributes * p;
int index;
int * row;
int * col;
{
	if (p->order)
	{
		*row = index / p->cols;
		*col = index % p->cols;
	}
	else
	{
		*row = index % p->rows;
		*col = index / p->rows;
	}
}

void GetCellAtPoint (p, x, y, row, col)
MatrixAttributes * p;
int x, y;
int * row;
int * col;
{
	int r = (y - p->pad/2) / (p->height + p->pad);
	int c = (x - p->pad/2) / (p->width + p->pad);

	if (r < 0)		r = 0;
	if (r >= p->rows)	r = p->rows-1;
	if (c < 0)		c = 0;
	if (c >= p->cols)	c = p->cols-1;
	*row = r;
	*col = c;
}
