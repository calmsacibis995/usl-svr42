/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4ico:polyinfo.h	1.1"
/* polyinfo.h
 * This is the description of one polyhedron file
 */

#define MAXVERTS 120
	/* great rhombicosidodecahedron has 120 vertices */
#define MAXNV MAXVERTS
#define MAXFACES 30
	/* (hexakis icosahedron has 120 faces) */
#define MAXEDGES 180
	/* great rhombicosidodecahedron has 180 edges */
#define MAXEDGESPERPOLY 20

typedef struct {
	double x, y, z;
} Point3D;

/* structure of the include files which define the polyhedra */
typedef struct {
	char *longname;		/* long name of object */
	char *shortname;	/* short name of object */
	char *dual;		/* long name of dual */
	int numverts;		/* number of vertices */
	int numedges;		/* number of edges */
	int numfaces;		/* number of faces */
	Point3D v[MAXVERTS];	/* the vertices */
	int f[MAXEDGES*2+MAXFACES];	/* the faces */
} Polyinfo;

/* end */
