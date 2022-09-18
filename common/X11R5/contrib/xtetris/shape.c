/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xtetris:shape.c	1.1"
#endif

#include "defs.h"

shape_type shape[7] = {

/*      bitmap    score X Y wid ht */
  { /* Shape 0 */
    { { 0x00000f00, 5,  { {0,1,4,1}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  0,4,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x00004444, 8,  { {1,0,1,4}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  1,1,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* #### */
      { 0x00000f00, 5,  { {0,1,4,1}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  0,4,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x00004444, 8,  { {1,0,1,4}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  1,1,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  },

  { /* Shape 1 */
    { { 0x0000cc00, 6,  { {0,0,2,2}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ##   */
      { 0x0000cc00, 6,  { {0,0,2,2}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ##   */
      { 0x0000cc00, 6,  { {0,0,2,2}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x0000cc00, 6,  { {0,0,2,2}, {0,0,0,0} }, {{0,0,0,0},{0,0,0,0}}, 1,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  },

  { /* Shape 2 */
    { { 0x00004e00, 5,  { {0,1,3,1}, {1,0,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*  #   */
      { 0x00004640, 5,  { {1,0,1,3}, {2,1,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  1,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ###  */
      { 0x00000e40, 6,  { {0,1,3,1}, {1,2,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x00004c40, 5,  { {1,0,1,3}, {0,1,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  },

  { /* Shape 3 */
    { { 0x0000c600, 6,  { {0,0,2,1}, {1,1,2,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ##   */
      { 0x00004c80, 7,  { {1,0,1,2}, {0,1,1,2} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*  ##  */
      { 0x0000c600, 6,  { {0,0,2,1}, {1,1,2,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x00004c80, 7,  { {1,0,1,2}, {0,1,1,2} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  },

  { /* Shape 4 */
    { { 0x00006c00, 6,  { {1,0,2,1}, {0,1,2,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*  ##  */
      { 0x00008c40, 7,  { {0,0,1,2}, {1,1,1,2} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ##   */
      { 0x00006c00, 6,  { {1,0,2,1}, {0,1,2,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x00008c40, 7,  { {0,0,1,2}, {1,1,1,2} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  },

  { /* Shape 5 */
    { { 0x00002e00, 6,  { {2,0,1,1}, {0,1,3,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*   #  */
      { 0x000088c0, 7,  { {0,0,1,3}, {1,2,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ###  */
      { 0x0000e800, 6,  { {0,0,3,1}, {0,1,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x0000c440, 7,  { {1,0,1,3}, {0,0,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  },

  { /* Shape 6 */
    { { 0x0000e200, 6,  { {0,0,3,1}, {2,1,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /* ###  */
      { 0x000044c0, 7,  { {1,0,1,3}, {0,2,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*   #  */
      { 0x00008e00, 6,  { {0,1,3,1}, {0,0,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,3,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} },   /*      */
      { 0x0000c880, 7,  { {0,0,1,3}, {1,0,1,1} }, {{0,0,0,0},{0,0,0,0}}, 2,  0,2,  {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } }, /*      */
  }
};

void define_shapes()
{

  int s, r, x, y;

  /* First set the highesty values (could have specified these statically, but the
     human cost is too high.  This is one-shot anyway). */

  for (s = 0; s < 7; s++)
    {
      shape_ptr sh = &shape[s];
      XGCValues gcv;

      for (r = 0; r < 4; r++)
	{
	  /* working on shape s */

	  rotshape_ptr rot = &sh->forms[r];
	  unsigned long unitson;
	  int i;


	  /* Scale the x,y,width,length */

	  for (i = 0; i < rot->nrect; i++)
	    {
	      XRectangle *rec = &rot->rect[i];

	      *rec = rot->urect[i];

	      rec->x *= resources.boxsize;
	      rec->y *= resources.boxsize;
	      rec->width *= resources.boxsize;
	      rec->height *= resources.boxsize;
	    }

	  rot->shadowx *= resources.boxsize;
	  rot->shadowwidth *= resources.boxsize;

	  /* set the highesty values. */

	  unitson = rot->unitson;
	  for (x = 0; x < 4; x++)
	    {
	      rot->highesty[x] = rot->highestx[x] = 0;
	      rot->lowestx[x] = -2;

	    }

	  for (y = 3; y >= 0; y -- )
	    for (x = 3; x >= 0; x--)
	      {
		if (unitson & 1)
		  {
		    if (rot->highesty[x] == 0)
		      rot->highesty[x] = y+1;
		    if (rot->highestx[y] == 0)
		      rot->highestx[y] = x+1;
		    rot->lowestx[y] = x-1;
		  }
		unitson >>= 1;
	      }
	}

      /* Now allocate the graphics context, and set it properly. */

      gcv.foreground = sh->foreground;
      gcv.background = sh->background;
      gcv.stipple = sh->stipple;
      gcv.fill_style = FillOpaqueStippled;

      sh->gc = XCreateGC( XtDisplay(canvas), XtWindow(canvas),
			 GCForeground|GCBackground|GCStipple|GCFillStyle,
			 &gcv );
    }
}


store_shape(shape_no, xpos, ypos, rot)
  int     shape_no, xpos, ypos, rot;
{
  register unsigned long unitson = shape[shape_no].forms[rot].unitson;

  int y, x;

  for (y = ypos+3; y >= ypos; y--)
    for (x = xpos+3; x >= xpos; x--)
      {

	if ((x >= 0) && (y >= 0))
	  if (unitson & 0x00000001)
	    grid[x][y] = &shape[shape_no];
	unitson >>= 1;
      }
}

create_shape()
{
  shape_no = next_no;
  rot = next_rot;
  next_no = random() % 7;
  next_rot = random() % 4;
  xpos = (UWIDTH / 2) - 1;
  ypos = -4;
}

