/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/error.h	1.17"
#endif

#ifndef ERROR_H
#define ERROR_H

#define APP_NAME	"prtsetup"

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define HELP_FILE		"dtadmin/printer.hlp"

#define TXT_addPrt		"prtsetup:1" FS \
    "Printer Setup:  Add %s Printer"
#define TXT_prtProp		"prtsetup:2" FS \
    "Printer Setup:  Printer Properties - %s"
#define TXT_noDelete		"prtsetup:3" FS "Could not delete printer"
#define TXT_noPerm		"prtsetup:4" FS \
    "You do not have permission to cancel this print request."
#define TXT_intrnlErr		"prtsetup:5" FS "Internal Error:\n\t%s\n"
#define TXT_errnoEq		"prtsetup:6" FS "\t(errno=%d)\n"
#define TXT_reallyDelete	"prtsetup:7" FS \
    "Delete printer(s):  %s.  Are you sure?"
#define TXT_activeJobs		"prtsetup:8" FS \
    "%s contains Print Requests that have not yet been printed.  " \
    "Delete this printer anyway?\n\n" \
    "Note:  Deleting the printer will cause the Print Requests to " \
    "be cancelled.  You may want to let these requests complete before " \
    "deleting the printer."
#define TXT_badName		"prtsetup:9" FS \
    "You must provide a printer name."
#define TXT_blankRemoteName	"prtsetup:10" FS \
    "Must specify a remote system."
#define TXT_badRemoteName	"prtsetup:11" FS "Remote system unknown!"
#define TXT_badAdmin		"prtsetup:12" FS \
    "Cannot change/set printer properties!\n\n"
#define TXT_printerExists	"prtsetup:14" FS \
    "A Printer with this name already exists!\n\n"
#define TXT_badAccess		"prtsetup:15" FS \
    "Printer files not readable/writeable!\n\n"
#define TXT_badCancel		"prtsetup:16" FS \
    "Could not cancel all print requests.  " \
    "This printer will not be deleted\n\n"
#define TXT_badDelete		"prtsetup:17" FS \
    "Could not delete printer.  " \
    "The print spooler is probably not running\n\n"
#define TXT_badSystem		"prtsetup:18" FS \
    "Could not change remote system access rights.\n"
#define TXT_nonexistent		"prtsetup:19" FS "Device does not exist\n"
#define TXT_noneSelected	"prtsetup:20" FS "No printer has been selected"
#define TXT_statusTitle		"prtsetup:21" FS \
    "Printer Setup:  Printer Control"
#define TXT_idlePrintf		"prtsetup:22" FS "%s is idle"
#define TXT_printingPrintf	"prtsetup:23" FS "%s is printing %s"
#define TXT_faultedPrintf	"prtsetup:24" FS \
    "%s is stopped with a printer fault"
#define TXT_disabledPrintf	"prtsetup:25" FS "%s is disabled"
#define TXT_noStatus		"prtsetup:26" FS "Can not get status for %s"
#define TXT_cantAccept		"prtsetup:27" FS \
    "Unable to accept new requests\n\n"
#define TXT_cantReject		"prtsetup:28" FS \
    "Unable to reject new requests\n\n"
#define TXT_cantEnable		"prtsetup:29" FS "Could not enable printer\n\n"
#define TXT_cantDisable		"prtsetup:30" FS \
    "Unable to disable printer\n\n"
#define TXT_acceptTxt		"prtsetup:31" FS \
    "Printer will now accept new requests\n\n"
#define TXT_rejectTxt		"prtsetup:32" FS \
    "Printer will now reject new requests\n\n"
#define TXT_enableTxt		"prtsetup:33" FS "Printer is enabled\n\n"
#define TXT_disableTxt		"prtsetup:34" FS "Printer is disabled\n\n"
#define TXT_whenText		"prtsetup:35" FS \
    "What do you want to do with the currently printing request?:\n\n"
#define TXT_acceptCap		"prtsetup:36" FS "New requests:"
#define TXT_enableCap		"prtsetup:37" FS "Printer"
#define TXT_badPgLen		"prtsetup:38" FS \
    "Page length must be a positive number"
#define TXT_badPgWid		"prtsetup:39" FS \
    "Page width must be a positive number"
#define TXT_badCpi		"prtsetup:40" FS \
    "Character pitch must be a positive number"
#define TXT_badLpi		"prtsetup:41" FS \
    "Line Pitch must be a positive number"
#define TXT_installTitle	"prtsetup:42" FS \
    "Printer Setup:  Install Printer Icon"
#define TXT_toolbox		"prtsetup:43" FS "Location:"
#define TXT_noPrtDir		"prtsetup:45" FS \
    "Could not create directory for printer defaults."
#define TXT_unknownErr		"prtsetup:47" FS "Unknown error!\n\n"

#define TXT_printer		"prtsetup:48" FS "Printer"
#define TXT_add			"prtsetup:49" FS "New..."
#define TXT_delete		"prtsetup:50" FS "Delete"
#define TXT_cancel		"prtsetup:51" FS "Cancel"
#define TXT_continue		"prtsetup:52" FS "Continue"
#define TXT_remoteAccess	"prtsetup:53" FS "Set Remote Access..."
#define TXT_properties		"prtsetup:54" FS "Properties..."
#define TXT_dfltPrt		"prtsetup:55" FS "Make Default"
#define TXT_control		"prtsetup:56" FS "Control Printer..."
#define TXT_installW		"prtsetup:57" FS "Install..."
#define TXT_help		"prtsetup:58" FS "Help"
#define TXT_parallel		"prtsetup:59" FS "Parallel"
#define TXT_serial		"prtsetup:60" FS "Serial"
#define TXT_remote		"prtsetup:61" FS "Remote"
#define TXT_setup		"prtsetup:62" FS "Printer_Setup"
#define TXT_apply		"prtsetup:63" FS "Apply"
#define TXT_reset		"prtsetup:64" FS "Reset"
#define TXT_lpt1		"prtsetup:65" FS "Lpt1"
#define TXT_lpt2		"prtsetup:66" FS "Lpt2"
#define TXT_com1		"prtsetup:67" FS "Com1"
#define TXT_com2		"prtsetup:68" FS "Com2"
#define TXT_other		"prtsetup:69" FS "Other"
#define TXT_device		"prtsetup:70" FS "Device:"
#define TXT_type		"prtsetup:71" FS "Type:"
#define TXT_name		"prtsetup:72" FS "Printer Name:"
#define TXT_port		"prtsetup:73" FS "Port:"
#define TXT_system		"prtsetup:74" FS "Remote System Name:"
#define TXT_rmtName		"prtsetup:75" FS "Remote Printer Name:"
#define TXT_rmtlpsys		"prtsetup:76" FS \
    "Printer Setup:  Set Remote Access"
#define TXT_basic		"prtsetup:77" FS "Basic"
#define TXT_configuration	"prtsetup:78" FS "Configuration"
#define TXT_communication	"prtsetup:79" FS "Communication"
#define TXT_requeue		"prtsetup:80" FS "Print Later"
#define TXT_complete		"prtsetup:81" FS "Finish Printing"
#define TXT_enable		"prtsetup:82" FS "Enabled"
#define TXT_disable		"prtsetup:83" FS "Disabled"
#define TXT_accept		"prtsetup:84" FS "Accept"
#define TXT_reject		"prtsetup:85" FS "Reject"
#define TXT_yes			"prtsetup:86" FS "Yes"
#define TXT_no			"prtsetup:87" FS "No"

#define TXT_allowRemote		"prtsetup:88" FS "Allow Remote Access?"
#define TXT_skipBanner		"prtsetup:89" FS "Always print Banner Page?"
#define TXT_alerter		"prtsetup:90" FS "Send Mail if printer fails?"
#define TXT_charPitch		"prtsetup:91" FS "Character Pitch:"
#define TXT_linePitch		"prtsetup:92" FS "Line Pitch:"
#define TXT_pageWidth		"prtsetup:93" FS "Page Width:"
#define TXT_pageLength		"prtsetup:94" FS "Page Length:"
#define TXT_baud		"prtsetup:95" FS "Baud Rate:"
#define TXT_parity		"prtsetup:96" FS "Parity:"
#define TXT_stopBits		"prtsetup:97" FS "Stop Bits:"
#define TXT_charSize		"prtsetup:98" FS "Character Size:"
#define TXT_in			"prtsetup:99" FS "in"
#define TXT_cm			"prtsetup:100" FS "cm"
#define TXT_chars		"prtsetup:101" FS "chars"

#define TXT_required		"prtsetup:102" FS "Yes"
#define TXT_optional		"prtsetup:103" FS "No"
#define TXT_b300		"prtsetup:105" FS "300"
#define TXT_b1200		"prtsetup:106" FS "1200"
#define TXT_b2400		"prtsetup:107" FS "2400"
#define TXT_b4800		"prtsetup:108" FS "4800"
#define TXT_b9600		"prtsetup:109" FS "9600"
#define TXT_b19200		"prtsetup:110" FS "19200"
#define TXT_even		"prtsetup:111" FS "Even"
#define TXT_odd			"prtsetup:112" FS "Odd"
#define TXT_none		"prtsetup:113" FS "None"
#define TXT_one			"prtsetup:114" FS "1"
#define TXT_two			"prtsetup:115" FS "2"
#define TXT_seven		"prtsetup:116" FS "7"
#define TXT_eight		"prtsetup:117" FS "8"

#define MNEM_printer		"prtsetup:118" FS "P"
#define MNEM_add		"prtsetup:119" FS "N"
#define MNEM_delete		"prtsetup:120" FS "D"
#define MNEM_cancel		"prtsetup:121" FS "C"
#define MNEM_continue		"prtsetup:122" FS "C"
#define MNEM_remoteAccess	"prtsetup:123" FS "R"
#define MNEM_properties		"prtsetup:124" FS "P"
#define MNEM_dfltPrt		"prtsetup:125" FS "D"
#define MNEM_control		"prtsetup:126" FS "C"
#define MNEM_installW		"prtsetup:127" FS "I"
#define MNEM_help		"prtsetup:128" FS "H"
#define MNEM_parallel		"prtsetup:129" FS "P"
#define MNEM_serial		"prtsetup:130" FS "S"
#define MNEM_remote		"prtsetup:131" FS "R"
#define MNEM_apply		"prtsetup:132" FS "A"
#define MNEM_reset		"prtsetup:133" FS "R"
#define MNEM_basic		"prtsetup:134" FS "B"
#define MNEM_configuration	"prtsetup:135" FS "C"
#define MNEM_communication	"prtsetup:136" FS "m"
#define MNEM_requeue		"prtsetup:137" FS "P"
#define MNEM_complete		"prtsetup:138" FS "F"

#define TXT_os			"prtsetup:139" FS "Remote Operating System is:"
#define TXT_sysv		"prtsetup:140" FS "System V"
#define TXT_bsd			"prtsetup:141" FS "BSD"
#define TXT_badInstall		"prtsetup:142" FS \
    "Unable to install printer(s):  "
#define TXT_actions		"prtsetup:143" FS "Actions"
#define MNEM_actions		"prtsetup:144" FS "A"
#define TXT_exit		"prtsetup:145" FS "Exit"
#define MNEM_exit		"prtsetup:146" FS "E"
#define TXT_appTitle		"prtsetup:147" FS "Printer Setup"

#define TXT_application		"prtsetup:148" FS "Printer Setup..."
#define MNEM_application	"prtsetup:149" FS "P"
#define TXT_appHelp		"prtsetup:150" FS "Printer Setup"
#define TXT_appHelpSect		"prtsetup:151" FS "10"
#define TXT_helpDesk		"prtsetup:152" FS "Help Desk..."
#define MNEM_helpDesk		"prtsetup:153" FS "H"
#define TXT_TOC			"prtsetup:154" FS "Table of Contents..."
#define MNEM_TOC		"prtsetup:155" FS "T"
#define TXT_tocHelp		"prtsetup:156" FS "Table of Contents"
#define TXT_propHelp		"prtsetup:157" FS "Properties"
#define TXT_propHelpSect	"prtsetup:158" FS "370"
#define TXT_ctrlHelp		"prtsetup:159" FS "Control Printer"
#define TXT_ctrlHelpSect	"prtsetup:160" FS "190"
#define TXT_rmtHelp		"prtsetup:161" FS "Set Remote Access"
#define TXT_rmtHelpSect		"prtsetup:162" FS "270"
#define TXT_installHelp		"prtsetup:163" FS "Install Printer"
#define TXT_installHelpSect	"prtsetup:164" FS "240"

#define TXT_helpW		"prtsetup:165" FS "Help..."
#define MNEM_helpW		"prtsetup:166" FS "H"

#define TXT_appName		"prtsetup:167" FS "Printer Setup"

#define TXT_addBtn		"prtsetup:168" FS "Add"
#define MNEM_addBtn		"prtsetup:169" FS "A"
#define TXT_save		"prtsetup:170" FS "Save"
#define MNEM_save		"prtsetup:171" FS "S"

#define TXT_sysName		"prtsetup:172" FS "System Name"
#define TXT_osType		"prtsetup:173" FS "O/S Type"
#define TXT_rmtAccess		"prtsetup:174" FS "Remote Access"
#define TXT_enabled		"prtsetup:175" FS "Enabled"
#define TXT_errorTitle		"prtsetup:176" FS "Printer Setup:  Message"
#define TXT_deleteTitle		"prtsetup:177" FS \
    "Printer Setup:  Delete Printer"
#define TXT_whenTitle		"prtsetup:178" FS \
    "Printer Setup:  Disable Printer"
#define TXT_invalidChar		"prtsetup:179" FS "Character '%c' is invalid"
#define TXT_installed		"prtsetup:180" FS \
    "Successfully installed printer(s):  %s\n\n"
#define TXT_install		"prtsetup:181" FS "Install"
#define MNEM_install		"prtsetup:182" FS "I"
#define TXT_otherDesc		"prtsetup:183" FS "Other"
#define TXT_addHelp		"prtsetup:184" FS "Add"
#define TXT_addHelpSect		"prtsetup:185" FS "320"

extern void	Error (Widget, char *, int);
extern void	ErrorConfirm (Widget widget, char *errorMsg, int noticeType,
			      XtCallbackProc callback, XtPointer closure);
extern char	*GetStr (char *idstr);

#endif /* ERROR_H */
