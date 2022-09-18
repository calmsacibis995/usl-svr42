/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:mail.c	1.60"
#endif

#define MAIL_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/FList.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include "mail.h"
#include "new.xpm"
#include "unread.xpm"
#include "read.xpm"
#include "deleted.xpm"

#define ZERO_MSGS	"\": 0 messages\n"
#define ZERO_MSGS_RO	"\": 0 messages [Read only]\n"

char *		BriefKeywords = NULL;	/* Keywords for the brief list */
char *		BriefKeywordsToIgnore = NULL;	/* Keywords unset in list */
char *		Record = NULL;		/* Name of file for out going mail */
char *		Folder = NULL;		/* Name of the output directory */
char *		Mbox = NULL;		/* Name of file for save and quit */
char *		Mprefix = NULL;		/* Prefix put before includes */
int		Version;		/* 40 = 4.0 mailx, 41 = 4.1 mailx */
char *		PrintCmd;		/* 'p' for 4.0 and 'bp' for 4.1 */
char *		Signature = NULL;	/* This is the users signature */

extern char *		PrintCommand;
extern o_ino_t		DummyDir;
extern HeaderSettings	ShowHeader;
extern RecordType	RecordOutgoing;

/* The following settings have the following meaning:
 *
 * hold:		Don't put read messages into mbox.  Put them
 *			back into the file they came from after exit
 *			or update.
 *
 * sendwait:		Tells mailx to wait for the background mailer to
 *			finish before returning to dtmail.
 *
 * noaskcc:		Tells mailx not to prompt for cc, bcc,
 * noaskbcc:		or subject during mail command.
 * noasksub:
 *
 * noautoprint:		I don't want to see the next message
 *			automatically after I delete a message.
 *
 * ignoreeof:		These two settings allow us to use "." to
 * dot:			terminate the input of a mail message.
 *
 * nodebug:		Need I say more?
 *
 * cmd=PrtMgr:		Used to specify the print command for Pipe command
 *
 * PAGER=:		Nullify any command the user may want for
 *			paging the mail messages (such as pg(1)).
 */

extern BaseWindowGizmo	MainWindow;
extern Widget		Root;
extern char *		Home;

static Pixmap NewPixmap = (Pixmap)0;
static Pixmap ReadPixmap;
static Pixmap UnreadPixmap;
static Pixmap DeletedPixmap;

MailRec *mailRec = (MailRec *)NULL;

void
DeleteMailRec (mp)
MailRec *mp;
{
	MailRec *	tp;
	MailRec *	last = (MailRec *)NULL;

	for (tp=mailRec; tp!=NULL; tp=tp->next) {
		if (mp == tp) {
			if (last != (MailRec *)NULL) {
				last->next = mp->next;
			}
			else {
				mailRec = mp->next;
			}
			break;
		}
		last = tp;
	}

	DeleteManageRec (mp->mng);

	/* Need to free many more things here */
	FreeSummaryOrDeleteList (GetSummaryListWidget (mp->mng), mp->summary);
	FreeSummaryOrDeleteList (GetDeletedListWidget (mp->mng), mp->deleted);
	FREE (mp->summary);
	FREE (mp->deleted);
	FREE (mp);
}

/*
 * Get the version of the mailx we are running with.
 * mailx version 4.0 outputs: mailx version 4.0
 * mailx version 4.1 outputs: 4.1
 * mailx version 4.2 outputs: 4.2
 */
static void
GetVersion (buf)
char *		buf;
{
	Version = 40;
	PrintCmd = PRINT_CMD;
	if (strncmp (buf, NTS_4DOT0, sizeof (NTS_4DOT0)-1) != 0) {
		/*
		 * Indicate that this is 4.1 or greater.
		 */
		Version = 41;
		PrintCmd = BPRINT_CMD;
	}
}

static MailRec *
CreateMailRec (filename)
char *	filename;
{
	MailRec *	mp;

	mp = (MailRec *)CALLOC (1, sizeof(MailRec));
	mp->next = mailRec;
	if (mailRec != NULL) {
		mp->alias = mailRec->alias;
	}
	else {
		mp->alias = (ListHead *)MALLOC (sizeof (ListHead));
	}
	mailRec = mp;

	mp->filename = STRDUP (filename);

	mp->alias = (ListHead *)0;
	mp->numBaseWindows = 0;
	mp->summary = (ListHead *)MALLOC (sizeof (ListHead));
	mp->deleted = (ListHead *)MALLOC (sizeof (ListHead));
	mp->summary->size = 0;
	mp->summary->items = (ListItem *)0;
	mp->summary->numFields = NUM_FIELDS;
	mp->summary->clientData = (XtArgVal)0;
	mp->deleted->size = 0;
	mp->deleted->items = (ListItem *)0;
	mp->deleted->numFields = NUM_FIELDS;
	mp->deleted->clientData = (XtArgVal)0;
	mp->noMail = False;
	mp->defaultItem = -1;
	mp->mng = (ManageRec *)0;
	mp->inode = 0;
	mp->rp = (ReadRec *)0;

	return mp;
}

static void
GetNumberOfItems (mp)
MailRec *mp;
{
	char *buf;

	buf = ProcessCommand (mp, FROM_DOLLAR, NULL);
	if (strcmp (buf, NTS_NO_APPL_MESS) == 0) {
		mp->summary->size = 0;
	}
	else {
		sscanf (buf+2, "%d", &mp->summary->size);
	}
}

void
SetStatusField (status, fields)
char *		status;
char **		fields;
{
	switch (status[0]) {
		case 'N': {
			fields[F_PIXMAP] = (char *)NewPixmap;
			fields[F_TEXT][5] = 'N';
			break;
		}
		case 'U': {
			status[0] = 'U';
			fields[F_TEXT][5] = 'U';
			fields[F_PIXMAP] = (char *)UnreadPixmap;
			break;
		}
		case 'D': {
			fields[F_PIXMAP] = (char *)DeletedPixmap;
			fields[F_TEXT][5] = 'D';
			break;
		}
		case 'H':
		case 'S':
		case 'M':
		case 'O':
		case 'R': {
			fields[F_PIXMAP] = (char *)ReadPixmap;
			fields[F_TEXT][5] = 'R';
			break;
		}
	}
}

void
CreateHeader (mp, i, string)
MailRec *mp;
int i;
char *string;
{
	char **		tmp;
	char 		buf[256];
	char 		text[256];
	char		status[10];
	char		num[10];
	static char *	regx = NULL;

	if (regx == NULL) {
		regx = (char *) regcmp (
			"(.)$0",
			" *([0-9]+)$1",
			" (.*)$2$",
			0
		);
	}

	mp->summary->items[i].set = False;
	mp->summary->items[i].fields =  (XtArgVal)MALLOC (
		sizeof (XtArgVal *) * mp->summary->numFields
	);
	tmp = (char **) mp->summary->items[i].fields;
	regex (regx, string+1, status, num, text);
	sprintf (buf, "%4s %1s %s", num, status, text);
	tmp[F_TEXT] = STRDUP (buf);
	/* Remember the message number for deleting and undeleting */
	mp->summary->items[i].clientData = (XtArgVal)atoi (num);
	SetStatusField (buf+5, tmp);
}

static void
ProcessHeaders (mp, buf)
MailRec *mp;
char *buf;
{
	int	i;
	char *	string;
	char *	start;
	char *	savept = NULL;

	/*
	 * Read in the bitmaps used to show the status of the 
	 * mail message.
	 */
	if (NewPixmap == (Pixmap)0) {
		NewPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)new_bits, new_width, new_height
		);
		ReadPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)read_bits, read_width, read_height
		);
		UnreadPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)unread_bits, unread_width, unread_height
		);
		DeletedPixmap = XCreateBitmapFromData (
			XtDisplay (Root),
			RootWindow (XtDisplay (Root), 0),
			(char *)deleted_bits, deleted_width, deleted_height
		);
	}
	start = buf;
	i = 0;
	if (mp->summary->size > 0) {
		for (i=0; i<mp->summary->size; i++) {
			if ((string = MyStrtok (start,"\n",&savept)) == NULL) {
				break;
			}
			start = NULL;
			CreateHeader (mp, i, string);
		}
		/* Indicate one item set */
		mp->defaultItem = GetDefaultItem (mp);
		FPRINTF((stderr, "Default selected = %d\n", 1));
	}
}

static void
GetMailItems (mp)
MailRec *mp;
{
	GetNumberOfItems (mp); /* Get mp->summary->size */
	if (mp->summary->size != 0) {
		mp->summary->items = (ListItem *)MALLOC(
			sizeof(ListItem)*mp->summary->size
		);
		/* Get the headers from mailx and stick them into
		 * the flat list.
		 */
		ProcessHeaders (mp, ProcessCommand (mp, FROM_1_DOLLAR, 0));
	}
}

static void
CreateMboxDir ()
{
	char		mboxDir[BUF_SIZE];
	static Boolean	first = True;
	static int	len;
	static mode_t	mask;

	if (first == True) {
		sprintf (mboxDir, "%s/%s", Home, NTS_MAILBOX);
		len = strlen (mboxDir);
		mask = GetUmask ();
		first = False;
	}
	/*
	 * Only create the directory if it is $HOME/mailbox.
	 */
	if (strncmp (mboxDir, Mbox, len) == 0) {
		if (StatFile (mboxDir, 0, 0) == 0) {
			if (mkdir (mboxDir, mask) == -1) {
				perror ("can't mknod: ");
			}
		}
	}
}

static char *
ExpandPath (buf)
char *	buf;
{
	char	path[BUF_SIZE];
	FILE *	fp;

	sprintf (path, "echo %s", buf);
	fp = popen (path, "r");
	if (fp == NULL) {
		return buf;
	}
	fgets (path, BUF_SIZE, fp);
	/* Remove trailing \n */
	*(path + strlen (path) - 1) = '\0';
	pclose (fp);
	FREE (buf);

	return STRDUP (path);
}

static char *
GetStringValue (buf, name)
char *	buf;
char *	name;
{
	if (buf == NULL || buf[0] != '=') {
		return STRDUP ("");
	}
	buf += 2;			/* Skip over '="' */
	buf[strlen (buf)-1] = '\0';	/* Remove trailing '"' */
	FPRINTF ((stderr, "%s=%s\n", name, buf));
	return STRDUP (buf);
}

static void
GetzzzIgnore (buf)
char *	buf;
{
	FREENULL (BriefKeywordsToIgnore);
	BriefKeywordsToIgnore = GetStringValue (buf, "zzzignore");
}

static void
GetzzzUnignore (buf)
char *	buf;
{
	FREENULL (BriefKeywords);
	BriefKeywords = GetStringValue (buf, "zzzunignore");
}

static void
GetMBOX (buf)
char *	buf;
{
	FREENULL (Mbox);
	Mbox = GetStringValue (buf, "MBOX");
	Mbox = ExpandPath (Mbox);
}

static void
GetzzzRecord (buf)
char *	buf;
{
	char *	tmp;

	tmp = GetStringValue (buf, "zzzrecord");
	if (strcmp (tmp, "Yes") == 0) {
		RecordOutgoing = DoIt;
	}
	else {
		RecordOutgoing = DontDoIt;
	}
}

static void
GetMprefix (buf)
char *	buf;
{
	FREENULL (Mprefix);
	Mprefix = GetStringValue (buf, "mprefix");
}

static void
GetFolder (buf)
char *	buf;
{
	FREENULL (Folder);
	Folder = GetStringValue (buf, "folder");
}

static void
GetRecord (buf)
char *	buf;
{
	FREENULL (Record);
	Record = GetStringValue (buf, "record");
}

static void
GetSign (buf)
char *	buf;
{
	char *	tp;
	char *	cp;

	FREENULL (Signature);
	Signature = GetStringValue (buf, "signature");

	/* Convert any "\n" to '\n' in the signature string */
	for (cp=Signature, tp=Signature; *cp!='\0'; cp++, tp++) {
		if (*cp == '\\' && *(cp+1) == 'n') {
			*tp = '\n';
			cp += 1;
		}
		else if (*cp == '\\' && *(cp+1) == 't') {
			*tp = '\t';
			cp += 1;
		}
		else {
			*tp = *cp;
		}
	}
	*tp = '\0';
}

static void
GetzzzNoBrief (buf)
char *	buf;
{
	ShowHeader = Full;
}

static Boolean
DefaultMbox ()
{
	static Boolean	first = True;
	char		tmp[BUF_SIZE];

	if (first == True) {
		first = False;
		sprintf (tmp, "%s/mbox", Home);
	}
	if (Mbox != NULL && strcmp (Mbox, tmp) == 0) {
		return True;
	}
	return False;
}

typedef struct _KeyWords {
	char *	word;
	int	len;
	PFV	func;
} KeyWords;

static KeyWords keywords[] = {
	{"mprefix",		 7,	GetMprefix},
	{"record",		 6,	GetRecord},
	{"folder",		 6,	GetFolder},
	{"sign",		 4,	GetSign},
	{"MBOX",		 4,	GetMBOX},
	{"zzzMBOX",		 7,	GetMBOX},
	{"zzzsign",		 7,	GetSign},
	{"zzznobrief",		10,	GetzzzNoBrief},
	{"zzzunignore",		11,	GetzzzUnignore},
	{"zzzignore",		 9,	GetzzzIgnore},
	{"zzzrecord",		 9,	GetzzzRecord},
};

/*
 * Look for mailx and dt variables
 */

void
GetSettings (mp, buf)
MailRec *	mp;
char *		buf;
{
	static char	text[BUF_SIZE];
	char		cmd[BUF_SIZE];
	char *		start = buf;
	char *		string;
	char *		savept = NULL;
	KeyWords *	p;
	int		i;
	int		n;

	n = XtNumber (keywords);
	RecordOutgoing = DontKnow;

	/* By default the header is brief unless dtmail has set zzznobrief */
	ShowHeader = Brief;

	while ((string = MyStrtok (start, "\n", &savept)) != NULL) {
		start = NULL;
		p = keywords;
		for (i=0; i<n; i++) {
			if (strncmp (string, p->word, p->len) == 0) {
				(*p->func) (string + p->len);
				break;
			}
			p += 1;
		}
	}
	/* Only change the default mbox if it is set to the user's
	 * home directory.  Otherwise, use either MBOX of zzzMBOX
	 */
	if (DefaultMbox () == True) {
		/* Create the default mbox */
		FREE (Mbox);
		sprintf (
			cmd,
			"%s/%s/%s",
			Home, NTS_MAILBOX, NTS_DEFAULT_SAVE_FILE
		);
		Mbox = STRDUP (cmd);
	}
	/*
	 * If the mbox directory doesn't exist then create it,
	 * but only if it is $HOME/mailbox.
	 */
	CreateMboxDir ();

	if (RecordOutgoing == DontKnow) {
		if (Record != NULL) {
			RecordOutgoing = DoIt;
		}
		else {
			RecordOutgoing = DontDoIt;
		}
	}

	/* If the user has specified a record value use that
	 * rather than the one we normally supply.
	 */
	if (Record == NULL) {
		if (Folder == NULL) {
			/* Create the default save location */
			sprintf (
				text,
				"%s/%s/%s",
				Home, NTS_MAILBOX, NTS_SENT_MAIL
			);
		}
		else {
			/* Create save location based on value in folder */
			sprintf (text, "%s/%s", Folder, NTS_SENT_MAIL);
		}
		Record = STRDUP (text);
	}
	if (RecordOutgoing == DoIt) {
		/* Tell mailx that we are recording outgoing mail */
		sprintf (cmd, "%s record=%s", SET_CMD, Record);
		(void)ProcessCommand (mp, cmd, NULL);
	}
	else {
		sprintf (cmd, "%s norecord", SET_CMD);
		(void)ProcessCommand (mp, cmd, NULL);
	}

	/* Set MBOX and folder to default or user specified location */
	sprintf (cmd, "%s MBOX=%s", SET_CMD, Mbox);
	(void)ProcessCommand (mp, cmd, NULL);
}

/*
 * Open mailx to the dummy file '/' and set all of the
 * necessary defaults.
 */

MailRec *
OpenMailx()
{
	char		buf[BUF_SIZE];
	MailRec *	mp;
	char *		cp;
	
	mp = CreateMailRec (DUMMY_FILE);
	(void)StatFile (DUMMY_FILE, &mp->inode, (off_t *)0);
	/* -N to mailx says don't give me initial headers */
	sprintf (buf, "C= exec %s -N -f %s", NTS_MAILX, mp->filename);
	FPRINTF ((stderr, "%s\n", buf));
	if (p3open (buf, mp->fp) != 0) {
		p3close (mp->fp);
		fprintf (stderr, "popen(%s) failed\n", buf);
		exit (1);
	}
	/* Strip off initial prompt */
	read (fileno(mp->fp[1]), buf, BUF_SIZE);
	sprintf (buf, NTS_SET_TEXT, PROMPT_TEXT);
	(void)ProcessCommand (mp, buf, NULL);
	/* Find out what version of mailx this is */
	GetVersion (ProcessCommand (mp, NTS_VERSION_CMD, NULL));
	/* This command allows us to terminate the mail */
	/* command with "\n.". */
	sprintf (buf, NTS_DEFAULTS, PrintCommand);
	(void)ProcessCommand (mp, SET_CMD, buf);

	return mp;
}

static Boolean
IsUnread (mp, i)
MailRec *mp;
int i;
{
	char **tmp;

	tmp = (char **)mp->summary->items[i].fields;
	if (tmp[F_TEXT][F_STATUS] == 'U') {
		return True;
	}
	return False;
}

static Boolean
IsNew (mp, i)
MailRec *mp;
int i;
{
	char **tmp;

	tmp = (char **)mp->summary->items[i].fields;
	if (tmp[F_TEXT][F_STATUS] == 'N') {
		return True;
	}
	return False;
}

int
GetDefaultItem (mp)
MailRec *	mp;
{
	int	i;

	/* Select the first New message.  If none, select the first unread
	 * message.  If none, select the first message.
	 */
	for (i=0; i<mp->summary->size; i++) {
		if (IsNew (mp, i) == True) {
			break;
		}
	}
	if (i == mp->summary->size) {
		for (i=0; i<mp->summary->size; i++) {
			if (IsUnread (mp, i) == True) {
				break;
			}
		}
	}
	if (i == mp->summary->size) {
		i = 0;
	}
	return i;
}

char *
GetErrorText (errnum, errmess, text, filename)
int	errnum;
char *	errmess;
char *	text;
char *	filename;
{
	char *		reason;
	static char	tmp[BUF_SIZE];

	if (errmess == NULL) {
		reason = GetTextGivenErrno (errnum);
	}
	else {
		reason = GetTextGivenText (errmess);
	}
	sprintf (tmp, GetGizmoText (text), filename, reason);
	return tmp;
}

static Boolean
ProcessFileCmd (MailRec *mp, BaseWindowGizmo *bw, char *buf, char *filename)
{
	char *		reason;
	static char *	regx = NULL;
	char		text[BUF_SIZE];
	char		errortext[BUF_SIZE];
	char *		ret;
	char *		readonly;
	char *		cp;
	int		i;

	/* Three things can come out of the file command:
 	 * The message "Your mail is being forwarded to <uid>",
	 * an error message, or a save message.
 	 * The error message will look something like:
 	 *
 	 *	/tmp/a/a/a/: No such file or directory
 	 *
 	 * The save message look something like:
 	 *
 	 *	"+mail.out": 145 messages 46 new 138 unread
 	 *
 	 * The fact that the error message has the filename
 	 * w/o quotes is used to differentiate it from the save
 	 * message.
	 *
	 * If we get the forwarding message it should be displayed.
	 * If we don't get an error message then the output
	 * from mailx is ignored.  This protects us from output
	 * like the following:
	 *
	 *	"/usr/davef/folder/mail.out" (Unexpected end-of-file).
	 *		(Unexpected end-of-file).
	 *		(Unexpected end-of-file).
	 *	updated.
 	 */

	readonly = NTS_READ_ONLY;
	i = sizeof (NTS_READ_ONLY)-1;
	if (Version == 41) {
		readonly = NTS_READ_ONLY_41;
		i = sizeof (NTS_READ_ONLY_41)-1;
	}
	if (strncmp (buf, readonly, i) == 0) {
		sprintf (text, GetGizmoText (CANT_READ_MF_OPEN));
		DisplayErrorPrompt (
			GetBaseWindowShell (bw), text
		);
		return False;
	}
	/*
	 * Check for the forwarding message
	 */
	if (strncmp (buf, NTS_FORWARD, sizeof (NTS_FORWARD)-1) == 0) {
		DisplayErrorPrompt (
			GetBaseWindowShell (bw),
			GetGizmoText (TXT_FORWARD)
		);
		return False;
	}
	/* An error is filename: <error message>
	 * If this happens display the error in the footer
	 * and return False.
	 * If the file is empty it is trapped below here.
	 * If the file doesn't exist it is also trapped before here.
	 */

	sprintf (text, "%s: ", filename);
	regx = (char *)regcmp (
		text,
		"(.*)$0",
		0
	);
	REGISTER_MALLOC (regx);
	ret = (char *)regex (regx, buf, errortext);
	FREE (regx);
	if (ret != NULL) {
		reason = GetErrorText (
			0, errortext, TXT_CANT_BE_OPENED, filename
		);
		DisplayErrorPrompt (GetBaseWindowShell (bw), reason);
		return False;
	}
	mp->noMail = False;
	if (strncmp (buf, NTS_NO_MAIL, sizeof (NTS_NO_MAIL)-1) == 0) {
		mp->noMail = True;
	}
	/* Look for ": 0 messages\n in the string.  This also means
	 * there is no mail.  But, only do this test if this isn't DummyDir.
	 */
	for (cp=buf; mp->inode!=DummyDir&&(cp=strchr (cp, '"'))!=NULL; cp++) {
		if (strncmp (cp, ZERO_MSGS, sizeof(ZERO_MSGS)) == 0 ||
		    strncmp (cp, ZERO_MSGS_RO, sizeof(ZERO_MSGS_RO)) == 0) {
			if (mp->size == 0) {
				mp->noMail = True;
				break;
			}
			/* If mailx says there are zero meesages, but
			 * the mailfile in nonzero in size then this isn't
			 * a mail file.
			 */
			sprintf (
				errortext,
				GetGizmoText (TXT_NOT_A_MAIL_FILE),
				filename
			);
			DisplayErrorPrompt (GetBaseWindowShell(bw), errortext);
			return False;
		}
	}
	FreeSummaryOrDeleteList (GetSummaryListWidget (mp->mng), mp->summary);
	FreeSummaryOrDeleteList (GetDeletedListWidget (mp->mng), mp->deleted);

	if (mp->noMail == True) {
		mp->summary->size = 0;
	}
	else {
		GetMailItems (mp);
	}

	return True;
}

/*
 * The following routine works around a bug in the 4.0 mailx.
 * You must use % for the file name because the file command
 * won't report the file as read only otherwise.
 */
static char *
Fix40Bug (filename)
char *	filename;
{
	if (Version == 40) {
		if (strcmp (GetUserMailFile (), filename) == 0) {
			return "%";
		}
	}
	return filename;
}

Boolean
SwitchMailx (mp, filename, bw)
MailRec *		mp;
char *			filename;
BaseWindowGizmo *	bw;
{
	char	buf[BUF_SIZE];
	o_ino_t	inode;

	/* We need to tell mailx to close this file and reopen it
	 * again.  This can be done with the "file" command on 
	 * the specified filename.
	 */
	inode = mp->inode;
	if (StatFile (filename, &mp->inode, &mp->size) == (time_t)0 &&
	    strcmp (filename, GetUserMailFile()) != 0) {
		sprintf (
			buf, GetGizmoText (TXT_FILE_DOESNT_EXIST), filename
		);
		DisplayErrorPrompt (GetBaseWindowShell (bw), buf);
	}
	else {
		if (ProcessFileCmd (
			mp, bw,
			ProcessCommand (mp, NTS_FILE_CMD, Fix40Bug (filename)),
			filename
		) == True) {
			FREE (mp->filename);
			mp->filename = STRDUP (filename);
			return True;
		}
	}
	mp->inode = inode;
	return False;
}

MailRec *
FindMailRec (wid)
Widget	wid;
{
	ReadRec *	rp;
	ManageRec *	mng;

	/* First look at the base windows and see if any of these are
	 * the shell of wid.
	 */
	if ((rp = FindReadRec (wid)) != (ReadRec *)0) {
		return rp->mp;
	}
	if ((mng = FindManageRec (wid)) != (ManageRec *)0) {
		return mng->mp;
	}
	if (FindSendRec (wid) != (SendRec *)0) {
		return mailRec;
	}
	/*
	 * Otherwise, simply return mailRec
	 */
	return mailRec;
}
