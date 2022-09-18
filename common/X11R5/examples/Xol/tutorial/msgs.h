/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olexamples:tutorial/msgs.h	1.1"
#endif

#ifndef __ss_msgs_h__
#define __ss_msgs_h__

/*
 *************************************************************************
 *
 * Description:
 *		This file contains message strings for s_sampler.
 *
 *	When adding strings, the following conventions should be used:
 *
 *		1. Message classes begin with OleC, e.g.
 *			#define OleCOlSamplerMsgs	"ss_msgs"
 *
 *		2. Message names begin with OleN, e.g.,
 *			#define	OleNinvalidResource	"invalidResource"
 *
 *		3. Message types begin with OleT, e.g.,
 *			#define	OleTsetValues		"setValues"
 *
 *		4. Message message strings begin with OleM and is followed
 *		   by the name string, and underbar '_', and concatenated
 *		   with the message type, for the above message name and type.
 *
 *******************************file*header*******************************
 */

#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>

/*
 *************************************************************************
 * Define the message classes here:  Use prefix of 'OleC'
 *************************************************************************
 */

#define OleCOlSamplerMsgs		"ss_msgs"

/*
 *************************************************************************
 * Define the message names here:  Use prefix of 'OleN'
 *************************************************************************
 */

#define OleNfooterMessage		"footerMessage"
#define OleNtopLevel			"topLevel"
#define OleNscrollingList		"scrollingList"
#define OleNscrolledWindow		"scrolledWindow"
#define OleNhelp			"help"
#define OleNnotice			"notice"
#define OleNstatictext			"statictext"
#define OleNexclusives			"exclusives"
#define OleNstub			"stub"
#define OleNflatCheckbox		"flatcheckBox"

/*
 *************************************************************************
 * Define the message types here:  Use prefix of 'OleT'
 *************************************************************************
 */

#define OleTtitle			"title"
#define OleTmessage			"message"
#define OleTitemLabel			"itemLabel"
#define OleTlabel			"label"
#define OleTstring			"string"
#define OleTpointerIn			"pointerIn"
#define OleTpointerOut			"pointerOut"
#define OleToblongButtonCB		"oblongButtonCB"
#define OleToblongGadgetCB		"oblongGadgetCB"
#define OleTnonExclusivesCB		"nonExclusivesCB"
#define OleTpopupApplyCB		"popupApplyCB"
#define OleTpopupSetDefaultsCB		"popupSetDefaultsCB"
#define OleTpopupResetCB		"popupResetCB"
#define OleTpopupResetFactoryCB		"popupResetFactoryCB"
#define OleTflatSelectCB		"flatSelectCB"
#define OleTbuttonSelected		"buttonSelected"
#define OleTgoodBye			"goodBye"
#define OleTscrollbarMoved		"scrollbarMoved"
#define OleTscrolledWindowVSB		"scrolledWindowVSB"
#define OleTscrolledWindowHSB		"scrolledWindowHSB"
#define OleTscrollingListMsg		"scrollingListMsg"
#define OleTtextfieldMsg		"textfieldMsg"

/*
 *************************************************************************
 * Define the default messages here:  Use prefix of 'OleM'
 * followed by the message name, an underbar <_>, and the message type.
 *************************************************************************
 */

extern String OleMtopLevel_title;
extern String OleMscrollingList_itemLabel;
extern String OleMscrolledWindow_string;

extern String OleMfooterMessage_pointerIn;
extern String OleMfooterMessage_pointerOut;
extern String OleMfooterMessage_oblongButtonCB;
extern String OleMfooterMessage_oblongGadgetCB;
extern String OleMfooterMessage_nonExclusivesCB;
extern String OleMfooterMessage_popupApplyCB;
extern String OleMfooterMessage_popupSetDefaultsCB;
extern String OleMfooterMessage_popupResetCB;
extern String OleMfooterMessage_popupResetFactoryCB;
extern String OleMfooterMessage_flatSelectCB;
extern String OleMfooterMessage_buttonSelected;
extern String OleMfooterMessage_goodBye;
extern String OleMfooterMessage_scrollbarMoved;
extern String OleMfooterMessage_scrolledWindowVSB;
extern String OleMfooterMessage_scrolledWindowHSB;
extern String OleMfooterMessage_scrollingListMsg;
extern String OleMfooterMessage_textfieldMsg;

extern String OleMhelp_title;
extern String OleMnotice_message;
extern String OleMstatictext_message;
extern String OleMexclusives_label;
extern String OleMstub_label;
extern String OleMflatCheckbox_label;

#endif /* __ss_msgs_h__ */
