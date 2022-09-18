/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgadd/presvr4.c	1.9.10.3"
#ident  "$Header: $"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <limits.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <pkginfo.h>
#include <pkgstrct.h>
#include <pkgdev.h>
#include <pkglocs.h>
#include "install.h"

extern struct admin
		adm;
extern struct pkgdev
		pkgdev;
extern char	*respfile,
		*tmpdir;
extern int	warnflag, nointeract;
extern char	*tempnam();
extern void	psvr4cnflct(), psvr4pkg(), psvr4mail(),
		echo(),
		logerr(),
		progerr(),
		quit();
extern int	copyf(),
		chmod(),
		pkgexecl(),
		chdir(),
		averify(),
		setlvl(),
		srchcfile(),
		pkgumount(),
		unlink();	
extern void	(*func)();

static int	ck_coredirs();

int	intfchg = 0;

#define MSG_SUCCEED \
	"\n## Pre-SVR4 package reports successful installation."
#define MSG_FAIL \
	"\n## Pre-SVR4 package reports failed installation."
#define MSG_MAIL \
	"An attempt to install the <%s> pre-SVR4 package \
	on <%s> completed with exit status <%d>."

#define ERR_NOCOPY	"unable to create copy of UNINSTALL script in <%s>"
#define ERR_NOINT \
"-n option cannot be used when removing pre-SVR4 packages" 
#define ERR_RESTORE \
	"attempt to restore permissions of <%s> files may have failed"
#define ERR_RESTFILE	"unable to restore permissions for <%s>"
#define ERR_RESPFILE	"response file is invalid for pre-SVR4 package"
#define ERR_PRESVR4 \
	"The current volume is in old (pre-SVR4) format; \
	the PKGASK command is not able to understand this \
	format"

#define user_pub 4
#define sys_pub 1

presvr4(ppkg) 
char	**ppkg;
{
	int	retcode, n, found = 0;
	char	*tmpcmd, *pt, path[PATH_MAX];
	void	(*tmpfunc)();
	char	*Rlist[] = {
		"Rlist",
		"RLIST",
		"rlist",
		NULL
	};
	level_t	lvl;
	FILE	*fp;
	char	rlist[PATH_MAX], pathname[PATH_MAX];

	echo("*** Installing Pre-SVR4 Package ***");
	if(nointeract) {
		progerr(ERR_NOINT);
		quit(1);
	}

	if(respfile) {
		progerr(ERR_RESPFILE);
		quit(1);
	}

	/*
	 * if we were looking for a particular package, verify
	 * the first media has a /usr/options file on it
	 * which matches
	 */
	psvr4pkg(ppkg);

	/*
	 * check to see if we can guess (via Rlist) what 
	 * pathnames this package is likely to install;
	 * if we can, check these against the 'contents'
	 * file and warn the administrator that these
	 * pathnames might be modified in some manner
	 */
	psvr4cnflct(0);

	if(chdir(tmpdir)) {
		progerr("unable to change directory to <%s>", tmpdir);
		quit(99);
	}

	(void) sprintf(path, "%s/install/INSTALL", pkgdev.dirname);
	tmpcmd = tempnam(tmpdir, "INSTALL");
	if(!tmpcmd || copyf(path, tmpcmd, 0L) || chmod(tmpcmd, 0500)) {
		progerr(ERR_NOCOPY, tmpdir);
		quit(99);
	}

	echo("## Executing INSTALL script provided by package");
	tmpfunc = signal(SIGINT, func);
	retcode = pkgexecl(NULL, NULL, NULL, SHELL, "-c", tmpcmd, pkgdev.bdevice,
		pkgdev.dirname, NULL);
	tmpfunc = signal(SIGINT, SIG_IGN);
	echo(retcode ? MSG_FAIL : MSG_SUCCEED);

	/* set the level for all files to USER_PUBLIC */
	lvl = user_pub;
	for(n=0; Rlist[n]; n++) {
		(void) sprintf(rlist, "%s/install/%s", pkgdev.dirname, Rlist[n]);
		if(access(rlist, 0) == 0) {
			found = 1;
			break;
		}
	}

	if(found) {
		if((fp = fopen(rlist, "r")) != NULL) {
			while(fgets(pathname, PATH_MAX, fp)) {
				if(pt = strpbrk(pathname, " \t\n"))
					*pt = '\0';
				setlvl(pathname, &lvl);
			}
		}
	}
	fclose(fp);
	
	/* set the level of the file installed 
	 * under /usr/options to SYS_PUBLIC
	 */
	lvl = sys_pub;
	(void) sprintf(pathname, "%s/%s.name", PKGOLD, *ppkg);
	printf("pathname = %s\n", pathname);
	setlvl(pathname, &lvl);

	(void) unlink(tmpcmd);
	(void) chdir("/");
	(void) pkgumount(&pkgdev);

	psvr4mail(adm.mail, MSG_MAIL, retcode, *ppkg ? *ppkg : "(unknown)");
	(void) signal(SIGINT, tmpfunc);

	/*
	 * attempt to restore permissions of all core files
	 */
	if(ck_coredirs()) {
		progerr(ERR_RESTORE, COREPKG);
		warnflag++;
	}

	intfchg++;
	return(retcode);
}

void
intf_reloc()
{
	char	path[PATH_MAX];

	(void) sprintf(path, "%s/intf_reloc", PKGBIN);
	(void) pkgexecl(NULL, NULL, NULL, SHELL, "-c", path, NULL);
}

static int
ck_coredirs()
{
	struct cfent entry;
	struct pinfo *pinfo;
	FILE	*fp;
	int	n, count, errflg, selected;
	char	contents[PATH_MAX];

	echo("## Restoring permissions of all <%s> directories.", COREPKG);

	errflg = 0;
	entry.pinfo = (NULL);

	(void) sprintf(contents, "%s/contents", PKGADM);
	if((fp = fopen(contents, "r")) == NULL) {
		progerr("unable to open contents file <%s>", contents);
		return(-1);
	}

	/* read entries in contents file, looking for directories
	 * which are referenced by the CORE package; if found, 
	 * reset the permissions to make sure this pre-svr4 package
	 * hasn't modified them
	 */
	count = 0;
	while(n = srchcfile(&entry, "*", fp, NULL)) {
		if(n < 0) {
			progerr("unable to process contents file");
			logerr("pathname: %s", entry.path);
			logerr("problem: %s", errstr);
			return(-1);
		}
		if(n == 0)
			break; /* done with file */

		if(!strchr("dx", entry.ftype))
			continue;
			
		/* check to see if the entry we just read
		 * is associated with the COREPKG
		 */
		selected = 0;
		pinfo = entry.pinfo;
		while(pinfo) {
			if(!strcmp(pinfo->pkg, COREPKG)) {
				selected++;
				break;
			}
			pinfo = pinfo->next;
		}
		if(!selected)
			continue; /* not selected */

		count++;
		if(averify(1, &entry.ftype, entry.path, &entry.ainfo, 1)) {
			progerr(ERR_RESTFILE, entry.path);
			errflg++;
		}
	}
	if(errflg == 0)
		echo("   %d directories successfully verified.", count);
	(void) fclose(fp);
	return(errflg);
}
