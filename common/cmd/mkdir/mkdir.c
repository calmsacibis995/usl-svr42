/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)mkdir:mkdir.c	1.7.11.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/mkdir/mkdir.c,v 1.1 91/02/28 18:33:15 ccs Exp $"

/***************************************************************************
 * Command: mkdir
 * Inheritable Privileges: P_MULTIDIR,P_SETFLEVEL,P_MACREAD,P_DACREAD,
 *			   P_MACWRITE,P_DACWRITE,P_FSYSRANGE
 *       Fixed Privileges: P_MACUPGRADE
 * Notes: mkdir makes a directory.
 * 	If -m is used with a valid mode, directories will be created in
 *	that mode.  Otherwise, the default mode will be 777 possibly
 *	altered by the process's file mode creation mask.
 *
 * 	If -p is used, make the directory as well as its non-existing
 *	parent directories. 
 *
 * 	-M indicates that the target directory is to be a Multi-Level
 *	Directory (valid only with the Enhanced Security Package).
 *
 * 	-l indicates that the target directory is to be created at a given
 * 	security level (valid only with the Enhanced Security Package).
 *
 *	P_MULTIDIR is needed for creating a Multi-Level Directory
 *	P_SETFLEVEL or P_MACUPGRADE is needed for setting the level on
 *	the directory.
 *	The other privileges may be needed to pass any access checks on
 *	directory path.
 ***************************************************************************/

#include	<signal.h>
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<string.h>
#include	<mac.h>
#include        <deflt.h>
#include	<priv.h>
#include	<stdlib.h>
#include	<pwd.h>
#include	<unistd.h>
#include	<locale.h>
#include	<pfmt.h>

extern int opterr, optind;
extern char *optarg;

int Mflag, lflag;
level_t level; /*internal format of the level specified in -l option */

static const char
	badmkdir[] = ":328:Cannot create directory \"%s\": %s\n",
	badMkdir[] = ":808:cannot create multilevel directories\n",
	badfilesys[] = ":809:File system for file %s does not support per-file labels\n";

#define DEFFILE "mac"	/* /etc/default file containing the default LTDB lvl */
#define LTDB_DEFAULT 2  /* default LID for non-readable LTDB */

/*
 * Procedure: main
 *
 * Restrictions: lvlin: P_MACREAD	mkdirp: <none>
 *		 mkdir(2): <none>	mkmld(2): <none>
 */
main(argc, argv)
int argc;
char *argv[];
{
	int 	pflag, errflg;
	int 	c, local_errno, saverr = 0;
	mode_t	mode;
	char 	*p, *m, *l;
	char 	*endptr;
	char *d;	/* the (path)name of the directory to be created */
	int err;
	void cleandir();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:mkdir");
	errno = 0; /* temporary soln., reset errno in case setlocale fails*/

	mode = S_IRWXU | S_IRWXG | S_IRWXO;
	pflag = errflg = Mflag = lflag = 0;
	local_errno = 0;

	while ((c=getopt(argc, argv, "m:pMl:")) != EOF) {
		switch (c) {
		case 'm':
			for(m = optarg; *m != '\0'; m++) {
				if(*m < '0' || *m > '7') {
					pfmt(stderr, MM_ERROR, ":14:Invalid mode\n");
					exit(2);
				}
			}
			umask(0);
			mode = (mode_t) strtol(optarg, (char **)NULL, 8);
			break;
		case 'l':
			lflag++;
			l = optarg;
			level = (level_t) check_lvl (l);
			break;
		case 'M':
			Mflag++;
			break;
		case 'p':
			pflag++;
			break;
		case '?':
			errflg++;
			break;
		}
	}

	argc -= optind;
	if(argc < 1 || errflg) {
		if (!errflg)
			pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		pfmt(stderr, MM_ACTION, 
			":810:Usage: mkdir [-m mode] [-p] [-M] [-l level] dirname ...\n");
		exit(2);
	}
	argv = &argv[optind];

        while(argc--) {
		d = *argv++;
		/* Skip extra slashes at the end of path */
		while ((endptr=strrchr(d, '/')) != NULL){
			p=endptr;
			p++;
			if (*p == '\0') {
				if (endptr != d) 
					*endptr='\0';
				else
					break;
			} else
				break;
		}


		/* When -p is set, invokes mkdirp library routine.
		 * Although successfully invoked, mkdirp sets errno to ENOENT 
		 * if one of the directory in the pathname does not exist,
		 * thus creates a confusion on success/failure status 
		 * possibly checked by the calling routine or shell. 
		 * Therefore, errno is reset only when
		 * mkdirp has executed successfully, otherwise save
		 * in local_errno.
		 * If -M is set, we call the local (to this source file)
		 * routine mkmldp() instead of mkdirp(). This routine
		 * performs the same function as mkdirp(), but creates
		 * an MLD as the last component. mkmldp() does not
		 * have the same problem with errno as mkdirp() has.
		 */ 
		if (pflag) { 
			if (Mflag) {
				if ((err = mkmldp(d,mode)) < 0)
					if (errno == ENOSYS)
						pfmt(stderr, MM_ERROR, badfilesys, d);
					else
						pfmt(stderr, MM_ERROR, badMkdir);
			} 
			else {
				err = mkdirp(d,mode);
				if (err < 0) {
					pfmt(stderr, MM_ERROR, badmkdir, d,
						strerror(errno));
					local_errno = errno;
				} else errno = 0;
			}

		/* No -p. Make only one directory 
		 */

		} else {
			/*
			 * If we are to make an MLD, call mkmld(2) to make
			 * the MLD. Otherwise, call mkdir(2) to make a
			 * regular directory.
			 */
			if (Mflag) 
				err = mkmld(d, mode);
			else
				err = mkdir(d, mode);
			if (err) {
				if (Mflag) {
					if (errno == ENOSYS)
						pfmt(stderr, MM_ERROR, badfilesys, d);
					else
						pfmt(stderr, MM_ERROR, badMkdir);
				} else
					pfmt(stderr, MM_ERROR, badmkdir, d,
						strerror(errno));
			}
		}

		/*
		 * If a level was specified, change the level.
		 * If the level change failed, remove the directory.
		 */
		if (!err && lflag) {
			if ((err = chglevel(d, &level)))
				cleandir(d);
		}
		if (err) {
			saverr = 2;
			errno = 0;
		}

	} /* end while */

	if (saverr)
		errno = saverr;

	/* When pflag is set, the errno is saved in local_errno */
	if (errno == 0 && local_errno)
		errno = local_errno;

        exit(errno ? 2: 0);
	/* NOTREACHED */
} /* main() */

/*
 * Procedure:     check_lvl
 *
 * Restrictions:
 *		lvlin: none
 *		lvlproc: none
 *		defopen: none
 *	        defclose: none
 *		defread: none
 *
 *
 * Inputs: Level of directory to be created.
 * Returns: 0 on success. On failure the appropriate error message is
 *          displayed and the process is terminated.
 *
 *
 * Notes: Attempt to read the LTDB at the current process level, if the
 *        call to lvlin fails, get the level of the LTDB (via defread),
 *        change the process level to that of the LTDB and try again.
 *   
 *        If the lvlin or lvlproc calls fail we assume MAC is not installed
 *        and exit with a nonzero status. 
 *
 */
check_lvl (dir_lvl)
char *dir_lvl;
{
	FILE    *def_fp;
	static char *ltdb_deflvl;
	level_t test_lid, ltdb_lid, curr_lvl;


/*
 *	If lvlin is successful the LTDB is readable at the current process
 *	level, the directory level is valid and no other processing is 
 *      required, return the LID corresponding to the level supplied as
 *      input with the -l option.
*/

	if (lvlin (dir_lvl, &test_lid) == 0)  
		return (test_lid);
	else
		if (errno == EINVAL) { /* Invalid level w/-l option */
			pfmt(stderr, MM_ERROR, ":811:specified level is not defined\n");
			exit(4);
		}

/*
 *	If the LTDB was unreadable:
 *	- Get the LTDB's level (via defread)
 *	- Get the current level of the process
 *	- Set p_setplevel privilege
 *	- Change process level to the LTDB's level
 *	- Try lvlin again, read of LTDB should succeed since process level
 *        is equal to level of the LTDB
 *
 *	Failure to obtain the process level or read the LTDB is a fatal
 *      error.
 */


	if ((def_fp = defopen(DEFFILE)) != NULL) {
		if (ltdb_deflvl = defread(def_fp, "LTDB"))
			ltdb_lid = (level_t) atol (ltdb_deflvl);
		(void) defclose(NULL);
	}
	
	if ((def_fp == NULL) || !(ltdb_deflvl))
		ltdb_lid = (level_t) LTDB_DEFAULT;


	if (ltdb_lid <= 0) {
		pfmt(stderr, MM_ERROR, ":812:LTDB is inaccessible\n");
		exit(5);
	}

	if (lvlproc (MAC_GET, &curr_lvl) == -1 && errno == ENOPKG) {
		pfmt(stderr, MM_ERROR, ":794:System service not installed\n");
		exit(3);
	}

	(void) procprivl (SETPRV, SETPLEVEL_W, 0);
	(void) lvlproc (MAC_SET, &ltdb_lid);

	if (lvlin (dir_lvl, &test_lid) == -1)  {
		if (errno == EINVAL) {
			pfmt(stderr, MM_ERROR, ":811:specified level is not defined\n");
			exit(4);
		}
		else {
			pfmt(stderr, MM_ERROR, ":812:LTDB is inaccessible\n");
			exit(5);
		}
	}

	(void) lvlproc (MAC_SET, &curr_lvl);
	(void) procprivl (CLRPRV, SETPLEVEL_W, 0);

	return (test_lid);

}
/*
 * Procedure:     chglevel
 *
 * Restrictions: lvlfile(2): <none>
 */
	
int
chglevel(dirp, levelp)
char *dirp;
level_t *levelp;
{
	int changed_mode = 0;	/*flag to tell if we changed MLD mode*/
	int curr_mode;		/* holds return from MLD_QUERY */
	int err;

	if (Mflag) {
		if ((curr_mode=mldmode(MLD_QUERY)) == MLD_VIRT){
			if (mldmode(MLD_REAL) < 0) {
				pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				      strerror(errno));
				exit(4);
			}
			changed_mode = 1;
		} else if (curr_mode < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	} /* if Mflag */
	err = lvlfile(dirp, MAC_SET, levelp);
	if (err) {
		if (errno == ENOSYS)
			pfmt(stderr, MM_ERROR, badfilesys, dirp);
		else
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
	}
	if (changed_mode) {
		if (mldmode(MLD_VIRT) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	}

	return err;

} /* chglevel() */

/*
 * Procedure: cleandir() - removes directory.  
 *
 * Notes: If the directory is a multi-level directory, then it should
 *	  be removed in real mode. 
 *
 * Restrictions: rmdir(2): <none>
 */
void
cleandir(d)
char *d;
{
	int changed_mode = 0;
	int curr_mode;

	if (Mflag) {    /* just created a multi-level directory */
		if ((curr_mode=mldmode(MLD_QUERY)) == MLD_VIRT){
			if (mldmode(MLD_REAL) < 0) {
				pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n",
				      strerror(errno));
				exit(4);
			}
			changed_mode = 1;
		} else if (curr_mode < 0 && errno != ENOPKG) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	} /* if Mflag */

        (void) rmdir(d);     

	if (changed_mode) {
		if (mldmode(MLD_VIRT) < 0) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(4);
		}
	}
} /* cleandir() */

/*
 * Procedure: mkmldp - creates an MLD and it's parents if the parents
 *		       do not exist yet.
 *
 * Restrictions: mkdir(2): <none>	mkmld(2): <none>
 *               access(2): <none>
 *
 * Notes: Returns -1 if fails for reasons other than non-existing parents.
 * 	  Does NOT compress pathnames with . or .. in them.
 */

static char *compress();
void free();

int
mkmldp(d, mode)
const char *d;
mode_t mode;
{
	char  *endptr, *ptr, *slash, *str, *lastcomp;

	/*
	 * Remove extra slashes from pathname.
	 */

	str=compress(d);

	/* If space couldn't be allocated for the compressed names, return. */

	if ( str == NULL )
		return(-1);

	ptr = str;

        /* Try to make the directory */
	if (mkmld(str, mode) == 0){
		free(str);
		return(0);
	}
	if (errno != ENOENT) {
		free(str);
		return(-1);
	}
	endptr = strrchr(str, '\0');
	ptr = endptr;
	slash = strrchr(str, '/');
	lastcomp = slash;

		/* Search upward for the non-existing parent */

	while (slash != NULL) {

		ptr = slash;
		*ptr = '\0';

			/* If reached an existing parent, break */

		if (access(str, 00) ==0)
			break;

			/* If non-existing parent*/

		else {
			slash = strrchr(str,'/');

				/* If under / or current directory, make it. */

			if (slash  == NULL || slash== str) {
				if (mkdir(str, mode)) {
					free(str);
					return(-1);
				}
				break;
			}
		}
	}
	/* Create directories starting from upmost non-existing parent*/

	while ((ptr = strchr(str, '\0')) != lastcomp){
		*ptr = '/';
		if (mkdir(str, mode)) {
			free(str);
			return(-1);
		}
	}
	*lastcomp = '/';

	/* Create the final component as an MLD */

	if (mkmld(str, mode)) {
		free(str);
		return -1;
	}
	
	free(str);
	errno = 0;
	return 0;
} /* mkmldp() */

/* 
 * Procedure: compress
 */

static char *
compress(str)
char *str;
{

	char *tmp;
	char *front;

	tmp=(char *)malloc(strlen(str)+1);
	if ( tmp == NULL )
		return(NULL);
	front = tmp;
	while ( *str != '\0' ) {
		if ( *str == '/' ) {
			*tmp++ = *str++;
			while ( *str == '/' )
				str++;
		}
		*tmp++ = *str++;
	}
	*tmp = '\0';
	return(front);
} /* compress() */
