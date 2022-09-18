/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:cmd2.c	1.8.2.8"
#ident "@(#)cmd2.c	1.21 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * More user commands.
 */

static int	delm ARGS((int *msgvec));
static int	ignore_retain_field ARGS((char *list[], struct ignret **ignretbuf, int doretcount, const char *msg));
static int	ignretshow ARGS((struct ignret **ignretbuf, const char *msg));
static int	save1 ARGS((char str[], int mark));
static int	Save1 ARGS((int *msgvec, int mark));
static void	savemsglist ARGS((char *file, int *msgvec, int mark));
static char	*snarf ARGS((char*,int*,int));
static int	unignore_retain_field ARGS((char *list[], struct ignret **ignretbuf, int doretcount, const char *msg));

/*
 * If any arguments were given, go to the next applicable argument
 * following dot, otherwise, go to the next applicable message.
 * If given as first command with no arguments, print first message.
 */

next(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register int *ip, *ip2;
	int list[2], mdot;

	if (*msgvec != NULL) {

		/*
		 * If some messages were supplied, find the
		 * first applicable one following dot using
		 * wrap around.
		 */

		mdot = dot - &message[0] + 1;

		/*
		 * Find the first message in the supplied
		 * message list which follows dot.
		 */

		for (ip = msgvec; *ip != NULL; ip++)
			if (*ip > mdot)
				break;
		if (*ip == NULL)
			ip = msgvec;
		ip2 = ip;
		do {
			mp = &message[*ip2 - 1];
			if ((mp->m_flag & MDELETED) == 0) {
				dot = mp;
				goto hitit;
			}
			if (*ip2 != NULL)
				ip2++;
			if (*ip2 == NULL)
				ip2 = msgvec;
		} while (ip2 != ip);
		pfmt(stdout, MM_ERROR, ":189:No messages applicable\n");
		return(1);
	}

	/*
	 * If this is the first command, select message 1.
	 * Note that this must exist for us to get here at all.
	 */

	if (!sawcom)
		goto hitit;

	/*
	 * Just find the next good message after dot, no
	 * wraparound.
	 */

	for (mp = dot+1; mp < &message[msgCount]; mp++)
		if ((mp->m_flag & (MDELETED|MSAVED)) == 0)
			break;
	if (mp >= &message[msgCount]) {
		pfmt(stdout, MM_WARNING, ateof);
		return(0);
	}
	dot = mp;
hitit:
	/*
	 * Print dot.
	 */

	list[0] = dot - &message[0] + 1;
	list[1] = NULL;
	return(type(list));
}

/*
 * Save a message in a file.  Mark the message as saved
 * so we can discard when the user quits.
 */
save(str)
	char str[];
{
	return(save1(str, 1));
}

/*
 * Copy a message to a file without affected its saved-ness
 */
copycmd(str)
	char str[];
{
	return(save1(str, 0));
}

/*
 * Save/copy the indicated messages at the end of the passed file name.
 * If mark is true, mark the message "saved."
 */
static int
save1(str, mark)
	char str[];
{
	char *fname;
	int f, *msgvec;

	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((fname = snarf(str, &f, 0)) == NOSTR)
		fname = Getf("MBOX");
	if (f==-1)
		return(1);
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			if (mark)
				pfmt(stdout, MM_ERROR,
					":190:No messages to save.\n");
			else
				pfmt(stdout, MM_ERROR,
					":191:No messages to copy.\n");
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if ((fname = expand(fname)) == NOSTR)
		return(1);
	savemsglist(fname, msgvec, mark);
	return(0);
}

Save(msgvec)
int *msgvec;
{
	return(Save1(msgvec, 1));
}

Copy(msgvec)
int *msgvec;
{
	return(Save1(msgvec, 0));
}

/*
 * save/copy the indicated messages at the end of a file named
 * by the sender of the first message in the msglist.
 */
static int
Save1(msgvec, mark)
int *msgvec;
{
	char recfile[PATHSIZE];

	getrecf(nameof(&message[*msgvec-1]), recfile, 1);
	savemsglist(safeexpand(recfile), msgvec, mark);
	return(0);
}

/*
 * save a message list in a file
 */
static void
savemsglist(fname, msgvec, mark)
	char *fname;
	int *msgvec;
{
	register int *ip, mesg;
	register struct message *mp;
	char *disp;
	FILE *obuf;
	struct stat statb;
	long lc, cc, t;
	int bnry;

	printf("\"%s\" ", fname);
	flush();
	if (stat(fname, &statb) >= 0)
		disp = gettxt(appendedid, appended);
	else
		disp = gettxt(newfileid, newfile);
	if ((obuf = fopen(fname, "a")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, fname, strerror(errno));
		return;
	}
	lc = cc = 0;
	bnry = 0;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		mp = &message[mesg-1];
		if (mp->m_text == M_binary) {
			bnry = 1;
		}
		if ((t = send(mp, obuf, 0, 0, 1)) < 0) {
			pfmt(stderr, MM_ERROR, badwrite,fname, strerror(errno));
			fclose(obuf);
			return;
		}
		touch(mesg);
		lc += t;
		cc += mp->m_size;
		if (mark)
			mp->m_flag |= MSAVED;
	}
	fflush(obuf);
	if (ferror(obuf))
		pfmt(stderr, MM_ERROR, badwrite, fname, strerror(errno));
	fclose(obuf);
	if (!bnry) {
		pfmt(stdout, MM_NOSTD, textsize, disp, lc, cc);
	} else {
		pfmt(stdout, MM_NOSTD, binarysize, disp, cc);
	}
}

/*
 * Write the indicated messages at the end of the passed
 * file name, minus header and trailing blank line.
 */

swrite(str)
	char str[];
{
	register int *ip, mesg;
	register struct message *mp;
	register char *fname, *disp;
	int f, *msgvec;
	long lc, cc, t, clen;
	FILE *obuf;
	struct stat statb;
	int bnry;

	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((fname = snarf(str, &f, 1)) == NOSTR)
		return(1);
	if (f==-1)
		return(1);
	if ((fname = expand(fname)) == NOSTR)
		return(1);
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == NULL) {
			pfmt(stdout, MM_ERROR, ":192:No messages to write.\n");
			return(1);
		}
		msgvec[1] = NULL;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	printf("\"%s\" ", fname);
	flush();
	if (stat(fname, &statb) >= 0)
		disp = gettxt(appendedid, appended);
	else
		disp = gettxt(newfileid, newfile);
	if ((obuf = fopen(fname, "a")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, fname, strerror(errno));
		return(1);
	}
	lc = cc = 0L;
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		lc += send(mp, obuf, 0, 0, 0);
		cc += mp->m_clen > 1 ? mp->m_clen - 1 : 0;
		mp->m_flag |= MSAVED;
		if (mp->m_text == M_binary) bnry = 1;
	}
	fflush(obuf);
	if (ferror(obuf))
		pfmt(stderr, MM_ERROR, badwrite, fname, strerror(errno));
	fclose(obuf);
	if (!bnry) {
		pfmt(stdout, MM_NOSTD, textsize, disp, lc, cc);
	} else {
		pfmt(stdout, MM_NOSTD, binarysize, disp, cc);
	}
	return(0);
}

/*
 * Snarf the file from the end of the command line and
 * return a pointer to it.  If there is no file attached,
 * just return NOSTR.  Put a null in front of the file
 * name so that the message list processing won't see it,
 * unless the file name is the only thing on the line, in
 * which case, return 0 in the reference flag variable.
 */

static char *
snarf(linebuf, flag, erf)
	char linebuf[];
	int *flag;
{
	register char *cp;
	char end;

	*flag = 1;
	cp = strlen(linebuf) + linebuf - 1;

	/*
	 * Strip away trailing blanks.
	 */
	while (*cp == ' ' && cp > linebuf)
		cp--;
	*++cp = 0;

	/*
	 * Now see if string is quoted
	 */
	if (cp > linebuf && any(cp[-1], "'\"")) {
		end = *--cp;
		*cp = '\0';
		while (*cp != end && cp > linebuf)
			cp--;
		if (*cp != end) {
			pfmt(stdout, MM_ERROR,
				":194:Syntax error: missing %c.\n", end);
			*flag = -1;
			return(NOSTR);
		}
		if (cp==linebuf)
			*flag = 0;
		*cp++ = '\0';
		return(cp);
	}

	/*
	 * Now search for the beginning of the file name.
	 */

	while (cp > linebuf && !any(*cp, "\t "))
		cp--;
	if (*cp == '\0') {
		if (erf)
			pfmt(stdout, MM_ERROR, ":195:No file specified.\n");
		*flag = 0;
		return(NOSTR);
	}
	if (any(*cp, " \t"))
		*cp++ = 0;
	else
		*flag = 0;
	return(cp);
}

/*
 * Split a command line into two parts: the message lists
 * and the remainder. Return a pointer to the latter (or NOSTR
 * if there is none). Indicate the presence of message lists
 * by returning 1 in the reference flag variable.
 * Put a null in front of the latter part so that the message
 * list processing won't see it, unless the latter part is the
 * only thing on the line, in which case, return 0 in the
 * reference flag variable.
 */

char *
snarf2(linebuf, flag)
	char linebuf[];
	int *flag;
{
	register char *cp, *cp2;
	char *svcp;

	*flag = 1;
	cp = (char*)skipspace(linebuf);
	svcp = cp;

	/* is there any message list? */

	/* search for something which is not a message list */
	while (isdigit(*cp) || any(*cp, ":&^$*/.+-")) {
		cp = (char*)skiptospace(cp);
		cp = (char*)skipspace(cp);
	}

	/* no message list? */
	if (cp == svcp)
		*flag = 0;

	/* separate the two parts */
	if (*cp && cp > linebuf)
		cp[-1] = '\0';

	/* check for quoted string */
	if (*cp == '"' || *cp == '\'') {
		/* zap any trailing quote */
		cp++;
		cp2 = cp + strlen(cp) - 1;
		while ((cp2 > cp) && isspace(*cp2))
			cp2--;
		if ((cp2 > cp) && (*cp2 == cp[-1]))
			*cp2 = '\0';
		else {
			pfmt(stdout, MM_ERROR,
				":194:Syntax error: missing %c.\n", cp[-1]);
			*flag = -1;
			return(NOSTR);
		}
	}
	return *cp ? cp : NOSTR;
}

/*
 * Delete messages.
 */

delete(msgvec)
	int msgvec[];
{
	return(delm(msgvec));
}

/*
 * Delete messages, then type the new dot.
 */

deltype(msgvec)
	int msgvec[];
{
	int list[2];
	int lastdot;

	lastdot = dot - &message[0] + 1;
	(void) delm(msgvec);
	list[0] = first(0, MDELETED);
	if (list[0] == NULL) {
		pfmt(stdout, MM_WARNING, ":196:No more messages\n");
		return(0);
	}
	if (value("reversedp") || (list[0] > lastdot)) {
		touch(list[0]);
		list[1] = NULL;
		return(type(list));
	}
	pfmt(stdout, MM_WARNING, ateof);
	return(0);
}

/*
 * Delete the indicated messages.
 * Set dot to some nice place afterwards.
 * Internal interface.
 */
static int
delm(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register *ip, mesg;
	int last = 0;

	for (ip = msgvec; *ip != NULL; ip++) {
		mesg = *ip;
		touch(mesg);
		mp = &message[mesg-1];
		mp->m_flag |= MDELETED|MTOUCH;
		mp->m_flag &= ~(MPRESERVE|MSAVED|MBOX);
		last = mesg;
	}
	dot = &message[last-1];
	return(0);
}

/*
 * Undelete the indicated messages.
 */
int
undelete(msgvec)
	int *msgvec;
{
	register struct message *mp;
	register *ip, mesg;

	for (ip = msgvec; ip-msgvec < msgCount; ip++) {
		mesg = *ip;
		if (mesg == 0)
			return(0);
		touch(mesg);
		mp = &message[mesg-1];
		dot = mp;
		mp->m_flag &= ~MDELETED;
	}
	return(0);
}

/*
 * Add the given header fields to the ignored list.
 * If no arguments, print the current list of ignored fields.
 */
igfield(list)
	char *list[];
{
    return ignore_retain_field(list, ignore, 0, nofieldignored);
}

/*
 * Add the given header fields to the retained list.
 * If no arguments, print the current list of retained fields.
 */
retainfield(list)
	char *list[];
{
    return ignore_retain_field(list, retain, 1, nofieldretained);
}

/*
 * Remove a list of fields from the ignore list.
 */
unigfield(list)
	char *list[];
{
    return unignore_retain_field(list, ignore, 0, nofieldignored);
}

/*
 * Remove a list of fields from the retained list.
 */
unretainfield(list)
	char *list[];
{
    return unignore_retain_field(list, retain, 1, nofieldretained);
}

/*
 * Add the given header fields to the ignored/retained list.
 * If no arguments, print the current list of ignored/retained fields.
 */
static int
ignore_retain_field(list, ignretbuf, doretcount, msg)
	char *list[];
	struct ignret **ignretbuf;
	int doretcount;
	const char *msg;
{
	char field[BUFSIZ];
	register int h;
	register struct ignret *retp;
	char **ap;

	if (argcount(list) == 0)
		return(ignretshow(ignretbuf, msg));
	for (ap = list; *ap != 0; ap++) {
		if (isignretained(*ap, ignretbuf))
			continue;
		istrcpy(field, *ap);
		h = hash(field);
		retp = (struct ignret *) pcalloc(sizeof (struct ignret));
		retp->i_field = pcalloc((unsigned)(strlen(field) + 1));
		strcpy(retp->i_field, field);
		retp->i_link = ignretbuf[h];
		ignretbuf[h] = retp;
		if (doretcount)
			retaincount++;
	}
	return(0);
}

/*
 * Print out all currently ignored/retained fields.
 */
static int
ignretshow(ignretbuf, msg)
	struct ignret **ignretbuf;
	const char *msg;
{
	register int h, count;
	struct ignret *retp;
	char **ap, **ring;

	count = 0;
	for (h = 0; h < HSHSIZE; h++)
		for (retp = ignretbuf[h]; retp != 0; retp = retp->i_link)
			count++;
	if (count == 0) {
		pfmt(stdout, MM_INFO, msg);
		return(0);
	}
	ring = (char **) salloc((count + 1) * sizeof (char *));
	ap = ring;
	for (h = 0; h < HSHSIZE; h++)
		for (retp = ignretbuf[h]; retp != 0; retp = retp->i_link)
			*ap++ = retp->i_field;
	*ap = 0;
	qsort((char*)ring, count, sizeof (char *), compstrs);
	for (ap = ring; *ap != 0; ap++)
		printf("%s\n", *ap);
	return(0);
}

/*
 * Remove a list of fields from the ignore/retain list.
 */
static int
unignore_retain_field(list, ignretbuf, doretcount, msg)
	char *list[];
	struct ignret **ignretbuf;
	int doretcount;
	const char *msg;
{
	char **ap, field[BUFSIZ];
	register int h, count = 0;
	register struct ignret *ignret1, *ignret2;

	if (argcount(list) == 0) {
		for (h = 0; h < HSHSIZE; h++) {
			ignret1 = ignretbuf[h];
			while (ignret1) {
				free(ignret1->i_field);
				ignret2 = ignret1->i_link;
				free((char *) ignret1);
				ignret1 = ignret2;
				count++;
			}
			ignretbuf[h] = NULL;
		}
		if (count == 0)
			pfmt(stdout, MM_INFO, msg);
		return 0;
	}
	for (ap = list; *ap; ap++) {
		istrcpy(field, *ap);
		h = hash(field);
		for (ignret1 = ignretbuf[h]; ignret1; ignret2 = ignret1, ignret1 = ignret1->i_link)
			if (strcmp(ignret1->i_field, field) == 0) {
				if (ignret1 == ignretbuf[h])
					ignretbuf[h] = ignret1->i_link;
				else
					ignret2->i_link = ignret1->i_link;
				free(ignret1->i_field);
				free((char *) ignret1);
				if (doretcount)
					retaincount--;
				break;
			}
	}
	return 0;
}
