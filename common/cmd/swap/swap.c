/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)swap:swap.c	1.10.4.6"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/swap/swap.c,v 1.1 91/02/28 20:11:23 ccs Exp $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
 *	Swap administrative interface.
 *	Used to add/delete/list swap devices. 
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<errno.h>
#include	<sys/param.h>
#include	<dirent.h>
#include	<sys/swap.h>
#include	<sys/immu.h>
#include	<sys/sysmacros.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<sys/uadmin.h>
#include	<vm/anon.h> 
#include	<fcntl.h> 
#include	<sys/ksym.h>

#define LFLAG 1
#define DFLAG 2
#define AFLAG 3
#define SFLAG 4

char *prognamep;

off_t find_size();

main(argc, argv)
int	argc;
char	**argv;
{
	extern char *optarg;
	extern int optind;
	register int c, flag = 0;
	register int ret;
	register off_t s_offset, length;
	char *pathname;

	prognamep = argv[0];
	if (argc < 2) {
		usage();
		exit(1);
	}

	while ((c = getopt(argc, argv, "lsd:a:")) != EOF) {
		char *char_p;
		switch (c) {
		case 'l': 	/* list all the swap devices */
			if (argc != 2 || flag ) {
				usage();
				exit(1);
			}
			flag |= LFLAG;
			ret = list();
			break;
		case 's':
			if (argc != 2 || flag) {
				usage();
				exit(1);
			}
			flag |= SFLAG;
			doswap();
			break;
		case 'd':
			if (argc < 3 || argc > 4 || flag ) {
				usage();
				exit(1);
			}
			flag |= DFLAG;
			pathname = optarg;
			if (argc > 3) {
				if ((s_offset =
				      strtol(argv[optind], &char_p, 10)) < 0 
				    || *char_p != '\0')
					exit(1);
			} else
				s_offset = 0;
			ret = delete(pathname, s_offset);
			break;
		
		case 'a':
			if (argc < 3 || argc > 5 || flag) {
				usage();
				exit(1);
			}
			flag |= AFLAG;
			pathname = optarg;
			if (argc > 4) {
				if ((s_offset =
				      strtol(argv[optind++], &char_p, 10)) < 0 
				    || *char_p != '\0')
					exit(1);
			} else
				s_offset = 0;
			if (argc > 3) {
				if ((length =
				      strtol(argv[optind], &char_p, 10)) < 1 
				    || *char_p != '\0')
					exit(1);
			} else
				length = find_size(pathname);
			ret = add(pathname, s_offset, length);
			break;
		case '?':
			usage();
			exit(1);
		}
	}
	if (!flag) {
		usage();
		exit(1);
	}
	return ret;
}


usage()
{
	(void) fprintf(stderr, "Usage:\t%s -l\n", prognamep);
	(void) fprintf(stderr, "\t%s -s\n", prognamep);
	(void) fprintf(stderr, "\t%s -d <file name> [ <low block> ]\n",
		prognamep);
	(void) fprintf(stderr,
		"\t%s -a <file name> [ [ <low block> ] <nbr of blocks> ]\n",
		prognamep);
}

#define ANONINFO "anoninfo"

#define ctok(x) ((ctob(x))>>10)

int
doswap()
{
        struct anoninfo ai;
        int nalloc;
        int kd;
	struct mioc_rksym rks;

        if ((kd = open("/dev/kmem", O_RDONLY)) == -1) {
                (void) fprintf(stderr, "swap: error opening /dev/kmem\n");
                return(-1);
        }
	rks.mirk_symname = ANONINFO;
	rks.mirk_buf = (void *) &ai;
	rks.mirk_buflen = sizeof(struct anoninfo);
        if ( ioctl(kd, MIOC_READKSYM, &rks) != 0) {
               	(void) fprintf(stderr, "swap: ioctl error: cannot get anoninfo from kernel\n");
               	return(-1);
	}

        nalloc = ctok(ai.ani_max - ai.ani_free);
        (void) printf(
           "\ntotal: %d allocated + %d reserved = %d blocks used, %d blocks available\n",
           nalloc*2, 2*(ctok(ai.ani_resv) - nalloc), 2*ctok(ai.ani_resv),
           2*ctok(ai.ani_max - ai.ani_resv));
}

int
list()
{
	register struct swaptable 	*st;
	register struct swapent	*swapent;
	register int	i;
	struct stat statbuf;
	char		*path;
	int		length, num, error=0;
	ushort 		type = 0;

retry:
	if ((num = swapctl(SC_GETNSWP, NULL)) == -1) {
		(void) fprintf(stderr,"swap: SC_GETNSWP failed\n");
		perror(prognamep);
		return(2);
	}
	if (num == 0) {
		(void) fprintf(stderr,"swap: no swap device configured:swap(SC_LIST)\n");
		return(1);
	}

	if ((st = (swaptbl_t *) malloc(num * sizeof(swapent_t) + sizeof(int))) == NULL) {
		(void) fprintf(stderr,"swap: malloc fails:swap(SC_LIST) aborted! Please try later.\n");
		perror(prognamep);
		return(2);
	}
	if ((path = (char *) malloc(num * MAXPATHLEN)) == NULL) {
		(void) fprintf(stderr,"swap: malloc fails:swap(SC_LIST) aborted! Please try later.\n");
		perror(prognamep);
		return(2);
	}
	swapent = st->swt_ent;
	for (i = 0 ; i < num ; i++, swapent++) {
		swapent->ste_path = path;
		path += MAXPATHLEN;
	}

	st->swt_n = num;
	if ((num = swapctl(SC_LIST, st)) == -1) {
		(void) fprintf(stderr,"swap: SC_LIST failed\n");
		perror (prognamep);
		return(2);
	}

        printf("%-35s dev  swaplo blocks   free\n", "path");

	swapent = st->swt_ent;
	for (i = 0;  i < num  ;  i++, swapent++) {
	    strlen (swapent->ste_path) < 35 ? 
	         printf("%-35s", swapent->ste_path) : printf("%s", swapent->ste_path);
		if (stat(swapent->ste_path, &statbuf) < 0)
			printf(" ?,? ");
		else {
			type = (statbuf.st_mode & (S_IFBLK | S_IFCHR));
			printf("%2d,%-2d",
				type ? major(statbuf.st_rdev) : major(statbuf.st_dev),
				type ? minor(statbuf.st_rdev) : minor(statbuf.st_dev));
		}
		printf(" %6d %6d %6d", 
			swapent->ste_start,
			swapent->ste_pages << DPPSHFT,
			swapent->ste_free << DPPSHFT);
		if(swapent->ste_flags & ST_INDEL)
			printf(" INDEL\n");
		else
			printf("\n");
	}
	return 0;

}

int
delete(path, offset)
char	*path;
int	offset;
{
	register swapres_t	*si;
	swapres_t		swpi;

	si = &swpi;
	si->sr_name = path;
	si->sr_start = offset;

	if (swapctl(SC_REMOVE, si) < 0){
		(void) fprintf(stderr,"swap: SC_REMOVE failed\n");
		perror (prognamep);
		return(2);
	}
	return 0;
}

int
add(path, offset, cnt)
char	*path;
int 	offset;
int 	cnt;
{
	register swapres_t	*si;
	swapres_t		swpi;

	if (path[0] != '/') {
		(void) fprintf(stderr, "swap: use full pathname for '%s'\n", path);
		return(1);
	}

	si = &swpi;
	si->sr_name = path;
	si->sr_start = offset;
	si->sr_length = cnt;

	if (swapctl(SC_ADD, si) < 0){
		(void) fprintf(stderr,"swap: SC_ADD failed\n");
		perror (prognamep);
		return(2);
	}
	return 0;
}


off_t
find_size(char *path)
{
	int		fd;
	struct stat	statb;

	if ((fd = open(path, O_RDONLY)) < 0) {
		perror(path);
		exit(1);
	}
	if (fstat(fd, &statb) < 0) {
		perror(path);
		exit(1);
	}
	close(fd);
	if (statb.st_size == 0) {
		fprintf(stderr, "%s: Can't determine size of device '%s';\n",
				prognamep, path);
		fprintf(stderr, "\tspecify size explicitly.\n");
		exit(1);
	}
	return statb.st_size / 512;
}
