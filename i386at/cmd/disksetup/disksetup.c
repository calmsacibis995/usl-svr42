/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)disksetup:i386at/cmd/disksetup/disksetup.c	1.4.9.34"
#ident	"$Header: $"

/* The file disksetup.c contains the architecture independent routines used   */
/* to install a hard disk as the boot disk or an additional disk. The tasks   */
/* it will handle are: retrieving the location of the UNIX partition, surface */
/* analysis, setting up the pdinfo, VTOC and alternates table and writing     */
/* them to the disk, loading the hard disk boot routine, issuing mkfs, labelit*/
/* and mount requests for slices which will be filesystems, and updating the  */
/* the vfstab file appropriately.					      */

#include <sys/types.h>
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
#include <sys/termios.h>
#include <deflt.h>

/*
 * The following structure is used to contain information about partitions
 */
#define TRUE		1
#define FALSE		0
#define RESERVED        34	/* reserved sectors at start of drive */
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
#define GOOD_BLK	0
#define SNAMSZ		33
#define BLKSZ		11
#define LINESIZE	512
#define ONEMB		1048576L
#define FOURMB		4194394L
#define FS_DIR		"/etc/fs"
#define TOKEN_SEP	"/"
#define MOUNT_CMD	"mount"
#define LABELIT_CMD	"labelit"
#define MKFS_CMD	"mkfs"
#define MKFS_STYL1	1
#define MKFS_STYL2	2
#define MKFS_STYL3	3
#define MKFS_STYL4	4
#define ROOT_INO	2	/* root inode number */

int     diskfd;         	/* file descriptor for raw wini disk */
int     vfstabfd = -1;         	/* file descriptor for /etc/vfstab */
int     childfd;         	/* file descriptor for exec'd children */
int     defaultsfd;         	/* file descriptor for default setup file */
short	defaultsflag = FALSE;	/* Flag to designate valid def. file found */
FILE	*defaultsfile;		/* Flag to designate valid def. file found */
short	defaults_rejected = TRUE; /* Flag to designate if defaults choose */
int	bootfd;			/* boot file descriptor */
int	bootdisk = 0;		/* flag signifying if device is boot disk */
int	installflg = 0;		/* flag signifying installing disk */
struct  disk_parms      dp;     /* Disk parameters returned from driver */
struct	vtoc		vtoc;	/* struct containing slice info */
struct  pdinfo		pdinfo; /* struct containing disk param info */
struct	alt_info 	alttbls;/* struct contains bad sec & track info */
#define sec 		alttbls.alt_sec
#define trk 		alttbls.alt_trk
char    replybuf[160];           /* used for user replies to questions */
char    *devname;		/* pointer to device name */
char    *bootname;		/* pointer to boot file name */
char    mkfsname[20];		/* pointer to device name to issue mkfs calls */
int     cylsecs;                /* number of sectors per cylinder */
long    cylbytes;               /* number of bytes per cylinder */
daddr_t	unix_base;		/* first sector of UNIX System partition */
daddr_t	unix_size;		/* # sectors in UNIX System partition */
daddr_t pstart;			/* next slice start location */
int	load_boot = FALSE;      /* flag for load boot option */
int	scsi_flag = FALSE;	/* flag indicating a scsi drive */
int	instsysflag = FALSE;    /* indicates 2nd disk of dual disk install*/
struct absio	absio;		/* buf used for RDABS & WRABS ioctls */
char	errstring[12] = "Disksetup";
/* querylist is used to request slices in the right order for a boot */
/* disk, i.e., stand, dump, swap, root, usr, home, var, tmp, etc.    */
/* the order creates precedence and physical location on the disk    */
     	int querylist[V_NUMPAR] = {0, 10, 6, 2, 1, 3, 4, 11, 12, 13, 14, 15, 0,
0, 0, 0};


/* sliceinfo has two purposes, first contain setup info for the first disk, */
/* second is to contain info the user chooses for setup of the disk. The */
/* sname field will contain the name of the slice/filesystem. The size field */
/* represents the minimum size slice can be for the system to install. The */
/* createflag designates if the slice is to be created. The field fsslice  */
/* designates the need to issue a mkfs on the slice. */
struct {char sname[SNAMSZ];	/* slice name */
	int  size;		/* recommended size if created */
	short createflag;	/* Turned on when user specified */
	short fsslice;		/* indicate valid file system slice */
	int fsblksz;		/* primary file system block size in bytes */
	char fstypname[SNAMSZ];	/* file system type name */
	short reqflag;		/* Used to indicate required slice (eg. / ) */
	int  minsz;		/* minimum recommended size */
	int flag;		/* general flag field */
	} sliceinfo[V_NUMPAR] = { 0 };

#define SL_NO_AUTO_MNT	0x1

/* The following structure contains the default file system
 * types and attributes located in the /etc/default directory.
 * The file /etc/default/fstyp contains available file system 
 * types. The the file system specific attributes are located in
 * /etc/defaults/file, where file is the name of the fs type.
 * 
 * Required identifiers in /etc/default/fstyp
 *		FSTYP=comma separated list of fstype names 
 *		BOOT_FSTYP=the boot file system type (default bfs)
 * Required identifiers in /etc/default/file (where file is fstyp name)
 *		BLKSIZE=comma separated list of block sizes
 *		MNTOPT=/etc/vfstab mount options
 *		MKFSTYP=1 of 3 mkfs types supported
 *		LABELIT=YES|NO (indicates whether labelit is called)
 */

#define MAXNAME 10	/* max length for fs type name */
#define MAXFS	10	/* max number of fs types */
#define MAXBLKCNT 5	/* max number of per fs block sizes */
#define MAXOPTCNT 80	/* max length of the mount option string */
#define MAXNAMLST 100 /* max length for the fstype name list */
#define BOOTFSTYPE 0x1	/* indicates a boot file system type */
#define DEF_DIR  "/etc/default/"

char fsnamelist[MAXNAMLST];
int fstyp_cnt;		/* cnt of fs types read from /etc/default/fstyp */
char *DEF_FILE = "fstyp";
	
struct fstyp {
	char fsname[MAXNAME];
	ulong blksize[MAXBLKCNT];
	char mntopt[MAXOPTCNT];
	short mkfstyp;
	short labelit;
	short blksiz_cnt;
	short flag;
} fstyp[MAXFS] = {0};

long totalmemsize = 0L;

int verify_flg = 	TRUE;	/* -V option default */
char options_string[] = "VSsIBb:x:d:m:"; 

extern	struct	badsec_lst *badsl_chain;
extern	int	badsl_chain_cnt;
extern  int	Show;
extern  int	Silent;
extern int	*alts_fd;
extern struct	alts_mempart *ap;	/* pointer to incore alts tables*/
extern char	Devfile[];
int    execfd = -1;
char   *execfile = NULL;
void writevtoc();
void offer_defaults();
extern int assign_dos();
extern void rd_fs_defaults();	/* read fs type defaults from /etc/defaults */
extern int find_fs_defaults();	/* find the fs default index */
extern int get_fs_blksize();	/* get fs specific available block sizes */
int xflg = 0;

void
main(argc,argv)
int argc;
char *argv[];
{
	register int	i, j;
	char	*p;
	extern char	*optarg;
	extern int	optind;
	int	c, errflg = 0;
	struct stat statbuf;
	int openflags=O_APPEND|O_CREAT|O_RDWR;

	while ((c = getopt(argc, argv, options_string)) != -1) {
		switch (c) {
		case 'S':
			Show = TRUE;
			break;
		case 's':
			Silent = TRUE;
			break;
		case 'b': 
			if ((bootfd = open(optarg, O_RDONLY)) == -1) {
				fprintf(stderr, "Unable to open specified boot routine.\n");
				exit(40);
			}
			bootname = optarg;
			load_boot = TRUE;  
			break;
		case 'B':
			bootdisk = TRUE;  
			openflags|=O_TRUNC;
			break;
		case 'I':
			installflg = TRUE;  
			break;
		case 'd' :
			if (((defaultsfd = open(optarg, O_RDONLY)) == -1) ||
			   ((defaultsfile = fdopen(defaultsfd, "r"))  == NULL))
				fprintf(stderr,"Unable to open defaults file.\n");
			else
				defaultsflag = TRUE;
			break;
		case 'V':
			verify_flg = TRUE;  
			break;
		case 'x' :
			xflg++;
			execfile = optarg;
			break;
			
		case 'm' :
			totalmemsize = atol(optarg);
			if(totalmemsize  <= 0L)
				totalmemsize = FOURMB;
			break;
			
		case '?':
			++errflg;
		}
	}

	if (argc - optind < 1)
		++errflg;
	if (errflg) {
		giveusage();
		exit(40);
	}
	if(xflg){
		execfd = open(execfile, openflags, 0666);
	}
	devname = argv[optind];
	strcpy( Devfile, devname );
	strncpy(mkfsname, devname, (strlen(devname) - 1)); 
	if (stat(devname, &statbuf)) {
		fprintf(stderr,"disksetup stat of %s failed\n",devname);
		perror(errstring);
		giveusage();
		exit(1);
	}
	if ((statbuf.st_mode & S_IFMT) != S_IFCHR) {
		fprintf(stderr,"disksetup: device %s is not character special\n",devname);
		giveusage();
		exit(1);
	}
	if ((diskfd=open(devname,O_RDWR)) == -1) {
		fprintf(stderr,"Disksetup unable to open %s",devname);
		exit(50);
	}
	alts_fd = &diskfd;
	if (ioctl(diskfd,V_GETPARMS,&dp) == -1) {
		fprintf(stderr,"Disksetup V_GETPARMS failed on %s",devname);
		exit(51);
	}

        if ((dp.dp_type == DPT_SCSI_HD) || (dp.dp_type == DPT_SCSI_OD))
		scsi_flag = TRUE;
	memset((char *)&sliceinfo, 0, sizeof(sliceinfo));
	get_unix_partition(); /*retrieve part. table from disk */

	if (load_boot)
		loadboot();  /* writes boot to track 0 of Unix part. */

	if (installflg) {  /* installing boot disk or additional disks */
		init_structs(); /* initialize pdinfo and vtoc */
		rd_fs_defaults();
		if (verify_flg) {
		/* search for media defects */
			if (do_surface_analysis()) {
				alloc_altsctr_part();/* genalt sctr prtition */
			}
		}

		/* make sure pstart is track aligned */
		if (pstart % (daddr_t)dp.dp_sectors) 
			pstart = (pstart / (daddr_t)dp.dp_sectors + 1) 
				 * dp.dp_sectors;
		if (defaultsflag == TRUE)
			offer_defaults();
		if (defaults_rejected == TRUE)
			setup_vtoc(); /* query user for slices and sizes      */
		if (updatebadsec())
			wr_altsctr();
		(void)assign_dos(); /* assign dos partitions to vtoc */
		writevtoc();  /*writes pdinfo, vtoc and alternates table      */
		create_fs();  /* Issues mkfs calls, mounts and updates vfstab */
		if(execfd >= 0) {
			exec_command("cp /tmp/vfstab /mnt/etc/vfstab");
			exec_command("chmod 644 /tmp/vfstab /mnt/etc/vfstab");
			close(execfd);
		}
	}
	exit(0);
}

giveusage()
{
	fprintf(stderr, "Usage: disksetup -BI[s] -b bootfile [-d configfile] raw-device (install boot disk)\n");
	fprintf(stderr, "Usage: disksetup -I [-d configfile] raw-device (install additional disk(s))\n");
	fprintf(stderr, "Usage: disksetup -b bootfile raw-device (write boot code to disk)\n");
}

fs_error(fsname)
char *fsname;
{
	fprintf(stderr,"Cannot create/mount the %s filesystem.  Please contact\n",fsname);
	fprintf(stderr,"your service representative for further assistance.\n");
	exit(1);
}

/* do_surface_analysis verifies all sectors in the Unix partition. It looks */
/* for bad tracks (3 or more bad sectors in the track) and bad sectors. All */
/* defects are then kept in the appropriate table (ie bad tracks in the bad */
/* track table). Alternates are then reserved for the found defects and for */
/* future defects. Number of alt sectors to be reserved should be the number*/
/* of bad sectors found + 1 sector/MB of space in UNIX partion (minimum 32) */
do_surface_analysis()
{
	extern void scsi_setup();
	extern int do_format;
	extern int ign_check;
	extern int no_format;
	extern int verify;
	extern int do_unix;

	if ((scsi_flag) && (!Silent)) {
		printf("Surface analysis of your disk is recommended\n");
		printf("but not required.\n\n");
		printf("Do you wish to skip surface analysis? (y/n) ");
		if (yes_response()) 
			return(1);
	}
	if (!Silent)
		printf("\nChecking for bad sectors in the UNIX System partition...\n\n");
	do_format = TRUE;
	ign_check = TRUE;
	no_format = TRUE;
	verify = TRUE;
	do_unix = TRUE;
	scsi_setup(devname);
	return(1);

}

/*
 * Writevtoc ()
 * Write out the updated volume label, pdinfo, vtoc, and alternate table.  We
 * assume that the pdinfo and vtoc, together, will fit into a single BSIZE'ed
 * block.  (This is currently true on even 512 byte/block systems;  this code
 * may need fixing if a data structure grows).
 * We are careful to read the block that the volume label resides in, and
 * overwrite the label at its offset;  writeboot() should have taken care of
 * leaving this hole.
 */
void
writevtoc()
{
  	char	*buf;
  	int len, i;

	for (i=1; i < V_NUMPAR; i++) 
		if (vtoc.v_part[i].p_size == 0)	 {
                                vtoc.v_part[i].p_tag = 0;
                                vtoc.v_part[i].p_flag = 0;
                                vtoc.v_part[i].p_start = 0;
		}


	/* allocate a buffer large enough to accomodate the alternates list. */
	len = ((sizeof(alttbls) + 511) / dp.dp_secsiz) * dp.dp_secsiz;
	if ((buf=malloc(len)) == NULL) {
		fprintf(stderr,"writevtoc -- can't allocate buffer\n");
		exit(69);
	}
	/* put pdinfo & vtoc into the same sector */
	*((struct pdinfo *)buf) = pdinfo;
	*((struct vtoc *)&buf[pdinfo.vtoc_ptr%dp.dp_secsiz]) = vtoc;
	absio.abs_sec = unix_base + VTOC_SEC;
	absio.abs_buf = buf;
	if (ioctl(diskfd, V_WRABS, &absio) == -1) {
		fprintf(stderr,"Error Writing pdinfo and VTOC.\n");
		exit(51);
	}
	/*	now do the alternate table	*/
	memcpy(buf, ((char *) &alttbls), sizeof(alttbls)); 

	for (i=0; i < len/512; i++) { 
		absio.abs_sec = unix_base + VTOC_SEC + 1 + i; 
		absio.abs_buf = (buf + (i * 512)); 
		if (ioctl(diskfd, V_WRABS, &absio) != 0) {
			fprintf(stderr,"Error writing alternates table to the disk!\n"); 
			exit(43); 
	    	} 
	} 
	free(buf);
	sync();
	ioctl(diskfd, V_REMOUNT, NULL);
	close(diskfd);
}

int 
yes_response()
{
	for (;;) {
		gets(replybuf);
		if (replybuf[0] == 'y' || replybuf[0] == 'Y') 
			return 1;
		if (replybuf[0] == 'n' || replybuf[0] == 'N') 
			return 0;
		printf("\nInvalid response - please answer with y or n.");
	}
}

fill_vtoc()
{
	int i, j, lastslice = 0;

	for (i=1; i < V_NUMPAR; i++) {
		if (bootdisk)
			j = querylist[i];
		else
			j = i;
		if (sliceinfo[j].size > 0) {
			lastslice = j;
			vtoc.v_part[j].p_start = pstart;
			vtoc.v_part[j].p_size = sliceinfo[j].size;
			pstart += sliceinfo[j].size;
			vtoc.v_part[j].p_flag = V_VALID;
			if (sliceinfo[j].fsslice == FALSE)
				vtoc.v_part[j].p_flag |= V_UNMNT;
			switch (j) {
			case ROOTSLICE  : if (bootdisk == TRUE)
						vtoc.v_part[j].p_tag = V_ROOT;
					  else
						vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			case SWAPSLICE  : if (strcmp(sliceinfo[j].sname,"/dev/swap")== 0)
						vtoc.v_part[j].p_tag = V_SWAP;
					  else
						vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			case STANDSLICE : if (strcmp(sliceinfo[j].sname,"/stand") == 0) 
						vtoc.v_part[j].p_tag = V_STAND;
					  else
						vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			case VARSLICE : if (strcmp(sliceinfo[j].sname,"/var") == 0) 
						vtoc.v_part[j].p_tag = V_VAR;
					  else
						vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			case HOMESLICE  : 
			case HOME2SLICE : if (strncmp(sliceinfo[j].sname,"/home",5) == 0)  
					  	vtoc.v_part[j].p_tag = V_HOME;
					  else
						vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			case USRSLICE   : vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			case DUMPSLICE  : if (strcmp(sliceinfo[j].sname,"/dev/dump") == 0)
					 	vtoc.v_part[j].p_tag = V_DUMP;
					  else
						vtoc.v_part[j].p_tag = V_USR;
				  	  break;
			default	 	: vtoc.v_part[j].p_tag = V_USR;
			}
		}
	}
	for (i=1; i < V_NUMPAR; i++) 
		if (vtoc.v_part[i].p_size > 0)
			vtoc.v_nparts = i+1;
	/* any remaining cyls are tacked onto last slice set up */
	if (vtoc.v_part[lastslice].p_start + vtoc.v_part[lastslice].p_size < 
	    unix_base + unix_size)
		vtoc.v_part[lastslice].p_size = unix_base + unix_size -
			vtoc.v_part[lastslice].p_start;
		
}

daddr_t
check_swapsz(numsects, memsize)
daddr_t numsects;
daddr_t memsize;

{
	daddr_t newsects;
	if ((memsize * (daddr_t)dp.dp_secsiz) > ONEMB * 24)
		newsects = ((numsects/cylsecs) / 2) * cylsecs;
	else
		if ((memsize * (daddr_t)dp.dp_secsiz) > ONEMB * 12)
			newsects = ((numsects/cylsecs) * 0.75) * cylsecs;
		else
			newsects = numsects;
	return newsects;
}

/* offer_defaults will read in defaults file, display the defaults, query the */
/* user if they want the defaults. If the user chooses the defaults, the vtoc */
/* and sliceinfo will be setup accordingly.				      */
void
offer_defaults()
{
	int i, n, slicenum, wflag = FALSE;
	int cnt, totalpcnt = 0;
	int prtflag = TRUE;
	daddr_t init_pstart, memsize, dfltsz, minsz;
	daddr_t oneMBsects = ONEMB/(daddr_t)dp.dp_secsiz; /* 1MB in sectors */
	daddr_t numsects, availsects, availcyls, reqcyls, totalcyls;
	FILE *pipe;
	short reqslice_err = FALSE;
	char slicename[SNAMSZ], fstyp[SNAMSZ], blksz[BLKSZ], sizetype, reqflg, line[LINESIZE];

	if(totalmemsize <= 0L){
		if (((pipe = popen("memsize", "r")) == NULL) ||
		   	(fscanf(pipe, "%ld",&memsize) != 1)) {
			fprintf(stderr,
			"Cannot retrieve size of memory, 4MB will be assumed\n");
			memsize = FOURMB;
		}
		if (pipe != NULL)
			(void)pclose(pipe);
		else
			memsize = FOURMB;
	} else memsize = totalmemsize;

	if (memsize % ONEMB != 0)
		memsize = ((memsize / ONEMB) + 1) * ONEMB;
	memsize /= (daddr_t)dp.dp_secsiz; /* convert memsize to sectors */
	availsects = (unix_base + unix_size) - pstart;
 
	for (i=1; (fgets(&line[0],LINESIZE,defaultsfile) != NULL); i++) {
		n = sscanf(&line[0],"%d %32s %32s %10s %d%c %d%c", &slicenum,
		    slicename, fstyp, blksz, &dfltsz, &sizetype, &minsz, &reqflg); 
		if ((n < 7) ||
		   ((slicenum < 1) || (slicenum >= V_NUMPAR) || (slicenum == BOOTSLICE)) ||
		   ((slicenum == ALTSCTRSLICE) || 
		    (slicenum == ALTSSLICE) || (slicenum == TALTSLICE))) {
			fprintf(stderr,"Disksetup: defaults file line %d is invalid and will be skipped.\n",i);
			continue;
		}
		strcpy(sliceinfo[slicenum].sname, slicename);
		sliceinfo[slicenum].createflag = TRUE;
		sliceinfo[slicenum].size = 0;
		if (((minsz *= oneMBsects) % cylsecs) != 0)
			minsz = (minsz/cylsecs + 1) * cylsecs;
		sliceinfo[slicenum].minsz = minsz;
		if (reqflg == 'R')
			sliceinfo[slicenum].reqflag = TRUE;
		else
			sliceinfo[slicenum].reqflag = FALSE;

		if (strcmp(fstyp, "-") != 0) {
			int x;
			x=atoi(blksz);
			if ((x % 512) != 0) {
				fprintf(stderr,"Disksetup: %s entry \
				has a bad block size '%d', for %s \
				file system, must be a multiple of 512\n",
				slicename, x, fstyp);
				fprintf(stderr, "Entry ignored\n");
				strncpy(sliceinfo[slicenum].sname,"\0",SNAMSZ);
				sliceinfo[slicenum].createflag = FALSE;
				sliceinfo[slicenum].reqflag = FALSE;
				continue;
			}
			strcpy(sliceinfo[slicenum].fstypname, fstyp);
			sliceinfo[slicenum].fsblksz = x;
		    	sliceinfo[slicenum].fsslice = TRUE;
		}
		switch (sizetype) {
		/* set size as neg. to flag for calc. after M and m entries */
		case 'W': 
			  sliceinfo[slicenum].size = -(dfltsz);
			  totalpcnt += dfltsz;
			  wflag = TRUE;
			  break;
		case 'm':
			  if (((numsects = dfltsz * memsize) % cylsecs) != 0)
				numsects = (numsects/cylsecs + 1) * cylsecs;

			  if ((strcmp(slicename, "/dev/swap") == 0) && (dfltsz == 2))
				numsects = check_swapsz(numsects, memsize);

			  if ((numsects <= availsects) && (numsects > minsz))
				sliceinfo[slicenum].size = numsects;
			  else if (availsects < minsz && (!Silent))
				sliceinfo[slicenum].size = 0;
			  else	sliceinfo[slicenum].size = availsects;

			  availsects -= sliceinfo[slicenum].size;
			  break;
		case 'M': 
			  if (((numsects = dfltsz * oneMBsects) % cylsecs) != 0)
				numsects = (numsects/cylsecs + 1) * cylsecs;

			  if (numsects <= availsects)
				sliceinfo[slicenum].size += numsects;
			  else if (availsects < minsz && (!Silent))
				sliceinfo[slicenum].size = 0;
			  else	sliceinfo[slicenum].size = availsects;

			  availsects -= sliceinfo[slicenum].size;
			  break;
		default:  break;
		}
	}
	if (wflag == TRUE) {
		if (availsects > 0) {
			availcyls = availsects / cylsecs;
			totalcyls = availcyls;
			for (i=1; i < V_NUMPAR; i++)
				if (sliceinfo[i].size < 0) {
					n = -(sliceinfo[i].size)*100/totalpcnt;
					reqcyls = (n * totalcyls) / 100;
					if ((reqcyls <= availcyls) &&
					   (reqcyls * cylsecs > sliceinfo[i].minsz))
						sliceinfo[i].size = reqcyls * cylsecs;
					else
						if (availcyls * cylsecs >= sliceinfo[i].minsz)
							sliceinfo[i].size =(sliceinfo[i].minsz/cylsecs + 1) * cylsecs;
						else
							sliceinfo[i].size = 0;
					availcyls -= sliceinfo[i].size/cylsecs;
					availsects -= sliceinfo[i].size;
				}
		}
		else /* W requests made but no sects left, set W slices to 0 */
			for (i=1; i < V_NUMPAR; i++)
				if (sliceinfo[i].size < 0) 
					sliceinfo[i].size = 0;
	}
        /* left over cylinders are added to the root slice */
	if ((availsects > 0) && (availsects > cylsecs)) {
		availcyls = availsects / cylsecs;
		sliceinfo[ROOTSLICE].size += availcyls * cylsecs;
	}
	fclose(defaultsfile);
	close(defaultsfd);
	if (Silent) {
		init_pstart = pstart;
		fill_vtoc();
		defaults_rejected = FALSE;
		return;
	}
	printf("The following slice sizes are the recommended configuration for your disk.\n");
	for (i=1; i < V_NUMPAR; i++)
		if (sliceinfo[i].createflag == TRUE && 
			sliceinfo[i].size > 0)
			if (sliceinfo[i].fsslice == TRUE)
				printf("A %s filesystem of %ld cylinders (%.1f MB)\n",
				  sliceinfo[i].sname, sliceinfo[i].size/cylsecs,
				  (float)sliceinfo[i].size*(float)dp.dp_secsiz/ONEMB);
			else
				printf("A %s slice of %ld cylinders (%.1f MB)\n",
				  sliceinfo[i].sname, sliceinfo[i].size/cylsecs,
				  (float)sliceinfo[i].size*(float)dp.dp_secsiz/ONEMB);
	for (i=1; i < V_NUMPAR; i++) 
		if ((sliceinfo[i].createflag == TRUE) && 
		   (sliceinfo[i].size == 0)) {
			if (prtflag == TRUE) {
				printf("\nBased on the default size recommendations, disk space was not available\n");
				printf("for the following slices:\n");
				prtflag = FALSE;
			}
			if (sliceinfo[i].fsslice == TRUE) 
				if (sliceinfo[i].reqflag == TRUE) {
					reqslice_err = TRUE;
					printf("The Required %s filesystem was not allocated space.\n",sliceinfo[i].sname);
					printf("This slice is required for successful installation.\n\n"); 
				}
				else
					printf("The %s filesystem.\n",sliceinfo[i].sname);
			else
				if (sliceinfo[i].reqflag == TRUE) {
					reqslice_err = TRUE;
					printf("The Required %s slice was not allocated space.\n",sliceinfo[i].sname);
					printf("This slice is required for successful installation.\n\n"); 
				}
				else
					printf("The %s slice.\n",sliceinfo[i].sname);
		}
	init_pstart = pstart;
	fill_vtoc();
	if (reqslice_err == TRUE) {
		/* flush input prior to prompt -- prevent typeahead */
		/* note that we treat tcflush as void -- may not
		 * succeed because input is from file
		 */
		(void) tcflush(0,TCIFLUSH);
		printf("\nThe default layout will not allow all required slices to be created.\n");
		printf("You will be required to designate the sizes of slices to create a\n");
		printf("valid layout for the slices you requested.\n\n");
	}
	else {
		printf("\nIs this configuration acceptable? (y/n) ");
		if (yes_response()) 
			defaults_rejected = FALSE;
	}
	if ((reqslice_err == TRUE) || (defaults_rejected == TRUE)) {
		pstart = init_pstart;
		for (i=1; i < V_NUMPAR; i++) 
			if (sliceinfo[i].createflag && vtoc.v_part[i].p_size) {
				vtoc.v_part[i].p_size = 0;
				vtoc.v_part[i].p_start = 0;
				vtoc.v_part[i].p_flag = 0;
			}
	}
}

/* setup_vtoc will make the calls to first obtain the slice configuration   */
/* info and then obtain the sizes for the slices the user choose.           */
setup_vtoc()
{
	daddr_t init_pstart = pstart;
	int i, define_slices = TRUE;
	short reqslice_err;

	if (defaultsflag == TRUE)
		define_slices = FALSE;
	for (;;) {
		reqslice_err = FALSE;
		if (define_slices == TRUE)
			if (bootdisk)
				get_bootdsk_slices();
			else
				get_slices();	
		get_slice_sizes();
		printf("\nYou have specified the following disk configuration:\n");
		for (i=1; i < V_NUMPAR; i++) {
			if (sliceinfo[i].createflag )
				if (sliceinfo[i].fsslice == TRUE)
					printf("A %s filesystem with %d cylinders (%.1f MB)\n",
					  sliceinfo[i].sname,vtoc.v_part[i].p_size/cylsecs,
					  (float)vtoc.v_part[i].p_size*(float)dp.dp_secsiz/ONEMB);
				else
					printf("A %s slice with %d cylinders (%.1f MB)\n",
					  sliceinfo[i].sname,vtoc.v_part[i].p_size/cylsecs,
					  (float)vtoc.v_part[i].p_size*(float)dp.dp_secsiz/ONEMB);
		/* Go through to set v_nparts to be the total number of */
		/* slices which includes slice 0 */
			if (vtoc.v_part[i].p_size > 0)
				vtoc.v_nparts = i + 1;
			if ((sliceinfo[i].reqflag == TRUE) &&
			   (vtoc.v_part[i].p_size == 0)) {
				printf("Required slice %s was not allocated space.\n", sliceinfo[i].sname);
				reqslice_err = TRUE;
			}
		}
		if (reqslice_err == TRUE) {
			printf("A required slice was not allocated space. You must reallocate the disk space\n");
			printf("such that all required slices are created.\n\n");
		}
		else {
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
		 	 * succeed because input is from file
			 */
			(void) tcflush(0,TCIFLUSH);
			printf("\nIs this allocation acceptable to you (y/n)? ");
			if (yes_response())
				break;
		}
		if (defaultsflag == FALSE) {
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
		 	 * succeed because input is from file
		 	 */
			(void) tcflush(0,TCIFLUSH);
			printf("\nYou have rejected the disk configuration. Do you want\n");
			printf("to redefine the slices to be created? (y/n)? ");
			if (yes_response())
				define_slices = TRUE;
			else
				define_slices = FALSE;
		}
		pstart = init_pstart;
		for (i=1; i < V_NUMPAR; i++) {
			if (sliceinfo[i].createflag && vtoc.v_part[i].p_size) {
				vtoc.v_part[i].p_size = 0;
				vtoc.v_part[i].p_start = 0;
				vtoc.v_part[i].p_flag = 0;
				if (define_slices == TRUE) {
					sliceinfo[i].createflag = 0;
					vtoc.v_part[i].p_tag = 0;
				}
			}
		}
	}
}
int
get_fs_type(req_flag, slice_indx)
int req_flag; /* does the slice require a mkfs type */
int slice_indx;
{
int fs_indx;
char tmp[MAXNAMLST];
char *p;
int s;
	if (req_flag == TRUE) 
		for (;;) {
			strcpy(tmp, fsnamelist);
			p = strtok(tmp, ",");
			s = strlen(p);
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
		 	 * succeed because input is from file
		 	 */
			(void) tcflush(0,TCIFLUSH);
			printf("\nEnter the filesystem type for this slice\n");
			printf("(%s), or <ENTER> to use the default (%s):", (fsnamelist+s+1), p);
			gets(replybuf);

			/* use the default fs type */
			if (strcmp(replybuf, "") == 0)
				strcpy(replybuf, p);

			if ((fs_indx=find_fs_defaults(replybuf, 0)) != -1) {
				strcpy(sliceinfo[slice_indx].fstypname, replybuf);
				sliceinfo[slice_indx].fsblksz = get_fs_blksize(fs_indx);
				return(TRUE);
			}

			printf("Invalid response - please answer with %s\n",
				fsnamelist);
		}
	else
		for (;;) {
			strcpy(tmp, fsnamelist);
			p = strtok(tmp, ",");
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
			 * succeed because input is from file
			 */
			(void) tcflush(0,TCIFLUSH);
			printf("\nEnter the filesystem type for this slice (%s),\n", fsnamelist);
			printf("type 'na' if no filesystem is needed, or press\n");
			printf("<ENTER> to use the default (%s):", p);
			gets(replybuf);

			/* use the default fs type */
			if (strcmp(replybuf, "") == 0)
				strcpy(replybuf, p);

			if ((strncmp(replybuf,"na",2) == 0) ||
		    	    (strncmp(replybuf,"NA",2) == 0))
				return(0);

			if ((fs_indx=find_fs_defaults(replybuf, 0)) != -1) {

				strcpy(sliceinfo[slice_indx].fstypname, replybuf);
				sliceinfo[slice_indx].fsblksz = get_fs_blksize(fs_indx);
				return(TRUE);
			}

			printf("\nInvalid response - please answer with (%s or na.\n\n", fsnamelist);
		}
}

/* get_bootdsk_slices queries the user on their preferences for the setup  */
/* of a boot disk. This allows for setup of root, swap, usr, usr2, dump,   */
/* stand, and home slices.						   */
get_bootdsk_slices()
{
int fs_indx;
	printf("You will now be queried on the setup of your disk. After you\n");
	printf("have determined which slices will be created, you will be \n");
	printf("queried to designate the sizes of the various slices.\n\n");
	sliceinfo[ROOTSLICE].createflag = TRUE;
	vtoc.v_part[ROOTSLICE].p_tag = V_ROOT;
	sprintf(sliceinfo[ROOTSLICE].sname,"/");
	printf("A root filesystem is required and will be created.\n");
	sliceinfo[ROOTSLICE].fsslice = get_fs_type(TRUE, ROOTSLICE);
	sliceinfo[SWAPSLICE].createflag = TRUE;
	vtoc.v_part[SWAPSLICE].p_tag = V_SWAP;
	sprintf(sliceinfo[SWAPSLICE].sname,"/dev/swap");
	sliceinfo[STANDSLICE].createflag = TRUE;
	sprintf(sliceinfo[STANDSLICE].sname,"/stand");
	sliceinfo[STANDSLICE].fsslice = TRUE;
	/* find boot file system type */
	if (fs_indx=find_fs_defaults("bfs", 0) == -1) {
		if ((fs_indx=find_fs_defaults("", BOOTFSTYPE)) != -1) {
			strcpy(sliceinfo[STANDSLICE].fstypname, fstyp[fs_indx].fsname);
			sliceinfo[STANDSLICE].fsblksz = 
				fstyp[fs_indx].blksize[0];
		} else {
			fprintf(stderr,"disksetup: no valid boot file system type found, default is bfs\n");
			fprintf(stderr,"disksetup: check /etc/default/bfs\n");
			exit(-1);
		}
	}
	else {
		sprintf(sliceinfo[STANDSLICE].fstypname, "bfs");
		sliceinfo[STANDSLICE].fsblksz = 512;
	}

	vtoc.v_part[STANDSLICE].p_tag = V_STAND;
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	printf("\nDo you wish to have separate root and usr filesystems (y/n)? ");
	if (yes_response()) {
		sliceinfo[USRSLICE].createflag = TRUE;
		vtoc.v_part[USRSLICE].p_tag = V_USR;
		sliceinfo[USRSLICE].fsslice = get_fs_type(TRUE, USRSLICE);
		sprintf(sliceinfo[USRSLICE].sname,"/usr");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	printf("\nDo you want to allocate a crash/dump area on your disk (y/n)? ");
	if (yes_response()) {
		sliceinfo[DUMPSLICE].createflag = TRUE;
		vtoc.v_part[DUMPSLICE].p_tag = V_DUMP;
		sprintf(sliceinfo[DUMPSLICE].sname,"/dev/dump");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	printf("\nDo you want to create a home filesystem (y/n)? "); 
	if (yes_response()) { 
		sliceinfo[HOMESLICE].createflag = TRUE; 
		vtoc.v_part[HOMESLICE].p_tag = V_HOME; 
		sliceinfo[HOMESLICE].fsslice = get_fs_type(TRUE, HOMESLICE);
		sprintf(sliceinfo[HOMESLICE].sname,"/home");
	} 
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	printf("\nDo you want to create a var filesystem (y/n)? "); 
	if (yes_response()) {
		sliceinfo[VARSLICE].createflag = TRUE;
		vtoc.v_part[VARSLICE].p_tag = V_VAR;
		sliceinfo[VARSLICE].fsslice = get_fs_type(TRUE, VARSLICE);
		sprintf(sliceinfo[VARSLICE].sname,"/var");
	}
	/* flush input prior to prompt -- prevent typeahead */
	/* note that we treat tcflush as void -- may not
	 * succeed because input is from file
	 */
	(void) tcflush(0,TCIFLUSH);
	printf("\nDo you want to create a home2 filesystem (y/n)? "); 
	if (yes_response()) { 
		sliceinfo[HOME2SLICE].createflag = TRUE; 
		vtoc.v_part[HOME2SLICE].p_tag = V_HOME; 
		sliceinfo[HOME2SLICE].fsslice = get_fs_type(TRUE, HOME2SLICE);
		sprintf(sliceinfo[HOME2SLICE].sname,"/home2");
	} 
}

/* chkname verifies that the name for the slice is a directory file
 * that it isn't allocated to another slice and that the directory
 * is not currently mounted on. 
 */
int
chkname(slicename,cur_index)
char *slicename;
int cur_index;
{
 	int i;
	struct stat statbuf;

        if (stat(slicename, &statbuf) == 0)
		if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
                        fprintf(stderr,"\n%s is not a directory file.\n", slicename);
                        return(1);
		}
		if (statbuf.st_ino == ROOT_INO) {
			fprintf(stderr,"\n%s directory is already mounted on.\n", slicename);
			return(1);
		}
	for (i=0; i<cur_index; i++) {
		if (strcmp(sliceinfo[i].sname,slicename) == 0) {
 			fprintf(stderr,"\n%s directory has already been used.\n",slicename);
			return(1);
		}
	}
	return(0);
}

/* get_slices will query the user to collect slices for configuring   */
/* additional disks added to the system. The user will be allowed to  */
/* choose the number of slices desired and the names of them.	      */
get_slices()
{
	long count, len, i, slices;
	long availslices=0;
	char *endptr;

	for (i=1; i < V_NUMPAR; i++)
		if (vtoc.v_part[i].p_size == 0)
			availslices++;
	printf("You will now be queried on the setup of your disk. After you\n");
	printf("have determined which slices will be created, you will be \n");
	printf("queried to designate the sizes of the various slices.\n");
	for (;;) {
		/* flush input prior to prompt -- prevent typeahead */
		/* note that we treat tcflush as void -- may not
		 * succeed because input is from file
		 */
		(void) tcflush(0,TCIFLUSH);
		printf("\nHow many slices/filesystems do you want created on the disk (1 - %d)? ", availslices);
		gets(replybuf);
		slices = strtol(replybuf, &endptr, 10);
		if ((replybuf != endptr) &&
		   ((slices > 0) && (slices <= availslices)))
			break;
		printf("Illegal value: %d; try again. \n", slices);
	}
	for (i = 1, count = 1; count <= slices && i < V_NUMPAR; i++) 
		if (vtoc.v_part[i].p_size == 0) {
			/* flush input prior to prompt -- prevent typeahead */
			/* note that we treat tcflush as void -- may not
			 * succeed because input is from file
			 */
			(void) tcflush(0,TCIFLUSH);
			for (;;) {
			printf("\nPlease enter the absolute pathname (e.g., /usr3) for \n");
			printf("slice/filesystem %d (1 - 32 chars)? ",count);
				gets(replybuf);
                                if (((len = strlen(replybuf) + 1) >= SNAMSZ) ||
                                   (replybuf[0] != '/')) {
			 /* flush input prior to prompt -- prevent typeahead */
			 /* note that we treat tcflush as void -- may not
			  * succeed because input is from file
			  */
			 		(void) tcflush(0,TCIFLUSH);
                                        printf("\nIllegal value: %s \n",replybuf);
					printf("Value must begin with '/' and contain 32 characters or less.\n");
                                }
                                else
                                    	if (chkname(replybuf,i) == 0)
                                                break;
			}
			sprintf(sliceinfo[i].sname, "          ");
			strncpy(sliceinfo[i].sname, replybuf, len);
			sliceinfo[i].createflag = TRUE;
			sliceinfo[i].size = 0;
			sliceinfo[i].fsslice = get_fs_type(FALSE, i); 
			vtoc.v_part[i].p_tag = V_USR;
			(void) tcflush(0,TCIFLUSH);
			printf("\nShould %s be automatically mounted during a reboot?\n",
					sliceinfo[i].sname);
			printf("Type \"no\" to override auto-mount or press enter to enable the option:");
		
			gets(replybuf);
			if ((strcmp(replybuf, "no") == 0) ||
			    (strcmp(replybuf, "NO") == 0))
				sliceinfo[i].flag = SL_NO_AUTO_MNT;
			count++;
		}
}

/* get_slice_sizes will go through the sliceinfo structure to query the */
/* user on the desired slice size. The slices to be queried on will have */
/* the createflag set. Slices which have predetermined sizes (boot and alts */
/* will have been setup in other routines.				*/
get_slice_sizes()
{
	long cyls, i, j;
	long minsum;
	long minsumrem;
	long remcyls, minsz, maxsz;
	/* calc cylsper_1mb by rounding up # of cyls >= 1 MB */
	int cylsper_1mb = (ONEMB + cylbytes/2) / cylbytes;
	char *endptr;

	minsum=0;
	for (i=1; i < V_NUMPAR; i++) {
		if (bootdisk)
			j = querylist[i];
		else
			j = i;
		if (sliceinfo[j].createflag) 
			minsum += (sliceinfo[j].minsz+cylsecs/2)/cylsecs;
	}
	remcyls = ((unix_base + unix_size) - pstart) / cylsecs;
	printf("\nYou will now specify the size in cylinders of each slice.\n");
	printf("(One megabyte of disk space is approximately %d cylinders.)\n", cylsper_1mb);
	for (i=1; i < V_NUMPAR; i++) {
		if (bootdisk)
			j = querylist[i];
		else
			j = i;
		if ((sliceinfo[j].createflag) && (remcyls > 0)) 
			for (;;) {
				minsz = 0;
				if (sliceinfo[j].minsz > 0) {
					minsz = (sliceinfo[j].minsz+cylsecs/2)/cylsecs;
					printf("\nThe recommended minimum size for the %s slice is %d cylinders (%d MB).\n",
					sliceinfo[j].sname, minsz,
					(sliceinfo[j].minsz*(int)dp.dp_secsiz+ONEMB/2)/ONEMB);
				}
				/* Keep track of min reqts for remaining
				 * slices. subtract this slice's min from that
				 * of the remaining slices.
				 */
				minsumrem = minsum - minsz;

				/* maxsz is the track of min reqts for remaining
				 * slices. subtract this slice's min from that
				 * of the remaining slices.
				 */

				maxsz = remcyls - minsumrem;

				/* flush input prior to prompt -- prevent typeahead */
				/* note that we treat tcflush as void -- may not
		 		 * succeed because input is from file
		 		 */
				(void) tcflush(0,TCIFLUSH);
				printf("How many cylinders would you like for %s (%d - %d)?\n",sliceinfo[j].sname,minsz,maxsz);
				printf("Hit <ENTER> for %d cylinders:\n", minsz);
				gets(replybuf);
				cyls = strtol(replybuf, &endptr, 10);
				if (replybuf[0] == '\0')
					/* if user typed return, use minimum value */
					cyls = minsz;
				if (cyls < minsz) {
					printf("Slice %s must be at least %d cylinders; please enter again\n", sliceinfo[j].sname, minsz);
					continue;
				}
				if (cyls > maxsz ) {
					printf("Slice %s must be no more than %d cylinders; please enter again\n", sliceinfo[j].sname, maxsz);
					continue;
				}
				vtoc.v_part[j].p_start = pstart;
				vtoc.v_part[j].p_size = cyls * cylsecs;
				vtoc.v_part[j].p_flag = V_VALID;
				pstart += cyls * cylsecs;
				if (sliceinfo[j].fsslice == 0)
					vtoc.v_part[j].p_flag |=V_UNMNT;
				remcyls -= cyls;
				minsum=minsumrem;
				break;
			}
	}
	if (remcyls)
		printf("\nNotice: The selections you've made will leave %d cylinders unused.\n",remcyls);
}

/* issue_mkfs will handle the details of the mkfs exec. The items to be dealt */
/* with include which mkfs, and where mkfs is 				      */
issue_mkfs(slice, rawdev, size, secspercyl)
int slice;
char *rawdev, *size, *secspercyl;
{
        char *inodectrl; /* will be set conditionally to "-o C" or ""
			  * depending on if Silent is on. This is
			  * for ufs and sfs mkfs only in order to
			  * allow auto install create file systems
			  * that won't break COFFs.
	  		  */
	int fs_indx;
	char inodearray[30] = {0};
	if (sliceinfo[slice].fsslice) {
		fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);

		switch (fstyp[fs_indx].mkfstyp) {

		case MKFS_STYL1 :
			if (!Silent && !xflg) {
				ino_t ricount;
				ino_t icount = (atol(size)/(sliceinfo[slice].fsblksz/512))/4;

				printf("Allocated approximately %d inodes for this file system\n", icount);
				printf("Specify a new value or press <Enter> to use the default:");
				printf("\n");
				gets(replybuf);
				if (strcmp(replybuf, "") != 0) {
					ricount=atol(replybuf);
					if  (ricount != icount) {
						strcpy(inodearray, ":");
						strncat(inodearray, replybuf,(sizeof(inodearray)-2));
					}
				}
			} else
				strcpy(inodearray, "");
			sprintf(replybuf,"%s%s%s%s%s -b %d %s %s%s 1 %s >/dev/null 2>&1",
				FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
				TOKEN_SEP, MKFS_CMD, sliceinfo[slice].fsblksz,
				rawdev, size, inodearray, secspercyl);
			break;

		case MKFS_STYL2 :
				
			sprintf(replybuf,"%s%s%s%s%s %s %s >/dev/null 2>&1",
				FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
				TOKEN_SEP, MKFS_CMD, rawdev,size);
			break;

		case MKFS_STYL3 :
			if (!Silent && !xflg) {
				long rinodeval;
				long inodeval = 2048;
				printf("One inode is allocated for each %d bytes of file system\n", inodeval);
				printf("space. Specify a value in units of bytes or press <Enter>\nto use the default value:");
				printf("\n");
				gets(replybuf);
				if (strcmp(replybuf, "") != 0) {
					rinodeval = atol(replybuf);
					if (rinodeval != inodeval) {
						strcpy(inodearray, ",nbpi=");
						strncat(inodearray, replybuf, (sizeof(inodearray)-6));
					}
				}
			} else
				strcpy(inodearray, "");
			if (Silent)
				inodectrl = "C,";
			else
				inodectrl = "";
			sprintf(replybuf,"%s%s%s%s%s -o %snsect=%d,ntrack=%d,bsize=%d%s %s %s >/dev/null 2>&1",
			FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
			TOKEN_SEP, MKFS_CMD, inodectrl,
			(int)dp.dp_sectors,(int)dp.dp_heads,
			sliceinfo[slice].fsblksz, inodearray, rawdev, size);
			break;

		case MKFS_STYL4 :
			if (!Silent && !xflg) {
				ino_t ricount;
				ino_t icount = ((atol(size)-256)/(sliceinfo[slice].fsblksz/512))/4;

				printf("Allocated approximately %d inodes for this file system. Specify a\n", icount);
				printf("new value or press <Enter> to use the default:");
				printf("\n");
				gets(replybuf);
				if (strcmp(replybuf, "") != 0) {
					ricount=atol(replybuf);
					if  (ricount != icount) {
						strcpy(inodearray, ",ninode=");
						strncat(inodearray, replybuf,(sizeof(inodearray)-9));
					}
				}
			} else
				strcpy(inodearray, "");

			if (Silent)
				inodectrl = "C,";
			else
				inodectrl = "";
			sprintf(replybuf,"%s%s%s%s%s  -o %sbsize=%d%s %s %s >/dev/null 2>&1",
				FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname,
				TOKEN_SEP, MKFS_CMD, inodectrl, sliceinfo[slice].fsblksz,
				inodearray, rawdev, size);
			break;

		default :
			fprintf(stderr,"disksetup: Invalid mkfs type specified for slice %s, file system type %s\n",
				sliceinfo[slice].sname, sliceinfo[slice].fstypname);
			fprintf(stderr,"entry ignored\n");
			return;
		}
	}

	if (Show)
		printf("Running cmd: %s\n", replybuf);
	if (exec_command(replybuf)) {
		fprintf(stderr,"Disksetup: unable to create filesystem on %s.\n",rawdev);
		perror(errstring);
		fs_error(sliceinfo[slice].sname);
	}
}

create_fs()
{
	int i, status;
	char secspercyl[5];
	char gap[2];
	int blkspersec;
	char buf[100];
	int fs_indx;

	blkspersec = dp.dp_secsiz / 512;
	sprintf(secspercyl, "%d", cylsecs);
	if (!Silent) {
		sprintf(buf, "echo \"\nFilesystems will now be created on the needed slices\n\"");
		exec_command(buf);
	}
	for (i=1; i < V_NUMPAR; i++) {
		if ((vtoc.v_part[i].p_size > 0) && (sliceinfo[i].fsslice == TRUE)) {
			char rawdev[20], size[12];
			sprintf(rawdev, "%s%x", mkfsname, i);
			if (!Silent) {
				sprintf(buf, "echo Creating the %s filesystem on %s \n",sliceinfo[i].sname,rawdev);
				exec_command(buf);
			}
			sprintf(size, "%ld", vtoc.v_part[i].p_size*blkspersec);
			issue_mkfs(i, rawdev, size, secspercyl);
			
			if (sliceinfo[i].fsslice == TRUE) {  
				fs_indx=find_fs_defaults(sliceinfo[i].fstypname,0);
				if (fs_indx != -1 && 
					fstyp[fs_indx].labelit == TRUE)

					label_fs(i, rawdev);
			}
			write_vfstab(i, rawdev);
		}
	}
	close(vfstabfd);
}

label_fs(slice, dev)
int slice;
char *dev;
{
	int status;
	char disk[7];

	sprintf(disk,"slic%d",slice);
	/* build labelit command */
	sprintf(replybuf,"%s%s%s%s%s %s %.6s %.6s >/dev/null 2>&1",
		FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname, TOKEN_SEP,
		LABELIT_CMD, dev, sliceinfo[slice].sname, disk);
	if (Show)
		printf("Running cmd: %s\n", replybuf);
	if (exec_command(replybuf)) {
		fprintf(stderr,"Disksetup: warning unable to label slice %s %s %s\n",dev, sliceinfo[slice].sname, disk);
		perror(errstring);
	}
}

write_vfstab(slice, dev)
int slice;
char *dev;
{
	char blkdev[20], buf[100], *tmppt;
	int perms, status, len;
	int mountfd, i, found = 0;
	struct stat statbuf;
	char	*mount_it;
	char option[MAXOPTCNT] = "-";

	sprintf(blkdev, "%s", dev);
	tmppt = blkdev;
	while (*tmppt != NULL) {
		if (*tmppt == 'r' || found) {
			*tmppt = *(tmppt+1);
			found = TRUE;
		}
		tmppt++;
	}
	if (bootdisk && slice == 1) {
			/* build mount cmd */
			sprintf(replybuf,"%s%s%s%s%s %s /mnt >/dev/null 2>&1",
			FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname, TOKEN_SEP,
			MOUNT_CMD, blkdev);
		if (Show)
			printf("Running cmd: %s\n", replybuf);
		if (exec_command(replybuf)) {
			fprintf(stderr,"Disksetup: cannot mount root\n");
			perror(errstring);
			fs_error(sliceinfo[slice].sname);
		}
		if(execfd >= 0) {
			sprintf(replybuf, "mkdir /mnt/etc; chmod 775 /mnt/etc\n");
			exec_command(replybuf);
			}
		else	if (mkdir("/mnt/etc", 0775) == -1) {
			fprintf(stderr,"Cannot create /mnt/etc.\n");
			perror(errstring);
			fs_error(sliceinfo[slice].sname);
		}
		if (execfd < 0)
			vfstabfd=open("/mnt/etc/vfstab",O_CREAT|O_WRONLY,0644);
		else 	vfstabfd=open("/tmp/vfstab",O_CREAT|O_WRONLY,0644);
		if(vfstabfd < 0){
			fprintf(stderr,"Cannot create /etc/vfstab.\n");
			perror(errstring);
			fs_error(sliceinfo[slice].sname);
		}
		else {

		    if (sliceinfo[slice].fsslice == TRUE) {
			int fs_indx;
			fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);
			if (strlen(fstyp[fs_indx].mntopt) > 0)
				strcpy(option, fstyp[fs_indx].mntopt); 
			len = sprintf(buf,
	"/dev/root	/dev/rroot	/	%s	1	no	%s\n",
	sliceinfo[slice].fstypname, option);

			if(write(vfstabfd, buf, len) != len ) {
				fprintf(stderr,"Disksetup: cannot write /etc/vfstab entry.\n");
				perror(errstring);
				fs_error(sliceinfo[slice].sname);
			}

		    }
		}
	}
	else {
	   if (vfstabfd < 0) {
		if (execfd  < 0) {
		     vfstabfd=open("/etc/vfstab",O_WRONLY|O_APPEND);
		     if(vfstabfd < 0) {
			     vfstabfd=open("/mnt/etc/vfstab",O_WRONLY|O_APPEND);
			     instsysflag = TRUE;
		     }
		} else {
			vfstabfd=open("/tmp/vfstab",O_WRONLY|O_APPEND);
			instsysflag = TRUE;
		}
		if (vfstabfd < 0) {
			instsysflag = FALSE;
			fprintf(stderr,"Disksetup: cannot open /etc/vfstab.\n");
			perror(errstring);
			fs_error(sliceinfo[slice].sname);
		}
	    }

		if( strcmp(sliceinfo[slice].sname, "/") == 0 ||
		    strcmp(sliceinfo[slice].sname, "/stand") == 0 ||
		    strcmp(sliceinfo[slice].sname, "/var") == 0 ||
		    sliceinfo[slice].flag & SL_NO_AUTO_MNT)
			mount_it = "no";
		else	mount_it = "yes";

		if (sliceinfo[slice].fsslice == TRUE) { 
			int fs_indx;
			fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);
			if (strlen(fstyp[fs_indx].mntopt) > 0)
				strcpy(option, fstyp[fs_indx].mntopt); 
		}
		if (sliceinfo[slice].fsslice == TRUE) 
			len = sprintf(buf, "%s	%s	%s	%s	1	%s	%s\n", blkdev, dev, sliceinfo[slice].sname, sliceinfo[slice].fstypname,
			mount_it, option);
		if(write(vfstabfd, buf, len) != len) {
			fprintf(stderr,"Disksetup: cannot write /etc/vfstab entry.\n");
			perror(errstring);
			fs_error(sliceinfo[slice].sname);
		}
		if ((bootdisk == TRUE) || (instsysflag == TRUE)) {
			sprintf(buf, "/mnt%s", sliceinfo[slice].sname);
			if (strncmp(sliceinfo[slice].sname,"/tmp",4) == 0)   
				perms = 01777;
			else 
				perms = 0755;
			if (execfd >= 0) {
				len = sprintf(replybuf, "mkdir %s; chmod %o %s\n",
					buf, perms, buf);
				write(execfd, replybuf, strlen(replybuf));
				}
			else	if (mkdir(buf, perms) != 0) {
				fprintf(stderr,"Disksetup: could not create %s mount point.\n",buf);
				perror(errstring);
				fs_error(buf);
			}
		}
		else {
			strcpy(buf, sliceinfo[slice].sname);
			sprintf(replybuf,"mkdir -m 0755 -p %s",buf);
			if (execfd >= 0)
				exec_command(replybuf);
			else	if (stat(buf, &statbuf) == -1)  {
				if (Show)
					printf("Running cmd: %s\n", replybuf);
				if (exec_command(replybuf) != 0) {
					fprintf(stderr,"Disksetup: could not create %s mount point.\n",buf);
					perror(errstring);
				}
			}
			else {
				if (!(statbuf.st_mode & S_IFDIR)) {
					fprintf(stderr,"Disksetup: %s is not a valid mount point\n",buf);
					fs_error(buf);
				}
			}
		}
		/* if a mount option is specified use it 
		 * in the mount request.
	 	 */
		if (sliceinfo[slice].fsslice == TRUE) {
			int fs_indx;
			fs_indx = find_fs_defaults(sliceinfo[slice].fstypname, 0);
			if (strlen(fstyp[fs_indx].mntopt) > 0) {
				strcpy(option, "-o");
				strcat(option, fstyp[fs_indx].mntopt); 
			} else
				strncpy(option, "", sizeof(option));

			sprintf(replybuf,"%s%s%s%s%s %s %s %s >/dev/null 2>&1",
			FS_DIR, TOKEN_SEP, sliceinfo[slice].fstypname, TOKEN_SEP,
			MOUNT_CMD, option, blkdev, buf);
		}

		if (Show)
			printf("Running cmd: %s\n", replybuf);
		if (exec_command(replybuf)) {
			fprintf(stderr,"Disksetup: unable to mount %s.\n",buf);
			perror(errstring);
			fs_error(sliceinfo[slice].sname);
		}
	}
}

/* Utility routine for turning signals on and off */
set_sig(n)
void (*n)();
{
	signal(SIGINT, n);
	signal(SIGQUIT, n);
	signal(SIGUSR1, n);
	signal(SIGUSR2, n);
}

set_sig_on()
{
	set_sig(SIG_DFL);
}

set_sig_off()
{
	set_sig(SIG_IGN);
}

/*
 *	Allocate the alternate sector/track partition 
 *	enter the partition information in the vtoc table
 */
alloc_altsctr_part()
{
	int	altsec_size;
	int	extra_alt;

	altsec_size = cylsecs - (pstart % cylsecs);
 	if ((extra_alt=(unix_size * (daddr_t)dp.dp_secsiz)/ONEMB) < 32)
		extra_alt = 32;
 	if (altsec_size < (badsl_chain_cnt + extra_alt))
		altsec_size += (((badsl_chain_cnt + extra_alt) 
				/ (int)dp.dp_sectors) + 1) * dp.dp_sectors;

	/* reserve space in chunks of cylinders to correspond with the	*/
	/*	calculated need for alternates.				*/
	vtoc.v_part[ALTSCTRSLICE].p_start = pstart;
	vtoc.v_part[ALTSCTRSLICE].p_flag  = V_VALID | V_UNMNT;
	vtoc.v_part[ALTSCTRSLICE].p_tag   = V_ALTSCTR;
	vtoc.v_part[ALTSCTRSLICE].p_size  = altsec_size;
	pstart += altsec_size;
	ap->ap_flag |= ALTS_ADDPART;
}

exec_command(buf)
char	*buf;
{
	int	len = strlen(buf);

	if(execfd >= 0) {
		if(write(execfd, buf, len) != len ||
		   write(execfd, "\n", 1) != 1)
				return(1);
		return(0);
		}
	return(system(buf));
}

/* this routine reads in the available fs types from /etc/default/fstyp
 * and /etc/default/"file" where "file" contains the file system specific
 * identifiers required by disksetup.
 */
void
rd_fs_defaults()
{
int i;
FILE  *fd;
char *p, *s;
char buf[20];

	/* read in the available file system types */
	i=0;
	if ((fd = defopen(DEF_FILE)) != NULL) {
		if ((p = defread(fd, "FSTYP")) == NULL || 
		   strcpy(fsnamelist, p) == 0 ||
		   (s = strtok(p, ",")) == (char *)NULL) {
			fprintf (stderr,"disksetup: FSTYP identifier invalid or not specified in %s%s\n", DEF_DIR, DEF_FILE);  
			exit(-1);
		}

		while(s != NULL && fstyp_cnt < MAXFS) {
			strncpy(fstyp[fstyp_cnt++].fsname, s, MAXNAME);
			s = strtok(NULL, ",");
		}

		/* check whether MAX file system types was exceeded */ 
		if (fstyp_cnt+1 == MAXFS) {
			fprintf(stderr,"disksetup: Too many fs types specified in %s%s\n", DEF_DIR, DEF_FILE);
			exit(-1);
		}

		/* read the boot fs type in */
		if ((p = defread(fd, "BOOT_FSTYP")) != NULL) {
			strncpy(fstyp[fstyp_cnt].fsname, p, MAXNAME);
			fstyp[fstyp_cnt++].flag |= BOOTFSTYPE;
		} else {
			printf("disksetup: BOOT_FSTYP identifier not specified in %s%s\n", DEF_DIR, DEF_FILE);
			exit(-1);
		}
		defclose(fd);
		/* read in file system specific identifiers */
		for (i=0;i<fstyp_cnt;i++) {
			if ((fd = defopen(fstyp[i].fsname)) == NULL) {
				fprintf(stderr,"disksetup: failed to open %s%s\n",
					DEF_DIR, fstyp[i].fsname); 
				exit(-1);
			}
			if ((p = defread(fd, "BLKSIZE")) != NULL ||
				*p != '\0') {
				int cnt = 0;
				s = strtok(p, ",");
				if (s == NULL) {
					fprintf(stderr,"disksetup: BLKSIZE identifer not specified in %s%s\n",
						DEF_DIR, fstyp[i].fsname);
					exit(-1);
				}
				while (s != NULL) {
					int blksz = atol(s);
					if ((blksz % 512) != 0) {
						fprintf(stderr, "disksetup: bad block size identfier specified in %s%s\n",
						DEF_DIR, fstyp[i].fsname);
						fprintf(stderr, "file system type ignored\n");
						strcpy(fstyp[i].fsname, "BADFSENTRY");
						cnt = 0;
						break;
					} 
					fstyp[i].blksize[cnt++] = atol(s);
					s = strtok(NULL, ",");

				}
				fstyp[i].blksiz_cnt = cnt;
			}
			if ((p = defread(fd, "MNTOPT")) != NULL) 
				strncpy(fstyp[i].mntopt, p, MAXOPTCNT);

			else
				strcpy(fstyp[i].mntopt, "");

			if ((p = defread(fd, "MKFSTYP")) != NULL) 
				fstyp[i].mkfstyp = atoi(p);
			else {
				fprintf(stderr,"disksetup: MKFSTYP identifer not found in %s%s\n",
					DEF_DIR, fstyp[i].fsname);
				exit(-1);
			}

			if ((p = defread(fd, "LABELIT")) != NULL) {
				if (strcmp(p, "YES") == 0 ||
					strcmp(p, "yes") == 0)

					fstyp[i].labelit = TRUE;
				else
					fstyp[i].labelit = FALSE;
			} else
				fstyp[i].labelit = FALSE;

			defclose(fd);
		}

				

	} else {

		fprintf(stderr,"disksetup: open for fs default file failed %s%s\n",
			DEF_DIR, DEF_FILE);
		exit(-1);

	}

}

/* this routine returns the file system default array
 * index for p 
 */
int
find_fs_defaults(p, flag)
char *p;
int flag;
{
int i;
	for(i=0;i<fstyp_cnt;i++){
		if (flag & BOOTFSTYPE && fstyp[i].flag & BOOTFSTYPE) 
			return(i);
		if (strcmp(p, fstyp[i].fsname) == 0)
			return(i);
	}
	return(-1);
			
}

/* give choices of available blksizes */
int
get_fs_blksize(indx)
int indx;
{
char buf[60] = {0};
char tmp[20] = {0};
int i, j;
int size;

	if (fstyp[indx].blksiz_cnt == 1)
		return(fstyp[indx].blksize[0]);
	for(i=0;i<fstyp[indx].blksiz_cnt;i++) {
		sprintf(tmp, "%d, ", fstyp[indx].blksize[i]);
		strcat(buf, tmp);
	}
	buf[strlen(buf)-2]  = '\0';	/* get rid of last comma */
	for(;;) {
		/* flush input prior to prompt -- prevent typeahead */
		/* note that we treat tcflush as void -- may not
		 * succeed because input is from file
		 */
		(void) tcflush(0,TCIFLUSH);
		printf("\nSpecify the block size from the the following list\n");
		printf("(%s), or press <ENTER> to use the first one:", buf); 
		gets(replybuf);

		/* use the default blocksize */
		if (strcmp(replybuf, "") == 0)
			return(fstyp[indx].blksize[0]);

		size = atol(replybuf);
		for(j=0;j<fstyp[indx].blksiz_cnt;j++)
			if (size == fstyp[indx].blksize[j])
				return(size);
		printf("\nInvalid response:  block size specified was '%s'\n", replybuf);
	}
}
