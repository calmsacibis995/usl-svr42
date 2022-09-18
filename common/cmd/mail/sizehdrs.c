/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/sizehdrs.c	1.5.2.3"
#ident "@(#)sizehdrs.c	1.7 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	sizeheader - return printed size of headers

    SYNOPSIS
	long sizeheader (Msg *pmsg)

    DESCRIPTION
	Return the amount of bytes which would be printed out by
	the headers of a message. It assumes that the message is being
	printed ordinarily instead of for a remote system. This
	routine is used by gendeliv() for deciding the size of the
	message being returned in a nondelivery report.
*/

static long mcounthdr ARGS((Hdrs *hp));
static long msg_sizehdr ARGS((Msg *pmsg));
static long for_sizehdr ARGS((Msg *pmsg));

static const int FromDelimSize = 0;	/* From headers don't have a delimiter */
static const int NormalDelimSize = 1;	/* Other headers have a space delimiter */
static const int NameValDelimSize = 2;	/* Name Value headers have a colon-space delimiter */

long sizeheader(pmsg)
Msg		*pmsg;
{
    static const char pn[] = "sizeheader";
    long ret = 0;
    Dout(pn, 0, "entered\n");

    switch (pmsg->type)
	{
	case Msg_msg:
	case Msg_deliv:
	case Msg_nondeliv:	ret = msg_sizehdr(pmsg); break;
	case Msg_forward:	ret = for_sizehdr(pmsg); break;
	default:
	    Dout(pn, 0, "Unknown message type = '%d'!\n", (int)pmsg->type);
	    errmsg(E_MBOX, ":397:Unknown message type = '%d'!\n", (int)pmsg->type);
	    break;
	}
    Dout(pn, 0, "returning %ld\n", ret);
    return ret;
}

/* Type == Msg_msg, Msg_deliv or Msg_nondeliv */
static long msg_sizehdr(pmsg)
Msg	*pmsg;
{
    long count = 1;	/* Count the newline at the end */
    Hdrs *hdr;

    /* Count the header lines */
    count += mcounthdr(pmsg->hdrinfo.hdrs[H_FROM]);
    for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	count += mcounthdr(hdr);

    return count;
}

/* Type == Msg_forward */
static long for_sizehdr(pmsg)
Msg	*pmsg;
{
    long count = 1;	/* Count the newline at the end */
    Hdrs *hdr;

    /* count all headers in the top forwarded message */
    for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	count += mcounthdr(hdr);

    /* for all other forwarded messages, count just the afwdfrom headers */
    for (pmsg = pmsg->parent; pmsg && pmsg->type == Msg_forward; pmsg = pmsg->parent)
	for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	    if (hdr->hdrtype == H_AFWDFROM)
		count += mcounthdr(hdr);

    /* for the original parent message, skip the afwdcnt and tcopy headers */
    if (pmsg)
        {
	count += mcounthdr(pmsg->hdrinfo.hdrs[H_FROM]);
	for (hdr = pmsg->hdrinfo.hdrhead; hdr; hdr = hdr->next)
	    if (hdr->hdrtype != H_AFWDCNT && hdr->hdrtype != H_TCOPY)
		count += mcounthdr(hdr);
	}

    return count;
}

/* Count a header type along with its continuation lines */
static long mcounthdr(hp)
Hdrs	*hp;
{
    long count;
    Hdrs *cp;
    int i = hp->hdrtype;
    int delimlen;

    switch (i)
	{
	case H_FROM:
	case H_RFROM:
	case H_FROM1:
	    delimlen = FromDelimSize;
	    break;

	case H_NAMEVALUE:
	    delimlen = NameValDelimSize;
	    break;

	default:
	    delimlen = NormalDelimSize;
	    break;
	}

    /* Count the header lines, +1 for the \n */
    count = strlen(hp->name) + delimlen +strlen(hp->value) + 1;

    for (cp = hp->cont; cp != (Hdrs*)NULL; cp = cp->cont)
	/* count the string and the \n at the end */
	count += strlen(cp->value) + 1;

    return count;
}
