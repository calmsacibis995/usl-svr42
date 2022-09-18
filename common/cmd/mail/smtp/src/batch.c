/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/batch.c	1.1.2.2"
#ident "@(#)batch.c	1.1 'attmail mail(1) command'"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "s_string.h"
#include "smtp.h"
#include "sched_decl.h"
#ifdef SVR4
#include <wait.h>
#endif

typedef struct node {
	char		*name;		/* host or C. file name */
	int		age;		/* age (0 = old, 1 = new) */
	struct node	*next;		/* next node in list */
	struct node	*list;		/* list of C. files for this host */
} NODE;

extern string *dest;
extern char *UPASROOT;
extern int debug;

static NODE *batchlist = (NODE *) 0;

static NODE *new_node(name, age)
char *name;
int age;
{
	register NODE *np;
	register char *cp;

	np = (NODE *) malloc(sizeof(NODE));
	if (np == (NODE *) 0)
		return (NODE *) 0;
	cp = malloc(strlen(name)+1);
	if (cp == (char *) 0) {
		free((char *) np);
		return (NODE *) 0;
	}
	(void) strcpy(cp, name);
	np->name = cp;
	np->age = age;
	np->next = (NODE *) 0;
	np->list = (NODE *) 0;
	return np;
}

static void delete_list(np)
register NODE *np;
{
	while (np) {
		register NODE *onp = np;
		np = np->next;
		if (onp->list)
			delete_list(onp->list);
		(void) free(onp->name);
		(void) free((char *) onp);
	}
}

void addtobatch(file, age)
char *file;
int age;
{
	register FILE *fp;
	register NODE *np, *cp;

	/* Find the host name that this ctl file gives */
	fp = parseline1(file);
	if (fp == NULL)
		return;
	(void) fclose(fp);

	/* Find this host name in the list */
	for (np = batchlist; np; np = np->next) {
		if (strcmp(np->name, s_to_c(dest)) == 0)
			break;
	}

	/* If none found, add one to front of list */
	if (np == (NODE *) 0) {
		np = new_node(s_to_c(dest), age);
		if (np == (NODE *) 0)
			return;
		np->next = batchlist;
		batchlist = np;
	}

	/* Add ctl file to front of list for this host */
	cp = new_node(file, age);
	if (cp == (NODE *) 0)
		return;
	cp->next = np->list;
	np->list = cp;
	if (age)
		np->age = age;
}

static void do2smtp(host, args)
char *host;
char **args;
{
	register char **cp;
	static string *cmd;
	int status, pid, n;

	cmd = s_reset(cmd);
	s_append(cmd, UPASROOT);
	s_append(cmd, SMTPBATCH);
	args[0] = s_to_c(cmd);

	log("dosmtp %s\n", host);

	/*
	 *  fork off the command
	 */
	pid = fork();
	switch (pid) {
	case -1:
		status = 1;
		break;
	case 0:
		close(0);
		execvp(args[0], args);
		_exit(1);
	default:
		while ((n = wait(&status)) >= 0)
			if (n == pid)
				break;
		if ((n < 0) || (status & 0xff))
			status = -1;
		else
			status = (status>>8) & 0xff;
		break;
	}

	if (status == 0)
		log("success\n");
	else
		log("fail %d\n", status);
}

void dobatch()
{
	register NODE *np, *cp;
	register int ai;
	char *args[256];

	for (np = batchlist; np; np = np->next) {
		if (np->age) {
			ai = 1;
			if (debug)
				args[ai++] = "-D";
			for (cp = np->list; cp; cp = cp->next) {
				args[ai++] = cp->name;
				if (ai >= 254) {
					args[ai] = (char *) 0;
					do2smtp(np->name, args);
					ai = 1;
					if (debug)
						args[ai++] = "-D";
				}
			}
			if (ai > (1 + debug)) {
				args[ai] = (char *) 0;
				do2smtp(np->name, args);
			}
		}
	}
	delete_list(batchlist);
	batchlist = (NODE *) 0;
}
