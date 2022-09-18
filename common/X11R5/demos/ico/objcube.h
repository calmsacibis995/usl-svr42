/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4ico:objcube.h	1.1"
/* objcube.h - structure values for cube */

{	"cube", "cube",	/* long and short names */
	"octahedron",	/* long name of dual */
	8, 12, 6,	/* number of vertices, edges, and faces */
	{		/* vertices (x,y,z) */
			/* all points must be within radius 1 of the origin */
#define T 0.577
		{  T,  T,  T },
		{  T,  T, -T },
		{  T, -T, -T },
		{  T, -T,  T },
		{ -T,  T,  T },
		{ -T,  T, -T },
		{ -T, -T, -T },
		{ -T, -T,  T },
#undef T
	},
	{	/* faces (numfaces + indexes into vertices) */
		/*  faces must be specified clockwise from the outside */
		4,	0, 1, 2, 3,
		4, 	7, 6, 5, 4,
		4, 	1, 0, 4, 5,
		4,	3, 2, 6, 7,
		4,	2, 1, 5, 6,
		4,	0, 3, 7, 4,
	}
},		/* leave a comma to separate from the next include file */
/* end */
