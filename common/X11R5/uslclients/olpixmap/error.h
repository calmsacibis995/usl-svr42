/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olpixmap:error.h	1.10"
#endif

#ifndef __Ol_Pixmap_Error_h__
#define __Ol_Pixmap_Error_h__

/*
 *************************************************************************
 *
 * Description:
 *		This file contains standard error message strings for
 *	use in the routines OlVaDisplayErrorMsg() and
 *	OlVaDisplayWarningMsg().
 *
 *	When adding strings, the following conventions should be used:
 *
 *		1. Classes begin with OlC, e.g.,
 *			#define OleCOlClientOlpixmapMsgs "olpix_msgs"
 *
 *		2. Names begin with OleN, e.g.,
 *			#define	OleNinvalidResource	"invalidResource"
 *
 *		3. Types begin with OleT, e.g.,
 *			#define	OleTsetValues		"setValues"
 *
 *		4. Error message strings begin with OleM and is followed
 *		   by the name string, and underbar '_', and concatenated
 *		   with the error type.  For the above error name and type.
 *
 *			#define OleMinvalidResource_setValues \
 *			   "SetValues: widget \"%s\" (class \"%s\"): invalid\
 *			    resource \"%s\", setting to %s"
 *
 *	Using these conventions, an example use of OlVaDisplayWarningMsg() 
 *	for a bad resource in FooWidget's SetValues procedure would be:
 *
 *	OlVaDisplayWarningMsg(display, OleNinvalidResource, OleTsetValues,
 *		OleCOlToolkitWarning, OleMinvalidResource_setValues,
 *		XtName(w), XtClass(w)->core_class.class_name,
 *		XtNwidth, "23");
 *
 *******************************file*header*******************************
 */

#define OleCOlClientOlpixmapMsgs	"olpix_msgs"

/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************
 */

#define OleNbadCommandLine		"badCommandLine"
#define OleNfixedString			"fixedString"
#define OleNfooterMsg			"footerMsg"  
#define OleNmnemonic			"mnem"

/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleTappName  			"appName"
#define OleTusage			"usage"

/*
 *************************************************************************
 * Draw menu types
 *************************************************************************
 */
  
#define OleTpixels			"pixels"  
#define OleTtext			"text"  
#define OleTlines			"lines"  
#define OleTsegments			"segments"  
#define OleTovals			"ovals"  
#define OleTcircles			"circles"  
#define OleTrectangles			"rectangles"  
#define OleTsquares			"squares"  
  
/*
 *************************************************************************
 * Edit menu types
 *************************************************************************
 */
  
#define OleTrotate			"rotate"
#define OleTreflect			"reflect"
#define OleTfill			"fill"
#define OleTrecolor			"recolor"
#define OleTclear			"clear"
#define OleTmove			"move"
#define OleTroll			"roll"

#define OleTcopy2			"copy2"
#define OleTmove2			"move2"

/*
 *************************************************************************
 * File Menu Types
 *************************************************************************
 */
  
#define OleTbadBrowse			"badBrowse"
#define OleTbadFormat			"badFormat"
#define OleTbrowse			"browse"
#define OleTcancel			"cancel"  
#define OleTcancelBrowse		"cancelBrowse"
#define OleTcancelOpen			"cancelOpen"
#define OleTdiscard			"discard"  
#define OleTerror			"error"
#define OleTsfile			"sfile"
#define OleTfileName			"fileName"
#define OleTnew				"new"
#define OleTnoFile			"noFile"
#define OleTnoMemory			"noMemory"
#define OleTnoPermission		"noPermission"
#define OleTnoSave			"noSave"
#define OleTopen			"open"
#define OleTsave			"save"
#define OleTsaveAs			"saveAs"

/*
 *************************************************************************
 * Initialize Label Types
 *************************************************************************
 */
  
#define OleTdraw			"draw"
#define OleTpalette			"palette"
  
#define OleTpixmap			"pixmap"

  
/*
 *************************************************************************
 * Property sheet label types
 *************************************************************************
 */

#define OleTbadHW			"badHW"
#define OleTdashed			"dashed"
#define OleTfilled			"filled"
#define OleTgrid			"grid"
#define OleThollow			"hollow"
#define OleTinteriors			"interiors"
#define OleTlinestyle			"linestyle"
#define OleTlinewidth			"linewidth"
#define OleToff				"off"
#define OleTon				"on"
#define OleTpixheight			"pixheight"
#define OleTpixwidth			"pixwidth"
#define OleTsolid			"solid"
#define OleTsprops			"sprops"
#define OleTuntitled			"untitled"

/* More Labels */

#define OleTshowPixmap	"showPixmap"
#define OleTzoom	"zoom"
#define OleTzoomIn	"zoomIn"
#define OleTzoomOut	"zoomOut"

#define OleTfile	"file"
#define OleTview	"view"
#define OleToptions	"options"
#define OleTproperties	"properties"
  
/**************************************************************************
 * Define the default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */

extern String OleMbadCommandLine_usage ;

extern String OleMfixedString_appName ;
extern String OleMfixedString_circles ;
extern String OleMfixedString_lines ;
extern String OleMfixedString_ovals ;
extern String OleMfixedString_pixels ;
extern String OleMfixedString_rectangles ;
extern String OleMfixedString_segments ;
extern String OleMfixedString_squares ;
extern String OleMfixedString_text ;

extern String OleMfooterMsg_clear ;
extern String OleMfooterMsg_copy ;
extern String OleMfooterMsg_copy2 ;
extern String OleMfooterMsg_fill ;
extern String OleMfooterMsg_recolor ;
extern String OleMfooterMsg_move ;
extern String OleMfooterMsg_move2 ;
extern String OleMfooterMsg_roll ;

extern String OleMfixedString_sfile;
extern String OleMfixedString_fileName;

extern String OleMfooterMsg_badBrowse ;
extern String OleMfooterMsg_badFormat ;
extern String OleMfooterMsg_cancelBrowse ;
extern String OleMfooterMsg_cancelOpen ;
extern String OleMfooterMsg_error ;
extern String OleMfooterMsg_new ;
extern String OleMfooterMsg_noFile ;
extern String OleMfooterMsg_noMemory ;
extern String OleMfooterMsg_noPermission ;
extern String OleMfooterMsg_noSave ;
extern String OleMfooterMsg_open ;
extern String OleMfooterMsg_save ;

extern String OleMfixedString_edit ;
extern String OleMfixedString_draw ;
extern String OleMfixedString_palette ;

extern String OleMfixedString_pixmap ;

extern String OleMfixedString_dashed ;
extern String OleMfixedString_filled ;
extern String OleMfixedString_grid ;
extern String OleMfixedString_hollow ;
extern String OleMfixedString_interiors ;
extern String OleMfixedString_linestyle ;
extern String OleMfixedString_linewidth ;
extern String OleMfixedString_off ;
extern String OleMfixedString_on ;
extern String OleMfixedString_pixheight ;
extern String OleMfixedString_pixwidth ;
extern String OleMfixedString_solid ;
extern String OleMfixedString_sprops ;
extern String OleMfooterMsg_badHW ;

extern String OleMfixedString_browse;
extern String OleMfixedString_open;
extern String OleMfixedString_save;
extern String OleMfixedString_saveAs;
extern String OleMfixedString_showPixmap;
extern String OleMfixedString_zoom;
extern String OleMfixedString_zoomIn;
extern String OleMfixedString_zoomOut;
extern String OleMfixedString_file;
extern String OleMfixedString_view;
extern String OleMfixedString_options;
extern String OleMfixedString_properties;
extern String OleMfixedString_discard;
extern String OleMfixedString_cancel;

extern String OleMfixedString_fill;
extern String OleMfixedString_recolor;
extern String OleMfixedString_clear;
extern String OleMfixedString_copy;
extern String OleMfixedString_move;
extern String OleMfixedString_roll;

extern String OleMmnemonic_copy;
extern String OleMmnemonic_move;
extern String OleMmnemonic_roll;
extern String OleMmnemonic_fill;
extern String OleMmnemonic_recolor;
extern String OleMmnemonic_clear;

extern String OleMmnemonic_browse;
extern String OleMmnemonic_open;
extern String OleMmnemonic_save;
extern String OleMmnemonic_saveAs;
extern String OleMmnemonic_discard;
extern String OleMmnemonic_cancel;

extern String OleMmnemonic_showPixmap;
extern String OleMmnemonic_zoom;
extern String OleMmnemonic_zoomIn;
extern String OleMmnemonic_zoomOut;

extern String OleMmnemonic_file;
extern String OleMmnemonic_view;
extern String OleMmnemonic_properties;
extern String OleMmnemonic_options;
extern String OleMmnemonic_palette;
extern String OleMmnemonic_edit;
extern String OleMmnemonic_draw;


extern String OleMmnemonic_pixels;
extern String OleMmnemonic_text;
extern String OleMmnemonic_lines;
extern String OleMmnemonic_segments;
extern String OleMmnemonic_ovals;
extern String OleMmnemonic_circles;
extern String OleMmnemonic_rectangles;
extern String OleMmnemonic_squares;

extern String OleMmnemonic_interiors;
extern String OleMmnemonic_linewidth;
extern String OleMmnemonic_linestyle;
extern String OleMmnemonic_grid;
extern String OleMmnemonic_pixwidth;
extern String OleMmnemonic_pixheight;

#endif /* __Ol_Pixmap_Error_h__ */  
