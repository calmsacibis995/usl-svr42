/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xdm:dtlogin.h	1.17"

#ifndef	_DTLOGIN_H_
#define	_DTLOGIN_H_

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#define	MAX_LOGNAME	1024

/*
 * This file contains all the message strings for the Desktop UNIX
 * Graphical Login client dtlogin, used by xdm.
 * 
 */

#define	string_malloc	"dtlogin:1" FS "Operation failed from lack of memory"
#define	string_greet1	"dtlogin:2" FS "Welcome to"
#define	string_greet2	"dtlogin:3" FS "Please enter your Login ID and Password."
#define	string_login_fail	"dtlogin:4" FS "Login attempt failed."
#define	string_pass_expired	"dtlogin:5" FS "Your Password has expired.  Please choose a new one."
#define	string_acct		"dtlogin:6" FS "Your Account has expired.  Please contact your systems administrator."
#define	string_pass_time	"dtlogin:7" FS "Your Password has expired.  Please contact your systems administrator."
#define	string_nomatch		"dtlogin:8" FS "The Passwords supplied do not match.  Try again."
#define	string_nopass		"dtlogin:9" FS "You must supply a password.  Try again."
#define	string_help		"dtlogin:10" FS "Enter your Login id in the Login id field.\nEnter your Password in the Password field.\nThe Password will not be echoed.\n\nThe Login button will log you into the system if you have typed your\nLogin id and Password.\n\nThe Reset button will remove any text that you have typed from\nthe Login id and Password fields.\n\nThe Exit button will return you to the Console Login prompt.\n\nThe Help button will display this message."

#define	label_log		"dtlogin:11" FS "Login"
#define	label_reset		"dtlogin:12" FS "Reset"
#define	label_exit		"dtlogin:13" FS "Exit"
#define	label_help		"dtlogin:14" FS "Help"

#define	label_login		"dtlogin:15" FS "Login ID:"
#define label_password		"dtlogin:16" FS "Password:"
#define	label_authPass		"dtlogin:17" FS "Enter New Password:"
#define	label_reauthPass	"dtlogin:18" FS "Enter It Again:"
#define	label_ok		"dtlogin:19" FS "OK"

#define	mnemonic_log		"dtlogin:20" FS "L"
#define	mnemonic_reset		"dtlogin:21" FS "R"
#define	mnemonic_exit		"dtlogin:22" FS "E"
#define	mnemonic_help		"dtlogin:23" FS "H"
#define mnemonic_apply		"dtlogin:24" FS "A"
#define	mnemonic_ok		"dtlogin:25" FS "O"

#define	error_passwd		"dtlogin:26" FS "ERROR: Could not store PASSWORD.  Login through Console"

#define	string_passwd_needed	"dtlogin:27" FS "Please enter a Password for your Account."

#define	string_maxtrys		"dtlogin:28" FS "Too many attempts.  Could not change Password."

#define	mnemonic_id		"dtlogin:29" FS "I"
#define	mnemonic_pass		"dtlogin:30" FS "P"

#define	string_noecho		"dtlogin:31" FS "Password entries will not be displayed as you type them."

#define	string_phelp1		"dtlogin:32" FS "You do not have a Password on this System and you need to have one.\nPlease enter your Password in the first Field.\nPlease retype the same Password in the second Field.\n\nThe text you type will not be displayed back to you."

#define	string_phelp2		"dtlogin:33" FS "Your current Password has expired.\nPlease enter a new Password in the first Field.\nPlease retype the same Password in the second Field.\n\nThe text you type will not be displayed back to you."

#define	label_ok1		"dtlogin:34" FS "OK"
#define	mnemonic_ok1		"dtlogin:35" FS "OK"

#define	string_tshort1		"dtlogin:36" FS "Password is too short - must be at least "
#define	string_tshort2		"dtlogin:37" FS " characters"
#define	string_nocirc		"dtlogin:38" FS "Password cannot be circular shift of loginid"
#define	string_schar		"dtlogin:39" FS "Password must contain at least two alphabetic characters\nand at least one numeric or special character"
#define	string_d3fpos		"dtlogin:40" FS "Passwords must differ by at least 3 positions"

#define	string_copyright	"dtlogin:41" FS "Copyright (c) 1992 UNIX System Laboratories, Inc."

#define	string_passneed		"dtlogin:42" FS "Password Needed"
#define	string_expire		"dtlogin:43" FS "Password Expired"

#define	string_badshell		"dtlogin:44" FS "Your SHELL is not valid"
#define	string_nohome		"dtlogin:45" FS "Unable to change to home directory"

#define	string_pflagt		"dtlogin:46" FS "Requested Password Change"
#define	string_pflag		"dtlogin:47" FS "You have requested a Password change.\nPlease Enter a New Password."

/*
 *	Structure to hold widgets that must be passed and used
 *	in multiple routines/files.
 */
typedef struct
	{
	Widget	login_text;
	Widget	password_text;
	Widget	login_fail;
	Widget	swin;
	Widget	icon_box;
	} Login_info;

extern	Login_info	*CreateLoginArea ();

#endif	/* Don't add anything after this endif */
