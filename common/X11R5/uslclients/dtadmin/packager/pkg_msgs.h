/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:packager/pkg_msgs.h	1.18"
#endif
/*
 * This file contains all the message strings for the Desktop UNIX
 * dtadmin client PackageMgr.
 * 
 */
#define	string_appName		"dtpkg:1" FS "Application Setup"
#define	string_badLog		"dtpkg:2" FS \
				 "Unable to determine application contents"
#define	string_noSelect		"dtpkg:3" FS "No set or application is selected"
#define	string_syspkgs		"dtpkg:4" FS \
			"%d %s Applications/Sets currently installed on %s"
#define	string_spoolpkgs	"dtpkg:5" FS \
			"%d Applications/Sets available for installation"
#define	string_mailMsg		"dtpkg:6" FS \
"The following software has been installed on the system;\n\
use Application_Setup to find icons for your Desktop.\n\n"
#define	string_noIcon		"dtpkg:7" FS "No icons selected to install"
#define	string_spoolPrompt	"dtpkg:8" FS "View Folder"
#define	string_promptMsg	"dtpkg:9" FS \
	"Choose a folder to examine for applications"

#define	tag_addOp		"dtpkg:10" FS "Software installation"
#define	tag_instOp		"dtpkg:11" FS "Creation of desktop icon"
#define	tag_delOp		"dtpkg:12" FS "Deletion"
#define	tag_good		"dtpkg:13" FS "succeeded"
#define	tag_bad			"dtpkg:14" FS "failed"
#define	folder_apps		"dtpkg:15" FS "Applications"

#define	format_opFmt		"dtpkg:20" FS "%s of %s %s"
#define	format_insFmt		"dtpkg:21" FS "Insert %s in %s and click %s"
#define	format_pkgcnt		"dtpkg:22" FS "%d %s %s"
#define	format_install		"dtpkg:23" FS "%s installed in %s Folder"
#define	format_iconcnt		"dtpkg:24" FS "%d icons installed in %s Folder"
#define	format_notPkg		"dtpkg:25" FS \
			"This %s is not in package format.\n\n"
#define	format_wait		"dtpkg:26" FS \
			"Cataloging applications on %s;\nplease wait."
#define	format_cantRead		"dtpkg:27" FS \
			"Unable to catalog applications on %s."
#define	format_numPkgs		"dtpkg:28" FS "%d %s packages in set %s."
#define	format_numSets		"dtpkg:29" FS " in %d sets"

#define	label_action		"dtpkg:30" FS "Actions"
#define	label_file		"dtpkg:31" FS "File"
#define	label_view		"dtpkg:32" FS "View"
#define	label_edit		"dtpkg:33" FS "Application"
#define	label_help		"dtpkg:34" FS "Help"
#define	label_add		"dtpkg:35" FS "Install"
#define	label_delete		"dtpkg:36" FS "Remove..."
#define	label_install		"dtpkg:37" FS "Install to Desktop"
#define	label_info		"dtpkg:38" FS "Properties..."
#define	label_icons		"dtpkg:39" FS "Show Contents..."
#define	label_cancel		"dtpkg:40" FS "Cancel"
#define	label_go		"dtpkg:41" FS "Continue"
#define	label_select		"dtpkg:42" FS "Select"
#define	label_exit		"dtpkg:43" FS "Exit"
#define	label_apps		"dtpkg:44" FS "Add-On"
#define	label_system		"dtpkg:45" FS "System"
#define	label_all		"dtpkg:46" FS "All"
#define	label_spooled		"dtpkg:47" FS "Other..."
#define	label_medium		"dtpkg:48" FS "medium"

#define	mnemonic_action		"dtpkg:50" FS "A"
#define	mnemonic_file		"dtpkg:51" FS "F"
#define	mnemonic_edit		"dtpkg:52" FS "p"
#define	mnemonic_view		"dtpkg:53" FS "V"
#define	mnemonic_help		"dtpkg:54" FS "H"
#define	mnemonic_add		"dtpkg:55" FS "I"
#define	mnemonic_delete		"dtpkg:56" FS "R"
#define	mnemonic_install	"dtpkg:57" FS "I"
#define	mnemonic_info		"dtpkg:58" FS "P"
#define	mnemonic_icons		"dtpkg:59" FS "S"
#define	mnemonic_cancel		"dtpkg:60" FS "C"
#define	mnemonic_go		"dtpkg:61" FS "o"
#define	mnemonic_select		"dtpkg:62" FS "S"
#define	mnemonic_exit		"dtpkg:63" FS "X"
#define	mnemonic_apps		"dtpkg:64" FS "O"
#define	mnemonic_system		"dtpkg:65" FS "S"
#define	mnemonic_all		"dtpkg:66" FS "A"
#define	mnemonic_spooled	"dtpkg:67" FS "O"

#define	string_invokeCustom	"dtpkg:70" FS "Invoking custom utility ..."
#define	string_invokePkgOp	"dtpkg:71" FS "Invoking package utility ..."
#define	string_addTitle		"dtpkg:72" FS "Add Application: "
#define	string_remTitle		"dtpkg:73" FS "Delete Application: "
#define	string_cusTitle		"dtpkg:74" FS "Custom Installation"
#define	string_iconName		"dtpkg:75" FS "Appl-n_Setup"
#define	string_iconTitle	"dtpkg:76" FS "Application Setup: Icons"
#define	string_infoTitle	"dtpkg:77" FS "Application Setup: Properties"
#define	string_badCustom	"dtpkg:78" FS "Unable to invoke custom utility."
#define	string_badPkgOp		"dtpkg:79" FS "Unable to invoke package utility."
#define	string_badInstPkg	"dtpkg:80" FS "Unable to invoke installpkg."
#define	string_badRemPkg	"dtpkg:81" FS "Unable to invoke removepkg."
#define	string_invokeInstPkg	"dtpkg:82" FS "Invoking  installpkg ..."
#define	string_invokeRemPkg	"dtpkg:83" FS "Invoking  removepkg ..."
#define	string_svr3Title	"dtpkg:84" FS \
			"Application Setup: SVR3.2 Package Installation"
#define	string_rmPkgTitle	"dtpkg:85" FS \
			"Application Setup: SVR3.2 Package Removal"
#define	string_mediaTitle	"dtpkg:86" FS "Application Setup: Package Media"
#define	string_msgTitle		"dtpkg:87" FS "Application Setup: Cataloging"
#define	string_pkgTitle		"dtpkg:88" FS "Application Setup: %s"

#define	info_name		"dtpkg:90" FS "Application Name:"
#define	info_desc		"dtpkg:91" FS "Description:"
#define	info_cat		"dtpkg:92" FS "Category:"
#define	info_vendor		"dtpkg:93" FS "Vendor:"
#define	info_version		"dtpkg:94" FS "Version:"
#define	info_arch		"dtpkg:95" FS "Architecture:"
#define	info_date		"dtpkg:96" FS "Date Installed"
#define	info_size		"dtpkg:97" FS "Size (blocks):"
#define	info_icons		"dtpkg:98" FS "Installable Icons:"
#define	info_execs		"dtpkg:99" FS \
				"No application icons. Executable Programs:"

#define	label_intro		"dtpkg:100" FS "Appl'n Setup..."
#define	label_toc		"dtpkg:101" FS "Table of Contents..."
#define	label_hlpdsk		"dtpkg:102" FS "Help Desk..."
#define	mnemonic_intro		"dtpkg:103" FS "A"
#define	mnemonic_toc		"dtpkg:104" FS "T"
#define	mnemonic_hlpdsk		"dtpkg:105" FS "K"

#define	help_intro		"dtpkg:106" FS "10"
#define	help_props		"dtpkg:107" FS "260"
#define	help_icons		"dtpkg:108" FS "220"
#define	help_uninstalled	"dtpkg:109" FS "120"
#define	help_folder		"dtpkg:110" FS "290"
#define	help_pkgwin		"dtpkg:111" FS "160"

#define	string_instTitle	"dtpkg:120" FS \
					"Application Setup: Installed - %s"
#define	string_uninstTitle	"dtpkg:121" FS \
					"Application Setup: Uninstalled - %s"
#define	label_inst		"dtpkg:122" FS "Installed Appl'ns"
#define	label_uninst		"dtpkg:123" FS "Uninstalled Appl'ns"
#define	mnemonic_inst		"dtpkg:124" FS "I"
#define	mnemonic_uninst		"dtpkg:125" FS "U"
#define	string_badClass		"dtpkg:126" FS \
					"Invalid icon class definition in %s."
#define	string_badHelp		"dtpkg:127" FS \
					"Invalid help file definition in %s."
#define	string_regClass		"dtpkg:128" FS \
		"Icon class definitions updated in Desktop Manager."
#define	string_regFailed	"dtpkg:129" FS \
		"Desktop Manager icon class definition update failed."
