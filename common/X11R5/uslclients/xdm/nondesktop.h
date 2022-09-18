/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:nondesktop.h	1.4"

#ifndef	_NONDESKTOP_H_
#define	_NONDESKTOP_H_

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/*
 * This file contains all the message strings for the Desktop UNIX
 * Graphical Login client nondesktop, used by xdm.
 * 
 */

#define	string_dhelp1		"nondesktop:1" FS "Press the Exit button to exit from Graphics\n\nPress the Cancel button to return to Graphical Login."
#define	string_dhelp2		"nondesktop:2" FS "Press the Desktop button to begin a Desktop session.\n\nPress the Exit button to exit from Graphics.\n\nPress the Cancel button to return to Graphical Login."

#define	label_dexit		"nondesktop:3" FS "Exit"
#define	label_dlogin		"nondesktop:4" FS "Cancel"
#define	label_dtuser		"nondesktop:5" FS "Start Desktop"
#define	label_xterm		"nondesktop:6" FS "Display Terminal"
#define	label_dhelp		"nondesktop:7" FS "Help"

#define	mnemonic_dexit		"nondesktop:8" FS "E"
#define	mnemonic_dlogin		"nondesktop:9" FS "C"
#define	mnemonic_dtuser		"nondesktop:10" FS "S"
#define	mnemonic_xterm		"nondesktop:11" FS "D"
#define mnemonic_dhelp		"nondesktop:12" FS "H"
#define	mnemonic_ok		"nondesktop:13" FS "O"

#define	string_nondesktop	"nondesktop:14" FS "You are not a Desktop user.  Do you want to..."

#define	string_nondesktop1	"nondesktop:15" FS "Your Desktop Preference indicates that your Desktop should\nnot be started.  You may: 1) Start your Desktop anyway or\n2) Click 'Exit' and enter the system through the Console Login."

#define	label_ok		"nondesktop:16" FS "OK"


#endif	/* Don't add anything after this endif */
