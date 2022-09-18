/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:myfopen.c	1.3.2.4"
#ident "@(#)myfopen.c	1.10 'attmail mail(1) command'"
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"

#undef	fopen
#undef	fclose

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Local version of fopen() and fclose(). These maintain a list of
 * file pointers which can be run down when we need to close
 * all files, such as before executing external commands.
 */

static void addnode ARGS((FILE *fp));
static void delnode ARGS((FILE *fp));

/* add a new node to the list */
static void
addnode(fp)
FILE *fp;
{
	register NODE *new;

	if ((new = (NODE *)malloc(sizeof(NODE))) == (NODE *)NULL) {
		(void) pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		exit(3);
	}
	new->fp = fp;
	new->next = fplist;
	fplist = new;
}

/* delete the given NODE from the list */
static void
delnode(fp)
FILE *fp;
{
	register NODE *cur, *prev;

	for (prev = cur = fplist; cur != (NODE *)NULL; prev = cur, cur = cur->next) {
		if (cur->fp == fp) {
			if (cur == fplist) {
				fplist = cur->next;
			} else {
				prev->next = cur->next;
			}
			free((char*)cur);
			break;
		}
	}
}

/* open a file and record it */
FILE *
my_fopen(filename, mode)
const char *filename, *mode;
{
	FILE *fp;

	if ((fp = fopen(filename, mode)) != (FILE *)NULL)
		addnode(fp);
	return(fp);
}

/* close a file and remove our record of it */
int
my_fclose(iop)
register FILE *iop;
{
	delnode(iop);
	return fclose(iop);
}
