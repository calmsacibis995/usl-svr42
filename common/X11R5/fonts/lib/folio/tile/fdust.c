/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:tile/fdust.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)fdust.c 1.3 89/05/19";
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
#include "rmbuffer.h"
#include "tile.h"
#include "tilelcl.h"

#define tile_FDMEMHEIGHT	    ((int32)(tile_BITDIMENSION+4))
#define tile_FDMEMWIDTHINBYTES	((int32)(tile_BYTETILEWIDTHINBYTES+4))
#define tile_FDMEMWIDTHINLONGS	((int32)(tile_BYTETILEWIDTHINLONGS+1))
#define tile_FDMEMAREAINLONGS	((int32)(tile_FDMEMHEIGHT * tile_FDMEMWIDTHINLONGS))

typedef int32 tile_FDWorkMem[tile_FDMEMAREAINLONGS];    

tile_FDWorkMem	tile_FDHorMem,
    	    	tile_FDVerMem;

/*
---------------------------------------------------------------------------- 
           P O S I T I O N   D E F I N I T I O N S
---------------------------------------------------------------------------- 
*/  
#define tile_GridToFDMemX(x)	    (((x) - tile_bbox.lox)+1)
#define tile_GridToFDMemY(y)	    ((tile_bbox.hiy - (y) - 1)+1)

#define tile_GridYToFDMemPtr(aptr,width,y)  (aptr + (tile_GridToFDMemY(y)* width))

#define tile_GridXYToFDMemPtr(aptr,width,x,y) (tile_GridYToFDMemPtr(aptr,width,(y)) + \
    	    	    	    	    	    	tile_GridToFDMemX(x))

#define tile_GridXYToFDHMemPtr(x,y)    tile_GridXYToFDMemPtr((int8 *)tile_FDHorMem, \
    	    	    	    	    	    	    tile_FDMEMWIDTHINBYTES,(x),(y))

#define tile_GridXYToFDVMemPtr(x,y)    tile_GridXYToFDMemPtr((int8 *)tile_FDVerMem, \
    	    	    	    	    	    	    tile_FDMEMWIDTHINBYTES,(x),(y))

/*
---------------------------------------------------------------------------- 
           S T A T E   D E F I N I T I O N S
---------------------------------------------------------------------------- 
*/  
/* directions */
#define tile_UP	    0
#define tile_DOWN   1
#define tile_RIGHT  2
#define tile_LEFT   3

typedef struct {
    bool    	righthanded;	/* TRUE: righthanded FALSE: lefthanded */
    bool    	start;	    	/* TRUE: contour was started */
    int32	x, y;	    	/* current X,Y, position */
    int32	dir;   	    	/* current direction one of the above*/
    int32	count; 	    	/* number of moves in current direction */
    bool	first;	    	/* TRUE if first segment of a contour	*/
    int32   	firstdir;   	/* direction for first segment */
    int32   	firstcount; 	/* number of moves for first segment */
} tile_moveState;

tile_moveState	tile_mstate;

   
/* 
------------------------------------------------------------------------
    	 F D U S T     B O O T   I N I T
------------------------------------------------------------------------
*/
void tile_FDBootInit()
{
    tile_ClearLongArray(tile_FDHorMem, tile_FDMEMAREAINLONGS);
    tile_ClearLongArray(tile_FDVerMem, tile_FDMEMAREAINLONGS);
}

/* 
------------------------------------------------------------------------
    	F D U S T   C L E A R    B A N D   I N   M E M
------------------------------------------------------------------------
*/
void	tile_FDClearBandInMem(memp)
    int32   *memp;
{
    Rint32  *lp;
    Rint32  size;

    /* remember to clear the bands above AND below the the tile */
    lp = tile_GridYToFDMemPtr(memp,tile_FDMEMWIDTHINLONGS,(tile_activebbox.hiy));
    size = (tile_activebbox.hiy - tile_activebbox.loy+2) * tile_FDMEMWIDTHINLONGS;
    while( size-- > 0 ) {
    	    *lp++ = 0;
    }
}

/* 
------------------------------------------------------------------------
    	F D U S T     A L L O C A T E     T I L E
------------------------------------------------------------------------
*/
void	tile_FDAllocateTile()
{   
    if ( !tile_empty) {
    	tile_FDClearBandInMem(tile_FDHorMem);
    	tile_FDClearBandInMem(tile_FDVerMem);
    }
}

/* 
------------------------------------------------------------------------
    	F D U S T   M A R K
------------------------------------------------------------------------
*/
void	tile_FDMark()
{
    int32  x,y;
    Rint32   val;
    int32   diff;
    Rint32  count;
    Rint8  *thisp,*nextp;
    
    x = tile_mstate.x;
    y = tile_mstate.y;
    count = tile_mstate.count;
    val = count > 255 ? 255 : count;
    switch((int)tile_mstate.dir) {
    	case tile_UP:
    	    	if (x >= tile_bbox.lox && x <= tile_bbox.hix) {
    	    	    if (y < tile_bbox.hiy && (y+count) > tile_bbox.loy) {
    	    	    	if ((diff = tile_bbox.loy - y) > 0) {
    	    	    	    y += diff; count -= diff;
    	    	    	}
    	    	    	if ((diff = (y+count)-tile_bbox.hiy) > 0) {
    	    	    	    count -= diff;
    	    	    	}
    	    	    	if (tile_mstate.righthanded) {
    	    	    	    thisp = tile_GridXYToFDVMemPtr(x,y);
    	    	    	    nextp = thisp-1;
    	    	    	} else {
    	    	    	    thisp = tile_GridXYToFDVMemPtr(x-1,y);
    	    	    	    nextp = thisp+1;
    	    	    	}
    	    	    	while(count-->0) {
    	    	    	    if (val > *nextp) {
    	    	    	    	*thisp = val;
    	    	    	    	*nextp = 0;
    	    	    	    } else {
    	    	    	    	*thisp = 0;
    	    	    	    }
    	    	    	    thisp -= tile_FDMEMWIDTHINBYTES;
    	    	    	    nextp -= tile_FDMEMWIDTHINBYTES;
    	    	    	}
    	    	    }
    	    	}
    	    	break;
    	case tile_DOWN:
    	    	if (x >= tile_bbox.lox && x <= tile_bbox.hix) {
    	    	    if (y > tile_bbox.loy && (y-count) < tile_bbox.hiy) {
    	    	    	if ((diff = y - tile_bbox.hiy) > 0) {
    	    	    	    y -= diff; count -= diff;
    	    	    	}
    	    	    	if ((diff = tile_bbox.loy - (y-count)) > 0) {
    	    	    	    count -= diff;
    	    	    	}
    	    	    	if (tile_mstate.righthanded) {
    	    	    	    thisp = tile_GridXYToFDVMemPtr(x-1,y-1);
    	    	    	    nextp = thisp+1;
    	    	    	} else {
    	    	    	    thisp = tile_GridXYToFDVMemPtr(x,y-1);
    	    	    	    nextp = thisp-1;
    	    	    	}
    	    	    	while(count-->0) {
    	    	    	    if (val > *nextp) {
    	    	    	    	*thisp = val;
    	    	    	    	*nextp = 0;
    	    	    	    } else {
    	    	    	    	*thisp = 0;
    	    	    	    }
    	    	    	    thisp += tile_FDMEMWIDTHINBYTES;
    	    	    	    nextp += tile_FDMEMWIDTHINBYTES;
    	    	    	}
    	    	    }
    	    	}
    	    	break;
    	case tile_RIGHT:
    	    	if ( y >= tile_bbox.loy && y <= tile_bbox.hiy) {
    	    	    if (x < tile_bbox.hix && (x+count) > tile_bbox.lox) {
    	    	    	if ((diff = tile_bbox.lox - x) > 0) {
    	    	    	    x += diff; count -= diff;
    	    	    	}
    	    	    	if ((diff = (x+count) - tile_bbox.hix) > 0) {
    	    	    	    count -= diff;
    	    	    	}
    	    	    	if (tile_mstate.righthanded) {
    	    	    	    thisp = tile_GridXYToFDHMemPtr(x,y-1);
    	    	    	    nextp = thisp - tile_FDMEMWIDTHINBYTES;
    	    	    	} else {
    	    	    	    thisp = tile_GridXYToFDHMemPtr(x,y);
    	    	    	    nextp = thisp + tile_FDMEMWIDTHINBYTES;
    	    	    	}
    	    	    	while(count-->0) {
    	    	    	    if (val > *nextp) {
    	    	    	    	*thisp++ = val;
    	    	    	    	*nextp++ = 0;
    	    	    	    } else {
    	    	    	    	*thisp++ = 0;
    	    	    	    	nextp++;
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}
    	    	break;
    	case tile_LEFT:
    	    	if ( y >= tile_bbox.loy && y <= tile_bbox.hiy) {
    	    	    if (x > tile_bbox.lox && (x-count) < tile_bbox.hix) {
    	    	    	if ((diff = x - tile_bbox.hix) > 0) {
    	    	    	    x -= diff; count -= diff;
    	    	    	}
    	    	    	if ((diff = tile_bbox.lox - (x-count)) > 0) {
    	    	    	    count -= diff;
    	    	    	}
    	    	    	if (tile_mstate.righthanded) {
    	    	    	    thisp = tile_GridXYToFDHMemPtr(x-1,y);
    	    	    	    nextp = thisp + tile_FDMEMWIDTHINBYTES;
    	    	    	} else {
    	    	    	    thisp = tile_GridXYToFDHMemPtr(x-1,y-1);
    	    	    	    nextp = thisp - tile_FDMEMWIDTHINBYTES;
    	    	    	}
    	    	    	while(count-->0) {
    	    	    	    if (val > *nextp) {
    	    	    	    	*thisp-- = val;
    	    	    	    	*nextp-- = 0;
    	    	    	    } else {
    	    	    	    	*thisp-- = 0;
    	    	    	    	nextp--;
    	    	    	    }
    	    	    	}
    	    	    }
    	    	}
    	    	break;
    }
}

/* 
------------------------------------------------------------------------
    	F D U S T   M O V E 
------------------------------------------------------------------------
*/
void	tile_FDMove(dir)
    int32   dir;
{
    if (tile_mstate.count==0) {
    	tile_mstate.dir = dir;
    	tile_mstate.count = 1;
    } else if (tile_mstate.dir == dir) {
    	tile_mstate.count++;
    } else {
    	if ( ! tile_mstate.first ) {
    	    tile_FDMark();
    	} else {
    	    tile_mstate.first = FALSE;
    	    tile_mstate.firstdir = tile_mstate.dir;
    	    tile_mstate.firstcount = tile_mstate.count;
    	}
    	switch((int)tile_mstate.dir) {
    	    case tile_UP:	tile_mstate.y += tile_mstate.count; break;
    	    case tile_DOWN:	tile_mstate.y -= tile_mstate.count; break;
    	    case tile_RIGHT:	tile_mstate.x += tile_mstate.count; break;
    	    case tile_LEFT:	tile_mstate.x -= tile_mstate.count; break;
    	}
    	tile_mstate.count = 1;
    	tile_mstate.dir = dir;
    }
}

/* 
------------------------------------------------------------------------
    	F D U S T   P R O C E S S   R O O K  M O V E S
------------------------------------------------------------------------
*/
void	tile_FDProcessRMoves()
{
    Rbyte    	*movesp;
    Rint32   	x,y;
    int32   	xdir,ydir;
    Rint32   	count;    	

    switch((int)rm_Direction(tile_rmbhdl)) {
    	case rm_UPRIGHT:    ydir=tile_UP; xdir=tile_RIGHT; break;
    	case rm_UPLEFT:     ydir=tile_UP; xdir=tile_LEFT; break;
    	case rm_DOWNRIGHT:  ydir=tile_DOWN; xdir=tile_RIGHT; break;
    	case rm_DOWNLEFT:   ydir=tile_DOWN; xdir=tile_LEFT; break;
    }
    rm_InitMovesp(tile_rmbhdl,movesp);

    if (tile_mstate.start) {
    	tile_mstate.righthanded = (tile_fdusttype == tile_RIGHTFDUST) ? TRUE:FALSE;
    	tile_mstate.start = FALSE;
    	tile_mstate.first = TRUE;
    	tile_mstate.x = rm_StartPos(tile_rmbhdl).x;
    	tile_mstate.y = rm_StartPos(tile_rmbhdl).y;
    	tile_mstate.count = 0;
    }
    count = pair_ManhattanDistance(&rm_StartPos(tile_rmbhdl),&rm_EndPos(tile_rmbhdl));
    while (count-->0) {
    	if (rm_GetMove(movesp) == rm_XMOVE) {
    	    tile_FDMove(xdir);
    	} else {
    	    tile_FDMove(ydir);
    	}
    }
}


/* 
------------------------------------------------------------------------
    	F D U S T   S T A R T   C O N T O U R 
------------------------------------------------------------------------
*/
void	tile_FDStartContour()
{
    if ( tile_fdusttype != tile_NOFDUST) {
    	tile_mstate.start = TRUE;
    }
}

/* 
------------------------------------------------------------------------
    	F D U S T   E N D   C O N T O U R 
------------------------------------------------------------------------
*/
void	tile_FDEndContour()
{
    if ( tile_fdusttype != tile_NOFDUST) {
    	if ( tile_mstate.dir == tile_mstate.firstdir) {
    	    tile_mstate.count += tile_mstate.firstcount;
    	    tile_FDMark();
    	} else {
    	    tile_FDMark();
    	    switch((int)tile_mstate.dir) {
    	    	case tile_UP:	tile_mstate.y += tile_mstate.count; break;
    	    	case tile_DOWN:	tile_mstate.y -= tile_mstate.count; break;
    	    	case tile_RIGHT:tile_mstate.x += tile_mstate.count; break;
    	    	case tile_LEFT:	tile_mstate.x -= tile_mstate.count; break;
    	    }
    	    tile_mstate.dir = tile_mstate.firstdir;
    	    tile_mstate.count = tile_mstate.firstcount;
    	    tile_FDMark();
    	}
    }
}

/* 
------------------------------------------------------------------------
    	F D U S T    F I L L   T I L E 
------------------------------------------------------------------------
*/
void	tile_FDFillTile(fdustp,bandheight)
    Rint8  *fdustp;
    Rint32  bandheight;
{
    Rint8  *hmemp,*vmemp;
    Rint32  width;

    hmemp = tile_GridYToFDMemPtr((int8 *)tile_FDHorMem,tile_FDMEMWIDTHINBYTES,
    	    	    	    	    	    	(tile_activebbox.hiy-1)) + 1;
    vmemp = tile_GridYToFDMemPtr((int8 *)tile_FDVerMem,tile_FDMEMWIDTHINBYTES,
    	    	    	    	    	    	(tile_activebbox.hiy-1)) + 1;
    while (bandheight-->0) {
    	width = tile_BYTETILEWIDTHINBYTES;
    	while(width-->0) {
    	    if ( *hmemp++ | *vmemp++ )
    	    	*fdustp++ = 1;
    	    else
    	    	fdustp++;
    	}
    	hmemp += 4;
    	vmemp += 4;
    }
}

