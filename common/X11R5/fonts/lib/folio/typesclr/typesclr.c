/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:typesclr/typesclr.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)typesclr.c 1.5 89/05/24";
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


#include <stdio.h>
#include <malloc.h>
#include "cdefs.h"
#include "common.h"
#include "rmbuffer.h"
#include "tile.h"
#include "arc.h"
#include "arcommon.h"
#include "path.h"
#include "typesclr.h"
#include "f3path.h"
#include "frmath.h"


int32	*type_WorkArea, *type_workAreaBase, *type_workAreaLimit;

trans_dTrans	type_trans; 	    /* current transformation */
path_handler	*type_path; 	    /* current path handler */
float	    	type_pixelratio;    /* real pixel size over ideal pixel size */

/* 
---------------------------------------------------------------------
    	G E T     S Y M B O L S
--------------------------------------------------------------------- 
*/
/* 
    Get all symbol codes for the given font <*fontname>. A maximum of 
    <maxcount> symbol codes  are placed in the given <codes> array.
    The return value contains the actual number of symbols in the
    file or <maxcount> whichever is less.
*/
int32	type_GetSymbolCodes(codes,maxcount)
    int32   *codes;
    int32   maxcount;
{
    return(f3_GetSymbolCodes(codes,maxcount));
}

/* 
---------------------------------------------------------------------
    	S E T      F O N T
--------------------------------------------------------------------- 
*/
/* 
    Set the current Font file name 
    Returns FALSE if it can no longer store font names.
*/
bool	type_SetFont(fontname)
    char    *fontname;
{
    return(f3_SetFont(fontname));
}


/* 
---------------------------------------------------------------------
    	S E T      S I Z E
--------------------------------------------------------------------- 
*/
/* 
    Set the current character size in units of 'em' 
*/
void	type_SetSize(size)
    double  size;
{
    f3_SetSize(size);
}

/* 
---------------------------------------------------------------------
    	S E T     C O D E
--------------------------------------------------------------------- 
*/
/*  
    Set the current character code 
*/
void	type_SetCode(code)
    int32   code;
{
    f3_SetCode(code);
}

/* 
---------------------------------------------------------------------
    	S E T   T R A N S F O R M A T I O N
--------------------------------------------------------------------- 
*/
/*
    Set the current transformation to the given <*trans>
*/
void	type_SetTrans(trans)
    trans_dTrans	*trans;
{
    type_trans = *trans;
    type_trans.dx = 0;
    type_trans.dy = 0;
}

/* 
---------------------------------------------------------------------
    	S E T   P I X E L R A T I O
--------------------------------------------------------------------- 
*/
/*
    Set the current pixel ratio <ratio> for the device
   pixelratio: (real pixel size over ideal pixel size)
*/
void	type_SetPixelRatio(ratio)
    float   ratio;
{
    type_pixelratio = ratio;
}

/* 
---------------------------------------------------------------------
    	I N I T   A C C E S S
--------------------------------------------------------------------- 
*/
/*
    Initialize the current character under the current transformation.
    Must be called AFTER modifying the current character definition  and/or the
    current transformation AND  BEFORE accessing functions that operate 
    on the current character under the current transformation. 
    Returns FALSE if the current character definition is not valid.
*/
bool	type_InitAccess()
{
    /* initialize path traversal */
    if ( ! (*type_path->InitTraversal)(&type_trans,type_pixelratio,(bool)TRUE) )
    	return(FALSE);
    return(TRUE);
}

/* 
---------------------------------------------------------------------
    	    G E T   F O N T  B B O X
--------------------------------------------------------------------- 
*/
/*
	Computes the bounding box <*bbox> for the font.
*/

bool	type_GetFontBBox(trans,bbox)
trans_dTrans	*trans;
bbox_dBBox		*bbox;
{
    return(f3_GetFontBBox(trans,bbox));
}

/* 
---------------------------------------------------------------------
    	    G E T   B B O X
--------------------------------------------------------------------- 
*/
/*
    Computes the Grid fitted bounding box <*bbox> for the current 
    character transformed by the current transformation. This 
    information may be used to set the dimension of the bitmap to be 
    filled by <type_MakeBitmap>
*/
void	type_GetBBox(bbox)
    bbox_iBBox	*bbox;
{
    (*type_path->GetBBox)(bbox);
    /* enlarge bbox to include special marking/filling cases */
    bbox_iEnlarge(bbox,bbox,(int32)1);
}

/* 
---------------------------------------------------------------------
    	    G E T   D I S P L A C E M E N T
--------------------------------------------------------------------- 
*/
/*
    Returns the displacement <*disp> for the current character transformed 
    by the current transformation.
*/
void	type_GetDisplacement(disp)
    pair_dXY	*disp;
{
	double		factor;
	pair_frXY	frdisp;

	factor = (double)(1 << type_path->LShift);
    (*type_path->GetDisp)(&frdisp);
	pair_Convert16FixedToDouble(disp,&frdisp);
	pair_XYScale(disp,disp,factor);
}


/*
    Returns the fixed point displacement <*disp> for the current character
    transformed by the current transformation.
*/
void	type_GetFrDisplacement(disp)
    pair_frXY	*disp;
{
    (*type_path->GetDisp)(disp);
}
/* 
---------------------------------------------------------------------
    	    G E T   A D V A N C E
--------------------------------------------------------------------- 
*/
/*
    Get the advance width of the current character under the current 
    transformation, remember to add this to the current position for 
    the next character.
*/
void	type_GetAdvance(advance)
    pair_dXY	*advance;
{
    trans_frTrans    autorel;
	pair_frXY		fradvance;
	double		factor;

	factor = (double)(1 << type_path->LShift);
    (*type_path->GetAutoRelative)(&autorel);
    fradvance.x = autorel.dx;
    fradvance.y = autorel.dy;
	pair_Convert16FixedToDouble(advance,&fradvance);
	pair_XYScale(advance,advance,factor);
}


/*
    Get the advance width of the current character,in fixed point,
    under the current transformation, remember to add this to the 
    current position for the next character.
*/
void	type_GetFrAdvance(advance)
    pair_frXY	*advance;
{
    trans_frTrans    autorel;

    (*type_path->GetAutoRelative)(&autorel);
    advance->x = autorel.dx;
    advance->y = autorel.dy;
}

/* 
---------------------------------------------------------------------
    	G E T   F D U S T    T Y P E
--------------------------------------------------------------------- 
*/
/*
    Map client dust control type to dust control type for tile handler.
*/
int32	type_GetFDustType(fdusttype)
    int32   fdusttype;
{
    switch ((int)fdusttype) {
    	case path_NOFDUST:  	return(tile_NOFDUST);
    	case path_RIGHTFDUST:  	return(tile_RIGHTFDUST);
    	case path_LEFTFDUST:  	return(tile_LEFTFDUST);
    }
}


/* 
---------------------------------------------------------------------
    	    P R O C E S S    A R C
--------------------------------------------------------------------- 
*/
/*
    Process one simple arc segment (a line, conic or a bezier) to
    generate Marks on the current tile to be later filled.
	A simple arc is one which is contained in one quadrant, is
	small enough to be tracable and fits in a rook move buffer.
*/
void type_ProcessSimpleArc(frarc,disp,neworigin)
arc_frSegment *frarc;
pair_iXY		*disp;
bool			neworigin;
{
	register    rm_bufferHandle bhdl;
	int32       size;

	switch (frarc->type) {
		case arc_LINETYPE:
			{
			pair_iXY        ifrom,ito;
			if ( ! tile_TileMaybeAffected(&frarc->bbox) )
				return;
			pair_RoundFixedToInt(&ifrom,&frarc->data.line.from);
            pair_RoundFixedToInt(&ito,&frarc->data.line.to);
            size = pair_ManhattanDistance( &ifrom, &ito );
      		bhdl = rm_AllocateBuffer( size );
			seg_TraceLine(&frarc->data.line,bhdl);
			if (neworigin) {
				pair_XYAdd(&ifrom,&ifrom,disp);
				pair_XYAdd(&ito,&ito,disp);
			}
			rm_bufferInitHeader(bhdl,&ifrom,&ito);
            tile_MarkTile( bhdl );
            rm_ClearBuffers( );
			}
			break;
		case arc_CONICTYPE:
			{
			pair_iXY        ifrom,ito; 
			if ( ! tile_TileMaybeAffected(&frarc->bbox) ) 
				return;
			pair_RoundFixedToInt(&ifrom,&frarc->data.conic.a); 
			pair_RoundFixedToInt(&ito,&frarc->data.conic.c); 
			size = pair_ManhattanDistance( &ifrom, &ito ); 
			bhdl = rm_AllocateBuffer( size );
			seg_TraceConic(&frarc->data.conic,bhdl); 
			if (neworigin) {
				pair_XYAdd(&ifrom,&ifrom,disp);
				pair_XYAdd(&ito,&ito,disp);
			}
			rm_bufferInitHeader(bhdl,&ifrom,&ito);
			tile_MarkTile( bhdl ); 
			rm_ClearBuffers( );  
			}
			break; 
		case arc_BEZIERTYPE:
			break;
	}
}

/*-----------------------------------------------------------------------*/

/*
    Process one arc segment (a line, conic or a bezier) to
    generate Marks on the current tile to be later filled.
*/
void type_ProcessArc(frarc)
arc_frSegment *frarc;
{
    arc_frSegment 	myarc;
	pair_iXY		disp;

	disp.x = 0; disp.y = 0;
    /* Process the arc iff it affects the current tile */
	/* first get the arc's bbox */
    arccommon_GetFrArcBBox(frarc,&frarc->bbox,type_path->LShift);
    if ( ! tile_TileMaybeAffected(&frarc->bbox) )
    	return;

	/* check if arc is simple -- in one quadrant and small enough */
	if (arccommon_SimpleArc(frarc,type_path->LShift)) {
		/* frarc is a simple arc with no shift */
		type_ProcessSimpleArc(frarc,&disp,(bool)FALSE);
	} else {							/* not simple, so spilt it */
    	arc_SplitArcAndFillStack(frarc,type_path->LShift);
		while (arc_PopStack(&myarc,&disp)) {
			type_ProcessSimpleArc(&myarc,&disp,(bool)TRUE);
		}
	}
}

/* 
---------------------------------------------------------------------
     F I L L   M E M O R Y 
--------------------------------------------------------------------- 
*/
/*
    Fill the current tile and transfer the data in the appropriate
    location of the given memory patch. The tile is ORed with the
    memory patch, this puts the burden of clearing the memory on
    the client if it so desires.
*/
void	type_FillMem(mem,tileorigin)
    type_memory     *mem;
    pair_iXY	    *tileorigin;
{
    Ruint32  	*tile, *bitmap;
    int32   	tileinc, bitmapinc;
    int32   	width;
    Rint32  	height,j;

    /* fill bitmap iff tile is not empty */
    if ( (tile = tile_FillTile()) != NULL ) {
    	/* position the bitmap pointer given the tile position */
    	bitmap = mem->bitmap + (tileorigin->x - mem->origin.x) / 32 + 
    	 ((mem->origin.y + mem->height) - (tileorigin->y + tile_BITDIMENSION))* 
    	    (mem->width >> 5);
    	if (tileorigin->y >= mem->origin.y)
    	    height = tile_BITDIMENSION;
    	else
    	    height = (tileorigin->y + tile_BITDIMENSION) - mem->origin.y;
    	if ((tileorigin->x + tile_BITDIMENSION) <= (mem->origin.x + mem->width))
    	    width =  tile_BITDIMENSION;
    	else
    	    width = (mem->origin.x + mem->width) - tileorigin->x;
    	width /= 32;
    	tileinc = tile_LONGWORDDIMENSION - width;
    	bitmapinc = (mem->width>>5) - width;
    	while (height-- > 0) {
    	    j = width;
    	    while (j-- > 0) {
    	    	*bitmap++ |= *tile++;
    	    }
    	    tile  += tileinc;
    	    bitmap += bitmapinc;
    	}
    }
}

/* 
---------------------------------------------------------------------
    	   T Y P E   S C A L E R
--------------------------------------------------------------------- 
*/
/* 
    Transform the current character using the current transformation and
    generate a bitmap defined by <mem>. If only an outline of the
    character is required set <outline> to TRUE otherwise set it to FALSE.
*/
void	type_MakeBitmap(mem,outline)
    type_memory     *mem;
    bool    	    outline;
{
    pair_dXY	    disp;
    pair_iXY	    tileorigin;
    int32   	    xlimit,ylimit;
    arc_frSegment	frarc;
    int32   	    filltype, fdusttype;
    
    
    /* 
    	Process the path for each tile touched by the given memory.

    */
    type_GetDisplacement(&disp);
    filltype = outline ? tile_STROKE1 : tile_NZFILL;
    fdusttype = type_GetFDustType((*type_path->GetDustControl)());
    ylimit = mem->origin.y - tile_BITDIMENSION;
    xlimit = mem->origin.x + mem->width;
    tileorigin.y = mem->origin.y + mem->height - tile_BITDIMENSION;
    while (tileorigin.y > ylimit) {
    	tileorigin.x = mem->origin.x;
    	while ( tileorigin.x < xlimit) {
    	    tile_AllocateTile(&tileorigin,filltype,fdusttype);
    	    while ((*type_path->NextSubpath)()) {
    	    	tile_FDStartContour();
    	    	while ((*type_path->NextArc)(&frarc)) {
    	    	    type_ProcessArc(&frarc);
    	    	}
    	    	tile_FDEndContour();
    	    }
    	    type_FillMem(mem,&tileorigin);
    	    tileorigin.x += tile_BITDIMENSION;
    	    (*type_path->RestartTraversal)();
    	}
    	tileorigin.y -= tile_BITDIMENSION;
    }
}


/* 
---------------------------------------------------------------------
    	   B O O T  I N I T
--------------------------------------------------------------------- 
*/
/* 
    Boot time initialization for typescaler
    Returns FALSE if typescaler can not initialize
    under current configuration;
*/
bool	type_BootInit()
{
    type_trans = trans_Identity;
    type_path = &f3_handler;
    type_pixelratio = 1.0;

#ifdef	MSDOS
	type_WorkArea=halloc((((int32)type_WORKAREASIZE)>>2),(int)4);
#else
	type_WorkArea=(int32*)malloc((int32)type_WORKAREASIZE);
#endif

	if (type_WorkArea==NULL)
		return(FALSE);
	type_workAreaBase=type_WorkArea;
	type_workAreaLimit=type_WorkArea+(((int32)type_WORKAREASIZE)>>2);

    if ( ! f3_PathInitialize(type_workAreaBase,type_workAreaLimit,
    	    type_FONTNAMESTORAGESIZE,type_EXECSTACKSIZE,
    	    type_OPSTACKSIZE,type_LOCALSTACKSIZE,
    	    type_INDEXCACHESIZE) )
    	return(FALSE);
    tile_BootInit();
    rm_BootInit();
    return(TRUE);
}
