/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/tile.h	1.1"
/*
 * @(#)tile.h 1.3 89/05/24
 *
 */
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

    COORDINATE SYSTEMS:
    ------------------

    let GRID coordinates have origin at lower left corner where 
            X increases to the right and Y increases upward.
    	    This is the coordinate system used for rookmoves, bboxes etc.

    let MEMORY coordinates have origin at upper left corner where
    	    X increases to the right and Y increases downward.
    	    This is the row-major coordinate system useful when a 
    	    rectangular region is declared as contiguous memory.


    TILE DEFINITION:
    ---------------

    A tile is defined to be a square of BITDIMENSION X BITDIMENSION
    representing an equivalent area of pixels in raster space.  Once
    allocated, a tile has a specific location in raster space and
    an associated bounding box.  Any pixel within the bounding box
    of the tile belongs to this and only this tile.


    TILE HANDLER:
    ------------

    Given a specific location for the tile in raster space and rook
    move segments affecting the tile, tile handler will mark and fill
    the tile according to the current mode.  A typical calling sequence
    for a path affecting one or more tiles is as follows:

    for (each tile affected by path) {
      ...AllocateTile(...);	     allocate tile, set position and mode 
      for (each path segment) {
    	if (...TileIsTouched(...))   check if path segment affects the tile
    	    ...MarkTile(...);	     mark the tile given a rook move sequence
      }
      tileptr = ...FillTile(..);     fill tile and get pointer to it
    	...
    }

    (see below for detail on procedure calls)

    FILLING/MARKING MODES:
    --------------------
    
    The mode is a combination of the filling type and floobie dust control:
    	    FILLTYPES    	    	    	    FLOOBIE DUST TYPES
    EOFILL  - 	parity fill 	    	    NOFDUST  	- no fdust
    NZFILL  - 	non-zero winding fill	    RIGHTFDUST	- right handed
    STROKE1 -	one pixel stroking  	    LEFTFDUST	- left handed

    The possible combinations for mode [FILLTYPE/FDUST] are :
        [EOFILL/NOFDUST]
    	[EOFILL/RIGHTFDUST]
    	[EOFILL/LEFTFDUST]
        [NZFILL/NOFDUST]
    	[NZFILL/RIGHTFDUST]
    	[NZFILL/LEFTFDUST]
        [STROKE1/NOFDUST]

    EOFILL and NZFILL both mark the tile and then scanfill horizontally between
    marks according to parity fill or non-zero fill respectively. STROKE1 on the
    other hand simply marks the tile for one pixel wide stroking.
    Floobie dust control (righthanded or lefthanded) if used, helps to add
    missing pixels when EOFILL and NZFILL operations are used when filling
    paths with almost tangent contours.  The outcome of the floobie dust control
    marking is simply ORed with the outcome of the filling operations on
    a given tile to obtain the final tile.

    The following examples depict Marking and Filling for various modes listed above. 
    The actual tile position will affect the following examples in that, only pixels 
    that fall inside the bounding box of the tile will be turned on.


    [EOFILL or NZFILL]  showing marks only 
    	    	    	Upward mark is <+1> downward mark is <-1>
    	    	    	Even-odd mark present if count is odd !
    	    	    	non-zero winding mark present if count is non-zero !
    	    	    	(Filling is done by horizontal scan filling between marks)

    	    	    	:           	    
                        :   	    	    	
                        :  	    	    	
   UPRIGHT          +- - -+ 	  DOWNRIGHT	  	    	
                    |X    |X      
    +-->        +- -+     +- -+     ---+        
    |           |X            |X       |    	
    |         +-+             +-+      |    	
              |X                |X     V    	
            +-+                 +-+ 	    	
            |X                    |X 	    	
    	  +-+                     +-+	        
          |X                        |X	        
          +                         +	       
          |X                        |X       
        +-+                         +-+                   
        |X                            |X    	    
        +                             +	    	    
  ......|X                            |X ....  	    
        +                             +	    	    
        |X                            |X     	    	
        +-+                         +-+	    	    
          |X                        |X	    	    
          +                         +	    	    
          |X                       |X	    	    
          +-+                     +-+	    	    
            |X                    |X 	    	    
    ^       +-+                 +-+    |    	    
    |         |X                |X     |    	    	
    |         +-+             +-+      |    	    	
    +--         |X            |X    <--+    	    	
                +- -+     +- -+    	    	      
  UPLEFT            |X    |X     DOWNLEFT   	    	
                    +- - -+ 	    	    	    
                      :	    	    	    	
                      :	    	    	    	
                      :	    	    	    	

    [STROKE1]	   the marks for stroking is <1> as is the same for
    	    	   any direction. There is no scan filling for
    	    	   one pixel stroking.

    	    	    	:  
                        : 
                       X:X X
   UPRIGHT          +- - -+        DOWNRIGHT	
                   X|X    |  X X  
    +-->        +- -+     +- -+     ---+
    |           |X            |  X     |
    |         +-+             +-+      |
              |X                |  X   V
            +-+                 +-+
            |X                    |  X 
    	  +-+                     +-+
          |X                        |X
          +                         +
          |X                        |  X
        +-+                         +-+                   
        |X                            |X
        +                             +
  ......|X                            |X ....  
        +                             +
        |X                           X| 
        +-+                         +-+
          |X                        |X
          +                         +
          |X                       X|
          +-+                     +-+
            |X                   X|
    ^       +-+                 +-+    |
    |         |X               X|      |
    |         +-+             +-+      |
    +--         |X X       X X|     <--+
                +- -+     +- -+    	    	      
  UPLEFT            |X X X|      DOWNLEFT
                    +- - -+
                      :
                      :
                      :


    [RIGHTHANDED FLOOBIE DUST]   showing the floobie dust tile only
    	    	    	    	this tile should be ORed with the
    	    	    	    	result from NZFILL and EOFILL operations.
                       :  
                       : 
                       : 
   UPRIGHT          +- - -+       DOWNRIGHT
                    |X X  |
    +-->        +- -+     +- -+     ---+
    |           |X X          |        |
    |         +-+             +-+      |
              |X                |      V
            +-+                 +-+
            |X                    |    
    	  +-+                     +-+
          |X                        |
          +                         +
          |X                        |  
        +-+                         +-+                   
        |X                            |
        +                             +
  ......|X                            | ....  
        +                             +
        |X                            | 
        +-+                         +-+
          |X                        |
          +                         +
          |X                        |
          +-+                     +-+
            |X                    |
    ^       +-+                 +-+    |
    |         |X                |      |
    |         +-+             +-+      |
    +--         |X X          |     <--+
                +- -+     +- -+    	    	      
  UPLEFT            |X    |      DOWNLEFT
                    +- - -+
                      :
                      :
                      :


    [LEFTHANDED FLOOBIE DUST]    showing the floobie dust tile only
    	    	    	    	this tile should be ORed with the
    	    	    	    	result from NZFILL and EOFILL operations.
                       :  
                       : 
                       : 
    DOWNLEFT        +- - -+        UPLEFT
                    |  X X|
    +---        +- -+     +- -+     <--+
    |           |          X X|        |
    |         +-+             +-+      |
    V         |                X|      |
            +-+                 +-+
            |                    X|    
    	  +-+                     +-+
          |                        X|
          +                         +
          |                        X|  
        +-+                         +-+                   
        |                            X|
        +                             +
  ......|                            X| ....  
        +                             +
        |                            X| 
        +-+                         +-+
          |                        X|
          +                         +
          |                        X|
          +-+                     +-+
            |                    X|
    |       +-+                 +-+    ^
    |         |                X|      |
    |         +-+             +-+      |
    +-->        |          X X|     ---+
                +- -+     +- -+    	    	      
  DOWNRIGHT         |  X X|      UPRIGHT
                    +- - -+
                      :
                      :
                      :



#endif /*COMMENT*/

#ifdef DEBUG
extern tile_debug;
#endif /*DEBUG*/

#define tile_BITDIMENSIONLOG	((int32)6) /* 2^6 = 64 */
#define tile_BITDIMENSION   	((int32)(1<<tile_BITDIMENSIONLOG))

#define tile_LONGWORDDIMENSIONLOG   ((int32)(tile_BITDIMENSIONLOG-5))
#define tile_LONGWORDDIMENSION	    ((int32)(1<<tile_LONGWORDDIMENSIONLOG))

/* definitions for BYTE tiles --> byte/pixel */
#define tile_BYTETILEWIDTHINBYTESLOG   	((int32)(tile_BITDIMENSIONLOG))
#define tile_BYTETILEWIDTHINBYTES   	((int32)(tile_BITDIMENSION))
#define tile_BYTETILEWIDTHINLONGSLOG   	((int32)(tile_BITDIMENSIONLOG-2))
#define tile_BYTETILEWIDTHINLONGS   	((int32)(tile_BITDIMENSION/4))
#define tile_BYTETILEAREAINLONGS    	((int32)(tile_BITDIMENSION *  tile_BYTETILEWIDTHINLONGS))

/* definitions for BIT tiles --> bit/pixel */
#define tile_BITTILEWIDTHINLONGSLOG     ((int32)(tile_LONGWORDDIMENSIONLOG))
#define tile_BITTILEWIDTHINLONGS    	((int32)(tile_LONGWORDDIMENSION))
#define tile_BITTILEAREAINLONGS     	((int32)(tile_BITDIMENSION *  tile_BITTILEWIDTHINLONGS))


/* 
------------------------------------------------------------------------
    	    B O O T   I N I T
------------------------------------------------------------------------
*/
/*
    Boot time initialization for tile handler
*/
extern void	tile_BootInit();

/* 
------------------------------------------------------------------------
    	A L L O C A T E     T I L E
------------------------------------------------------------------------
*/

/* Fill types */
#define tile_EOFILL  	0	    	/* parity fill */
#define	tile_NZFILL 	1	    	/* non-zero winding fill */
#define tile_STROKE1 	2   	    	/* one pixel line */

/* Floobie Dust	control types */
#define tile_NOFDUST 	0	    	/* no floobie dust control */
#define tile_RIGHTFDUST	1   	    	/* right handed */
#define tile_LEFTFDUST	2   	    	/* left handed */

/*
    Allocates a new tile given a position and marking/filling modes.
    <*origin> is the lower left corner of tile in GRID coordinates.
    <filltype> is one of the above fill types
    <fdusttype> is one of the above floobie dust control 
*/

extern void	tile_AllocateTile(/* origin, filltype, fdusttype */);
/*    pair_iXY	*origin;  */
/*    int32   	filltype;   */
/*    int32    	fdusttype;  */


/*
------------------------------------------------------------------------- 
    	T I L E   M A Y B E   A F F E C T E D 
------------------------------------------------------------------------- 
*/    
/*
    Check if bounding box of object may affect the tile.
*/
extern bool	tile_TileMaybeAffected(/* bbox */);
/*    bbox_iBBox    *bbox;	*/

/* 
------------------------------------------------------------------------
    	M A R K   T I L E
------------------------------------------------------------------------
*/
/*
    Given a rook move segment defined by the rook move buffer handle <bhdl>
    the intermediate representations of the allocated tile are marked based
    on the filling type and floobie dust control.
*/
extern void	tile_MarkTile(/* bhdl */);
/*    rm_bufferHandle 	bhdl;	*/


/* 
------------------------------------------------------------------------
    	F I L L    T I L E
------------------------------------------------------------------------
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
*/
extern uint32 	*tile_FillTile();












