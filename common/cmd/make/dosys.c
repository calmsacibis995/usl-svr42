/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)make:dosys.c	1.19"

#include "defs"
#include <signal.h>
#include <errno.h>

#ifndef MAKE_SHELL
#define MAKE_SHELL	"/usr/bin/sh"
#endif

extern errno;

/*
**	Declate local functions and make LINT happy.
*/

static int	any_metas();
static int	doshell();
static int	await();
static void	doclose();
static int	doexec();
static int waitpid;

int
dosys(comstring, nohalt, Makecall, tarp)	/* do the system call */
CHARSTAR comstring;
int	nohalt, Makecall;
NAMEBLOCK tarp;
{
	register CHARSTAR str = comstring;

	while (	*str == BLANK || *str == TAB ) 
		str++;
	if (!*str)
		return(-1);	/* no command */

	if (IS_ON(NOEX) && !Makecall)
		return(0);

	if(IS_ON(BLOCK)){
		if(!tarp->tbfp)
			(void)open_block(tarp);
		else
			(void)fflush(tarp->tbfp);

		(void)fflush(stdout);
		(void)fflush(stderr);
			
	}

	if (any_metas(str))
		return( doshell(str, nohalt, tarp) );
	else
		return( doexec(str, tarp) );
}


static int
any_metas(s)		/* Are there are any Shell meta-characters? */
register CHARSTAR s;
{
	while ( *s )
		if ( funny[(unsigned char) *s++] & META )
			return(YES);
	return(NO);
}


static int
doshell(comstring, nohalt, tarp)
CHARSTAR comstring;
int	nohalt;
NAMEBLOCK tarp;
{
	int	fork(), execl(), execlp();

void	enbint();


#ifdef MKDEBUG
	if (IS_ON(DBUG)) 
		printf("comstring=<%s>\n",comstring);
#endif
	if ( !(waitpid = fork()) ) {
		void	setenv();
		char *getenv(), *myshell;
		enbint(SIG_DFL);
		doclose(tarp);
		setenv();

		if (((myshell = getenv("SHELL")) == NULL) || (*myshell == CNULL))
			myshell = MAKE_SHELL;
		if (*myshell == '/')
			(void)execl(myshell, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
		else
			(void)execlp(myshell, "sh", (nohalt ? "-c" : "-ce"), comstring, 0);
		if(errno == E2BIG)
			fatal("couldn't load shell: Argument list too long (bu22)");
		else
			fatal("couldn't load shell (bu22)");
	} else if ( waitpid == -1 )
		fatal("couldn't fork");

	return( await() );
}


static int
await()
{
	register int	pid;
	int	wait(), status;
	void	intrupt();
	void	enbint();

	enbint(intrupt);
	if(IS_ON(PAR))
		return(waitpid);

	while ((pid = wait(&status)) != waitpid)
		if (pid == -1)
			fatal("bad wait code (bu23)");
	return(status);
}


static void
doclose(tarp)	/* Close open directory files before exec'ing */
NAMEBLOCK tarp;
{
	register OPENDIR od = firstod;
	for (; od; od = od->nextopendir)
		if ( od->dirfc )
			(void)closedir(od->dirfc);

	if(IS_ON(BLOCK)){

		
		(void)close(fileno(stdout));
		
		if(dup(fileno(tarp->tbfp)) < 0 )
			fatal1("Fail to dup block file(%d)", fileno(tarp->tbfp));

		(void)close(fileno(stderr));

		if(dup(fileno(tarp->tbfp)) < 0 )
			fatal1("Fail to dup block file(%d)", fileno(tarp->tbfp));

		(void)fclose(tarp->tbfp);
	}
}


static int
doexec(str, tarp)
register CHARSTAR str;
NAMEBLOCK tarp;
{
	int	execvp();
	void	enbint();

	if ( *str == CNULL )
		return(-1);	/* no command */
	else {
		int	fork();
		register CHARSTAR t = str, *p;
		int incsize = ARGV_SIZE, aryelems = incsize, numelems = 0;
		CHARSTAR *argv = (CHARSTAR *) ck_malloc(aryelems * sizeof(CHARSTAR));

		p = argv;
		for ( ; *t ; ) {
			*p++ = t;
			if (++numelems == aryelems) {
				aryelems += incsize;
				argv = (CHARSTAR *)
					realloc(argv, aryelems * sizeof(CHARSTAR));
				if (!argv) fatal1("doexec: out of memory");
				p = argv + (aryelems - incsize);
			}
			while (*t != BLANK && *t != TAB && *t != CNULL)
				++t;
			if (*t)
				for (*t++ = CNULL; *t == BLANK || *t == TAB; ++t)
					;
		}

		*p = NULL;

		if ( !(waitpid = fork()) ) {
			void	setenv();
			enbint(SIG_DFL);
			doclose(tarp);
			setenv();
			(void)execvp(str, argv);
			if(errno == E2BIG)
				fatal1("cannot load %s : Argument list too long (bu24)", str);
			else
				fatal1("cannot load %s (bu24)", str);
		} else if ( waitpid == -1 )
			fatal("couldn't fork");

		free(argv);
		return( await() );
	}
}
