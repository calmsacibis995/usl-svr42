/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/path.h	1.1"
/*
 * @(#)path.h 1.2 89/03/10
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


/*--------------------------------------------------------------------*/
/*
	The set of routines that identify a path handler.
*/

typedef struct _path_handler{
		bool	(*InitTraversal)();
		void	(*RestartTraversal)();
		bool	(*NextSubpath)();
		bool	(*NextArc)();
		void	(*SavePath)();
		void	(*RestorePath)();
		void	(*GetBBox)();
		void	(*GetDisp)();
		void	(*GetAutoRelative)();
		int32	(*GetPathSize)();
		int32	(*GetDustControl)();
		int8	  LShift;
} path_handler;



/*
	DEFINITIONS 
*/

/* These are the return values for <GetDustControl> */

#define	path_NOFDUST		0
#define	path_RIGHTFDUST		1
#define	path_LEFTFDUST		2


#ifdef	COMMENT

/*--------------------------------------------------------------------*/
/*
	The actual traversal functions.
*/
/*--------------------------------------------------------------------*/


bool	...InitTraversal(/*trans,pixelratio,closePath*/);
/*trans_dTrans	*trans;					 to be applied to the path */
/*float			pixelratio;		ratio of real pixels to ideal ones */ 
/*bool			closePath;				 force closure for filling */
/*
	This function should be called before any path can be traversed.
	It is assumed that the path has been created before this call is 
	made. If the existing path is not a valid one, the function will 
	return false. If all goes well, it will return true.
*/


void	...RestartTraversal();
/*
	This function provides a quick way to traverse a path that has been
	traversed before. It assumes that the transformation and closure
	of the path are the same as the last traversal.
*/


bool	...NextSubpath();
/*
	This function should be called to go to the next subpath of the 
	current path. It has to be called to get the first subpath of a path,
	and for all the subsequent ones. If it is called in the middle of a
	subpath, it will ignore the rest of the subpath information and go 
	to the next one. If the end of a subpath is reached, the traversal
	will not go to the next subpath automatically; there has to be an
	explicit call for a new subpath. It will return false when there are
	no more subpaths.
*/


bool	...NextArc(/*arc*/);
/*arc_frSegment		*arc;*/
/*
	This function should be called to get the next arc from the current 
	subpath. The structure for the arc has to be provided so that the 
	data can be put in it. The function will return true if it found an
	arc and false otherwise.
*/


/*--------------------------------------------------------------------*/
/*
	Some functions that provide information about a general path.
*/
/*--------------------------------------------------------------------*/


void	...GetBBox(/*trans, box*/);
/*trans_frTrans	*trans; */
/*bbox_iBBox	*box;*/
/*
	Puts the path's bounding box, after the current path's transformation
	has been applied, in the given structure.
*/


void	...GetDisp(/*disp*/);
/*pair_frXY	*disp;*/
/*
	Puts the path's displacement, after the current path's transformation
	has been applied, in the given structure.
*/


void	...GetAutoRelative(/*autorel*/);
/*trans_frTrans	*autorel;*/
/*
	Puts the path's autorelative transformation in the given structure.
*/


int32	...GetPathSize();
/*
	Returns the path's size.
*/


int32	...GetDustControl();
/*
    Returns the path's floobie dust control; always path_NOFDUST for
   general paths. 
*/

/*--------------------------------------------------------------------*/
/*
	Functions to manipulate the position of a general path in memory.
*/
/*--------------------------------------------------------------------*/


void	...SavePath(/*memorybegin*/);
/*int32	*memorybegin;*/
/*
	This function will save the path that has been defined in the path
	handler's memory into the memory provided by the caller. It assumes
	that there will be enough memory available for the whole path; that
	is to say, the caller has called <gpath_GetPathsize> prior to this
	call to determine the size of the path he wants to store.
*/


void	...RestorePath(/*startpath*/);
/*int32	*startpath;*/
/*
	This function will restore the path beginning at 'startpath' into
	the path handler's own memory. The next call has to be a
	<gpath_InitTraversal> ;  the caller cannot resume traversal of the
	path in mid-path.
*/

#endif /*COMMENT*/
