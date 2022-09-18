/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/savehdrs.c	1.7.2.3"
#ident "@(#)savehdrs.c	2.14 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	savehdrs - save a header in a message's header list

    SYNOPSIS
	void save_a_hdr(Hdrinfo *phdrinfo, const char *val, int hdrtype, const char *name)
	void save_mta_hdr(Hdrinfo *phdrinfo, const char *val, int hdrtype, const char *name)
	void save_cont_hdr(Hdrinfo *phdrinfo, const char *val)
	void save_a_txthdr(Hdrinfo *phdrinfo, char *val, int hdrtype)

    DESCRIPTION
	Save info on each header line. The header is of the type "hdrtype".
	The name is only used for hdrtype==H_NAMEVALUE.

	Save_a_hdr() is passed a header which is to be saved at the end of the
	header list.

	Save_mta_hdr() is passed a header which is to be saved in MTA order.

	Save_cont_hdr() is passed a continuation header to be added after
	the header previously added via save_a_hdr() or save_mta_hdr().

	Save_a_txthdr() is passed a header which needs to be split apart
	before being stored. (Newlines will also be stripped.)
*/

static void save_hdr_info ARGS((Hdrinfo *phdrinfo, Hdrs *nhp, int hdrtype));

void save_a_hdr(phdrinfo, val, hdrtype, name)
Hdrinfo	*phdrinfo;
const char *val;
int hdrtype;
const char *name;
{
    Hdrs *nhp = new_Hdrs(hdrtype, name, val);
    if ((hdrtype == H_FROM) || (hdrtype == H_RFROM))
	{
	/* Keep these headers separate. There will only be one of each. */
	phdrinfo->hdrs[hdrtype] = nhp;
	}

    else
	{
	if (phdrinfo->hdrhead)
	    {
	    /* Add new element to tail of list */
	    Hdrs *ohp = phdrinfo->hdrtail;
	    ohp->next = nhp;
	    nhp->prev = ohp;
	    phdrinfo->hdrtail = nhp;
	    }

	else
	    {
	    /* Empty list so far. New element goes first */
	    phdrinfo->hdrhead = phdrinfo->hdrtail = nhp;
	    }

	save_hdr_info(phdrinfo, nhp, hdrtype);
	}

    /* remember the last header entered for use in continuation lines */
    phdrinfo->last_hdr = nhp;
}

void save_mta_hdr(phdrinfo, val, hdrtype, name)
Hdrinfo	*phdrinfo;
const char *val;
int hdrtype;
const char *name;
{
    Hdrs *ohp, *nhp;
    /*
    4 cases:
	no headers yet
	    same as save_a_hdr()
	found a header of that type,
	    go to last header of that type in the line of headers of that type
	    append new header there
	no header of that type yet, but there is a header in MTA order afterwards
	    prepend new header in front of that header
	no header of that type yet, and none in MTA order afterwards
	    same as save_a_hdr()
    */

    /* no headers yet? */
    if (!phdrinfo->hdrhead)
	{
	save_a_hdr(phdrinfo, val, hdrtype, name);
	return;
	}

    ohp = phdrinfo->hdrs[hdrtype];
    if (!ohp)
	{
	/* no header of that type yet, ... */
	int i;
	/* is there a header afterwards? */
	for (i = hdrtype + 1; i < H_CONT; i++)
	    if (phdrinfo->hdrs[i])
		{
		/* ... prepend to phdrinfo->hdrs ... */
		Hdrs *prevhp;
		ohp = phdrinfo->hdrs[i];
		prevhp = ohp->prev;
		nhp = new_Hdrs(hdrtype, name, val);

		if (!prevhp)		/* first header in entire list */
		    {
		    phdrinfo->hdrhead = nhp;
		    nhp->next = ohp;
		    ohp->prev = nhp;
		    }

		else
		    {			/* splice nhp into list */
		    nhp->next = prevhp->next;
		    nhp->prev = prevhp;
		    prevhp->next = nhp;
		    if (!nhp->next)
			phdrinfo->hdrtail = nhp;
		    }

		save_hdr_info(phdrinfo, nhp, hdrtype);

		/* remember the last header entered for use in continuation lines */
		phdrinfo->last_hdr = nhp;
		return;
		}

	/* no header in MTA order after that type yet */
	save_a_hdr(phdrinfo, val, hdrtype, name);
	return;
	}

    /* found a header of that type */
    while (ohp->next && ohp->next->hdrtype == hdrtype)
	ohp = ohp->next;

    /* ... append to ohp ... */
    nhp = new_Hdrs(hdrtype, name, val);
    nhp->next = ohp->next;
    ohp->next = nhp;
    nhp->prev = ohp;

    if (nhp->next)
	nhp->next->prev = nhp;
    else
	phdrinfo->hdrtail = nhp;

    save_hdr_info(phdrinfo, nhp, hdrtype);

    /* remember the last header entered for use in continuation lines */
    phdrinfo->last_hdr = nhp;
}

void save_a_txthdr(phdrinfo, val, hdrtype)
Hdrinfo	*phdrinfo;
char	*val;
int	hdrtype;
{
    char delim, *hdrval;

    trimnl(val);

    /* determine the type of the delimiter. */
    switch (hdrtype)
	{
	case H_CONT:
	    save_cont_hdr(phdrinfo, val);
	    return;

	case H_FROM:
	case H_RFROM:
	case H_FROM1:
	    delim = ' ';
	    break;

	default:
	    delim = ':';
	    if (phdrinfo->fnuhdrtype == 0)
		phdrinfo->fnuhdrtype = hdrtype;
	    break;
	}

    /* Find the text after the delimiter. */
    if ((hdrval = strchr(val,delim)) != (char*)NULL)
	{
	*hdrval++ = '\0';
	hdrval = (char*)skipspace(hdrval);
	}

    else
	hdrval = val;

    save_a_hdr(phdrinfo, hdrval, hdrtype, val);
}

void save_cont_hdr(phdrinfo, val)
Hdrinfo	*phdrinfo;
const char	*val;
{
    static const char pn[] = "save_cont_hdr";
    Hdrs *ohp = phdrinfo->last_hdr;

    if (ohp == (Hdrs*)NULL)
	{
	/* This shouldn't happen.....? */
	/* No headline has been found so far. How */
	/* did we think this is a continuation of something? */
	if (debug > 0)
	    {
	    Dout(pn, 0, "H_CONT with no hdr yet\n");
	    abort();
	    }

	/* Throw it on the floor... (!) */
	return;
	}

    /* Since we ONLY walk down 'cont' chains, */
    /* we only need forward links. */
    while (ohp->cont != (Hdrs*)NULL)
	ohp = ohp->cont;

    /* Add this one to the end of the list... */
    ohp->cont = new_Hdrs(H_CONT, (char*)0, val);
    return;
}

static void save_hdr_info(phdrinfo, nhp, hdrtype)
Hdrinfo	*phdrinfo;
Hdrs *nhp;
int	hdrtype;
{
    /* Save type of first non-UNIX header line */
    if ((hdrtype != H_FROM1) && (phdrinfo->fnuhdrtype == 0))
	phdrinfo->fnuhdrtype = hdrtype;

    /* Remember that these header line types were in orig. msg.  */
    switch (hdrtype)
	{
	case H_AFWDFROM:
	    phdrinfo->orig_aff++;
	    phdrinfo->affcnt++;
	    break;
	case H_RECEIVED:
	    phdrinfo->orig_rcv++;
	    break;
	case H_TCOPY:
	    phdrinfo->orig_tcopy++;
	    break;
	}

    /* remember first header of this type */
    if (!phdrinfo->hdrs[hdrtype])
	phdrinfo->hdrs[hdrtype] = nhp;
}

