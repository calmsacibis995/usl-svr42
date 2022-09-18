/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libiaf:common/lib/libiaf/iaf/setid.c	1.8.2.4"
#ident  "$Header: setid.c 1.3 91/07/10 $"

/* LINTLIBRARY */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<grp.h>
#include	<sys/vnode.h>
#include	<audit.h>
#include	<mac.h>
#include	<priv.h>
#include	<ia.h>
#include	<iaf.h>
#include	<unistd.h>
#include	<time.h>
#include	<sys/resource.h>
#include	<sys/secsys.h>

extern	int	fchmod(),
		lvlout(),
		auditctl(),
		auditevt(),
		fdevstat(),
		getrlimit(),
		setrlimit();

extern	size_t	strlen();

extern	pid_t	fork(),
		wait();

extern	void	exit();

extern	unsigned long	strtoul();

static	void	_ck_uinfo(),
		setup_ulimits(),
		dump_arec();

static	uinfo_t	uinfo = NULL;

/*
 * Procedure:	set_id
 *
 * Restrictions:
 *		auditctl(2):	none
 *		auditevt(2):	none
 *		setrlimit(2):	none
 *		lvlproc:	none
 *		fchown(2):	none
 *		fchmod(2):	none
 *		setgid(2):	none
 *		setgroups(2):	none
 *		setuid(2):	none
 *
 * Notes:	called by ``tmchild'' to set the necessary attributes
 *		for this user based on information retrieved via the
 *		``ava'' stream or the I&A database file.
*/
int
set_id(namep)
	char *namep;
{
	int	i; 
	int	audit = 0, mac = 0;
	char    **avap;
	char    *p, *tp, *u_name;
	char	*ttygrp = "tty";
	static	struct	group	*grpstr;
	uid_t	uid;
	gid_t	gid;
	gid_t	gidcnt;
	gid_t	*groups;
	level_t level = 0;
	level_t *ia_lvlp;
	long    lvlcnt;
	long	def_ulimit = 0;
	aevt_t	aevt;
	actl_t	actl;
	struct	devstat	devstat;
	struct	rlimit	rlimit;
	
	if (namep) {
		if ((ia_openinfo(namep, &uinfo)) || (uinfo == NULL)) {
			return 1;
		}
		ia_get_uid(uinfo, &uid);
		ia_get_gid(uinfo, &gid);
		ia_get_sgid(uinfo, &groups, &gidcnt);
		if (lvlproc(MAC_GET, &level) == 0) {
			mac++;
			if (ia_get_lvl(uinfo, &ia_lvlp, &lvlcnt))
				return 1;
			level = *ia_lvlp;
		}

		if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == 0) {
			if (actl.auditon == AUDITON)
				audit++;
			ia_get_mask(uinfo, aevt.emask);
		}
		u_name = namep;
	}
	else {
		if (( avap = retava(0)) == NULL) 
			return 1;

		if ((u_name = getava("LOGNAME", avap)) == NULL)
			return 1;

		if ((p = getava("UID", avap)) != NULL)
			uid = atol(p);
		else
			return 1;

		if ((p = getava("GID", avap)) != NULL) 
			gid = atol(p);
		else
			return 1;

		if ((p = getava("GIDCNT", avap)) != NULL) {
			gidcnt = atol(p);

			if (gidcnt) {
				if ((p = getava("SGID", avap)) == NULL) 
					return 1;
				groups = (gid_t *) calloc(gidcnt, sizeof(gid_t));
				for  (i=0; i < gidcnt; i++) {
					groups[i] = strtol(p, &tp, 10);
					p = ++tp;
				}
			}
		} else
			return 1;

		if ((p = getava("ULIMIT", avap)) != NULL) 
			def_ulimit = atol(p);
		else
			return 1;

		if ((p = getava("AUDITMASK", avap)) != NULL) {
			audit++;
			for  (i=0; i < ADT_EMASKSIZE; i++) {
				aevt.emask[i] = strtoul(p, &tp, 10);
				p = ++tp;
			}
		} else if (auditctl(ASTATUS, &actl, sizeof(actl_t)) == 0 &&
			   actl.auditon == AUDITON) {
			return 1;
		}

		if (( p = getava("LID", avap)) != NULL) {
			mac++;
			level = atol(p);
		} else if (lvlproc(MAC_GET, &level) == 0) {
			return 1;
		}

	}

	/* Set mode to r/w user & w group, owner to user and group to tty */
	/* use fildes 0                                                   */

	(void) fchmod(0,S_IRUSR|S_IWUSR|S_IWGRP);

	if ((grpstr = getgrnam(ttygrp)) == NULL) 
		(void) fchown(0, uid, gid);
	else
		(void) fchown(0, uid, grpstr->gr_gid);

	if (def_ulimit > 0L) {
		(void) getrlimit(RLIMIT_FSIZE, &rlimit);
		rlimit.rlim_cur = rlimit.rlim_max = (def_ulimit << SCTRSHFT);
		(void) setrlimit(RLIMIT_FSIZE, &rlimit);
	}
	/* Set user's audit mask	*/
	if (audit) {
		aevt.uid = uid;
		if (auditevt(ASETME, &aevt, sizeof(aevt_t))) {
			_ck_uinfo();
			return 1;
		}
		if (auditevt(AYAUDIT, NULL, sizeof(aevt_t))) {
			_ck_uinfo();
			return 1;
		}
		dump_arec(uid,gid,level,mac,u_name,avap);
	}

	if (setgid(gid) == -1) {
		_ck_uinfo();
		return 1;
	}

	/* Initialize the supplementary group access list. */
	if (setgroups(gidcnt, groups)) {
		_ck_uinfo();
		return 1;
	}

	/*
	 * set the MAC level
	 */

	if (mac) {
		level_t cur_level;

		if (fdevstat(0, DEV_GET, &devstat) != 0) {
			_ck_uinfo();
			return 1;
		}

		/* Make sure the device has been allocated	*/

		if (devstat.dev_relflag == DEV_SYSTEM) {
			_ck_uinfo();
			return 1;
		}

		if (flvlfile(0, MAC_GET, &cur_level)) {
			_ck_uinfo();
			return 1;
		}

		/* if we need to change the level of the device,
		   it must be in the DEV_PRIVATE state.	*/

		if (cur_level != level) {
			if (devstat.dev_state != DEV_PRIVATE) {
				devstat.dev_state = DEV_PRIVATE;
				if (fdevstat(0, DEV_SET, &devstat)) {
					_ck_uinfo();
					return 1;
				}
			}
			if (flvlfile(0, MAC_SET, &level)) {
				_ck_uinfo();
				return 1;
			}
		}

		/* Move the device to the DEV_PUBLIC state so
		   the user can access it without privilege.	*/

		if (devstat.dev_state != DEV_PUBLIC) {
			devstat.dev_state = DEV_PUBLIC;
			if (fdevstat(0, DEV_SET, &devstat)) {
				_ck_uinfo();
				return 1;
			}
		}

		if (lvlproc(MAC_SET, &level) != 0) {
			_ck_uinfo();
			return 1;
		}
	}

	if (setuid(uid) == -1) {
		_ck_uinfo();
		return 1;
	}

	setup_ulimits(mac, level, u_name);

	_ck_uinfo();

	return 0;
}


#define	auser	"/usr/bin/adminuser %s > /dev/null 2>&1"

/*
 * Procedure:	setup_ulimits
 *
 * Restrictions:
 *		lvlout:		none
 *		fork(2):	none
 *		exec(2):	P_ALLPRIVS
 *		setrlimit(2):	none
 *
 * Notes:	determines if the user is an administrator and if
 *		so, sets the `hard'' limits for all resource limits
 *		to ``unlimited''.
*/
static	void
setup_ulimits(mac, lvl, namp)
	int	mac;
	level_t	lvl;
	char	*namp;
{
	int	i,
		res,
		adm = 0,
		status = 0;
	pid_t	pid;
	char	*cmd,
		*bufp,
		*sh_cmd = "/sbin/sh",
		*sys_class = "system:";
	struct	rlimit	rlimit;

	bufp = '\0';

	if (mac) {
		bufp = (char *)malloc((unsigned int)(i = lvlout(&lvl, bufp,
			0, LVL_FULL)));
		if (bufp != NULL) {
			(void) lvlout(&lvl, bufp, i, LVL_FULL);
		}
		if (!strncmp(bufp, sys_class, strlen(sys_class))) {
			adm = 1;
		}
	}
	/*
	 * the MAC feature isn't installed.  If the privilege mechanism
	 * is file-based fork() and exec() the ``adminuser'' command to
	 * determine if the user is an administrator.
	*/
	else if (secsys(ES_PRVID, 0) < 0) {
		cmd = (char *)malloc(strlen(auser) + strlen(namp) + (size_t)1);
		(void) sprintf(cmd, auser, namp);

		if ((pid = fork()) < 0) {
			/*
			 * the fork failed so just return.
			*/
			return;
		}
		if (pid == (pid_t) 0) {
			/*
			 * in the child.
			*/
			(void) execl(sh_cmd, sh_cmd, "-c", cmd, (char *)NULL);
			exit(1);
		}
		else {
			/*
			 * in the parent.  Sit quietly and
			 * wait for the child to terminate.
			*/
			(void) wait(&status);
		}
		/*
		 * child has terminated.  Now the parent checks
		 * the exit value.
		*/
		if (((status >> 8) & 0377) != 0) {
			/*
			 * exit value from child was not zero (0)
			 * so just return since the user isn't
			 * considered an administrator.
			*/
			return;
		}
		else {
			adm = 1;
		}
	}
	if (adm) {
		for (res = 0; res < RLIM_NLIMITS; res++) {
			(void) getrlimit(res, &rlimit);
			rlimit.rlim_max = RLIM_INFINITY;
			(void) setrlimit(res, &rlimit);
		}
	}
	return;
}


/*
 * Procedure:	_ck_uinfo
 *
 * Notes:	checks if storage for data from the master file was allocated.
 *		If so, it frees the storage space.
*/
static	void
_ck_uinfo()
{
	if (uinfo != NULL) {
		ia_closeinfo(uinfo);
	}
	return;
}

/*
 * Procedure:	dump_arec
 *
 * Restrictions:
 *		auditdmp(2):	none
 *
 * Notes:	Constructs the audit record for the event login 
 */	
static	void
dump_arec(uid,gid,level,mac,namep,avap)
uid_t uid;
gid_t gid;
level_t level;
int mac;
char *namep;
char **avap;
{
        arec_t		rec;		/* auditdmp(2) structure */
        alogrec_t	alogrec;	/* login record structure */
	char    	*tty = NULL;
	level_t 	*ia_p;
	long 		count;

	rec.rtype = ADT_LOGIN;
        rec.rsize = sizeof(struct alogrec);

	/* At this point the user has passed I&A validations. */
	/* Therefore only a successful audit record is dumped */
        rec.rstatus = 0;

	alogrec.uid = uid;
	alogrec.gid = gid;
	alogrec.hlid = level; /* Current level of user */
	alogrec.ulid = 0;     /* Default level of user */
	alogrec.vlid = 0;     /* Not used for event login */
	alogrec.tty[0]='\0';
	alogrec.bamsg[0]='\0'; /* Not used for event login */

	if (mac) {
		if (uinfo != NULL)
			/* The current level and the default level are the same */
			alogrec.ulid = level;
		else {
			/* Retrieve the default level */
			if ((ia_openinfo(namep, &uinfo)) == 0) {
				if ((ia_get_lvl(uinfo, &ia_p, &count)) == 0 )
					alogrec.ulid = *ia_p;
			}
		}
	}

	if (avap) {
		if ((tty = getava("TTY", avap)) != NULL) {
			(void)strncpy(alogrec.tty,tty,ADT_TTYSZ);
			alogrec.tty[ADT_TTYSZ-1]='\0';
		}
	}
		

        rec.argp = (char *)&alogrec;

        (void) auditdmp(&rec, sizeof(arec_t));
        return;
}
