/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:errors.h	1.9"
#endif
#ifndef __Ol_Olam_Error_h__
#define __Ol_Olam_Error_h__

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
 *			#define OleCOlClientOlamMsgs	"olam_msgs"
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

#define OleCOlClientOlamMsgs	"olam_msgs"

/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************
 */
#define OleNbutton				"btn"
#define OleNcaption				"cptn"

#define OleNfilevalidate		        "filevalidate"
#define OleNfilerhost		                "filerhost"
#define OleNfilepfsl		                "filepfsl"
#define OleNfilefooter		                "filefooter"
#define OleNfileolam		                "fileolam"
#define OleNfilerdisp		                "filerdisp"
#define OleNfilefile_stuff		        "filefile_stuff"
#define OleNfilecommon		                "filecommon"
#define OleNfilecreate		                "filecreate"

#define OleNfilelist_del		        "filelist_del"
#define OleNfilelist_get		        "filelist_get"
#define OleNfilelist_head		        "filelist_head"
#define OleNfilelist_ins		        "filelist_ins"
#define OleNfilelist_new		        "filelist_new"
#define OleNfilelist_prev		        "filelist_prev"
#define OleNfilelist_tail		        "filelist_tail"

#define OleNmnemonic				"mnem"

#define OleNtextField				"tf"
#define OleNtitle				"title"
/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleTmsg1  			(OLconst char *)"msg1"
#define OleTmsg2  			(OLconst char *)"msg2"
#define OleTmsg3  			(OLconst char *)"msg3"
#define OleTmsg4  			(OLconst char *)"msg4"
#define OleTmsg5  			(OLconst char *)"msg5"
#define OleTmsg6  			(OLconst char *)"msg6"
#define OleTmsg7  			(OLconst char *)"msg7"
#define OleTmsg8  			(OLconst char *)"msg8"
#define OleTmsg9  			(OLconst char *)"msg9"
#define OleTmsg10  			(OLconst char *)"msg10"
#define OleTmsg11  			(OLconst char *)"msg11"
#define OleTmsg12  			(OLconst char *)"msg12"
#define OleTmsg13  			(OLconst char *)"msg13"
#define OleTmsg14  			(OLconst char *)"msg14"
#define OleTmsg15  			"msg15"
#define OleTmsg16  			"msg16"
#define OleTmsg17  			"msg17"
#define OleTmsg18  			"msg18"
#define OleTmsg19  			"msg19"
#define OleTmsg20  			"msg20"
#define OleTmsg21  			"msg21"
#define OleTmsg22  			"msg22"

#define OleTappName  			"appName"
#define OleTusage			"usage"

/* Titles */
/* Outgoing remote displays - popup window */
#define OleTpopupOut			"popout"

/* Administration accepted remote hosts */
#define OleTpopupIn			"popin"
/* Help window */
#define OleTdispHelp			"dispH"
#define OleThostHelp			"hostH"
/* Insert menu button */
#define OleTinsert			"insert"

/* text field labels */
#define OleTdisplay			"disp"
#define OleThost			"host"
#define OleTnetspec			"netspec"

/* button labels */
#define OleTbefore			"bef"
#define OleTafter			"aft"
#define OleTdel				"del"
#define OleTapplyEdit			"appledt"
#define OleTyes				"yes"
#define OleTno				"no"
/*
 *************************************************************************
 * Define the default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */
extern String OleMbutton_applyEdit;
extern String OleMbutton_after;
extern String OleMbutton_before;
extern String OleMbutton_del;
extern String OleMbutton_yes;
extern String OleMbutton_no;

extern String OleMcaption_host;
extern String OleMcaption_display;

extern String OleMfilevalidate_msg1 ;
extern String OleMfilevalidate_msg2 ;
extern String OleMfilevalidate_msg3 ;
extern String OleMfilevalidate_msg4 ;
extern String OleMfilevalidate_msg5 ;

extern String OleMfilerhost_msg1 ;
extern String OleMfilerhost_msg2 ;
extern String OleMfilerhost_msg3 ;

extern String OleMfilerdisp_msg1 ;
extern String OleMfilerdisp_msg2 ;
extern String OleMfilerdisp_msg3 ;
extern String OleMfilerdisp_msg3 ;
extern String OleMfilerdisp_msg4 ;
extern String OleMfilerdisp_msg5 ;
extern String OleMfilerdisp_msg6 ;
extern String OleMfilerdisp_msg7 ;

extern String OleMfilepfsl_msg1 ;
extern String OleMfilepfsl_msg2 ;

extern String OleMfileolam_msg1 ;
extern String OleMfileolam_msg2 ;

extern String OleMfilefooter_msg1 ;

extern String OleMfilefile_stuff_msg1 ;
extern String OleMfilefile_stuff_msg2 ;
extern String OleMfilefile_stuff_msg3 ;

extern String OleMfilecommon_msg1 ;
extern String OleMfilecommon_msg2 ;
extern String OleMfilecommon_msg3 ;
extern String OleMfilecommon_msg4 ;
extern String OleMfilecommon_msg5 ;
extern String OleMfilecommon_msg6 ;
extern String OleMfilecommon_msg7 ;
extern String OleMfilecommon_msg8 ;
extern String OleMfilecommon_msg9 ;
extern String OleMfilecommon_msg10 ;
extern String OleMfilecommon_msg11 ;
extern String OleMfilecommon_msg12 ;
extern String OleMfilecommon_msg13 ;
extern String OleMfilecommon_msg14 ;

extern String OleMfilelist_del_msg1 ;
extern String OleMfilelist_del_msg2 ;
extern String OleMfilelist_del_msg3 ;

extern String OleMfilelist_get_msg1 ;

extern String OleMfilelist_head_msg1 ;

extern String OleMfilelist_ins_msg1 ;

extern String OleMfilelist_new_msg1 ;
           
extern String OleMfilelist_next_msg1 ;

extern String OleMfilelist_prev_msg1 ;
extern String OleMfilelist_prev_msg2 ;
extern String OleMfilelist_prev_msg3 ;

extern String OleMfilelist_tail_msg1 ;

extern String OleMmnemonic_before;
extern String OleMmnemonic_after;
extern String OleMmnemonic_insert;
extern String OleMmnemonic_del;
extern String OleMmnemonic_applyEdit;

extern String OleMmnemonic_yes;
extern String OleMmnemonic_no;

extern String OleMtextField_display;
extern String OleMtextField_host;
extern String OleMtextField_netspec;

extern String OleMtitle_popupOut;
extern String OleMtitle_popupIn;
extern String OleMtitle_dispHelp;
extern String OleMtitle_hostHelp;
extern String OleMtitle_insert;

#endif /* __Ol_Olam_Error_h__ */
