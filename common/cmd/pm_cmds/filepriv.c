/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pm_cmds:filepriv.c	1.28.3.3"
#ident  "$Header: filepriv.c 1.5 91/07/05 $"

/*
 * Command: filepriv
 *
 * Inheritable Privileges:	None
 * Inheritable Autorizations:	None
 *
 *       Fixed Privileges:	None
 *       Fixed Authorizations:	None
 *
 * Notes:	Set, remove, or print privileges associated with files.
 *
 */

#include	"pdf.h"

#define	RMV	0	/* These are definitions for the current */
#define	SET	1	/* options to the "filepriv" command. */
#define	PRT	2	/* "print" is really no options at all */

#define	OPDF	"/etc/security/tcb/oprivs"
#define	OPTMP	"/etc/security/tcb/.t_oprivs"
#define	PRVTMP	"/etc/security/tcb/.temp_privs"
#define	PRVDIR	"/etc/security/tcb"

#define	SYNTAX	":1:Incorrect usage\n"
#define	NOPERM	":64:Permission denied\n"
#define	BADENT	":692:Bad entry found in \"%s\"\n"
#define	NOACCES	":693:Cannot access file \"%s\"\n"
#define	NOEXEC	":694:\"%s\" not an executable file\n"
#define	BADOPTS	":695:Incompatible options specified\n"
#define	ARGREQ	"uxlibc:2:Option requires an argument -- %c\n"
#define	NOTFULL	":696:\"%s\" is not an absolute pathname\n"
#define	UNPRIV	":697:Undefined process privilege \"%s\"\n"
#define	NOENT	":698:No such file or directory for file \"%s\"\n"
#define	NOSYSC	":699:\"filepriv\" system call not in operation\n"
#define	NOPRIVS	":700:Cannot determine supported privilege sets\n"
#define	NOSUPP	":701:\"%s\" set not supported by this privilege mechanism\n"
#define	USAGE	":702:Usage: filepriv [-f priv[,...]] [-i priv[,...]] file ...\n"
#define	BADPRV	":703:Cannot use \"%s\" as both fixed and inheritable privilege\n"
#define	PDFPROB	":704:Problem writing \"%s\"\n\t\t      privileges for \"%s\" unchanged\n"

extern	int	link(),
		chown(),
		access(),
		unlink(),
		lvlfile(),
		lckprvf(),
		privnum(),
		filepriv(),
		procpriv(),
		ulckprvf(),
		getrlimit(),
		set_supported();

extern	char	*strrchr(),
		*privname(),
		*realpath();

extern	long	getcksum();

extern	setdef_t	*init_sdefs();

extern	unsigned long	umask();

static	void	usage(),
		setvec(),
		clean_up(),
		rpt_error(),
		clean_exit(),
		printprivs(),
		restore_pfile();

struct	pdf	*getpfent();

static	int	update(),
		ck_file(),
		dflag = 0,
		fflag = 0,
		iflag = 0,
		nsets = 0,
		nproc = 0,
		addpriv(),
		dofiles(),
		fcount = 0,
		gen_cksum = 1,	/* default is to always generate a cksum */
		get_procmax(),
		update_pfile(),
		setflg[PRVMAXSETS] = { 0 },
		washere[PRVMAXSETS] = { 0 };

static	char	*label,
		*getval(),
		*prvs4file,
		procmask[NPRIVS],
		res_path[MAXPATHLEN],
		*privargp[PRVMAXSETS],
		prvmask[PRVMAXSETS][NPRIVS],
		privcase[PRVMAXSETS][ABUFSIZ];

static	priv_t	pvec[NPRIVS],
		*fbufp = &pvec[0];

static	struct	pdf	nent;		/* used by ck_file() and update() */

static	setdef_t	*sdefs = (setdef_t *)0;

static	ulong	objtyp = PS_FILE_OTYPE;

main(argc, argv)
	int    argc;
	char **argv;
{
	extern	int	optind;
	extern	char	*optarg;
	char	*cp,
		*ptr,
		*cmdnm,
		*setname,
		*lprefix = "UX:";
	FILE	*def_fp;
	register int	psize = 0, i,
			opterr = 0, j,
			c, status = SUCCESS;
	register unsigned	nsize = 0;

	(void) setlocale(LC_ALL, "");
	setcat("uxcore");
	/*
	 * get the "simple" name of the command for use
	 * in diagnostic messages.
	*/
	cmdnm = cp = argv[0];
	if ((cp = strrchr(cp, '/')) != NULL) {
		cmdnm = ++cp;
	}
	label = (char *)malloc(strlen(cmdnm) + strlen(lprefix) + 1);
	(void) sprintf(label, "%s%s", lprefix, cmdnm);
	(void) setlabel(label);

	/*
	 * open the privcmds default file (if it exists)
	 * and determine if the cksum should be generated.
	*/
	if ((def_fp = defopen(PRIV_DEF)) != NULL) {
		if ((ptr = defread(def_fp, "GEN_CKSUM")) != NULL) {
			if (*ptr) {
				if (!strcmp(ptr, "No")) {
					/*
					 * DON'T generate a cksum value
					*/
					gen_cksum = 0;
				}
			}
		}
		(void) defclose(def_fp);
	}
	/* do initialization of required data structures */

	for (i = 0; i < NPRIVS; ++i)
		pvec[i] = (priv_t)0;

	for (i = 0; i < PRVMAXSETS; ++i) {
		privargp[i] = &privcase[i][0];
		for (j = 0; j < NPRIVS; ++j) {
			prvmask[i][j] = 0;
		}
	}

	if ((nsets = secsys(ES_PRVSETCNT, 0)) < 0) {
		nsets = 0;
	}
	else {
		sdefs = init_sdefs(nsets);
	}

	nproc = get_procmax(sdefs);

	/*
	 * Read in the argument list using the approved method
	 * of "getopt".  Only three arguments allowed (four,
	 * if you count NO arguments at all).
	 */
	while((c = getopt(argc, argv, "df:i:")) != EOF ) {
		switch(c) {
		case 'd':		/* delete keyletter */
			++dflag;
			continue;
		case 'f':		/* fixed privileges keyletter */
			++fflag;
			setname = "fixed";
			++setflg[c];
			/*
			 * Parse the argument list and set up for use
			 * later on.
			 */
			setvec(c, setname, optarg, fbufp, &fcount);
			if (sdefs && !set_supported(sdefs, nsets, setname, objtyp)) {
				if (setflg[c] == 1) {
					(void) pfmt(stderr, MM_WARNING, NOSUPP, setname);
				}
			}
			continue;
		case 'i':		/* inheritable privileges keyletter */
			++iflag;
			setname = "inher";
			++setflg[c];
			/*
			 * Parse the argument list and set up for use
			 * later on.
			 */
			setvec(c, setname, optarg, fbufp, &fcount);
			if (sdefs && !set_supported(sdefs, nsets, setname, objtyp)) {
				if (setflg[c] == 1) {
					(void) pfmt(stderr, MM_WARNING, NOSUPP, setname);
				}
			}
			continue;
		case '?':		/* Oops!!  Bad keyletter */
			opterr++;
			continue;
		}
		break;
	}
	if (opterr) {
		usage(BADSYN, MM_ACTION);	/* this routine exits! */
	}
	/*
	 * Strip off everything but the remaining files (if any)
	 * from the original "argv" vector and adjust "argc".
	 */
	argv = &argv[optind];
	argc -= optind;

	if (!argc) {			/* error, or no files specified */
		(void) pfmt(stderr, MM_ERROR, SYNTAX);
		usage(BADSYN, MM_ACTION);	/* this routine exits! */
	}
	/*
	 * The "-d" keyletter was specified with either the "-f"
	 * or "-i" keyletter.
	 */
	if ((fflag || iflag) && dflag) {
		(void) pfmt(stderr, MM_ERROR, BADOPTS);
		usage(BADSYN, MM_ACTION);
	}
	/*
	 * Start processing.  Do this case if either "-f" or
	 * "-i" keyletter (or both) was specified.
	 */
	if (fflag || iflag) {
		/* set up "prvs4file" ptr for use later */

		for (i = 0; i < PRVMAXSETS; ++i) {
			if ((psize = strlen(privargp[i]))) {
				nsize += psize;
			}
		}
		prvs4file = (char *)malloc(nsize + 1);
		prvs4file[0] = '\0';
		for (i = 0; i < PRVMAXSETS; ++i) {
			if (strlen(privargp[i])) {
				(void) strcat(prvs4file, privargp[i]);
			}
		}
		if (nproc) {
			status = dofiles(SET, argc, argv, fbufp, fcount);
		}
	}
	/*
	 * Do this case if the "-d" keyletter was specified.
	 */
	else if (dflag) {
		status = dofiles(RMV, argc, argv, fbufp, fcount);
	}
	/*
	 * This case is for printing the file privileges (no options).
	 */
	else {
		status = dofiles(PRT, argc, argv, fbufp, NPRIVS);
	}

	exit(status);
	/* NOTREACHED */
}


/*
 * Procedure:	dofiles
 *
 * Notes:	This routine scans the remaining argument list and makes the
 *		appropriate "filepriv" call.
 */
static	int
dofiles(act, arg, vcp, prvec, nents)
	int	act;
	int	arg;
	char	*vcp[];
	priv_t	*prvec;
	int	nents;
{
	register int	cnt, i,
			fail = 0,
			rcode = 0,
			locked = 0,
			nfiles = 0,
			success = 0,
			fp_failed = 0,
			sav_errno = 0;

	if (arg > 1)		/* more than one file (used for PRT) */
		++nfiles;

	if (act == SET || act == RMV) {
		if (lckprvf() != SUCCESS) {
			/*
			 * Locking of privilege data file failed.
			 * Check "errno" to find out why.
			 */
			rpt_error(errno, PDF);
			exit(1);
		}
		/*
		 * If locking was successful, set the "locked" variable
		 * for use later.
		 */
		locked = 1;

		/* ignore all the signals */
		for (i = 1; i < NSIG; i++) {
			if (sigset(i, SIG_IGN) == SIG_ERR) {
				/*
				 * If the signal was SIGKILL or SIGSTOP
				 * the errno will be EINVAL so set these
				 * signals to call a clean up routine.
				 */
				if (errno == EINVAL) {
					(void) sigset(i, (void (*)()) clean_exit);
				}
			}
		}
	}
	for (i = 0; i < arg; ++i) {
		/*
		 * Always clear errno for each invocation of arg.
		 */
		errno = 0;

		switch (act) {
		case SET:
		case RMV:
			fp_failed = sav_errno = 0;	/* reset */
			/*
			 * Check if the file name starts with a "/".
			 * This indicates that it's a full pathname.
			 */
			if (vcp[i][0] != '/') {
				++fail;
				(void) pfmt(stderr, MM_ERROR, NOTFULL, vcp[i]);
				continue;
			}
			/*
			 * Check out the attributes of the named file.
			 * See if this filepriv command can access it.
			 */
			if (ck_file(act, vcp[i])) {
				++fail;
				sav_errno = errno;
				rpt_error(errno, vcp[i]);
				if ((act == SET) || (act == RMV && sav_errno == EACCES)) {
					continue;
				}
			}
			/*
			 * Do the update of the temporary privilege file
			 * now.  If the update fails, don't do call the
			 * filepriv system call.
			 */
			if (update(act)) {
				(void) pfmt(stderr, MM_WARNING, PDFPROB, PDF, vcp[i]);
				continue;
			}
			/*
			 * Actual filepriv system call.  If it fails,
			 * call rpt_error() to determine the error.
			 */
			if ((cnt = filepriv(vcp[i], PUTPRV, prvec, nents)) == FAILURE) {
				/*
				 * filepriv() failed, check errno.  If errno
				 * is EPERM, issue diagnostic message and quit
				 * since no other call to filepriv() will work.
				 */
				if (errno == EPERM) {
					/*
				 	 * Restore the  privilege data file.
					 */
					restore_pfile();
					/*
		 			 * Unlock the privilege data file.
		 			 * for use by others.
					 */
					(void) ulckprvf();
					(void) pfmt(stderr, MM_ERROR, NOPERM);
					exit(1);
				}
				++fail;
				sav_errno = errno;
				rpt_error(errno, vcp[i]);
				if ((act == SET) || (act == RMV && sav_errno == EACCES)) {
					restore_pfile();
					continue;
				}
				fp_failed = 1;
			}
			if (!fp_failed)
				++success;

			(void) unlink(OPTMP);

			break;
		case PRT:
			if ((cnt = filepriv(vcp[i], GETPRV, prvec, nents)) > 0) {
				/*
				 * Print the privileges associated with
				 * the filename passed.  This information
				 * is taken from the kernel privilege
				 * table itself, not the privilege data file.
				 */
				if (sdefs) {
					++success;
					printprivs(vcp[i], nfiles, prvec, cnt);
				}
				else {
					++fail;
					(void) ulckprvf();
					(void) pfmt(stderr, MM_ERROR, NOPRIVS);
					exit(1);
				}
			}
			else if (cnt == FAILURE) {
				/* report the error and continue */
				++fail;
				rpt_error(errno, vcp[i]);
			}
			break;
		}
	}
	if (locked) {
		/*
		 * Unlock the privilege data file for use by others.
		 */
		(void) ulckprvf();
		locked = 0;
		/* reset all the signals */
		for (i = 1; i < NSIG; i++)
			(void) sigset(i, SIG_DFL);
	}
	if (fail) {
		if (success)
			rcode = SOMEBAD;
		else
			rcode = INVARG;
	}
	return rcode;
}


/*
 * Procedure:	usage
 *
 * Notes:	This routine prints out the usage message and then exits
 *		with the value passed as an argument.
 */
void
usage(ecode, action)
	int	ecode,
		action;
{
	int	i, spaces = 16;		/* 16 is the minimum number of spaces */
	char	*space = " ",		/* for header on 2nd  line of message */
		buffer[BUFSIZ],
		*p = &buffer[0];

	spaces += ((action == MM_ACTION) ? 1 : 0) + (int) strlen(label);

	(void) strcat(p, USAGE);

	/*
	 * There is more to this message so align the output to
	 * insure that the message matches the one on the manual page.
	 */
	for (i = 0; i < spaces; ++i)
		(void) strcat(p, space);

	/*
	 * Add on the remainder of the message after aligning it correctly.
	 */
	(void) strcat(p, gettxt(":705", "filepriv -d file ...\n"));

	(void) pfmt(stderr, action, p);

	exit(ecode);
}


/*
 * Procedure:	rpt_error
 *
 * Notes:	This routine checks the errno passed in and outputs the
 *		correct message via the pfmt() routine.
 */
static	void
rpt_error(err, filep)
	int	err;
	char	*filep;
{
	switch (err) {
	case EACCES:
		(void) pfmt(stderr, MM_ERROR, NOACCES, filep);
		break;
	case EINVAL:
		(void) pfmt(stderr, MM_ERROR, NOEXEC, filep);
		break;
	case EPERM:
		(void) pfmt(stderr, MM_ERROR, NOPERM);
		break;
	case ENOPKG:
		(void) pfmt(stderr, MM_ERROR, NOSYSC);
		break;
	case ENOENT:
	case ENOTDIR:
		(void) pfmt(stderr, MM_ERROR, NOENT, filep);
		break;
	default:
		if (*filep) {
			(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s %s\n",
				filep, strerror(errno));
		}
		else
			(void) pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				strerror(errno));
		break;
	}
	return;
}


/*
 * Procedure:	setvec
 *
 * Notes:	This routine scans the "argp" argument.  This should be a comma
 *		separated list of privileges.   It sets up the buffer passed to
 *		"filepriv".   It also adjusts the count for the buffer which is
 *		the number of elements in the array.
 */
void
setvec(type, setname, argp, fprivp, fcnt)
	char	type;
	char	*setname;
	char	*argp;
	priv_t	fprivp[];
	int	*fcnt;
{
	char	tmp[ABUFSIZ],
		name[BUFSIZ],
		pbuf[BUFSIZ],
		tmask[NPRIVS];

	int	i, j;
	char	*pbp = &pbuf[0];
	register int	n = 0, comma = 0,
			had_name = 0, didsome = 0;
	extern	char	*privname();

	tmp[0] = '\0';

	j = *fcnt;

	/*
	 * Initialize the tmask variable to all 0's.
	 */
	for (i = 0; i < NPRIVS; ++i)
		tmask[i] = 0;

	/*
	 * Scan the argument list.
	 */
	while (*argp) {
		while (*argp == ',')
			++argp;			/* skip over this character */
		name[0] = '\0';			/* clear out "name" */
		if (*argp) {
			/*
			 * get the name of the privilege and place in "name"
			*/
			argp = getval(argp, name);
			/*
			 * Check to see if this is a "known" privilege.
			 */
			if ((n = privnum(name)) < 0){
				/*
			 	 * It wasn't a "known" privilege, so print
			 	 * a message.
				 */
				(void) pfmt(stderr, MM_ERROR, UNPRIV, name);
				exit(1);
			}
			++had_name;
			if (n == P_ALLPRIVS) {
				if (washere[type]++)
					continue;
				/*
				 * This is the pseudo-privilege "allprivs"
				 * so set up prvmask[type] appropriately.
				 */
				for (i = 0; i < NPRIVS; ++i) {
					if (addpriv(i, type) == FAILURE) {
						/*
						 * "addpriv" failed because this
						 * priv existed in more than one						 * vector.
						 */
						(void) pfmt(stderr, MM_ERROR, BADPRV,
								privname(pbp, i));
						exit(1);
					}
					tmask[i] = 1;
				}
				if (nproc == NPRIVS) {	/* process had all privileges */
					(void) strcat(tmp, "allprivs");
					didsome = 1;
					break;		/* exit from "while" */
				}
				else {
					/*
					 * Only set what's in "procmask" and
					 * clean up "prvmask[type]".
					 */
					for (i = 0; i < NPRIVS; ++i) {
						if (procmask[i]) {
							if (comma) {
								comma = 0;
								(void) strcat(tmp, ",");
							}
							(void) strcat(tmp, privname(pbp, i));
							tmask[i] = comma = 1;
							didsome = 1;
						}
						else {
							tmask[i] = prvmask[type][i] = 0;
						}
					}
				}
			}
			else {
				if (prvmask[type][n]) {	/* name already set */
					continue;
				}
				if (addpriv(n, type) == FAILURE) {
					/*
					 * "addpriv" failed because this priv
					 * existed in more than one vector.
					 */
					(void) pfmt(stderr, MM_ERROR, BADPRV, name);
					exit(1);
				}
				if (procmask[n]) {	/* priv is in process max */
					if (comma) {
						comma = 0;
						(void) strcat(tmp, ",");
					}
					(void) strcat(tmp, name);
					tmask[n] = comma = 1;
					didsome = 1;
				}
				else {		/* priv was'nt in process max */
					tmask[n] = prvmask[type][n] = 0;
				}
			}
		}
	}	/* end of "while" loop */

	/*
	 * Now do the actual setting of the buffers for the call to filepriv.
	 */
	if (didsome) {
		for (i = 0; i < NPRIVS; ++i) {
			if (tmask[i]) {
				fprivp[j++] = pm_prid(type, i);
			}
		}

		/* adjust the count for "filepriv" */
	
		*fcnt = j;
	
		if (setflg[type] > 1) {
			(void) strcat(privargp[type], ",");
		}
		else {
			(void) strcat(privargp[type], "%");
			(void) strcat(privargp[type], setname);
			(void) strcat(privargp[type], ",");
		}

		(void) strcat(privargp[type], tmp);

	}	/* end of "didsome" */
	else if (!had_name) {
		(void) pfmt(stderr, MM_ERROR, ARGREQ, type);
		usage(BADSYN, MM_ACTION);
	}
}


/*
 * Procedure:	update
 *
 * Notes:	This routine updates the privilege data file.  The update could
 *		either be in the form of adding or removing a particular entry.
 *		If this is a new entry, the data is taken from the global
 *		variable nent which is populated by the ck_file()
 *		routine.
 */
static	int
update(action)
	register int	action;
{
	register int	written = 0,
			pdf_stat = 0,
			pdf_error = 0,
			end_of_file = 0,
			new_file_stat = 0;
	struct	stat	buf,
			nfsb,
			pdfs;
	struct	pdf	*pdf;
	extern	int	errno,
			badent;
	char		newbuf[FP_BSIZ];
	FILE		*tpfp;

	if (stat(nent.pf_filep, &nfsb) < 0) {
		/*
		 * The stat() call failed. The information returned from
		 * this call is required if the action is SET so return
		 * an error and don't do the subsequent filepriv() call
		 * since this indicates something changed between the call
		 * to the ck_file() routine and this point.
		 */
		if (action == SET) {
			return 1;
		}
	}
	else {
		/*
		 * The stat() call was successful so use this
		 * information when removing an entry from the PDF.
		 */
		new_file_stat = 1;
	}
	/*
	 * Set umask to 0464 so file will be created with correct mode.
	 */
	(void) umask(~(S_IRUSR|S_IRGRP|S_IWGRP|S_IROTH));
	/*
	 * Do unconditional unlink of temporary privilege
	 * data file.  Shouldn't exist anyway!!
	 */
	(void) unlink(PRVTMP);
	/*
	 * See if the PDF file already exists.  If not, get
	 * the required information from the directory that
	 * the PDF resides in.
	 */
	if (stat(PDF, &buf) < 0) {
		/*
	 	 * Stat the directory that the files are created in.
		 */
		if (stat(PRVDIR, &buf) < 0) {
			return 1;
		}
	}
	/*
	 * Open the temporary file for writing.
	 * Should have access.
	 */
	if ((tpfp = fopen(PRVTMP, "w")) == NULL) {
		return 1;
	}
	(void) setvbuf(tpfp, (char *)newbuf, _IOLBF, sizeof(newbuf));
	/*
	 * Copy privilege data files to temp file, replacing matching
	 * lines with new privilege data attributes.
	 */
	while (!end_of_file) {
		errno = 0;
		pdf_stat = 1;		/* assume successful stat of PDF file */
		if ((pdf = getpfent()) != NULL) {	/* read a line */
			if (stat(pdf->pf_filep, &pdfs) < 0) {
				/*
			 	 * Couldn't stat the file specified in
			 	 * the PDF so set flag indicating this
				 * and clear "errno".
				 */
				errno = pdf_stat = 0;
			}
			if (action == SET) {
				if (pdf_stat) {
					if (pdfs.st_dev == nfsb.st_dev
						&& pdfs.st_ino == nfsb.st_ino) {

						if (!strcmp(pdf->pf_filep, nent.pf_filep)) {
							if (putpfent(&nent, tpfp)) {
								clean_up(tpfp, PRVTMP);
								return 1;
							}
							written = 1;
						}
						else {
							/*
							 * The file must be hard-linked
							 * so replace everything but the
							 * name with the new information.
							 */
							pdf->pf_cksum = nent.pf_cksum;
							pdf->pf_size = nent.pf_size;
							pdf->pf_validity = nent.pf_validity;
							pdf->pf_privs = nent.pf_privs;
							if (putpfent(pdf, tpfp)) {
								clean_up(tpfp, PRVTMP);
								return 1;
							}
						}
						/*
						 * If the link count is 1 (no other
						 * links) set action to RMV to delete
						 * any duplicate lines from the file.
						 */
						if (nfsb.st_nlink == 1) {
							action = RMV;
						}
						else {
							--nfsb.st_nlink;
						}
						continue;
					}
				}
				else if (!strcmp(pdf->pf_filep, nent.pf_filep)) {
					if (nfsb.st_nlink == 1) {
						action = RMV;
					}
					else {
						--nfsb.st_nlink;
					}
					if (putpfent(&nent, tpfp)) {
						clean_up(tpfp, PRVTMP);
						return 1;
					}
					written = 1;
					continue;
				}
			}
			if (action == RMV) { 
				if (pdf_stat && new_file_stat) {
					if (pdfs.st_dev == nfsb.st_dev
						&& pdfs.st_ino == nfsb.st_ino) {
						continue;
					}
				}
				else if (!strcmp(pdf->pf_filep, nent.pf_filep)) {
					continue;
				}
			}
			/*
			 * If we got to here and this isn't the same file,
			 * write the old record.
			 */
			if (putpfent(pdf, tpfp)) {
				clean_up(tpfp, PRVTMP);
				return 1;
			}
		}
		else {	/* a NULL was returned when reading, check it out */
			if (!badent)	/* just hit EOF, that's all */
				end_of_file = 1;
			else if (badent) {	/* something amiss */
				++pdf_error;	/* set error */
				badent = 0;	/* and reset badent */
			}
		}
	}	/* end of while loop */
	/*
	 * If EOF was hit while the action is SET and the record was
	 * not written out, do it NOW!!
	 */
	if (action == SET && !written) {
		if (putpfent(&nent, tpfp)) {
			clean_up(tpfp, PRVTMP);
			return 1;
		}
	}
	/*
	 * Be nice, and close the privilege data file.
	 */
	(void) endpfent();
	/*
	 * While you're at it, also close the temporary file.
	 */
	(void) fclose(tpfp);
	/*
	 * Check if while reading the file any errors were
	 * encountered.  And if so, print out a message.
	 */
	if (pdf_error) {
		(void) pfmt(stderr, MM_ERROR, BADENT, PDF);
	}
	/*
	 * Set the level of the temporary file to whatever
	 * the level is for either the existing PDF or the
	 * directory in which this file will eventually reside.
	 */
	(void) lvlfile(PRVTMP, MAC_SET, &buf.st_level);

	/*
	 * Do a chown() on the new privilege data file.
	 */
	(void) chown(PRVTMP, buf.st_uid, buf.st_gid);
	/*
	 * Keep a copy of the old privilege data file in case
	 * the call to filepriv() fails later in the code.
	 */
	(void) link(OPDF, OPTMP);
	/*
	 * Rename the current PDF to the old PDF;
	 * rename the temporary file to the new PDF.
	 */
	if (update_pfile(PDF, OPDF, PRVTMP)) {
		/*
		 * Won't need that saved copy of the old
		 * privilege data file since the update failed.
		 */
		(void) unlink(OPTMP);
		return 1;
	}
	return 0;
}


/*
 * Procedure:	update_pfile
 *
 * Notes:	This routine performs all the necessary checks when moving
 *		the current privilege data file to the old privilege data
 *		file and renaming the temporary privilege data file to the
 *		current privilege data file.
 */
static	int 
update_pfile(pfilep, opfilep, tpfilep)
	char *pfilep;		/* privilege data file */
	char *opfilep;		/* old privilege data file */
	char *tpfilep;		/* temporary privilege data file */
{
	/*
	 * First check to see if there was an existing privilege
	 * data file.
	 */
	if (access(pfilep, 0) == 0) {
		/* if so, remove old privilege data file */
		if (access(opfilep, 0) == 0) {
			if (unlink(opfilep) < 0) {
				(void) unlink(tpfilep);
				return 1;
			}
		}
		/* rename privilege data file to old privilege data file */
		if (rename(pfilep, opfilep) < 0) {
			(void) unlink(tpfilep);
			return 1;
		}
	}
	if (access(tpfilep, 0) == 0) {
		/* rename temporary privilege data file to privilege data file */
		if (rename(tpfilep, pfilep) < 0) {
			(void) unlink(tpfilep);
			if (unlink(pfilep) < 0) {
				if (link(opfilep, pfilep) < 0) { 
					return 1;
				}
			}
			return 1;
		}
	}
	return 0;
}


/*
 * Procedure:	addpriv
 *
 * Notes:	This routine checks all privilege "type"s (except the "type"
 *		passed) to determine if the privilege is already in another
 *		privilege set.  If it is, it's an ERROR.
 *
 *		Otherwise, just set a bit in the named "type" set and return.
 */
static	int
addpriv(pos, type)
	int	pos;
	char	type;
{
	register int	i;

	for (i = 0; i < PRVMAXSETS; ++i) {
		if (i == type)
			continue;
		if (prvmask[i][pos])	/* priv exists in more than one set */
			return FAILURE;
	}

	prvmask[type][pos] = 1;

	return SUCCESS;
}


/*
 * Procedure:	get_procmax
 *
 * Notes:	This routine gets the current processes "maximum" privilege set
 *		and sets up a vector that has a 1 turned on if the privilege is
 *		on in the maximum set. It then returns the number of privileges
 *		that are set in the processes "maximum" set.
 */
static	int
get_procmax(sdefs)
	setdef_t	*sdefs;
{
	priv_t		*buff;
	register int	i, j,
			nprvs,
			cnt = 0,
			count = 0;


	/* initialize "procmask" to all 0's */

	for (i = 0; i < NPRIVS; ++i)
		procmask[i] = 0;

	nprvs = 0;
	for(i = 0; i < nsets; ++i) {
		if (sdefs[i].sd_objtype == PS_PROC_OTYPE) {
			nprvs += sdefs[i].sd_setcnt;
		}
	}

	if (nprvs) {
		buff = (priv_t *) malloc(nprvs * sizeof(priv_t));
		if (!buff) {
			/* no place to store information */
			return 0;
		}
	}
	else {
		/* no place to store information */
		return 0;
	}

	count = procpriv(GETPRV, buff, nprvs);

	for (i = 0; i < count; ++i) {
		for (j = 0; j < NPRIVS; ++j) {
			if (pm_max(j) == buff[i]) {
				procmask[j] = 1;
				++cnt;
				break;	/* inner "for" loop */
			}
		}
	}
	return cnt;
}


/*
 * Procedure:	printprivs
 *
 * Notes:	This routine is called by "filepriv" to print the privileges
 *		associated with the named file ("filep").  The output spec-
 *		ification depends on whether or not more than one file was
 *		specified, and whether or not the file has privilege.
 */
static	void
printprivs(filep, tot, pvec, cnt)
	char	*filep;
	int	tot;
	priv_t	pvec[];
	int	cnt;
{
	register int	i, j, k,
			legend = 0, printed = 0, n = 0;
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
					if ((sd->sd_mask | (j)) == pvec[k]) {
						++n;
						if (!legend) {
							if (tot)
								(void) printf("%s: ", filep);
							(void) printf("%s\t", sd->sd_name);
							legend = 1;
						}
						if (printed) {
							printed = 0;
							(void) strcat(tbp, ",");
						}
						(void) strcat(tbp, privname(pbp, j));
						printed = 1;
					}
				}
			}
			if (legend) {
				if (n == sd->sd_setcnt) { 
					(void) printf("allprivs");
				}
				else {
					(void) printf("%s", tbp);
				}
				(void) printf("\n");
			}
			n = printed = legend = 0;
			tbuf[0] = '\0';
		}
	}
}


/*
 * Procedure:	clean_up
 *
 * Notes:	closes the temporary PDF file and removes it if
 *		the write to the temporary file failed for any
 *		reason.
 */
static	void
clean_up(tfp, tfname)
	FILE	*tfp;
	char	*tfname;
{
	(void) fclose(tfp);
	(void) unlink(tfname);
	(void) endpfent();
	return;
}


/*
 * Procedure:	getval
 *
 * Notes:	This function places into a specified destination characters
 *		which are delimited by either a "," or 0.  It obtains the
 *		characters from a line of characters.
 */
static	char	*
getval(sourcep, destp)
	register char	*sourcep;
	register char	*destp;
{
	while (*sourcep != ',' && *sourcep != '\0')
		*destp++ = *sourcep++;
	*destp = 0;
	return sourcep;
}


/*
 * Procedure:	clean_exit
 *
 * Notes:	cleans up all temporary and lock files if the
 *		command catches any signal that returned SIG_ERR
 *		when attempting to set the signal to SIG_IGN.
 */
static	void
clean_exit()
{
	static		struct	rlimit	rl;
	register	int	i;

	(void) endpfent();
	restore_pfile();
	(void) ulckprvf();

	if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
		/*
		 * Start at file descriptor 3 since we don't
		 * want to close stdin, stdout, or stderr!
		 */
		for (i = 3; i < rl.rlim_cur; ++i) {
			(void) close(i);
		}
	}
	(void) unlink(PRVTMP);
	return;
}


/*
 * Procedure:	restore_pfile
 *
 * Notes:	restores the privilege data files (privs and oprivs) to
 *		their original contents if filepriv() failed while modifying
 *		privileges.
 */
static	void
restore_pfile()
{
	if (access(OPTMP, 0) == 0) {
		(void) rename(OPDF, PDF);
		(void) rename(OPTMP, OPDF);
	}
	return;
}


/*
 * Procedure:	ck_file
 *
 * Notes:	checks the attributes of the named file to determine
 *		if the filepriv command can access the file and obtain
 *		necessary information.  If successful, it populates the
 *		data for a new entry in the global variable nent.
 */
static	int
ck_file(act, filep)
	int	act;
	char	*filep;
{
	char		*newfp,
			*res_pathp = &res_path[0];
	struct	stat	nfsb;
	extern	int	errno;

	/*
	 * First try to resolve the filename specified by
	 * calling the realpath() routine.  Then get
	 * information concerning the specified file via
	 * the stat() system call.
	 *
	 * NOTE:  There is a slight implementation problem
	 *	  problem here.  If the privilege used to
	 *	  set file privileges was P_SETSPRIV, the
	 *	  command technically does not require READ
	 *	  privilege since the system call ignores any
	 *	  access checks.  However, not having any READ
	 * 	  access privilege would cause the realpath()
	 *	  routine and the stat() system call to fail.
	 */
	if ((newfp = realpath(filep, res_pathp)) == NULL) {
		errno = 0;
		newfp = filep;
	}
	if (stat(newfp, &nfsb) < 0) {
		if (act == SET) {
			return 1;
		}
		errno = 0;
	}
	if (act == SET) {
		/*
		 * If this isn't a regular file then
		 * privileges can't be set on it.
		 */
		if ((nfsb.st_mode & S_IFMT) != S_IFREG) {
			return 1;
		}
		if (gen_cksum) {
			/*
			 * Calculate a checksum for the file being adding
			 * to the PDF.
			 */
			if ((nent.pf_cksum = getcksum(newfp)) == FAILURE) {
				return 1;
			}
		}
		else {
			/*
			 * Don't use the cksum field when writing out
			 * this entry.
			 */
			nent.pf_cksum = -1;
		}
		/*
		 * Set up structure for the file just "stat"ed for
		 * use later when writing out to the PDF.
		 */
		nent.pf_size = nfsb.st_size;
		nent.pf_validity = nfsb.st_ctime;
		nent.pf_privs = prvs4file;
	}
	/*
	 * Set the pf_filep element since this is used in both
	 * the SET and RMV cases in the update() routine.
	 */
	nent.pf_filep = newfp;

	return 0;
}
