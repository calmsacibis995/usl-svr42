/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:desktop/instcmd/instcmd.c	1.2.1.12"

#include <sys/types.h>
#include <sys/mount.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/fsid.h>
#include <sys/fstyp.h>
#include <malloc.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <stdio.h>
#include <string.h>
#include <sys/vtoc.h>
#include <sys/alttbl.h>
#include <sys/altsctr.h>
#include <sys/param.h>
#include <sys/badsec.h>
#include <sys/fdisk.h>
#include <sys/sysi86.h>
#include <sys/errno.h>
#include <sys/termios.h>
#include <sys/wait.h>
#include <stdio.h>

#define ROOTSLICE	1
#define SWAPSLICE	2
#define USRSLICE	3
#define HOMESLICE	4
#define DOSSLICE	5
#define DUMPSLICE	6
#define BOOTSLICE	7
#define ALTSSLICE	8
#define TALTSLICE	9
#define STANDSLICE	10
#define VARSLICE	11
#define HOME2SLICE	12
#define ALTSCTRSLICE	8   /*reuse 8 either ALTSSLICE or ALTSCTRSLICE exists*/

#define	Fs_Dirty	28
#define ONEMB		1048576L

extern	time_t	time();
extern	int	errno;
char	*program = NULL;

/* ######################### commands routines ######################### */

/* the mount  command */
struct fsmount_data {
	char	*type;
	int	flags;
	int	arg1, arg2;
} fstype[] = {
	{ "bfs", MS_DATA, 0, 0 },
	{ "s5", MS_DATA, 0, 0 },
	{ "ufs", MS_FSS, 0, 0 },
	{ "sfs", MS_FSS, 0, 0 },
	{ "vxfs", MS_FSS, 0, 0 },
	{ 0 , 0, 0, 0 }
};


do_mount(argc, argv)
int	argc;
char	*argv[];
{
	int	flags = 0;
	int	verbose = 0;
	struct fsmount_data *fsp = fstype;
	int	opendb_flag = OpenMountDb();

	if(argc == 1) {
		if(opendb_flag >= 0)
				  list_entries();
		exit(0);
	}

	if(argc < 3) {
errorandout:
	     fprintf(stderr,"Usage: %s [-rv] raw-device mount-point\n",
			program);
	     exit(1);
	}

	if(argv[1][0] == '-' ){
		char	*p = &argv[1][1];

		for(; *p != '\0'; p++)
			if(*p == 'r')
				flags = MS_RDONLY;
			else if(*p == 'v')
				verbose++;
			else goto errorandout;
		argc--;
		argv++;
	}

	if(argc != 3) 
		goto errorandout;

	for(;fsp->type != 0;fsp++) {
	  int rv;
	  rv = mount(argv[1], argv[2], flags|fsp->flags, fsp->type, fsp->arg1,fsp->arg2);

	  if (rv < 0 && errno != EINVAL) {
	     	fprintf(stderr,"%s: Failed to mount %s on %s; errno %d\n",
				program, argv[1], argv[2], errno);
	     	   if ( errno == Fs_Dirty ) printf("%s\n", fsp->type);
	     exit(errno);
	  }	
	  else if(rv >= 0){
		if(opendb_flag >= 0){
			add_entry(argv[1], argv[2], fsp->type);
			closemntdb();
		}
		if ( verbose ) 
	     	   printf("%s\t%ld\n", fsp->type, time(0L));
		exit(0);
	  }
	  else if(verbose) {
	     	fprintf(stderr,"%s: Failed to mount %s on %s type(%s); errno %d\n",
				program, argv[1], argv[2], fsp->type, errno);
	     	   if ( errno == Fs_Dirty ) printf("%s\n", fsp->type);
		}
	}
	exit(errno);
}

/* the umount  command */

do_umount(argc, argv)
int	argc;
char	*argv[];
{

	if(argc != 2) {
	     fprintf(stderr,"Usage: %s raw-device|mount-point\n",
			program);
	     exit(1);
	}
	del_mount(argv[1]);
	if(umount(argv[1]) < 0) {
	     fprintf(stderr,"%s: Failed to umount %; errno %d\n",
			program, argv[1], errno);
	     exit(errno);
	}
	exit(0);
}

/* the fakevtoc  command */

int     diskfd;         	/* file descriptor for raw wini disk */
struct  disk_parms      dp;     /* Disk parameters returned from driver */
struct	vtoc	*vtocptr;	/* struct containing slice info */
struct  pdinfo	*pdinfoptr; /* struct containing disk param info */
daddr_t	unix_base;		/* first sector of UNIX System partition */
daddr_t	unix_size;		/* # sectors in UNIX System partition */
struct absio	absio;
struct mboot	mboot1;
struct mboot	mboot2;
struct ipart	*fdp, *unix_part;
int	cylsecs;
long	cylbytes;
int	sectorsize;

int
readsoftvtoc(rawdev)
char	*rawdev;
{
	struct stat statbuf;
	register	int	i;
	char		*buf;
	struct ipart	*tmpfdp;
	static struct vtoc vtocbuf;

	vtocptr = &vtocbuf;

	if (stat(rawdev, &statbuf)) {
		fprintf(stderr, "%s: %s stat of %s failed\n",
				program, rawdev);
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,"%s: device %s is not character special\n",
			program, rawdev);
		exit(1);
	}
	if ((diskfd=open(rawdev,O_RDWR)) == -1) {
		fprintf(stderr,"%s unable to open %s", program, rawdev);
		exit(50);
	}
	if (ioctl(diskfd,V_GET_SOFT_VTOC, vtocptr) == -1) {
		fprintf(stderr,"%s V_GET_SOFT_VTOC failed on %s",
			program, rawdev);
		exit(51);
	}
	return(0);
}

int
readvtoc(rawdev)
char	*rawdev;
{
	struct stat statbuf;
	register	int	i;
	char		*buf;
	struct ipart	*tmpfdp;

	if (stat(rawdev, &statbuf)) {
		fprintf(stderr, "%s: %s stat of %s failed\n",
				program, rawdev);
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,"%s: device %s is not character special\n",
			program, rawdev);
		exit(1);
	}
	if ((diskfd=open(rawdev,O_RDWR)) == -1) {
		fprintf(stderr,"%s unable to open %s", program, rawdev);
		exit(50);
	}
	if (ioctl(diskfd,V_GETPARMS,&dp) == -1) {
		fprintf(stderr,"%s V_GETPARMS failed on %s",
			program, rawdev);
		exit(51);
	}

	cylsecs = (int)dp.dp_heads * dp.dp_sectors;
	cylbytes = (long)cylsecs * dp.dp_secsiz;
	sectorsize =  (int) dp.dp_secsiz;

	absio.abs_sec = 0;
	absio.abs_buf = (char *)&mboot1;
	if (ioctl(diskfd, V_RDABS, &absio) < 0) {
		fprintf(stderr,"%s unable to read partition table from %s",
			program, rawdev);
		exit(70);
	}

	/* find an active UNIX System partition */
	unix_part = NULL;
	tmpfdp = fdp = (struct ipart *)mboot1.parts;
	for (i = FD_NUMPART; i-- > 0; ++tmpfdp) {
#ifdef DEBUG
		fprintf(stderr, "Partition %d has type %d\n",
				i, tmpfdp->systid);
#endif
		if ((tmpfdp->systid == UNIXOS) && (tmpfdp->bootid == ACTIVE))
				unix_part = tmpfdp;
	}
	if (unix_part == NULL) {
		fprintf(stderr, "%s: No active UNIX System partition in partition table!\n", program);
		exit(71);
	}
	unix_base = unix_part->relsect;
	unix_size = unix_part->numsect;
	absio.abs_sec = unix_base + VTOC_SEC;
	buf = absio.abs_buf = (char *)&mboot2;
	if (ioctl(diskfd, V_RDABS, &absio) == -1) {
		fprintf(stderr,"Error Reading pdinfo and VTOC.\n");
		exit(51);
	}

	buf = absio.abs_buf;
	pdinfoptr = (struct pdinfo *) absio.abs_buf; 

	vtocptr = (struct vtoc *)&buf[pdinfoptr->vtoc_ptr%dp.dp_secsiz];
	return(0);
}

/*
 * since we prompt for password in rc script now, we need the ability
 * to "force" a controlling TTY to be allocated while passwd command
 * is running. This alias for instcmd takes an option login ID,
 * assumes "root" otherwise, and runs passwd on this login ID.
 */

do_setpasswd(argc, argv)
int argc;
char	*argv[];
{
	char *login;
	int i,fd;
	char buf [5120];

	if(argc > 2) {
		fprintf(stderr, "Usage: %s <login ID>\n", program);
		exit(1);
	}
	if(argc < 2) {
		login="root";
	} else {
		login=argv[1];
	}

	setpgrp(); /* become group leader */

	for (i=0; i<20; i++)
		close(i);

	fd=open("/dev/console",O_RDWR); /* stdin */
	if (fd==-1)
		exit (4);
	dup(fd);		        /* stdout */
	dup(fd);		        /* stderr */

	/* should now have controlling TTY */
	sprintf (buf, "su %s -c /usr/bin/passwd",login);
	i=system(buf);
	/* i is the process status -- use WIFEXITED to ensure
	 * that process wasn't interrupted. Return 1 if it was,
	 * otherwise whatever WEXITSTATUS says
	 */
	if (WIFEXITED(i)) {
		exit(WEXITSTATUS(i));
	}
	exit (1);
}


do_fakevtoc(argc, argv)
int	argc;
char	*argv[];
{
	int	swapsize, rootsize;
	
	if(argc != 2) {
		fprintf(stderr, "Usage: %s raw-device\n", program);
		exit(1);
	}
        readvtoc(argv[1]);
	swapsize = vtocptr->v_part[SWAPSLICE].p_size * sectorsize;
	/* swap should be at least 7MB */
	if(swapsize < (7*ONEMB)) {
		fprintf(stderr, "SWAPSLICE is too small; %d bytes\n",
			vtocptr->v_part[SWAPSLICE].p_size * sectorsize);
		exit(1);
	}
	/* take 5MB for the temp root from swap */
	swapsize -= (5*ONEMB);
	swapsize = (swapsize/sectorsize) + 1;
	rootsize = vtocptr->v_part[SWAPSLICE].p_size - swapsize;

        vtocptr->v_part[DOSSLICE].p_tag = V_OTHER;
        vtocptr->v_part[DOSSLICE].p_flag = V_VALID;
        vtocptr->v_part[DOSSLICE].p_start = vtocptr->v_part[SWAPSLICE].p_start+
			swapsize;
        vtocptr->v_part[DOSSLICE].p_size = rootsize;
	vtocptr->v_part[SWAPSLICE].p_size -= rootsize;

	if (ioctl(diskfd, V_SET_SOFT_VTOC, vtocptr) == -1) {
		fprintf(stderr,"Error SETING SOFT VTOC. errno %d\n", errno);
		exit(errno);
	}
	exit(0);
}

do_slicesize(argc, argv)
int	argc;
char	*argv[];
{
	int	slicesize;
	int	sliceno;
	char	buf[32];
	int	softslice = 0;
	
	if(argc == 4 && strcmp("-s", argv[1]) == 0){
		softslice = 1;
		argc--;
		argv++;
	}

	if(argc != 3) {
		fprintf(stderr, "Usage: %s raw-device slice-no\n", program);
		exit(1);
	}

	if(softslice)
		readsoftvtoc(argv[1]);
        else	readvtoc(argv[1]);
	sliceno = atoi(argv[2]);

	if(sliceno >= 0 || sliceno < 16) {
		slicesize = vtocptr->v_part[sliceno].p_size;
		sprintf(buf, "%d", slicesize); 
		write(1, buf, strlen(buf));	
		exit(0);
	}
	write(1, "0", 1);
	exit(1);
}

do_printvtoc(argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	if(argc != 2) {
		fprintf(stderr, "Usage: %s raw-device\n", program);
		exit(1);
	}

        readvtoc(argv[1]);

	fprintf(stderr, "sanity is 0x%x\n", vtocptr->v_sanity);
	fprintf(stderr, "volume is %s\n", vtocptr->v_volume);
	fprintf(stderr, "nparts is %d\n", vtocptr->v_nparts);
	for(i=0; i< (int) vtocptr->v_nparts; i++)
	  if(vtocptr->v_part[i].p_tag != V_UNUSED)
	  {
	  	switch(vtocptr->v_part[i].p_tag)
		{
		case V_BOOT: printf("%x	V_BOOT", i); break;
		case V_ROOT: printf("%x	V_ROOT", i); break;
		case V_SWAP: printf("%x	V_SWAP", i); break;
		case V_USR: printf("%x	V_USR", i); break;
		case V_BACKUP: printf("%x	V_BACKUP", i); break;
		case V_ALTS: printf("%x	V_ALTS", i); break;
		case V_OTHER: printf("%x	V_OTHER", i); break;
		case V_ALTTRK: printf("%x	V_ALTTRK", i); break;
		case V_STAND: printf("%x	V_STAND", i); break;
		case V_VAR: printf("%x	V_VAR", i); break;
		case V_HOME: printf("%x	V_HOME", i); break;
		case V_DUMP: printf("%x	V_DUMP", i); break;
		case V_ALTSCTR: printf("%x	V_ALTSCTR", i); break;
	  	default: printf("%x	V_UNKOWN", i); break;
	  	}
                if (vtocptr->v_part[i].p_flag ==  V_VALID ) \
			printf("	MOUNTABLE");
		printf("\n");
#ifdef DEBUG	
	  fprintf(stderr, " p_tag 0x%x, p_flag 0x%x, p_start %ld, p_size %ld\n",
		vtocptr->v_part[i].p_tag,
		vtocptr->v_part[i].p_flag,
		vtocptr->v_part[i].p_start,
		vtocptr->v_part[i].p_size);
#endif
	  }
	  exit(0);
}

/* 
 * For debugging only
 * Not used by the installation procedures
 */

do_listswap(argc, argv)
int	argc;
char	*argv[];
{
	register int	i;
	register swapres_t	*si;
	register struct swaptable	*st;
	register struct swapent *swapent;
	swapres_t		swpi;
	int	num = 0, rtn = 0;
	char	*swapfile;
	int	sliceno;

	if(argc != 2) {
		fprintf(stderr, "Usage: %s root-device \n",
			program);
		exit(1);
	}

	readvtoc(argv[1]);

	if ((num = swapctl(SC_GETNSWP, NULL)) == -1){
		fprintf(stderr,"%s: can't retrieve swap devs\n", program); 
		fflush(stderr);
		exit(1);
	}

        st = (swaptbl_t *) malloc(5 * sizeof(swapent_t) + sizeof(int));
        if (st == NULL) {
                fprintf(stderr,"%s: Malloc for swap setup failed\n", program);
		fflush(stderr);
		exit(1);
	}
	else {
		register int i;
		int	 found = 0;

		swapent=st->swt_ent;
		for (i=0; i<5; i++, swapent++)
			swapent->ste_path = (char *) malloc(MAXPATHLEN);
		st->swt_n = 5;
		if ((num = swapctl(SC_LIST, st)) == -1){
			fprintf(stderr,"%s: can't retrieve swap devs\n", program);
			fflush(stderr);
			exit(1);
		}
		swapent=st->swt_ent;
		for (i=0; i<num; i++, swapent++)
			fprintf(stderr, "SWAP DEVICE %d: <%s>\n", 
				i, st->swt_ent[i].ste_path);
	}
	exit(0);
}

do_sleep(argc, argv)
int	argc;
char	*argv[];
{
	int	seconds;

	if(argc != 2) {
		fprintf(stderr, "Usage: %s seconds\n",
			program);
		exit(1);
	}

	seconds = atoi(argv[1]);
	if(seconds > 0)
		sleep(seconds);
	exit(0);
}

/* the addswap  command */

do_addswap(argc, argv)
int	argc;
char	*argv[];
{
	register int	i;
	register swapres_t	*si;
	register struct swaptable	*st;
	register struct swapent *swapent;
	swapres_t		swpi;
	int	num = 0, rtn = 0;
	char	*swapfile;
	int	sliceno;
	int	softslice = 0;
	
	if(argc == 5 && strcmp("-s", argv[1]) == 0){
		softslice = 1;
		argc--;
		argv++;
	}

	if(argc != 4) {
		fprintf(stderr,"Usage: %s [-s] root-device swap-dev slice-no\n",
			program);
		exit(1);
	}

	if(softslice)
		readsoftvtoc(argv[1]);
        else	readvtoc(argv[1]);

	swapfile = argv[2];
	sliceno = atoi(argv[3]);

	if ((num = swapctl(SC_GETNSWP, NULL)) == -1){
		fprintf(stderr,"%s: can't retrieve swap devs\n", program); 
		fflush(stderr);
		exit(1);
	}

        st = (swaptbl_t *) malloc(5 * sizeof(swapent_t) + sizeof(int));
        if (st == NULL) {
                fprintf(stderr,"%s: Malloc for swap setup failed\n", program);
		fflush(stderr);
		exit(1);
	}
	else {
		register int i;
		int	 found = 0;

		swapent=st->swt_ent;
		for (i=0; i<5; i++, swapent++)
			swapent->ste_path = (char *) malloc(MAXPATHLEN);
		st->swt_n = num;
		if ((num = swapctl(SC_LIST, st)) == -1){
			fprintf(stderr,"%s: can't retrieve swap devs\n", program);
			fflush(stderr);
			exit(1);
		}
/* check if hard disk swap devive added previously, can't reset */
		swapent=st->swt_ent;
		for (i=0; i<num; i++, swapent++)
			if (strcmp(st->swt_ent[i].ste_path, swapfile) == 0){
				exit(0);
			}
	}
/* Add swap device/file to system swap devs */
	si = &swpi;
	si->sr_name = swapfile;
	si->sr_start = 0;
	si->sr_length = vtocptr->v_part[sliceno].p_size;

	if (swapctl(SC_ADD, si) < 0) {
		fprintf(stderr,"%s: add of disk swap device %s failed\n",
			 swapfile, program);
		 fflush(stderr);
		perror(program);
		exit(1);	
	}
	exit(0);	
}


/* the removeswap  command */

do_removeswap(argc, argv)
int	argc;
char	*argv[];
{
	char	*deldev;
	swapres_t	swpi;

	if(argc != 2) {
		fprintf(stderr, "Usage: %s swapfile\n", program);
		exit(1);
	}

	swpi.sr_name = argv[1];
	swpi.sr_start = 0;

        fprintf(stderr,"REMOVING the SWAP device %s from the SWAP list\n", 
		swpi.sr_name); fflush(stderr);

	if (swapctl(SC_REMOVE, &swpi) < 0) {
		fprintf(stderr,"%s: remove of swap device %s failed\n", 
			 program, swpi.sr_name);
		perror(program);
		fflush(stderr);
		exit(1);
	}
	exit(0);
}

do_uadmin(argc, argv)
int	argc;
char	*argv[];
{
	int	cmd, fcn;

	if(argc != 3) {
		fprintf(stderr, "Usage: %s command function\n", program);
		exit(1);
	}
	cmd = atoi(argv[1]);
	fcn = atoi(argv[2]);
	if (uadmin(cmd, fcn, 0) < 0) 
		exit(1);
	exit(0);
}

do_ttyflushin(argc, argv)
int	argc;
char	*argv[];
{

	if (tcflush(0,TCIFLUSH) < 0) {
		fprintf(stderr,"%s unable to tcflush stdin\n", program);
		exit(50);
	}
	exit (0);
}


do_v_remount(argc, argv)
int	argc;
char	*argv[];
{
	struct stat statbuf;
	register	int	i;
	char		*buf, *rawdev;

	if ((argc < 2) || (argc > 2)) {
		fprintf(stderr,"Usage: %s rawdev ]]\n", program);
		exit(1);
	}
	rawdev=argv[1];
	if (stat(rawdev, &statbuf)) {
		fprintf(stderr, "%s: %s stat of %s failed\n",
				program, rawdev);
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,"%s: device %s is not character special\n",
			program, rawdev);
		exit(1);
	}
	if ((diskfd=open(rawdev,O_RDWR)) == -1) {
		fprintf(stderr,"%s unable to open %s\n", program, rawdev);
		exit(50);
	}
	if (ioctl(diskfd,V_REMOUNT,0) < 0) {
		fprintf(stderr,"%s unable to V_REMOUNT %s\n", program, rawdev);
		exit(50);
	}
	(void) close(diskfd);
	exit (0);
}

do_chroot(argc, argv)
int	argc;
char	*argv[];
{
	int	i;

	if(argc < 2) {
		fprintf(stderr,"Usage: %s rootdir [command [arg1 arg2 ...]]\n", program);
		exit(1);
	}
	if(chroot(argv[1]) < 0) {
		fprintf(stderr,"chroot %s FAILED: errno %d\n", errno);
		exit(1);
	}
	if(chdir("/") < 0) {
		fprintf(stderr,"chroot: chdir(/) FAILED: errno %d\n", errno);
		exit(1);
	}
	if(argc == 2){
		execl("/sbin/sh", "sh", 0);
		execl("/bin/sh", "sh", 0);
		fprintf(stderr, "chroot: there is no /sbin/sh or /bin/sh\n");
		exit(1);
	}
	argv[argc] = NULL;
	execvp(argv[2], &argv[2]);	
	fprintf(stderr,"chroot: execv %s FAILED: errno %d\n", argv[2], errno);
	exit(1);
}


main(argc, argv)
int	argc;
char	*argv[];
{
	char	*p = argv[0];
	

	program = argv[0];
	while(*p != '\0'){
		if(*p == '/')
			program = p + 1;
		p++;
	}

	if(strcmp(program, "slicesize") == 0) {
		do_slicesize(argc, argv);
		exit(1);
	} else if(strcmp(program, "chroot") == 0) {
		do_chroot(argc, argv);
		exit(1);
	} else if(strcmp(program, "uadmin") == 0) {
		do_uadmin(argc, argv);
		exit(1);
	} else if(strcmp(program, "ttyflushin") == 0) {
		do_ttyflushin(argc, argv);
		exit(1);
	} else if(strcmp(program, "v_remount") == 0) {
		do_v_remount(argc, argv);
		exit(1);
	} else if(strcmp(program, "listswap") == 0) {
		do_listswap(argc, argv);
	} else if(strcmp(program, "addswap") == 0) {
		do_addswap(argc, argv);
	} else if(strcmp(program, "removeswap") == 0) {
		do_removeswap(argc, argv);
	} else if(strcmp(program, "printvtoc") == 0) {
		do_printvtoc(argc, argv);
	} else if(strcmp(program, "setpasswd") == 0) {
		do_setpasswd(argc, argv);
		exit(0);
	} else if(strcmp(program, "fakevtoc") == 0) {
		do_fakevtoc(argc, argv);
		exit(0);
	} else if(strcmp(program, "sleep") == 0) {
		do_sleep(argc, argv);
		exit(0);
	}
	else if(strcmp(program, "instcmd") == 0) {
		if(argc == 1){
			fprintf(stderr,"Usage: instcmd [mount|umount] ..\n");
			exit(1);
		}
		argc--; argv++;
		program = argv[0];

		if(strcmp(program, "mount") == 0) {
			do_mount(argc, argv);
		} else if(strcmp(program, "umount") == 0) {
			do_umount(argc, argv);
		}
	}
	exit(1);
}

struct mount_entry {
	char	mount_dir[64];
	char	mount_dev[64];
	char	mount_fstype[16];
	char	mount_flags;
	};

static  int	mntfd = -1;
char	*mntdbfile = "/tmp/mntdb";
#define	VALID	01

OpenMountDb()
{
	if((mntfd = open(mntdbfile, O_RDWR| O_CREAT, 0666)) < 0){
		fprintf(stderr, "Cannot open %s, errno %d\n", mntdbfile, errno);
		return(-1);
	}
	return(0);
}

del_mount(special)
char	*special;
{
	struct mount_entry entry;	
	if(OpenMountDb() < 0)
		return(-1);

	lseek(mntfd, 0, 0);
	while(read(mntfd, &entry, sizeof(entry)) == sizeof(entry))
		if(entry.mount_flags & VALID)
			if(strcmp(special, entry.mount_dev) == 0 ||
			   strcmp(special, entry.mount_dir) == 0	){

				entry.mount_flags = 0;
				lseek(mntfd, -sizeof(entry), 1);
				write(mntfd, (char *) &entry, sizeof(entry));
				closemntdb();
				return(1);
			}
	return(0);
}

list_entries()
{
	struct mount_entry entry;	

	lseek(mntfd, 0, 0);
	while(read(mntfd, &entry, sizeof(entry)) == sizeof(entry))
		if(entry.mount_flags & VALID) {
			printf("%s (%s) is mounted on %s\n",
				entry.mount_dev, entry.mount_fstype, entry.mount_dir);
				return(1);
		}
	return(0);
}

closemntdb()
{
	if(mntfd >= 0)
		close(mntfd);
}

add_entry(dev, dir, type)
char	*dev, *dir, *type;
{
	struct mount_entry entry;	


	lseek(mntfd, 0, 0);
	while(read(mntfd, &entry, sizeof(entry)) == sizeof(entry))
		if((entry.mount_flags & VALID) == 0){
			lseek(mntfd, -sizeof(entry), 1);
			break;
		}

	sprintf(entry.mount_dev, "%s", dev);
	sprintf(entry.mount_dir, "%s", dir);
	sprintf(entry.mount_fstype, "%s", type);
	entry.mount_flags = VALID;
	write(mntfd, (char *) &entry, sizeof(entry));

	return(0);
}
