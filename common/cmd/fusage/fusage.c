/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)fusage:fusage.c	1.15.14.8"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/fusage/fusage.c,v 1.1 91/02/28 17:31:36 ccs Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mnttab.h>
#include <sys/utsname.h>
#include <nserve.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/nserve.h>
#include <sys/rf_sys.h>
#include <sys/ksym.h>
#include <sys/vfs.h>
#include <errno.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define	ROOTVFS		"rootvfs"
#define KMEM		"/dev/kmem"
#define ADVTAB		"/etc/dfs/sharetab"
#define	REMOTE		"/etc/dfs/fstypes"
#define	FSTYPE_MAX	 8
#define	REMOTE_MAX	64
#define FSTYP_LEN	 4

#ifndef SZ_PATH
#define SZ_PATH		128
#endif

struct advlst {
	char resrc[SZ_RES+1];
	char dir[SZ_PATH+1];
	char fstyp[FSTYP_LEN+1];
} *advlst;

extern	int	rfsys();	/* undocumented system call */

static	int	getline();
static	int	loadadvl();
static	int	remote();
static	int	shouldprint();
static	int	isinfs();
static	int	rread();
static	long	getcount();
static	void	prdat();

#define	Fprintf	(void)fprintf
#define	Printf	(void)printf

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*cmdname;
	char 	str[SZ_RES+SZ_MACH+SZ_PATH+20];
	ulong	fs_blksize;
	FILE 	*atab;
	FILE	*mtab;
	int 	nadv	= 0;	/* number of advertised resources */
	ulong	clientsum;
	ulong	advsum;
	int	local;
        int     fswant	= 0;
	int	ii;
	int	jj;
	int	exitcode= 0;
	struct client 	*clientp;
	struct statfs	fsi;
	struct utsname	myname;
	struct mnttab 	mnttab;

	
	cmdname = argv[0];		/* get command name from argv list */
	for (ii = 0; ii < argc; ii++) {
		if (argv[ii][0] == '-') {
			Fprintf(stderr, "Usage: %s [mounted file system]\n",
			  argv[0]);
			Fprintf(stderr, "          [advertised resource]\n");
			Fprintf(stderr,
			  "          [mounted block special device]\n");
			exit(1);
		}
	}

	/*
	 * Determine RFS status.  If it is installed, begin the RFS things
	 * to do.
	 */
	if ((rfsys(RF_RUNSTATE) != -1 || errno != ENOPKG) &&
	  (atab = fopen(ADVTAB, "r")) != NULL) {
		int max_clients;

		/*
		 * we are not going to store the complete line for each entry
		 * in advtab.  Only the resource name and the path name
		 * are important here (client names are unnecessary).
		 */
		while (getline(str, sizeof(str), atab) != 0) {
			nadv++;
		}
		if ((advlst =
		  (struct advlst *)malloc(nadv * sizeof(struct advlst))) == 0) {
			Fprintf(stderr,
			  "fusage: cannot get memory for advtab\n");
			exit(1);
		}
		rewind(atab);
		/*
		 * load advlst from advtab, ignoring non-RFS entries
		 */
		ii = 0;
		while ((getline(str, sizeof(str), atab) != 0) && (ii < nadv)) {
			if (loadadvl(str, &advlst[ii]) == -1) {
				nadv--;
				continue;
			}
			ii++;
		}
		/* 
		 * find out what NSRMOUNT is and allocate memory for struct
		 * client accordingly.
		 */
		if ((max_clients = rfsys(RF_TUNEABLE, T_NSRMOUNT)) <= 0) {
			perror (cmdname);
			exit(1);
		}
		if ((clientp = (struct client *)malloc(max_clients *
		  sizeof(struct client))) == NULL ) {
			Fprintf(stderr, "%s: memory allocation failed\n",
			  cmdname);
			exit(1);
		}
	}
	(void)uname(&myname);
	Printf("\nFILE USAGE REPORT FOR %.8s\n\n", myname.nodename);
	if ((mtab = fopen(MNTTAB, "r")) == NULL) {
		Fprintf(stderr,"fusage: cannot open %s", MNTTAB);
		perror("open");
		exit(1);
	}
	
	/* 
	 * Process each entry in the mnttab.  If the entry matches a requested
	 * name, or all are being reported (default), print its data.  If the
	 * entry names a file system containing one or more advertised resources
	 * that are requested, print their data relative to this entry.
	 */
	while (getmntent(mtab, &mnttab) != -1) {
		if (mnttab.mnt_special == NULL || mnttab.mnt_mountp == NULL)
			continue;
		if (remote(mnttab.mnt_fstype))
			continue;
		if (shouldprint(argc, argv, mnttab.mnt_special,
		  mnttab.mnt_mountp)) {
			Printf("\n\t%-15s      %s\n", mnttab.mnt_special, 
			  mnttab.mnt_special);
			fswant = 1;
		} else {
			fswant = 0;
		}
		if (statfs(mnttab.mnt_mountp, &fsi, sizeof(struct statfs), 0)
		  < 0) {
			fs_blksize = 1024;  /* per file system basis */
			Printf("forced fs_blksize\n");
		} else {
			fs_blksize = fsi.f_bsize;
		}

		advsum = 0;
		for (ii = 0; ii < nadv; ii++) {
			int		n_clients;
			struct client 	*cl_p;

			if (isinfs(mnttab.mnt_mountp, advlst[ii].dir) &&
			  shouldprint(argc, argv, advlst[ii].resrc,
			  advlst[ii].dir)) {
				Printf("\n\t%15s", advlst[ii].resrc);

				/* get client list of this resource  */
				if ((n_clients = rfsys(RF_CLIENTS,
				  advlst[ii].resrc, clientp)) < 0) {
					Fprintf(stderr,
					  "%s: can't find client list: %s/n",
					  cmdname, strerror(errno));
					exit(1);
				}
				
				if (n_clients == 0) {
					Printf(" (%s) ...no clients\n",
					  advlst[ii].dir);
					continue;
				}
				Printf("      %s\n", advlst[ii].dir);
				for (jj = clientsum = 0, cl_p = clientp;
				  jj < n_clients; jj++, cl_p++) {
					prdat(cl_p->cl_node, cl_p->cl_bcount,
					  1024); /* client data in KB */
					clientsum += cl_p->cl_bcount;
				}
				prdat("Sub Total", clientsum, 1024);
				advsum += clientsum;
			}
		}
		if (fswant) {
			Printf("\n\t%15s      %s\n", "", mnttab.mnt_mountp);
			if ((local = getcount(mnttab.mnt_mountp, cmdname))
			  != -1) {
				prdat(myname.nodename,(ulong)local,fs_blksize);
			}
			if (advsum) {
				prdat("Clients", advsum, fs_blksize);
				prdat("TOTAL", (ulong)local+advsum, fs_blksize);
			}
		}
	}
	for (ii = 1; ii < argc; ii++) {
		if (argv[ii][0] != '\0') {
			exitcode = 2;
			Fprintf(stderr,"'%s' invalid\n", argv[ii]);
		}
	}
	exit(exitcode);
	/* NOTREACHED */
}

/*
 * Should the file system/resource named by dir and special be printed?
 */
int
shouldprint(argc, argv, dir, special)
	int	argc;
	char	*argv[];
	char	*dir;
	char	*special;
{
	int	found;
	int	i;

	found = 0;
	if (argc == 1) {
		return 1;	/* the default is "print everything" */
	}
	for (i = 0; i < argc; i++) {
		if (!strcmp(dir, argv[i]) || !strcmp(special, argv[i])) {
			argv[i][0] = '\0';	/* done with this arg */
			found++;		/* continue scan to find */
		}				/* duplicate requests */
	}
	return found;
}

void
prdat(s, n, bsize)
	char	*s;
	ulong	n;
	ulong	bsize;
{
	Printf("\t\t\t%15s %10lu KB\n", s, n * bsize / 1024);
}

/*
 * Is 'advdir' within mountp?
 */
int
isinfs(mountp, advdir)
	char		*mountp;
	char		*advdir;
{
	struct stat	mpstat;
	struct stat	advstat;

	(void)stat(mountp, &mpstat);
	(void)stat(advdir, &advstat);
	if (advstat.st_dev == mpstat.st_dev) {
		return 1;
	}
	return 0;
}

/*
 * Read up to len characters from the file fp and toss remaining characters
 * up to a newline.  If the last line is not terminated in a '\n', returns 0;
 */ 
int
getline(str, len, fp)
	char	*str;
	int	len;
	FILE	*fp;
{
	int	c;
	int	i = 1;
	char	*s = str; 

	for ( ; ; ) {
		switch (c = getc(fp)) {
		case EOF:
			*s = '\0';
			return 0;
		case '\n':
			*s = '\0';
			return 1;
		default:
			if (i < len) {
				*s++ = (char)c;
			}
			i++;
		}
	}
}

/*
 * Loads in one line from the ADVTAB file.
 * Returns 0 for success, -1 for failure, ie. if 
 * file system type for that line is not RFS.
 */
int
loadadvl(s, advx)
	char		*s;
	struct advlst	*advx;
{
	int		i;

	while (isspace(*s)) {
		s++;
	}

	for (i = 0; !isspace(*s) && i < SZ_MACH + SZ_PATH + 1; i++) {
		advx->dir[i] = *s++;
	}
	advx->dir[i] = '\0';

	while (isspace(*s)) {
		s++;
	}
	for (i = 0; !isspace(*s) && (i < SZ_RES); i++) {
		advx->resrc[i] = *s++;
	}
	advx->resrc[i] = '\0';
	while (isspace(*s)) {
		s++;
	}
	for (i = 0; !isspace(*s) && i < FSTYP_LEN; i++) {
		advx->fstyp[i] = *s++;
	}
	advx->fstyp[i] = '\0';;
	if (strcmp(advx->fstyp,"rfs") != 0) {
		return(-1);
	}
	return(0);
}

static long
getcount(fs_name, cmdname)
	char		*fs_name;
	char		*cmdname;
{
	struct stat	statb;
	vfs_t		vfs_buf;
	vfs_t		*next_vfs;
	vfs_t		*rvfs;
	int		memfd;
	struct mioc_rksym rks;

	if (stat(fs_name, &statb) == -1) {
		Fprintf(stderr, "%s: stat failed\n", cmdname);
		perror(fs_name);
		return -1;
	}
	
	/* open KMEM */
	if ((memfd = open(KMEM, O_RDONLY)) == -1) {
		Fprintf(stderr, "%s: open %s error\n", cmdname, KMEM);
		perror(KMEM);
		return -1;
	}

	rks.mirk_symname = ROOTVFS;
	rks.mirk_buf = (void *) &rvfs;
	rks.mirk_buflen = sizeof(struct vfs *);
	if(ioctl(memfd, MIOC_READKSYM, &rks) != 0) {
		Fprintf(stderr, "%s: failed to get rootvfs \n", cmdname);
		return -1;
	}
	next_vfs = rvfs;
	while (next_vfs) {
		if (rread(memfd, (off_t)next_vfs, (char *)&vfs_buf,
		  sizeof(struct vfs)) == -1) {
			Fprintf(stderr, "%s: cannot read next vfs \n", cmdname);
			(void)close(memfd);
			return -1;
		}
		/* check if this is the same device */
		if (vfs_buf.vfs_dev == statb.st_dev) {
			(void)close(memfd);
			return vfs_buf.vfs_bcount;
		} else {
			next_vfs = vfs_buf.vfs_next;
		}
	}

	/*
	 * not found in vfs list
	 */
	Fprintf(stderr, "%s: %s not found in kernel\n", cmdname, fs_name);
	(void)close(memfd);
	return -1;
}
		
int
rread(device, position, buffer, count)
	int		device;
	off_t		position;
	char		*buffer;
	unsigned	count;
{
	int	rval;
	
	if (lseek(device, position, 0) == (long)-1) {
		Fprintf(stderr, "Seek error in %s\n", KMEM);
		return -1;
	}
	if ((rval = read(device, buffer, count)) == -1) {
		Fprintf(stderr, "Read error in %s\n", KMEM);
		return -1;
	}
	return rval;
}

/*
 * Returns 1 if fstype is a remote file system type, 0 otherwise.
 */
int
remote(fstype)
	char		*fstype;
{
	char		buf[BUFSIZ];
	char		*fs;
	static int	first_call = 1;
 	static FILE	*fp;

	if (first_call) {
		if ((fp = fopen(REMOTE, "r")) == NULL) {
			Fprintf(stderr, "Unable to open %s\n", REMOTE);
			return 0;
		} else {
			first_call = 0;
		}
	} else if (fp != NULL) {
		rewind(fp);
	} else {
		return 0;	/* open of REMOTE had previously failed */
	}
	if (fstype == NULL || strlen(fstype) > FSTYPE_MAX) {
		return	0;	/* not a remote */
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		fs = strtok(buf, " \t");
		if (!strcmp(fstype, fs)) {
			return	1;	/* is a remote fs */
		}
	}
	return	0;	/* not a remote */
}
