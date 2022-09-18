/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:dashboard/dashmsgs.h	1.8"
#endif
/*
 * This file contains all the message strings for the Desktop UNIX
 * dtadmin client LoginMgr.
 * 
 */
#define	string_appName		"dtdash:01" FS "System Status"
#define	string_cantReset	"dtdash:02" FS "unable to reset date/time."
#define	string_cantRead		"dtdash:03" FS "unable to read file: "
#define	string_cantWrite	"dtdash:04" FS "unable to write new "
#define	string_cantBuild	"dtdash:05" FS \
			 "unable to display status entry.  Not enough Memory."
#define	string_cantGauge	"dtdash:06" FS "unable to determine disk usage"
#define	string_cantUpdate	"dtdash:07" FS "unable to reset status component"
#define	string_badFile		"dtdash:08" FS "file appears corrupted: "
#define	string_badTick		"dtdash:09" FS "update time must be blank or digits"
#define	string_noInfo		"dtdash:10" FS "no information available about "
#define	string_propOK		"dtdash:11" FS "Status properties applied"
#define	string_dfltOK		"dtdash:12" FS "Status defaults updated in "
#define	string_resetOK		"dtdash:13" FS "Status properties reset"
#define	string_iconName		"dtdash:14" FS "System Status"
#define string_propTitle	"dtdash:15" FS "System Status: Properties"
#define string_OSfmt		"dtdash:16" FS "UNIX System V %s %s / %s"

#define	label_year		"dtdash:20" FS "year"
#define	label_month		"dtdash:21" FS "month"
#define	label_day		"dtdash:22" FS "day"
#define	label_hour		"dtdash:23" FS "hour"
#define	label_minute		"dtdash:24" FS "minute"
#define	label_sec		"dtdash:25" FS "second"
#define	label_second		"dtdash:26" FS "second(s)"
#define	label_type		"dtdash:27" FS "clock type: "
#define	label_tick		"dtdash:28" FS "update clock every "
#define	label_gauge		"dtdash:29" FS "check disks every "

#define	label_prop		"dtdash:30" FS "Properties..."
#define	label_help		"dtdash:31" FS "Help"
#define	label_action		"dtdash:32" FS "Actions"
#define	label_exit		"dtdash:33" FS "Exit"
#define	label_apply		"dtdash:34" FS "Apply"
#define	label_reset		"dtdash:35" FS "Reset"
#define	label_default		"dtdash:36" FS "Set Defaults"
#define	label_cancel		"dtdash:37" FS "Cancel"

#define	mnemonic_prop		"dtdash:40" FS "P"
#define	mnemonic_help		"dtdash:41" FS "H"
#define	mnemonic_action		"dtdash:42" FS "A"
#define	mnemonic_exit		"dtdash:43" FS "E"
#define	mnemonic_apply		"dtdash:44" FS "A"
#define	mnemonic_reset		"dtdash:45" FS "R"
#define	mnemonic_default	"dtdash:46" FS "S"
#define	mnemonic_cancel		"dtdash:47" FS "C"

#define	tag_mbyte		"dtdash:50" FS "Mbytes"
#define	tag_login		"dtdash:51" FS "Login:"
#define	tag_netnode		"dtdash:52" FS "Network node:"

#define	label_intro		"dtdash:60" FS "System Status..."
#define	label_toc		"dtdash:61" FS "Table of Contents..."
#define	label_hlpdsk		"dtdash:62" FS "Help Desk..."

#define	mnemonic_intro		"dtdash:65" FS "S"
#define	mnemonic_toc		"dtdash:66" FS "T"
#define	mnemonic_hlpdsk		"dtdash:67" FS "K"

#define	help_intro		"dtdash:70" FS "10"
#define	help_props		"dtdash:71" FS "40"
