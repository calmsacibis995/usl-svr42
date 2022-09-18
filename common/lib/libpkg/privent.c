/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libpkg:common/lib/libpkg/privent.c	1.1.6.5"
#ident "$Header: $"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "priv.h"
#include <pkgstrct.h>
#include <pkglocs.h>
#include <sys/secsys.h>
#include <errno.h>

#define	CMD		"/sbin/filepriv"
#define	ABUFSIZ		(((NPRIVS + 1) * PRVNAMSIZ) + (NPRIVS + 1))
extern	char		*privname();
extern	int		srchcfile();

static 	struct 	pm_setdef *sdefs=(struct pm_setdef *)0;
static 	int 	nsets=0;
static 	ulong 	objtyp = PS_FILE_OTYPE;
static 	priv_t	getprivp[NPRIVS];
static	void	printprivs();

int
getprivs(path, fixstrp, inhstrp, getfix, getinh)
char *path;
char *fixstrp, *inhstrp;
int  *getfix, *getinh;
{
	int i, cnt;

	/* this piece of code determines what state you are in
	 * by finding out what privileges(fixed or inheritable
	 * or neither) can be obtained from the filepriv command
	 */
	nsets = secsys(ES_PRVSETCNT, 0);
	if(nsets < 0){
		nsets = 0;
	}
	sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t));
	if(!sdefs){
		printf("memory allocation failure, errno = %d\n", errno);
		return(-1);
	}
	secsys(ES_PRVSETS, (char *)sdefs);

	/* initialize the getprivp array */
	if((cnt = filepriv(path, GETPRV, getprivp, NPRIVS)) >= 0) {
		/* get the privileges as a string and place into
		 * the fixed privlege string and inherited privilege
		 * string to be compared later with the 
		 * privilges to check or set
		 */
		printprivs(cnt, fixstrp, inhstrp, getfix, getinh);
	}

	return(cnt);
}

int
setprivs(path, privfix_str, privinh_str)
char *path;
char *privfix_str, *privinh_str;
{
	char *strg[7];
	int status, ret=1, i=0;
	
	strg[i++] = CMD;
/****
 ****	This section is being commented out for 4.1dt since it is not
 ****	intended to run LPM and it is a waste of time to set inheritable
 ****	privileges.  The code directly following (marked 4.1dt) replaces
 ****	this segment to only set fixed privileges. 
 ****
 ****	if(!strcmp(privfix_str, "NULL") && !strcmp(privinh_str, "NULL")) {
 ****		return(0);
 ****	} else {
 ****		if(!strcmp(privfix_str, "NULL")) {
 ****			strg[i++] = "-i";
 ****			strg[i++] = privinh_str;
 ****		} else {
 ****			strg[i++] = "-f";
 ****			strg[i++] = privfix_str;
 ****			if(strcmp(privinh_str, "NULL")) {
 ****				strg[i++] = "-i";
 ****				strg[i++] = privinh_str;
 ****			}
 ****		}
 ****	}
 ****/

	if(strcmp(privfix_str, "NULL")) {	/* 4.1dt */
		strg[i++] = "-f";		/* 4.1dt */
		strg[i++] = privfix_str;	/* 4.1dt */
	} else					/* 4.1dt */
		return(0);			/* 4.1dt */

	strg[i++] = path;	
	strg[i] = NULL;

	if(fork() == 0) {
		(void) close(2);
		(void) execv(CMD, strg);
		exit(1);
	}
	(void) wait(&status);
	if((status >> 8) != 0)
		ret = -1;

	return(ret);
}

/*
 * This routine is called by getprivs to change 
 * the privileges into string format
*/
static	void
printprivs(cnt, fixstrp, inhstrp, getfix, getinh)
int	cnt;
char	*fixstrp, *inhstrp;
int	*getfix, *getinh;
{
	int	i, j, k, printed = 0;
	char		tbuf[ABUFSIZ],
			pbuf[PRVNAMSIZ],
			*tbp = &tbuf[0],
			*pbp = &pbuf[0];
	setdef_t	*sd;

	/* initialize  variables */
	sd = sdefs;
	tbuf[0] = '\0';

	for (i = 0; i < nsets; ++i, ++sd) {
		if (sd->sd_objtype == objtyp) {
			for (k = 0; k < cnt; ++k) {
				for (j = 0; j < sd->sd_setcnt; ++j) {
					if ((sd->sd_mask | (j)) == getprivp[k]) {
						if (printed) {
							printed = 0;
							(void) strcat(tbp, ",");
						}
						(void) strcat(tbp, privname(pbp, j));
						printed = 1;
					}
				}
			}
			if(!strcmp(sd->sd_name, "fixed")) {
				(*getfix)++;
				(void) strcpy(fixstrp, tbp);
			}
			if(!strcmp(sd->sd_name, "inher")) {
				(*getinh)++;
				(void) strcpy(inhstrp, tbp);
			}
			
			printed = 0;
			tbuf[0] = '\0';
		}
	}
}

int 
rmprivs(path)
char *path;
{
	char *strg[4];
	int status, ret=0, i=0;
	
	strg[i++] = CMD;
	strg[i++] = "-d";
	strg[i++] = path;	
	strg[i] = NULL;

	if(fork() == 0) {
		(void) close(2);
		(void) execv(CMD, strg);
		exit(1);
	}
	(void) wait(&status);
	if((status >> 8) != 0)
		ret = -1;

	return(ret);
}

int
reset(path)
char	*path;
{
	struct cfent	local_entry;
	struct stat	buf;
	char		contents[PATH_MAX];
	FILE		*cfp;

	/*
	 * This function is only called during installation and removal of hard linked
	 * files to check if the file being linked to (<path>) has privileges, and if 
	 * so, reset them.  This is required since the ctime of <path> changes when the
	 * link count changes, resulting in all privileges being unset.  Since privileges
	 * are applicable only for executable files, we need only to perform this for
	 * executable files.  For each file <path> that is executable, look through the
	 * contents file to determine if privileges are associated with it.  If so, we'll
	 * reset them.
	 */
	if(stat(path, &buf) < 0)
		return(1);
	if(buf.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) {
		(void) sprintf(contents, "%s/contents", PKGADM);
		if((cfp = fopen(contents, "r")) != NULL) {
			local_entry.pinfo = NULL;
			srchcfile(&local_entry, path, cfp, (FILE *)NULL);
			fclose(cfp);
		}
		if(strcmp(local_entry.ainfo.priv_fix, "NONE")) {
			if(setprivs(path, local_entry.ainfo.priv_fix, local_entry.ainfo.priv_inh) < 0) {
				return(1);
			}
		}
	}
	return(0);
}
