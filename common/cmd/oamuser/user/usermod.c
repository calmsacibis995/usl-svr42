/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/usermod.c	1.9.17.12"
#ident  "$Header: usermod.c 2.1 91/07/29 $"

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<limits.h>
#include	<grp.h>
#include	<string.h>
#include	<userdefs.h>
#include	"users.h"
#include	"messages.h"
#include 	<shadow.h>
#include	"uidage.h"
#include 	<pwd.h>
#include 	<signal.h>
#include 	<errno.h>
#include 	<time.h>
#include 	<unistd.h>
#include 	<stdlib.h>
#include	<deflt.h>
#include	<mac.h>
#include	<ia.h>
#include	<sys/vnode.h>
#include	<audit.h>
#include	<priv.h>
#include	<fcntl.h>
#include	<locale.h>
#include	<pfmt.h>

/*
 * Usage:
 *
 * no MAC - no auditing installed (base system)
 *
 *	usermod [-u uid [-o] | -g group | -G group[[,group]...] | -d dir [-m] |
 *		-s shell | -c comment | -l new_logname | -f inactive |
 *		-e expire | -p passgen] login
 *
 * MAC - no auditing installed
 *
 *	usermod [-u uid [-o] | -g group | -G group[[,group]...] | -d dir [-m] |
 *		-s shell | -c comment | -l new_logname | -f inactive |
 *		-e expire | -p passgen | -h [operator2]level [-h level [...]] |
 *		-v def_level] login
 *
 * no MAC - auditing installed
 *
 *	usermod [-u uid [-o] | -g group | -G group[[,group]...] | -d dir [-m] |
 *		-s shell | -c comment | -l new_logname | -f inactive |
 *		-e expire | -p passgen | -a [operator1]event[,...]] login
 *
 * MAC and auditing installed
 *
 *	usermod [-u uid [-o] | -g group | -G group[[,group]...] | -d dir [-m] |
 *		-s shell | -c comment | -l new_logname | -f inactive |
 *		-e expire | -p passgen | -h [operator2]level [-h level [...]] |
 *		-v def_level | -a [operator1]event[,...]] login
 *
 * Level:	SYS_PRIVATE
 *
 * Inheritable Privileges:	P_OWNER,P_AUDIT,P_COMPAT,P_DACREAD,P_DACWRITE
 *				P_FILESYS,P_MACREAD,P_MACWRITE,P_MULTIDIR,
 *				P_SETPLEVEL,P_SETUID,P_FSYSRANGE,P_SETFLEVEL
 * Fixed Privileges:		none
 *
 * Files:	/etc/passwd
 *		/etc/shadow
 *		/etc/.pwd.lock
 *		/etc/security/ia/master
 *
 * Notes: 	This command modifies user logins on the system.
 */

extern	void	errmsg(),
		adumprec();

extern	long	strtol();

extern	struct	tm	*getdate();

extern	struct	passwd	*getpwnam();

extern	int	lvlia(),
		lvlin(),
		isbusy(),
		getopt(),
		cremask(),
		uid_age(),
		rm_files(),
		auditctl(),
		valid_uid(),
		check_perm(),
		edit_group(),
		all_numeric(),
		create_home(),
		restore_ia(),
		valid_group(),
		valid_login(),
		call_udelete(),
		**valid_lgroup(),
		ck_and_set_flevel();

extern	char	*optarg,			/* used by getopt */
		*errmsgs[],
		argvtostr();

extern	int	errno,
		optind,
		opterr,				/* used by getopt */
		getdate_err;

static	void	fatal(),
		rm_all(),
		clean_up(),
		bad_news(), 
		validate(),
		file_error(),
		no_service(),
		replace_all();

static	char	*pgenp,
		*logname,			/* login name to add */
		*sgetsave(),
		*Dir = NULL;

static  struct  passwd	*sgetpwnam();

static	struct	master	*masp,
			*mastp,
			*new_mastp;

static	struct	index	index,
			*indxp = &index;

static	struct	adtuser	adtuser,
			*adtp = &adtuser;

static	level_t	*lidlstp,
		lidlist[64];

static	int	mac = 0,
		plus = 0,
		audit = 0,
		minus = 0,
		cflag = 0,
		hflag = 0,
		pflag = 0,
		vflag = 0,
		**Gidlist,
		Gidcnt = 0,
		replace = 0,
		new_home = 0,
		call_mast = 0,
		call_pass = 0,
		call_shad = 0,
		new_groups = 0,
		new_master = 0;

static	long	lidcnt = 0;

static	gid_t	*Sup_gidlist;

static	int	rec_pwd(),
		ck_p_sz(),
		ck_s_sz(),
		getgrps(),
		mod_audit(),
		ck_def_lid(),
		mod_passwd(),
		mod_master(),
		mod_shadow();

/*
 * The following variables can be supplied on the command line
 */
static	char	*Fpass = NULL,
		*Group = NULL,
		*Inact = NULL,
		*Shell = NULL,
		*Expire = NULL,
		*Uidstr = NULL,
		*Comment = NULL,
		*Def_lvl = NULL,
		*usr_lvl = NULL,
		*Auditmask = NULL,
		*new_logname = NULL;

/*
 * The following variable MUST be global so that it is
 * accessible to the external error printing routine "errmsg()".
 */
char	*msg_label = "UX:usermod";

static	const	char
	*miss	= ":0:Unexpected failure.  Password file(s) missing\n",
	*busy	= ":0:Password file(s) busy.  Try again later\n",
	*nochg	= ":0:Unexpected failure.  Password files unchanged\n",
	*badent	= ":0:Bad entry found in \"%s\".  Run pwconv.\n",
	*a_badent = ":0:Bad entry found in audit file.  Run auditcnv.\n";

/*
 * These strings are used as replacements in the ``usage'' line depending
 * on the features installed so they should never have their own catalog
 * index numbers.
 */
static	char
	*aud_str = " | -a [operator1]event[,...]",
	*mac_str = " | -h [operator2]level [-h level [...]] |\n               -v def_level";

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	i, ch, cll, uid = 0,
		mflag = 0, oflag = 0;
	gid_t	gid = 1;

	level_t	level;

	struct	spwd	shadow_st;
	struct	passwd	passwd_st,
			*pstruct;

	char	*cmdlp,
		lvlfl[64],
		*grps = NULL,		/* multi groups from command line */
		*Mac_check = "SYS_PRIVATE";

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore.abi");
	(void) setlabel(msg_label);

	opterr = 0;			/* no print errors from getopt */

	/*
	 * save command line arguments
	*/
	if ((cmdlp = (char *)argvtostr(argv)) == NULL) {
		adumprec(ADT_MOD_USR, 1, strlen(argv[0]), argv[0]);
                clean_up(1);
        }

	cll = (int) strlen(cmdlp);

	tzset();


	/*
	 * Determine if the MAC feature is installed.  If
	 * so, then  the ``lvlin()'' call  will return 0.
	*/
	if (lvlin(Mac_check, &level) == 0)
		mac = 1;
	/*
	 * Determine if the AUDIT feature is installed.
	 * If so, update the appropriate data files with
	 * audit information.
	*/
	if (access(AUDITMASK, EFF_ONLY_OK) == 0) {
		audit = 1;
	}

	errno = 0;

	while((ch = getopt(argc, argv, "c:d:e:f:G:g:l:mop:s:u:a:h:v:")) != EOF)
		switch(ch) {
		case 'c':
			if (strlen(optarg))
				Comment = optarg;
			cflag = 1;
			break;
		case 'd':
			Dir = optarg;
			break;

		case 'e':
			Expire = optarg;
			break;

		case 'f':
			Inact = optarg;
			break;

		case 'G':
			grps = optarg;
			break;

		case 'g':
			Group = optarg;
			break;

		case 'l':
			new_logname = optarg;
			break;

		case 'm':
			mflag = 1;
			break;

		case 'o':
			oflag = 1;
			break;

		case 'p':
			if (strlen(optarg))
				Fpass = optarg;
			pflag = 1;
			break;

		case 's':
			Shell = optarg;
			break;

		case 'u':
			Uidstr = optarg;
			break;

		case 'a':
			if (audit) {
				Auditmask = optarg;
			}
			else {
				no_service(M_NO_AUDIT, EX_SYNTAX, cll, cmdlp);
			}
			break;

		case 'h':
			if (mac) {
				usr_lvl = optarg;
				if (!*usr_lvl) {
					fatal(M_MUSAGE, cll, cmdlp, mac_str,
						(audit ? aud_str : ""));
				}
				call_pass = call_mast = 1;
				if (!lidcnt) {
					if (*usr_lvl == '+') {
						++plus;
						++usr_lvl;
					}
					else if (*usr_lvl == '-') {
						++minus;
						++usr_lvl;
					}
					else
						++replace;
				}
				if (lvlin(usr_lvl, &level) == 0)
					lidlist[lidcnt++] = level;
				else {
					if (lidcnt && ((*usr_lvl == '+') ||
						(*usr_lvl == '-'))) {

						errmsg(M_INVALID_LVL, "", "");
						fatal(M_MUSAGE, cll, cmdlp, mac_str,
							(audit ? aud_str : ""));
					}
					else {
						fatal(M_INVALID_LVL, cll, cmdlp, NULL, NULL);
					}
				}
				hflag = 1;
			}
			else {
				no_service(M_NO_MAC_H, EX_SYNTAX, cll, cmdlp);
			}
			break;

		case 'v':
			if (mac) {
				Def_lvl = optarg;
				if (!*Def_lvl) {
					fatal(M_MUSAGE, cll, cmdlp, mac_str,
						(audit ? aud_str : ""));
				}
				vflag = 1;
			}
			else {
				no_service(M_NO_MAC_V, EX_FAILURE, cll, cmdlp);
			}
			break;

		case '?':
			errmsg(M_MUSAGE, (mac ? mac_str : ""),
				(audit ? aud_str : ""));
			adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
			clean_up(EX_SYNTAX);
			break;
		}	/* end of switch */

	/*
	 * check syntax
	*/
	if (optind != argc - 1) {
		errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
		clean_up(EX_SYNTAX);
	}
	if ((uid && !oflag) || (mflag && !Dir)) {
		errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
		adumprec(ADT_MOD_USR, EX_SYNTAX, cll, cmdlp);
		clean_up(EX_SYNTAX);
	}

	logname = argv[optind];

	(void) strcpy(indxp->name, logname);

	/*
	 * Set a lock on the password file so that no other
	 * process can update the "/etc/passwd" and "/etc/shadow"
	 * at the same time.  Also, ignore all signals so the
	 * work isn't interrupted by anyone.  In addition,
	 * check appropriate privileges.
	*/
	if (lckpwdf() != 0) {
		if (access(SHADOW, EFF_ONLY_OK|W_OK) != 0) {
			(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			adumprec(ADT_MOD_USR, 1, cll, cmdlp);
			clean_up(1);
		} 
		(void) pfmt(stderr, MM_ERROR, busy);
		adumprec(ADT_MOD_USR, 8, cll, cmdlp);
		clean_up(8);
	}
	for (i = 1; i < NSIG; ++i)
		(void) sigset(i, SIG_IGN);

	/*
	 * clear errno since it was set by some of the calls to sigset
	 */
	errno = 0;

	if (((pstruct = sgetpwnam(logname)) == NULL) || (getiasz(indxp) != 0)) {
		errmsg(M_EXIST, logname);
		adumprec(ADT_MOD_USR, EX_NAME_NOT_EXIST, cll, cmdlp);
		clean_up(EX_NAME_NOT_EXIST);
	}
	
	mastp = (struct master *) malloc(indxp->length);

	if (mastp == NULL) {
		(void) pfmt(stderr, MM_ERROR, ":0:failed malloc\n");
		adumprec(ADT_MOD_USR, 1, cll, cmdlp);
		clean_up(1);
	}
	if (getianam(indxp, mastp) != 0) {
		(void) pfmt(stderr, MM_ERROR, ":0:failed getianam\n");
		adumprec(ADT_MOD_USR, 1, cll, cmdlp);
		clean_up(1);
	}


	if (Dir && isbusy(logname)) {
		errmsg(M_BUSY, logname, "change");
		adumprec(ADT_MOD_USR, EX_BUSY, cll, cmdlp);
		clean_up(EX_BUSY);
	}

	if (new_logname && strcmp(new_logname, logname)) {
		switch (valid_login(new_logname, (struct pwd **) NULL)) {
		case INVALID:
			fatal(M_INVALID, cll, cmdlp, new_logname, "login name");
			/*NOTREACHED*/

		case NOTUNIQUE:
			errmsg(M_USED, new_logname);
			adumprec(ADT_MOD_USR, EX_NAME_EXISTS, cll, cmdlp);
			clean_up(EX_NAME_EXISTS);
			/*NOTREACHED*/
		default:
			call_mast = 1;
			call_shad = call_pass = 1;
			passwd_st.pw_name = new_logname;
			shadow_st.sp_namp = new_logname;
			(void) strcpy(mastp->ia_name, new_logname);
			break;
		}
		/*
	 	* even though the ``logname'' might be valid, check to
	 	* see if it's ALL numeric in which case print a diagnostic
	 	* indicating that this might not be such a good idea.
		*/
		if (all_numeric(new_logname)) {
			(void) pfmt(stderr, MM_WARNING, errmsgs[M_NUMERIC],
				new_logname);
		}
		if (audit) {
			if (getadtnam(new_logname, adtp) == 0)
				fatal(M_USED, cll, cmdlp, new_logname, "");
		}
	}
	else {
		passwd_st.pw_name = logname;
		shadow_st.sp_namp = logname;
	}
	/*
	 * check and set the default values for new users
	 * and initialize both password structures with valid
	 * information for the new user
	*/
	validate(&passwd_st, &shadow_st, pstruct, cll, cmdlp,
			grps, gid, uid, mflag, oflag);

	/*
	 * update the password file
	*/
	if (call_pass && mod_passwd(&passwd_st)) {
		file_error(cll, cmdlp);
	}
	/*
	 * update the shadow file
	*/
	if (call_shad && mod_shadow(&shadow_st)) {
		file_error(cll, cmdlp);
	}
	/*
	 * update the audit file
	*/
	if (audit && (*Auditmask || new_logname)) {
		if (mod_audit()) {
			file_error(cll, cmdlp);
		}
	}
	/*
	 * set the master structure and call putiaent
	*/
	if (call_mast && mod_master()) {
		file_error(cll, cmdlp);
	}
	if (mac) {
		if (replace) {
			lidlstp = &lidlist[0];
		}
		else if (minus) {
			lidlstp = mastp->ia_lvlp;
			lidcnt = mastp->ia_lvlcnt;
		}
		if (vflag && !(plus || replace || minus)) {
			lidlstp = mastp->ia_lvlp;
			lidcnt = mastp->ia_lvlcnt;
		}
		if (new_logname || vflag || hflag) {
			if (lvlia(IA_WRITE, (level_t **)lidlstp,
				passwd_st.pw_name, &lidcnt)) {
				(void) pfmt(stderr, MM_ERROR, ":0:can not update level file\n");
				file_error(cll, cmdlp);
			}
		}
		if (new_logname) {
			(void) strcpy(lvlfl, LVLDIR);
			(void) strcat(lvlfl, logname);
			(void) unlink(lvlfl);
		}
	}

	if(new_home){
		(void) rm_files(pstruct->pw_dir);
	}

	replace_all(cll, cmdlp);

	(void) ulckpwdf();

	adumprec(ADT_MOD_USR, 0, cll, cmdlp);

	clean_up(0);

	/*NOTREACHED*/
}


/*
 * Procedure:	ck_p_sz
 *
 * Notes:	Check for the size of the whole passwd entry
 */
static	int
ck_p_sz(pwp)
	struct	passwd	*pwp;
{
	char ctp[128];

	/*
	 * Ensure that the combined length of the individual
	 * fields will fit in a passwd entry. The 1 accounts for the
	 * newline and the 6 accounts for the colons (:'s)
	*/
	if ((int) (strlen(pwp->pw_name) + 1 +
		(int) sprintf(ctp, "%ld", pwp->pw_uid) +
		(int) sprintf(ctp, "%ld", pwp->pw_gid) +
		(int) strlen(pwp->pw_comment) +
		(int) strlen(pwp->pw_dir)	+
		(int) strlen(pwp->pw_shell) + 6) > (ENTRY_LENGTH - 1)) {

		(void) pfmt(stderr, MM_ERROR, "New password entry too long");
		return 1;
	}
	return 0;
}


/*
 * Procedure:	ck_s_sz
 *
 * Notes:	Check for the size of the whole shadow entry
 */
static	int
ck_s_sz(ssp)
	struct	spwd	*ssp;
{
	char ctp[128];

	/*
	 * Ensure that the combined length of the individual
	 * fields will fit in a shadow entry. The 1 accounts for the
	 * newline and the 7 accounts for the colons (:'s)
	*/
	if ((int) (strlen(ssp->sp_namp) + 1 +
		(int) strlen(ssp->sp_pwdp) +
		(int) sprintf(ctp, "%ld", ssp->sp_lstchg) +
		(int) sprintf(ctp, "%ld", ssp->sp_min) +
		(int) sprintf(ctp, "%ld", ssp->sp_max) + 
		(int) sprintf(ctp, "%ld", ssp->sp_warn) + 
		(int) sprintf(ctp, "%ld", ssp->sp_inact) + 
		(int) sprintf(ctp, "%ld", ssp->sp_expire) + 7) > (ENTRY_LENGTH - 1)) {

		(void) pfmt(stderr, MM_ERROR, "New password entry too long");
		return 1;
	}
	return 0;
}


/*
 * Procedure:	file_error
 *
 * Notes:	issues a diagnostic message, removes all temporary files
 *		(unconditionally), allows updtate access to the password
 *		files again, writes an ausit record, and calls clean_up().
 */
static	void
file_error(cmdll, cline)
	int	cmdll;
	char	*cline;
{
	(void) pfmt(stderr, MM_ERROR, nochg);
	rm_all();
	(void) ulckpwdf();
	adumprec(ADT_MOD_USR, 6, cmdll, cline);
	clean_up(6);
}


/*
 * Procedure:	bad_news
 *
 * Notes:	Exactly the same functionality as ``file_error'' above
 *		but it prints a different diagnostic message and status
 *		number.
 */
static	void
bad_news(cmdll, cline)
	int	cmdll;
	char	*cline;
{
	(void) pfmt(stderr, MM_ERROR, miss);
	rm_all();
	(void) ulckpwdf();
	adumprec(ADT_MOD_USR, 7, cmdll, cline);
	clean_up(7);
}


/*
 * Procedure:	fatal
 *
 * Notes:	prints out diagnostic message, writes an audit record,
 *		and calls clean_up() with the given exit value.
 */
static	void
fatal(msg_no, cmdll, cline, arg1, arg2)
	int	msg_no,
		cmdll;
	char	*cline,
		*arg1,
		*arg2;
{
	errmsg(msg_no, arg1, arg2);
	adumprec(ADT_MOD_USR, EX_BADARG, cmdll, cline);
	(void) ulckpwdf();
	clean_up(EX_BADARG);
}


/*
 * Procedure:	mod_passwd
 *
 * Notes:	modify user attributes in "/etc/passwd" file
 */
static	int
mod_passwd(passwd)
	struct	passwd	*passwd;
{
	struct	stat	statbuf;
	struct	passwd	*pw_ptr1p;
	FILE		*fp_temp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(PASSWD, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & (S_IRUSR|S_IRGRP|S_IROTH)));

 	if ((fp_temp = fopen(PASSTEMP , "w")) == NULL)
		return errno;

	/*
	 * open the password file.
	*/
	setpwent();
	/*
	 * The while loop for reading PASSWD entries
	*/
	while (!end_of_file) {
		if ((pw_ptr1p = (struct passwd *) getpwent()) != NULL) {
			if (!strcmp(pw_ptr1p->pw_name, logname)) {
				found = 1;
				if (*new_logname)
					pw_ptr1p->pw_name = passwd->pw_name;
				if (*Uidstr)
					pw_ptr1p->pw_uid = passwd->pw_uid;
				if (*Group)
					pw_ptr1p->pw_gid = passwd->pw_gid;
				if (cflag) {
					pw_ptr1p->pw_gecos = passwd->pw_gecos;
					pw_ptr1p->pw_comment = passwd->pw_comment;
				}
				if (*Dir)
					pw_ptr1p->pw_dir = passwd->pw_dir;
				if (*Shell) {
					pw_ptr1p->pw_shell = passwd->pw_shell;
				}
				/*
				 * check the size of the password entry.
				*/
				if (ck_p_sz(pw_ptr1p)) {
					endpwent();
					(void) fclose(fp_temp);
					return 1;
				}
			}
			if (putpwent(pw_ptr1p, fp_temp)) {
				endpwent();
				(void) fclose(fp_temp);
				return 1;
			}
		} 
		else { 
			if (errno == 0)			 /* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip*/
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	/*
	 * close the password file
	*/
	endpwent();

	(void) fclose(fp_temp);

	if (error >= 1) {
		(void) pfmt(stderr, MM_ERROR, badent, PASSWD);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, PASSTEMP))
		return errno;

	if (chown(PASSTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}


/*
 * Procedure:	mod_shadow
 *
 * Notes:	modify user attributes in "/etc/shadow" file
 */
static	int
mod_shadow(shadow)
	struct	spwd	*shadow;
{
	struct	stat	statbuf;
	struct	spwd	*sp_ptr1p;
	FILE		*fp_temp;
	register int	error = 0,
			found = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(SHADOW, &statbuf) < 0) 
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(SHADTEMP , "w")) == NULL)
		return errno;

	/*
	 * The while loop for reading SHADOW entries
	*/
	while (!end_of_file) {
		if ((sp_ptr1p = (struct spwd *)getspent()) != NULL) {
			if (!strcmp(logname, sp_ptr1p->sp_namp)) {
				found = 1;
				if (*new_logname)
					sp_ptr1p->sp_namp = shadow->sp_namp;
				if (*Inact)
			      		sp_ptr1p->sp_inact = shadow->sp_inact;
				if (*Expire)
			      		sp_ptr1p->sp_expire = shadow->sp_expire;
				if (pflag) {
					if (strlen(Fpass)) {
						sp_ptr1p->sp_flag &=
							(unsigned) 0xffffff00;
						sp_ptr1p->sp_flag |=
							(unsigned) pgenp[0];
					}
					else
						sp_ptr1p->sp_flag &=
							(unsigned) 0xffffff00;
				}
				/*
	 			* check the size of the shadow entry.
				*/
				if (ck_s_sz(sp_ptr1p)) {
					(void) fclose(fp_temp);
					endspent();
					return 1;
				}
			}
			if (putspent(sp_ptr1p, fp_temp)) {
				(void) fclose(fp_temp);
				endspent();
				return 1;
			}
		} 
		else { 
			if (errno == 0)			/* end of file */
				end_of_file = 1;
			else if (errno == EINVAL) {	/* Bad entry found, skip */
				error++;
				errno = 0;
			}  
			else			/* unexpected error found */
				end_of_file = 1;		
		}
	}

	/*
	 * close the shadow file
	*/
	endspent();

	(void) fclose(fp_temp);

	if (error >= 1) {
		(void) pfmt(stderr, MM_ERROR, badent, SHADOW);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, SHADTEMP))
		return errno;

	if (chown(SHADTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}


/*
 * Procedure:	mod_audit
 *
 * Notes:	modifies user's audit record if auditing is installed.
 */
static	int
mod_audit()
{
	struct	stat	statbuf;
	struct	adtuser	adtuser;
	struct	adtuser	*adtup = &adtuser;
	FILE		*fp_temp;
	register int	ret = 0,
			found = 0,
			i, error = 0,
			end_of_file = 0;

	errno = 0;

	if (stat(AUDITMASK, &statbuf) < 0)
		return errno;

	(void) umask(~(statbuf.st_mode & S_IRUSR));

 	if ((fp_temp = fopen(MASKTEMP, "w")) == NULL) {
		return errno;
	}

	/*
	 * open the audit file
	*/
	setadtent();

	while (!end_of_file) {
		if ((ret = getadtent(adtup)) == 0) {
			if (!strcmp(logname, adtup->ia_name)) {
				found = 1;
				if (*new_logname)
					(void) strcpy(adtup->ia_name, adtp->ia_name);
				for (i = 0; i < ADT_EMASKSIZE; ++i)
					adtup->ia_amask[i] = adtp->ia_amask[i];
			}
			if (putadtent(adtup, fp_temp)) {
				(void) fclose(fp_temp);
				endadtent();
				return 1;
			}
		}
		else {
			if (ret == -1) 
				end_of_file = 1;
			else if (errno == EINVAL){	/*bad entry found, skip */
				error++;
				errno = 0;
			} else		/* unexpected error found */
				end_of_file = 1;
		}	
	}
	/*
	 * close the audit file
	*/
	endadtent();

	(void) fclose(fp_temp);

	if (error >= 1) {
		(void) pfmt(stderr, MM_ERROR, a_badent, AUDITMASK);
	}

	if (!found)
		return 1;

	/*
	 * try to set the level of the new file.  If the process
	 * can't, that means this is being run by a non-admin-
	 * istrator and they shouldn't be allowed the privilege
	 * of updating any files that contain trusted data.
	*/
	if (errno = ck_and_set_flevel(statbuf.st_level, MASKTEMP))
		return errno;

	if (chown(MASKTEMP, statbuf.st_uid, statbuf.st_gid) < 0) {
		return errno;
	}

	return error;
}


/*
 * Procedure:	rm_all
 *
 * Notes:	does an unconditional unlink (without error checking)
 *		for all temporary files created by usermod.
 */
static	void
rm_all()
{
	(void) unlink(SHADTEMP);
	(void) unlink(PASSTEMP);
	(void) unlink(MASKTEMP);
}


/*
 * Procedure:	replace_all
 *
 * Notes:	renames all the REAL files created to the OLD file,
 * 		renames all the TEMP files created to the REAL file.
 */
static	void
replace_all(cll, cmdlp)
	int	cll;
	char	*cmdlp;
{
	if (call_pass) {
		if (rename(PASSWD, OPASSWD) == -1)
			file_error(cll, cmdlp);

		if (rename(PASSTEMP, PASSWD) == -1) {
			if (link (OPASSWD, PASSWD))
				bad_news(cll, cmdlp);
			file_error(cll, cmdlp);
		}
	}
	if (call_shad) {
		if (access(OSHADOW, 0) == 0) {
			if (rename(SHADOW, OSHADOW) == -1) {
				if (rec_pwd()) 
					bad_news(cll, cmdlp);
				else file_error(cll, cmdlp);
			}
		}
		
		if (rename(SHADTEMP, SHADOW) == -1) {
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news(cll, cmdlp);
			if (rec_pwd()) 
				bad_news(cll, cmdlp);
			else file_error(cll, cmdlp);
		}
	}
	if (audit && (*Auditmask || new_logname)) {
		if (access(OAUDITMASK, 0) == 0) {
			if (rename(AUDITMASK, OAUDITMASK) == -1) {
				(void) unlink(MASKTEMP);
				if (rename(OSHADOW, SHADOW) == -1)
					bad_news(cll, cmdlp);
				if (rec_pwd()) 
					bad_news(cll, cmdlp);
				else file_error(cll, cmdlp);
			}
		}

		if (rename(MASKTEMP, AUDITMASK) == -1) {
			(void) unlink(MASKTEMP);
			if (rename(OSHADOW, SHADOW) == -1)
				bad_news(cll, cmdlp);
			if (rec_pwd()) 
				bad_news(cll, cmdlp);
			else file_error(cll, cmdlp);
		}
	}
	restore_ia(); /* Restore attributes on master and index files*/
	return;
}


/*
 * Procedure:	no_service
 *
 * Notes:	prints out three error messages indicating that this
 *		particular service was not available at this time,
 *		writes an audit record, and calls clean_up() with the
 *		value passed as exit_no.
 */
static	void
no_service(msg_no, exit_no, cll, cline)
	int	msg_no,
		exit_no,
		cll;
	char	*cline;
{
	errmsg(msg_no);
	errmsg(M_NO_SERVICE);
	errmsg(M_MUSAGE, (mac ? mac_str : ""), (audit ? aud_str : ""));
	adumprec(ADT_MOD_USR, exit_no, cll, cline);
	clean_up(exit_no);
}


/*
 * Procedure:	validate
 *
 * Notes:	this routine sets the values in the password structures
 *		for any value that can be specified via the command line.
 *		This is where ALL validation for those values takes place.
 */
static	void
validate(pass, shad, o_pass, cll, cmdlp, grps, gid, uid, mflg, oflg)
	struct	passwd	*pass;
	struct	spwd	*shad;
	struct	passwd	*o_pass;
	int		cll;
	char		*cmdlp;
	char		*grps;
	gid_t		gid;
	int		uid,
			mflg,
			oflg;
{
	register int	i, j, k,
			ret = 0,
			dupcnt = 0,
			existed = 0,
			use_skel = 0;
	struct	stat	statbuf;
	struct	group	*g_ptr;
	char		*ptr,
			*s_dir = NULL,
			lvlname[LVL_MAXNAMELEN + 1];
	struct	tm	*tm_ptr;
	long		lvlcnt,
			date = 0,
			inact = 0;
	level_t		level,
			*lvlp,
			*nlvlp,
			*ia_lvlp,
			def_lid = *mastp->ia_lvlp;
	uinfo_t		uinfo;



	Gidlist = (int **)0;
	Sup_gidlist = (gid_t *) malloc((sysconf(_SC_NGROUPS_MAX) * sizeof(gid_t)));

	if (mac) {
		if (hflag) {
			j = lidcnt;
			/*
			 * check to see if there were any duplicate
			 * levels passed as arguments via the -h option.
			*/
			for (i = 0; i < lidcnt; ++i) {
				for (k = (i + 1); k < lidcnt; ++k) {
					if (lidlist[i] && lidlist[i] == lidlist[k]) {
						--j;
						lidlist[k] = 0;
					}
				}
			}
			lidcnt = j;
			/*
			 * sort the lidlist
			*/
			for (i = 0; i < lidcnt; ++i) {
				if (!lidlist[i]) {
					if (((i + 1) < lidcnt) && lidlist[i + 1]) {
						lidlist[i] = lidlist[i + 1];
						--j;
					}
				}
			}
			lidcnt = j;
			/*
			 * add levels
			*/
			if (plus) {
				/*
			 	 * remove duplicates
				*/
				for (i = 0; i < lidcnt; i++) {
					lvlp = mastp->ia_lvlp;
					for (j = 0; j < mastp->ia_lvlcnt; j++) {
						if (lidlist[i] == *lvlp++) {
							++dupcnt;
							lidlist[i] = 0;
						}
					}
				}
				j = dupcnt;
				for (i = 0; j; i++) {
					if (lidlist[i] == 0) {
						--j;
						for (k = i; k < lidcnt; k++) { 
							lidlist[k] = lidlist[k + 1];
						}
					}
				}
				lidcnt -= dupcnt;

				lidlstp = (level_t *)calloc((lidcnt+mastp->ia_lvlcnt),
						sizeof(level_t));
				if (lidlstp == NULL) {
					(void) pfmt(stderr, MM_ERROR, ":0: failed calloc\n");
					adumprec(ADT_MOD_USR, 1, cll, cmdlp);
					exit(1);
				}
				lvlp = mastp->ia_lvlp;
				nlvlp = lidlstp;
				for (i = 0; i < mastp->ia_lvlcnt; i++) 
					*nlvlp++ = *lvlp++;
				for (i = 0; i < lidcnt; i++) 
					*nlvlp++ = lidlist[i];
				lidcnt += mastp->ia_lvlcnt;
			}
			/*
			 * delete levels
			*/
			if (minus) {
				for (i = 0; i < lidcnt; i++) {
					lvlp = mastp->ia_lvlp;
					nlvlp = mastp->ia_lvlp;
					for (j = 0; j < mastp->ia_lvlcnt; j++, lvlp++) {
						if (lidlist[i] == *lvlp) {
							nlvlp = lvlp;
							for (k = j; k < mastp->ia_lvlcnt; k++) 
								*lvlp++ = *++nlvlp;
							mastp->ia_lvlcnt--;
							break;
						}
					}
				}
			}
		}
		/*
		 * if the -v option was present on the command line,
		 * check the validity of the value now.
		*/
		if (vflag) {
			if (lvlin(Def_lvl, &level) == 0) 
				def_lid = level;
			else {
				fatal(M_INVALID_DLVL, cll, cmdlp, "", "");
			}
		}
		/*
		 * general processing if either MAC-related flag was
		 * specified on the command line.
		*/
		if (hflag || vflag) {
			if (replace) {
				lidlstp = &lidlist[0];
			}  
			else if (minus) {
				lidlstp = mastp->ia_lvlp;
				lidcnt = mastp->ia_lvlcnt;
			}
			if (vflag && !(plus || replace || minus)) {
				lidlstp = mastp->ia_lvlp;
				lidcnt = mastp->ia_lvlcnt;
			}
			if (ck_def_lid(lidlstp, lidcnt, def_lid)) {
				if (minus) {
					lvlout(&def_lid, &lvlname[0],
						MAXNAMELEN + 1, LVL_FULL);
					fatal(M_NO_DEFLVL, cll, cmdlp, lvlname, "");
				}
				fatal(M_INVALID_DLVL, cll, cmdlp, "", "");
			}
			if (vflag || ((lidcnt > mastp->ia_lvlcnt) && replace)
				|| (lidcnt && plus)) {

				call_mast = 1;
				new_master = 1;
			}
		}
	}

	if (Uidstr) {
		/* convert Uidstr to integer */
		uid = (int) strtol(Uidstr, &ptr, (int) 10);
		if (*ptr) {
			fatal(M_INVALID, cll, cmdlp, Uidstr, "uid");
		}
		if (uid_age(CHECK, uid, 0) == 0) {
			errmsg(M_UID_AGED, uid);
			adumprec(ADT_ADD_USR, EX_ID_EXISTS, cll, cmdlp);
			clean_up(EX_ID_EXISTS);
		}

		if (uid != o_pass->pw_uid) {
			switch (valid_uid(uid, NULL)) {
			case NOTUNIQUE:
				if (!oflg) {
					/* override not specified */
					errmsg(M_UID_USED, uid);
					adumprec(ADT_MOD_USR, EX_ID_EXISTS, cll, cmdlp);
					clean_up(EX_ID_EXISTS);
				}
				if (uid <= DEFRID)
				/* FALLTHROUGH */
			case RESERVED:
				(void) pfmt(stderr, MM_WARNING,
					errmsgs[M_RESERVED], uid);
				break;
			case TOOBIG:
				fatal(M_TOOBIG, cll, cmdlp, "uid", Uidstr);
				break;
			case INVALID:
				fatal(M_INVALID, cll, cmdlp, Uidstr, "uid");
				break;
			}
			call_mast = 1;
			call_pass = 1;
			pass->pw_uid = mastp->ia_uid = uid;
		}
		else
			Uidstr = NULL;
	}
	else {
		uid = o_pass->pw_uid;
	}

	if (Group) {
		switch (valid_group(Group, &g_ptr)) {
		case INVALID:
			fatal(M_INVALID, cll, cmdlp, Group, "group id");
			/*NOTREACHED*/
		case TOOBIG:
			fatal(M_TOOBIG, cll, cmdlp, "gid", Group);
			/*NOTREACHED*/
		case UNIQUE:
			errmsg(M_GRP_NOTUSED, Group);
			adumprec(ADT_MOD_USR, EX_NAME_NOT_EXIST, cll, cmdlp);
			clean_up(EX_NAME_NOT_EXIST);
			/*NOTREACHED*/
		}

		gid = g_ptr->gr_gid;

		if (gid == o_pass->pw_gid)
			Group = NULL;
		else {
			call_mast = 1;
			call_pass = 1;
			new_groups = 1;
			pass->pw_gid = mastp->ia_gid = gid;
		}
	}
	else {
		gid = o_pass->pw_gid;
	}
	/*
	 * put user's primary group into list so its added to master
	 * file information later (if necessary).
	*/
	Gidcnt = getgrps(pass->pw_name, gid);

	if (grps && *grps) {
		if (Gidlist = valid_lgroup(grps, gid)) {
			Gidcnt = i = 0;
			Sup_gidlist[Gidcnt++] = gid;
			while ((unsigned)Gidlist[i] != -1)
				Sup_gidlist[Gidcnt++] = (gid_t) Gidlist[i++];
		}
		else {
			adumprec(ADT_MOD_USR, EX_BADARG, cll, cmdlp);
			clean_up(EX_BADARG);
		}
		/*
		 * redefine login's supplentary group memberships
		*/
		ret = edit_group(logname, new_logname, Gidlist, 1);

		if (ret != EX_SUCCESS) {
			errmsg(M_UPDATE, "modified");
			adumprec(ADT_ADD_USR_GRP, ret, cll, cmdlp);
			clean_up(ret);
		}
		else {
			adumprec(ADT_ADD_USR_GRP, ret, cll, cmdlp);
		}

		if (Gidcnt > mastp->ia_sgidcnt) {
			new_master = 1;
		}
		call_mast = 1;
		new_groups = 1;
	} 

	if (Dir) {
		if (REL_PATH(Dir)) {
			fatal(M_RELPATH, cll, cmdlp, Dir, "");
		}
		if (INVAL_CHAR(Dir)) {
			fatal(M_INVALID, cll, cmdlp, Dir, "directory name");
		}
		if (strcmp(o_pass->pw_dir, Dir) != 0) {
			pass->pw_dir = Dir;
			if ((int)(strlen(Dir)) > (int)mastp->ia_dirsz )
				new_master = 1;
			else
				(void) strcpy(mastp->ia_dirp, Dir);

			mastp->ia_dirsz = strlen(Dir);
			call_mast = 1;
			call_pass = 1;
		}
		else {
			if (stat(o_pass->pw_dir, &statbuf) == 0) {
				/*
				 * ignore -m option if ``Dir'' is not
				 * different and the directory existed.
				*/
				mflg = 0;
				Dir = NULL;
			}
			use_skel = 1;
		}
	}
	if (mflg) {
		if (stat(Dir, &statbuf) == 0) {
			/* Home directory exists */
			if (check_perm(statbuf, o_pass->pw_uid, 
			   o_pass->pw_gid, S_IWOTH|S_IXOTH) != 0) {
				errmsg(M_NO_PERM, logname, Dir);
				adumprec(ADT_MOD_USR, EX_NO_PERM, cll, cmdlp);
				clean_up(EX_NO_PERM);
			}
			existed = 1;
		}
		if (stat(o_pass->pw_dir, &statbuf) < 0) {
			if (ia_openinfo(o_pass->pw_name, &uinfo) || (uinfo == NULL)) {
				statbuf.st_level = 0;
			}
			else {
				if (ia_get_lvl(uinfo, &ia_lvlp, &lvlcnt)) {
					statbuf.st_level = 0;
				}
				else {
					statbuf.st_level = *ia_lvlp;
				}
				ia_closeinfo(uinfo);	/* close master file */
			}
		}
		else {
			s_dir = o_pass->pw_dir;
		}
		if (use_skel) {
			existed = 1;
			s_dir = SKEL_DIR;
		}
		ret = create_home(Dir, s_dir, logname, existed, uid,
			gid, statbuf.st_level);

		if (ret == EX_SUCCESS) {
			new_home = 1;
		}
		else {
			if (!existed) {
				(void) rm_files(Dir);
			}
			adumprec(ADT_MOD_USR, M_NOSPACE, cll, cmdlp);
			clean_up(EX_NOSPACE);
		}
	}

	if (Shell) {
		int	abits = EFF_ONLY_OK | EX_OK | X_OK;

		if (REL_PATH(Shell))
			fatal(M_RELPATH, cll, cmdlp, Shell, "");
		if (INVAL_CHAR(Shell)) {
      			fatal(M_INVALID, cll, cmdlp, Shell, "shell name"); 
		}
		if (strcmp(o_pass->pw_shell, Shell) != 0) {
			/*
			 * check that shell is an executable file
			*/
			if (access(Shell, abits) != 0)
				fatal(M_INVALID, cll, cmdlp, Shell, "shell");
			else {
				pass->pw_shell = Shell;
				if ((int) strlen(Shell) > (int) mastp->ia_shsz)
					new_master = 1;
				else
					(void) strcpy((char *)mastp->ia_shellp, Shell);
				mastp->ia_shsz = strlen(Shell);
				call_mast = 1;
				call_pass = 1;
			}
		}
		else {
			/*
			 * ignore -s option if shell is not different
			*/
			Shell = NULL;
		}
	}

	if (cflag) {
		if (strlen(Comment)) {
			if ((int)(strlen(Comment)) > CMT_SIZE)
				fatal(M_INVALID, cll, cmdlp, Comment, "Comment");
			if (INVAL_CHAR(Comment)) {
				fatal(M_INVALID, cll, cmdlp, Comment, "comment");
			}
			/*
			 * ignore comment if its not different from passwd entry
			*/
			if (!strcmp(o_pass->pw_comment, Comment))
				cflag = 0;

		}
		else {
			Comment = NULL;
		}
		if (cflag) {		/* not ignored above */
			call_pass = 1;
			pass->pw_comment = pass->pw_gecos = Comment;
		}
	}
	/*
	 * inactive string is a positive integer
	*/
	if (Inact) {
		/* convert Inact to integer */
		inact = (int) strtol(Inact, &ptr, (int) 10);
		if (*ptr || inact < -1) {
			fatal(M_INVALID, cll, cmdlp, Inact, "inactivity string");
		}
		call_mast = 1;
		call_shad = 1;
		shad->sp_inact = mastp->ia_inact = inact;
	}

	/* expiration string is a date, newer than today */
	if (Expire) {
		if (!*Expire) {
			Expire = "0";
			shad->sp_expire = 0;
		}
		else {
			(void) putenv(DATMSK);
                	if ((tm_ptr = getdate(Expire)) == NULL) {
				if ((getdate_err > 0) && 
				     (getdate_err < GETDATE_ERR)) 
					fatal(M_GETDATE+getdate_err - 1, cll,
						cmdlp, "", "");
				else
					fatal(M_INVALID, cll, cmdlp, Expire, "expire date");
			}
 
			if ((date =  mktime(tm_ptr)) < 0)
				fatal(M_INVALID, cll, cmdlp, Expire, "expire date");

       			if ((shad->sp_expire = (date / DAY)) <= DAY_NOW)
				fatal(M_INVALID, cll, cmdlp, Expire, "expire date");
		}
		call_mast = 1;
		call_shad = 1; 
		mastp->ia_expire = shad->sp_expire;
	}

	if (pflag) {
		if (strlen(Fpass)) {
			pgenp = (char *) malloc((int) strlen(Fpass) + 1);

			(void) sprintf(pgenp, "%s", Fpass);
			if ((strlen(pgenp) > (size_t)1) || !isprint(pgenp[0])) {
				errmsg(M_BAD_PFLG);
				clean_up(EX_SYNTAX);
			}
			mastp->ia_flag &= (unsigned) 0xffffff00;
			mastp->ia_flag |= (unsigned) pgenp[0];
		}
		else
			mastp->ia_flag &= (unsigned) 0xffffff00;

		call_mast = 1;
		call_shad = 1;
	}
	/*
	 * auditing event types or classes
	*/
	if (audit) {
		if (getadtnam(logname, adtp) != 0) {
			(void) pfmt(stderr, MM_ERROR, ":0:%s not found in audit file\n",
				logname);
			adumprec(ADT_MOD_USR, 1, cll, cmdlp);
			clean_up(1);
		}
		if (*Auditmask) {
			if (!cremask(ADT_UMASK, Auditmask, adtp->ia_amask)) {
				for (i = 0; i < ADT_EMASKSIZE; i++)
					mastp->ia_amask[i] = adtp->ia_amask[i];

				call_mast = 1;
			}
			else {
				audit = 0;
			}
		}
		if (audit && *new_logname)
			(void) strcpy(adtp->ia_name, new_logname);
	}
	return;
}


/*
 * Procedure:	rec_pwd
 *
 * Notes:	unlinks the current password file ("/etc/passwd") and
 *		links the old password file to the current.  Only called
 *		if there is a problem after new password file was created.
 */
static	int
rec_pwd()
{
	if (unlink (PASSWD) || link (OPASSWD, PASSWD))
		return -1;
	return 0;
}


static	int
ck_def_lid(lidp, cnt, d_lid)
	level_t	*lidp;
	long	cnt;
	level_t	d_lid;
{
	int	i;
	level_t *p;

	p = lidp;

	for (i = 0; i < cnt; i++) {
		if (d_lid == *p++) {
			*--p = *lidp;
			*lidp = d_lid;
			return 0;
		}
	}
	return 1;
}


/*
 * Procedure:	mod_master
 *
 * Notes:	modifies user's master file entry
 */
static	int

mod_master()
{
	long	dirsz = 0,
		mastsz = 0;

	int	i, cnt = 0,
		shsz = 0,
		g_cnt = 0;
	char	*tmpsh = NULL,
		*tmpdir = NULL;

	if (new_master) {
		if (plus || replace)
			mastsz = ( sizeof(struct master) 
				+ mastp->ia_dirsz + mastp->ia_shsz + 2
				+ (lidcnt * sizeof(level_t) ) );
		else
			mastsz = (sizeof(struct master) 
				+ mastp->ia_dirsz + mastp->ia_shsz + 2
				+ (mastp->ia_lvlcnt * sizeof(level_t)));

		if (new_groups)
			mastsz += (Gidcnt * sizeof(gid_t));
		else
			mastsz += (mastp->ia_sgidcnt * sizeof(gid_t));

		new_mastp = (struct master *) malloc(mastsz);

		if (new_mastp == NULL) {
			(void) pfmt(stderr, MM_ERROR, ":0:failed malloc\n");
			return 1;
		}

		masp = new_mastp;

		(void) strcpy(new_mastp->ia_name, mastp->ia_name);
		(void) strcpy(new_mastp->ia_pwdp, mastp->ia_pwdp);

		new_mastp->ia_uid = mastp->ia_uid;
		new_mastp->ia_gid = mastp->ia_gid;
		new_mastp->ia_lstchg = mastp->ia_lstchg;
		new_mastp->ia_min = mastp->ia_min;
		new_mastp->ia_max = mastp->ia_max;
		new_mastp->ia_warn = mastp->ia_warn;
		new_mastp->ia_inact = mastp->ia_inact;
		new_mastp->ia_expire = mastp->ia_expire;
		new_mastp->ia_flag = mastp->ia_flag;

		if (audit) {
			for (i = 0; i < ADT_EMASKSIZE; i++)
				new_mastp->ia_amask[i] = mastp->ia_amask[i];
		}

                new_mastp->ia_dirsz = mastp->ia_dirsz;
                new_mastp->ia_shsz = mastp->ia_shsz;

		if (plus || replace)
                	new_mastp->ia_lvlcnt = lidcnt;
		else
                	new_mastp->ia_lvlcnt = mastp->ia_lvlcnt;

		if (new_groups)
			new_mastp->ia_sgidcnt = Gidcnt;
		else
			new_mastp->ia_sgidcnt = mastp->ia_sgidcnt;

                new_mastp++;


		if (plus) {
			(void) memcpy(new_mastp, lidlstp, (lidcnt * sizeof(level_t)));
			new_mastp = (struct master *) ((char *)new_mastp 
					+ (lidcnt * sizeof(level_t)));
		}
		else if (replace) {
			(void) memcpy(new_mastp, &lidlist[0], (lidcnt * sizeof(level_t)));
			new_mastp = (struct master *) ((char *)new_mastp
					+ (lidcnt * sizeof(level_t)));
		}
		else {
			(void) memcpy(new_mastp, mastp->ia_lvlp, (mastp->ia_lvlcnt * sizeof(level_t)));
			new_mastp = (struct master *) ((char *)new_mastp 
				  + (mastp->ia_lvlcnt * sizeof(level_t)));
		}
		if (new_groups) {
			(void) memcpy(new_mastp, &Sup_gidlist[0], (Gidcnt * sizeof(gid_t)));
			new_mastp = (struct master *) ((char *)new_mastp
					+ (Gidcnt * sizeof(gid_t)));
		}
		else {
			(void) memcpy(new_mastp, mastp->ia_sgidp, (mastp->ia_sgidcnt * sizeof(gid_t)));
			new_mastp = (struct master *) ((char *) new_mastp
			 	  + (mastp->ia_sgidcnt * sizeof(gid_t)));
		}
		

		if (*Dir) 
                	(void) strcpy((char *)new_mastp, Dir);
		else
			(void) strcpy((char *)new_mastp, mastp->ia_dirp);

                new_mastp = (struct master *) ((char *)new_mastp
				+ (mastp->ia_dirsz + 1));

		if (*Shell)  
                	(void) strcpy((char *) new_mastp, Shell);
		else
			(void) strcpy((char *)new_mastp, mastp->ia_shellp);

		new_mastp = masp;

		if (putiaent(logname, new_mastp))
			return 1;
	}
	else {
		cnt = mastp->ia_lvlcnt;
		g_cnt = mastp->ia_sgidcnt;
		dirsz = mastp->ia_dirsz;
		shsz = mastp->ia_shsz;
		tmpdir = (char *)malloc(dirsz);
		tmpsh = (char *)malloc(shsz);
		(void) strcpy(tmpdir, mastp->ia_dirp);
		(void) strcpy(tmpsh, mastp->ia_shellp);

		masp = mastp;
		mastp++;

		if (replace) {
			(void) memcpy(mastp, &lidlist[0], (lidcnt * sizeof(level_t)));
			mastp = (struct master *) ((char *)mastp
				+ (lidcnt * sizeof(level_t)));
		}
		else  
			mastp = (struct master *) ((char *)mastp
				+ (cnt * sizeof(level_t)));

		if (new_groups) {
			(void) memcpy(mastp, &Sup_gidlist[0], (Gidcnt * sizeof(gid_t)));
			mastp = (struct master *) ((char *)mastp
				+ (Gidcnt * sizeof(gid_t)));
		}
		else {
			mastp = (struct master *) ((char *)mastp
				+ (g_cnt * sizeof(gid_t)));
		}
		if (Dir) 
               		(void) strcpy((char *)mastp, Dir);
		else 
			(void) strcpy((char *)mastp, tmpdir);

                mastp = (struct master *) ((char *)mastp + (dirsz + 1));

		if (Shell) 
                	(void) strcpy((char *)mastp, Shell);
		else
			(void) strcpy((char *)mastp, tmpsh);

		mastp = masp;

		if (replace)
			mastp->ia_lvlcnt = lidcnt;
		if (new_groups)
			mastp->ia_sgidcnt = Gidcnt;

		if (putiaent(logname, mastp))
			return 1;
	}
	return 0;
}
/*
 * Procedure:	getgrps
 *
 * Notes:	reads the group entries from the "/etc/group" file
 *		for the specified user name.
 */
static	int
getgrps(name, gid)
	char	*name;
	gid_t	gid;
{
	register int	i;
	int		ngroups = 0,
			grp_max = 0;
	register struct	group *grp;
 
	grp_max = sysconf(_SC_NGROUPS_MAX);

	if (gid >= 0)
		Sup_gidlist[ngroups++] = gid;

	setgrent();

	while (grp = getgrent()) {
		if (grp->gr_gid == gid)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (strcmp(grp->gr_mem[i], name))
				continue;
			if (ngroups == grp_max) {
				endgrent();
				return ngroups;
			}
			Sup_gidlist[ngroups++] = grp->gr_gid;
		}
	}
	endgrent();
	return ngroups;
}


/*
 * Procedure:	clean_up
 *
 * Notes:	checks to see if a new home directory was created.
 *		If so, it removes it and then calls exit() with the
 *		argument passed.
 */
static	void
clean_up(ex_val)
	int	ex_val;
{
	if (new_home && ex_val)
		(void) rm_files(Dir);

	exit(ex_val);
}

/*
 * Procedure:	sgetsave
 *
 * Notes:	 Function for sgetpwnam() which allocates space
 * 		 and copies data to passwd structure.
 */
static	char *
sgetsave(s)
	char *s;
{
	char *new = malloc((unsigned) strlen(s) + 1);
	
	if (new != NULL) {
		(void) strcpy(new, s);
	}
	return new;
}

/*
 * Procedure:	sgetpwnam
 *
 * Notes:	Save the result of a getpwnam.
 */
static	struct	passwd *
sgetpwnam(name)
	char *name;
{
	static struct passwd save;  /*passwd structure */
	register struct passwd *p;  /* pointer to passwd struct */

	if ((p = getpwnam(name)) == NULL) /* return ptr to name in passwd file */
		return NULL;
	
    	/* If previously used, free it */

	if (save.pw_name) {
		free(save.pw_name);
		free(save.pw_passwd);
		free(save.pw_comment);
		free(save.pw_gecos);
		free(save.pw_dir);
		free(save.pw_shell);
	}
	/* Save the data p is pointing to in save struct*/
	save = *p;
	save.pw_name = sgetsave(p->pw_name);
	save.pw_passwd = sgetsave(p->pw_passwd);
	save.pw_comment = sgetsave(p->pw_comment);
	save.pw_gecos = sgetsave(p->pw_gecos);
	save.pw_dir = sgetsave(p->pw_dir);
	save.pw_shell = sgetsave(p->pw_shell);

	return &save;  
}
