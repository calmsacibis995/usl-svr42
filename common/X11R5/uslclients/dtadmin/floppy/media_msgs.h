/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:floppy/media_msgs.h	1.27"
#endif

/*
 * This file contains all the message strings for the Desktop UNIX
 * dtadmin client MediaMgr.
 * 
 */

#define	string_hasData		"dtmedia:1" FS \
			"Floppy in drive has data; formatting will erase this"

#define	string_insMsg		"dtmedia:2" FS \
				"Insert %s in drive and click %s"

#define	string_unreadDisk	"dtmedia:4" FS \
			"%s in drive %s is unreadable; try another."
#define	string_isFormatted	"dtmedia:5" FS \
			"Floppy in drive %s is already formatted"

#define	string_fmtDOStoUNIX 	"dtmedia:7" FS \
				"Floppy has DOS format; reformat it?"
#define	string_notOwner 	"dtmedia:8" FS \
	"You do not have permission to open media that contain File Systems"

#define	string_doingFmt  	"dtmedia:9" FS "formatting please wait ..."
#define	string_doingMkfs 	"dtmedia:10" FS \
			"Creating file system please wait ..."
#define	string_opOK		"dtmedia:11" FS "%s complete"
#define	string_opFailed		"dtmedia:12" FS "%s failed"
#define	string_opKilled		"dtmedia:13" FS "%s cancelled"
#define	string_bkupSummary 	"dtmedia:14" FS "%s %s to %s: %s"
#define	string_waitIndex	"dtmedia:15" FS "Creating index; please wait ..."
#define	string_overwrite	"dtmedia:16" FS "Overwrite files if they exist"
#define	string_bdocTitle	"dtmedia:17" FS "Enter a file to backup to"
#define	string_rstTitle		"dtmedia:18" FS "Enter a file to restore from"
#define	string_noneset		"dtmedia:19" FS "No files currently selected"
#define	string_rstSummary	"dtmedia:20" FS "%s files from %s"
#define	string_notaBkup		"dtmedia:21" FS "%s is not a backup archive"
#define	string_unreadFile	"dtmedia:22" FS "<%s> is unreadable"
#define	string_newFile		"dtmedia:23" FS \
		"<%s> already exists. Choose a different name for this backup"
#define	string_cantWrite	"dtmedia:24" FS \
			"Unable to write %s; check write-protection"
#define	string_etc		"dtmedia:25" FS "selected files"
#define	string_badMalloc	"dtmedia:26" FS "Memory allocation error"

#define	string_fmtOp		"dtmedia:27" FS "Formatting"
#define	string_mkfsOp		"dtmedia:28" FS "File system creation"
#define	string_stderr		"dtmedia:29" FS "MediaMgr: %s: %s"

#define	label_open		"dtmedia:30" FS "Open ..."
#define	label_cancel		"dtmedia:31" FS "Cancel"
#define	label_sched		"dtmedia:32" FS "Schedule ..."
#define	label_backup		"dtmedia:33" FS "Backup"
#define	label_restore		"dtmedia:34" FS "Restore"
#define	label_format		"dtmedia:35" FS "Format"
#define	label_rewind		"dtmedia:36" FS "Rewind"
#define	label_reset		"dtmedia:37" FS "Reset"
#define	label_file		"dtmedia:38" FS "File"
#define	label_help		"dtmedia:39" FS "Help"
#define	label_type		"dtmedia:40" FS "Type"
#define	label_insert		"dtmedia:41" FS "Insert"
#define	label_log		"dtmedia:42" FS "Save File List"

#define	label_doc		"dtmedia:44" FS "File"
#define	label_action		"dtmedia:45" FS "Actions"
#define	label_exit		"dtmedia:46" FS "Exit"
#define	label_setdoc		"dtmedia:47" FS "Set File"
#define	label_immed		"dtmedia:48" FS "Immediate"
#define	label_save		"dtmedia:49" FS "Save"
#define	label_saveas		"dtmedia:50" FS "Save As ..."
#define	label_copy		"dtmedia:51" FS "Copy ..."
#define	label_delete		"dtmedia:52" FS "Erase"
#define	label_convert		"dtmedia:53" FS "Convert on Copy"
#define	label_noconvert		"dtmedia:54" FS "Don't Convert"
#define	label_selectF		"dtmedia:55" FS "Copy"
#define	label_continue		"dtmedia:56" FS "Continue"
#define	label_overwrite		"dtmedia:57" FS "Overwrite"

#define	mnemonic_selectF	"dtmedia:59" FS "C"
#define	mnemonic_open		"dtmedia:60" FS "O"
#define	mnemonic_cancel		"dtmedia:61" FS "C"
#define	mnemonic_sched		"dtmedia:62" FS "S"
#define	mnemonic_backup		"dtmedia:63" FS "B"
#define	mnemonic_restore	"dtmedia:64" FS "R"
#define	mnemonic_format		"dtmedia:65" FS "F"
#define	mnemonic_reset		"dtmedia:66" FS "R"
#define	mnemonic_file		"dtmedia:67" FS "F"
#define	mnemonic_help		"dtmedia:68" FS "H"
#define	mnemonic_type		"dtmedia:69" FS "T"
#define	mnemonic_insert		"dtmedia:70" FS "I"
#define	mnemonic_log		"dtmedia:71" FS "A"
#define	mnemonic_action		"dtmedia:72" FS "A"
#define	mnemonic_exit		"dtmedia:73" FS "X"
#define	mnemonic_immed		"dtmedia:74" FS "I"
#define	mnemonic_save		"dtmedia:75" FS "S"
#define	mnemonic_saveas		"dtmedia:76" FS "V"
#define	mnemonic_copy		"dtmedia:77" FS "P"
#define	mnemonic_delete		"dtmedia:78" FS "R"
#define	mnemonic_continue	"dtmedia:79" FS "O"

#define	label_highDns		"dtmedia:80" FS "HIGH"
#define	label_lowDns		"dtmedia:81" FS "LOW"

#define	label_bkupFmt		"dtmedia:82" FS "Backup Use"
#define	label_s5Fmt		"dtmedia:83" FS "Desktop Folder"
#define	label_dosFmt		"dtmedia:84" FS "DOS Format"

#define	mnemonic_highDns	"dtmedia:85" FS "G"
#define	mnemonic_lowDns		"dtmedia:86" FS "W"

#define	mnemonic_bkupFmt	"dtmedia:87" FS "B"
#define	mnemonic_s5Fmt		"dtmedia:88" FS "D"
#define	mnemonic_dosFmt		"dtmedia:89" FS "M"

#define	label_devCaption	"dtmedia:90" FS "Device: "
#define	label_dnsCaption	"dtmedia:91" FS "Density: "
#define	label_fmtCaption	"dtmedia:92" FS "Type: "

#define	label_targetCaption	"dtmedia:99" FS "Target File: "
#define	label_filesCaption	"dtmedia:100" FS "Files: "

#define	label_bkupToCaption	"dtmedia:101" FS "Backup To: "
#define	label_bkupTypeCaption	"dtmedia:102" FS "Backup Type: "
#define	label_bkupClassCaption	"dtmedia:103" FS "Backup Class: "
#define	label_rstFromCaption	"dtmedia:104" FS "Restore From: "

#define	label_systemClass	"dtmedia:105" FS "Full System"
#define	label_selfClass		"dtmedia:106" FS "Personal"
#define	label_userClass		"dtmedia:107" FS "Other Users"
#define	label_complType		"dtmedia:108" FS "Complete"
#define	label_incrType		"dtmedia:109" FS "Incremental"
#define	label_selectFiles	"dtmedia:110" FS "Selected Files"
#define	label_select		"dtmedia:111" FS "Select All"
#define	label_unselect		"dtmedia:112" FS "Unselect All"
#define	label_show		"dtmedia:113" FS "Show Files"
#define	label_edit		"dtmedia:114" FS "Edit"
#define	label_exclude		"dtmedia:115" FS "Exclude"
#define	label_groupCaption	"dtmedia:116" FS "Files & Folders:"
#define	label_prev_group	"dtmedia:117" FS "Prev Group"
#define	label_next_group	"dtmedia:118" FS "Next Group"

#define	mnemonic_systemClass	"dtmedia:120" FS "Y"
#define	mnemonic_selfClass	"dtmedia:121" FS "P"
#define	mnemonic_userClass	"dtmedia:122" FS "T"
#define	mnemonic_complType	"dtmedia:123" FS "E"
#define	mnemonic_incrType	"dtmedia:124" FS "N"
#define	mnemonic_selectFiles	"dtmedia:125" FS "S"
#define	mnemonic_select		"dtmedia:126" FS "S"
#define	mnemonic_unselect	"dtmedia:127" FS "U"
#define	mnemonic_show		"dtmedia:128" FS "H"
#define	mnemonic_edit		"dtmedia:129" FS "E"
#define	mnemonic_exclude	"dtmedia:130" FS "E"
#define	mnemonic_convert	"dtmedia:131" FS "V"
#define	mnemonic_noconvert	"dtmedia:132" FS "N"

#define	label_convertCaption	"dtmedia:140" FS "UNIX <-> DOS:"
#define	label_parentdir		"dtmedia:141" FS "previous directory"

#define	string_UtoMcopy		"dtmedia:150" FS \
			"UNIX file(s) copied to DOS floppy"
#define	string_MtoUcopy		"dtmedia:151" FS \
			"file(s) copied from DOS floppy to UNIX System."
#define	string_UtoMerror	"dtmedia:152" FS \
			"error copying UNIX file(s) to DOS floppy"
#define	string_cantCopy		"dtmedia:153" FS "Unable to copy DOS file(s)"
#define	string_cantDelete	"dtmedia:154" FS "Unable to delete DOS file(s)"

#define	string_cpyTitle		"dtmedia:156" FS "Copy DOS Files"
#define	string_cpyPrompt	"dtmedia:157" FS "Choose the destination folder"
#define	string_cantList		"dtmedia:158" FS \
			"Unable to read DOS directory"
#define	string_DOStitle		"dtmedia:159" FS "DOS Floppy - %s"

#define	string_fmtVolume	"dtmedia:160" FS "%s number %d"
#define	string_callSched	"dtmedia:161" FS \
			"Invoking Task Scheduler utility"
#define	string_cantIndex	"dtmedia:162" FS \
			"Error indexing files for backup - cancelling"
#define	string_doingBkup	"dtmedia:163" FS \
			"Creating backup archive; please wait ..."
#define	string_doingRst		"dtmedia:164" FS \
			"Restore in progress; please wait ..."
#define	string_savedAs		"dtmedia:165" FS "Backup script saved as %s"
#define	string_notScript	"dtmedia:166" FS "Not a backup script file"
#define	string_readIndex	"dtmedia:167" FS \
			"Reading contents; please wait ..."
#define	string_startBkup	"dtmedia:168" FS "Starting backup at %s"
#define	string_skip		"dtmedia:169" FS "Skipping older file:\n\n%s"

#define	label_toc		"dtmedia:170" FS "Table of Contents..."
#define	label_hlpdsk		"dtmedia:171" FS "Help Desk..."
#define	label_bkrst		"dtmedia:172" FS "Backup-Restore..."
#define	label_fmtHlp		"dtmedia:173" FS "Format..."
#define	label_dosHlp		"dtmedia:174" FS "DOS Floppy..."
#define	mnemonic_toc		"dtmedia:175" FS "T"
#define	mnemonic_hlpdsk		"dtmedia:176" FS "K"
#define	mnemonic_fmtHlp		"dtmedia:177" FS "F"
#define	mnemonic_bkrst		"dtmedia:178" FS "B"
#define	mnemonic_dos		"dtmedia:179" FS "D"

#define	help_intro		"dtmedia:180" FS "10"
#define	help_bkup_open		"dtmedia:181" FS "160"	
#define	help_bkup_save		"dtmedia:182" FS "210"	
#define	help_bkup_confirm	"dtmedia:183" FS "90"	
#define	help_rst_intro		"dtmedia:184" FS "240"	
#define	help_rst_confirm	"dtmedia:185" FS "260"	
#define	help_format		"dtmedia:186" FS "90"
#define	help_dos_intro		"dtmedia:187" FS "150"
#define	help_dos_copy		"dtmedia:188" FS "240"

#define	string_mountErr		"dtmedia:190" FS "Unable to mount %s"
#define	string_umountErr	"dtmedia:191" FS "Unable to unmount %s"
#define	string_noRoom		"dtmedia:192" FS \
			"Insufficient space on %s for %s"
#define	string_cantOpen		"dtmedia:193" FS "Unable to open folder %s"
#define	string_skipFile		"dtmedia:194" FS "Skipping older file:\n\n%s"
#define	string_dosDirCpy	"dtmedia:195" FS \
	"Cannot copy DOS directory %s; double click to see files."
#define	string_genMedia		"dtmedia:196" FS "media volume"

#define	title_doingBkup		"dtmedia:200" FS "Backup in Progress"
#define	title_doingRst		"dtmedia:201" FS "Restore in Progress"
#define	title_doingFmt		"dtmedia:202" FS "Format in Progress"
#define	title_confirmBkup	"dtmedia:203" FS "Backup:  Confirmation Notice"
#define	title_confirmRst	"dtmedia:204" FS "Restore:  Confirmation Notice"
#define	title_confirmFmt	"dtmedia:205" FS "Format:  Confirmation Notice"
#define	title_bkupUsers		"dtmedia:206" FS "Backup:  User List"
#define	title_bkupOpen		"dtmedia:207" FS "Backup:  Open Script"
#define	title_bkupSave		"dtmedia:208" FS "Backup:  Save Script"
#define	title_confirmUmt	"dtmedia:209" FS "Exit:  Confirmation"
#define	title_confirmIns	"dtmedia:210" FS "Exit:  Insert Media"

#define	string_reInsMsg		"dtmedia:211" FS \
				"Re-insert media in drive %s"
#define	string_busyMsg		"dtmedia:212" FS \
    "%s is still in use.  Click Exit to close the %s folder window or click " \
    "Cancel to leave it open."
#define	string_selected		"dtmedia:213" FS \
	"Selected item(s): %d                       Total item(s): %d"


#define	label_selectAll		"dtmedia:220" FS "Select All Files"
#define	label_unselectAll	"dtmedia:221" FS "Unselect All"
#define label_createDir		"dtmedia:222" FS "Create Directory..."
#define label_createOpen	"dtmedia:223" FS "Create & Open"
#define label_create		"dtmedia:224" FS "Create"
#define label_name		"dtmedia:225" FS "Name:"
#define label_help3dot		"dtmedia:226" FS "Help..."

#define mnemonic_selectAll	"dtmedia:230" FS "S"
#define mnemonic_unselectAll	"dtmedia:231" FS "U"
#define mnemonic_createDir	"dtmedia:232" FS "C"
#define mnemonic_createOpen	"dtmedia:233" FS "O"
#define mnemonic_create		"dtmedia:234" FS "R"
#define mnemonic_name		"dtmedia:235" FS "N"

#define title_createDir		"dtmedia:240" FS "File: Create DOS Directory"
#define string_cantCopyDirs	"dtmedia:242" FS "Cannot copy directories.  You must open a directory to copy its contents."
#define string_copyFilesOnly	"dtmedia:243" FS "Cannot copy special files or directories.  You must open a directory to copy its contents."
#define string_cantCopySpecial	"dtmedia:244" FS "Cannot copy special files."
#define string_fillInName	"dtmedia:245" FS "Must provide a Directory Name."
#define	string_DOSdelete	"dtmedia:246" FS "Selected item(s) erased"
#define title_eraseConfirm	"dtmedia:247" FS "File: Erase"
#define string_eraseConfirm	"dtmedia:248" FS "Erase selected item(s).  Are you sure?"
#define string_nothingErased	"dtmedia:249" FS "Erase canceled."
#define string_rmdirFailed	"dtmedia:251" FS "Could not erase selected directory(ies).  Directory(ies) might not be empty or floppy might be write-protected."
#define string_mixedEraseFailed	"dtmedia:252" FS "Could not erase selected items.  Directory(ies) might not be empty or floppy might be write-protected."
#define title_errorNotice	"dtmedia:253" FS "DOS Error"
#define	string_filesRenamed	"dtmedia:254" FS "Warning: file name(s) truncated when copied to DOS floppy"
#define string_UtoMcopyFailed	"dtmedia:255" FS "Could not copy selected file(s) to DOS floppy"
#define string_MtoUcopyFailed	"dtmedia:256" FS "Could not copy selected file(s) to UNIX System"
#define string_fileEraseFailed	"dtmedia:257" FS "Could not erase selected file(s)"
#define string_dirCreateFailed	"dtmedia:258" FS "Could not create directory on DOS floppy"
#define string_findErrors	"dtmedia:259" FS "%s finished but some item(s) could not be backed-up"

#define title_mounted		"dtmedia:270" FS "Backup: Media Unavailable"
#define	string_mounted		"dtmedia:271" FS \
    "Error: The floppy is currently in use.  " 
#define title_in_use		"dtmedia:272" FS "Backup: Overwrite Data?"
#define	string_in_use		"dtmedia:273" FS \
    "Media in drive has data. Overwrite?"
#define	string_unknownErr	"dtmedia:274" FS \
    "Transient error in writing media. Please try again."

#define	mnemonic_overwrite	"dtmedia:280" FS "O"

#define string_wrongDensity	"dtmedia:281" FS "Formatting wrong density; please check"
#define	string_cantWriteScript	"dtmedia:282" FS "%s failed: cannot write to file \"%s\""
#define	string_scheduling	"dtmedia:283" FS "Schedule"
