/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)sfs.cmds:sfs/fsck/main.c	1.4.4.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/sfs/fsck/main.c,v 1.1 91/02/28 17:26:53 ccs Exp $"

/***************************************************************************
 * Command: sfs/fsck
 * Inheritable Privileges: P_DACREAD,P_DACWRITE,P_DEV,P_COMPAT,P_MACREAD,
 *													P_MACWRITE
 *       Fixed Privileges: None
 *
 ***************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <priv.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/mntent.h>
#include <sys/fs/sfs_fs.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define _KERNEL
#include <sys/fs/sfs_fsdir.h>
#undef _KERNEL
#include <sys/mnttab.h>
#include <sys/signal.h>
#include <string.h>
#include "fsck.h"
#include <sys/vfstab.h>
#include <mac.h>

int	mflag = 0;		/* sanity check only */
char	hotroot;

extern int	optind;
extern char	*optarg;

int mnt_passno = 1;

int	exitstat;	/* exit status (set to 8 if 'No' response) */
char	*rawname(), *unrawname(), *blockcheck(), *hasvfsopt();
int	catch(), catchquit(), voidquit();
int	returntosingle;
int	rootfs = 0;		/* am I checking the root? */

char *subopts [] = {
#define PREEN		0
	"p",
#define BLOCK		1
	"b",
#define DEBUG		2
	"d",
#define READ_ONLY	3
	"r",
#define	ONLY_WRITES	4
	"w",
	NULL
};

/*
 * Following are the comments on the changed code for the SFS version
 * of fsck.  It is assumed that the UFS version is understood.  If
 * time permits, all this code needs to be completely documented.
 *
 * 1. ACL Storage
 *      The odd inodes are used to store security information.  NACLI
 *      entries can be stored in the secure inode.  If there are more
 *      ACL entries, they have to be stored in extended ACL blocks.
 *      If the rest does not fit in a logical block, another extended
 *      block is allocated, etc.  If the rest fits within a logical
 *      block, a block rounded up to a fragment is allocated.
 *      An ACL block will contain the file inode, number of entries in
 *      the block, and backward and forward links to other ACL blocks.
 *      Fsck uses this knowledge to traverse ACL blocks in pass1,
 *      pass1b, and pass4.
 *
 * 2. Security Functions
 *      The UFS version of fsck makes use of routine ckinode to
 *      perform specific pass checks on direct and indirect blocks of an
 *      inode.  The SFS version has expanded this notion to perform
 *      security pass checks on an inode.  Security pass checks exist
 *      for pass1, pass1b, and pass4 only:
 *              - in pass1, all valid inodes are checked for ACL validity.
 *              - in pass1b, additional ACL duplicate blocks are checked.
 *              - in pass4, ACL blocks may have to be removed from the
 *                inode block map or duplicate list.
 *
 * 3. Security Inode State Map
 *      A security inode state map is used to perform security checks
 *      on specific inodes only.  The following states are known:
 *              SEC_BADACL      - ACL is determined to be invalid
 *              SEC_DUPACL      - ACL and file reference same data block
 *              SEC_INOVIS      - inode previously checked (useful for
 *                                pass 2)
 *
 * 4. Function Additions
 *      The inodesc structure is expanded to include field id_secfunc
 *      to hold the security function.  Wherever id_func is set,
 *      id_secfunc is set also.  Since security functions exist for
 *      pass1, pass1b, and pass4 only, id_secfunc is set to 0 on other
 *      cases.  Note that pass2sechk is not a security pass function
 *      assigned to this field.
 *
 * 5. Least Privilege Additions
 *	The commad may inherit the privileges listed in the header comment.
 *	The MACREAD privilege is needed only when the the MAC level of
 *	the process is not initialized. Otherwise the MACREAD privilege
 *	is cleared using procprivl() system call.
 */

/*
 * Procedure:     main
 *
 * Restrictions:
 *               getopt: none
 *               printf: none
 *               fprintf: none
 *               fopen: P_MACREAD, only if the MAC level of the process
 *			is initialized.
 *               getvfsent: none
 *               perror: none
 *               fclose: none
*/

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int pid, passno, anygtr, sumstatus;
	char	*name;
	int	c;
	char	*suboptions,	*value;
	int	suboption;
	level_t level;

	sync();

	/*
	 * When the root file system is checked in /sbin/bcheckrc
	 * the MAC level of the process hasn't been initialized, so
	 * we need the MACREAD privilege to read VFSTAB,MNTTAB and 
	 * /etc/passwd files. 
	 *
	 * If the MAC level of the processes is initialized, the 
	 * process should be able to read these files without the 
	 * MACREAD privilege. In this case the MACREAD privilege
	 * is restricted.
	 */

	if ((lvlproc(MAC_GET,&level) == 0) && (level != 0))
		procprivl(CLRPRV,MACREAD_W,0);

	while ((c = getopt (argc, argv, "mnNo:VyY")) != EOF) {
		switch (c) {

		case 'm':
			mflag++;
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'o':
			/*
			 * sfs specific options.
			 */
			suboptions = optarg;
			while (*suboptions != '\0') {
				switch ((suboption = getsubopt(&suboptions, subopts, &value))) {
		
				case PREEN:
					preen++;
					break;
		
				case BLOCK:
					if (value == NULL) {
						usage ();
					} else {
						bflag = atoi(value);
					}
					printf("Alternate super block location: %d\n",
					    bflag);
					break;
		
				case DEBUG:
					debug++;
					break;
		
				case READ_ONLY:
					break;
		
				case ONLY_WRITES:	/* check only writable filesystems */
					wflag++;
					break;
		
				default:
					usage();
				}
			}
			break;

		case 'V':
			{
				int	opt_count;
				char	*opt_text;

				(void) fprintf (stdout, "fsck -F sfs ");
				for (opt_count = 1; opt_count < argc ; opt_count++) {
					opt_text = argv[opt_count];
					if (opt_text)
						(void) fprintf (stdout, " %s ", opt_text);
				}
				(void) fprintf (stdout, "\n");
			}
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		case '?':
			usage();
		}
	}
	argc -= optind;
	argv = &argv[optind];
	rflag++; /* check raw devices */
	if (signal(SIGINT, SIG_IGN) != (int)SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	if (argc) {
		while (argc-- > 0) {
			if (wflag && !writable(*argv)) {
				(void) fprintf (stderr, "not writeable '%s'\n", *argv);
				argv++;
			} else {
				if (strcmp (*argv, "/") == 0)
					rootfs = 1;
				checkfilesys(*argv);
				rootfs = 0;
				argv++;
			}
		}
		exit(exitstat);
	}
	sumstatus = 0;
	passno = 1;
	do {
		FILE *vfstab;
		struct vfstab vfsbuf;
		char	*raw;

		anygtr = 0;



		if ((vfstab = fopen(VFSTAB, "r")) == NULL) 
			errexit("Can't open checklist file: %s\n", VFSTAB);

		while ((getvfsent(vfstab, &vfsbuf)) == 0) {
			if (strcmp(vfsbuf.vfs_fstype, MNTTYPE_SFS)) {
				continue;
			}
			if (wflag && hasvfsopt(&vfsbuf, MNTOPT_RO)) {
				continue;
			}
/*			bcopy (&mnt, mntdup(mnt), sizeof(mnt)); */
			if (numbers(vfsbuf.vfs_fsckpass))
				mnt_passno = atoi(vfsbuf.vfs_fsckpass);
printf("mnt_pass = %d\n", mnt_passno);
			if (preen == 0 ||
			    passno == 1 && mnt_passno == passno) {
				if ((preen == 1) && rflag) {
					if (strcmp (vfsbuf.vfs_mountp, "/") == 0)
						rootfs = 1;
					raw =
					    rawname(unrawname(vfsbuf.vfs_special));
					checkfilesys(raw);
					rootfs = 0;
				} else {
					name = blockcheck(vfsbuf.vfs_special);
					if (name != NULL)
						checkfilesys(name);
					else if (preen)
						exit(36);
				}
			} else if (mnt_passno > passno)
				anygtr = 1;
			else if (mnt_passno == passno) {
				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(36);
				}
				if (pid == 0) {
					(void)signal(SIGQUIT, voidquit);
					if ((preen == 1) && rflag) {
						raw =
						    rawname(unrawname(vfsbuf.vfs_special));
						checkfilesys(raw);
						exit(exitstat);
					} else {
						name = blockcheck(vfsbuf.vfs_special);
						if (name == NULL)
							exit(36);
						checkfilesys(name);
						exit(exitstat);
					}
				}
			}
		}
		fclose(vfstab);
		if (preen) {
			int status;
			while (wait(&status) != -1)
				sumstatus |= WHIBYTE(status);
		}
		passno++;
	} while (anygtr);
	if (sumstatus)
		exit(36);
	if (returntosingle)
		exit(31+2);
	exit(exitstat);
}


/*
 * Procedure:     checkfilesys
 *
 * Restrictions:
 *                printf: none
*/

checkfilesys(filesys)
	char *filesys;
{
	daddr_t n_ffree, n_bfree;
	struct dups *dp;
	struct zlncnt *zlnp;

	mountedfs = 0;
	if ((devname = setup(filesys)) == 0) {
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
		exit(36);
	}
	if (mflag)
		check_sanity (filesys);	/* this never returns */
	/*
	 * 1: scan inodes tallying blocks used
	 */
	if (preen == 0) {
		if (mountedfs)
			printf("** Currently Mounted on %s\n", sblock.fs_fsmnt);
		else
			printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
		if (mflag) {
			printf("** Phase 1 - Sanity Check only\n");
			return;
		} else
			printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

	/*
	 * 2: traverse directories from root to mark all connected directories
	 */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
	pwarn("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
	if (preen)
	    printf("\n");
	pwarn("(%d frags, %d blocks, %.1f%% fragmentation)\n",
	    n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);
	/*
         *      when computing correct number of files,
         *      exclude alternate inodes
         */
        if (debug &&
         (n_files -= imax / NIPFILE - SFSROOTINO - sblock.fs_cstotal.cs_nifree))
		printf("%d files missing\n", n_files);
	if (debug) {
		n_blks += sblock.fs_ncg *
			(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
		n_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
		n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
		if (n_blks -= fmax - (n_ffree + sblock.fs_frag * n_bfree))
			printf("%d blocks missing\n", n_blks);
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %d,", dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %d,", zlnp->zlncnt);
			printf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = (struct dups *)0;
	if (dfile.mod)
		fixstate = 1;
	else
		fixstate = 0;
	if (sblock.fs_state + (long)sblock.fs_time != FSOKAY) {
		if (nflag) {
			printf("%s FILE SYSTEM STATE NOT SET TO OKAY\n",
				devname);
			fixstate = 0;
		} else	{
			printf("%s FILE SYSTEM STATE SET TO OKAY\n",
				devname);
			fixstate = 1;
		}
		
	}	
	if (fixstate) {
		(void)time(&sblock.fs_time);
		sblock.fs_state = FSOKAY - (long)sblock.fs_time;
		sbdirty();
	}
	ckfini();
	free(blockmap);
	free(statemap);
	free((char *)lncntp);
	if (!dfile.mod)
		return;
	if (!preen) {
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
	}
	if (mountedfs || rootfs) {
		exit(40);
	}
}


/*
 * Procedure:     blockcheck
 *
 * Restrictions:
 *                stat(2): none
 *                printf: none
*/

char *
blockcheck(name)
	char *name;
{
	struct stat stslash, stblock, stchar;
	char *raw;
	int looped = 0;

	rootfs = 0;
	if (stat("/", &stslash) < 0){
		printf("Can't stat root\n");
		return (0);
	}
retry:
	if (stat(name, &stblock) < 0){
		printf("Can't stat %s\n", name);
		return (0);
	}
	if (stblock.st_mode & S_IFBLK) {
		raw = rawname(name);
		/* Allow access overrides */
		if (stat(raw, &stchar) < 0){
			printf("Can't stat %s\n", raw);
			return (0);
		}
		if (stchar.st_mode & S_IFCHR) {
			if (stslash.st_dev == stblock.st_rdev) {
				rootfs++;
				if (!rflag)
					raw = unrawname(name);
			}
			return (raw);
		} else {
			printf("%s is not a character device\n", raw);
			return (0);
		}
	} else if (stblock.st_mode & S_IFCHR) {
		if (looped) {
			printf("Can't make sense out of name %s\n", name);
			return (0);
		}
		name = unrawname(name);
		looped++;
		goto retry;
	}
	printf("Can't make sense out of name %s\n", name);
	return (0);
}



/*
 * Procedure:     check_sanity
 *
 * Restrictions:
 *                stat(2): none
 *                fprintf:  none
 *                printf: none
 * Notes:
 *        exit 0 - file system is unmounted and okay
 *        exit 32 - file system is unmounted and needs checking
 *        exit 33 - file system is mounted
 *        exit 34 - cannot stat device
 *
 *	  root is mounted readonly at boot time
 *	  exit 101 - indicates it is okay, remount it.
 */

check_sanity(filename)
char	*filename;
{
	struct stat stbd, stbr;

	if (stat(filename, &stbd) < 0) {
		fprintf(stderr, "sfs fsck: sanity check failed : cannot stat %s\n", filename);
		exit(34);
	}
	stat("/", &stbr);
	if (stbr.st_dev == stbd.st_rdev) {	/* root file system */
		if ((sblock.fs_state + (long)sblock.fs_time) != FSOKAY) {
			fprintf(stderr, "sfs fsck: sanity check: root file system needs checking\n");
			exit(32);
		} else {
			fprintf(stderr, "sfs fsck: sanity check: root file system okay\n");
			exit(101);
		}
	}
	if (stbd.st_flags & _S_ISMOUNTED) {
		fprintf(stderr, "sfs fsck: sanity check: %s already mounted\n", filename);
		exit(33);
	}

	if ((sblock.fs_state + (long)sblock.fs_time) != FSOKAY) {
printf("time=%x, state=%x\n", sblock.fs_time, sblock.fs_state);
		fprintf(stderr, "sfs fsck: sanity check: %s needs checking\n", filename);
		exit(32);
	}
	fprintf(stderr, "sfs fsck: sanity check: %s okay\n", filename);
	exit(0);
}


/*
 * Procedure:     unrawname
 *
 * Restrictions:
 *                stat(2): none
 *                sprintf: none
*/

char *
unrawname(cp)
	char *cp;
{
	char *dp = rindex(cp, '/');
	struct stat stb;
	static char rawbuf[MAXPATHLEN];

	if (dp == 0)
		return (cp);

	if (stat(cp, &stb) < 0) 
		return (cp);

	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);

	/* for device naming convention /dev/dsk/c1d0s2 */
	sprintf(rawbuf, "/dev/dsk/%s", dp+1);

	if (stat(rawbuf, &stb) == 0) 
		return(rawbuf);
	
	/* for device naming convention /dev/save */
	if (*(dp+1) != 'r')
		return (cp);

	(void)strcpy(dp+1, dp+2);
	return (cp);
}


/*
 * Procedure:     rawname
 *
 * Restrictions:
 *                sprintf: none
 *                stat(2): none
*/

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[MAXPATHLEN];
	char *dp = rindex(cp, '/');
	struct stat statb;

	if (dp == 0)
		return (0);

	/* for device naming convention /dev/dsk/c1d0s2 */
	sprintf(rawbuf, "/dev/rdsk/%s", dp+1);

	if (stat(rawbuf, &statb) == 0) 
		return (rawbuf);

	/* for device naming convention /dev/save */
	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);
	return (rawbuf);
}


/*
 * Procedure:     mntopt
 *
 * Restrictions:  none
*/

static char *
mntopt(p)
        char **p;
{
        char *cp = *p;
        char *retstr;

        while (*cp && isspace(*cp))
                cp++;
        retstr = cp;
        while (*cp && *cp != ',')
                cp++;
        if (*cp) {
                *cp = '\0';
                cp++;
        }
        *p = cp;
        return (retstr);
}


/*
 * Procedure:     hasvfsopt
 *
 * Restrictions:  none
*/

char *
hasvfsopt(vfs, opt)
        register struct vfstab *vfs;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
        strcpy(tmpopts, vfs->vfs_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + vfs->vfs_mntopts);
        }
        return (NULL);
}


/*
 * Procedure:     hasmntopt
 *
 * Restrictions:  none
*/

char *
hasmntopt(mnt, opt)
        register struct mnttab *mnt;
        register char *opt;
{
        char *f, *opts;
        static char *tmpopts;

        if (tmpopts == 0) {
                tmpopts = (char *)calloc(256, sizeof (char));
                if (tmpopts == 0)
                        return (0);
        }
        strcpy(tmpopts, mnt->mnt_mntopts);
        opts = tmpopts;
        f = mntopt(&opts);
        for (; *f; f = mntopt(&opts)) {
                if (strncmp(opt, f, strlen(opt)) == 0)
                        return (f - tmpopts + mnt->mnt_mntopts);
        }
        return (NULL);
}


/*
 * Procedure:     usage
 *
 * Restrictions:
 *                fprintf: none
*/

usage ()
{
	(void) fprintf (stderr,
	    "sfs usage: fsck [-F sfs] [generic options] [-o p,b=#,w] [special ....]\n");
	exit (31+1);
}
