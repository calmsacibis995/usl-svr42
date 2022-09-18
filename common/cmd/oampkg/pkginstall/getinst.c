/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/getinst.c	1.8.8.9"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <valtools.h>
#include <pkginfo.h>
#include <limits.h>
#include "install.h"

extern struct admin 
		adm;
extern char	*pkgarch, 
		*pkgvers,
		*msgtext,
		*pkgabrv;

extern int	opresvr4,
		update,
		logmode,
		maxinst,
		nointeract;

extern CKMENU	*allocmenu();
extern void	free(),
		nqtext(),
		printmenu(),
		progerr(),
		quit();
extern int	setitem(),
		pkginfo(),
		ckitem(),
		ckyorn();


#define MSG_UNIQ1	\
"\\nInstances of the <%s> package are already installed on the system. \
Since current administration requires creation of a new instance \
and since the number of instances installed has reached a maximum, \
a new instance cannot be created."

#define MSG_NOINTERACT	\
"\\nUnable to determine whether to overwrite an existing \
package instance, or add a new instance."

#define MSG_NEWONLY	\
"\\nA version of the <%s> package is already installed \
on this machine.  Current administration does not allow new instances \
of an existing package to be created, nor existing instances to be overwritten."

#define MSG_SAME	\
"\\nThis appears to be an attempt to install the \
same architecture and version of a package which \
is already installed.  This installation will \
attempt to overwrite this package.\\n"

#define MSG_OVERWRITE	\
"\\nCurrent administration does not allow \
new instances of an existing package to \
be created.  However, the installation \
service was unable to determine which  \
package instance to overwrite."

static char	newinst[15];
static char	*nextinst(), *prompt();

char *
getinst(info, npkgs, package)
struct pkginfo *info;
int	npkgs;
char *package;
{
	int	i, samearch, nsamearch;
	char	*inst, *sameinst;
	char	*getenv();
	char	lname[PATH_MAX];
	FILE	*switchfp;
	FILE	*fp;

	switchfp = stderr;
	if (logmode) {
		if(package) {
			(void) sprintf(lname, "/var/sadm/install/logs/%s.log", package);
			if ((fp = fopen(lname, "a")) != NULL)
				switchfp = fp; 
		}
	}

	if(ADM(instance, "newonly") || ADM(instance, "quit")) {
		msgtext = MSG_NEWONLY;
		nqtext(switchfp, msgtext, pkgabrv);
		quit(4);
	} 

	samearch = nsamearch = 0;
	sameinst  = NULL;
	for(i=0; i < npkgs; i++) {
		if(info[i].arch && !strcmp(info[i].arch, pkgarch)) {
			samearch = i;
			nsamearch++;
		   	if(info[i].version && !strcmp(info[i].version, pkgvers) && !ADM(instance, "unique"))
				sameinst = info[i].pkginst;
		}
	}

	if(sameinst && nsamearch < 2) {
		if(!ADM(instance, "overwrite"))
			nqtext(switchfp, MSG_SAME);
		inst = sameinst; /* can't be overwriting a pre-svr4 package */
		update++;
	/*
	 * Don't do unique handling for SIP packages since not all of their
	 * member packages have to be installed at once and this may be an
	 * attempt at installing a set member package that was not installed
	 * previously.  Treat all packages whose category is 'set' just as
	 * if the instance token in the admin file is "overwrite".
	 */
	} else if(ADM(instance, "overwrite") || !strcmp(info[samearch].catg, "set")) {
		if(npkgs == 1) {
			samearch = 0; /* use only package we know about */
			inst = info[samearch].pkginst;
			if(info[samearch].status == PI_PRESVR4)
				opresvr4++; /* overwriting a pre-svr4 package */
		}
		else 
			inst = prompt(info, npkgs);
		update++;
	} else if(ADM(instance, "unique")) {
		if(maxinst <= npkgs) {
			/* too many instances */
			msgtext = MSG_UNIQ1;
			nqtext(switchfp, msgtext, pkgabrv);
			quit(4);
		}
		inst = nextinst();
	} else if(nointeract) {
		msgtext = MSG_NOINTERACT;
		nqtext(switchfp, msgtext);
		quit(5);
	} else {
		inst = prompt(info, npkgs);
		if(!strcmp(inst, "new"))
			inst = nextinst();
		else {
			update++;
			/* see if this instance is presvr4 */
			for(i=0; i < npkgs; i++) {
				if(!strcmp(inst, info[i].pkginst)) {
					if(info[i].status == PI_PRESVR4)
						opresvr4++;
					break;
				}
			}
		}
	}
	if(switchfp != stderr)
		(void) fclose(switchfp);

	return(inst);
}

static char *
nextinst()
{
	struct pkginfo info;
	int	n;

	n = 2; /* requirements say start at 2 */

	info.pkginst = NULL;
	(void) strcpy(newinst, pkgabrv);
	while(pkginfo(&info, newinst, NULL, NULL) == 0)
		(void) sprintf(newinst, "%s.%d", pkgabrv, n++);
	return(newinst);
}

#define PROMPT0 \
"Do you want to overwrite this installed instance"

#define PROMPT1	\
"Do you want to create a new instance of this package"

#define HELP1 \
"The package you are attempting to install already \
exists on this machine.  You may choose to create a \
new instance of this package by answering 'y' to this \
prompt.  If you answer 'n' you will be asked to choose \
one of the instances which is already installed to be overwritten."

#define HEADER	\
"The following instance(s) of the <%s> \
package are already installed on this machine:"

#define PROMPT2	\
"Enter the identifier for the instance that you want to overwrite"

#define HELP2 \
"The package you are attempting to install already \
exists on this machine.  You may choose to overwrite \
one of the versions which is already installed by \
selecting the appropriate entry from the menu."

static char *
prompt(info, npkgs)
struct pkginfo *info;
int	npkgs;
{
	CKMENU	*menup;
	int	i, n;
	char	ans[INPBUF], *inst;
	char	header[256];
	char	temp[1024];
	

	(void) sprintf(header, HEADER, pkgabrv);
	menup = allocmenu(header, CKALPHA);

	for(i=0; i < npkgs; i++) {
		if(!info[i].arch)
			(void) sprintf(temp, "%s %s\n %s", info[i].pkginst,
				info[i].name, info[i].version);
		else
			(void) sprintf(temp, "%s %s\n(%s) %s", info[i].pkginst,
				info[i].name, info[i].arch, info[i].version);
		if(setitem(menup, temp)) {
			progerr("no memory");
			quit(99);
		}
	}

	if(npkgs == 1) {
		printmenu(menup);
		if(n = ckyorn(ans, NULL, NULL, NULL, PROMPT0))
			quit(n);
		if(ans[0] != 'y')
			quit(3);
		(void) strcpy(newinst, info[0].pkginst);
	} else {
		if(n = ckitem(menup, &inst, 1, NULL, NULL, HELP2, PROMPT2))
			quit(n);
		(void) strcpy(newinst, inst);
	}
	(void) setitem(menup, 0); /* clear resource usage */
	free(menup); /* clear resource usage */

	return(newinst);
}
