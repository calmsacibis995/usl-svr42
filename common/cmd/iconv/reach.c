/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)iconv:reach.c	1.1.6.2"
#ident  "$Header: reach.c 1.3 91/07/02 $"

#include <stdio.h>
#include <ctype.h>
#include <pfmt.h>
#include "symtab.h"
#include "kbd.h"

extern struct node *root;
extern int numnode;
extern int nerrors;
extern int optR;

struct node *findprev();
struct node *nalloc();
static const char mapmsg[] = ":90:Map %s:\n";

/*
 * Just for kicks...Take a map, just before output, and see if
 * all 256 byte values can be generated from it.  This is the "-r"
 * option.
 */

reachout(nam, t, nt, one, cn, ncn)
	char *nam;	/* map name */
	register unsigned char *t;	/* text */
	register int nt;			/* n bytes of text */
	unsigned char *one;	/* oneone table */
	register struct cornode *cn;	/* cornodes */
	register int ncn;		/* number of cornodes */
{
	register int i;
	register unsigned char *tp;	/* tmp for "t" */
	char tab[256];	/* check table */
	int pted0, pted1, pted2;	/* see if messages printed or not */

	pted0 = pted1 = pted2 = 0;
	/*
	 * If we have a oneone table, then only results that are in
	 * it can be input to sequences.  If we have no oneone table,
	 * then all 256 values can be used in sequences.
	 */
	if (one) {
		for (i = 0; i < 256; i++)
			tab[i] = 0;
	}
	else {
		for (i = 0; i < 256; i++)
			tab[i] = 1;
	}
	if (one) {	/* check results in oneone table */
		for (i = 0; i < 256; i++) {
			++tab[*one];
			++one;
		}
	}
	/*
	 * Check here for bytes NOT in the one-one table that
	 * ARE in the nodes: they cannot be reached!
	 */
	if (one) {
		for (i = 0; i < ncn; i++) {
			if (! tab [ cn[i].c_val ]) {
				if (!pted0) {
					pfmt(stderr, MM_INFO, mapmsg, nam);
					++pted0;
				}
				if (!pted1)
					pfmt(stderr, MM_INFO, ":91:Cannot be generated by 'key', but used elsewhere:\n");
				++pted1;
				prinval(cn[i].c_val, optR);
			}
		}
	}
	if (pted1)
		fprintf(stderr, "\n");
	/*
	 * Now, check for things in TEXT.  After that, if there is
	 * anything that cannot be generated in any way, tell the user.
	 */
	tp = t;
	for (i = 0; i < nt; i++) {	/* check text */
		++tab[*tp];
		++tp;
	}
	for (i = 0; i < 256; i++) {
		if (! tab[i]) {
			if (!pted0) {
				pfmt(stderr, MM_INFO, mapmsg, nam);
				++pted0;
			}
			if (!pted2)
				pfmt(stderr, MM_INFO, ":92:Cannot be generated in any way:\n");
			++pted2;
			prinval(i, optR);
		}
	}
	if (pted2)
		fprintf(stderr, "\n");
	if (pted1 || pted2)
		return(0);	/* cannot generate some values */
	return(1);	/* all can be generated */
}
	
prinval(i, opt)
	int i, opt;
{
	if (opt) {
		switch (i) {
		case ' ':
			pfmt(stderr, MM_NOSTD, ":93:(SPACE) ", i); return;
		case '\177':
			pfmt(stderr, MM_NOSTD, ":94:(DEL) ", i); return;
		case '\r':
			fprintf(stderr, "\r ", i); return;
		case '\n':
			fprintf(stderr, "\n ", i); return;
		case '\t':
			fprintf(stderr, "\t ", i); return;
		default:
			break;
		}
		if (isprint (i))
			fprintf(stderr, "%c ", i);
		else if (iscntrl (i))
			fprintf(stderr, "^%c ", i);
		else
			fprintf(stderr, "\\%03o ", i);
		return;
	}
	fprintf(stderr, "\\%03o ", i);
}