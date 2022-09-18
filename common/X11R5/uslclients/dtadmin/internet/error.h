/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/error.h	1.21"
#endif

#ifndef __Ol_Inet_Error_h__
#define __Ol_Inet_Error_h__

#include <Gizmos.h>

#define	HELP_FILE		"dtadmin/inet.hlp"

#define GGT GetGizmoText
#define SET_HELP(id)		(id.title = GGT(id.title),\
				id.section = id.section?GGT(id.section):0)
#define SET_LABEL(id,n,name)    id[n].label = (XtArgVal)GGT(label##_##name);\
                                id[n].mnemonic = (XtArgVal)*GGT(mnemonic##_##name);

#define	format_usage		"InternetMgr:001" FS "Usage: %s [filename]"

#define	string_appName		"InternetMgr:101" FS "Internet Setup"
#define	string_badName		"InternetMgr:102" FS "Invalid System Name!"
#define	string_badAddr		"InternetMgr:103" FS "Invalid network address!"
#define	string_cp1Fail		"InternetMgr:104" FS "Could not copy remote Hosts table to local system."
#define	string_cp2Fail		"InternetMgr:105" FS "Could not access remote Hosts table."
#define	string_cpOK		"InternetMgr:106" FS "Get Remote Systems - completed."
#define	string_blankName	"InternetMgr:107" FS "Must enter a System Name."
#define	string_noLogin		"InternetMgr:108" FS "Cannot determine who you are!"
#define	string_sameNode		"InternetMgr:112" FS "Can not install an icon for your own system!"
#define	string_blankAddr	"InternetMgr:113" FS "Part of the Network Address is blank."
#define	string_fopenWrite	"InternetMgr:114" FS "Failed to open %s for writing."
#define	string_updated		"InternetMgr:115" FS "Systems List saved."
#define	string_deleteConfirm	"InternetMgr:116" FS "Delete %s.  Are you sure?"
#define	string_header		"InternetMgr:117" FS "System         Address          Comment     "
#define	string_listenerFail	"InternetMgr:118" FS "Attempt to configure the TCP/IP network listener on this machine failed. You should reboot your system." 
#define	string_noLocal		"InternetMgr:119" FS "Network Address has not yet been specified for your system."
#define	string_uucpFail		"InternetMgr:120" FS "Attempt to configure UUCP through TCP/IP failed.  See the log file (%s) for details."
#define	string_networkInit	"InternetMgr:121" FS "Network has been initialized."
#define	string_execFail		"InternetMgr:122" FS "Execution of UNIX System command '%s' failed."
#define	string_startNet		"InternetMgr:123" FS "Starting up the network ..."
#define	string_noAccessNodeDir	"InternetMgr:124" FS "Could not create entry under $HOME/.node"
#define	string_noItem		"InternetMgr:125" FS "No System entries currently exist."
#define	string_installDone	"InternetMgr:126" FS "\"%s\" installed."
#define	string_noMultiple	"InternetMgr:127" FS "Can only install one system at a time."
#define	string_installFailed	"InternetMgr:128" FS "Installation of \"%s\" failed!"
#define	string_noSelect		"InternetMgr:129" FS "You have not selected anything."
#define	string_pathDeny		"InternetMgr:130" FS "\"%s\" does not exist or permission to access is denied!"
#define	string_pathExist	"InternetMgr:131" FS "\"%s\" already exists!"
#define	string_longFilename	"InternetMgr:132" FS "\"%s\" is too long. Name must be 14 characters or less."
#define	string_noTCP		"InternetMgr:133" FS "The TCP network is unavailable on this system.  You may need to reboot your system."
#define	string_notFound		"InternetMgr:134" FS "System specified not in the list!"
#define	string_setupNet		"InternetMgr:135" FS "Setting up the 'listener'. Please wait ..."
#define	string_itemNotSave	"InternetMgr:136" FS "Current changes to Systems List have not been saved."
#define	string_expandWarn	"InternetMgr:137" FS "You are about to copy the Network Hosts Table from the selected system.  Click\
\"Append\" if you wish to append the information to your current Hosts \
Table.  Click \"Replace\" if you wish to replace your current Hosts Table.  Click\
\"Cancel\" if you want to cancel this operation."
#define	string_openFail		"InternetMgr:138" FS "Can't open %s."
#define	string_accessFail	"InternetMgr:139" FS "Can't access %s."
#define	string_writeFail	"InternetMgr:140" FS "Can't write to %s."
#define	string_createFail	"InternetMgr:141" FS "Can't create %s."
#define	string_noFile		"InternetMgr:142" FS "'%s' does not exist and you are not authorized to create it!"
#define	string_noExpand		"InternetMgr:143" FS "Can not get systems list from your own system!"

#define	label_actions		"InternetMgr:201" FS "Actions"
#define	label_msystem		"InternetMgr:202" FS "System"
#define	label_search		"InternetMgr:203" FS "Search"
#define	label_expand		"InternetMgr:204" FS "Get Systems List"
#define	label_install		"InternetMgr:205" FS "Install..."
#define	label_minstall		"InternetMgr:206" FS "Install"
#define	label_location		"InternetMgr:207" FS "Location:"
#define	label_help		"InternetMgr:208" FS "Help"
#define	label_saveList		"InternetMgr:209" FS "Save Systems List"
#define	label_open		"InternetMgr:210" FS "Open"
#define	label_exit		"InternetMgr:211" FS "Exit"
#define	label_discard		"InternetMgr:212" FS "Discard"
#define	label_new		"InternetMgr:213" FS "New..."
#define	label_delete		"InternetMgr:214" FS "Delete"
#define	label_cancel		"InternetMgr:215" FS "Cancel"
#define	label_mfind		"InternetMgr:216" FS "Find..."
#define	label_find		"InternetMgr:217" FS "Find"
#define	label_continue		"InternetMgr:218" FS "Continue"
#define	label_properties	"InternetMgr:219" FS "Properties..."
#define	label_first		"InternetMgr:220" FS "First"
#define	label_last		"InternetMgr:221" FS "Last"
#define	label_apply		"InternetMgr:222" FS "Apply"
#define	label_reset		"InternetMgr:223" FS "Reset"
#define	label_name		"InternetMgr:224" FS "Name:"
#define	label_system		"InternetMgr:225" FS "System Name:"
#define	label_full		"InternetMgr:226" FS "Show Full Address:"
#define	label_local		"InternetMgr:227" FS "Local System Name:"
#define	label_netAddr		"InternetMgr:228" FS "Network Address:"
#define	label_comment		"InternetMgr:229" FS "Comment:"
#define	label_permit		"InternetMgr:230" FS "Permit local access to:"
#define	label_none		"InternetMgr:231" FS "No One"
#define	label_all		"InternetMgr:232" FS "All Users"
#define	label_specify		"InternetMgr:233" FS "Specific Users"
#define	label_allow		"InternetMgr:234" FS "Allowed Users:"
#define	label_login		"InternetMgr:235" FS "Login ID:"
#define	label_insert		"InternetMgr:236" FS "Insert"
#define	label_applyEdit		"InternetMgr:237" FS "Apply Edits"
#define	label_net1Addr		"InternetMgr:238" FS "Network Address (1):"
#define	label_net2Addr		"InternetMgr:239" FS "Network Address (2):"
#define	label_net3Addr		"InternetMgr:240" FS "Network Address (3):"
#define	label_setup		"InternetMgr:241" FS "Internet Setup ..."
#define	label_toc		"InternetMgr:242" FS "Table of Contents ..."
#define	label_desk		"InternetMgr:243" FS "Help Desk ..."
#define	label_save		"InternetMgr:244" FS "Save"
#define	label_append		"InternetMgr:245" FS "Append"
#define	label_replace		"InternetMgr:246" FS "Replace"
#define	label_propertiesTitle	"InternetMgr:247" FS "Remote System Properties"
#define	label_quitWarn		"InternetMgr:248" FS "Quit Warning"
#define	label_propertiesLTitle	"InternetMgr:249" FS "Local System Properties"

#define	mnemonic_actions	"InternetMgr:401" FS "A"
#define	mnemonic_msystem	"InternetMgr:402" FS "S"
#define	mnemonic_search		"InternetMgr:403" FS "r"
#define	mnemonic_expand		"InternetMgr:404" FS "G"
#define	mnemonic_install	"InternetMgr:405" FS "I"
#define	mnemonic_minstall	"InternetMgr:406" FS "I"
#define	mnemonic_help		"InternetMgr:407" FS "H"
#define	mnemonic_saveList	"InternetMgr:408" FS "S"
#define	mnemonic_open		"InternetMgr:409" FS "O"
#define	mnemonic_exit		"InternetMgr:410" FS "X"
#define	mnemonic_discard	"InternetMgr:411" FS "D"
#define	mnemonic_new		"InternetMgr:412" FS "N"
#define	mnemonic_delete		"InternetMgr:413" FS "D"
#define	mnemonic_cancel		"InternetMgr:414" FS "C"
#define	mnemonic_mfind		"InternetMgr:415" FS "F"
#define	mnemonic_find		"InternetMgr:416" FS "F"
#define	mnemonic_continue	"InternetMgr:417" FS "C"
#define	mnemonic_properties	"InternetMgr:418" FS "P"
#define	mnemonic_next		"InternetMgr:419" FS "N"
#define	mnemonic_prev		"InternetMgr:420" FS "P"
#define	mnemonic_first		"InternetMgr:421" FS "i"
#define	mnemonic_last		"InternetMgr:422" FS "L"
#define	mnemonic_apply		"InternetMgr:423" FS "A"
#define	mnemonic_reset		"InternetMgr:424" FS "R"
#define	mnemonic_setup		"InternetMgr:425" FS "I"
#define	mnemonic_toc		"InternetMgr:426" FS "T"
#define	mnemonic_desk		"InternetMgr:427" FS "H"
#define	mnemonic_all		"InternetMgr:428" FS "l"
#define	mnemonic_specify	"InternetMgr:429" FS "S"
#define	mnemonic_allow		"InternetMgr:430" FS "l"
#define	mnemonic_none		"InternetMgr:431" FS "N"
#define	mnemonic_insert		"InternetMgr:432" FS "I"
#define	mnemonic_applyEdit	"InternetMgr:433" FS "E"
#define	mnemonic_save		"InternetMgr:434" FS "S"
#define	mnemonic_append		"InternetMgr:435" FS "A"
#define	mnemonic_replace	"InternetMgr:436" FS "R"


#define	help_setup		"InternetMgr:501" FS "10"
#define	help_find		"InternetMgr:502" FS "250"
#define	help_install		"InternetMgr:503" FS "50"
#define	help_properties		"InternetMgr:504" FS "200"
#define	help_expand		"InternetMgr:505" FS "100"
#define	help_toc		"InternetMgr:506" FS "170"
#define	help_desk		"InternetMgr:507" FS "180"

#define	title_install		"InternetMgr:601" FS "Install Remote System Icon"
#define	title_setup		"InternetMgr:602" FS "Internet Setup"
#define	title_find		"InternetMgr:603" FS "Find"
#define	title_hinstall		"InternetMgr:604" FS "Install"
#define	title_properties	"InternetMgr:605" FS "Properties"
#define	title_expand		"InternetMgr:606" FS "Get Systems List"
#define	title_toc		"InternetMgr:607" FS "Table of Contents"
#define	title_appendWarn	"InternetMgr:608" FS "Append Systems List: Warning"
#define	title_deleteSys		"InternetMgr:609" FS "Delete System"

#endif /* __Ol_Inet_Error_h__ */
