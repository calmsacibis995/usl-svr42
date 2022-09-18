/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:dialup/dtcopy.h	1.8"
#endif

/*
 * Desktop remot file copy
 */

#define GGT		GetGizmoText

/* This is the path that will be used for mail command executions */
#define	BIN		"PATH=/usr/bin "
#define	DTMSG		"/usr/X/desktop/rft/dtmsg"
#define	WORKDIR		"/var/spool/uucppublic"
#define	UUCPDIR		"/var/spool/uucppublic"
#define WORKDIRMODE     (S_ISVTX | S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO) /* 50777 */
#define LASTCHAR(s)	(s+strlen(s)-1)

	/* Network services primitives */


/*	
 * Contents of node.attrs files. Tokens of the
 * form name=value
 */ 

#define	DUSER		"DUSER="	/* Destination user name */
#define DMACH		"DMACH="        /* Destination machine name */
#define DESTINY		"DESTINY="      /* Is the destination machine a Destiny? */
#define	DPATH		"DPATH="        /* Destination path to place the file */

/*	
 * Contents of the device files. Tokens of the
 * form name=value
 */ 

#define	PORT		"PORT="		/* Port name */
#define TYPE		"TYPE="		/* Modem? DataKit? Direct? */

/* Machine name length */

#define	DST_LEN		20
#define	PATH_LEN	128
#define	UNAMESIZE	20
#define	BUF_SIZE	1024

/* Link list of name=value structures */

typedef	struct	stringll {
	char	*name;
	char	*value;
	struct	stringll  *next;
} stringll_t;

#define	NULL_STRING	(stringll_t *)0

/* These defs are used by nview.c and dtcopy.c to define the */
/* I18n message						     */

#define TXT_TITLE		"dtcopy:1" FS "Remote System: Properties"
#define FormalClientName   	TXT_TITLE

#define ClientName		"dtcopy:2" FS "Remote System"
#define ClientClass		"dtcopy"
#define TXT_DESTINATION		"dtcopy:3" FS "Destination System:"
#define TXT_RECIPIENT		"dtcopy:4" FS "Recipient:"
#define TXT_DELIVERY		"dtcopy:5" FS "Delivery Method:"

#define TXT_DEST_NO		"dtcopy:10" FS "Remote system not specified."
#define TXT_LOGIN_NO		"dtcopy:11" FS "Unable to determine who you are!"
#define TXT_NODE_NO		"dtcopy:12" FS "Unable to load remote system's properties."
#define TXT_FOLDER_NO		"dtcopy:13" FS "Must specify a complete folder path."
#define TXT_SAVE_NO		"dtcopy:14" FS "Unable to save remote system's properties."
#define TXT_OPTION		"dtcopy:15" FS "Wrong option."
#define TXT_SRC_NO		"dtcopy:16" FS "No source files/folder specified."
#define TXT_EFILE_NO		"dtcopy:17" FS "Unable to open error file."
#define TXT_SUCCESS		"dtcopy:18" FS "Request succeeded."
#define TXT_FAIL		"dtcopy:19" FS "Request failed; an error report will be sent to you via mail."
#define TXT_QUEUED		"dtcopy:20" FS "Request submitted. Mail will be sent to you when the files are sent."
#define TXT_MAIL		"dtcopy:21" FS "An error has occurred while sending the files to the remote system.\nThe following is the error output from UNIX System command:\n"

