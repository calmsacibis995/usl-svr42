/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#pragma ident	"@(#)Gizmo:error.h	1.3"
#endif

#ifndef _error_h
#define _error_h

/*
 *************************************************************************
 * Define the error classes here:  Use prefix of 'OleC'
 *************************************************************************
 */

#define OleCOlClientLibMsgs		"olclb_msgs"

/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************
 */

#define OleNbadCixels 			"badCixels"
#define OleNbadColormap 		"badColormap"
#define OleNbadColors			"badColors"
#define OleNbadDefine			"badDefine"
#define OleNbadPixels			"badPixels"
#define OleNbadPixmap			"badPixmap"
#define OleNbadSyntax			"badSyntax"
#define OleNfileDecorate		"fileDecorate"
#define OleNfileRegex			"fileRegex"
#define OleNfileExpand			"fileExpand"
#define OleNfileOlutil			"fileOlutil"
#define OleNstorage			"storage"
#define OleNbadRealloc			"badRealloc"

/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleTstackOverflow		"stackOverflow"
#define OleTtooManyColorsV1		"tooManyColorsV1"
#define OleTtooManyCharsPerPixel	"tooManyCharsPerPixel"
#define OleTcolorsNotDefined		"colorsNotDefined"
#define OleTbadColormapEntry1		"badColormapEntry1"
#define OleTbadColormapEntry2		"badColormapEntry2"
#define OleTbadCixelValue		"badCixelValue"
#define OleTbadColorDefSpec		"badColorDefSpec"
#define OleTpixelsNotDefined		"pixelsNotDefined"
#define OleTbadPixmapLineLength		"badPixmapLineLength"
#define OleTcixel1NotInPrevColormap	"cixel1NotInPrevColormap"
#define OleTcixel2NotInPrevColormap	"cixe21NotInPrevColormap"
#define OleTbadDefineNameType1		"badNameDefineType1"
#define OleTbadDefineNameType2		"badNameDefineType2"

#define OleTnotPrintable	 	"notPrintable" 
#define OleTbadCixel1			"badCixel1"
#define OleTbadCixel2			"badCixel2"
#define OleTbadFont			"badFont"
#define OleTentry1			"entry1"
#define OleTentry2			"entry2"
#define OleTcolorDefSpec		"colorDefSpec"
#define OleTnotDefined			"notDefined"
#define OleTtooMany			"tooMany"
#define OleTnameCharsPerPixel		"nameCharsPerPixel"
#define OleTnameHeight			"nameHeight"
#define OleTnameNcolors			"nameNcolors"
#define OleTnameWidth			"nameWidth"
#define OleTcharsPerPixel		"charsPerPixel"
#define OleTnotDefined			"notDefined"
#define OleTlineLength1			"lineLength1"
#define OleTlineLength2			"lineLength2"
#define OleTtooFewLines			"tooFewLines"
#define OleTcloseBracket		"closeBracket"
#define OleTnameColors			"nameColors"
#define OleTnameMono			"nameMono"
#define OleTnamePixels			"namePixels"
#define OleTcaptionName			"captionName"
#define OleTinputName			"inputName"
#define OleTchoiceName			"choiceName"
#define OleTstackName			"stackName"
#define OleTfree			"free"
#define OleTbadCalloc			"badCalloc"
#define OleTbadMalloc			"badMalloc"
#define OleTnullPointer			"nullPointer"
#define OleTfailed			"failed"

/*
 *************************************************************************
 * Define the default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */

#define OleMbadCixels_notPrintable  "bad cixel value : must be printable"
#define OleMbadColormap_badCixel1   "cixel \"%c\" not in previous colormap"
#define OleMbadColormap_badCixel2   "cixel \"%c%c\" not in previous colormap"
#define OleMbadColormap_entry1      "bad colormap entry : must be '\"c\" , \"colordef\",'"
#define OleMbadColormap_entry2      "bad colormap entry : must be '\"cC\" , \"colordef\",'"
#define OleMbadColors_colorDefSpec  "bad colordef specification : #RGB or colorname"
#define OleMbadColors_notDefined    "colors not defined"
#define OleMbadColors_tooMany       "Too many different colors, version 1"
#define OleMbadDefine_nameCharsPerPixel "bad '#define NAME_chars_per_pixel n' [1][2]"
#define OleMbadDefine_nameHeight    "bad '#define NAME_height n'"
#define OleMbadDefine_nameNcolors   "bad '#define NAME_ncolors n'"
#define OleMbadDefine_nameWidth     "bad '#define NAME_width n'"
#define OleMbadPixels_charsPerPixel "version 1.1 handles only 1 or 2 chars_per_pixel"
#define OleMbadPixels_notDefined    "pixels not defined"
#define OleMbadPixmap_lineLength1   "pixmap line length %d exceeds maximum of %d"
#define OleMbadPixmap_lineLength2   "bad pixmap line length %d (widths out of sync)" 
#define OleMbadPixmap_tooFewLines   "%d too few pixmap lines"
#define OleMbadSyntax_closeBracket  "bracket missing '} ;'"
#define OleMbadSyntax_nameColors    "bad 'static char * NAME_colors[] = {'"
#define OleMbadSyntax_nameMono      "missing '} ;' in NAME_mono[]"
#define OleMbadSyntax_namePixels    "bad 'static char * NAME_pixels[] = {'" 

#endif /* _error_h */
