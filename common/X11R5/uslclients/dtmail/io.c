/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:io.c	1.36"
#endif

#define IO_C

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stropts.h>
#include <poll.h>
#include "mail.h"

extern ReadRec *	readRec;
extern RecordType	RecordOutgoing;
extern ListHead		IgnoreList;
extern char *		Home;
extern char *		Signature;
extern char *		Mbox;
extern HeaderSettings	ShowHeader;
extern int		Version;

char *	ProcessCommand();

#define NEW		"New mail has arrived.\nLoaded "
#define NEW_MAIL_CMD	"newmail"

void
AddNewMail (mp, buf, num)
MailRec *	mp;
char *		buf;
int		num;
{
	int	i;
	char *	start = buf;
	char *	string;
	char *	savept = NULL;

	mp->summary->size += num;
	mp->summary->items = (ListItem *) REALLOC (
		mp->summary->items,
		sizeof (ListItem)*mp->summary->size
	);
	for (i=mp->summary->size-num; i<mp->summary->size; i++) {
		if ((string = MyStrtok (start, "\n", &savept)) == NULL) {
			break;
		}
		start = NULL;
		CreateHeader (mp, i, string);
	}
	DisplayNewMailMessage (mp, mp->filename);
	if (mp->mng != (ManageRec *)0) {
		UpdateList (GetSummaryListGizmo (mp->mng));
	}
}

static void
CheckForNewMail (mp, buf)
MailRec *mp;
char *buf;
{
	int size;
	char *cp;
	char *tp;
	char *orig;

	cp = buf;
	/*
	 * Look for the string:
	 *	New mail has arrived.
	 *	Loaded 1 new message
	 * If the string is found load the new mail
	 * messages based on the headers that follow
	 * this message.
	 */
	while ((cp = strchr (cp, NEW[0])) != NULL) {
		orig = cp;
		if (strncmp (cp, NEW, strlen (NEW)) == 0) {
			tp = cp+strlen(NEW);
			sscanf (tp, "%d", &size);
			cp = strchr (tp, '\n');
			cp += 1;
			AddNewMail (mp, cp, size);
			*orig = '\0';
			break;
		}
		cp += 1;
	}
}

static char *
GatherInput (mp)
MailRec *mp;
{
	char *		cp;
	char *		pp;
	int		i;
	int		cnt;
	static int	size;
	static char *	text = NULL;

	/* Loop on stdout and stderr until the input is terminated
	 * with prompt text.
	 */
	if (text == NULL) {
		size = BUF_SIZE;
		text = (char *)MALLOC (size);
	}
	cp = text;  
	pp = cp;
	do {
		i = size - 1 - (int)(cp - text);
		cnt = read (fileno(mp->fp[1]), cp, i);
		cp += cnt;
		*cp = '\0';
		if (cnt == i) {		/* Is it time to grow this buffer? */
			i = (int)(cp - text);
			size += BUF_SIZE;
			text = REALLOC (text, size);
			if (text == NULL) {
				FPRINTF ((stderr, "can't realloc %d\n", size));
				exit (1);
			}
			cp = text + i;
		}
		pp = cp;
		if ((int)(pp-text) >= PROMPT_LEN) {	/* cp points to '\0' */
			pp -= PROMPT_LEN;
		}
	} while (strcmp (PROMPT_TEXT, pp) != 0);
	*pp = '\0';
	if (strncmp (text, NTS_UX_MAILX, sizeof(NTS_UX_MAILX)) == 0) {
		strcpy (text, text+sizeof(NTS_UX_MAILX));
	}
	CheckForNewMail (mp, text);
	return text;
}

/*
 * WriteToMailx is different from ProcessCommand in that ProcessCommand
 * expects output from mailx after the command is sent to mailx.
 */

void
WriteToMailx (mp, string, len)
MailRec *mp;
char *string;
int len;
{
	write (fileno(mp->fp[0]), string, len);
	FPRINTF ((stderr, "%s", string));
}

char *
ProcessCommand (mp, cmd, name)
MailRec *mp;
char *cmd;
char *name;	/* Possible filename to follow item numbers */
{
	static char *string = NULL;
	static int size = 256;
	int len;

	if (string == NULL) {
		string = (char *)MALLOC (size);
	}
	/* Output the command to mailx */
	if (cmd != NULL) {
		len = strlen (cmd) + 2;
		if (name != NULL) {
			len += strlen (name) + 1;
		}
		if (len >= size) {
			size += len;
			string = (char *)REALLOC (string, size);
		}
		strcpy (string, cmd);
		if (name != NULL) {
			strcat (string, " ");
			strcat (string, name);
		}
		strcat (string, "\n");
		FPRINTF ((stderr, "%s", string));
		write (fileno (mp->fp[0]), string, strlen (string));
	}
	/* Gather up all the lines upto the prompt */
	return GatherInput (mp);
}

static char *
GetNextNum (set, item, i)
Boolean		set;
ListItem *	item;
int		i;
{
	static int last = 0;
	char num[256];

	num[0] = '\0';
	if (set == True) {
		if (last == 0)      sprintf (num, "%d", item[i].clientData);
		else if (last == 1) sprintf (num, "-");
		last += 1;
	}
	else {
		if (last == 1)      sprintf (num, " ");
		else if (last > 0)  sprintf (num, "%d ", item[i-1].clientData);
		last = 0;
	}
	return num;
}

static void
Stuff (buf, len, num)
char **	buf;
int *	len;
char *	num;
{
	static int	size = 256;

	if (*buf == NULL) {
		*buf = (char *)MALLOC (size);
		*buf[0] = '\0';
	}
	if (num != NULL && num[0] != '\0') {
		*len += strlen (num)+1;
		if (*len >= size) {
			size = *len + 256;
			*buf = (char *)REALLOC (
				*buf, size
			);
		}
		strcat (*buf, num);
	}
	return;
}

/* Construct a compact list of messages that are selected
 * between the values start and end-1.
 */

char *
CompactListOfSelectedItems (mp, lp, name, start, end)
MailRec *	mp;
ListHead *	lp;
char *		name;	/* Possible filename to follow item numbers */
int		start;
int		end;
{
	int		len = 0;
	static char *	buf = NULL;
	char *		num;
	int		i;

	if (buf != NULL) {
		buf[0] = '\0';
	}
	for (i=start; i<end; i++) {
		num = GetNextNum ((int)lp->items[i].set, lp->items, i);
		Stuff (&buf, &len, num);
	}
	num = GetNextNum (False, lp->items, i);
	Stuff (&buf, &len, num);
	if (name != NULL && buf[0] != '\0') {
		Stuff (&buf, &len, name);
	}
	return buf;
}

/* Construct a list of messages that are selected
 * between the values start and end-1.
 */

char *
ListOfSelectedItems (mp, lp, name, start, end)
MailRec *	mp;
ListHead *	lp;
char *		name;	/* Possible filename to follow item numbers */
int		start;
int		end;
{
	char		num[10];
	int		i;
	int		len;
	static char *	buf = NULL;
	static int	size = 256;

	if (buf == NULL) {
		buf = (char *)MALLOC (size);
	}
	len = 0;
	if (buf != NULL) {
		buf[0] = '\0';
	}
	for (i=start; i<end; i++) {
		if (lp->items[i].set == True) {
			sprintf (num, "%d ", lp->items[i].clientData);
			Stuff (&buf, &len, num);
		}
	}
	if (name != NULL && buf[0] != '\0') {
		Stuff (&buf, &len, name);
	}
	return buf;
}

char *
ListOfSelectedReadMessages (mp)
MailRec *	mp;
{
	ReadRec *	nrp;
	char		num[10];
	int		i;
	int		len;
	static char *	buf = NULL;
	static int	size = 256;

	if (buf == NULL) {
		buf = (char *)MALLOC (size);
	}
	len = 0;
	if (buf != NULL) {
		buf[0] = '\0';
	}
	for (nrp=readRec; nrp!=(ReadRec *)0; nrp=nrp->next) {
		if (nrp->mp != mp) {
			continue;
		}
		i = ItemNumber (mp, nrp->message);
		if (mp->summary->items[i].set == True) {
			sprintf (num, "%d ", nrp->message);
			Stuff (&buf, &len, num);
		}
	}
	return buf;
}

int
OpenFile (shell, filename)
Widget	shell;
char *	filename;
{
	int		fp;
	char		buf[BUF_SIZE];
	extern int	errno;

	strcpy (buf, GetGizmoText (TXT_CANT_OPEN));
	strcat (buf, filename);
	strcat (buf, " ");
	if ((fp = open (filename, O_RDONLY)) < 0) {
		strcat (buf, GetTextGivenErrno (errno));
		DisplayErrorPrompt(shell, buf);
		return -1;
	}
	return fp;
}

char *
ReadFile (shell, filename)
Widget		shell;
char *		filename;
{
	char *	buf;
	char *	cp;
	int	size;
	int	cnt;
	int	fp;

	if ((fp = OpenFile (shell, filename)) == -1) {
		return NULL;
	}

	size = 0;
	buf = (char *)MALLOC (1);
	buf[0] = '\0';
	do {
		if ((buf = (char *)REALLOC (buf, size+BUF_SIZE)) == NULL) {
			DisplayErrorPrompt (
				shell, GetGizmoText (TXT_TOO_LARGE)
			);
			return NULL;
		}
		cp = buf+size;  
		cnt = read (fp, cp, BUF_SIZE-1);
		if (cnt != -1) {
			size += cnt;
			cp += cnt;
			*cp = '\0';
		}
	} while (cnt == BUF_SIZE-1);
	
	close (fp);
	return buf;
}

void
UpdateMailrc (mp)
MailRec *	mp;
{
	FILE *		fp;
	FILE *		tp;
	char *		mailrc;
	char		buf[BUF_SIZE];
	char *		cp;
	char *		temp;
	char **		tmp;
	int		i;
	char *		text;
	ListHead *	hp;
	Boolean		first;

	if ((mailrc = (char *)getenv(GetGizmoText(TXT_MAILRC_ENV))) == NULL) {
		mailrc = GetGizmoText (TXT_DOT_MAILRC);
	}
	chdir (Home);
	if ((fp = fopen (mailrc, "r")) == NULL) {
		/* If it doesn't open them create one */
		if ((fp = fopen (mailrc, "w+")) == NULL) {
			return;
		}
	}

	temp = tmpnam (NULL);
	if ((tp = fopen (temp, "w+")) == NULL) {
		fclose (fp);
		return;
	}

	while ((cp = (char *)fgets (buf, BUF_SIZE, fp)) != NULL) {
		/* Don't transfer and settings that start with zzz to
		 * the new temporary file.
		 */
		text = NTS_SET_ZZZ;
		if (strncmp (text, buf, strlen (text)) == 0) {
			continue;
		}
		/* Don't transfer dtmail comments */
		text = NTS_POUND_DTMAIL;
		if (strncmp (text, buf, strlen (text)) == 0) {
			continue;
		}
		fputs (buf, tp);
	}
	/*
	 * Output the comments telling the user how to update both mailx
	 * and dtmail.
	 */
	fputs (NTS_COMMENT0, tp);
	fputs (NTS_COMMENT1, tp);
	fputs (NTS_COMMENT2, tp);
	fputs (NTS_COMMENT3, tp);
	fputs (NTS_COMMENT4, tp);
	fputs (NTS_COMMENT0, tp);
	/*
	 * Output the signature
	 */
	if (Signature != NULL) {
		fprintf (tp, NTS_SET_ZZZSIGN);
		/* Convert any '\n' to "\n" */
		for (cp=Signature; *cp!='\0'; cp++) {
			if (*cp == '\n') {
				if (*(cp+1) != '\0') {
					/* Don't output trailing '\n' */
					fputs ("\\n", tp);
				}
			}
			else if (*cp == '\t') {
				fputs ("\\t", tp);
			}
			else {
				putc (*cp, tp);
			}
		}
		fputs ("\"\n", tp);
	}
	/*
	 * Output the zzzMBOX setting.
	 */
	if (Mbox != NULL) {
		fprintf (tp, NTS_SET_ZZZMBOX, Mbox);
	}
	if (RecordOutgoing == DoIt) {
		fputs (NTS_SET_ZZZRECORD_YES, tp);
	}
	else {
		fputs (NTS_SET_ZZZRECORD_NO, tp);
	}
	if (ShowHeader == Full) {
		fputs (NTS_SET_ZZZNOBRIEF, tp);
	}
	/* Output the unignore list */
	first = True;
	hp = &IgnoreList;
	for (i=0; i<hp->size; i++) {
		if (hp->items[i].set == True) {
			tmp = (char **) hp->items[i].fields;
			if (first == True) {
				fputs (NTS_SET_ZZZUNIGNORE, tp);
				first = False;
			}
			else {
				fprintf (tp, " ");
			}
			fprintf (tp, "%s", tmp[0]);
		}
	}
	if (first == False) {
		fprintf (tp, "\"\n");
	}
	/* Output the ignore list */
	hp = &IgnoreList;
	first = True;
	for (i=0; i<hp->size; i++) {
		if (hp->items[i].set == False) {
			tmp = (char **) hp->items[i].fields;
			if (first == True) {
				fputs (NTS_SET_ZZZIGNORE, tp);
				first = False;
			}
			else {
				fprintf (tp, " ");
			}
			fprintf (tp, "%s", tmp[0]);
		}
	}
	if (first == False) {
		fprintf (tp, "\"\n");
	}

	fclose (tp);
	fclose (fp);

	/* Delete the old mailrc and move this temporary file to mailrc */

	sprintf (buf, "mv %s %s", temp, mailrc);
	system (buf);
}
