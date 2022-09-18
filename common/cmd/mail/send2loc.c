/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2loc.c	1.7.2.5"
#ident "@(#)send2loc.c	1.18 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2local - send to all local users

    SYNOPSIS
	void send2local(Msg *pmsg, int wherefrom, int level)

    DESCRIPTION
	send2local() will traverse the local recipient list
	(on the queue wherefrom) and send it locally. Level is
	used to prevent a forwarding loop.
*/

static void noquotes ARGS((string *orig, char *val));
static SendSurgRet send2forward ARGS((Msg *pmsg, Recip *r, char *frwrdbuf, int wherefrom, int level));
static SendSurgRet send2forwardpipe ARGS((Msg *pmsg, Recip *r, int wherefrom, const char *cmd));
static void send2forwarduser ARGS((Msg *pmsg, string *whofrom, char *name, int level));
static void send2here ARGS((Msg *pmsg, Recip *r, int wherefrom, int wheresuccess));
static int safefile ARGS((char*));
static int trylock ARGS((Msg *pmsg, Recip *r, int wherefrom));

void send2local(pmsg, wherefrom, level)
Msg	*pmsg;
int	wherefrom;
int	level;
{
    static const char pn[] = "send2local";
    Recip *l;	/* list head */
    Recip *r;	/* recipient at list head */
    char frwrdbuf[FWRDBUFSIZE];
    int where_ack = surr_len + RECIPS_TEMP2;
    int where_nak = surr_len + RECIPS_TEMP;
    int where_tmp = surr_len + RECIPS_TEMP3;
    int dofailsafe;

    Dout(pn, 0, "Entered\n");
    if (flgT)
	{
	Dout(pn, 0, "Suppressing execution phase (-T); reporting names\n");
	for (l = recips_head(pmsg, wherefrom); ((r = l->next) != (Recip*) NULL); )
	    {
	    r->local = islocal(s_to_c(r->name), &r->useruid);
	    Tout(pn,
		"If no SUCCESS via mailsurr file, local delivery of %s would %s\n",
		s_to_c(r->name), (r->local ? "succeed" : "fail"));
	    send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_SUCCESS);
	    }
	return;
	}

    dofailsafe = cksvdir();

    for (l = recips_head(pmsg, wherefrom); (r = l->next) != (Recip*) NULL; )
	{
	r->local = islocal(s_to_c(r->name), &r->useruid);
	Dout(pn, 40, "looking at '%s', local='%d'\n", s_to_c(r->name), r->local);
	if (!r->local)
	    {
	    Dout(pn, 0, "\t%s is not a local name\n", s_to_c(r->name));
	    if (flglb)
		pfmt(stdout, MM_INFO, ":440:%s >> %s INVALID\n", s_to_c(recip_parent(r)->name), s_to_c(r->name));

	    else if (flgd)
		pfmt(stderr, MM_ERROR, ":344: %s is invalid!\n", s_to_c(r->name));

	    else
		{
		string *reparent = recip_parent(r)->name;
		if (strcmp(s_to_c(r->name), s_to_c(reparent)) == 0)
		    pfmt(stderr, MM_ERROR, ":409:Can't send to %s\n", s_to_c(r->name));
		else
		    pfmt(stderr, MM_ERROR, ":480:Can't send to %s (%s)\n", s_to_c(r->name), s_to_c(reparent));
		}

	    /* send a NAK for this recipient */
	    if ((pmsg->surg_rc != SURG_RC_DEF) && !flglb)
		{
		int svsurg_rc = pmsg->surg_rc;
		pmsg->surg_rc = r->SURRrc;
		send2mvrecip(pmsg, wherefrom, where_tmp);
		retmail(pmsg, where_tmp, E_SURG, (char*)0);
		send2mvrecip(pmsg, where_tmp, surr_len + RECIPS_FAILURE);
		r->SURRrc = pmsg->surg_rc;
		pmsg->surg_rc = svsurg_rc;
		}

	    else
		send2mvrecip(pmsg, wherefrom, where_nak);
	    }

	else if (dofailsafe)
	    {
	    string *buf = s_xappend((string*)0, failsafe, "!", s_to_c(r->name), (char*)0);
	    Dout(pn, 0, "\tfailsafe: now sending to '%s'\n", s_to_c(buf));
	    if (flglb || flgd)
		pfmt(stdout, MM_INFO, ":441:%s is being forwarded to '%s'\n", s_to_c(r->name), s_to_c(buf));
	    if (!flglb)
		send2forwarduser(pmsg, r->name, s_to_c(buf), level);
	    send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_DONE);
	    s_free(buf);
	    }

	else
	    {
	    int fwrd;
	    if (!trylock(pmsg, r, wherefrom))
		continue;

	    /* Check for old-style forwarding */
	    oldforwarding(s_to_c(r->name));
	    fwrd = areforwarding(s_to_c(r->name), frwrdbuf, sizeof(frwrdbuf));
	    unlock();	/* All done checking forwarding, OK to unlock now */

	    if (fwrd)
		{
		SendSurgRet rc;
		Dout(pn, 0, "%s is forwarded to '%s'\n", s_to_c(r->name), frwrdbuf);

		/* If the msg was forwarded, it was completely finished there */
		/* >>unless<< a >| was found, in which case we also have to */
		/* deliver it locally. */
		rc = send2forward(pmsg, r, frwrdbuf, wherefrom, level);

		/* all taken care of */
		if (rc == FAILURE)
		    send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_DONE);

		/* must still deliver locally */
		else if (rc == CONTINUE)
		    send2here(pmsg, r, wherefrom, where_ack);

		/* must still acknowledge */
		else /* rc == SUCCESS */
		    send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_DONE);
		}

	    else
		send2here(pmsg, r, wherefrom, where_ack);
	    }
	}

    /* These messages could not be delivered locally */
    if (recips_exist(pmsg, where_nak))
	{
	Dout(pn, 0, "NAKs exist\n");
	/* If the last surrogate failed, pass it off as a surrogate problem. */
	if (!flglb)
	    retmail(pmsg, where_nak, ((pmsg->surg_rc != SURG_RC_DEF) ? E_SURG : E_USER), (char*)0);
	send2move(pmsg, where_nak, surr_len + RECIPS_FAILURE);
	}

    /* Now run through the successfully delivered messages and */
    /* do a gendeliv() for the messages which were delivered locally. */
    if (recips_exist(pmsg, where_ack))
	{
	Dout(pn, 0, "generating ACKs\n");
	if (!flglb)
	    gendeliv(pmsg, where_ack);
	send2move(pmsg, where_ack, surr_len + RECIPS_SUCCESS);
	}
}

/*
    NAME
	send2forward - handle "Forward to" information

    SYNOPSIS
	SendSurgRet send2forward(Msg *pmsg, Recip *r, char *frwrdbuf, int wherefrom, int level);

    DESCRIPTION
	For the list of names in the list, forward the message to that user.
	If the name is the same as the original recipient, ignore the name
	and return an indication to deliver the message locally. If the name
	starts with "|", send the message to the mail_pipe process. If the name
	starts with ">|", send the message to the mail_pipe process, but also
	ignore the exit code and return an indication to deliver the message locally.

    RETURNS
	CONTINUE => successful execution, also need to save message
	SUCCESS  => successful execution, need to send ACK back
	FAILURE  => unsuccessful execution, already sent NAK back
*/

static SendSurgRet send2forward(pmsg, r, frwrdbuf, wherefrom, level)
Msg	*pmsg;
Recip	*r;
char	*frwrdbuf;
int	wherefrom;
int	level;
{
    static const char pn[] = "send2forward";
    SendSurgRet ret = SUCCESS;
    register char *tsp, *q;

    Dout(pn, 0, "Entered\n");
    if (pmsg->hdrinfo.affcnt > FWRDLEVELS)
	{
	error = E_UNBND;
	retmail(pmsg, 0, E_UNBND, ":429:Unbounded multi-machine forwarding loop\n");
	return FAILURE;
	}

    for (tsp = (char*)skipspace(frwrdbuf); *tsp; )
	{
	/*
	 * Pick off names one at a time for forwarding.
	 * If name starts with '|' (pipe symbol), or '>|',
	 * assume rest of line is command to pipe to.
	 */
	if ((tsp[0] == '|') || ((tsp[0] == '>') && (tsp[1] == '|')))
	    {
	    SendSurgRet tret;
	    if (flglb || flgd)
		pfmt(stdout, MM_INFO, ":441:%s is being forwarded to '%s'\n", s_to_c(r->name), tsp);
	    if (flglb)
		tret = (tsp[0] == '|') ? SUCCESS : CONTINUE;
	    else
		tret = send2forwardpipe(pmsg, r, wherefrom, tsp);
	    if (ret != CONTINUE)
		ret = tret;
	    break;
	    }

	/* Find end of current foward-to name. */
	if ((q = strpbrk(tsp," \t\n,")) != (char *)NULL)
	    *q = '\0';

	if (tsp[0] == '\0')
	    {
	    /* Will get here if there were trailing */
	    /* blanks before the newline */
	    break;
	    }

	/* If the user's name is listed, deliver it */
	/* via send2local() above, but not here. */
	if (strcmp(tsp, s_to_c(r->name)) == SAME)
	    {
	    ret = CONTINUE;
	    continue;
	    }

	if (flglb || flgd)
	    pfmt(stdout, MM_INFO, ":441:%s is being forwarded to '%s'\n", s_to_c(r->name), tsp);
	if (!flglb)
	    send2forwarduser(pmsg, r->name, tsp, level);
	/*
	 * Assume that the 'Forward to' line in the mailfile
	 * has a newline before the terminating NULL
	 * or this will take you to never-never land.
	 */
	tsp = q + 1;
	}

    return ret;
}

/*
    NAME
	send2here - deliver message to user's mail box

    SYNOPSIS
	void send2here(Msg *pmsg, Recip *r, int wherefrom, int wheresuccess);

    DESCRIPTION
	Deliver a message to a user's mail box. On successful
	delivery, move the message to wheresuccess.
	Otherwise, move the message to RECIPS_FAILURE
*/

static void send2here(pmsg, r, wherefrom, wheresuccess)
Msg	*pmsg;
Recip	*r;
int	wherefrom;
int	wheresuccess;
{
    static const char pn[] = "send2here";
    string *lmbox = 0;
    FILE *malf;

    Dout(pn, 0, "Entered => '%s'\n", s_to_c(r->name));
    if (flglb)
	{
	pfmt(stdout, MM_INFO, ":446:%s >> %s/%s\n", s_to_c(recip_parent(r)->name), maildir, s_to_c(r->name));
	send2mvrecip(pmsg, wherefrom, wheresuccess);
	return;
	}

    if (flgd)
	pfmt(stdout, MM_INFO, ":442:>> %s/%s\n", maildir, s_to_c(r->name));

    if (!trylock(pmsg, r, wherefrom))
	return;

    lmbox = s_xappend(lmbox, maildir, s_to_c(r->name), (char*)0);
    createmf(r->useruid, s_to_c(lmbox));

    /* Disallow links. */
    if (!safefile(s_to_c(lmbox)))
	{
	unlock();
	error = E_FILE;
	Dout(pn, 0, "%s must be regular or character special file with no links, error set to %d\n",
	    s_to_c(lmbox), error);
	retmail(pmsg, wherefrom, E_FILE, ":158:'%s' must be regular or character special file with no links\n", s_to_c(lmbox));
	send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_FAILURE);
	s_free(lmbox);
	return;
	}

    /* Append letter to mail box */
    if (((malf = fopen(s_to_c(lmbox), "a")) == NULL) ||
	(mcopylet(pmsg, malf, ORDINARY) == FALSE) ||
	((fclose(malf) == EOF) && (malf = 0, 1)))
	{
	if (malf)
	    (void) fclose(malf);
	unlock();
	error = E_FILE;
	Dout(pn, 0, "cannot append to '%s', error set to %d\n", s_to_c(lmbox), error);
	retmail(pmsg, wherefrom, E_FILE, ":149:Cannot append to %s\n", s_to_c(lmbox));
	send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_FAILURE);
	s_free(lmbox);
	return;
	}

    unlock();
    send2mvrecip(pmsg, wherefrom, wheresuccess);
    s_free(lmbox);
}

/*
    NAME
	safefile - check mailbox to see if it's safe to write there

    SYNOPSIS
	int safefile(char*);

    DESCRIPTION
	Make certain that the mailfile has no links and is either a regular file
	or a character-special file. This latter check allows one to create
	a /dev/null node under mail for a mailbox which is just being thrown away.
*/

#ifdef SVR4
# include <sys/statvfs.h>
#endif

static int safefile(f)
char *f;
{
    struct stat statb;
#ifdef SVR4
    struct statvfs statvfsb;
#endif

#ifdef SVR3
    if (stat(f, &statb) < 0)
#else /* don't follow symbolic links */
    if (lstat(f, &statb) < 0)
#endif
	return 1;

    /* is the file type okay? */
    if (((statb.st_mode & S_IFMT) != S_IFREG) &&
	((statb.st_mode & S_IFMT) != S_IFCHR))
	return 0;

    /* are there other links? */
    if (statb.st_nlink != 1)
	return 0;

#ifdef SVR4
    /* is it a named filesystem mount? */
    if ((statvfs(f, &statvfsb) == 0) &&
	(strcmp(statvfsb.f_basetype, "namefs") == 0))
	return 0;
#endif
    return 1;
}

/*
    NAME
	send2forwardpipe - handle "Forward to |"

    SYNOPSIS
	SendSurgRet send2forwardpipe(Msg *pmsg, Recip *r, int wherefrom, const char *cmd, int ckrc);

    DESCRIPTION
	Transmit the message to a Forwarded pipe command. The recipient name
	is moved off to a temporary queue while being worked on by send2exec(),
	and then moved back so that send2forward() and send2local() will find it
	in the right place.

	For | commands, the default treatment of the command's exit value is S=0;F=*;.
	For >| commands, the default treatment of the command's exit value is C=*;.

    RETURNS
	CONTINUE => successful execution, also need to save message
	SUCCESS  => successful execution, need to send ACK back
	FAILURE  => unsuccessful execution, already sent NAK back
*/

static SendSurgRet send2forwardpipe(pmsg, r, wherefrom, cmd)
Msg		*pmsg;
Recip		*r;
int		wherefrom;
const char	*cmd;
{
    static const char pn[] = "send2forwardpipe";
    Hdrs	*hptr;				/* used to look up headers */
    string	*buf = 0;			/* holds the command to execute */
    int		whereexec = surr_len + RECIPS_TEMP3;
    string	*contype = s_new();		/* Content-Type: */
    string	*subject = s_new();		/* Subject: */
    char	flgxbuf[10];			/* holds -x# for passing to mail_pipe */
    SendSurgRet	ret;				/* return value */
    char	*retrc;				/* return code list */
    int		mustfree = 0;			/* retrc was malloc'd */
    int		cmdtype = (cmd[0] == '|');	/* remember which command type: | or >| */
    string	*rclist;			/* temporarily holds return code list */

    Dout(pn, 0, "Entered\n");
    send2mvrecip(pmsg, wherefrom, whereexec);

    /* Skip past | or >| */
    cmd += cmdtype ? 1 : 2;
    cmd = skipspace(cmd);

    Dout(pn, 0, "piping to '%s'\n", cmd);

    /* Copy in the default return code lists. */
    rclist = s_copy(cmdtype ? "S=0;F=*;" : "C=*;");
    s_restart(rclist);
    retrc = setsurg_rc(rclist, 0, t_transport, (int*)0, (int*)0, (int*)0, (int*)0);
    s_free(rclist);

    /* Pick off the return code list */
    if (isalpha(cmd[0]) && (cmd[1] == '='))
	{
	/* now copy in the specified setting */
	rclist = s_copy(cmd);
	s_restart(rclist);
	retrc = setsurg_rc(rclist, 1, t_transport, (int*)0, (int*)0, (int*)0, (int*)0);
	mustfree = 1;
	s_free(rclist);

	/* skip past the return code list */
	cmd = skiptospace(cmd);
	cmd = skipspace(cmd);
	Dout(pn, 0, "cmd now set to '%s'\n", cmd);
	}

    if ((hptr = pmsg->hdrinfo.hdrs[H_CTYPE]) != (Hdrs *)NULL)
	noquotes(contype, hptr->value);

    if ((hptr = pmsg->hdrinfo.hdrs[H_SUBJ]) != (Hdrs *)NULL)
	noquotes(subject, hptr->value);

    /* Pass any invocation debug option to the PIPER */
    if (flgx != 0)
	(void) sprintf(flgxbuf,"-x%d", flgx);
    else
	flgxbuf[0] = '\0';

    buf = s_xappend(buf, PIPER, " ", flgxbuf,
	" -r \"", s_to_c(r->name),
	"\" -R \"", s_to_c(pmsg->Rpath),
	"\" -c \"", s_to_c(contype),
	"\" -S \"", s_to_c(subject), "\"", (char*)0);
    Dout(pn, 0, "PIPER exec == '%s'\n", s_to_c(buf));
    if (!lock(s_to_c(r->name), 1))
	{
	Dout(pn, 0, "Cannot lock!\n");
	retmail(pmsg, whereexec, E_LOCK, ":428:Cannot lock mailbox\n");
	send2bmvrecip(pmsg, whereexec, wherefrom);
	s_free(buf);
	s_free(contype);
	s_free(subject);
	if (mustfree)
	    free(retrc);
	return FAILURE;
	}

    r->cmdl = buf;
    r->cmdr = 0;
    send2exec(pmsg, whereexec, 0);
    unlock();
    s_delete(r->cmdl);
    Dout(pn, 0, "PIPER complete, result %d\n", pmsg->surg_rc);

    ret = (SendSurgRet) retrc[pmsg->surg_rc];
    if (ret == FAILURE)
	{
	error = E_SURG;
	Dout(pn, 0, "PIPER command failed, error set to %d\n", error);
	r->cmdl = s_copy(cmd);
	retmail(pmsg, whereexec, E_SURG, ":430:PIPER command '%s' failed\n", cmd);
	s_delete(r->cmdl);
	send2bmvrecip(pmsg, whereexec, wherefrom);
	}

    else
	send2bmvrecip(pmsg, whereexec, wherefrom);

    s_free(buf);
    s_free(contype);
    s_free(subject);
    if (mustfree)
	free(retrc);
    return ret;
}

/*
    NAME
	noquotes - create a copy of the given string, changing " to '

    SYNOPSIS
	static void noquotes(string *orig, char *val)

    DESCRIPTION
	noquotes() copies "val" to the string, converting all occurrences
	of " to '.
*/
static void noquotes(orig, val)
string *orig;
char *val;
{
    for ( ; *val; val++)
	{
	if (*val == '"')
	    s_putc(orig, '\'');
	else
	    s_putc(orig, *val);
	}
    s_terminate(orig);
}

/*
    NAME
	send2forwarduser - handle "Forward to user"

    SYNOPSIS
	void send2forwarduser(Msg *pmsg, string *whofrom, char *name, int level);

    DESCRIPTION
	Forward the message from user "whofrom" to user "name".
	It also increments the level to prevent forwarding loops.
*/

static void send2forwarduser(parent, whofrom, name, level)
Msg	*parent;
string	*whofrom;
char	*name;
int	level;
{
    static const char pn[] = "send2forwarduser";
    Msg		msg;
    string	*new_afrom;
    Hdrs	*hptr;
    char	buf[30];

    Dout(pn, 0, "Entered\n");
    init_Msg(&msg);
    msg.parent = parent;
    ckdlivopts(parent);
    msg.delivopts = parent->delivopts;
    msg.type = Msg_forward;
    add_recip(&msg, name, FALSE, (Recip*)0, FALSE, FALSE, 0, 0, 0);
    mktmp(&msg.tmpfile);

    /* If there was a >To: field, generate a new one. */
    if ((hptr = parent->hdrinfo.hdrs[H_TCOPY]) != (Hdrs *)NULL)
	{
	string *new_tcopy = 0;
	char cbuf[1024];
	/* Pick comment field off of ">To:" line and put into cbuf */
	cbuf[0] = '\0';
	getcomment(hptr->value, cbuf);
	new_tcopy = s_xappend(new_tcopy, name, cbuf, (char*)0);
	save_a_hdr(&msg.hdrinfo, s_to_c(new_tcopy), H_TCOPY, (char*)0);
	s_free(new_tcopy);
	}

    /* Add an Auto-Forwarded-From: field */
    new_afrom = s_xappend((string*)0, thissys, "!", s_to_c(whofrom), (char*)0);
    save_a_hdr(&msg.hdrinfo, s_to_c(new_afrom), H_AFWDFROM, (char*)0);
    s_free(new_afrom);
    msg.Rpath = s_append(msg.Rpath, s_to_c(parent->Rpath));
    msg.orig = s_append(msg.orig, s_to_c(whofrom));
    msg.msgsize = parent->msgsize;
    msg.binflag = parent->binflag;
    msg.ret_on_error = parent->ret_on_error;
    msg.hdrinfo.affcnt = parent->hdrinfo.affcnt + 1;
    (void) sprintf(buf, "%d", msg.hdrinfo.affcnt);
    save_a_hdr(&msg.hdrinfo, buf, H_AFWDCNT, (char*)0);

    sendlist(&msg, level + 1);
    fini_Msg(&msg);
}

/*
    NAME
	cksvdir - check for /var/mail/:saved, /var/mail/:forward and /var/mail/:readlock

    SYNOPSIS
	int cksvdir()

    DESCRIPTION
	If the :saved directory is not there, check for failsafe processing
	(and forward to $FAILSAFE!name), or try to create the directory.
	Return TRUE if doing failsafe processing.

	The :saved directory may not be there if this is the very first time
	that mail has been sent at this security level.

	The :forward and :readlock directories are also checked for.
*/

int cksvdir()
{
    static const char pn[] = "cksvdir";
    const char *mailrdlk = RDLKDIR;

    Dout(pn, 0, "Entered\n");
    if (access(mailsave, F_OK) != CSUCCESS)
	{
	Dout(pn, 0, ":saved directory not present!\n");
	if (failsafe)
	    {
	    Dout(pn, 0, "doing failsafe processing!\n");
	    return 1;
	    }

	else
	    {
	    int omask = umask(0);
	    Dout(pn, 0, "creating :saved directory!\n");
	    /* If it fails here, there's not much we can do for now. */
	    /* The mail will be rejected later. */
	    if (mkdir(mailsave, 0775) == -1)
		Dout(pn, 0, "mkdir %s(0775) failed!\n", mailsave);
	    if (umask(omask) == (mode_t)-1)
		Dout(pn, 0, "umask(%d) failed!\n", omask);
	    if ((chown(mailsave, 0, mailgrp) == -1) &&
	        (posix_chown(mailsave) == -1))
		Dout(pn, 0, "chown(%s, 0, %d) failed!\n", mailsave, mailgrp);
	    }
	}

    if (access(mailfwrd, F_OK) != CSUCCESS)
	{
	int omask = umask(0);
	Dout(pn, 0, "creating :forward directory!\n");
	if (mkdir(mailfwrd, 0775) == -1)
	    Dout(pn, 0, "mkdir %s(0775) failed!\n", mailfwrd);
	if (umask(omask) == (mode_t)-1)
	    Dout(pn, 0, "umask(%d) failed!\n", omask);
	if ((chown(mailfwrd, 0, mailgrp) == -1) &&
	    (posix_chown(mailfwrd) == -1))
	    Dout(pn, 0, "chown(%s, 0, %d) failed!\n", mailfwrd, mailgrp);
	}

    if (access(mailrdlk, F_OK) != CSUCCESS)
	{
	int omask = umask(0);
	Dout(pn, 0, "creating :forward directory!\n");
	if (mkdir(mailrdlk, 0775) == -1)
	    Dout(pn, 0, "mkdir %s(0775) failed!\n", mailrdlk);
	if (umask(omask) == (mode_t)-1)
	    Dout(pn, 0, "umask(%d) failed!\n", omask);
	if ((chown(mailrdlk, 0, mailgrp) == -1) &&
	    (posix_chown(mailrdlk) == -1))
	    Dout(pn, 0, "chown(%s, 0, %d) failed!\n", mailrdlk, mailgrp);
	}

    return 0;
}

/*
    NAME
	trylock - try to lock the mailbox, return the mail if we can't

    SYNOPSIS
	static int trylock(Msg *pmsg, Recip *r, int wherefrom)

    DESCRIPTION
	trylock() is a small helper function which attempts to lock the mailbox.
	If it can't, it returns the mail.
*/
static int trylock(pmsg, r, wherefrom)
Msg *pmsg;
Recip *r;
int wherefrom;
{
    static const char pn[] = "trylock";
    if (!lock(s_to_c(r->name), 1))
	{
	Dout(pn, 0, "Cannot lock!\n");
	retmail(pmsg, wherefrom, E_LOCK, ":428:Cannot lock mailbox\n");
	send2mvrecip(pmsg, wherefrom, surr_len + RECIPS_FAILURE);
	return 0;
	}
    return 1;
}
