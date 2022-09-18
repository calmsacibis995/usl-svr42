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

#ident	"@(#)ufs.cmds:ufs/labelit/labelit.c	1.6.9.8"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fs.d/ufs/labelit/labelit.c,v 1.1 91/02/28 17:28:59 ccs Exp $"

/*
 * Label a file system volume.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mntent.h>
#include <sys/vnode.h>
#include <sys/acl.h>
#include <fcntl.h>
#include <sys/fs/sfs_inode.h>
#include <sys/sysmacros.h>
#include <sys/fs/sfs_fs.h>
#include <archives.h>
#include <sys/stat.h>

union sbtag {
	char		dummy[SBSIZE];
	struct fs	sblk;
} sb_un, altsb_un;

#define sblock sb_un.sblk
#define altsblock altsb_un.sblk

#define IFTAPE(s)       (equal(s, "/dev/rmt", 8) || equal(s, "rmt", 3))
#define TAPENAMES "'/dev/rmt'"

struct volcopy_label	Tape_hdr;

extern int	optind;
extern char	*optarg;

int	status;
int	blk;

static char * getfsnamep();
static char time_buf[80];
#define DATE_FMT	"%a %b %e %H:%M:%S %Y\n"

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	opt, i;
	int	nflag = 0;
	char	*special = NULL;
	char	*fsname = NULL;
	char	*volume = NULL;
	char	*p;

	while ((opt = getopt (argc, argv, "?no:")) != EOF) {
		switch (opt) {

		case 'o':		/* specific options (none defined yet) */
			break;
		case 'n':
			nflag++;
			break;

		case '?':
			usage();
		}
	}
	if (optind > (argc - 1)) {
		usage ();
	}
	argc -= optind;
	argv = &argv[optind];
	special = argv[0];
	if (argc > 0) {
		fsname = argv[1];
		if (strlen(fsname) > 6) {
			(void) fprintf (stderr, "labelit: fsname must be less than 7 characters\n");
			usage();
		}
	}
	if (argc > 1) {
		volume = argv[2];
		if (strlen(volume) > 6) {
			(void) fprintf (stderr, "labelit: volume must be less than 7 characters\n");
			usage();
		}
	}
	if (nflag) {
		if (!IFTAPE(special)) {
			fprintf(stderr, "labelit: `-n' option for tape only\n");
			fprintf(stderr, "\t'%s' is not a valid tape name\n", special);
			fprintf(stderr, "\tValid tape names begin with %s\n", TAPENAMES);
			usage();
			exit(33);
		}
		if (!fsname || !volume) {
			fprintf(stderr, "labelit: `-n' option requires fsname and volume\n");
			usage();
			exit(33);
		}
	}
        if (IFTAPE(special)) {
                int     fso;

                if ((fso = open(special, O_RDONLY)) < 0) {
                        (void) fprintf (stderr, "labelit: ");
                        perror ("open");
                        exit (31+1);
                }

		if (!nflag) {
			if(read(fso, &Tape_hdr, sizeof(Tape_hdr)) != sizeof(Tape_hdr)) {
				fprintf(stderr, "labelit: cannot read label\n");
				exit(31+1);
			}
			printf("%s floppy volume: %.6s, reel %d of %d reels\n",
    			Tape_hdr.v_magic, Tape_hdr.v_volume, Tape_hdr.v_reel, Tape_hdr.v_reels);
			cftime(time_buf, DATE_FMT, &Tape_hdr.v_time);
			printf("Written: %s", time_buf);
			if((argc==2 && Tape_hdr.v_reel>1) || equal(Tape_hdr.v_magic,"Finc",4))
				exit(0);
			if (read(fso, &sblock, BBSIZE) != BBSIZE) {
				fprintf(stderr, "labelit: cannot read boot block\n");
				exit(31+1);
			}
			if (read(fso, &sblock, SBSIZE) != SBSIZE) {
				fprintf(stderr, "labelit: cannot read superblock\n");
				exit(31+1);
				}
			cftime(time_buf, DATE_FMT, &sblock.fs_time);
			printf("Date last modified: %s", time_buf);
			p = getfsnamep(&sblock);
			fprintf (stdout, "fsname: %.6s\n", p);
			fprintf (stdout, "volume: %.6s\n", p+6);
			if(argc == 1){
				close(fso);
				exit(0);
			}
      	     	}
		else /* nflag */
                	printf("Skipping label check!\n");
               	printf("Labeling tape - any contents will be destroyed!!\n");
                printf("NEW fsname = %.6s, NEW volname = %.6s -- DEL if wrong!!\n",
		    fsname,volume);
                sleep(10);
		memset(&Tape_hdr, '\0', sizeof(Tape_hdr));
		memset(&sb_un, '\0', SBSIZE);
                strcpy(Tape_hdr.v_magic, "Volcopy");
                sprintf(Tape_hdr.v_volume, "%.6s", volume);
		close(fso);
                if ((fso = open(special, O_WRONLY)) < 0) {
                        (void) fprintf (stderr, "labelit: ");
                        perror ("open");
                        exit (31+1);
                }
                if (write(fso, &Tape_hdr, sizeof(Tape_hdr)) != sizeof(Tape_hdr)) {
                        fprintf(stderr, "labelit: cannot write label\n");
                        exit(31+1);
                }
		if (write(fso, &sblock, BBSIZE) != BBSIZE) {
       			fprintf(stderr, "labelit: cannot write bootblock\n");
       			exit(31+1);
		}
		p = getfsnamep(&sblock);
		for (i = 0; i < 12; i++)
			p[i] = '\0';		
		for (i = 0; (i < 6) && (fsname[i]); i++, p++)
			*p = fsname[i];
		if (i < 6) 
			p = p + (6 - i);
		for (i = 0; (i < 6) && (volume[i]); i++, p++)
			*p = volume[i];
                if (write(fso, &sblock, SBSIZE) != SBSIZE) {
                        fprintf(stderr, "labelit: cannot write superblock\n");
                        exit(31+1);
                }
                close(fso);
        } else 
                label(special, fsname, volume);
        exit(0);
}

usage ()
{

	(void) fprintf (stderr, "ufs usage: labelit [-F ufs] [generic options] [-n] special [fsname volume]\n");
	exit (31+1);
}

label (special, fsname, volume)
	char		*special;
	char		*fsname;
	char		*volume;
{
	int	f;
	int	i;
	char	*p, *savep;
	int	offset;
	struct	stat statarea;

	if (fsname == NULL) {
		f = open(special, O_RDONLY);
	} else {
		f = open(special, O_RDWR);
	}
	if (f < 0) {
		(void) fprintf (stderr, "labelit: ");
		perror ("open");
		exit (31+1);
	}

	if(fstat(f, &statarea) < 0) {
		(void) fprintf(stderr, "labelit: ");
		perror("fstat");
		exit(32);
	}
	if ((statarea.st_mode & S_IFMT) != S_IFBLK &&
	    (statarea.st_mode & S_IFMT) != S_IFCHR) {
		(void) fprintf(stderr, "labelit: ");
		(void) fprintf(stderr, "%s is not special device\n", special);
		exit(32);
	}
	if (statarea.st_flags & _S_ISMOUNTED) {
		(void) fprintf(stderr, "labelit: ");
		(void) fprintf(stderr, "%s is mounted\n", special);
		exit(32);
	}

	if (lseek(f, SBLOCK * DEV_BSIZE, 0) < 0) {
		(void) fprintf (stderr, "labelit: ");
		perror ("lseek");
		exit (31+1);
	}
	if (read(f, &sblock, SBSIZE) != SBSIZE) {
		(void) fprintf (stderr, "labelit: ");
		perror ("read");
		exit (31+1);
	}
	if (sblock.fs_magic != UFS_MAGIC) {
		(void) fprintf (stderr, "labelit: ");
		(void) fprintf (stderr, "bad super block magic number\n");
		exit (31+1);
	}
	p = getfsnamep(&sblock);
	savep = p;
	if (fsname != NULL) {
		for (i = 0; i < 12; i++)
			p[i] = '\0';		
		for (i = 0; (i < 6) && (fsname[i]); i++, p++)
			*p = fsname[i];
	}
	if (volume != NULL) {
		if (i < 6) 
			p = p + (6 - i);
		for (i = 0; (i < 6) && (volume[i]); i++, p++)
			*p = volume[i];
	}
	if (fsname != NULL) {
		if (lseek(f, SBLOCK * DEV_BSIZE, 0) < 0) {
			(void) fprintf (stderr, "labelit: ");
			perror ("lseek");
			exit (31+1);
		}
		if (write(f, &sblock, SBSIZE) != SBSIZE) {
			(void) fprintf (stderr, "labelit: ");
			perror ("write");
			exit (31+1);
		}
		for (i = 0; i < sblock.fs_ncg; i++) {
		offset = cgsblock(&sblock, i) * sblock.fs_fsize;
		if (lseek(f, offset, 0) < 0) {
			(void) fprintf (stderr, "labelit: ");
			perror ("lseek");
			exit (31+1);
		}
		if (read(f, &altsblock, SBSIZE) != SBSIZE) {
			(void) fprintf (stderr, "labelit: ");
			perror ("read");
			exit (31+1);
		}
		if (altsblock.fs_magic != UFS_MAGIC) {
			(void) fprintf (stderr, "labelit: ");
			(void) fprintf (stderr, "bad alternate super block(%i) magic number\n", i);
			exit (31+1);
		}
		bcopy((char *)&(sblock.fs_rotbl[blk]),
			(char *)&(altsblock.fs_rotbl[blk]), 12);
		
		if (lseek(f, offset, 0) < 0) {
			(void) fprintf (stderr, "labelit: ");
			perror ("lseek");
			exit (31+1);
		}
		if (write(f, &altsblock, SBSIZE) != SBSIZE) {
			(void) fprintf (stderr, "labelit: ");
			perror ("write");
			exit (31+1);
		}
		}
	}
	fprintf (stdout, "fsname: %.6s\n", savep);
	fprintf (stdout, "volume: %.6s\n", savep+6);
}

equal(s1,s2,ct)
        char *s1, *s2;
        int ct;
{
        return  !strncmp(s1, s2, ct);
}

/* Determine where in fs_rotbl the fsname should be placed */

char *
getfsnamep(sblockp)
struct fs *sblockp;
{
	int i;

	if (sblockp->fs_nspf == 0) {
		return((char *)&(sblockp->fs_rotbl[0]));
	}
	blk = sblockp->fs_spc * sblockp->fs_cpc/sblockp->fs_nspf;
	for (i = 0; i < blk; i += sblockp->fs_frag)
		/* void */;
	i -= sblockp->fs_frag;
	blk = i / sblockp->fs_frag;
	return((char *)&(sblockp->fs_rotbl[blk]));
}
