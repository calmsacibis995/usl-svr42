/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/msetup_ex.c	1.1.2.2"
#ident "@(#)msetup_ex.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	msetup_exec - set up an argument vector from a message

    SYNOPSIS
	char **msetup_exec(Msg *pmsg, int whereexec)

    DESCRIPTION
	Parse the surrogate command pointed to by pmsg and
	whereexec into a list of arguments appropriate
	for use by exec().
*/

#define MAXARGS 512
#define CHUNKSIZE 128
static char	*_argvec[MAXARGS]; /* enough to begin with */
static int	argcnt = MAXARGS-1;
static char	**argvec = &_argvec[0];

char **msetup_exec(pmsg, whereexec)
Msg	*pmsg;
int	whereexec;
{
    register int i = 0;
    Recip *r;

    r = recips_head(pmsg, whereexec)->next;
    i = parse_execarg(s_to_c(r->cmdl), i, &argcnt, &argvec, CHUNKSIZE, _argvec);
    for ( ; r && r->cmdr; r = r->next)
	i = parse_execarg(s_to_c(r->cmdr), i, &argcnt, &argvec, CHUNKSIZE, _argvec);
    argvec[i] = (char *)NULL;

    return (i == 0) ? (char**)NULL : argvec;
}
