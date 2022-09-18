/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)shl:misc.c	1.5.8.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/shl/misc.c,v 1.1 91/02/28 20:09:30 ccs Exp $"
#include 	"defs.h"
#include	<unistd.h>
#include	<utmp.h>

char	*prs();
char 	*getenv();
char 	*def_shell	= "/bin/sh";

get_new_layer(name)
	char *name;
{
	int i;
	int free_layer = 0;

	for (i=1; i<= max_index; ++i)
	{
		if (layers[i])
		{
			if (strcmp(name, layers[i]->name) == 0)
			{
				pfmt(stderr, MM_ERROR, 
					":15:Layer %s already exists\n",
					prs(name));
				return(0);
			}
		}
		else
		{
			if (free_layer == 0)
				free_layer = i;
		}
	}

	if (free_layer == 0 && max_index == MAX_LAYERS - 1)
	{
		pfmt(stderr, MM_ERROR, ":16:Only %d layers allowed\n", 
			max_index);
		return(0);
	}
	else
		return(free_layer > 0 ? free_layer : ++max_index);
}
	

lookup_layer(name)
	char *name;
{
	int i;
	char tmpname[4];

	if (strlen(name) == 1 && name[0] > '0' && name[0] < '8')
	{
		sprintf(tmpname, "(%s)", name);
		name = tmpname;
	}

	for (i = 1; i <= max_index; ++i)
	{
		if (layers[i] != 0)
			if (strcmp(name, layers[i]->name) == 0)
				return(i);
	}




	pfmt(stderr, MM_ERROR, ":17:Layer %s not found\n", prs(name));
	return(0);
}


free_layer(i)
	int i;
{
	free((char *)layers[i]);
	layers[i] = 0;
	if (i == max_index)
		max_index--;
}


pid_t
spawn_shell(index)
	int index;
{

	pid_t i;
	int j;
	char *shell;
	int fd;

	if (pipe(fildes) == SYSERROR)
	{
		pfmt(stderr, MM_ERROR, 
			":18:Cannot open pipe for synchronization: %s\n",
			strerror (errno));
		return(0);
	}

	switch (i = fork())
	{
		case 0:					/* child */
			setpgrp();			/* new process group */

			if (setgid(gid) ==SYSERROR || setuid(uid) == SYSERROR)
			{
				pfmt(stderr, MM_ERROR, 
					":19:Cannot set group or user id: %s\n",
					strerror (errno));
				exit(10);
			}

			close(fildes[0]);
			close(0);			/* stdin */

			set_dev(chan + index);
			if ((fd = open(cntlf, O_RDWR))  == SYSERROR)	
			{
				pfmt(stderr, MM_ERROR, 
					":20:Virtual tty open failed: %s\n",
					strerror (errno));
				exit(6);
			}
			else if (fd != 0)
			{
				pfmt(stderr, MM_ERROR, 
					":21:Error on virtual tty open\n");
				exit(2);
			}

			if (write(fildes[1], "x", 1) == SYSERROR)
			{
				pfmt(stderr, MM_ERROR, 
					":22:Synchronization problem: %s\n",
					strerror (errno));
				exit(11);
			}

			close(1);		/* stdout */
			dup(0);

			close(2);		/* stderr */
			dup(0);

			for (j = 3; j < 20; ++j)
				close(j);


			if ((shell = getenv("SHELL")) == 0)
				shell = def_shell;


			if (ioctl(fd, TCSETAW, &ttysave) == SYSERROR)
			{
				pfmt(stderr, MM_ERROR, badioctl, strerror(errno));
				exit(8);
			}

			setenv("PS1", layers[index]->name);

			restore_sig();

			execlp(shell, shell, "-i", 0);

			pfmt(stderr, MM_ERROR, 
				":23:Cannot execute %s: %s\n", shell, 
				strerror (errno));
			exit(3);

		case (pid_t)SYSERROR:
			pfmt(stderr, MM_ERROR,
				":24:fork() failed: %s\n",
				strerror (errno));
			return(0);

		default:			/* parent */
			close(fildes[1]);
			return(i);
	}
}



char *prs(s)
	char *s;
{
	static char buffout[128];
	register char *ptr = buffout;
	register char c;

	while (*s != '\0') 
	{
		c = (*s & 0177) ;
		
		/* translate a control character into a printable sequence */

		if (c < '\040') 
		{	/* assumes ASCII char */
			*ptr++ = '^';
			*ptr++ = (c + 0100);	/* assumes ASCII char */
		}
		else if (c == 0177) 
		{	/* '\0177' does not work */
			*ptr++ = '^';
			*ptr++ = '?';
		}
		else 
		{	/* printable character */
			*ptr++ = c;
		}

		++s;
	}

	*ptr = '\0';
	return(buffout);
}

extern char **environ;
char *new_env[512];
char *malloc();
char *strcat();

setenv(name, value)
	char *name;
	char *value;
{
	char **e = environ;	
	char *buffer;

	int len;
	int i = 0;
	int flag = 0;	

	len = strlen(name) + 1;
	buffer = malloc(len + strlen(value));

	strcpy(buffer, name);
	strcat(buffer, "=");


	while (e[i])
	{
		if (!flag && (strncmp(buffer, e[i], len) == 0))
		{
			flag = 1;
			strcat(buffer, value);
			new_env[i] = strcat(buffer, " ");
		}
		else 
			new_env[i] = e[i];
		
		if (++i > 510)
			return;
	}

	if (!flag)
	{
		strcat(buffer, value);
		new_env[i] = strcat(buffer, " ");
	}

	new_env[i+1] = 0;
	environ = new_env;
}


adj_utmp(indx)
	int indx;
{
	char name[9];

	sprintf(name, "sxt/%03d", conv(chan + indx));

	strcpy(u_entry->ut_line, name);
	pututline(u_entry);
}


help()
{
	pfmt(stdout, MM_NOSTD, ":25:block name [name ...]\n");
	pfmt(stdout, MM_NOSTD, ":26:create [name]\n");
	pfmt(stdout, MM_NOSTD, ":27:delete name [name ...]\n");
	pfmt(stdout, MM_NOSTD, ":28:help or ?\n");
	pfmt(stdout, MM_NOSTD, ":29:layers [-l] [name ...]\n");
	pfmt(stdout, MM_NOSTD, ":30:quit\n");
	pfmt(stdout, MM_NOSTD, ":31:toggle\n");
	pfmt(stdout, MM_NOSTD, ":32:resume [name]\n");
	pfmt(stdout, MM_NOSTD, ":33:unblock name [name ...]\n");
}
