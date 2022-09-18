/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/f3path.h	1.1"
/*
 * @(#)f3path.h 1.2 89/03/10
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


/*  	F3 PATH DEFINTION
 *
 *    	F3 paths are characters, symbols, logos, etc., described in the F3 language.
 *    	A path is defined by three items:
 *    	    	- Font name: The name of the file where the item exists.
 *    	    	- Character code: The number identifying the item inside the file.
 *    	    	- Character size: A scale to be applied to the dimensions of the
 *    	    	  item (the actual size depends also on the transformation, of
 *    	          course).
 */

typedef struct {
    	path_handler
    	    	*handler;
    	char	*name;
    	int 	 code;
    	float	 size;
}   	f3_PathTYPE;

extern	bool   	f3_SetFont( /* char *name */ );
extern	void	f3_SetSize( /* float size */ );
extern	void	f3_SetCode( /* int code */ );
/*  	Each function sets the respective component of the current F3 path
 *   	definition.
 */

extern	void	f3_SetGridFitting(  /* bool gridfit; bool controldust */ );
/* gridfit FALSE = no grid fitting; TRUE (default) = normal grid fitting */
/* control dust FALSE = no dust control; TRUE (default) = control dust	*/

/*  	PATH SAVING & RESTORING
 *
 *    	The current F3 path (i.e., the one defined by the most recent settings of
 *    	font name, size and character code) can be sized, saved and restored by
 *    	the following functions.
 */

extern	int 	f3_SizeOfPath();
/*  	Returns the amount of memory required to save the current F3 path, in
 *   	bytes.
 */

extern	void	f3_SavePath( /* char *destp */ );
/*  	Stores the description of the current F3 path in consecutive, increasing
 *   	memory locations starting at <destp>. It is the caller's responsibility
 *    	to ensure that enough space (i.e., f3_SizeOfPath() bytes) is available.
 */

extern	void	f3_RestorePath( /* char *sourcep */ );
/*  	Restores (i.e., makes current) the F3 path stored at <sourcep>; the previous
 *  	F3 path is destroyed.
 */

/*  	FONT NAME REGISTRATION
 *
 *  	It is important the remember that when an F3 path definition is saved, only the
 *  	pointer to the font name is stored, not the actual data; this can lead to problems
 *  	if the data is changed by the application. A way to avoid this problem is to
 *  	"register" font names.
 */
extern	char	*f3_RegisterName(   /* char *fname; */ );
/*  	Returns a pointer to a unique copy of <fname> */

/*	FONT BOUNDING BOX
 */
extern	bool	 f3_GetFontBBox( /* trans_dTrans *t, bbox_dBBox *b */ );
/*	Returns in <*b> the maximal bounding box (in pixels) for the
	current font at the current size, under the transformation
	<*t>
 */


/*  	F3 HANDLER INITIALIZATION
 *
 *  	An application using the F3 path handler must first initialize it. The purpose
 *  	of the initialization is to configure certain work areas; the sizes of these areas
 *  	will generally influence the performance and limits of the F3 interpreter.
 */
extern	bool	f3_PathInitialize(  /*	int 	*storeb, *storel;
    	    	    	    	    	int 	 fnamestorage;
    	    	    	    	    	int 	 xsavestorage;
    	    	    	    	    	int 	 ostackentries;
    	    	    	    	    	int 	 tempentries;
    	    	    	    	    	int 	 fontindices;	*/ );
/*  	<storeb>, <storel>: base and limit of the reserved work area; the rest of the arguments
 *    	    describe the way the area is partitioned (a total of ~ 64K bytes is suggested).
 *  	<fnamestorage>: bytes allocated to the font name registration area (sugg. 2000).
 *  	<xsavestorage>: bytes allocated to the execution stack (sugg. 2000)
 *  	<ostackentries>: number of entries in the operand stack (sugg. 100)
 *  	<tempentries>: number of entries for local storage (sugg. 2000)
 *  	<fontindices>: number of different font headers cached (sugg. 10)
 *
 *  	The rest of the storage (hopefully most of it) is used to cache preamble states.
 */

extern	path_handler	f3_handler;


