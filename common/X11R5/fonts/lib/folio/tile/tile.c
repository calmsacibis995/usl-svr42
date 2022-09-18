/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:tile/tile.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)tile.c 1.4 89/05/24";
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


#ifdef COMMENT

    IMPLEMENTATION:

    TILE TYPES:
    ----------
    An intermediate representation of the tile, where each pixel is
    represented by a byte is stored in the <WorkArray>. The EOFILL,
    NZFILL and STROKE1 operations are performed on this array only.
    A byte/pixel is necessary for the NZFILL where the winding count
    is any positive or non-positive number (hopefully in the -128 to
    +128 range). Using the same tile representation for the EOFILL and
    STROKE1 operations help to keep most of the marking code common
    as well as to speed the process (at the expense of memory as usual) 
    since no bit compaction overhead is used while marking.

    For the cases of "floobie dust" control operations another copy of
    the tile, namely <FDustArray>, is used to mark the "floobie dust"
    pixels.

    The client, however, is returned a tile where each pixel is represented
    by a bit. This <FinalArray> is created by the <FillTile> function
    called after all the marking is completed. In this last stage, the 
    <WorkArray> is scan filled for the cases of EOFILL or NZFILL and the
    result is ORed with the <FDustArray> if "floobie dust" control is used.
    The final is result is compacted from a byte/pixel representation
    to a bit/pixel representation in the <FinalArray>. The filling, ORing
    and compaction operations are all performed on the fly w/o any 
    intermediate storage.

    MARKING:
    -------
    The same marking operations are used for EOFILL and NZFILL for the
    stated purpose of keeping the code common. The upward mark is <+1>
    and downward mark is <-1>.  The distinction between EOFILL and
    NZFILL comes into play in the filling stage.  For EOFILL a mark
    is assumed to be present if the pixel location has an ODD count.
    For the NZFILL case, a mark is present if the count is NONZERO.

    The mark for STROKE1 and the "floobie dust" control operations is
    direction independent and simply represented by a <1> at the
    current pixel location.

    THE ACTIVE BAND:
    ---------------
    The marking operations keep track of an active band in the tile
    where pixels may be turned on.  Maintaining an active band is
    simply a performance decision to speed up the filling/compaction
    stages as well as the clearing of the various tile representations
    for the next set of tile operations.

    <AllocateTile> <TileIsTouched> <FillTile>
    -----------------------------------------
    These client interfaces are straightforward and explained in
    the respective routines below.

    <MarkTile>
    ----------
    The Marking operations are more complex and occupy the majority
    of the code for the tilehandler.  Each of the different marking
    modes follow a similar flow.  The complexity is simply due to
    the different marking positions and the boundary conditions when
    the same path is traveled using different modes.(see tile.h for
    some "marking" examples).
    
    The general algorithm is as follows:
    	1) STOP if a given path segment does not affect the tile.
    	2) If a path begins "AboveTile" or "BelowTile" the horizontal
    	   band defined by the tile location, simply follow path in to 
    	   band without marking anything.
    	  (Note the definitions of "Above" and "Below" vary w/ the mode)
    	3) The current path location can now be one of the following:
    	    	"LeftOfTile" 	"InsideTile" or     "RightOfTile"
    	    (Note again these definitions vary with the mode)
    	4) Depending on the mode and the current location, the path
    	   will be traced further by simply "Moving" or "Marking & Moving"
          (i.e. <MoveFromRight>, <MarkFromInside> ... )
    	5) As the path is traced, a regional boundary may be crossed and
    	   the options are again to be "AboveTile", "BelowTile",
    	    "LeftOfTile", "InsideTile" or "RightOfTile".
    	6) Individual cases given path direction, path endpoints and 
    	    current location determines:
    	    	i)  limits to verify after each move to stop
    	    	    moving/marking when boundary is crossed.
    	    	ii) What tracing/marking routine to goto if a
    	    	    boundary is crossed. (One option is of course to STOP)    

    	NOTES:
    	    (1)	The "marking" operations require the initialization
    	    	of a pointer <*bp> into the <WorkArray> or <FDustArray>.
    	    	Given a current grid coordinate GRID(X,Y) the pointer
    	    	<*bp> is initialized differently depending on the mode.

    	    (2) for FLOOBIE DUST CONTROL, the path have to be traced twice
    	    	once to mark the EOFILL and NZFILL information on the 
    	    	<WorkArray> and a second time to mark the <FDustArray>.
    	

#endif /*COMMENT*/

#include "cdefs.h"
#include "common.h"
#include "rmbuffer.h"
#include "tile.h"
#include "tilelcl.h"

#ifdef DEBUGLEVEL2
#include "showtile.h"
#endif /*DEBUGLEVEL2*/

#ifdef DEBUG
int32	tile_debug;
#endif /*DEBUG*/

int32	tile_WorkArray[tile_BYTETILEAREAINLONGS];   /* BYTE TILE (INT32 ALIGNED) */
int32	tile_FDustArray[tile_BYTETILEAREAINLONGS];  /* BYTE TILE (INT32 ALIGNED) */
uint32	tile_FinalArray[tile_BITTILEAREAINLONGS];   /* BIT TILE */

/* tile types */
#define tile_WORKTILE  	0
#define tile_FDUSTTILE	1
#define tile_FINALTILE 	2

int32   	tile_filltype;   	/* see tile.h types */
int32   	tile_fdusttype;    	/* see tile.h for types */
bbox_iBBox	tile_bbox;	    	/* bounding box for tile */
bbox_iBBox	tile_activebbox;	/* bounding box for active region */
bool    	tile_empty;	    	/* true if tile is empty */

rm_bufferHandle tile_rmbhdl;	    	/* handle to rook move buffer */

/*
---------------------------------------------------------------------------- 
           P O S I T I O N   D E F I N I T I O N S
---------------------------------------------------------------------------- 
*/  

/* 
    see COORDINATE SYSTEMS definition in tile.h 

    Convert GRID oordinates to MEMory coordinates for marking and filling
    various representations of the tile.
    The convention for the pixel location given a point GRID(X,Y) is 
    above and to the right of the point.
*/
    
#define tile_GridToMemX(x)  	    	    (((x) - tile_bbox.lox))

#define tile_GridToMemY(y)  	    	    ((tile_bbox.hiy - (y) - 1))


#define tile_GridYToArrayPtr(aptr,widthlog,y)  (aptr + (tile_GridToMemY(y)<<widthlog))

#define tile_GridXYToArrayPtr(aptr,widthlog,x,y) (tile_GridYToArrayPtr(aptr,widthlog,(y)) + \
    	    	    	    	    	    	tile_GridToMemX(x))


#define tile_GridYToWorkArrayPtr(y) 	    tile_GridYToArrayPtr((int8 *)tile_WorkArray,   \
    	    	    	    	    	    	tile_BYTETILEWIDTHINBYTESLOG,(y))

#define tile_GridXYToWorkArrayPtr(x,y) 	    tile_GridXYToArrayPtr((int8 *)tile_WorkArray,  \
    	    	    	    	    	    	tile_BYTETILEWIDTHINBYTESLOG,(x),(y))

#define tile_GridYToFDustArrayPtr(y) 	    tile_GridYToArrayPtr((int8 *)tile_FDustArray,  \
    	    	    	    	    	    	tile_BYTETILEWIDTHINBYTESLOG,(y))

#define tile_GridXYToFDustArrayPtr(x,y)     tile_GridXYToArrayPtr((int8 *)tile_FDustArray, \
    	    	    	    	    	    	tile_BYTETILEWIDTHINBYTESLOG,(x),(y))

#define tile_GridYToFinalArrayPtr(y)   	    tile_GridYToArrayPtr(tile_FinalArray,  \
    	    	    	    	    	    	tile_BITTILEWIDTHINLONGSLOG,(y))

/* Boundary definitions */
#define tile_AboveHorizontalBand(y) 	    ((y) >= tile_bbox.hiy)
#define tile_BelowHorizontalBand(y) 	    ((y) < tile_bbox.loy)
#define tile_LeftOfTile(x)  	    	    ((x) < tile_bbox.lox)
#define tile_RightOfTile(x)  	    	    ((x) >= tile_bbox.hix)


/*
------------------------------------------------------------------------- 
             C L E A N   U P                                                
------------------------------------------------------------------------- 
*/
    
void	tile_ClearLongArray( lp, size )
    Rint32   *lp;
    Rint32  size;
{
    while ( size-- > 0 )
    	*lp++ = 0;
}


/* Clear horizontal strip where the BBox is within the tile */ 

void	tile_ClearBandInTile( tilep, widthlog )
    Rint32  *tilep;
    int32   widthlog;
{
    Rint32   	*lp;
    Rint32  	size;

    lp = tile_GridYToArrayPtr(tilep,widthlog,(tile_activebbox.hiy-1));
    size = (tile_activebbox.hiy - tile_activebbox.loy) << widthlog;
    while( size-- > 0 ) {
    	    *lp++ = 0;
    }
}


/* 
------------------------------------------------------------------------
    	    B O O T   I N I T
------------------------------------------------------------------------
*/
/*
    Boot time initialization for tile handler
*/
void	tile_BootInit( )
{
#ifdef DEBUG
    tile_debug = FALSE;
#endif /*DEBUG*/
    /* clear tile arrays */
    tile_ClearLongArray(tile_WorkArray,  tile_BYTETILEAREAINLONGS);
    tile_ClearLongArray(tile_FDustArray, tile_BYTETILEAREAINLONGS);
    tile_ClearLongArray(tile_FinalArray, tile_BITTILEAREAINLONGS);
    tile_FDBootInit( );
    tile_empty = TRUE;
}

/*
------------------------------------------------------------------------- 
    	T I L E   M A Y B E   A F F E C T E D 
------------------------------------------------------------------------- 
*/    
/*
    Check if bounding box of object may affect the tile.
*/
bool	tile_TileMaybeAffected(bbox)
    bbox_iBBox *bbox;
{
    if (tile_fdusttype != tile_NOFDUST)
    	return(TRUE);
    if ( (bbox->lox > tile_bbox.hix) ||
         (bbox->loy > tile_bbox.hiy) ||
    	 (bbox->hiy < tile_bbox.loy) )
    	return(FALSE);
    else
    	return(TRUE);
}

/*
------------------------------------------------------------------------- 
       	A L L O C A T  E   T I L E
------------------------------------------------------------------------- 
*/    
/*
    Allocates a new tile given a position and marking/filling modes.
    <*origin> is the lower left corner of tile in GRID coordinates.
    <filltype> is one of even-odd fill, non-zero winding fill or
    	    	one pixel stroke
    <fdusttype> is one of no floobiedust control, righthanded or
    	     	lefthanded floobie dust control
*/
void	tile_AllocateTile(origin, filltype, fdusttype)
    pair_iXY	*origin;
    int32   	filltype;
    int32    	fdusttype;
{
    tile_filltype = filltype;
    tile_fdusttype = fdusttype;
    if ( !tile_empty) {
    	tile_ClearBandInTile( tile_WorkArray, tile_BYTETILEWIDTHINLONGSLOG );
    	tile_ClearBandInTile( tile_FDustArray,tile_BYTETILEWIDTHINLONGSLOG );
    	tile_ClearBandInTile( tile_FinalArray,tile_BITTILEWIDTHINLONGSLOG );
    }
    tile_FDAllocateTile();
    bbox_iFill(&tile_bbox, origin->x, origin->y, 
    	    	    tile_BITDIMENSION, tile_BITDIMENSION );
    tile_empty = TRUE;

#ifdef DEBUGLEVEL2
    if ( tile_debug) {
    	if (stile_display != stile_NULLTILE)
    	    stile_ShowTileGrid( origin,tile_BITDIMENSION,tile_BITDIMENSION );
    }
#endif /*DEBUGLEVEL2*/
} 

/*
---------------------------------------------------------------------------- 
    	U P D A T E    A C T I V E   B A N D 
---------------------------------------------------------------------------- 
*/  
void	tile_UpdateActiveBand(y1,y2)
    int32   y1,y2;
{
    int32   hiy,loy;
        
    /* 
    	compute hiy and loy for active band, stretch band to
    	always include the stroke and floobie dust cases. This is
    	a conservative approach. Better safe than sorry.
    */

    hiy = min(max(y1,y2)+1, tile_bbox.hiy);
    loy = max(min(y1,y2)-1, tile_bbox.loy);
    if (tile_empty) {
    	tile_empty = FALSE;
      	tile_activebbox.loy = loy;
       	tile_activebbox.hiy = hiy;
    } else {
       	tile_activebbox.hiy = max(tile_activebbox.hiy,hiy); 	
       	tile_activebbox.loy = min(tile_activebbox.loy,loy); 	
    }	    	    	    	    	    	
}


/*
---------------------------------------------------------------------------- 
    	M O V E   A N D / O R   M A R K   D E F I N I T I O N S
---------------------------------------------------------------------------- 
*/  
/*
    Tracing/Marking MACRO definitions:
        
    The main macros are <MoveCheckOne> and <MoveCheckXY>.  Their purpose
    is to track the path from the current location and move along the
    path as well as marking the tile if instructed to do so. The tracking
    continues until the limitin condition(s) is reached. When a limit
    is reached, one of the given Procedures is implemented. A <Proc>
    may be a <MoveFromRight> <MarkFromInside> etc. or simply <NullProc>
    in ecah case no further tracking of the path is necessary.
        
    <XAction> <YAction> move/mark action to take depending on the rook move
    <var>   is either <x> or <y> or <cnt> depending on the case
    <limit> <xlimit> <ylimit> are the ending conditions
    <Proc> <XProc> <YProc> are the routines to execute when <limit> is reached.
    	    	    	see below for detail

    Implicit declarations for the macros below

    Rint8   <*movesp>;
    Rint32  <x>,<y>;	    Current position in GRID coordinates
    Rint32  <cnt>;  	    a count for the number of moves
    Rbyte   <*bp>;    	    for marking Work array or FDust array
    Rint32  <lastmovey>;    1 if previous move was a ymove, 0 if xmove
*/

#define tile_MoveCheckOne(XAction,YAction,var,limit,Proc)    	    	\
    	    { 	    	    	    	    	    	    	    	\
    	    	Rint32 lim;   	    	    	    	    		\
    	    	lim = limit;	    	    	    	    	    	\
    	    	while (TRUE) {	    	    	    	    	    	\
    	    	    if (var==lim) {Proc(movesp,x,y);break;}   	    	\
    	    	    if (rm_GetMove(movesp)==rm_XMOVE)	    	    	\
    	    	    	    { XAction(); }	    	    	    	\
    	    	    else    { YAction(); }  	    	    	    	\
       	    	}   	    	    	    	    	    	    	\
    	    }

#define tile_MoveCheckXY(XAction,YAction,xlimit,ylimit,XProc,YProc)    	\
    	    { 	    	    	    	    	    	    	    	\
    	    	Rint32 xlim,ylim;   	    	    	    	    	\
    	    	xlim = xlimit; ylim = ylimit;	    	    	    	\
    	    	while (TRUE) {	    	    	    	    	    	\
    	    	    if (x==xlim) {XProc(movesp,x,y);break;}   	    	\
    	    	    if (y==ylim) {YProc(movesp,x,y);break;}   	    	\
    	    	    if (rm_GetMove(movesp)==rm_XMOVE)	    	    	\
    	    	    	    { XAction(); }	    	    	    	\
    	    	    else    { YAction(); }  	    	    	    	\
    	    	}   	    	    	    	    	    	    	\
    	    }

/*   <XAction>, <YAction> for SIMPLE MOVES */

#define tile_MoveRight()    {x++;}
#define tile_MoveLeft()	    {x--;}
#define tile_MoveUp()	    {y++;}
#define tile_MoveDown()	    {y--;}
    
    
/*  <XAction> <YAction> for INSIDE MOVES starting w/ <InsideMoveRight> */
    
#define tile_MovePtrRight(bp)	    {bp++;}
#define tile_MovePtrLeft(bp)	    {bp--;}
#define tile_MovePtrUp(bp)  	    {bp -= tile_BYTETILEWIDTHINBYTES;}
#define tile_MovePtrDown(bp)	    {bp += tile_BYTETILEWIDTHINBYTES;}

#define tile_MarkPtrDown(bp)	    {*bp -= 1;}
#define tile_MarkPtrUp(bp)  	    {*bp += 1;}
#define tile_MarkPtr(bp)	    {*bp = 1;}

#define tile_InsideMoveRight()	{tile_MoveRight();tile_MovePtrRight(bp);}
#define tile_InsideMoveUp() 	{tile_MoveUp();tile_MarkPtrUp(bp);tile_MovePtrUp(bp);}
#define tile_InsideMoveLeft()	{tile_MoveLeft();tile_MovePtrLeft(bp);}
#define tile_InsideMoveDown()	{tile_MoveDown();tile_MovePtrDown(bp);tile_MarkPtrDown(bp);}

/* Same as the above eight macros, but counting the number of moves only */

#define tile_InsideCntMoveRight()   {cnt--;tile_MovePtrRight(bp);}
#define tile_InsideCntMoveUp()	    {cnt--;tile_MarkPtrUp(bp);tile_MovePtrUp(bp);}
#define tile_InsideCntMoveLeft()    {cnt--;tile_MovePtrLeft(bp);}
#define tile_InsideCntMoveDown()    {cnt--;tile_MovePtrDown(bp);tile_MarkPtrDown(bp);}

/* more INSIDE MOVES for STROKING ONE PIXEL LINES */
#define tile_S1InsideMoveRight() {tile_MoveRight();if (!lastmovey) {tile_MarkPtr(bp);} \
    	    	    	    	  tile_MovePtrRight(bp);lastmovey=0;}

#define tile_S1InsideMoveUp()	 {tile_MoveUp();tile_MarkPtr(bp); \
    	    	    	    	  tile_MovePtrUp(bp);lastmovey=1;}

#define tile_S1InsideMoveLeft()	 {tile_MoveLeft();if (!lastmovey) {tile_MarkPtr(bp);} \
    	    	    	    	  tile_MovePtrLeft(bp);lastmovey=0;}

#define tile_S1InsideMoveDown()	 {tile_MoveDown();tile_MarkPtr(bp); \
    	    	    	    	  tile_MovePtrDown(bp);lastmovey=1;}

/* Same as the above eight macros, but counting the number of moves only */

#define tile_S1InsideCntMoveRight() {cnt--;if (!lastmovey) {tile_MarkPtr(bp);} \
    	    	    	    	     tile_MovePtrRight(bp);lastmovey=0;}

#define tile_S1InsideCntMoveUp()    {cnt--;tile_MarkPtr(bp);tile_MovePtrUp(bp);lastmovey=1;}

#define tile_S1InsideCntMoveLeft()  {cnt--;if (!lastmovey) {tile_MarkPtr(bp);} \
    	    	    	    	     tile_MovePtrLeft(bp);lastmovey=0;}

#define tile_S1InsideCntMoveDown()  {cnt--;tile_MarkPtr(bp);tile_MovePtrDown(bp);lastmovey=1;}


/* <Proc>,<XProc>,<YProc> alternatives */

#define tile_NullProc(movesp,x,y)  {}
void	tile_MoveFromLeft();
void	tile_MoveFromRight();

void	tile_MarkFromLeft();
void	tile_MarkFromInside();
void	tile_S1MarkFromInside();

    
/*
------------------------------------------------------------------------- 
    	M O V E    F R O M   R I G H T
------------------------------------------------------------------------- 
*/

void	tile_MoveFromRight(movesp,x,y)
    Rbyte   *movesp;
    Rint32  x,y;
{
    switch((int)rm_Direction(tile_rmbhdl)) {
    case rm_UPRIGHT:
    	break;
    case rm_UPLEFT:
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
    	    tile_MoveCheckXY(tile_MoveLeft,tile_MoveUp,
    	 	        tile_bbox.hix-1,tile_bbox.hiy,tile_MarkFromInside,tile_NullProc);
    	} else {
    	    tile_MoveCheckOne(tile_MoveLeft,tile_MoveUp,
    	 	        x,tile_bbox.hix-1,tile_MarkFromInside);
    	}
    	break;
    case rm_DOWNLEFT:
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
    	    tile_MoveCheckXY(tile_MoveLeft,tile_MoveDown,
    	    	tile_bbox.hix-1,tile_bbox.loy,tile_MarkFromInside,tile_NullProc);
    	} else {
    	    tile_MoveCheckOne(tile_MoveLeft,tile_MoveDown,
    	    	    	    x,tile_bbox.hix-1,tile_MarkFromInside);
    	}
    	break;
    case rm_DOWNRIGHT:
    	break;
    }
}

/*
------------------------------------------------------------------------- 
    	M A R K    L E F T    C O L U M N
------------------------------------------------------------------------- 
*/  

void	tile_MarkLeftColumn(rmbboxp)
    bbox_iBBox	*rmbboxp;
{
    Rint8   *bp;
    Rint32  hiy, loy, cnt;
    int32   direction; 

    hiy = min(rmbboxp->hiy, tile_bbox.hiy);
    loy = max(rmbboxp->loy, tile_bbox.loy);
    cnt = hiy - loy;
    direction = rm_Direction(tile_rmbhdl);
    if ( (direction == rm_UPLEFT) || (direction == rm_UPRIGHT) ) {
        bp = tile_GridYToWorkArrayPtr(loy);
    	while (cnt-- > 0) { tile_MarkPtrUp(bp);tile_MovePtrUp(bp);}
    } else {
        bp = tile_GridYToWorkArrayPtr(hiy);
    	while (cnt-- > 0) { tile_MovePtrDown(bp);tile_MarkPtrDown(bp);}
    }
    tile_UpdateActiveBand(loy,hiy);
}

/*
------------------------------------------------------------------------- 
    	M A R K    F R O M   L E F T   
------------------------------------------------------------------------- 
*/    

void	tile_MarkFromLeft(movesp,x,y)
    Rbyte   *movesp;
    Rint32  x,y;
{
    Rint8   *bp;
    Rint32  hiy,loy,cnt;

    switch((int)rm_Direction(tile_rmbhdl)) {
    case rm_UPRIGHT:
        bp = tile_GridYToWorkArrayPtr(y);
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
    	    tile_MoveCheckXY(tile_MoveRight,tile_InsideMoveUp,
    	    	    tile_bbox.lox,tile_bbox.hiy,tile_MarkFromInside,tile_NullProc);
    	} else {
    	    tile_MoveCheckOne(tile_MoveRight,tile_InsideMoveUp,
    	    	    	    	x,tile_bbox.lox,tile_MarkFromInside);
    	}
    	break;
    case rm_UPLEFT:
    	hiy = min(rm_EndPos(tile_rmbhdl).y, tile_bbox.hiy);
    	loy = max(y, tile_bbox.loy);
        cnt = hiy - loy;
        bp = tile_GridYToWorkArrayPtr(loy);
    	while (cnt-- > 0)  { tile_MarkPtrUp(bp);tile_MovePtrUp(bp); }
    	break;
    case rm_DOWNRIGHT:
        bp = tile_GridYToWorkArrayPtr(y);
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
    	    tile_MoveCheckXY(tile_MoveRight,tile_InsideMoveDown,
    	    	    	tile_bbox.lox,tile_bbox.loy,tile_MarkFromInside,tile_NullProc);
    	} else {
    	    tile_MoveCheckOne(tile_MoveRight,tile_InsideMoveDown,
    	    	    	x,tile_bbox.lox,tile_MarkFromInside);
    	}
    	break;
    case rm_DOWNLEFT:
    	hiy = min(y, tile_bbox.hiy);
    	loy = max(rm_EndPos(tile_rmbhdl).y, tile_bbox.loy);
        cnt = hiy - loy;
        bp = tile_GridYToWorkArrayPtr(hiy);
    	while (cnt-- > 0) { tile_MovePtrDown(bp);tile_MarkPtrDown(bp); }
    	break;
    }
}

/*
------------------------------------------------------------------------- 
    	M O V E   F R O M   L E F T  
------------------------------------------------------------------------- 
*/    
void	tile_MoveFromLeft(movesp,x,y)
    Rbyte   *movesp;
    Rint32  x,y;
{

    switch((int)rm_Direction(tile_rmbhdl)) {
    case rm_UPRIGHT:
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
    	    tile_MoveCheckXY(tile_MoveRight,tile_MoveUp,tile_bbox.lox,tile_bbox.hiy,
    	    	    	    	tile_MarkFromInside,tile_NullProc);
    	} else {
    	    tile_MoveCheckOne(tile_MoveRight,tile_MoveUp,
    	    	        	x,tile_bbox.lox,tile_MarkFromInside);
    	}
    	break;
    case rm_UPLEFT:
     	break;
    case rm_DOWNRIGHT:
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
    	    tile_MoveCheckXY(tile_MoveRight,tile_MoveDown,
    	    	    	tile_bbox.lox,tile_bbox.loy-1,tile_MarkFromInside,tile_NullProc);
    	} else {
    	    tile_MoveCheckOne(tile_MoveRight,tile_MoveDown,
    	    	    	x,tile_bbox.lox,tile_MarkFromInside);
    	}
    	break;
    case rm_DOWNLEFT:
     	break;
    }
}

/*
------------------------------------------------------------------------- 
    	M A R K    F R O M    I N S I D E 
------------------------------------------------------------------------- 
*/    

void	tile_MarkFromInside(movesp,x,y)
    Rbyte   *movesp;
    Rint32  x,y;
{
    Rint32  cnt;
    Rint8   *bp;

    if (tile_filltype == tile_STROKE1) {
    	tile_S1MarkFromInside(movesp,x,y);
    	return;
    }
    bp = tile_GridXYToWorkArrayPtr(x,y);
    switch((int)rm_Direction(tile_rmbhdl)) {
    case rm_UPRIGHT:
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_InsideMoveRight,tile_InsideMoveUp,
    	    	    	    	tile_bbox.hix,tile_bbox.hiy,tile_NullProc,tile_NullProc);
    	    } else {
    	    	tile_MoveCheckOne(tile_InsideMoveRight,tile_InsideMoveUp,
    	    	    	    	y,tile_bbox.hiy,tile_NullProc);
    	    }
    	}  else if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_InsideMoveRight,tile_InsideMoveUp,
    	    	    	    	x,tile_bbox.hix,tile_NullProc);
    	} else {
  	    cnt = rm_EndPos(tile_rmbhdl).y - y + rm_EndPos(tile_rmbhdl).x - x;
    	    tile_MoveCheckOne(tile_InsideCntMoveRight,tile_InsideCntMoveUp,
    	    	    	    	cnt,0,tile_NullProc);
	}
    	break;
    case rm_UPLEFT:
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_InsideMoveLeft,tile_InsideMoveUp,
    	    	    	    tile_bbox.lox,tile_bbox.hiy,tile_MarkFromLeft,tile_NullProc);
    	    } else {
    	    	tile_MoveCheckOne(tile_InsideMoveLeft,tile_InsideMoveUp,
    	    	    	    	y,tile_bbox.hiy,tile_NullProc);
    	    }
    	} else if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_InsideMoveLeft,tile_InsideMoveUp,
    	    	    	    	x,tile_bbox.lox,tile_MarkFromLeft);
    	} else {
  	    cnt = rm_EndPos(tile_rmbhdl).y - y + x - rm_EndPos(tile_rmbhdl).x;
    	    tile_MoveCheckOne(tile_InsideCntMoveLeft,tile_InsideCntMoveUp,
    	    	    	    	cnt,0,tile_NullProc);
	}
    	break;
    case rm_DOWNLEFT:
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_InsideMoveLeft,tile_InsideMoveDown,
     	    	    	    tile_bbox.lox,tile_bbox.loy,tile_MarkFromLeft,tile_NullProc);
    	    } else {
    	    	tile_MoveCheckOne(tile_InsideMoveLeft,tile_InsideMoveDown,
    	    	    	    y,tile_bbox.loy,tile_NullProc);
    	    }
    	} else if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_InsideMoveLeft,tile_InsideMoveDown,
    	    	    	    x,tile_bbox.lox,tile_MarkFromLeft);
    	} else {
  	    cnt = y - rm_EndPos(tile_rmbhdl).y + x - rm_EndPos(tile_rmbhdl).x;
    	    tile_MoveCheckOne(tile_InsideCntMoveLeft,tile_InsideCntMoveDown,
    	    	    	    cnt,0,tile_NullProc);
	}
    	break;
    case rm_DOWNRIGHT:
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_InsideMoveRight,tile_InsideMoveDown,
    	    	    	    tile_bbox.hix,tile_bbox.loy,tile_NullProc,tile_NullProc); 
    	    } else {
    	    	tile_MoveCheckOne(tile_InsideMoveRight,tile_InsideMoveDown,
    	    	    	    y,tile_bbox.loy,tile_NullProc);
    	    }
    	} else if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_InsideMoveRight,tile_InsideMoveDown,
    	    	    	    x,tile_bbox.hix,tile_NullProc);
    	} else {
  	    cnt = y - rm_EndPos(tile_rmbhdl).y + rm_EndPos(tile_rmbhdl).x - x;
    	    tile_MoveCheckOne(tile_InsideCntMoveRight,tile_InsideCntMoveDown,
    	    	    	    cnt,0,tile_NullProc);
	}
    	break;
    }
}


/*
------------------------------------------------------------------------- 
    	(STROKE ONE)  M A R K    F R O M    I N S I D E 
------------------------------------------------------------------------- 
*/    

void	tile_S1MarkFromInside(movesp,x,y)
    Rbyte   *movesp;
    Rint32  x,y;
{
    Rint32  cnt;
    Rint8   *bp;
    Rint32  lastmovey;

    if ( rm_IsFirstMove(tile_rmbhdl,movesp) ) 
    	lastmovey = 0;
    else    
    	lastmovey = (rm_PeekPrevMove(movesp) == rm_YMOVE) ? 1 : 0;
    bp = tile_GridXYToWorkArrayPtr(x,y);
    switch((int)rm_Direction(tile_rmbhdl)) {
    case rm_UPRIGHT:
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_S1InsideMoveRight,tile_S1InsideMoveUp,
    	    	    	    	tile_bbox.hix,tile_bbox.hiy,tile_NullProc,tile_NullProc);
    	    } else {
    	    	tile_MoveCheckOne(tile_S1InsideMoveRight,tile_S1InsideMoveUp,
    	    	    	    	y,tile_bbox.hiy,tile_NullProc);
    	    }
    	}  else if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_S1InsideMoveRight,tile_S1InsideMoveUp,
    	    	    	    	x,tile_bbox.hix,tile_NullProc);
    	} else {
  	    cnt = rm_EndPos(tile_rmbhdl).y - y + rm_EndPos(tile_rmbhdl).x - x;
    	    tile_MoveCheckOne(tile_S1InsideCntMoveRight,tile_S1InsideCntMoveUp,
    	    	    	    	cnt,0,tile_NullProc);
	}
    	break;
    case rm_UPLEFT:
    	if (tile_AboveHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_S1InsideMoveLeft,tile_S1InsideMoveUp,
    	    	    	    tile_bbox.lox-1,tile_bbox.hiy,tile_NullProc,tile_NullProc);
    	    } else {
    	    	tile_MoveCheckOne(tile_S1InsideMoveLeft,tile_S1InsideMoveUp,
    	    	    	    	y,tile_bbox.hiy,tile_NullProc);
    	    }
    	} else if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_S1InsideMoveLeft,tile_S1InsideMoveUp,
    	    	    	    	x,tile_bbox.lox-1,tile_NullProc);
    	} else {
  	    cnt = rm_EndPos(tile_rmbhdl).y - y + x - rm_EndPos(tile_rmbhdl).x;
    	    tile_MoveCheckOne(tile_S1InsideCntMoveLeft,tile_S1InsideCntMoveUp,
    	    	    	    	cnt,0,tile_NullProc);
	}
    	break;
    case rm_DOWNLEFT:
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_S1InsideMoveLeft,tile_S1InsideMoveDown,
    	    	    	    tile_bbox.lox-1,tile_bbox.loy-1,tile_NullProc,tile_NullProc);
    	    } else {
    	    	tile_MoveCheckOne(tile_S1InsideMoveLeft,tile_S1InsideMoveDown,
    	    	    	    y,tile_bbox.loy-1,tile_NullProc);
    	    }
    	} else if (tile_LeftOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_S1InsideMoveLeft,tile_S1InsideMoveDown,
    	    	    	    x,tile_bbox.lox-1,tile_NullProc);
    	} else {
  	    cnt = y - rm_EndPos(tile_rmbhdl).y + x - rm_EndPos(tile_rmbhdl).x;
    	    tile_MoveCheckOne(tile_S1InsideCntMoveLeft,tile_S1InsideCntMoveDown,
    	    	    	    cnt,0,tile_NullProc);
	}
    	break;
    case rm_DOWNRIGHT:
    	if (tile_BelowHorizontalBand(rm_EndPos(tile_rmbhdl).y)) {
       	    if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x))  {
    	    	tile_MoveCheckXY(tile_S1InsideMoveRight,tile_S1InsideMoveDown,
    	    	    	    tile_bbox.hix,tile_bbox.loy-1,tile_NullProc,tile_NullProc); 
    	    } else {
    	    	tile_MoveCheckOne(tile_S1InsideMoveRight,tile_S1InsideMoveDown,
    	    	    	    y,tile_bbox.loy-1,tile_NullProc);
    	    }
    	} else if (tile_RightOfTile(rm_EndPos(tile_rmbhdl).x)) {
    	    tile_MoveCheckOne(tile_S1InsideMoveRight,tile_S1InsideMoveDown,
    	    	    	    x,tile_bbox.hix,tile_NullProc);
    	} else {
  	    cnt = y - rm_EndPos(tile_rmbhdl).y + rm_EndPos(tile_rmbhdl).x - x;
    	    tile_MoveCheckOne(tile_S1InsideCntMoveRight,tile_S1InsideCntMoveDown,
    	    	    	    cnt,0,tile_NullProc);
	}
    	break;
    }
}

/*
------------------------------------------------------------------------- 
    	    T I L E    I S   T O U C H E D
------------------------------------------------------------------------- 
*/
/*
    Given a bounding box for a  path segment, <*bboxp>, and one of 
    the four possible path directions, <direction>, as declared
    by the rook move direction definition, returns true if tile
    of given type <tiletype> is affected. 
*/
bool	tile_isTouched(rmbboxp, tiletype, direction)
    register bbox_iBBox	*rmbboxp;
    int32   	    	tiletype,direction;
{
    int32   touched;

    touched = TRUE;
    if ( tiletype == tile_WORKTILE ) {
    	if (tile_filltype == tile_STROKE1) {    
    	    if ( (rmbboxp->lox >= tile_bbox.hix) ||
    	    	 (rmbboxp->hix <  tile_bbox.lox) || 
   	     	 (rmbboxp->loy >= tile_bbox.hiy) ||
   	     	 (rmbboxp->hiy < tile_bbox.loy) )
    	    	touched = FALSE;
    	} else {
    	    if ( (rmbboxp->lox >= tile_bbox.hix) ||
    	    	 (rmbboxp->loy >= tile_bbox.hiy) ||
             	 (rmbboxp->hiy <= tile_bbox.loy) )
    	    	touched = FALSE;
    	}
    }
    return(touched);
}


/*
------------------------------------------------------------------------- 
        	M A R K    T I L E    	(MAIN)
------------------------------------------------------------------------- 
*/
/*
    Given a rook move segment defined by the rook move buffer handle <bhdl>
    the intermediate representations of the allocated tile are marked based
    on the filling type and floobie dust control.
*/
void	tile_MarkTile( bhdl )
    rm_bufferHandle 	bhdl;
{ 
    bbox_iBBox	*rmbboxp;
    Rbyte    	*movesp;
    Rint32   	x,y;
    Rint16   	direction;    	

#ifdef DEBUGLEVEL2
    if (tile_debug) {
    	stile_ShowRMSequence( bhdl );
    }
#endif /*DEBUGLEVEL2*/

    tile_rmbhdl = bhdl;
    /* Mark floobiedust array if necessary */
    if ( tile_fdusttype != tile_NOFDUST ) {
    	tile_FDProcessRMoves();
    }

    /* Now mark Work array */
    rmbboxp = &rm_BBox( bhdl );
    x = rm_StartPos(bhdl).x;
    y = rm_StartPos(bhdl).y;
    direction = rm_Direction(bhdl);
    rm_InitMovesp(bhdl,movesp);

    /* Process paths outside tile */
    if (! tile_isTouched(rmbboxp,(int32)tile_WORKTILE,direction))
    	return; 
    if (tile_filltype != tile_STROKE1) {
    	if (rmbboxp->hix <= tile_bbox.lox) {
    	    tile_MarkLeftColumn(rmbboxp);
    	    return;
    	}
    }

    /* Process paths starting outside horizontal band and moving into band */
    if ( tile_AboveHorizontalBand(y) || tile_BelowHorizontalBand(y) ) {  
        switch(direction) {
    	case rm_UPRIGHT:
    	    tile_MoveCheckOne(tile_MoveRight,tile_MoveUp,y,tile_bbox.loy,tile_NullProc);
    	    break;
    	case rm_UPLEFT:
    	    tile_MoveCheckOne(tile_MoveLeft,tile_MoveUp,y,tile_bbox.loy,tile_NullProc);
    	    break;
    	case rm_DOWNLEFT:
    		if (tile_filltype != tile_STROKE1) {
    	    	tile_MoveCheckOne(tile_MoveLeft,tile_MoveDown,y,tile_bbox.hiy,tile_NullProc);
			} else {
    	    	tile_MoveCheckOne(tile_MoveLeft,tile_MoveDown,y,tile_bbox.hiy-1,tile_NullProc);
			}
    	    break;
    	case rm_DOWNRIGHT:
    		if (tile_filltype != tile_STROKE1) {
    	    	tile_MoveCheckOne(tile_MoveRight,tile_MoveDown,y,tile_bbox.hiy,tile_NullProc);
			} else {
    	    	tile_MoveCheckOne(tile_MoveRight,tile_MoveDown,y,tile_bbox.hiy-1,tile_NullProc);
			}
    	    break;
    	}
    }
    
    tile_UpdateActiveBand(y,rm_EndPos(bhdl).y);

    /* Process paths starting inside horizontal band & to the left of tile */
    if ( tile_LeftOfTile(x) ) {
    	if (tile_filltype == tile_STROKE1)
    	    tile_MoveFromLeft(movesp, x, y);
    	else
    	    tile_MarkFromLeft(movesp, x, y);    
    }

    /* Process paths starting inside horizontal band & to the right of tile */
    else if ( tile_RightOfTile(x) )
    	tile_MoveFromRight(movesp, x, y);

    /* Process paths starting inside tile */
    else
    	tile_MarkFromInside(movesp, x, y);
}    

/*
------------------------------------------------------------------------- 
    	T I L E    F I L L E R   D E F I N I T I O N S
------------------------------------------------------------------------- 
*/    

/* look up table for parity filling */
uint8	tile_FillTable[256] = {	    	
#include "filltbl.h"
};

/*
    Implicit declarations for the macros below

    Rint8   	<*sourcep>  	    pointer to work array 
    Rint8   	<*fdustp>  	    pointer to floobie dust array
    Ruint8   	<*destp>  	    pointer to final array 
    Rint8   	<prevstate> 	    previous state for filling next element 
    Ruint8  	<*filltablep>  	    pointer to parity fill table
    Ruint8  	<tmpbyte>
    Ruint32 	<tmplong>
    int32   	<bandheight>
    int8    	<width>	    	    band width in long words

*/

#define tile_FillBand(WAByteFill,FDByteFill)	    	    	    	\
    	    	{   	    	    	    	    	    	    	\
    	    	    while (bandheight-- > 0) {	    	    	    	\
    	    	    	prevstate=0;    	    	    	    	\
    	    	    	width = tile_BITTILEWIDTHINLONGS;	    	\
    	    	    	while (width-- > 0) {	    	    	    	\
    	    	    	    tile_FillLongword(WAByteFill,FDByteFill);	\
    	    	    	}   	    	    	    	    	    	\
    	    	    }	    	    	    	    	    	    	\
    	    	}


#define tile_FillLongword(WAByteFill,FDByteFill)	    	    	\
    	    	    	    	{   	    	    	    	    	\
    	    	    	    	    tmplong = 0;        	    	\
    	    	    	    	    WAByteFill(); FDByteFill();	    	\
    	    	    	    	    tmplong |= ((uint32)tmpbyte << 24);	    	\
    	    	    	    	    WAByteFill(); FDByteFill();	    	\
    	    	    	    	    tmplong |= ((uint32)tmpbyte << 16);	    	\
    	    	    	    	    WAByteFill(); FDByteFill();	    	\
    	    	    	    	    tmplong |= ((uint32)tmpbyte << 8);	    	\
    	    	    	    	    WAByteFill(); FDByteFill();	    	\
    	    	    	    	    tmplong |= (uint32)tmpbyte;	    	    	\
    	    	    	    	    *destp++ = tmplong;	    	    	\
    	    	    	    	 }

/* <WAByteFill> alternatives */

#define tile_EOByteFill()    	{	    	    	    	    	\
    	    	    	    	    tmpbyte = 0;    	    	    	\
    	    	    	    	    tile_ModifyByte(tile_EOByteTest);  	\
    	        	    	    tmpbyte = *(filltablep + tmpbyte);	\
    	                	    if (prevstate == 1) 	    	\
    		    	    	    	tmpbyte = ~tmpbyte;	    	\
	    	        	    prevstate = tmpbyte & 0x1;	    	\
    	    	    	    	}

#define tile_NZByteFill()   	{   	    	    	    	    	\
    	    	    	    	    tmpbyte=0;	    	    	    	\
    	    	    	    	    tile_ModifyByte(tile_NZByteTest);	\
    	    	    	    	}

#define tile_S1ByteFill()    	{   	    	    	    	    	\
    	    	    	    	    tmpbyte=0;	    	    	    	\
    	    	    	    	    tile_ModifyByte(tile_S1ByteTest);	\
    	    	    	    	}


/* <FDByteFill> alternatives */

#define tile_FDByteFill()   	{   tile_ModifyByte(tile_FDByteTest); }

#define tile_FDNullFill()   	{}



#define tile_ModifyByte(ByteTest)	    	    	    	    	\
    	    	    	    	{	    	    	    	    	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x80;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x40;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x20;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x10;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x08;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x04;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x02;	\
    	    	    	    	    if (ByteTest()) tmpbyte |= 0x01;	\
    	    	    	    	}

/* <ByteTest> alternatives */

#define tile_EOByteTest()   (*sourcep++ & 0x1)

#define tile_NZByteTest()   (prevstate= prevstate + *sourcep++)

#define tile_S1ByteTest()   (*sourcep++)

#define tile_FDByteTest()   (*fdustp++)



/*
------------------------------------------------------------------------- 
    	F I L L   T I L E    (MAIN)
------------------------------------------------------------------------- 
*/    
/*
    Intermediate representations of tile is filled and/or converted
    using the fill type and floobie dust control, if applicable,
    to obtain the resulting final tile.  The final tile is represented by
    an array of unsigned 32 bit integers of width tile_BITTILEWIDTHINLONGS. 
    This final tile is represented using MEMORY coordinates where each bit 
    represents one pixel in the tile.

    Returns a pointer to the final tile.
    Returns NULL if tile is empty

    IMPLEMENTATION:
    The tile filler operates on a horizontal band of the tile 
    "the active band" where previous marking operations may have affected
    the tile.  Information from the <WorkArray> for EOFILL, NZFILL an STROKE1
    marking operations is combined if necessary with information from the
    floobiedust array <FDustArray> and compacted to represent the <FinalArray>
    where each bit represents a pixel.

    The scan filling operations (for EOFILL and NZFILL) as well as the OR'ing
    for floobie dust information and the compaction from  a byte/pixel to
    a bit/pixel representation are done on the fly on an 8 pixel at a time basis.
    The macros to process 8 pixels for the fill operations are:
    	    	 <EOByteFill>, <NZByteFill>, <S1ByteFill>
    The additional macro to add floobie dust control information is <FDByteFill>

    Each case be distinguished by simply checking the <filltype> and <fdusttype>.
    The band filler macro <FillBand> can then be called with the appropriate
    Byte filler macros described above.
    
*/
uint32 	*tile_FillTile()
{
    Rint8    	*sourcep, *fdustp;
    Ruint32  	*destp;
    Rint8   	prevstate;
    Ruint8  	tmpbyte, *filltablep;
    Ruint32 	tmplong;
    int32   	bandheight;
    int8    	width;

    if (tile_empty)
    	return(NULL);

    sourcep = tile_GridYToWorkArrayPtr(tile_activebbox.hiy-1);
    fdustp = tile_GridYToFDustArrayPtr(tile_activebbox.hiy-1);
    destp = tile_GridYToFinalArrayPtr(tile_activebbox.hiy-1);
    bandheight = tile_activebbox.hiy - tile_activebbox.loy;
    /* Prepare floobiedust array for Filling if necessary */
    if ( tile_fdusttype != tile_NOFDUST ) {
    	tile_FDFillTile(fdustp,bandheight);
    }
    if ( tile_filltype == tile_EOFILL ) {
    	filltablep = tile_FillTable;
    	if (tile_fdusttype == tile_NOFDUST) {
			while (bandheight-- > 0) {
                 prevstate=0;
                 width = tile_BITTILEWIDTHINLONGS;
                 while (width-- > 0) {
                     tmplong = 0;
                     tile_EOByteFill(); 
                     tmplong |= (((uint32)tmpbyte) << 24);
                     tile_EOByteFill(); 
                     tmplong |= (((uint32)tmpbyte) << 16);
                     tile_EOByteFill(); 
                     tmplong |= (((uint32)tmpbyte) << 8);
                     tile_EOByteFill(); 
                     tmplong |= (uint32)tmpbyte;
                     *destp++ = tmplong;
                 }
             }
		} else {
        	while (bandheight-- > 0) {
				prevstate=0; 
				width = tile_BITTILEWIDTHINLONGS;
				while (width-- > 0) {
					tmplong = 0;
					tile_EOByteFill(); 
					tile_FDByteFill();
					tmplong |= (((uint32)tmpbyte) << 24);
					tile_EOByteFill(); 
					tile_FDByteFill();
					tmplong |= (((uint32)tmpbyte) << 16);
					tile_EOByteFill(); 
					tile_FDByteFill();
					tmplong |= (((uint32)tmpbyte) << 8);
					tile_EOByteFill(); 
					tile_FDByteFill();
					tmplong |= (uint32)tmpbyte;
					*destp++ = tmplong;
				}
			}
		}
    } else if ( tile_filltype == tile_NZFILL ) { 
    	if (tile_fdusttype == tile_NOFDUST) {
			while (bandheight-- > 0) {
				prevstate=0; 
				width = tile_BITTILEWIDTHINLONGS;
				while (width-- > 0) {
					tmplong = 0;
					tile_NZByteFill();
					tmplong |= (((uint32)tmpbyte) << 24);
					tile_NZByteFill(); 
					tmplong |= (((uint32)tmpbyte) << 16);
					tile_NZByteFill(); 
					tmplong |= (((uint32)tmpbyte) << 8);
					tile_NZByteFill(); 
					tmplong |= (uint32)tmpbyte;
					*destp++ = tmplong;
				}
			}
    	} else {
			while (bandheight-- > 0) {
				prevstate=0; 
				width = tile_BITTILEWIDTHINLONGS;
				while (width-- > 0) {
					tmplong = 0;
					tile_NZByteFill(); 
					tile_FDByteFill();
					tmplong |= (((uint32)tmpbyte) << 24);
					tile_NZByteFill(); 
					tile_FDByteFill();
					tmplong |= (((uint32)tmpbyte) << 16);
					tile_NZByteFill(); 
					tile_FDByteFill();
					tmplong |= (((uint32)tmpbyte) << 8);
					tile_NZByteFill(); 
					tile_FDByteFill();
					tmplong |= (uint32)tmpbyte;
					*destp++ = tmplong;
				}
			}
    	}
    } else { /* tile_STROKE1 */
		while (bandheight-- > 0) {
			prevstate=0; 
			width = tile_BITTILEWIDTHINLONGS;
			while (width-- > 0) {
				tmplong = 0;
				tile_S1ByteFill(); 
				tmplong |= (((uint32)tmpbyte) << 24);
				tile_S1ByteFill(); 
				tmplong |= (((uint32)tmpbyte) << 16);
				tile_S1ByteFill(); 
				tmplong |= (((uint32)tmpbyte) << 8);
				tile_S1ByteFill(); 
				tmplong |= (uint32)tmpbyte;
				*destp++ = tmplong;
			}
		}
    }

#ifdef DEBUGLEVEL2
    if (tile_debug) {
    	    stile_ShowTile( );
    }
#endif /*DEBUGLEVEL2*/

    return(&tile_FinalArray[0]);
}
