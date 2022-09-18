/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/showtile.h	1.1"
/*
 * @(#)showtile.h 1.2 89/03/10
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


#ifdef FULLBLOWN
/* DEFINITIONS FROM TILE.C EXPORTED FOR TESTING ONLY */
extern int32	tile_WorkArray[/*tile_BYTETILEAREAINLONGS*/];/* BYTE TILE (INT32 ALIGNED) */
extern int32	tile_FDustArray[/*tile_BYTETILEAREAINLONGS*/];/* BYTE TILE (INT32 ALIGNED) */
extern uint32	tile_FinalArray[/*tile_BITTILEAREAINLONGS*/]; /* BIT TILE */

extern int32   	    tile_filltype;   	/* see tile.h for types */
extern int32   	    tile_fdusttype;    	/* see tile.h for types */
extern bbox_iBBox   tile_bbox;	    	/* bounding box for tile */
extern bbox_iBBox   tile_activebbox;	/* bounding box for active region */
#endif /*FULLBLOWN*/

extern int32	stile_magpower;
extern bool	stile_usegrid;
extern bool 	stile_showrm;

/* display types */
extern int32	stile_display;
#define stile_NULLTILE	    	0
#ifdef FULLBLOWN
#define stile_WORKTILE  	1
#define stile_FDUSTTILE 	2
#define stile_FINALTILE 	3
#define stile_EMPTYTILE	    	4
#endif /*FULLBLOWN*/
