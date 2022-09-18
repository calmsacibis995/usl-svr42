/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)mkmsgs:mkmsgs.c	1.4.11.2"
#ident  "$Header: mkmsgs.c 1.3 91/08/13 $"

/***************************************************************************
 * Command: mkmsgs
 * Inheritable Privileges: P_MACWRITE,P_SETFLEVEL
 *       Fixed Privileges: None
 * Notes: create message files for use by gettxt. 
 *	  Privileges are required if the file will be created in the public
 *	  directory, /usr/lib/<locale>/LC_MESSAGES.
 ***************************************************************************/


/* 
 * Create message files in a specific format.
 * the gettxt message retrieval function must know the format of
 * the data file created by this utility.
 *
 * 	FORMAT OF MESSAGE FILES

	 __________________________
	|  Number of messages      |
	 --------------------------
	|  offset to the 1st mesg  |
	 --------------------------
	|  offset to the 2nd mesg  |
	 --------------------------
	|  offset to the 3rd mesg  |
	 --------------------------
	|          .		   |
	|	   .	           |
	|	   .		   |
	 --------------------------
	|  offset to the nth mesg  |
	 --------------------------
	|    message #1
	 --------------------------
	|    message #2
	 --------------------------
	|    message #3
	 --------------------------
		   .
		   .
		   .
	 --------------------------
	|    message #n
	 --------------------------
*
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <priv.h>
#include <mac.h>
#include <errno.h>
#include <unistd.h>

/* 
 * Definitions
 */

#define	LINESZ	2048	/* max line in input base */
#define STDERR  2
#define P_locale	"/usr/lib/locale/"	/* locale info directory */
#define L_locale	sizeof(P_locale)
#define MESSAGES	"/LC_MESSAGES/"		/* messages category */

/*
 * external functions
 */

extern void	*malloc();
extern char	*tempnam();
extern char	*strccpy();
extern int	access();
extern void	exit();
extern void	free();
extern int	getopt();
extern int	mkdir();

/*
 * external variables
 */

extern  int	errno;		/* System error code */
extern  char	*sys_errlist[]; /* Error messages */
extern  int	sys_nerr;	/* Number of sys_errlist entries */
extern  int	optind;
extern  char	*optarg;

/*
 * internal functions
 */

static  char	*syserr();	/* Returns description of error */
static  void	usage();        /* Displays valid invocations */
static  int	mymkdir();      /* Creates sub-directories */
static  void	clean();	/* removes work file */

/*
 * static variables
 */

static	char	*cmdname;	/* Last qualifier of arg0 */
static	char    *workp;		/* name of the work file */
static	uid_t	uid;		/* owner of data files */
static	gid_t	gid;		/* group of data files */
static	level_t	level;		/* MAC level of data files */
static  mac_installed;		/* flag to see if MAC is installed */
static	level_t mkmsgs_lid;	/* level identifier of mkmsgs */

/*
 * Procedure:     main
 *
 * Restrictions:
                 getpwnam:	none
                 getgrnam:	none
                 fprintf:	none
                 lvlin:		none
                 getopt:	none
                 access(2):	none
                 fopen:		none
                 tempnam:	none
                 fgets:		none
                 fwrite:	none
                 fclose:	none
                 stat(2):	none
                 fread:		none
                 unlink(2):	none
                 chown(2):	none
                 lvlfile(2):	none
*/


main(argc, argv)
int argc;
char *argv[];
{
	int c;				/* contains option letter */
	char	*ifilep;		/* input file name */
	char	*ofilep;		/* output file name */
	char	*localep; 		/* locale name */
	char	*localedirp;    	/* full-path name of parent directory
				 	 * of the output file */
	char	*outfilep;		/* full-path name of output file */
	FILE *fp_inp; 			/* input file FILE pointer */
	FILE *fp_outp;			/* output file FILE pointer */
	char *bufinp, *bufworkp;	/* pointers to input and work areas */
	int  *bufoutp;			/* pointer to the output area */
	char *msgp;			/* pointer to the a message */
	int num_msgs;			/* number of messages in input file */
	int iflag;			/* -i option was specified */
	int oflag;			/* -o option was slecified */
	int nitems;			/* number of bytes to write */
	char *pathoutp;			/* full-path name of output file */
	struct stat buf;		/* buffer to stat the work file */
	unsigned size;			/* used for argument to malloc */
	struct passwd *pwent;		/* passwd entry for file owner */
	struct group *grpent;		/* group entry for file group */
	int i;				

	/* Initializations */

	localep = (char *)NULL;
	num_msgs = 0;
	iflag   = 0;
	oflag   = 0;

	/* Get name of command */

	if (cmdname = strrchr(argv[0], '/'))
		++cmdname;
	else
		cmdname = argv[0];

	/* Get owner/group information for file ownership */

	if ((pwent = getpwnam("root")) == (struct passwd *)NULL ||
	    (grpent = getgrnam("sys")) == (struct group *)NULL) {
		fprintf(stderr, "%s: can't get owner/group information\n",
			cmdname);
		exit(1);
	} else {
		uid = pwent->pw_uid;
		gid = grpent->gr_gid;
	}

	if (lvlproc(MAC_GET, &mkmsgs_lid) == -1) {
		if (errno == ENOPKG)
			mac_installed = 0;
		else {
			(void)fprintf(stderr, 
			   "%s: cannot perform lvlproc() call.\n",
				cmdname);
			(void)exit(1);
		}
	}
	else
		mac_installed = 1;


	/* Get MAC level for file */

	if (mac_installed) {
		if (lvlin("SYS_PUBLIC", &level) == -1) {
			(void)fprintf(stderr, 
				"%s: can't get level internal format\n",
			cmdname);
			(void)exit(1);
		}
	else

		/*
	         * 1 corresponds to SYS_PUBLIC
	         */
		level = 1;
	}

	/* Check for invalid number of arguments */

	if (argc < 3 || argc > 6)
		usage();

	/* Get command line options */

	while ((c = getopt(argc, argv, "oi:")) != EOF) {
		switch (c) {
		case 'o':
			oflag++;
			break;
		case 'i':
			iflag++;
			localep = optarg;
			break;
		case '?':
			usage();
			break;
		}
	}

	/* Initialize pointers to input and output file names */

	ifilep = argv[optind];
	ofilep = argv[optind + 1];

	/* check for invalid invocations */

	if (iflag && oflag && argc != 6)
		usage();
	if (iflag && ! oflag && argc != 5)
		usage();
	if (! iflag && oflag && argc != 4)
		usage();
	if (! iflag && ! oflag && argc != 3)
		usage();

	/* Construct a  full-path to the output file */

	if (localep) {
		size = L_locale + strlen(localep) +
			 sizeof(MESSAGES) + strlen(ofilep);
		if ((pathoutp = malloc(2 * (size + 1))) == NULL) {
			(void)fprintf(stderr, "%s: malloc error (size = %d)\n",
					cmdname, size);
			exit(1);
		}
		localedirp = pathoutp + size + 1;
		(void)strcpy(pathoutp, P_locale);
		(void)strcpy(&pathoutp[L_locale - 1], localep);
		(void)strcat(pathoutp, MESSAGES);
		(void)strcpy(localedirp, pathoutp);
		(void)strcat(pathoutp, ofilep);
	}

	/* Check for overwrite error conditions */

	if (! oflag) {
		if (iflag) {
			if (access(pathoutp, F_OK) == 0) {
				(void)fprintf(stderr, "%s: Message file \"%s\" already exists;\ndid not overwrite it\n", cmdname, pathoutp);
				if (localep)
					free(pathoutp);
				exit(1);
			}
		}
		else  
			if (access(ofilep, F_OK) == 0) {
				(void)fprintf(stderr, "%s: Message file \"%s\" already exists;\ndid not overwrite it\n", cmdname, ofilep);
				if (localep)
					free(pathoutp);
				exit(1);
			}
	}

	(void)umask(0022);
	
	/* Open input file */
	if ((fp_inp = fopen(ifilep, "r")) == NULL) {
		(void)fprintf(stderr, "%s: %s: %s\n",
			cmdname, ifilep, syserr());
		exit(1);
	}

	/* Allocate buffer for input and work areas */

	if ((bufinp = malloc(2 * LINESZ)) == NULL) {
		(void)fprintf(stderr, "%s: malloc error (size = %d)\n",
					cmdname, 2 * LINESZ);
		exit(1);
	}
	bufworkp = bufinp + LINESZ;

	if (sigset(SIGINT, SIG_IGN) == SIG_DFL)
		(void)sigset(SIGINT, clean);

	/* Open work file */

	workp = tempnam(".", "xx");
	if ((fp_outp = fopen(workp, "a+")) == NULL) {
		(void)fprintf(stderr, "%s: %s: %s\n", cmdname, workp, syserr());
		if (localep)
			free(pathoutp);
		free(bufinp);
		exit(1);
	}

	/* Search for C-escape sequences in input file and 
	 * replace them by the appropriate characters.
	 * The modified lines are copied to the work area
	 * and written to the work file */

	for(;;) {
		if (!fgets(bufinp, LINESZ, fp_inp)) {
			if (!feof(fp_inp)) {
				(void)fprintf(stderr,"%s: %s: %s\n",
					cmdname, ifilep, syserr());
				free(bufinp);
				if (localep)
					free(pathoutp);
				exit(1);
			}
			break;
		}
		if(*(bufinp+strlen(bufinp)-1)  != '\n') {
			(void)fprintf(stderr, "%s: %s: data base file: error on line %d\n", cmdname, ifilep, num_msgs);
			free(bufinp);
			exit(1);
		}
		*(bufinp + strlen(bufinp) -1) = (char)0; /* delete newline */
		num_msgs++;
		(void)strccpy(bufworkp, bufinp);
		nitems = strlen(bufworkp) + 1;
		if (fwrite(bufworkp, sizeof(*bufworkp), nitems, fp_outp) != nitems) {
			(void)fprintf(stderr, "%s: %s: %s\n",
				cmdname, workp, syserr());
			exit(1);
		}
	}
	free(bufinp);
	(void)fclose(fp_outp);

	/* Open and stat the work file */

	if ((fp_outp = fopen(workp, "r")) == NULL) {
		(void)fprintf(stderr, "%s: %s: %s\n", cmdname, workp, syserr());
		exit(1);
	}
	if ((stat(workp, &buf)) != 0) {
		(void)fprintf(stderr, "%s: %s: %s\n", cmdname, workp, syserr());
	}

	/* Find the size of the output message file 
	 * and copy the control information and the messages
	 * to the output file */

	size = sizeof(int) + num_msgs * sizeof(int) + buf.st_size;

	if ( (bufoutp = (int *)malloc((uint)size)) == NULL ) {
		(void)fprintf(stderr, "%s: malloc error (size = %d)\n",
				cmdname, size);
		exit(1);
	}
	bufinp = (char *)bufoutp;
	if ( (fread(bufinp + sizeof(int) + num_msgs * sizeof(int), 
	    sizeof(*bufinp), buf.st_size, fp_outp)) != buf.st_size ) {
		free(bufinp);
		(void) fprintf(stderr, 
			"%s: %s: %s\n", cmdname, workp, syserr());
	}
	(void) fclose(fp_outp);
	unlink(workp);
	free(workp);
	msgp = bufinp + sizeof(int) + num_msgs * sizeof(int);
	*bufoutp = num_msgs;
	*(bufoutp + 1) =
		(bufinp + sizeof(int) + num_msgs * sizeof(int)) - bufinp;

	for(i = 2; i <= num_msgs; i++) {
		*(bufoutp + i) = (msgp + strlen(msgp) + 1) - bufinp;
		msgp = msgp + strlen(msgp) + 1;
	}

	if (iflag) { 
		outfilep = pathoutp;
		if (mymkdir(localedirp) == 0) {
			free(bufinp);
			if (localep)
				free(pathoutp);
			exit(1);
		}
		/* Only need privilege for files under locale directory */
		/* Need P_MACWRITE to be able to write message file */
	}
	else
		outfilep = ofilep;


	if ((fp_outp = fopen(outfilep, "w")) == NULL) {
		(void)fprintf(stderr, "%s: %s: %s\n",
				cmdname, outfilep, syserr());
		free(bufinp);
		if (localep)
			free(pathoutp);
		exit(1);
	}

	if (fwrite((char *)bufinp, sizeof(*bufinp), size, fp_outp) != size) {
		(void)fprintf(stderr, "%s: %s: %s\n",
				cmdname, ofilep, syserr());
		free(bufinp);
		if (localep)
			free(pathoutp);
		exit(1);
	}

	(void)fclose(fp_outp);

	free(bufinp);
	if (localep)
		free(pathoutp);

	/*
	 * If creating a public message data file,
	 * make sure it has right owner and MAC level.
	 * Need privilege (SETFLEVEL) to change MAC levels.
	 */
	if (iflag) {
		if(chown(outfilep, uid, gid) == -1) {
			(void)fprintf(stderr, "%s: %s: %s\n", 
					cmdname, outfilep, syserr());
			exit(1);
		}	
		if (lvlfile(outfilep, MAC_SET, &level) == -1) {
			if (errno == ENOSYS) {
				(void)fprintf(stderr, 
				 "file (%s): WARNING: the file system type does not support labeling\n", outfilep);
			}
			else {
				(void)fprintf(stderr, "%s: %s: %s\n", 
					cmdname, outfilep, syserr());
				(void)exit(1);
			}
		}
	}
	exit(0);
}

/*
 * Procedure:     syserr
 *
 * Restrictions:  none
 */

/*
 * syserr()
 *
 * Return a pointer to a system error message.
 */

static char *
syserr()
{
	return (errno <= 0 ? "No error (?)"
	    : errno < sys_nerr ? sys_errlist[errno]
	    : "Unknown error (!)");
}

/*
 * Procedure:     usage
 *
 * Restrictions:
 *                fprintf:	none
 */

static void
usage()
{
	(void)fprintf(stderr, "Usage: %s [-o] inputstrings outputmsgs\n",
				cmdname);
	(void)fprintf(stderr, "       %s [-o] [-i locale] inputstrings outputmsgs\n", cmdname);
	exit(1);
}

/*
 * Procedure:     mymkdir
 *
 * Restrictions:
 *               access(2):	none
 *               mkdir(2):	none
 *               chown(2):	none
 *               lvlfile(2):	none
 *               fprintf:	none
 */

static int
mymkdir(localdir)
char	*localdir;
{
	char	*dirp;
	char	*s1 = localdir;
	char	*path;

	if ((path = malloc(strlen(localdir)+1)) == NULL)
		return(0);
	*path = '\0';
	while( (dirp = strtok(s1, "/")) != NULL ) {
		s1 = (char *)NULL;
		(void)strcat(path, "/");
		(void)strcat(path, dirp);
		if (access(path, X_OK) == 0)
			continue;

		/* Need P_MACWRITE to create directories and P_SETLEVEL
		 * to change their MAC levels.
		 */
		if ( ( access(path, F_OK) != 0) &&
		     ( mkdir(path, 0777) == -1 ||
		      chown(path, uid, gid) == -1 ) ) {
			(void)fprintf(stderr, "%s: %s: %s\n", 
					cmdname, path, syserr());
			(void)free(path);
			return(0);
		}
		/*
		 * change the level to SYS_PUBLIC 
		 */
		if (lvlfile(path, MAC_SET, &level) == -1) {
			if (errno == ENOSYS) {
				(void)fprintf(stderr, 
				 "directory (%s): WARNING: the file system type does not support labeling\n", path);
			}
			else {
				(void)fprintf(stderr, "%s: %s: %s\n", 
					cmdname, path, syserr());
				(void)free(path);
				return(0);
			}
		}
	}
	free(path);
	return(1);
}

/*
 * Procedure:     clean
 *
 * Restrictions:
 *               unlink(2):	none
*/

static void
clean()
{
	(void)sigset(SIGINT, SIG_IGN);
	if (workp)
		(void)unlink(workp);
	exit(1);
}
