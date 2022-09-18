/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:olwsm/dtprop.h	1.17" */

#ifndef	_dtprop_h_
#define	_dtprop_h_

/******************************file*header********************************

    Description:
	This file contains the resource name definitions and their
	default values for Desktop properties.

	These are shared between dtm.c (where the resources are gotten
	from initially) and the property sheet.
*/
/* Define resource names:
	Use #define's since resource spec in property sheets is
	"*resourceName".  This can be achieved by making use of
	pre-processor string concatenation:
		"*" XtNresourceName.
*/
#define XtNfolderCols		"folderCols"
#define XtNfolderRows		"folderRows"
#define XtNgridWidth		"gridWidth"
#define XtNgridHeight		"gridHeight"
#define XtNshowFullPaths	"showFullPaths"
#define XtNsyncInterval		"syncInterval"
#define XtNtoolboxCols		"toolboxCols"
#define XtNtoolboxRows		"toolboxRows"
#define XtNtreeDepth		"treeDepth"
	
/* Define default values */
#define DEFAULT_FOLDER_COLS		5
#define DEFAULT_FOLDER_ROWS		2
#define DEFAULT_GRID_HEIGHT		62
#define DEFAULT_GRID_WIDTH		84
#define DEFAULT_SHOW_PATHS		False
#define DEFAULT_SYNC_INTERVAL		1000	/* in millisecs */
#define DEFAULT_TREE_DEPTH		2

#endif	/* _dtprop_h_ */
