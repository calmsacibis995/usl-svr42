/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*                       nls_expand.c
 *
 *  nls_expand [-t] path
 *
 *  Expand the path "path" like the fmli-internal command "readfile".
 *  if "-t" is specified, do not print out the expanded path, only 
 *  set exit code right.
 *  Exit codes: 0: file exists, !=0: file not found
 *		255: syntax error
*/

#ident	"@(#)face:src/filecab/fileb/nls_expand.c	1.1"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int try(char *base,char *nls,char *name,int output)
{
	char path[BUFSIZ];
	int  retval;

	strcpy(path,base);
	strcat(path,"/");
	if (nls) if (*nls)
	{
		strcat(path,nls);
		strcat(path,"/");
	}
	strcat(path,name);
	retval=access(path,F_OK) ? 1 : 0;
	if (retval == 0 && output)
		printf("%s\n",path);
	return(retval);
}

void main(int argc,char **argv)
{
	char base[BUFSIZ],name[BUFSIZ];
	char nlsvar[100];
	char *p;
	int  flag,oflag,ac;

	if (argc < 2) exit (255);

	oflag=strcmp(argv[1],"-t");
	ac = oflag ? 1 : 2;

	if (ac >= argc) exit (255);

	strcpy(base,argv[ac]);
	p=strrchr(base,'/');
	if (p)
	{
		strcpy(name,p+1);
		*p=0;
	}
	else
	{
		strcpy(name,base);
		strcpy(base,".");
	}

	flag=1;
	p=getenv("LC_MESSAGES");
	if (p) 
		if (strcmp(p,"C") && *p) flag=try(base,p,name,oflag);
	if (flag)
	{
	  p=getenv("LANG");
	  if (p)
	  	if (strcmp(p,"C") && *p) flag=try(base,p,name,oflag);
	}
	if (flag) flag=try(base,NULL,name,oflag);
	exit(flag);
}

