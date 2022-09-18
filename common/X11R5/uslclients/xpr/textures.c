#ident	"@(#)xpr:textures.c	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

/*
 * The ``textures'' in this file are not stored as bitmaps,
 * but as arrays (flattened matrices) of integers.
 */

/**
 ** _4x4 - LIST OF IMITATION GRAY-SCALES (TEXTURES)
 **/

static char		_4x4_0[16] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
static char		_4x4_1[16] = {
	1, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
static char		_4x4_2[16] = {
	1, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 0
};
static char		_4x4_3[16] = {
 	1, 0, 1, 0,
	0, 0, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 0
};
static char		_4x4_4[16] = {
	1, 0, 1, 0,
	0, 0, 0, 0,
	1, 0, 1, 0,
	0, 0, 0, 0
};
static char		_4x4_5[16] = {
	1, 0, 1, 0,
	0, 1, 0, 0,
	1, 0, 1, 0,
	0, 0, 0, 0
};
static char		_4x4_6[16] = {
	1, 0, 1, 0,
	0, 1, 0, 0,
	1, 0, 1, 0,
	0, 0, 0, 1
};
static char		_4x4_7[16] = {
	1, 0, 1, 0,
	0, 1, 0, 1,
	1, 0, 1, 0,
	0, 0, 0, 1
};
static char		_4x4_8[16] = {
	1, 0, 1, 0,
	0, 1, 0, 1,
	1, 0, 1, 0,
	0, 1, 0, 1
};
static char		_4x4_9[16] = {
	1, 1, 1, 0,
	0, 1, 0, 1,
	1, 0, 1, 0,
	0, 1, 0, 1
};
static char		_4x4_10[16] = {
	1, 1, 1, 0,
	0, 1, 0, 1,
	1, 0, 1, 1,
	0, 1, 0, 1
};
static char		_4x4_11[16] = {
	1, 1, 1, 1,
	0, 1, 0, 1,
	1, 0, 1, 1,
	0, 1, 0, 1
};
static char		_4x4_12[16] = {
	1, 1, 1, 1,
	0, 1, 0, 1,
	1, 1, 1, 1,
	0, 1, 0, 1
};
static char		_4x4_13[16] = {
	1, 1, 1, 1,
	1, 1, 0, 1,
	1, 1, 1, 1,
	0, 1, 0, 1
};
static char		_4x4_14[16] = {
	1, 1, 1, 1,
	1, 1, 0, 1,
	1, 1, 1, 1,
	0, 1, 1, 1
};
static char		_4x4_15[16] = {
	1, 1, 1, 1,
	1, 1, 1, 1,
	1, 1, 1, 1,
	0, 1, 1, 1
};
static char		_4x4_16[16] = {
	1, 1, 1, 1,
	1, 1, 1, 1,
	1, 1, 1, 1,
	1, 1, 1, 1
};

/*
 * List the textures in increasing order of ``intensity''.
 * On the screen the least intense ``color'' is black, and the
 * most intense is white--thus the least intense texture is solid
 * black and the most is all white. Thus the textures are list in
 * decreasing order of density of 1-bits.
 */
char			*_4x4[17] = {
	_4x4_16,
	_4x4_15,
	_4x4_14,
	_4x4_13,
	_4x4_12,
	_4x4_11,
	_4x4_10,
	_4x4_9,
	_4x4_8,
	_4x4_7,
	_4x4_6,
	_4x4_5,
	_4x4_4,
	_4x4_3,
	_4x4_2,
	_4x4_1,
	_4x4_0
};
