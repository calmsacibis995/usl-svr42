/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef __olwsm_error_h__
#define __olwsm_error_h__

#pragma ident	"@(#)dtm:olwsm/error.h	1.24"

/* Note: OpenLookP.h includes Error.h */

#include <Xol/OpenLookP.h>
#include "../dm_strings.h"

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
 *			#define OleCOlClientOlpixmapMsg	"olpixmap_msgs"
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

#if	defined(__STDC__)
# define concat(a,b) a ## b
# define concat4(a,b,c,d) a ## b ## c ## d
#else
# define concat(a,b) a/**/b
# define concat4(a,b,c,d) a/**/b/**/c/**/d
#endif

#define OLGM(y,x)	OlGetMessage(dpy, Labels[n++], BUFSIZ, \
				     concat(OleN,x), \
				     concat(OleT,y), \
				     OleCOlClientOlwsmMsgs, \
				     concat4(OleM,x,_,y), \
				     (XrmDatabase)NULL)

#define OLG(y,x)	(char *)Dm__gettxt(concat4(TXT_,x,_,y))

#define OleCOlClientOlwsmMsgs	"olwsm_msgs"

#ifdef USE_TOOLKIT_I18N
#define OLG(y,x)	(char *)OlGetMessage(dpy, NULL, BUFSIZ, \
				     concat(OleN,x), \
				     concat(OleT,y), \
				     OleCOlClientOlwsmMsgs, \
				     concat4(OleM,x,_,y), \
				     (XrmDatabase)NULL)


/*
 *************************************************************************
 * Define the error names here:  Use prefix of 'OleN'
 *************************************************************************
 */

#define OleNbadCommandLine		"badCommandLine"
#define OleNdupMsg			"dupMsg"
#define OleNerrorMsg			"errorMsg"
#define OleNfixedString			"fixedString"
#define OleNfooterMsg			"footerMsg"
#define OleNhelpTag			"helpTag"
#define OleNmaxLabel			"maxLabel"
#define OleNminLabel			"minLabel"
#define OleNmnemonic			"mnemonic"
#define OleNpageLabel			"pageLabel"
#define OleNwarningMsg			"warningMsg"

/*
 *************************************************************************
 * Define the error types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleT2D				"2D"
#define OleT3D				"3D"
#define OleTabort			"abort"
#define OleTaccel			"accel"
#define OleTaccepted			"accepted"
#define OleTadjust			"adjust"
#define OleTafter			"after"
#define OleTalternate			"alternate"
#define OleTalways			"always"
#define OleTapply			"apply"
#define OleTapplyall			"applyall"
#define OleTapplyEdits			"applyEdits"
#define OleTasAGroup			"asAGroup"
#define OleTbadAssert			"badAssert"
#define OleTbadKeyMatch			"badKeyMatch"
#define OleTbadModMatch			"badModMatch"
#define OleTbasicSet			"basicSet"
#define OleTbeep			"beep"
#define OleTbefore			"before"
#define OleTblackOnWhite		"blackOnWhite"
#define OleTblue			"blue"
#define OleTbottom			"bottom"
#define OleTborder			"border"
#define OleTcacheExceed			"cacheExceed"
#define OleTcalculator			"calculator"
#define OleTcannotWrite			"cannotWrite"
#define OleTchangeGUI			"changeGUI"
#define OleTchooseColor			"chooseColor"
#define OleTclickSelect			"clickSelect"
#define OleTclock			"clock"
#define OleTcolor			"color"
#define OleTcolorChoices		"colorChoices"  
#define OleTcolorCombo			"colorCombo"  
#define OleTcolorSample			"colorSample"  
#define OleTcolorStart			"colorStart"
#define OleTcolorError			"colorError"
#define OleTconstrain			"constrain"
#define OleTcontinue			"continue"
#define OleTcustom			"custom"
#define OleTdamping			"damping"
#define OleTdateTime			"dateTime"
#define OleTdeletePrime			"deletePrime"
#define OleTdesktop			"desktop"
#define OleTdispDefault			"dispDefault"
#define OleTdispLang			"dispLang"
#define OleTdispMenu			"dispMenu"
#define OleTdistinct			"distinct"
#define OleTdragRight			"dragRight"
#define OleTduplicate			"duplicate"
#define OleTexit			"exit"
#define	OleTfactory			"factory"
#define	OleTfile			"file"
#define OleTflatKeys			"flatKeys"
#define OleTfollow			"follow"
#define OleTfontGroup			"fontGroup"
#define OleTforkFailed			"forkFailed"
#define OleTfunction			"function"
#define OleTgray			"gray"
#define OleTgreen			"green"
#define OleThelp			"help"
#define OleThelpKeyClr			"helpKeyColor"
#define OleThelpModel			"helpModel"
#define OleTicons			"icons"
#define OleTindividually		"individually"
#define OleTinitKeyHelp			"initKeyHelp"
#define OleTinputArea			"inputArea"
#define OleTinputFocus			"inputFocus"
#define OleTinputLang			"inputLang"
#define OleTinputWindow			"inputWindow"
#define OleTinsert			"insert"
#define OleTinterface			"interface"
#define OleTdupModifier			"dupModifier"
#define OleTinvocation			"invocation"
#define OleTkbdProps			"kbdProps"
#define OleTlayering			"layering"
#define OleTleft			"left"
#define OleTlist			"list"
#define OleTlmr				"lmr"
#define OleTlocation			"location"
#define OleTlogin			"login"
#define OleTmenu			"menu"
#define OleTmenuLabels			"menuLabels"
#define OleTmenuMarkR			"menuMarkR"
#define OleTmiddle			"middle"
#define OleTmisc			"misc"
#define OleTmneFollow			"mneFollow"
#define OleTmnePreface			"mnePreface"
#define OleTmneSetting			"mneSetting"
#define OleTmneUnspec			"mneUnspec"
#define OleTmnemonic			"mnemonic"
#define OleTmodifier			"modifier"
#define OleTmouseAcc			"mouseAcc"
#define OleTmouseBtn			"mouseBtn"
#define OleTmouseEq			"mouseEq"
#define OleTmouseM			"mouseM"
#define OleTmouseMod			"mouseMod"
#define OleTmouseS			"mouseS"
#define OleTmouseSelect			"mouseSelect"
#define OleTmovePointer			"movePointer"
#define OleTmultiClick			"multiClick"
#define OleTneedOladduser		"needOladduser"
#define OleTname			"name"
#define OleTnever			"never"
#define OleTnextChoice			"nextChoice"
#define OleTno				"no"
#define OleTnoDefaults			"noDefaults"
#define OleTnoFile			"noFile"
#define OleTnoShow			"noShow"
#define OleTnoStepParent		"noStepParent"
#define OleTnoWidget			"noWidget"
#define OleTnone			"none"
#define OleTnotices			"notices"
#define OleTnowPress			"nowPress"
#define OleTnum				"num"
#define OleTnumFormat			"numFormat"
#define OleToff				"off"
#define OleTolam			"olam"
#define OleTolfm			"olfm"
#define OleTolps			"olps"
#define OleTonHighlight			"onHighlight"
#define OleTonNoShow			"onNoShow"
#define OleTonShow			"onShow"
#define OleTonUnderline			"onUnderline"
#define OleTotherControl		"otherControl"
#define OleToutgoing			"outgoing"
#define OleTpixeditor			"pixeditor"
#define OleTpointer			"pointer"
#define OleTpoorColors			"poorColors"
#define OleTpreface			"preface"
#define OleTprimary			"primary"
#define OleTprogMenu			"progMenu"
#define OleTprograms			"programs"
#define OleTproperties			"properties"
#define OleTpropsTitle			"propsT"
#define OleTpushpin			"pushpin"
#define OleTquoting			"quoting"
#define OleTred				"red"
#define OleTrefresh			"refresh"
#define OleTreset			"reset"
#define OleTright			"right"
#define OleTsameColors			"sameColors"
#define OleTscrollPan			"scrollPan"
#define OleTselect			"select"
#define OleTsetLocale			"setLocale"
#define OleTsetMenuDef			"setMenuDef"
#define OleTshow			"show"
#define OleTsettings			"settings"
#define OleTspecSetting			"specSetting"
#define OleTsuppSetting			"suppSetting"
#define OleTstepParentNotComposite	"stepParentNotComposite"
#define OleTterm			"term"
#define OleTtextBG			"textBG"
#define OleTtextFG			"textFG"
#define OleTtop				"top"
#define OleTtotal			"total"
#define OleTuseThisArea			"useThisArea"
#define OleTutils			"utils"
#define OleTvideo			"video"
#define OleTview			"view"
#define OleTwMenu			"wMenu"
#define OleTwProps			"wProps"
#define OleTwantExit			"wantExit"
#define OleTwhiteOnBlack		"whiteOnBlack"
#define OleTwindowBG			"windowBG"
#define OleTworkspace			"workspace"
#define OleTwsm				"wsm"
#define OleTyes				"yes"

/*
 *************************************************************************
 * Define the default error messages here:  Use prefix of 'OleM'
 * followed by the error name, an underbar <_>, and the error type.
 *************************************************************************
 */

extern String OleMdupMsg_follow ;
extern String OleMdupMsg_mneFollow ;
extern String OleMdupMsg_preface ;
extern String OleMdupMsg_mnePreface ;
extern String OleMdupMsg_total ;
extern String OleMdumMsg_mneUnspec ;

extern String OleMerrorMsg_badAssert ;
extern String OleMerrorMsg_distinct ;
extern String OleMerrorMsg_dupModifier ;
extern String OleMerrorMsg_needOladduser ;
extern String OleMerrorMsg_noWidget ;
extern String OleMerrorMsg_otherControl ;

extern String OleMfixedString_2D ;
extern String OleMfixedString_3D ;
extern String OleMfixedString_abort;
extern String OleMfixedString_accel ;
extern String OleMfixedString_accepted ;
extern String OleMfixedString_adjust ;
extern String OleMfixedString_after ;
extern String OleMfixedString_alternate ;
extern String OleMfixedString_always ;
extern String OleMfixedString_apply ;
extern String OleMfixedString_applyall ;
extern String OleMfixedString_applyEdits ;
extern String OleMfixedString_asAGroup ;
extern String OleMfixedString_basicSet ;
extern String OleMfixedString_beep ;
extern String OleMfixedString_before ;
extern String OleMfixedString_blackOnWhite ;
extern String OleMfixedString_blue ;
extern String OleMfixedString_border ;
extern String OleMfixedString_bottom ;
extern String OleMfixedString_calculator ;
extern String OleMfixedString_clickSelect ;
extern String OleMfixedString_clock ;
extern String OleMfixedString_color ;
extern String OleMfixedString_colorChoices ;
extern String OleMfixedString_colorCombo ;
extern String OleMfixedString_colorSample ;
extern String OleMfixedString_constrain ;
extern String OleMfixedString_continue ;
extern String OleMfixedString_copy ;
extern String OleMfixedString_custom ;
extern String OleMfixedString_cut ;
extern String OleMfixedString_damping ;
extern String OleMfixedString_dateTime ;
extern String OleMfixedString_delete ;
extern String OleMfixedString_desktop ;
extern String OleMfixedString_dispDefault ;
extern String OleMfixedString_dispLang ;
extern String OleMfixedString_dispMenu ;
extern String OleMfixedString_dragRight ;
extern String OleMfixedString_duplicate ;
extern String OleMfixedString_edit ;
extern String OleMfixedString_exit ;
extern String OleMfixedString_factory ;
extern String OleMfixedString_file ;
extern String OleMfixedString_fontGroup ;
extern String OleMfixedString_function ;
extern String OleMfixedString_gray ;
extern String OleMfixedString_green ;
extern String OleMfixedString_help ;
extern String OleMfixedString_helpKeyClr ;
extern String OleMfixedString_helpModel ;
extern String OleMfixedString_icons ;
extern String OleMfixedString_individually ;
extern String OleMfixedString_inputArea ;
extern String OleMfixedString_inputFocus ;
extern String OleMfixedString_inputLang ;
extern String OleMfixedString_inputMethod ;
extern String OleMfixedString_inputWindow ;
extern String OleMfixedString_insert ;
extern String OleMfixedString_interface ;
extern String OleMfixedString_invocation ;
extern String OleMfixedString_kbdProps ;
extern String OleMfixedString_layering ;
extern String OleMfixedString_left ;
extern String OleMfixedString_lmr ;
extern String OleMfixedString_location ;
extern String OleMfixedString_login ;
extern String OleMfixedString_menu ;
extern String OleMfixedString_menuLabels ;
extern String OleMfixedString_menuMarkR ;
extern String OleMfixedString_misc ;
extern String OleMfixedString_mneSetting ;
extern String OleMfixedString_mnemonic ;
extern String OleMfixedString_modifier ;
extern String OleMfixedString_mouseAcc ;
extern String OleMfixedString_mouseBtn ;
extern String OleMfixedString_mouseEq ;
extern String OleMfixedString_mouseM ;
extern String OleMfixedString_mouseMod ;
extern String OleMfixedString_mouseS ;
extern String OleMfixedString_mouseSelect ;
extern String OleMfixedString_movePointer ;
extern String OleMfixedString_multiClick ;
extern String OleMfixedString_name ;
extern String OleMfixedString_never ;
extern String OleMfixedString_nextChoice ;
extern String OleMfixedString_no ;
extern String OleMfixedString_noShow ;
extern String OleMfixedString_none ;
extern String OleMfixedString_notices ;
extern String OleMfixedString_numFormat ;
extern String OleMfixedString_off ;
extern String OleMfixedString_olam ;
extern String OleMfixedString_olfm ;
extern String OleMfixedString_olps ;
extern String OleMfixedString_onHighlight ;
extern String OleMfixedString_onNoShow ;
extern String OleMfixedString_onShow ;
extern String OleMfixedString_onUnderline ;
extern String OleMfixedString_outgoing ;
extern String OleMfixedString_paste ;
extern String OleMfixedString_pixeditor ;
extern String OleMfixedString_pointer ;
extern String OleMfixedString_primary ;
extern String OleMfixedString_progMenu ;
extern String OleMfixedString_programs ;
extern String OleMfixedString_properties ;
extern String OleMfixedString_propsTitle;
extern String OleMfixedString_red ;
extern String OleMfixedString_refresh ;
extern String OleMfixedString_reset ;
extern String OleMfixedString_right ;
extern String OleMfixedString_scrollPan ;
extern String OleMfixedString_setLocale ;
extern String OleMfixedString_setMenuDef ;
extern String OleMfixedString_select ;
extern String OleMfixedString_settings ;
extern String OleMfixedString_show ;
extern String OleMfixedString_specSetting ;
extern String OleMfixedString_suppSetting ;
extern String OleMfixedString_term ;
extern String OleMfixedString_textBG ;
extern String OleMfixedString_textFG ;
extern String OleMfixedString_top ;
extern String OleMfixedString_utils ;
extern String OleMfixedString_video ;
extern String OleMfixedString_view ;
extern String OleMfixedString_whiteOnBlack ;
extern String OleMfixedString_windowBG ;
extern String OleMfixedString_workspace ;
extern String OleMfixedString_wsm ;
extern String OleMfixedString_yes ;

extern String OleMfooterMsg_changeGUI ;
extern String OleMfooterMsg_chooseColor ;
extern String OleMfooterMsg_colorStart ;
extern String OleMfooterMsg_colorError ;
extern String OleMfooterMsg_deletePrime ;
extern String OleMfooterMsg_initKeyHelp ;
extern String OleMfooterMsg_nowPress ;
extern String OleMfooterMsg_quoting ;
extern String OleMfooterMsg_sameColors ;
extern String OleMfooterMsg_useThisArea ;
extern String OleMfooterMsg_wantExit ;

extern String OleMhelpTag_exit ;
extern String OleMhelpTag_progMenu ;
extern String OleMhelpTag_programs ;
extern String OleMhelpTag_properties ;
extern String OleMhelpTag_pushpin ;
extern String OleMhelpTag_refresh ;
extern String OleMhelpTag_utils ;
extern String OleMhelpTag_wMenu ;
extern String OleMhelpTag_wProps ;
extern String OleMhelpTag_workspace ;

extern String OleMinternal_cacheExceed ;

extern String OleMinvalidResource_flatKeys ;
extern String OleMinvalidResource_noStepParent ;
extern String OleMinvalidResource_stepParentNotComposite ;

extern String OleMmaxLabel_damping ;
extern String OleMmaxLabel_dragRight ;
extern String OleMmaxLabel_menuMarkR ;
extern String OleMmaxLabel_mouseAcc ;
extern String OleMmaxLabel_multiClick ;
  
extern String OleMminLabel_damping ;
extern String OleMminLabel_dragRight ;
extern String OleMminLabel_menuMarkR ;
extern String OleMminLabel_mouseAcc ;
extern String OleMminLabel_multiClick ;

extern String OleMmnemonic_abort ;
extern String OleMmnemonic_accepted ;
extern String OleMmnemonic_after ;
extern String OleMmnemonic_apply ;
extern String OleMmnemonic_applyEdits ;
extern String OleMmnemonic_before ;
extern String OleMmnemonic_color ;
extern String OleMmnemonic_continue ;
extern String OleMmnemonic_delete ;
extern String OleMmnemonic_desktop ;
extern String OleMmnemonic_exit ;
extern String OleMmnemonic_factory ;
extern String OleMmnemonic_icons ;
extern String OleMmnemonic_insert ;
extern String OleMmnemonic_kbdProps ;
extern String OleMmnemonic_left ;
extern String OleMmnemonic_middle ;
extern String OleMmnemonic_misc ;
extern String OleMmnemonic_mouseM ;
extern String OleMmnemonic_mouseS ;
extern String OleMmnemonic_no ;
extern String OleMmnemonic_olam ;
extern String OleMmnemonic_olfm ;
extern String OleMmnemonic_olps ;
extern String OleMmnemonic_outgoing ;
extern String OleMmnemonic_progMenu ;
extern String OleMmnemonic_programs ;
extern String OleMmnemonic_properties ;
extern String OleMmnemonic_refresh ;
extern String OleMmnemonic_reset ;
extern String OleMmnemonic_right ;
extern String OleMmnemonic_setLocale ;
extern String OleMmnemonic_utils ;
extern String OleMmnemonic_yes ;

extern String OleMpageLabel_color ;
extern String OleMpageLabel_desktop ;
extern String OleMpageLabel_icons ;
extern String OleMpageLabel_misc ;
extern String OleMpageLabel_mouseM ;
extern String OleMpageLabel_progMenu ;
extern String OleMpageLabel_setLocale ;
extern String OleMpageLabel_settings ;

extern String OleMwarningMsg_badKeyMatch ;
extern String OleMwarningMsg_badModMatch ;
extern String OleMwarningMsg_cannotWrite ;
extern String OleMwarningMsg_forkFailed ;
extern String OleMwarningMsg_noDefaults ;
extern String OleMwarningMsg_noFile ;
extern String OleMwarningMsg_poorColors ;
#endif /* USE_TOOLKIT_I18N */

#endif /* __olwsm_error_h__ */  
