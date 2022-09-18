/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved. 					*/

#ident	"@(#)rm:rm.c	1.21.4.2"
#ident "$Header: rm.c 1.2 91/08/13 $"

/***************************************************************************
 * Command: rm
 *
 * Inheritable Privileges: P_MACREAD,P_MACWRITE,P_DACREAD,
 *			   P_DACWRITE,P_COMPAT,P_FILESYS
 *       Fixed Privileges: None
 *
 * Notes: rm [-fir] file ...
 *
 *
 ***************************************************************************/



#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<dirent.h>
#include	<limits.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<priv.h>

#define ARGCNT		5		/* Number of arguments */
#define CHILD		0	 
#define	DIRECTORY	((buffer.st_mode&S_IFMT) == S_IFDIR)
#define	SYMLINK		((buffer.st_mode&S_IFMT) == S_IFLNK)
#define	FAIL		-1
#define MAXFORK		100		/* Maximum number of forking attempts */
#define MAXFILES        OPEN_MAX  - 2	/* Maximum number of open files */
#define	MAXLEN		DIRBUF-24  	/* Name length (1024) is limited */
				        /* stat(2).  DIRBUF (1048) is defined */
				        /* in dirent.h as the max path length */
#define	NAMESIZE	MAXNAMLEN + 1	/* "/" + (file name size) */
#define TRUE		1
#define	WRITE		02

/* Structure used to record failed deletes during recursion after
  running out of file descriptors and starting to close + reopen 
  directories. It's yet another linked list. */
/* It would be sufficient just to keep name of LAST failure in current 
   directory, except some (hypothetical) file system might decide 
   to have readdir() return directories in another order on a second pass,
   especially if some entries have been deleted (imagine compaction) */
struct	elt {
	struct	elt	*next;
	char		*name;
};

static	int	errcode;
static	int interactive, recursive, silent; /* flags for command line options */

void	exit();
void	free();
void	*malloc();
unsigned sleep();
int	getopt();
int	lstat();
int	access();
int	isatty();
int	rmdir();
int	unlink();

static	struct elt *in_list();			
static	int	rm();
static	int	undir();
static	int	yes();
static	int	mypath();

static const char badopen[] = ":4:Cannot open %s: %s\n";
static const char askdir[] = ":1129:Directory %s. Remove (y/n)? ";
static const char nomem[] = ":312:Out of memory: %s\n";

/*
 * Procedure:     main
 *
 * Restrictions:
 *                setlocale: none
 *                getopt: none
 *                pfmt: none
 */

main(argc, argv)
	int	argc;
	char	*argv[];
{
	extern int	optind;
	int	errflg = 0;
	int	c;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:rm");

	while ((c = getopt(argc, argv, "fri")) != EOF)
		switch (c) {
		case 'f':
			silent = TRUE;
			break;
		case 'i':
			interactive = TRUE;
			break;
		case 'r':
			recursive = TRUE;
			break;
		case '?':
			errflg = 1;
			break;
		}

	/* 
	 * For BSD compatibility allow '-' to delimit the end 
	 * of options.
	 */
	if (optind < argc && !strcmp(argv[optind], "-")) 
		optind++;

	argc -= optind;
	argv = &argv[optind];
	
	if (argc < 1 || errflg) {
		if (!errflg)
			(void)pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
		(void)pfmt(stderr, MM_ACTION,
			":433:Usage: rm [-fir] file ...\n");
		exit(2);
	}

	while (argc-- > 0) {
		(void) rm (*argv);
		argv++;
	}

	exit(errcode ? 2 : 0);
	/* NOTREACHED */
}


/*
 * Procedure:     rm
 *
 * Restrictions:
 *                lstat(2): none
 *                pfmt: none
 *                strerror: none
 *                access(2): none
 *                isatty: none
 *                unlink(2): none
 * Returns 1 if file is deleted, else 0.
 */

static int
rm(path)
	char	*path;
{
	struct stat buffer;

	/* 
	 * Check file to see if it exists.
	 */

	if (lstat(path, &buffer) == FAIL) {
		if (!silent) {
			pfmt(stderr, MM_ERROR, ":5:Cannot access %s: %s\n",
				path, strerror(errno));
			++errcode;
		}
		return 0;
	}
	
	/*
	 * If it's a directory, remove its contents.
	 */
	if (DIRECTORY) {
		return undir(path, buffer.st_dev, buffer.st_ino);
	}
	
	/*
	 * If interactive, ask for acknowledgement.
	 */
	if (interactive) {
		pfmt(stderr, MM_NOSTD, ":1130:Remove file %s (y/n)? ", path);
		if (!yes())
			return 0;
	} else if (!silent) {
		/* 
		 * If not silent, and stdin is a terminal, and there's
		 * no write access, and the file isn't a symblic link,
		 * ask for permission.
		 */
		if (access(path, WRITE) == FAIL
		   && isatty(fileno(stdin)) && !SYMLINK) {
			pfmt(stderr, MM_NOSTD, ":1131:%s: %o mode. Remove (y/n)? ",
				path,buffer.st_mode & 0777);
			/*
			 * If permission isn't given, skip the file.
			 */
			if (!yes())
				return 0;
		}
	}

	/*
	 * If the unlink fails, inform the user if interactive or not silent.
	 */
	if (unlink(path) == FAIL){
		if (!silent || interactive)
			pfmt(stderr,MM_ERROR,":436:%s not removed: %s.\n",path, strerror(errno));
		++errcode;
		return 0;
	}
	return 1;
}


/*
 * Procedure:     undir
 *
 * Restrictions:
 *                pfmt: none
 *                opendir: none
 *                strerror: none
 *                sprintf: none
 *                rmdir(2): none
 * Returns 1 if dir is deleted, else 0.
 */

static int
undir(path, dev, ino)
	char	*path;
	dev_t	dev;
	ino_t	ino;
{
	char	*newpath;
	DIR	*name;
	struct dirent *direct;
	struct	elt	*fail=NULL, *tail=NULL, *ep;
	char	*target;
	
	/*
	 * If "-r" wasn't specified, trying to remove directories
	 * is an error.
	 */
	if (!recursive) {
		pfmt(stderr, MM_ERROR, ":437:%s not removed: directory\n", path);
		++errcode;
		return 0;
	}

	/*
	 * If interactive and this file isn't in the path of the
	 * current working directory, ask for acknowledgement.
	 */
	if (interactive && !mypath(dev, ino)) {
		pfmt(stderr, MM_NOSTD, askdir, path);
		/*
		 * If the answer is no, skip the directory.
		 */
		if (!yes())
			return 0;
	}
	
	/*
	 * Open the directory for reading.
	 */
	if ((name = opendir(path)) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, path, strerror(errno));
		exit(2);
	}
	
	/*
	 * Read every directory entry.
	 */
	while ((direct = readdir(name)) != NULL) {
		/*
		 * Ignore "." and ".." entries.
 		 */
		if(!strcmp(direct->d_name, ".")
		  || !strcmp(direct->d_name, ".."))
			continue;

		/* Skip already failed entries, if any */
		if (in_list(fail, direct->d_name))
			continue;
			
		/*
		 * Try to remove the file.
		 */
		newpath = (char *)malloc((strlen(path) + NAMESIZE + 1));

		if (newpath == NULL) {
			pfmt(stderr, MM_ERROR, nomem, strerror(errno));
			exit(1);
		}
		
		/*
		 * Limit the pathname length so that a clear error message
		 * can be provided.
		 */
		if (strlen(path) + strlen(direct->d_name)+2 >= (size_t)MAXLEN) {
			pfmt(stderr, MM_ERROR, ":438:Path too long (%d/%d).\n",
			  strlen(path)+strlen(direct->d_name)+1, MAXLEN);
			exit(2);
		}

		sprintf(newpath, "%s/%s", path, direct->d_name);
 
		/*
		 * If a spare file descriptor is available, just call the
		 * "rm" function with the file name; otherwise close the
		 * directory and reopen it when the child is removed.
		 */
		if (name->dd_fd >= MAXFILES) {
			target = strdup(direct->d_name);
			if (!target) {
				pfmt(stderr, MM_ERROR, nomem, 
					strerror(errno));
				exit(1);
			}
			closedir(name);
			if (!rm(newpath)) {
				/* Failure means we need to remember */
				ep = malloc(sizeof(struct elt));
				if (!ep) {
					pfmt(stderr, MM_ERROR, nomem, 
						strerror(errno));
					exit(1);
				}
				if (tail)
					tail->next = ep;
				else
					fail = tail = ep;
				ep->next = NULL;
				ep->name = target;
			} else {
				free(target);
			}
			if ((name = opendir(path)) == NULL) {
				pfmt(stderr, MM_ERROR, badopen, path,
					strerror(errno));
				exit(2);
			}
		} else
			(void) rm(newpath);
 
		free(newpath);
	}

	/*
	 * Close the directory we just finished reading.
	 */
	closedir(name);

	while (fail) {
		ep = fail;
		fail = ep->next;
		free(ep->name);
		free(ep);
	}
	
	/*
	 * The contents of the directory have been removed.  If the
	 * directory itself is in the path of the current working
	 * directory, don't try to remove it.
	 * When the directory itself is the current working directory, mypath()
	 * has a return code == 2.
	 */
	switch (mypath(dev, ino)) {
	case 2:
		pfmt(stderr, MM_ERROR,
			":439:Cannot remove any directory in the path\n\tof the current working directory\n\t%s\n",path);
		++errcode;
		return 0;
	case 1:
		return 0;
	case 0:
		break;
	}
	
	/*
	 * If interactive, ask for acknowledgement.
	 */
	if (interactive) {
		pfmt(stderr, MM_NOSTD, askdir, path);
		if (!yes())
			return 0;
	}
	if (rmdir(path) == FAIL) {
		pfmt(stderr, MM_ERROR, ":440:Cannot remove directory %s: %s\n",
			path, strerror(errno));
		++errcode;
		return 0;
	}
	return 1;
}

/* Check if an element is in a list */
static	struct elt	*in_list(list, name) 
	struct	elt	*list;
	char	*name;
{
	while (list) {
		if (strcmp(list->name, name) == 0) 
			return list;
		list = list->next;
	}
	return NULL;
}

/*
 * Procedure:     yes
 *
 * Restrictions:
 *                 getchar: none
 */

static int
yes()
{
	int	i, b;

	i = b = getchar();
	while (b != '\n' && b != '\0' && b != EOF)
		b = getchar();
	return(i == 'y');
}


/*
 * Procedure:     mypath
 *
 * Restrictions:
 *               pfmt: none
 *               strerror: none
 *               lstat(2): none
 */

static int
mypath(dev, ino)
	dev_t	dev;
	ino_t	ino;
{
	struct stat buffer;
	dev_t	lastdev = (dev_t) -1;
	ino_t	lastino = (ino_t) -1;
	char	*path;
	int	i, j;

	for (i = 1; ; i++) {
		/*
		 * Starting from ".", walk toward the root, looking at
		 * each directory along the way.
		 */
		path = (char *)malloc((3 * (uint)i));

		if (path == NULL) {
			pfmt(stderr, MM_ERROR, nomem, strerror(errno));
			exit(1);
		}

		strcpy(path, ".");
		for (j = 1; j < i; j++) 
			if (j == 1)
				strcpy(path, "..");
			else
				strcat(path,"/..");

		lstat(path, &buffer);
		/*
		 * If we find a match, the directory (dev, ino) passed to mypath()
		 * is an ancestor of ours. Indicated by return 1;
		 *
		 * If (i == 1) the directory (dev, ino) passed to mypath() is our
		 * current working directory. Indicated by return 2;
		 * 
		 */
		if (buffer.st_dev == dev && buffer.st_ino == ino)
			if (i == 1)
				return 2;
			else
				return 1;
		
		/*
		 * If we reach the root without a match, the given
		 * directory is not in our path.
		 */
		if (buffer.st_dev == lastdev && buffer.st_ino == lastino) 
			return 0;
	
		/*
		 * Save the current dev and ino, and loop again to go
		 * back another level.
		 */
		lastdev = buffer.st_dev;
		lastino = buffer.st_ino;
		free(path);
	}
}
