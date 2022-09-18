/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:send.c	1.69"
#endif

#define SEND_C

#define FIX_TEW

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Vendor.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <Xol/OlCursors.h>	/* For OlGetBusyCursor */
#include <Shell.h>
#include "mail.h"
#include <Gizmo/MenuGizmo.h>
#include "SendGizmo.h"

#define MAIL_CMD	"m\n"
#define SIGN_CMD	"m\n~a\n~x"
#define REPLY_ALL_CMD	"r"
#define REPLY_CMD	"R"
#define SET_CRT_PAGER	"set crt= PAGER=\"cat > %s\""
#define UNSET_CRT_PAGER	"unset crt PAGER"
#define TILDE_P_X	"~p\n~x"
#define TILDE_X		"~x"
#define TILDE_Q		"~Q"
#define EOT		"EOT\n"
#define DOT		"~."
#define PREFIX		"> "

extern char *		PrintCommand;
extern BaseWindowGizmo	MainWindow;
extern char *		PrintCmd;
extern int		LastSelectedMessage;
extern MailRec *	LastMailRec;
extern int		errno;
extern char *		Mprefix;
extern char *		Signature;
extern SendRec *	sendRec;
extern MailRec *	mailRec;
extern Widget		Root;
extern Boolean		SendOnly;

typedef enum {
	NoError, ErrorDisplayed, ErrorNotDisplayed
} ErrorTypes;

typedef enum {
	MenuNew, MenuOpen, MenuInclude, MenuSave,
	MenuSaveAs, MenuPrint, MenuProperties, MenuExit
} SendFileItemIndex;

typedef enum {
	MenuUndo, MenuCut, MenuCopy, MenuPaste,
	MenuDelete, MenuSelect, MenuUnselect
} SendEditItemIndex;

typedef enum {
	MenuSend, MenuAliases
} SendMailItemIndex;

typedef enum {
	MenuFile, MenuEdit, MenuMail, MenuHelp
} SendMenuItemIndex;

char *			SendName = SEND;
static void		DoInclude();

SendRec *
CreateSendRec ()
{
	SendRec *	sp;
	Widget		shell;

	sp = AddToSendList ();
	InitOriginalText (sp);
	sp->saveFilename = NULL;
	sp->used = True;
	sp->exitPending = False;
	sp->saveAsPopup = (FileGizmo *)0;
	sp->openPopup = (FileGizmo *)0;
	MapGizmo (BaseWindowGizmoClass, sp->baseWGizmo);
	/* The following is needed if the send window is 
	 * being reused.
	 */
	XtMapWidget (shell = GetBaseWindowShell (sp->baseWGizmo));
	XRaiseWindow (XtDisplay(shell), XtWindow (shell));

	return sp;
}

void
ManageSend ()
{
	SendRec *	sp;
	SendGizmo *	gizmo;
	Widget		shell;
	static char *	outgoing = NULL;

	if (outgoing == NULL) {
		outgoing = GetGizmoText (TXT_OUT_GOING);
	}
	sp = CreateSendRec ();
	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);

	SetBaseWindowTitle (sp->baseWGizmo, outgoing);
	SetSendTextAndHeader (gizmo, NULL, NULL, NULL, NULL, Signature);
	shell = GetSendTextWidget (gizmo);
	if (OlCanAcceptFocus(shell, CurrentTime) != False) {
		OlSetInputFocus (shell, RevertToParent, CurrentTime);
	}
}

Widget
GetToplevelShell (wid)
Widget	wid;
{
	Widget		shell;

	for (shell=wid; shell!=(Widget)0; shell=XtParent(shell)) {
		if (XtIsSubclass(shell, topLevelShellWidgetClass) == True) {
			break;
		}
	}
	return shell;
}

SendRec *
FindSendRec (wid)
Widget wid;
{
	SendRec *	sp;
	Widget		shell;

	shell = GetToplevelShell (wid);
	for (sp=sendRec; sp!=(SendRec *)0; sp=sp->next) {
		if (shell == GetBaseWindowShell (sp->baseWGizmo)) {
			break;
		}
		if (wid == sp->baseWGizmo->icon_shell) {
			break;
		}
	}
	return sp;
}

/* After sending a mail message EOT should appear after
 * mailx sees the DOT.  
 */

static Boolean
CheckForEOT (sp, buf)
SendRec *	sp;
char *		buf;
{
	char *cp;
	int i;

	/* First look for the EOT response from mailx */

	i = strlen (EOT);
	if (strncmp (EOT, buf, i) != 0) {
		FPRINTF ((stderr, "Didn't encounter EOT\n"));
		return False;
	}

	cp = buf + i;	/* Point past EOT */

	/*
	 * I anything else is after the EOT it is most likely
	 * an error message - display the errors and return false.
	 */
	if (cp[0] != '\0') {
		CreateTransErrorModal (
			GetBaseWindowShell (sp->baseWGizmo), cp
		);
		return False;
	}
	return True;
}

/* Output one tilda command for each line of text followed
 * by the line of text.
 */

static void
OutputTildeAndText (mp, tilde, text)
MailRec *mp;
char *tilde;
char *text;
{
	char *	buf = text;
	char *	cp;
	char *	savept = NULL;

	if (text == NULL) {
		return;
	}
	while ((cp = MyStrtok (buf, "\n", &savept)) != NULL) {
		buf = NULL;
		WriteToMailx (mp, tilde, strlen (tilde));
		WriteToMailx (mp, cp, strlen (cp));
		WriteToMailx (mp, "\n", 1);
	}
}

static void
OutputMailText (mp, wid, subject, to, cc, bcc, text, insertCR)
MailRec *	mp;
Widget		wid;
char *		text;
char *		subject;
char *		to;
char *		cc;
char *		bcc;
Boolean		insertCR;	/* Insert crs at textual wrap points if TRUE */
{
	char *	cp;
	int	i;

	WriteToMailx (mp, MAIL_CMD, strlen (MAIL_CMD));
	cp = ExpandAlias(to);
	OutputTildeAndText (mp, "~t ", cp);
	FREE (cp);
	OutputTildeAndText (mp, "~s ", subject);
	cp = ExpandAlias(cc);
	OutputTildeAndText (mp, "~c ", cp);
	FREE (cp);
	cp = ExpandAlias(bcc);
	OutputTildeAndText (mp, "~b ", cp);
	FREE (cp);

	if (insertCR == True) {
		cp = NULL;
		for (i=0; (cp = OlGetWrappedLine (wid, i))!=NULL; i++) {
			if (cp[0] == '~') {
				WriteToMailx (mp, "~", 1);
			}
			WriteToMailx (mp, cp, strlen (cp));
			WriteToMailx (mp, "\n", 1);
		}
	}
	else {
		i = strlen (text);
		WriteToMailx (mp, text, i);
		if (i > 0 && text[i-1] != '\n') {
			WriteToMailx (mp, "\n", 1);
		}
	}
}

static Boolean
TextDiffers (s1, s2)
char *	s1;
char *	s2;
{
	if (s1 == s2) return False;
	if (s1 == NULL && s1 != NULL) return True;
	if (s1 != NULL && s1 == NULL) return True;
	if (strcmp (s1, s2) == 0) return False;
	return True;
}

Boolean
TextChanged (sp)
SendRec *	sp;
{
	char *		text;
	char *		subject;
	char *		to;
	char *		cc;
	char *		bcc;
	SendGizmo *	gizmo;

	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	GetSendText (gizmo, &subject, &to, &cc, &bcc, &text);
	if (TextDiffers (text,		sp->origText)    == True ||
	    TextDiffers (to,		sp->origTo)      == True ||
	    TextDiffers (subject,	sp->origSubject) == True ||
	    TextDiffers (cc,		sp->origCc)      == True ||
	    TextDiffers (bcc,		sp->origBcc)     == True) {
		return True;
	}
	FREENULL (subject);
	FREENULL (to);
	FREENULL (cc);
	FREENULL (bcc);
	FREENULL (text);
	return False;
}

static void
CheckAddress (address, buf)
char *	address;
char *	buf;
{
	char *	sp;
	char *	cp;
	char *	cc;
	char *	addr;

	if (address == NULL || address[0] == '\0') {
		return;
	}
	addr = STRDUP (address);
	/* First remove any comments */
	while ((cp = strrchr (addr, '(')) != NULL) {
		if (cp != NULL) {
			sp = strchr (cp, ')');
			if (sp == NULL) {
				*cp = NULL;
			}
			else {
				strcpy (cp, sp+1);
			}
		}
	}
	/*
	 * If the addr has a "<" in it then the real address
	 * is within <>.
	 */
	if ((cp = strchr (addr, '<')) != NULL) {
		if ((sp = strchr (cp, '>')) != NULL) {
			*sp = '\0';
		}
		strcpy (addr, cp+1);
	}
	sp = addr;
	while ((cc = strtok (sp, " ,")) != NULL) {
		if (cc[0] == '-') {
			strcat (buf, cc);
			strcat (buf, "\n");
			FPRINTF ((stderr, "invalid address - %s\n", cc));
		}
		sp = NULL;
	}
	FREE (addr);
}

static Boolean
CheckForInvalidAddresses (sp, to, cc, bcc)
SendRec *	sp;
char *		to;
char *		cc;
char *		bcc;
{
	char	buf[BUF_SIZE];
	char	invalid[BUF_SIZE];

	invalid[0] = '\0';
	CheckAddress (to, invalid);
	CheckAddress (cc, invalid);
	CheckAddress (bcc, invalid);

	if (invalid[0] != '\0') {
		strcpy (buf, GetGizmoText (TXT_INVALID_ADDRESS));
		strcat (buf, "\n\n");
		strcat (buf, invalid);
		CreateInvalidAddressModal (
			GetBaseWindowShell (sp->baseWGizmo), buf
		);
		return False;
	}
	return True;
}

static void
SendSend (sp)
SendRec *	sp;
{
	static char *	sent = NULL;
	char		buf[BUF_SIZE];
	char *		text;
	char *		subject;
	char *		to;
	char *		cc;
	char *		bcc;
	int		size;
	MailRec *	mp;
	SendGizmo *	gizmo;

	if (sent == NULL) {
		sent = GetGizmoText (TXT_MESSAGE_SENT);
	}

	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	GetSendText (gizmo, &subject, &to, &cc, &bcc, &text);

	mp = mailRec;

	/* Combine ~s subject, ~c cc, ~b bcc, and */
	/* text. */

	if (CheckForInvalidAddresses (sp, to, cc, bcc) == False) {
		return;
	}
	OutputMailText (
		mp, GetSendTextWidget (gizmo), subject, to, cc, bcc, text, True
	);
	if (CheckForEOT (sp, ProcessCommand (mp, DOT, NULL)) == True) {
		/* Indicate mail was sent */
		FREENULL (sp->origText);
		FREENULL (sp->origSubject);
		FREENULL (sp->origCc);
		FREENULL (sp->origTo);
		FREENULL (sp->origBcc);

		sp->origText = text;
		sp->origTo = to;
		sp->origSubject = subject;
		sp->origCc = cc;
		sp->origBcc = bcc;

		DisplayInLeftFooter (sp->baseWGizmo, sent, True);
	}
}

void
SendAgainCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);

	CancelCB (wid, client_data, call_data);
	SendSend (sp);
}

static void
SendIt (sp)
SendRec *	sp;
{
	if (TextChanged (sp) == False) {
		CreateNoChangeModal (GetBaseWindowShell (sp->baseWGizmo));
		return;
	}
	SendSend (sp);
}

void
LeaveSend (sp)
SendRec *	sp;
{

	UnmapShell (GetBaseWindowShell (sp->baseWGizmo));
	MarkUnused (sp);
}

void
ReallyExitCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);

	XtPopdown ((Widget)_OlGetShellOfWidget (wid));

	LeaveSend (sp);
}

void
NewCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);

	XtPopdown ((Widget)_OlGetShellOfWidget (wid));
	if (sp->saveFilename != NULL) {
		FREE (sp->saveFilename);
		sp->saveFilename = NULL;
		SetFileCriteria (
			sp->saveAsPopup, GetCurrentDirectory(), NULL
		);
	}
	InitOriginalText (sp);
}

static void
SendNew (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	Widget		shell;
	SendRec *	sp = FindSendRec (wid);

	shell = GetBaseWindowShell (sp->baseWGizmo);
	if (TextChanged (sp) == True) {
		CreateNewSendModal(shell);
		return;
	}
	NewCB (wid, client_data, call_data);
}

void
SendExitCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);
	Widget		shell;

	shell = GetBaseWindowShell (sp->baseWGizmo);
	if (TextChanged (sp) == True) {
		CreateTextChangedModal(shell);
		return;
	}
	LeaveSend (sp);
}

static char *
GetSubject (buf)
char *buf;
{
	static char *regx = NULL;
	static char subject[BUF_SIZE];
	char *cp;

	if (regx == NULL) {
		regx = (char *)regcmp ("[\n]+Subject: ([^\n]+)$0", 0);
	}
	cp = (char *)regex (regx, buf, subject);
	if (cp != NULL) {
		return STRDUP (subject);
	}
	return NULL;
}

static char *
GetCc (buf, c)
char *buf;
char c;
{
	static char *	regxcc = NULL;
	static char *	regxto;
	static char *	regxbcc;
	char		cc[BUF_SIZE];
	char *		store;
	char *		text = NULL;
	char *		cp;
	char *		tmp;
	char *		savept = NULL;
	long		n;

	if (regxcc == NULL) {
		regxto = (char *)regcmp ("To:(.*)$0", 0);
		regxbcc = (char *)regcmp ("[\n]+Bcc:(.*)$0", 0);
		regxcc = (char *)regcmp ("[\n]+Cc:(.*)$0", 0);
	}
	store = cc;
	n = strlen (buf);
	/*
	 * Get at least enough space to store all of buf.
	 * We need to do this because a cc list could be
	 * almost as big as buf.
	 */
	if (n > sizeof(cc)-1) {
		store = MALLOC (n);
	}
	switch (c) {
		case 'c': cp = (char *)regex (regxcc, buf, store); break;
		case 't': cp = (char *)regex (regxto, buf, store); break;
		case 'b': cp = (char *)regex (regxbcc, buf, store); break;
	}
	if (cp != NULL) {
		text = CALLOC (1, sizeof(char));
		tmp = store;
		while ((cp = MyStrtok (tmp, "\n", &savept)) != NULL) {
			tmp = NULL;
			if (cp[0] != ' ') {
				break;
			}
			for (; *cp==' '; cp++);
			text = (char *)REALLOC (
				text, strlen(text)+strlen(cp)+2
			);
			strcat (text, cp);
			strcat (text, " ");
		}
	}
	if (store != cc) {
		FREE (store);
	}
	return text;
}

/* Get To:, Cc: and Subject from buf.
 * Put these values into the send gizmo.
 */
static SendRec *
ReplyToSender (sp, mp, pcbuf, message)
SendRec *	sp;
MailRec *	mp;
char *		pcbuf;		/* Buffer from ProcessCommand */
int		message;
{
	char *		subject;
	char *		cc;
	char *		to;
	SendGizmo *	gizmo;
	Widget		shell;
	char		text[BUF_SIZE];
	char *		buf;

	/*
	 * The buffer returned from ProcessCommand needs to be copied,
	 * otherwise, the next call to ProcessCommand will wipe out
	 * its value.
	 */
	buf = STRDUP (pcbuf);
	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	/*
	 * Get who the message was from
	 * and put this name in the title bar
	 */
	sprintf (text, "%s %d\n%s", REPLY_CMD, message, TILDE_X);
	cc = ProcessCommand (mp, text, NULL);
	to = GetCc (cc, 't');
	sprintf (text, GetGizmoText (TXT_REPLY_TO), to);
	SetBaseWindowTitle (sp->baseWGizmo, text);
	FREE (to);

	subject = GetSubject (buf);
	cc = GetCc (buf, 'c');
	to = GetCc (buf, 't');

	SetSendTextAndHeader (gizmo, subject, to, cc, NULL, Signature);
	shell = GetSendTextWidget (gizmo);
	if (OlCanAcceptFocus(shell, CurrentTime) != False) {
		OlSetInputFocus (shell, RevertToParent, CurrentTime);
	}
	FREENULL (subject);
	FREENULL (to);
	FREENULL (cc);
	FREENULL (buf);
	return sp;
}

static Boolean
OnlyTabOrSpace (cp)
char *	cp;
{
	if (cp != NULL) {
		for (; *cp; cp++) {
			if (*cp != ' ' || *cp != '\t') {
				return False;
			}
		}
	}
	return True;
}

static Boolean
SignEqualsText (text)
char *	text;
{
	Boolean	equal = False;

	if ((text != NULL && text[0] != '\0') &&
	    (Signature != NULL && Signature[0] != '\0')) {
		if (strcmp (Signature, text) == 0) {
			equal = True;
		}
	}
	else if ((text == NULL || text[0] == '\0') &&
	    (Signature == NULL || Signature[0] == '\0')) {
		equal = True;
	}
	return equal;
}

/*
 * Two things can happen when a drop occurs on a send window:
 * 1. If the window is unmodified, i.e., signature only, then
 * it is equivalent of doing a reply to sender with att.
 * 2. If modifications occurred to the text then an include
 * will be performed.
 */
void
SendDropReply (sp, wid)
SendRec *	sp;
Widget		wid;
{
	MailRec *	mp = FindMailRec (wid);
	char		cmd[BUF_SIZE];
	int		message = LastSelectedMessage;
	char *		text;
	char *		subject;
	char *		to;
	char *		cc;
	char *		bcc;
	Widget		shell;
	SendGizmo *	gizmo;
	extern void	Include ();
	extern void	AttachMessage ();

	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	GetSendText (gizmo, &subject, &to, &cc, &bcc, &text);

	if (SignEqualsText (text) == True &&
	    OnlyTabOrSpace (subject) == True &&
	    OnlyTabOrSpace (to) == True      &&
	    OnlyTabOrSpace (cc) == True      &&
	    OnlyTabOrSpace (bcc) == True     ) {
		/* Do a respond */
		/* First, respond to the selected message and collect
		 * the subject, to and cc lines.  Put these into
		 * the send window.
		 * Then terminate the respond with ~x.
		 */
		sprintf (cmd, "%s %d\n%s", REPLY_CMD, message, TILDE_X);
		ReplyToSender (
			sp, mp, ProcessCommand (mp, cmd, NULL), message
		);
		AttachMessage (mp, sp, message);
	}
	else {
		/* Note: TextChanged isn't used here because we don't
		 * want to do this operation if the sender contains
		 * any text at all.
		 */
		/* Do an include */
		LastMailRec = mp;
		Include (sp);
	}
	MapGizmo (BaseWindowGizmoClass, sp->baseWGizmo);
	shell = GetBaseWindowShell (sp->baseWGizmo);
	XRaiseWindow (XtDisplay (shell), XtWindow (shell));
	FREENULL (subject);
	FREENULL (to);
	FREENULL (cc);
	FREENULL (bcc);
	FREENULL (text);
}

static SendRec *
ReadReply (Widget wid, Boolean all)
{
	SendRec *	sp;
	MailRec *	mp = FindMailRec (wid);
	ReadRec *	rp = FindReadRec (wid);
	char		cmd[BUF_SIZE];

	/* First, respond to the selected message and colect
	 * the subject, to and cc lines.  Put these into
	 * the send window.
	 * Then terminate the respond with ~x.
	 */
	sprintf (
		cmd, "%s %d\n%s",
		(all == True) ? REPLY_ALL_CMD : REPLY_CMD,
		rp->message, TILDE_X
	);
	sp = CreateSendRec ();
	ReplyToSender (sp, mp, ProcessCommand (mp, cmd, NULL), rp->message);
	return sp;
}

static void
AttachMessage (mp, sp, message)
MailRec *	mp;
SendRec *	sp;
int		message;
{
	char	cmd[BUF_SIZE];

	if (sp == (SendRec *)0) {
		return;
	}
	/* Get the message to be included */
	sprintf (cmd, "%s %d", PrintCmd, message);
	DoInclude (sp, ProcessCommand (mp, cmd, NULL));
}

void
ReadReplyAllAttCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp;
	MailRec *	mp = FindMailRec (wid);

	sp = ReadReply (wid, True);
	AttachMessage (mp, sp, mp->rp->message);
}

void
ReadReplySenderAttCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *       sp;
	MailRec *       mp = FindMailRec (wid);

	sp = ReadReply (wid, False);
	AttachMessage (mp, sp, mp->rp->message);
}

void
ReadReplyAllCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	(void)ReadReply (wid, True);
}

void
ReadReplySenderCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	(void)ReadReply (wid, False);
}

static SendRec *
Reply (Widget wid, Boolean all, int *message)
{
	MailRec *	mp = FindMailRec (wid);
	SendRec *	sp = (SendRec *)0;
	int		i;
	char		cmd[BUF_SIZE];

	for (i=0; i<mp->summary->size; i++) {
		if (mp->summary->items[i].set == True) {
			/* First, respond to the selected message and colect
			 * the subject, to and cc lines.  Put these into
			 * the send window.
			 * Then terminate the respond with ~x.
			 */
			sprintf (
				cmd, "%s %d\n%s",
				(all == True) ? REPLY_ALL_CMD : REPLY_CMD,
				mp->summary->items[i].clientData,
				TILDE_X
			);
			*message = MessageNumber (mp, i);
			sp = CreateSendRec ();
			(void) ReplyToSender (
				sp, mp, ProcessCommand (mp, cmd, NULL),
				*message
			);
		}
	}
	return sp;
}

void
ReplyAllAtt (wid)
Widget wid;
{
	SendRec *	sp;
	MailRec *       mp = FindMailRec (wid);
	int		message;
	
	sp = Reply (wid, True, &message);
	AttachMessage (mp, sp, message);
}

void
ReplySenderAttProc (wid)
Widget wid;
{
	SendRec *	sp;
	MailRec *       mp = FindMailRec (wid);
	int		message;
	
	sp = Reply (wid, False, &message);
	AttachMessage (mp, sp, message);
}

void
ReplyAll (wid)
Widget wid;
{
	int	dummy;

	(void)Reply (wid, True, &dummy);
}

void
ReplySenderProc (wid)
Widget wid;
{
	int	dummy;

	(void)Reply (wid, False, &dummy);
}

static char *
InsertPrefix (text)
char *	text;
{
	int	cr = 0;
	int	size;
	char *	cp;
	char *	tp;
	char *	buf;
	char	last;

	/*
	 * First find out how many prefixes to insert into the text
	 */

	for (size=0, cp=text; *cp!='\0'; size++, cp++) {
		if (*cp == '\n') {
			cr += 1;
		}
	}
	if (*(cp-1) != '\n') {
		cr += 1;
	}

	if (Mprefix == NULL) {
		Mprefix = PREFIX;
	}
	buf = MALLOC (strlen(Mprefix) * cr + size + 1);

	/*
	 * Insert the prefix before every line
	 */
	last = '\n';
	size = strlen (Mprefix);
	for (tp=buf, cp=text; *cp!='\0'; tp++, cp++) {
		if (last == '\n') {
			strcpy (tp, Mprefix);
			tp += size;
		}
		*tp = *cp;
		last = *cp;
	}
	*tp = '\0';
	return buf;
}

static void
RealForward (mp, message)
MailRec *	mp;
int		message;
{
	SendGizmo *	gizmo;
	SendRec *	sp;
	char *		buf;
	char *		text;
	Widget		w;
	unsigned int	i;
	Widget		shell;
	char		cmd[BUF_SIZE];
	char *		to;

	sp = CreateSendRec ();
	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	/*
	 * Get who the message was from
	 * and put this name in the title bar
	 */
	sprintf (cmd, "%s %d\n%s", REPLY_CMD, message, TILDE_X);
	buf = ProcessCommand (mp, cmd, NULL);

	/*
	 * Change title on base window
	 */
	text = GetCc (buf, 't');
	sprintf (cmd, GetGizmoText (TXT_FORWARD_FROM), text);
	FREE (text);
	SetBaseWindowTitle (sp->baseWGizmo, cmd);

	/* Get the message to be included */
	sprintf (cmd, "%s %d", PrintCmd, message);
	buf = ProcessCommand (mp, cmd, NULL);
	text = strchr (buf, '\n') + 1;	/* Skip over "message #:" */
	text = InsertPrefix (text);

	SetSendTextAndHeader (gizmo, NULL, NULL, NULL, NULL, text);
	w = GetSendTextWidget (gizmo);
	OlTextEditGetLastPosition (w, &i);
	OlTextEditSetCursorPosition (w, i, i, i);
	OlTextEditInsert (w, Signature, strlen (Signature));
	shell = GetSendTextWidget (gizmo);
	if (OlCanAcceptFocus(shell, CurrentTime) != False) {
		OlSetInputFocus (shell, RevertToParent, CurrentTime);
	}
	FREE (text);
}

void
Forward (mp)
MailRec *	mp;
{
	int		i;

	for (i=0; i<mp->summary->size; i++) {
		if (mp->summary->items[i].set == True) {
			/*
			 * Gather up the contents of this mail message
			 * and put it into the send window.
			 */
			RealForward (mp, MessageNumber (mp, i));
		}
	}
}

void
ReadForwardCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	MailRec *	mp = FindMailRec (wid);
	ReadRec *	rp = FindReadRec (wid);

	RealForward (mp, rp->message);
}

void
DeleteSendRec (sp)
SendRec *	sp;
{
	SendRec *	tp;
	SendRec *	last = (SendRec *)0;

	if (sp == (SendRec *)0) {
		return;
	}
	for (tp=sendRec; tp!=(SendRec *)0; tp=tp->next) {
		if (sp == tp) {
			if (last != (SendRec *)0) {
				last->next = sp->next;
			}
			else {
				sendRec = sp->next;
			}
			break;
		}
		last = tp;
	}
	XtDestroyWidget (GetBaseWindowShell (sp->baseWGizmo));
	FreeGizmo (BaseWindowGizmoClass, sp->baseWGizmo);
	FREE (sp);
}

static void
SendPrint (sp)
SendRec *	sp;
{
	MailRec *	mp = mailRec;
	char *		text;
	char *		subject;
	char *		to;
	char *		cc;
	char *		bcc;
	SendGizmo *	gizmo;
	FILE *		fd;
	char *		tmpfile;
	char		cmd[BUF_SIZE];

	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);

	tmpfile = tmpnam(0);
	if ((fd = fopen (tmpfile, "w")) == NULL) {
		SetBaseWindowMessage (
			sp->baseWGizmo, GetGizmoText (TXT_CANT_OPEN_4_PRINT)
		);
		return;
	}
	fclose (fd);

	/* 
	 * By setting the crt variable to 0 and the PAGER
	 * variable to cat > tmpfile, a ~p will write the
	 * composed message to the tmpfile.
	 */

	sprintf (cmd, SET_CRT_PAGER, tmpfile);
	(void)ProcessCommand (mp, cmd, NULL);

	/* Get the subject, cc, text, etc from the send gizmo
	 * and output it to mailx and print it using ~p
	 */
	GetSendText (gizmo, &subject, &to, &cc, &bcc, &text);

	OutputMailText (
		mp, GetSendTextWidget(gizmo), subject, to, cc, bcc, text, False
	);
	FREENULL (subject);
	FREENULL (to);
	FREENULL (cc);
	FREENULL (bcc);
	FREENULL (text);
	/* Send the message to the printer */
	(void)ProcessCommand (mp, TILDE_P_X, NULL);

	/* Turn off crt and PAGER */
	(void)ProcessCommand (mp, UNSET_CRT_PAGER, NULL);

	while (XtPending ()) {
		XEvent event;

		XtNextEvent (&event);
		XtDispatchEvent (&event);
	}

	sprintf (cmd, "cat %s | %s", tmpfile, PrintCommand);
	if (system (cmd) == 0) {
		DisplayInLeftFooter (
			sp->baseWGizmo, GetGizmoText (TXT_SEND_PRINTED), True
		);
	}
	else {
		DisplayInLeftFooter (
			sp->baseWGizmo, GetGizmoText (TXT_PRINT_FAILED), True
		);
	}
	unlink (tmpfile);
}

static void
SaveMessage (sp, filename, saveas)
SendRec *	sp;
char *		filename;
Boolean		saveas;
{
	int		size;
	MailRec *	mp = mailRec;
	o_ino_t		inode;
	FILE *		fp;
	char *		cp;
	char		buf[BUF_SIZE];
	char *		subject;
	char *		cc;
	char *		to;
	char *		bcc;
	char *		text;
	Widget		shell;
	SendGizmo *	gizmo;

	shell = GetBaseWindowShell (sp->baseWGizmo);
	if (saveas == True &&
	    StatFile (sp->saveFilename, &inode, (off_t *)0) != (time_t)0) {
		CreateFileExistsModal (shell);
		return;
	}
	FPRINTF ((stderr, "Write %s\n", sp->saveFilename));
	if ((fp = fopen (sp->saveFilename, "w")) == NULL) {
		cp = GetErrorText (
			errno, NULL, TXT_CANT_SAVE_FILE, sp->saveFilename
		);
		DisplayErrorPrompt (shell, cp);
		fclose (fp);
		return;
	}
	BringDownPopup (GetFileGizmoShell (sp->saveAsPopup));

	/*
	 * Get the text from the send gizmo and save it into the file.
	 */
	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	GetSendText (gizmo, &subject, &to, &cc, &bcc, &text);
	fprintf (fp, "To: %s\n", to);
	fprintf (fp, "Subject: %s\n", subject);
	fprintf (fp, "Cc: %s\n", cc);
	fprintf (fp, "Bcc: %s\n\n", bcc);
	size = strlen (text);
	if (size != 0) {
		fputs (text, fp);
		if (text[size-1] != '\n') {
			fputc ('\n', fp);
		}
	}
	fclose (fp);

	FREENULL (subject);
	FREENULL (to);
	FREENULL (cc);
	FREENULL (bcc);
	FREENULL (text);

	sprintf (buf, GetGizmoText (TXT_SAVED_IN), sp->saveFilename);
	DisplayInLeftFooter (sp->baseWGizmo, buf, True);
}

void
RemoveFileCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);
	char *		cp;
	SendGizmo *	gizmo;

	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);

	BringDownPopup (_OlGetShellOfWidget (wid));

	if (unlink (sp->saveFilename) != 0) {
		cp = GetErrorText (
			errno, NULL, TXT_CANT_SAVE_FILE, sp->saveFilename
		);
		DisplayErrorPrompt (GetSendTextWidget (gizmo), cp);
		return;
	}
	SaveMessage (sp, sp->saveFilename, False);
}

static void
SaveReply (sp)
SendRec *	sp;
{
	/* This routine can only be called if sp->saveFilename
	 * is non NULL, so don't test for its existance.
	 */
	SaveMessage (sp, sp->saveFilename, False);
}

void
SaveSaveReplyAsCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);
	int		n;
	int		flag;

	if (sp->saveFilename != NULL) {
		FREE (sp->saveFilename);
	}
	n = ExpandFileGizmoFilename (sp->saveAsPopup, &flag);
	if ((flag != 0 && n == 1) || n == 0) {
		sp->saveFilename = GetFilePath (sp->saveAsPopup);
	}
	else {
		return;
	}
	SaveMessage (sp, sp->saveFilename, True);
}

static void
SaveReplyAs (sp)
SendRec *	sp;
{
	if (sp->saveAsPopup == (FileGizmo *)0) {
		CreateSaveReplyAsPopup (sp);
	}
	MapGizmo (FileGizmoClass, sp->saveAsPopup);
}

static char *
GetContents (to, n, text)
char *	to;
int	n;
char **	text;
{
	char *	cp;

	cp = *text;
	if (strncmp (cp, to, n) != 0) {
		return NULL;
	}
	*text = strchr (cp, '\n');
	if (*text == NULL) {
		return NULL;
	}
	cp = cp + n + 1;
	**text = '\0';
	*text += 1;
	return cp;
}

static ErrorTypes
DisplaySavedText (sp, shell, filename)
SendRec *	sp;
Widget		shell;
char *		filename;
{
	SendGizmo *	gizmo;
	ErrorTypes	errorType = ErrorNotDisplayed;
	char *		subject;
	char *		to;
	char *		cc;
	char *		bcc;
	char *		text;
	char *		cp;
	char		buf[BUF_SIZE];

	if ((text = ReadFile (shell, filename)) == NULL) {
		return ErrorDisplayed;
	}
	
	gizmo = (SendGizmo *) QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	cp = text;
	if (
	    (to =	GetContents ("To:",	3, &cp)) != NULL &&
	    (subject =	GetContents ("Subject:",8, &cp)) != NULL &&
	    (cc =	GetContents ("Cc:",	3, &cp)) != NULL &&
	    (bcc =	GetContents ("Bcc:",	4, &cp)) != NULL
	) {
		if (cp[0] == '\n') {
			errorType = NoError;
			cp += 1;
		}
	}

	if (errorType == NoError) {
		SetSendTextAndHeader (gizmo, subject, to, cc, bcc, cp);
	}
	MapGizmo (BaseWindowGizmoClass, sp->baseWGizmo);
	XRaiseWindow (XtDisplay(shell), XtWindow (shell));
	FREE (text);
	return errorType;
}

void
SendOpenCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	SendRec *	sp = FindSendRec (wid);
	MailRec *	mp = mailRec;
	o_ino_t		inode;
	Widget		shell;
	char *		filename;
	char *		tmp;
	char		buf[BUF_SIZE];
	int		n;
	int		flag;

	n = ExpandFileGizmoFilename(sp->openPopup, &flag);
	if (flag && n == 1) {
		filename = GetFilePath (sp->openPopup);
		SetFileGizmoMessage (sp->openPopup, " ");
	}
	else {
		SetFileGizmoMessage (
			sp->openPopup, GetGizmoText (TXT_INVALID_SELECTION)
		);
		return;
	}

	shell = GetBaseWindowShell (sp->baseWGizmo);

	if (DisplaySavedText(sp, shell, filename)  == ErrorNotDisplayed) {
		sprintf (buf, GetGizmoText (TXT_NOT_A_SAVE_FILE), filename);
		DisplayErrorPrompt (shell, buf);
		FREE (filename);
		return;
	}
	BringDownPopup (GetFileGizmoShell (sp->openPopup));
	FREE (filename);
}

static void
Open (sp)
SendRec *	sp;
{
	if (sp->openPopup == (FileGizmo *)0) {
		CreateSendOpenPopup (sp);
	}
	MapGizmo (FileGizmoClass, sp->openPopup);
}

static void
DoInclude (sp, buf)
SendRec *	sp;
char *		buf;
{
	SendGizmo *	gizmo;
	char *		text;

	text = strchr (buf, '\n') + 1;	/* Skip over "message #:" */
	text = InsertPrefix (text);

	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	OlTextEditInsert (GetSendTextWidget (gizmo), text, strlen (text));
	FREE (text);
}

static void
Include (sp)
SendRec *	sp;
{
	char		cmd[BUF_SIZE];

	/* Get the message to be included */
	sprintf (cmd, "%s %d", PrintCmd, LastSelectedMessage);
	DoInclude (sp, ProcessCommand (LastMailRec, cmd, NULL));
}

void
SendFileCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	SendRec *		sp = FindSendRec (wid);

	DisplayInLeftFooter (sp->baseWGizmo, "", True);
	switch (p->item_index) {
		case MenuNew: {
			SendNew (wid, client_data, call_data);
			break;
		}
		case MenuOpen: {
			Open (sp);
			break;
		}
		case MenuInclude: {
			Include (sp);
			break;
		}
		case MenuSave: {
			SaveReply (sp);
			break;
		}
		case MenuSaveAs: {
			SaveReplyAs (sp);
			break;
		}
		case MenuPrint: {
			SendPrint (sp);
			break;
		}
		case MenuProperties: {
			SendPropPopupCB (wid, client_data, call_data);
			break;
		}
		case MenuExit: {
			SendExitCB (wid, client_data, call_data);
			break;
		}
	}
}

void
SendEditCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	SendRec *		sp = FindSendRec (wid);
	Widget			w;
	SendGizmo *		gizmo;

	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	w = GetSendTextWidget (gizmo);

	DisplayInLeftFooter (sp->baseWGizmo, "", True);
	switch (p->item_index) {
		case MenuUndo: {
			OlActivateWidget(w, OL_UNDO, NULL);
			break;
		}
		case MenuCut: {
#ifdef FIX_TEW
			OlActivateWidget(w, OL_CUT, NULL);
#else
			(void) OlTextEditCopySelection(w, True);
#endif
			break;
		}
		case MenuCopy: {
#ifdef FIX_TEW
			OlActivateWidget(w, OL_COPY, NULL);
#else
			(void) OlTextEditCopySelection(w, False);
#endif
			break;
		}
		case MenuPaste: {
			OlActivateWidget(w, OL_PASTE, NULL);
			break;
		}
		case MenuDelete: {
			OlTextEditInsert(w, "", 0);
			break;
		}
		case MenuSelect: {
			OlActivateWidget(w, OLM_KSelectAll, NULL);
			break;
		}
		case MenuUnselect: {
			OlActivateWidget(w, OLM_KDeselectAll, NULL);
			break;
		}
	}
}
void
SendMailCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	OlFlatCallData *	p = (OlFlatCallData *)call_data;
	SendRec *		sp = FindSendRec (wid);

	DisplayInLeftFooter (sp->baseWGizmo, "", True);
	DtLockCursor (
		GetBaseWindowShell (sp->baseWGizmo),
		1000L, NULL, NULL,
		OlGetBusyCursor(wid)
	);
	switch (p->item_index) {
		case MenuSend: {
			SendIt (sp);
			break;
		}
		case MenuAliases: {
			AliasShowPopupCB (wid, client_data, call_data);
			break;
		}
	}
}


static void
IncludeFile (sp, filename)
SendRec *	sp;
char *		filename;
{
	SendGizmo *	gizmo;
	Widget		shell;
	char *		buf;
	char *		text;
	char		cmd[BUF_SIZE];

	shell = GetBaseWindowShell (sp->baseWGizmo);

	/* Get the message to be included */
	if ((buf = ReadFile (shell, filename)) == NULL) {
		return;
	}
	text = InsertPrefix (buf);

	gizmo = (SendGizmo *)QueryGizmo (
		BaseWindowGizmoClass, sp->baseWGizmo, GetGizmoGizmo,
		SendName
	);
	OlTextEditInsert (GetSendTextWidget (gizmo), text, strlen (text));
	FREE (buf);
	FREE (text);
}

static void
SendAppProc(wid, client_data, call_data)
Widget		wid;
XtPointer	client_data;
XtPointer	call_data;
{
	DtDnDInfoPtr	dip = (DtDnDInfoPtr)call_data;
	SendRec *	sp = FindSendRec (wid);
	Widget		shell;

	if (dip->error != 0) {
		dip->send_done = False;
		return;
	}
	if (dip->nitems != 1) {
		FPRINTF ((stderr, "Need notice here\n"));
		return;
	}
	if (sp == (SendRec *)0) {
		sp = CreateSendRec ();
	}
	shell = GetBaseWindowShell (sp->baseWGizmo);
	if (dip->files != (char **)0) {
		/* Try to read this file in as a saved file */
		if (DisplaySavedText(sp, shell, *dip->files)!=ErrorDisplayed) {
			/* If that fails just include it */
			IncludeFile (sp, *dip->files);
		}
	}
}

Boolean
SendDropNotify (
	Widget w, Window win, Position x, Position y, Atom selection,
	Time timestamp, OlDnDDropSiteID drop_site_id,
	OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
	XtPointer closure)
{
	DtDnDInfoPtr dip;

	FPRINTF ((stderr, "Got a drop on send\n"));
	dip = DtGetFileNames (
		w, selection, timestamp, send_done, SendAppProc, closure
	);

	if (dip != (DtDnDInfoPtr)0) {
		if (dip->error) {
			_OlTextEditTriggerNotify (
				w, win, x, y, selection, timestamp,
				drop_site_id, op, send_done, forwarded, closure
			);
		}
		/*
		 * FIX: free dip (but how?)
		 */
	}

	return True;
}
