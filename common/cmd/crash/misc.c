/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:common/cmd/crash/misc.c	1.2"
#ident "$Header: misc.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash functions:  ?, help, redirect,
 * and quit, as well as the command interpreter.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/var.h>
#include <sys/user.h>
#include <setjmp.h>
#include <locale.h>
#include "crash.h"

long pipesig;				/* pipe signal catch */
int opipe = 0;				/* open pipe flag */


/* returns argcnt, and args contains all arguments */
int
getcmd()
{
	char *p;
	int i;
	static char line[LINESIZE+1];
	FILE *ofp;
	
	ofp = fp;
	printf("> ");
	fflush(stdout);
	if(fgets(line,LINESIZE,stdin) == NULL)
		exit(0);
	line[LINESIZE] = '\n';
	p = line;
	while(*p == ' ' || *p == '\t') {
		p++;
	}
	if(*p == '!') {
		system(p+1);
		argcnt = 0;
	}
	else {
		for(i = 0; i < NARGS; i++) {
			if(*p == '\n') {
				*p = '\0';
				break;
			}
			while(*p == ' ' || *p == '\t')
				p++;
			args[i] = p;
			if(strlen(args[i]) == 1)
				break;
			if(*p == '!') {
				p = args[i];
				if(strlen(++args[i]) == 1)
					error("no shell command after '!'\n");
				pipesig = (long)signal(SIGPIPE,SIG_IGN);
				if((fp = popen(++p,"w")) == NULL) {
					fp = ofp;
					error("cannot open pipe\n");
				}
				if(rp != NULL)
					error("cannot use pipe with redirected output\n");
				opipe = 1;
				break;
			}
			if(*p == '(')
				while((*p != ')') && (*p != '\n'))
					p++;
			while(*p != ' ' && *p != '\n')
				p++;
			if(*p == ' ' || *p == '\t')
				*p++ = '\0';
		}
		args[i] = NULL;
		argcnt = i;
	}
}


/* get arguments for ? function */
int
getfuncs()
{
	int c;

	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	prfuncs();
}

/* print all function names in columns */
int
prfuncs()
{
	int i,j,len;
	struct func *ff;
	char tempbuf[20];

	len = (tabsize + 3) / 4;
	for(i = 0; i < len; i++) {
		ff = functab + i;
		for(j = 0; j < 4; j++) {
			if(*ff->description != '(')
				fprintf(fp,"%-15s",ff->name);
			else {
				tempbuf[0] = 0;
				strcat(tempbuf,ff->name);
				strcat(tempbuf," ");
				strcat(tempbuf,ff->description);
				fprintf(fp,"%-15s",tempbuf);
			}
			ff += len;
			if((ff - functab) >= tabsize)
				break;
		}
		fprintf(fp,"\n");
	}
	fprintf(fp,"\n");
}

/* get arguments for help function */
int
gethelp()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) {
		do {
			prhelp(args[optind++]);
		}while(args[optind]);
	}
	else prhelp("help");
}

/* print function information */
int
prhelp(string)
char *string;
{
	int found = 0;
	struct func *ff,*a,*aa;

	for(ff=functab;ff->name;ff++) {
		if(!strcmp(ff->name,string)){
			found = 1;
			break;
		}
	}
	if(!found)
		error("%s does not match in function list\n",string);
	if(!strcmp(ff->description,"NA"))  /* remove in next release */
		pralias(ff);
	if(*ff->description == '(') {
		for(a = functab;a->name != NULL;a++)
			if((a->call == ff->call) && (*a->description != '('))
					break;
		fprintf(fp,"%s %s\n",ff->name,a->syntax);
		if(findstring(a->syntax,"tbl_entry"))
			fprintf(fp,"\ttbl_entry = slot number | address | symbol | expression | range\n");
		if(findstring(a->syntax,"st_addr"))
			fprintf(fp,"\tst_addr = address | symbol | expression\n");
		fprintf(fp,"%s\n",a->description);
	}
	else {
		fprintf(fp,"%s %s\n",ff->name,ff->syntax);
		if(findstring(ff->syntax,"tbl_entry"))
			fprintf(fp,"\ttbl_entry = slot number | address | symbol | expression | range\n");
		if(findstring(ff->syntax,"st_addr"))
			fprintf(fp,"\tst_addr = address | symbol | expression\n");
		fprintf(fp,"%s\n",ff->description);
	}
	fprintf(fp,"alias: ");
	for(aa = functab;aa->name != NULL;aa++)
		if((aa->call == ff->call) && (strcmp(aa->name,ff->name)) &&
			strcmp(aa->description,"NA"))
				fprintf(fp,"%s ",aa->name);
	fprintf(fp,"\n");
	fprintf(fp,"\tacceptable aliases are uniquely identifiable initial substrings\n");
}

/* find tbl_entry or st_addr in syntax string */
int
findstring(syntax,substring)
char *syntax;
char *substring;
{
	char string[81];
	char *token;

	strcpy(string,syntax);
	token = strtok(string,"[] ");
	while(token) {
		if(!strcmp(token,substring))
			return(1);
		token = strtok(NULL,"[] ");
	}
	return(0);
}

/* this function and all obsolete aliases should be removed in next release */
/* print valid function names for obsolete aliases */
int
pralias(ff)
struct func *ff;
{
	struct func *a;

	fprintf(fp,"Valid calls to this function are:  ");
	for(a = functab;a->name;a++)
		if((a->call == ff->call) && (strcmp(a->name,ff->name)) &&
			(strcmp(a->description,"NA")))
				fprintf(fp,"%s ",a->name);
	error("\nThe alias %s is not supported on this processor\n",
		ff->name);
}


/* terminate crash session */
int
getquit()
{
	if(rp)
		fclose(rp);
	exit(0);
}

/* get arguments for redirect function */
int
getredirect()
{
	int c;
	int close = 0;

	optind = 1;
	while((c = getopt(argcnt,args,"w:c")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	close = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind]) 
		prredirect(args[optind],close);
	else prredirect(NULL,close);
}

/* print results of redirect function */
int
prredirect(string,close)
char *string;
int close;
{
	if(close)
		if(rp) {
			fclose(rp);
			rp = NULL;
			strcpy(outfile,"stdout");
			fp = stdout;
		}
	if(string) {
		if(rp) {
			fclose(rp);
			rp = NULL;
		}
		if(!(rp = fopen(string,"a")))
			error("unable to open %s\n",string);
		fp = rp;
		strncpy(outfile,string,ARGLEN);
	}
	fprintf(fp,"outfile = %s\n",outfile);
	if(rp)
		fprintf(stdout,"outfile = %s\n",outfile);
}
