/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:error.h	1.14"
#endif

#ifndef __olps_error_h__
#define __olps_error_h__

/*
 *************************************************************************
 *
 * Description:
 *		This file contains standard error message strings for
 *		olprintscreen.
 *
 *	When adding strings, the following conventions should be used:
 *
 *		1. Error classes begin with OleC, e.g.
 *			#define OleCOlClientOlpsMsgs	"olps_msgs"
 *
 *		2. Error names begin with OleN, e.g.,
 *			#define	OleNinvalidResource	"invalidResource"
 *
 *		3. Error types begin with OleT, e.g.,
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

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/ChangeBar.h>

/*
 *************************************************************************
 * External function definitions: these functions are in Xol/Error.c
 *************************************************************************
 */

extern void OlVaDisplayErrorMsg OL_ARGS((
	Display *	d, 
	OLconst char *	s1, 
	OLconst char * 		s2, 
	OLconst char * 		s3, 
	OLconst char * 		s4, 
	...
));

extern void OlVaDisplayWarningMsg OL_ARGS((
	Display *	d, 
	OLconst char * 		s1, 
	OLconst char * 		s2, 
	OLconst char * 		s3, 
	OLconst char * 		s4, 
	...
));

extern void OlError OL_ARGS((
	OLconst char * 		s
));

extern void OlWarning OL_ARGS((
	OLconst char * 		s
));

extern OlErrorHandler OlSetErrorHandler OL_ARGS((
	OlErrorHandler	oleh
));

extern OlVaDisplayErrorMsgHandler OlSetVaDisplayErrorMsghandler OL_ARGS((
	OlVaDisplayErrorMsgHandler	ovdemh
));

extern OlVaDisplayWarningMsgHandler OlSetVaDisplayWarningMsgHandler OL_ARGS((
	OlVaDisplayWarningMsgHandler	oldwmh
));

extern OlWarningHandler OlSetWarningHandler OL_ARGS((
	OlWarningHandler		owh
));

extern XrmDatabase
OlOpenDatabase OL_ARGS((
                        Display *,
                        OLconst char *
));

extern void
OlCloseDatabase OL_ARGS((
                         XrmDatabase
));

extern char * OlGetMessage OL_ARGS((
	Display *	d, 
	char *		c, 
	int		i, 
	OLconst char * 		s1, 
	OLconst char * 		s2, 
	OLconst char * 		s3, 
	OLconst char * 		s4, 
	XrmDatabase	db
));

#if	defined(__STDC__)
#define concat(a,b)	a ## b
#else
#define concat(a,b)	a/**/b
#endif  
  
/*
 *************************************************************************
 * Define the error classes here:  Use prefix of 'OleC'
 *************************************************************************
 */
  
#define OleCOlClientOlpsMsgs	"olps_msgs"

/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************
 */

#define OleNerrorMsg			"errorMsg"
#define OleNfooterMsg			"footerMsg"
#define OleNbadFilename			"badFilename"
#define OleNbutton			"button"
#define OleNcaption			"caption"
#define OleNfileMain			"fileMain"
#define OleNmnemonic			"mnem"
#define OleNprintScreen			"printScreen"
#define OleNtitle			"title"
#define OleNbadFormat			(OLconst char *)"badFormat"
#define OleNbadPages			"badPages"
#define OleNbadScale			"badScale"
#define OleNbadLeftOffset		"badLeftOffset"
#define OleNbadTopOffset		"badTopOffset"
#define OleNbadFieldWidth		"badFieldWidth"
#define OleNbadFieldHeight		"badFieldHeight"
#define OleNbadCommand			"badCommand"

/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleTareaSpooled			"areaSpooled"
#define OleTbuffer			"buffer"
#define OleTcantAllocateColors		"cantAllocateColors"
#define OleTcantCallocBuffer		"cantCallocBuffer"
#define OleTcantCallocName		"cantCallocName"
#define OleTcantFindVisual		"FindVisual"
#define OleTcantReadHeader		"cantReadHeader"
#define OleTcantReadName		"cantReadName"
#define OleTcantReadMap		        "cantReadMap"
#define OleTcantReadPixmap	        "cantReadPixmap"
#define OleTcantQueryMap	        "cantQueryMap"
#define OleTcontentsSpooled		"contentsSpooled"
#define OleTcolorsTest		        "colorsTest"
#define OleTcolors		        "colors"
#define OleTdoneCreate			"doneCreate"
#define OleTerrorCreatingFile		"errorCreatingFile"
#define OleTerrorOpeningFile		"errorOpeningFile"
#define OleTexiting	         	"exiting"
#define OleTheaderTooSmall		"headerTooSmall"
#define OleTformatMissMatch		"formatMissMatch"
#define OleTfileLoaded			"fileLoaded"
#define OleTfileNotOrdinary		"fileNotOrdinary"
#define OleTfileSpooled			"fileSpooled"
#define OleTfileUnaccessible		"fileUnaccessible"
#define OleTfileWritten			"fileWritten"
#define OleTisAdirectory		"isAdirectory"

/* Labels for buttons and captions */

#define OleTlabelArea			"area"
#define OleTlabelCapture		"capt"
#define OleTlabelCaptureImage		"captimg"
#define OleTlabelContents		"contents"
#define OleTlabelDefaultPath		"defpath"
#define OleTlabelFile			"file"
#define OleTlabelFooter			"foot"
#define OleTlabelHeader			"header"
#define OleTlabelImageFile		"imagef"
#define OleTlabelLandscape		"landscape"
#define OleTlabelLeftOffset		"leftofst"

#define OleTlabelMaxHt			"maxht"
#define OleTlabelMaxWidth		"maxwid"

#define OleTlabelOff			"off"
#define OleTlabelOn			"on"
#define OleTlabelOpen			"Open"
#define OleTlabelOpen2			"Open2"
#define OleTlabelOptions		"options"
#define OleTlabelOrientation		"orient"
#define OleTlabelPixformat		"pixformat"
#define OleTlabelOutputFormat		"outfmt"
#define OleTlabelPages			"pgs"
#define OleTlabelPortrait		"portrait"

#define OleTlabelPrint			"Print"
#define OleTlabelPrintCmd		"pcmd"
#define OleTlabelProperties		"props"
#define OleTlabelReverseVideo		"revvideo"
#define OleTlabelSave			"Save"
#define OleTlabelCancel			"Cancel"
#define OleTlabelSave2			"Sav2"
#define OleTlabelScale			"scale"
#define OleTlabelScreen			"screen"
#define OleTlabelTopOffset		"topofst"
#define OleTlabelWindow			"window"
#define OleTlabelZpixmap		"zpixmap"
#define OleTlabelXYpixmap		"xypixmap"

#define OleTloading			"loading"
#define OleTnewColorsNeeded		"newColorsNeeded"
#define OleToldColorsMatch		"oldColorsMatch"
#define OleTnoContents			"noContents"
#define OleTnoContentsPrint		"noContentsPrint"
#define OleTnoContentsSave		"noContentsSave"
#define OleTnonexistentFile		"nonexistentFile"
#define OleTnoMemory			"noMemory"
#define OleTnoPathAccess		"noPathAccess"
#define OleTnotDumpFile	         	"notDumpFile"
#define OleTnoWinAttrs			"noWinAttrs"
#define OleToverwrite			"overwrite"
#define OleTpathInvalid		        "pathInvalid"
#define OleTpathUnaccessible	        "pathUnaccessible"
#define OleTprepareContents		"prepareContents"
#define OleTprepareFile			"prepareFile"
#define OleTprintFailed			"printFailed"

#define OleTtooManyFiles		"tooManyFiles"
#define OleTsaveCancelled		"saveCancelled"
#define OleTopenCancelled		"openCancelled"
#define OleToperCancelled		"operCancelled"
#define OleTprintCancelled		"printCancelled"
#define OleTsaveContents		"saveContents"
#define OleTscreenSpooled		"screenSpooled"
#define OleTtooManyPlanes		"tooManyPlanes"
#define OleTwindowSpooled		"windowSpooled" 
#define OleTwinNameTest		         "winNameTest" 
#define OleTwinName		         "winName" 
#define OleTblankDefault		"blankDefault"
#define OleTbadShmimage			"badShimimage"
#define OleTbadShmget			"badShmget"
#define OleTbadShmat			"badShmat"
#define OleTfileTitle			"fileTitle"
#define OleTprintScreen			"printScreen"
#define OleTgeneral			(OLconst char *)"general"
#define OleTsave			"save"
#define OleTsaveFile			"saveFile"
#define OleTopenFile			"openFile"
#define OleTprintFile			"printFile"
#define OleTblankValue			"blankValue"
#define OleTnotPositive			"notPositive"

/*
 *************************************************************************
 * Define the extern-ed default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */

extern String OleMerrorMsg_cantAllocateColors;
extern String OleMerrorMsg_cantCallocBuffer;
extern String OleMerrorMsg_cantCallocName;
extern String OleMerrorMsg_cantFindVisual;
extern String OleMerrorMsg_cantReadHeader;
extern String OleMerrorMsg_cantReadName;
extern String OleMerrorMsg_cantReadMap;
extern String OleMerrorMsg_cantReadPixmap;
extern String OleMerrorMsg_cantQueryMap;
extern String OleMerrorMsg_exiting;
extern String OleMerrorMsg_headerTooSmall;
extern String OleMerrorMsg_newColorsNeeded;
extern String OleMerrorMsg_oldColorsMatch;
extern String OleMerrorMsg_formatMissMatch;
extern String OleMerrorMsg_noMemory;
extern String OleMerrorMsg_noWinAttrs;
extern String OleMerrorMsg_tooManyPlanes;

extern String OleMfooterMsg_tooManyFiles;
extern String OleMfooterMsg_areaSpooled;
extern String OleMfooterMsg_buffer;
extern String OleMfooterMsg_contentsSpooled;
extern String OleMfooterMsg_colorsTest;
extern String OleMfooterMsg_colors;
extern String OleMfooterMsg_doneCreate;
extern String OleMfooterMsg_errorCreatingFile;
extern String OleMfooterMsg_errorOpeningFile;
extern String OleMfooterMsg_fileLoaded;
extern String OleMfooterMsg_fileNotOrdinary;
extern String OleMfooterMsg_fileSpooled;
extern String OleMfooterMsg_fileUnaccessible;
extern String OleMfooterMsg_fileWritten;
extern String OleMfooterMsg_isAdirectory;
extern String OleMfooterMsg_loading;
extern String OleMfooterMsg_noContents;
extern String OleMfooterMsg_noContentsPrint;
extern String OleMfooterMsg_noContentsSave;
extern String OleMfooterMsg_nonexistentFile;
extern String OleMfooterMsg_noPathAccess;
extern String OleMfooterMsg_notDumpFile;
extern String OleMfooterMsg_overwrite;
extern String OleMfooterMsg_pathInvalid;
extern String OleMfooterMsg_pathUnaccessible;
extern String OleMfooterMsg_prepareContents;
extern String OleMfooterMsg_prepareFile;
extern String OleMfooterMsg_printFailed;
extern String OleMfooterMsg_saveCancelled;
extern String OleMfooterMsg_openCancelled;
extern String OleMfooterMsg_operCancelled;
extern String OleMfooterMsg_printCancelled;
extern String OleMfooterMsg_saveContents;
extern String OleMfooterMsg_screenSpooled;
extern String OleMfooterMsg_windowSpooled;
extern String OleMfooterMsg_winNameTest;
extern String OleMfooterMsg_winName;
extern String OleMfileMain_badShmimage;
extern String OleMfileMain_badShmget;
extern String OleMfileMain_badShmat;
extern String OleMfileMain_fileTitle;
extern String OleMfileMain_printScreen;
extern String OleMfileMain_general;
extern String OleMfileMain_save;
extern String OleMprintScreen_saveFile;
extern String OleMprintScreen_openFile;
extern String OleMprintScreen_printFile;
extern String OleMbadFilename_blankDefault;
extern String OleMbadFormat_blankValue;
extern String OleMbadPages_notPositive;
extern String OleMbadScale_notPositive;
extern String OleMbadLeftOffset_notPositive;
extern String OleMbadTopOffset_notPositive;
extern String OleMbadFieldWidth_notPositive;
extern String OleMbadFieldHeight_notPositive;
extern String OleMbadCommand_blankValue;

extern String OleMbutton_labelCapture;
extern String OleMbutton_labelCaptureImage;
extern String OleMbutton_labelFile;
extern String OleMbutton_labelOff;
extern String OleMbutton_labelOn;
extern String OleMbutton_labelOpen;
extern String OleMbutton_labelPrint;
extern String OleMbutton_labelSave;
extern String OleMbutton_labelCancel;
extern String OleMbutton_labelLandscape;
extern String OleMbutton_labelPortrait;
extern String OleMbutton_labelZpixmap;
extern String OleMbutton_labelXYpixmap;

extern String OleMbutton_overwrite; /*"Overwrite" */
extern String OleMbutton_saveCancelled; /* Cancel */
extern String OleMbutton_labelOpen2; /* Open... */
extern String OleMbutton_labelSave2; /* Save As... */
extern String OleMbutton_labelContents; /* Contents */
extern String OleMbutton_labelArea; /* Area */
extern String OleMbutton_labelWindow; /* Window */
extern String OleMbutton_labelScreen; /* Screen */
extern String OleMbutton_labelImageFile; /* Image File... */
extern String OleMbutton_labelOptions; /* Options... */
extern String OleMbutton_labelProperties; /* Properties... */

extern String OleMcaption_labelDefaultPath; /* "Default Pathname:" */
extern String OleMcaption_labelOutputFormat; /* "Output Format: */
extern String OleMcaption_labelPrintCmd; /* "Print Command:" */
extern String OleMcaption_labelOrientation; /* "Orientation:" */
extern String OleMcaption_labelPixformat; /* "Pixmap Format:" */
extern String OleMcaption_labelReverseVideo; /* "Reverse Video:" */
extern String OleMcaption_labelHeader; /* "Header:" */
extern String OleMcaption_labelFooter; /* "Footer: */
extern String OleMcaption_labelPages; /* "Pages:" */
extern String OleMcaption_labelScale; /* "Scale" */
extern String OleMcaption_labelLeftOffset; /* "Left Offset:" */
extern String OleMcaption_labelTopOffset; /* "Top Offset:" */
extern String OleMcaption_labelMaxHt; /* "Max height:" */
extern String OleMcaption_labelMaxWidth; /* "Max width:" */


extern String OleMmnemonic_overwrite;
extern String OleMmnemonic_saveCancelled;

extern String OleMmnemonic_labelOpen2;
extern String OleMmnemonic_labelSave2;

extern String OleMmnemonic_labelSave;
extern String OleMmnemonic_labelCancel;
extern String OleMmnemonic_labelOpen;

extern String OleMmnemonic_labelContents;
extern String OleMmnemonic_labelArea;
extern String OleMmnemonic_labelWindow;
extern String OleMmnemonic_labelScreen;
extern String OleMmnemonic_labelImageFile;

extern String OleMmnemonic_labelFile;
extern String OleMmnemonic_labelPrint;
extern String OleMmnemonic_labelCaptureImage;
extern String OleMmnemonic_labelOptions;
extern String OleMmnemonic_labelProperties;

extern String OleMmnemonic_labelDefaultPath;
extern String OleMmnemonic_labelOutputFormat;
extern String OleMmnemonic_labelPrintCmd;
extern String OleMmnemonic_labelOrientation;
extern String OleMmnemonic_labelPixformat;
extern String OleMmnemonic_labelReverseVideo;
extern String OleMmnemonic_labelHeader;
extern String OleMmnemonic_labelFooter;
extern String OleMmnemonic_labelPages;
extern String OleMmnemonic_labelScale;
extern String OleMmnemonic_labelLeftOffset;
extern String OleMmnemonic_labelTopOffset;
extern String OleMmnemonic_labelMaxHt;
extern String OleMmnemonic_labelMaxWidth;

extern String OleMtitle_labelProperties; /* Print Screen: Properties */
extern String OleMtitle_labelOpen; /* Print Screen: Open File */
extern String OleMtitle_labelSave; /* Print Screen: Save File */
extern String OleMtitle_labelPrint; /* Print Screen: Print File */
#endif /* __olps_error_h__ */
