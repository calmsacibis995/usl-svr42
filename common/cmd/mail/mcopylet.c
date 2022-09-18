/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mcopylet.c	1.6.2.3"
#ident "@(#)mcopylet.c	1.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	mcopylet - copy a given message to a file pointer

    SYNOPSIS
	int mcopylet(Msg *pmsg, FILE *f, CopyLetFlags remote)

    DESCRIPTION
	Mcopylet() will copy the letter "pmsg" to the
	given file pointer. It does it appropriately
	for the type of message.

    RETURNS
	TRUE on a completely successful copy.
*/

static int copyrest ARGS((Msg *pmsg, FILE *sfp, CopyLetFlags remote));
static int mcopyhdr ARGS((Hdrs *hp, FILE *sfp));
static int msg_copylet ARGS((Msg *pmsg, FILE *sfp, CopyLetFlags remote));
static int del_copylet ARGS((Msg *pmsg, FILE *sfp, CopyLetFlags remote));
static int for_copylet ARGS((Msg *pmsg, FILE *sfp, CopyLetFlags remote));
static int non_copylet ARGS((Msg *pmsg, FILE *sfp, CopyLetFlags remote));
static Hdrs *print_forward_let ARGS((Msg *pmsg, CopyLetFlags remote, FILE *sfp, int top, Hdrs *tcopy));

static const char *FromDelim = "";		/* From headers don't have a delimiter */
static const char NormalDelim[] = " ";		/* Other headers have a space delimiter */
static const char NameValueDelim[] = ": ";	/* We have to restore the : as well as the space */

static volatile sig_atomic_t pipecatcher;

/* Catch SIG_PIPE and stop the writing. */
/* ARGSUSED */
static void catchsigpipe(i)
int i;
{
    pipecatcher = 1;
}

int mcopylet(pmsg, sfp, remote)
Msg		*pmsg;
FILE		*sfp;
CopyLetFlags	remote;
{
    static const char pn[] = "mcopylet";
    int ret = 0;
    void (*savsig)();
    Dout(pn, 0, "entered, remote=%s, type=%d\n",
	(remote == REMOTE)   ? "REMOTE" :
	(remote == ORDINARY) ? "ORDINARY" :
			       "UNKNOWN", pmsg->type);

    pipecatcher = 0;
    savsig = signal(SIGPIPE, catchsigpipe);

    switch (pmsg->type)
	{
	case Msg_msg:		ret = msg_copylet(pmsg, sfp, remote); break;
	case Msg_forward:	ret = for_copylet(pmsg, sfp, remote); break;
	case Msg_deliv:		ret = del_copylet(pmsg, sfp, remote); break;
	case Msg_nondeliv:	ret = non_copylet(pmsg, sfp, remote); break;
	default:
	    Dout(pn, 0, "Unknown message type = '%d'!\n", (int)pmsg->type);
	    errmsg(E_MBOX, ":397:Unknown message type = '%d'!\n", (int)pmsg->type);
	    break;
	}

    (void) signal(SIGPIPE, savsig);
    /* doublecheck pipecatcher just in case it wasn't reflected */
    /* in the return value of *_copylet(). */
    if (pipecatcher)
	{
	Dout(pn, 40, "pipecatcher set, setting ret=>0\n");
	ret = 0;
	}
    Dout(pn, 0, "returning %d\n", ret);
    return ret;
}

/* Type == Msg_msg */
/* A regular message. */
static int msg_copylet(pmsg, sfp, remote)
Msg		*pmsg;
FILE		*sfp;
CopyLetFlags	remote;
{
    register Hdrs *hdr;

    /* Print the first From/remote-from header line */
    if (!mcopyhdr(pmsg->hdrinfo.hdrs[(remote == REMOTE) ? H_RFROM : H_FROM], sfp))
	return 0;

    /* copy all the first header */
    for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	if (!mcopyhdr(hdr, sfp))
	    return 0;

    return copyrest(pmsg, sfp, remote);
}

/* Write out the header */
static int mcopyhdr(hp, sfp)
Hdrs		*hp;
FILE		*sfp;
{
    static const char pn[] = "mcopyhdr";
    const char *delim;
    register Hdrs *cp;

    /* Pick a tag and delimiter */
    switch (hp->hdrtype)
	{
	case H_NAMEVALUE:
	    delim = NameValueDelim;
	    break;
	case H_FROM:
	case H_RFROM:
	case H_FROM1:
	    delim = FromDelim;
	    break;
	default:
	    delim = NormalDelim;
	    break;
	}

    /* Print the header */
    (void) fputs(hp->name, sfp);
    (void) fputs(delim, sfp);
    (void) fputs(hp->value, sfp);
    (void) putc('\n', sfp);

    /* Now print the continuation lines */
    for (cp = hp->cont; cp != (Hdrs*)NULL; cp = cp->cont)
	(void) fprintf(sfp, "%s\n", cp->value);

    /* check for errors */
    if (ferror(sfp) || pipecatcher)
	{
	Dout(pn, 40, "fprintf failed\n");
	return 0;
	}

    return 1;
}

/* Write out the body */
static int copyrest(pmsg, sfp, remote)
Msg		*pmsg;
FILE		*sfp;
CopyLetFlags	remote;
{
    static const char pn[] = "copyrest";
    if (fwrite("\n", sizeof(char), 1, sfp) != 1)
	{
	Dout(pn, 40, "fwrite failed\n");
	return 0;
	}

    if (pmsg->tmpfile.tmpf)
	{
	rewind(pmsg->tmpfile.tmpf);
	if (copystream(pmsg->tmpfile.tmpf, sfp) == 0)
	    {
	    Dout(pn, 40, "copystream failed\n");
	    return 0;
	    }
	/* The newline is written out to guarantee that the From header */
	/* of the next message begins at the beginning of a line. */
	if (pmsg->msgsize > 0 && (remote == ORDINARY) && fwrite("\n", sizeof(char), 1, sfp) != 1)
	    {
	    Dout(pn, 40, "fwrite failed\n");
	    return 0;
	    }
	}

    if (fflush(sfp) == EOF)
	{
	Dout(pn, 40, "fflush failed\n");
	return 0;
	}

    return 1;
}

/* Type == Msg_deliv */
/* Acknowledgement message. All we have are headers, */
/* so print them out like a regular message. */
static int del_copylet(pmsg, sfp, remote)
Msg		*pmsg;
FILE		*sfp;
CopyLetFlags	remote;
{
    return msg_copylet(pmsg, sfp, remote);
}

/* Type == Msg_forward */
/* A forwarded message. All but the top parent message include */
/* only the Auto-Forwarded-From and other sundry forwarding */
/* information; most headers are kept in the original message. */
static int for_copylet(pmsg, sfp, remote)
Msg		*pmsg;
FILE		*sfp;
CopyLetFlags	remote;
{
    register Msg *pm;

    Hdrs *svhdr = print_forward_let(pmsg, remote, sfp, 1, pmsg->hdrinfo.hdrs[H_TCOPY]);
    if (!svhdr)
	return 0;

    /* Find the ultimate parent ... */
    for (pm = pmsg; pm->parent != 0; pm = pm->parent)
	if (pm->type != Msg_forward)
	    break;

    /* ... and print its body. */
    switch (pm->type)
	{
	case Msg_nondeliv:	/* Output the parent message with all its headers. */
	    return mcopylet(pm, sfp, ORDINARY);
	case Msg_msg:
	case Msg_deliv:
	case Msg_forward:	/* this shouldn't be able to happen */
	default:		/* this shouldn't be able to happen */
	    return copyrest(pm, sfp, remote);
	}
}

/*
    Print the headers for a forwarded message. We have to dip down to the
    parent which is being forwarded to get most of the headers. To that we
    have to add in all of the Auto-Forwarded-From headers from all of the
    forwarding that's occurred. We follow with the remaining headers from
    the original parent. Special handling of H_TCOPY and H_AFWDCOUNT must
    also happen. The H_TCOPY header from the top-most forwarded message
    must replace the H_TCOPY header in the original parent. (It is guaranteed
    that the one will be present only if the other is also present.) The
    H_AFWDCOUNT header associated with the top-most forwarded message
    will be printed and all others ignored.
*/
static Hdrs *print_forward_let(pmsg, remote, sfp, top, tcopy)
Msg		*pmsg;
CopyLetFlags	remote;
FILE		*sfp;
int		top;
Hdrs		*tcopy;
{
    static Hdrs bogushdr;
    Hdrs *hdr;
    int affspot;

    if (pmsg->type == Msg_forward)
	{
	/* print all headers prior to this message's headers */
	Hdrs *svhdr = print_forward_let(pmsg->parent, remote, sfp, 0, tcopy);
	if (!svhdr)
	    return 0;

	/* now print this message's headers */
	for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	    /* skip all Auto-forwarded-count headers except the top one */
	    if (hdr->hdrtype == H_AFWDCNT)
		{
		if (top && !mcopyhdr(hdr, sfp))
		    return 0;
		}

	    else if (hdr->hdrtype == H_TCOPY)
		/* EMPTY */ ;

	    else if (!mcopyhdr(hdr, sfp))
		return 0;

	/* Only the top message will carry on printing the */
	/* rest of the headers from the original parent. */
	if (top && svhdr != &bogushdr)
	    for (hdr = svhdr; hdr; hdr = hdr->next)
		if (hdr->hdrtype == H_TCOPY)
		    {
		    if (tcopy && !mcopyhdr(tcopy, sfp))
			return 0;
		    }

		else if (hdr->hdrtype == H_AFWDCNT)
		    /* EMPTY */ ;

		else if (!mcopyhdr(hdr, sfp))
		    return 0;

	return svhdr;
	}

    /* Print other types of headers. */
    /* First find out where to put the H_AFWDFROM headers. */
    affspot = pckaffspot(&pmsg->hdrinfo, 0);

    /* Print the first From/remote-from header line */
    if (!mcopyhdr(pmsg->hdrinfo.hdrs[(remote == REMOTE) ? H_RFROM : H_FROM], sfp))
	return 0;

    /* Now, print all headers up to that type. */
    for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	if (hdr->hdrtype == affspot)
	    break;

	else if (hdr->hdrtype == H_TCOPY)
	    {
	    if (tcopy && !mcopyhdr(tcopy, sfp))
		return 0;
	    }

	else if (hdr->hdrtype == H_AFWDCNT)
	    /* EMPTY */ ;

	else if (!mcopyhdr(hdr, sfp))
	    return 0;

    /* Now print all headers of that type. */
    for ( ; hdr && (hdr->hdrtype == affspot); hdr = hdr->next)
	if (!mcopyhdr(hdr, sfp))
	    return 0;

    /* Return the next header in the list. */
    /* If we ran out of header, return a bogus header that can be */
    /* checked for separately from a null header, which indicates */
    /* that we couldn't print. */
    if (hdr)
	return hdr;
    return &bogushdr;
}

/* Type == Msg_nondeliv */
/* NAK message. Copy the NAK headers, followed by the message being returned. */
static int non_copylet(pmsg, sfp, remote)
Msg		*pmsg;
FILE		*sfp;
CopyLetFlags	remote;
{
    return msg_copylet(pmsg, sfp, remote) &&
	mcopylet(pmsg->parent, sfp, ORDINARY);
}
