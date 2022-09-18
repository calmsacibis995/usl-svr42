/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/add_recip.c	1.9.2.2"
#ident "@(#)add_recip.c	2.11 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	add_recip, madd_recip - add recipients to recipient list

    SYNOPSIS
	int add_recip(Msg *pmsg, char *name, int checkdups,
	    Recip *parent, int fullyresolved, int stripbangs,
	    int wherefrom, int wheretemp, int whereto)
	int madd_recip(Msg *pmsg, char *name, int checkdups,
	    Recip *parent, int fullyresolved, int stripbangs,
	    int wherefrom, int wheretemp, int whereto)

    DESCRIPTION
	add_recip() adds the name to a message's recipient linked list. If
	"checkdups" is set, it first checks to make certain that the name is not in
	any of the message's lists. The parent is the name being translated. It and
	"fullyresolved" are passed to new_Recip().
	
	The name is normally added the linked list pointed to by "whereto", which is
	typically list 0. If a parent's parent is the same as the current name, and
	that name is on the RECIPS_DONE list, move that parent back onto an
	"appropriate" active list. If the "appropriate" list is the same as
	"wherefrom", and "whereto" is 0, then use "wheretemp" instead.

	If "stripbangs" is set, leading !'s will be removed.

	madd_recips() is given a list of names separated by white space. Each name
	is split off and passed to add_recips().

    RETURN
	1 - one of the names translated to is the same as the name
	    translated from
*/

int add_recip (pmsg, name, checkdups, parent, fullyresolved, stripbangs, wherefrom, wheretemp, whereto)
Msg	*pmsg;
char	*name;
int	checkdups;
Recip	*parent;
int	fullyresolved;
int	stripbangs;
int	wherefrom;
int	wheretemp;
int	whereto;
{
    static const char pn[] = "add_recip";
    Reciplist		*plist = &pmsg->preciplist[whereto];
    register int	i;
    register Recip	*r, *rp, *l;

    if ((name == (char *)NULL) || (*name == '\0')) {
	Tout(pn, "translation to NULL name ignored\n");
	return 0;
    }

    if (*skiptospace(name) != '\0') {
	Tout(pn, "'%s' not added due to imbedded spaces\n", name);
	return 0;
    }

    /* if necessary, get rid of leading !'s */
    if (stripbangs)
	while (*name == '!')
	    name++;

    /* look for a duplicate */
    if (checkdups == TRUE) {
	/* Look at the parents to find the name. If found, and that */
	/* parent is in the DONE category, move the parent back */
	/* onto its next surrogate queue. */
        Tout(pn, "looking for parents which are the same as %s\n", name);
	for (r = parent->parent; r != (Recip*)NULL; r = r->parent)
	    if (strcmp(s_to_c(r->name), name) == 0) {
		/* Is it in the DONE category? */
		if (r->lastsurrogate != -1) {
		    /* Find the DONE recipient. */
		    Dout(pn, 20, "found %s, lastsurrogate == %d\n", s_to_c(r->name), r->lastsurrogate);
		    for (l = recips_head(pmsg, surr_len + RECIPS_DONE); l->next != (Recip*)NULL; l = l->next)
			if (r == l->next) {
			    Reciplist *nlist;
			    if (flgd || flglb)
			        pfmt(stdout, MM_INFO, ":347:Translation: '%s' -> '%s'\n", s_to_c(parent->name), name);
			    Tout(pn, "%s reinserted at %d\n", s_to_c(r->name), r->lastsurrogate);

			    /* Remove the recipient from the DONE list. */
			    l->next = r->next;
			    if (pmsg->preciplist[surr_len+RECIPS_DONE].last_recip == r)
			        pmsg->preciplist[surr_len+RECIPS_DONE].last_recip = l;

			    /* Patch it into end of whereto, r->lastsurrogate+1 or wheretemp. */
			    if (whereto != 0)
			        nlist = &pmsg->preciplist[whereto];
			    else if (wherefrom == r->lastsurrogate + 1)
				nlist = &pmsg->preciplist[wheretemp];
			    else
				nlist = &pmsg->preciplist[r->lastsurrogate + 1];
			    nlist->last_recip->next = r;
			    nlist->last_recip = r;
			    r->next = (Recip *)NULL;
			    r->lastsurrogate = -1;
			    return 0;
			}
		}
		break;
	    }

	/* Look at all recipients for the duplicate. */
        Tout(pn, "looking for any recipient which is the same as %s\n", name);
	for (i = 0; i < surr_len + RECIPS_MAX; i++)
	    for (r = recips_head(pmsg, i)->next; (r != (Recip*)NULL); r = r->next)
		if (strcmp(s_to_c(r->name), name) == 0) {
		    Tout(pn, "duplicate recipient '%s' not added to list\n", name);
		    if ((flgd || flglb) && parent)
			pfmt(stdout, MM_INFO, ":346:Translation ignored: duplicate recipient '%s'\n", name);
		    return parent && (strcmp(s_to_c(parent->name), name) == 0);
		}
    }

    if ((flgd || flglb) && parent)
	pfmt(stdout, MM_INFO, ":347:Translation: '%s' -> '%s'\n", s_to_c(parent->name), name);

    /* allocate space for the recipient name */
    rp = new_Recip(name, parent, fullyresolved);

    plist->last_recip->next = rp;
    plist->last_recip = rp;
    Tout(pn, "'%s' added to recipient list\n", name);

    return parent && (strcmp(s_to_c(parent->name), name) == 0);
}

int madd_recip (pmsg, namelist, checkdups, parent, fullyresolved, stripbangs, wherefrom, wheretemp, whereto)
Msg	*pmsg;
char	*namelist;
int	checkdups;
Recip	*parent;
int	fullyresolved;
int	stripbangs;
int	wherefrom;
int	wheretemp;
int	whereto;
{
    register char	*name;
    register int	rc = 0;
    for (name = strtok(namelist, " \t"); name; name = strtok((char*)0, " \t"))
	rc |= add_recip(pmsg, name, checkdups, parent, fullyresolved, stripbangs, wherefrom, wheretemp, whereto);
    return rc;
}
