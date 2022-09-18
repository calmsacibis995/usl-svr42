/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fdisk:i386at/cmd/fdisk/fdisk.c	1.16"

/*
*	FILE:	fdisk.c 
*	Description:
*		This file will read the current Partition table on the
*		given device and will read the drive parmeters. 
*		The user can then select various operations from a
*		supplied menu.
*/

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/fdisk.h>
#include <sys/termios.h>
#include <sys/sysi86.h>

/* Values for MINUNIX_DSK0/MINUNIX_DSK1 are rounded down
 * 1MB to workaround percentage rounding problems.
 * NOTE: Size is in 512-byte units!
 */
#define ABSMINUNIX_DSK0 102400L /* 50Mb Absolute Min UNIX partition size for
				 * 1st disk 
				 */
#define MINUNIX_DSK0 120832L  /* Recommended min UNIX partition size for 1st
			       * disk is 60 MB, set to 59 MB for rounding 
			       * purpose.
			       */
#define MINUNIX_DSK1 79872L   /* Min UNIX partition size for 2nd disk
			       * is 40 MB, set to 39 MB for rounding
			       * purpose.
			       */
#define CLR_SCR "[1;1H[0J"
#define CLR_LIN "[0K"
#define HOME "[1;1H[0K[2;1H[0K[3;1H[0K[4;1H[0K[5;1H[0K[6;1H[0K[7;1H[0K[8;1H[0K[9;1H[0K[10;1H[0K[1;1H"
#define Q_LINE "[22;1H[0K[21;1H[0K"
#define W_LINE "[12;1H[0K[11;1H[0K"
#define E_LINE "[24;1H[0K[23;1H[0K"
#define M_LINE "[13;1H[0K[14;1H[0K[15;1H[0K[16;1H[0K[17;1H[0K[18;1H[0K[19;1H[0K[13;1H"
#define T_LINE "[1;1H[0K"

char partsize_flag = 0;
char partsizeUsage[]= "Usage: partsize [-s] [-d] [raw-device]";
char psize_dflag = 0;
char psize_numparts = 0;
char psize_partstate = 0;
char update_boot = 0;

void partsize_unix_size();
void partsize_dump_parts();

typedef struct pseudo_ipart {
	unsigned long bootid;	/* bootable or not */
	unsigned long beghead;	/* beginning head, sector, cylinder */
	unsigned long begsect;	
	unsigned long begcyl;
	unsigned long systid;	/* OS type */
	unsigned long endhead;	/* ending head, sector, cylinder */
	unsigned long endsect;	
	unsigned long endcyl;	
	unsigned long relsect;	/* first sector relative to start of disk */
	unsigned long numsect;	/* number of sectors in partition */
} psize_part_t;

psize_part_t	psize_parts[FD_NUMPART];


char Usage[]= "Usage: fdisk [raw-device]";
char Ostr[] = "Other";
char Dstr[] = "pre-5.0DOS";
char DDstr[]= "DOS-DATA";
char EDstr[]= "EXT-DOS";
char HDstr[] = "DOS";
char Ustr[] = "UNIX System";
char Actvstr[] = "Active";
char NAstr[]   = "      ";

struct absio absbuf;
struct mboot *Bootblk;

char	*Bootsect;
char	*Nullsect;

unsigned char Bootcod[] = {
0x33, 0xc0, 0xfa, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0x8e, 0xc0, 0x8e, 0xd8, 0xfb, 
0x8b, 0xf4, 0xbf, 0x00, 0x06, 0xb9, 0x00, 0x02, 0xfc, 0xf3, 0xa4, 0xea, 
0x1d, 0x06, 0x00, 0x00, 0xb0, 0x04, 0xbe, 0xbe, 0x07, 0x80, 0x3c, 0x80, 0x74, 
0x0c, 0x83, 0xc6, 0x10, 0xfe, 0xc8, 0x75, 0xf4, 0xbe, 0xbd, 0x06, 0xeb, 
0x43, 0x8b, 0xfe, 0x8b, 0x14, 0x8b, 0x4c, 0x02, 0x83, 0xc6, 0x10, 0xfe, 0xc8, 
0x74, 0x0a, 0x80, 0x3c, 0x80, 0x75, 0xf4, 0xbe, 0xbd, 0x06, 0xeb, 0x2b, 
0xbd, 0x05, 0x00, 0xbb, 0x00, 0x7c, 0xb8, 0x01, 0x02, 0xcd, 0x13, 0x73, 0x0c, 
0x33, 0xc0, 0xcd, 0x13, 0x4d, 0x75, 0xef, 0xbe, 0x9e, 0x06, 0xeb, 0x12, 
0x81, 0x3e, 0xfe, 0x7d, 0x55, 0xaa, 0x75, 0x07, 0x8b, 0xf7, 0xea, 0x00, 0x7c, 
0x00, 0x00, 0xbe, 0x85, 0x06, 0x2e, 0xac, 0x0a, 0xc0, 0x74, 0x06, 0xb4, 
0x0e, 0xcd, 0x10, 0xeb, 0xf4, 0xfb, 0xeb, 0xfe, 0x4d, 0x69, 0x73, 0x73, 0x69, 
0x6e, 0x67, 0x20, 0x6f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6e, 0x67, 
0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 0x00, 0x45, 0x72, 0x72, 0x6f, 0x72, 
0x20, 0x6c, 0x6f, 0x61, 0x64, 0x69, 0x6e, 0x67, 0x20, 0x6f, 0x70, 0x65, 
0x72, 0x61, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x73, 0x79, 0x73, 0x74, 0x65, 0x6d, 
0x00, 0x49, 0x6e, 0x76, 0x61, 0x6c, 0x69, 0x64, 0x20, 0x70, 0x61, 0x72, 
0x74, 0x69, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x74, 0x61, 0x62, 0x6c, 0x65, 0x00, 
0x41, 0x75, 0x74, 0x68, 0x6f, 0x72, 0x20, 0x2d, 0x20, 0x53, 0x69, 0x65, 
0x67, 0x6d, 0x61, 0x72, 0x20, 0x53, 0x63, 0x68, 0x6d, 0x69, 0x64, 0x74, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
struct disk_parms Drv_parm;
struct ipart	Table[FD_NUMPART];
struct ipart Old_Table[FD_NUMPART];
int Numcyl;
ulong Numsectors;
int Dev;
int disk_size;
int disk_selected;
daddr_t minunix;
int E333flag = 0;
int dos_size = 0;
int dosprcnt = 0;
char Dfltdev[] = "/dev/rdsk/c0t0d0s0";	/* default fixed disk drive */
#define CBUFLEN 80
char s[CBUFLEN];

/** The following has been added for SCSI support **/
/** diskstr and diskstrU are used for messages    **/

char diskstrU[7]; /** either "SECOND" or "SCSI" **/
char diskstr[7]; /** either "second" or "SCSI" **/
int unixend;

char *basename();
int d_flag, s_flag;
int I_flag; /* set when fdisk is called from diskadd or install scripts */

void
process_partsize_opts(argc, argv)
int *argc;
char **argv[];
{
	int c, i, j;
	unsigned char *tblptr;
	int dosend, unixstart;
 	char	*p;

	d_flag = s_flag = I_flag = 0;

 	if(*argc == 1 )
		return;

	for (i=0; i<FD_NUMPART; i++) {
	       	psize_parts[i].bootid = 0;
	       	psize_parts[i].systid = UNUSED;
	       	psize_parts[i].beghead = 0;
	       	psize_parts[i].begsect = 0;
	       	psize_parts[i].begcyl = 0;
	       	psize_parts[i].endhead = 0;
	       	psize_parts[i].endsect = 0;
	       	psize_parts[i].endcyl = 0;
	       	psize_parts[i].relsect = UNUSED;
	       	psize_parts[i].numsect = UNUSED;
	}

	while(*argc > 1 && (*argv)[1][0] == '-') {
 		p = &(*argv)[1][1];
 		while(*p != '\0'){
		   switch (*p) {
			default: /* show options and exit */
     				fprintf(stderr,"%s\n",partsizeUsage);
     				exit(1);
 			case 'd': /* dump fdisk table */
 				d_flag ++;
				break;
 			case 's': /* size of disk in Mb */
 				s_flag ++;
				break;
		   }
		   p++;
		}
		*argc = *argc - 1;
		*argv = *argv + 1;
	}
	if ((s_flag && d_flag)) {
		fprintf(stderr,"partsize: only one option can be used.\n");
     		fprintf(stderr,"%s\n",partsizeUsage);
     		exit(1);
	}
}


main(argc,argv)
int argc;
char *argv[];
{
	int c, i, j;
	unsigned char *tblptr;
	int dosend, unixstart;
 	char	*p;

	p = basename(argv[0]);

	if (!strcmp(p,"partsize")) {
		partsize_flag++;
		process_partsize_opts(&argc, &argv);
	} else {
		/* If this gets extended any further then getopt
		 * should be used. This is an undocumented option
		 * used during installation to suspress some output.
		 */
		if ( argc > 1 && argv[1][0] == '-' && argv[1][1] == 'I') {
			I_flag++;	 /* Install silent option  */
			argv = argv + 1;  /* increment argv to point to device arg  */

		} else if (argc > 1 && argv[1][0] == '-' && argv[1][1] == '?') {
			fprintf(stderr, "%s\n", Usage);
			exit(1);

		} else if (argc > 3) {
			fprintf(stderr, "%s\n", Usage);
			exit(1);
		}
		
			
	}
	if ((!I_flag && argc == 1) || (I_flag && argc == 2))  {
            if ( (Dev = open(Dfltdev, O_RDWR|O_CREAT, 0666)) == -1){
	        fprintf(stderr,"%s: Default device (%s) cannot be opened\n",p,Dfltdev);
	        exit(1);
	    }
        } else if ( (Dev = open(argv[1],O_RDWR) ) == -1 ) {
		if ((strcmp("/dev/rdsk/1s0",argv[1]) == 0 ||
			strcmp("/dev/rdsk/c0t1d0s0") == 0) 
			&& errno == ENXIO) {

			fprintf (stderr,"The second disk is not installed on your system.\n");
			fprintf (stderr,"Use this command to create partitions on the second\n");
			fprintf (stderr,"disk after it has been installed.\n");
	       }
		else {
		fprintf(stderr,"%s: cannot open %s\n",p,argv[1]);
	    	if (partsize_flag) fprintf(stderr,"%s\n",partsizeUsage);
		else fprintf(stderr,"%s\n",Usage);
		fprintf(stderr,"\nMake sure that the disk is properly installed\n");
		}
	    exit(1);
	}

	/** disk_selected is used to make sure the **/
	/** proper messages are displayed.	   **/

	if((!I_flag && argc == 1) || (I_flag && argc == 2))
	{
		disk_selected = 0;
	}
	else if (strcmp("/dev/rdsk/c0t0d0s0", argv[1]) == 0 ||
			strcmp("/dev/rdsk/0s0", argv[1]) == 0) {
		disk_selected = 0;
	}
	else {  /* default to secondary disk */
		disk_selected = 1;
		strcpy(diskstrU,"SECOND");
		strcpy(diskstr,"second");
	}

	if ( ioctl(Dev, V_GETPARMS, &Drv_parm,NULL) == -1 ) {
		fprintf(stderr,"%s: illegal device - %s\n",p,argv[1]);
	    	if (partsize_flag) fprintf(stderr,"%s\n",partsizeUsage);
		else fprintf(stderr,"%s\n",Usage);
		exit(1);
	}
	
	if ((Drv_parm.dp_type != DPT_WINI) &&
	   (Drv_parm.dp_type != DPT_SCSI_HD) &&
	   (Drv_parm.dp_type != DPT_SCSI_OD)) {
		fprintf(stderr,"%s: Can only be executed on a disk. \n",p);
		exit(1);
	}

	if ( sysi86(SI86BUSTYPE) == MCA_BUS && Drv_parm.dp_type != DPT_WINI ) {
		Numcyl = Drv_parm.dp_cyls - 3;
	} else {
		Numcyl = Drv_parm.dp_cyls;
	}
	Numsectors = Numcyl*Drv_parm.dp_heads*Drv_parm.dp_sectors;
	disk_size = ((long) Numsectors * (long) Drv_parm.dp_secsiz ) / (long) 1048576;;
	if (partsize_flag && s_flag) {
		printf("%ld\n",disk_size);
		exit (0);
	}

	if (disk_selected > 0)
		minunix = MINUNIX_DSK1;
	else {
		/* set minimum size for UNIX partition to MINUNIX_DSK0 if
		 * the hard disk size (not including the first cylinder) is at
		 * least MINUNIX_DSK0 + 1 track big.
		 * (Some systems require that the first track be
		 * unused by the hard disk. Hardware config info is kept there)

		 * If the disk size (minus the 1st track) is below
		 * ABSMINUNIX_DSK0, keep the min at ABSMINUNIX_DSK0 (don't
		 * allow a UNIX partition to be created. Note that
		 * installation should prevent this case from occurring,
		 * but we will be defensive...

		 * Otherwise, the minimum size partition will be the total
		 * disk space minus the space for the 1st track.
		 */

		/* minunix is in 512-byte units so it compares
		 * against MINDSK* constants.  Note that other parts of
		 * fdisk assume a 512-byte secsize and would break if
		 * secsiz was 1024. But let's make the new code
		 * as generic as possible by dividing secsiz by
		 * 512 (so if secsiz happens to be 1024, this code
		 * will work).  
		 */
		minunix = (unsigned long) (Numsectors - Drv_parm.dp_sectors) *
			      (unsigned long) Drv_parm.dp_secsiz /
			      512;

		if (minunix >= MINUNIX_DSK0)
			minunix=MINUNIX_DSK0;
		else if (minunix < ABSMINUNIX_DSK0)
			minunix=ABSMINUNIX_DSK0;
	}

	/*
	 * Allocate memory to hold 2 complete sectors:
	 * 1) The Boot sector i.e. Sector 0;
	 * 2) A sector's worth of zeros.
	 */
	Bootsect = (char *) malloc (2 * Drv_parm.dp_secsiz);
	if (Bootsect == NULL) {
		fprintf (stderr,"Fdisk: Unable to obtain %d bytes of temporary memory.\n",
			2*Drv_parm.dp_secsiz);
		exit(1);
	}

	Nullsect = Bootsect + Drv_parm.dp_secsiz;
	for (i=0; (ushort_t)i < Drv_parm.dp_secsiz; i++) {
		Nullsect[i] = 0;
	}

	/* read partition table from disk
	 * with new ioctl		*/
	absbuf.abs_sec = 0;
	absbuf.abs_buf = Bootsect;
	if ( ioctl(Dev, V_RDABS, &absbuf ) == -1) {
		fprintf(stderr,"%s: Error reading partition table\n",p);
		fprintf(stderr,"Possibly wrong device node, use a node that starts from cylinder 0\n");
		exit(1);
	}
	Bootblk = (struct mboot *) Bootsect;

	copytbl();

	if (partsize_flag && d_flag) {
	   partsize_dump_parts();
	   exit(0);
	}

	if (partsize_flag) {
	   partsize_unix_size();
	   exit(0);
	}

	if ( Table[0].systid == UNUSED ) {
		if ((sysi86(SI86RDID) == C4) && (sysi86(SI86RDBOOT) == 0))
			E333flag = 1;
	    	printf("The recommended default partitioning for your disk is:\n\n");
	 	if ((E333flag == 1) && ((disk_size - 4) > 40)) {
			dos_size = 4;
			dosprcnt = dos_size*100/disk_size;
			if (dosprcnt == 0)
				dosprcnt++;
	    		printf("    %d%% \"UNIX System\" -- lets you run UNIX System programs\n", 100-dosprcnt);
	    		printf("    %d%% \"DOS (v. 3.2 or later) only\" \n\n",dosprcnt);
		}
		else 

	    		printf("  a 100%% \"UNIX System\" partition. \n\n");
		/* flush stdin if tty, but don't check err val because
 		 * it might be that stdin is a file. Therefore, just
		 * make it a void call. This is to avoid typeahead.
		 */
		(void) tcflush(0, TCIFLUSH);
	    	printf("To select this, please type \"y\".  To partition your disk\n");
	    	printf("differently, type \"n\" and the \"fdisk\" program will let you\n");
	    	printf("select other partitions. ");
		gets(s);
		rm_blanks(s);
	    	while ( !( ((s[0] == 'y') || (s[0] == 'Y') ||
		        (s[0] == 'n') || (s[0] == 'N')) &&
			(s[1] == 0))) {
			printf(" Please answer \"y\" or \"n\": ");
			gets(s);
			rm_blanks(s);
	    	}
	    	if ( s[0] == 'y' || s[0] == 'Y' ) {
			/* Default scenario ! */
	        	nulltbl();
	        	unixend = Numcyl -1;
			if (E333flag > 0) {
				dosend = (Numcyl * dosprcnt) / 100 - 1;  
	        		if ( ((Numcyl * 10) % 100) > 50) 
					dosend++;
	        		unixstart = dosend + 1;
	        		Table[0].systid = DOSOS12; 	/* Dos  */
	        		Table[0].bootid = 0;	        /* InActive */
	        		Table[0].beghead = 1;
	        		Table[0].begsect = 1;
	        		Table[0].begcyl = 0;
	        		Table[0].endhead = Drv_parm.dp_heads - 1;
	        		Table[0].endsect = Drv_parm.dp_sectors | (char)((dosend >> 2) & 0x00c0);
	        		Table[0].endcyl = (char)(dosend & 0x00ff);
	        		Table[0].relsect = Drv_parm.dp_sectors; /* start on 2nd trk */
	        		Table[0].numsect = (long)(dosend + 1) * Drv_parm.dp_heads *
					 Drv_parm.dp_sectors - Drv_parm.dp_sectors;
				i = 1;

				/*
				 * Zero-out the new DOS partition.
				 */
				absbuf.abs_sec = Table[0].relsect;
				absbuf.abs_buf = Nullsect;
				if (ioctl(Dev, V_WRABS, &absbuf) == -1) {
					fprintf (stderr,"Fdisk: Error initializing DOS partition.\n");
					exit(1);
				}
			}
			else {
				i = 0;
				unixstart = 0;
			}
	        	/* now set up UNIX System partition */
	        	Table[i].systid = UNIXOS;   /* UNIX */
	        	Table[i].bootid = ACTIVE;
			if (unixstart == 0) {
	        		Table[i].beghead = 1;
	        		Table[i].begsect = 1;
				Table[i].relsect = Drv_parm.dp_sectors; /* start on 2nd trk */
				Table[i].numsect = (long)(Numsectors) - Drv_parm.dp_sectors;
			}
			else {
	        		Table[i].beghead = 0;
	        		Table[i].begsect = 1 | (char)((unixstart >> 2) & 0xc0);
	        		Table[i].relsect = (long)(unixstart * Drv_parm.dp_heads * (long) Drv_parm.dp_sectors);
	        		Table[i].numsect = (long)(Numcyl - unixstart) * Drv_parm.dp_heads * Drv_parm.dp_sectors;
			}
	        	Table[i].begcyl = (char)(unixstart & 0x00ff);
	        	Table[i].endhead = Drv_parm.dp_heads - 1;
	        	Table[i].endsect = Drv_parm.dp_sectors | (char)((unixend >> 2) & 0x00c0);
	        	Table[i].endcyl = (char)(unixend & 0x00ff);
		        for(j=i+1; j<FD_NUMPART; j++ ) {
	 	    		Table[j].systid = UNUSED; /* null out other partitions */ 
	 	    		Table[j].bootid = 0;
	        	}
	        	cpybtbl();
			absbuf.abs_sec = 0;
			absbuf.abs_buf = Bootsect;
	        	if (ioctl(Dev, V_WRABS, &absbuf) == -1) {
				fprintf(stderr,"Fdisk: error writing boot record\n");
				exit(1);
			}
	        	exit(0);
	    	}
	}
	printf(CLR_SCR);
	copytbl();
        cpyoldtbl();
	disptbl();
	/*
	printf(Q_LINE);
	printf("Enter Selection: ");
	*/
	while (1) {
	    	stage0(argv[1]);
		copytbl();
		disptbl();

	}
}

stage0(file)
char *file;
{
	dispmenu(file);
	while (1) {
	    printf(Q_LINE);
	    /* flush stdin if tty, but don't check err val because
 	     * it might be that stdin is a file. Therefore, just
	     * make it a void call. This is to avoid typeahead.
	     */
	    (void) tcflush(0, TCIFLUSH);
	    printf("Enter Selection: ");
	    gets(s);
	    rm_blanks(s);
 
	    while ( !((s[0] >= '0') && (s[0] < '6') && (s[1] == 0))) {
	    	printf(E_LINE); /* clear any previous error */
		printf("Please enter a one digit number between 0 and 5");
		printf(Q_LINE);
		/* flush stdin if tty, but don't check err val because
 		 * it might be that stdin is a file. Therefore, just
		 * make it a void call. This is to avoid typeahead.
		 */
		(void) tcflush(0, TCIFLUSH);
		printf("Enter Selection: ");
		gets(s);
		rm_blanks(s);
	    }
	    printf(E_LINE);
	    switch(s[0]) {
	    case '0':
		memcpy(Bootblk->bootinst, Bootcod, BOOTSZ);
		printf(E_LINE);
		printf("The system master boot code has been overwritten.\n");
		printf("You must \"Update\" for the change to take effect.");
		update_boot = 1;
		break;
	    case '1':
		if (pcreate() == -1)
		    return;
		break;
	    case '2':
		if (pchange() == -1)
		    return;
		break;
	    case '3':
		if (pdelete() == -1)
		    return;
		break;
            case '4':
		chk_ptable(0);  /* updates disk part. table if it has changed */
                close(Dev);
                exit(0);
	    case '5':
	   	close(Dev);
		exit(0);
	    default:
		break;
	    }
            cpybtbl();
	    disptbl();
	    dispmenu(file);
	}
}
pcreate()
{
unsigned char tsystid = 'z';
int i,j;
int startcyl, endcyl;

	if ( Table[3].systid != UNUSED ) {
		printf(E_LINE);
		printf("The partition table is full! \n");
		printf("you must delete an old partition before creating a new one\n");
		return(-1);
	}
	i = 0;
	j = 0;
	for (i=0; (i<FD_NUMPART) && (Table[i].systid != UNUSED); i++) {
		startcyl = (int)(((Table[i].begsect & 0xC0) << 2) + 
				 Table[i].begcyl);
		endcyl = (int)(((Table[i].endsect & 0xC0) << 2) + 
				 Table[i].endcyl);
		j = j + endcyl - startcyl + 1;
	}
	if (j >= Numcyl) {
		printf(E_LINE);
		printf("There is no more room on the disk for another partition.\n");
		printf("You must delete a partition before creating a new one.\n");
		return(-1);
	}
		
	while (tsystid == 'z') {
		/* flush stdin if tty, but don't check err val because
 		 * it might be that stdin is a file. Therefore, just
		 * make it a void call. This is to avoid typeahead.
		 */
		(void) tcflush(0, TCIFLUSH);
		printf(Q_LINE);
		printf("Indicate the type of partition you want to create\n");
		printf("  (1=UNIX System, 2=pre-5.0DOS, 3=DOS, 4=other, x=Exit). ");
		gets(s);
		rm_blanks(s);
		if (s[1] != 0) {
			printf(E_LINE);
	    		printf("Illegal selection, try again");
			continue;
		}
		switch(s[0]) {
		case '1':		/* UNIX System partition */
		    tsystid = UNIXOS;
		    break;
		case '2':	/* DOS partition */
		    tsystid = DOSOS12;	/* create 12 bit FAT partition */
		    break;	/* DOS 'format' changes this parameter */
		case '3':
		    tsystid = DOSHUGE;	/* post 4.0 DOS - support for >32mb
					 * partitions.
					 */
			break;
		case '4':	/* OTHER partition - potentially used for */
		    tsystid = OTHEROS;	/* DB application had ID 0 now 98 */
		    break;


		case 'x':		/* exit */
		case 'X':
		    printf(E_LINE);
		    return(-1);
		default:
		    printf(E_LINE);
		    printf("Illegal selection, try again.");
		    continue;
		}
	}
	printf(E_LINE);
	i = specify(tsystid);
	if ( i == -1 ) return(-1);
	/* if install mode make UNIX the Active partition */ 
	if (I_flag) {
		if (tsystid == UNIXOS) {
			for ( j=0; j<FD_NUMPART; j++)
				if ( j == i ) {
	    	    			Table[j].bootid = ACTIVE;
		    			printf(E_LINE);
		    			printf("Partition %d is now the Active partition",j+1);
				} else
	    	    			Table[j].bootid = 0;
		}
		return(1);
	}
	printf(E_LINE);
	printf(Q_LINE);


	/* flush stdin if tty, but don't check err val because
 	 * it might be that stdin is a file. Therefore, just
	 * make it a void call. This is to avoid typeahead.
	 */
	(void) tcflush(0, TCIFLUSH);
	if (disk_selected > 0) {
		printf("\nDo you want this to become the \"Active\" partition? \n");
		printf("UNIX partitions must be in the \"Active\" state to be accessible.\n");
		printf("Type \"y\" to mark the partition \"Active\" or \"n\". ");
	}
	else {
		printf("\nDo you want this partition to become the Active boot partition?\n");
		printf("UNIX partitions must be in the \"Active\" state to be accessible.\n");
		printf("Type \"y\" to mark the partition \"Active\" or \"n\". ");
	}
	gets(s);
	rm_blanks(s);
	while ( (s[1] != 0) &&
		((s[0] != 'y')&&(s[0] != 'Y')&&(s[0] != 'n')&&(s[0] != 'N')))
	{
	    printf(E_LINE);
	    /* flush stdin if tty, but don't check err val because
 	     * it might be that stdin is a file. Therefore, just
	     * make it a void call. This is to avoid typeahead.
	     */
	    (void) tcflush(0, TCIFLUSH);
	    printf(" Please answer \"y\" or \"n\": ");
	    gets(s);
	    rm_blanks(s);
	}
	printf(E_LINE);
	if (s[0] == 'y' || s[0] == 'Y' ) {
	    for ( j=0; j<FD_NUMPART; j++)
		if ( j == i ) {
	    	    Table[j].bootid = ACTIVE;
		    printf(E_LINE);
		    printf("Partition %d is now the Active partition",j+1);
		}
		else
	    	    Table[j].bootid = 0;
	}
	else
	    Table[i].bootid = 0;
	return(1);
}

specify(tsystid)
unsigned char tsystid;
{
	int	i, j,
		percent = -1;
	int	cyl, cylen, startcyl, endcyl, maxfree;
	long	sector, jsec;
	int	round = 0;

	printf(Q_LINE);
	/* flush stdin if tty, but don't check err val because
 	 * it might be that stdin is a file. Therefore, just
	 * make it a void call. This is to avoid typeahead.
	 */
	(void) tcflush(0, TCIFLUSH);
	if (tsystid == UNIXOS) 
		printf("The UNIX System partition requires at least %d%% of the disk.\n",
			(int) (minunix*100/(daddr_t)(Numsectors)+ 1));
	printf("Indicate the percentage of the disk you want this partition \n");
	printf("to use (or enter \"c\" to specify in cylinders). ");
	gets(s);
	rm_blanks(s);
	if ( s[0] != 'c' ){	/* specifying size in percentage of disk */
	    i=0;
	    while(s[i] != '\0') {
		if ( s[i] < '0' || s[i] > '9' ) {
		    printf(E_LINE);
		    printf("Illegal Percentage value specified\n");
		    printf("Please re-create the partition");
		    return(-1);
		}
		i++;
		if ( i > 3 ) {
		    printf(E_LINE);
		    printf("Illegal Percentage value specified\n");
		    printf("Please re-create the partition");
		    return(-1);
		}
	    }
	    if ( (percent = atoi(s)) > 100 ) {
		printf(E_LINE);
		printf("Percentage specified is too large, enter a value between %d and 100\n", (minunix*100/(daddr_t)(Numsectors)+ 1));
		printf("Please re-create the partition");
		return(-1);
	    }
	    if (percent < 1 ) {
		printf(E_LINE);
		printf("Percentage specified is too small, enter a value between %d and 100\n", (minunix*100/(daddr_t)(Numsectors)+ 1));
		printf("Please re-create the partition");
		return(-1);
	    }
            cylen = (Numcyl * percent) / 100;
	    if (((Numcyl * percent) % 100) > 50)
			round = 1;

	    /* Make sure DOS partition is within 32MB range (65536 cyl) */
	    if (tsystid == DOSOS12){
		/* Subtracting 1 cylinder may get us in range */
		if ((long)(cylen*Drv_parm.dp_heads*Drv_parm.dp_sectors) >
			(long)MAXDOS && (long)((--cylen)*Drv_parm.dp_heads
			*Drv_parm.dp_sectors) > (long)MAXDOS) {

			int n;
			int remainder;
			n =(int)(MAXDOS*100/(int)(Drv_parm.dp_heads*
				Drv_parm.dp_sectors)/Numcyl);
			if ((remainder = ((int)MAXDOS*100/(int)(Drv_parm.dp_heads*
				Drv_parm.dp_sectors)%(int)Numcyl)) != 0) 
				if ((remainder * 100 / Numcyl) > 50)
				/* round up */
					n++;
			printf(E_LINE);
			printf("Maximum size for a DOS partition is %d%%.\n",
				n <= 100 ? n : 100);
			printf("Please re-create the partition");
			return(-1);
	    	}
		else if (round && ((long)((cylen+1)*Drv_parm.dp_heads*
			Drv_parm.dp_sectors) <= (long)MAXDOS))

			cylen++;
	    } else
			/* round up for non-DOS partition */
			if (round)
				cylen++;


	    /* determine if large enough for minimum UNIX System partition */
	    if ((tsystid == UNIXOS) && (percent < (int) (minunix*100/(daddr_t)(Numsectors) + 1 ))) {
		printf(E_LINE);
		printf("Minimum size for UNIX System partition is %d%%.\n", (int)(minunix*100/(daddr_t)(Numsectors)+ 1));
		printf("Please re-create the partition");
		return(-1);
	    }
	    endcyl = -1;
	    maxfree = 0;
	    for ( j=0; j<FD_NUMPART; j++) {
		if ( Table[j].systid == UNUSED )
		    break;
	        startcyl = (int)(((Table[j].begsect & 0xC0)*4)+Table[j].begcyl);
		if (j == 0 && startcyl == 0){
	        	endcyl = (int)(((Table[j].endsect & 0xC0)*4)+Table[j].endcyl);
			continue;
		}
		if (((startcyl - endcyl) > cylen ) ||
		   ((endcyl == -1) && ((startcyl*100+Numcyl-1)/Numcyl >= percent)) ||
		   ((endcyl != -1) && (((startcyl-endcyl)*100+Numcyl-1)/Numcyl >= percent)))  {
		    /* space for partition here */
		    cyl = endcyl+1;
		    endcyl = endcyl + cylen;
		    if (endcyl >= startcyl) {
			endcyl = startcyl -1;
			cylen = endcyl - cyl + 1;
		    }
		    for ( i=3; i>j; i-- )
			Table[i] = Table[i-1];
		    i = j;
		    j = 4;
		    break;
		    }
		if ( (startcyl - endcyl) > maxfree )
		    maxfree = startcyl - endcyl -1;
	        endcyl = (int)(((Table[j].endsect & 0xC0)*4)+Table[j].endcyl);
	    }
	    if ( j < FD_NUMPART ) {
		i = j;
                /* if % requested is roundable to remaining cyls give them all */
                if ((((Numcyl-endcyl-1)*1000/Numcyl+5)/10) == percent)
			cylen = (Numcyl - endcyl) - 1;
		if ( (Numcyl - endcyl) > cylen ) {
		    cyl = endcyl + 1;
		    endcyl = endcyl + cylen;
		} else {
		    if ( (Numcyl - endcyl) > maxfree )
			maxfree = Numcyl - endcyl -1;
		    printf(E_LINE);
		    printf("Partition defined is too large - Maximum size available is %d%%\n",((maxfree*1000/Numcyl+5)/10));
		    printf("Please re-create the partition");
		    return(-1);
		}
	    }
	} else {	/* specifying size in cylinders */
	    printf(E_LINE);
	    printf(Q_LINE);
	    printf("Enter starting cylinder number: ");
	    if ( (cyl = getcyl()) == -1 ) {
		printf(E_LINE);
		printf("Illegal number, please re-create the partition");
	        return(-1);
	    }
	    if (cyl >= (unsigned int)Numcyl) {
		printf(E_LINE);
		printf("Cylinder %d out of bounds, maximum is %d\n", 
		       cyl,Numcyl - 1);
		return(-1);
	    }
	    for (i=0; i<FD_NUMPART; i++) {
	        if ( Table[i].systid == UNUSED ) {
		    break; /* no more partitions allocated */
	        }
	        startcyl = (int)(((Table[i].begsect & 0xC0)*4)+Table[i].begcyl);
	        endcyl = (int)(((Table[i].endsect & 0xC0)*4)+Table[i].endcyl);
	        if ( cyl < startcyl ) {
		    for ( j=3; j>i; j--)
		    {
		        Table[j] = Table[j-1];
		    }
		    break; /* before this one */
	        }
	        if ( (cyl>=startcyl) && (cyl<=endcyl) ) {
			printf(E_LINE);
			printf("Partition cannot be created because it overlaps with an existing partition.\n");
			printf("Please re-create the partition");
			return(-1);
	        }
	    }
	}
	/* cyl is starting cylinder */
	if (cyl >= 1024) {
		printf(E_LINE);
		printf("Starting cylinder of a partition must be less than 1024\n");
		printf("Please re-create the partition");
		return(-1);
	}
	sector = cyl*Drv_parm.dp_heads*Drv_parm.dp_sectors;
	if ( cyl == 0 ) {
            Table[i].beghead = 1;    /* if part. on cyl 0 start on 2nd track */
            Table[i].begsect = 1;
	    Table[i].relsect = Drv_parm.dp_sectors;
	}
	else {
	    Table[i].beghead = 0;
	    Table[i].begsect = 1;
	    Table[i].relsect = (long)(cyl * Drv_parm.dp_heads*Drv_parm.dp_sectors);
	}
	Table[i].systid = tsystid;
	Table[i].endhead = Drv_parm.dp_heads-1;
	Table[i].endsect = Drv_parm.dp_sectors;
	Table[i].begcyl = (char)cyl;
	startcyl = cyl;
	cyl=(cyl>>8)<<6;
	Table[i].begsect |= (char)cyl;
	if ( percent == -1 ) {
	    printf(Q_LINE);
	    printf("Enter partition size in cylinders: ");
	    if ( (cylen = getcyl()) == -1 ) {
		printf(E_LINE);
		printf("Illegal number, please re-create the partition");
	        return(-1);
	    }
	    /* determine if large enough for minimum UNIX System partition */
	    if ((tsystid == UNIXOS) && (long)((long)cylen*Drv_parm.dp_heads*Drv_parm.dp_sectors) < minunix ) {
		printf(E_LINE);
		printf("Minimum size for UNIX System partition is %d cylinders.\n",
			minunix/(daddr_t)(Drv_parm.dp_heads*Drv_parm.dp_sectors) + 1);
		printf("Please re-create the partition");
		return(-1);
	    }
	    cyl = startcyl + cylen -1;
	    if ( i < 3 ) {
		if ( Table[i+1].systid == UNUSED ) {
		    /* last partition */
		    endcyl = Numcyl-1;
		} else {
	           endcyl = (int)(((Table[i+1].begsect & 0xC0)*4)+Table[i+1].begcyl) -1;
		}
	        if (cyl > endcyl) {
		    int percent;

		    percent = ((endcyl-startcyl)*1000/Numcyl+5)/10;
	            printf(E_LINE);
	            printf("Partition defined is too large - Maximum size available is %d cylinders (%d%%)",endcyl-startcyl+1,percent);
	            return(-1);
	        }
	    }
	    else {
	        if ( cyl >= Numcyl ) {
		    int percent;

		    percent = ((Numcyl - startcyl)*1000/Numcyl+5)/10;
	            printf(E_LINE);
	            printf("Partition defined is too large - Maximum size available is %d cylinders (%d%%)",Numcyl - startcyl, percent);
	            return(-1);
	        }
	    }
	    endcyl = cyl;
	}
	Table[i].endcyl = (char)endcyl;
	endcyl=(endcyl>>8)<<6;
	Table[i].endsect |= (char)endcyl;
	if (cylen < 3 ) {
	    printf(E_LINE);
	    printf("Partition too small - Minimum size is 3 cylinders.");
	    return(-1);
	}	
	Table[i].numsect = (long) cylen * Drv_parm.dp_heads * Drv_parm.dp_sectors;
	if ( startcyl == 0 )
	    Table[i].numsect -= Table[i].relsect;
	return(i);
}

getcyl()
{
int slen, i, j;
unsigned int cyl;
	/* flush stdin if tty, but don't check err val because
 	 * it might be that stdin is a file. Therefore, just
	 * make it a void call. This is to avoid typeahead.
	 */
	(void) tcflush(0, TCIFLUSH);
	gets(s);
	rm_blanks(s);
	slen = strlen(s);
	j = 1;
	cyl = 0;
	for ( i=slen-1; i>=0; i--) {
		if ( s[i] < '0' || s[i] > '9' ) {
			return(-1);
		}
		cyl += (j*(s[i]-'0'));
		j*=10;
	}
	return(cyl);
}
disptbl()
{
	int i;
	unsigned int startcyl, endcyl, length, percent, remainder;
	char *stat, *type;
	unsigned char *t;
	float mb_size;
	unsigned long totsec = ((ulong) Numcyl*(ulong)Drv_parm.dp_heads*(ulong)Drv_parm.dp_sectors); 

	/* get total disk size in MB's */
	mb_size = (float)totsec/(float)2048;
	printf(HOME);
	printf(T_LINE);
	printf("             Total disk size is %d cylinders (%4.1f MB)\n\n", Numcyl, mb_size);
	printf("                                        Cylinders                Approx\n");
	printf("Partition   Status       Type       Start   End   Length    %%      MB\n");
	printf("=========   ======    ===========   =====   ===   ======   ===   ======");
	for ( i=0; i<FD_NUMPART; i++) {
		if ( Table[i].systid == UNUSED ) {
		    	printf("\n");
			printf(CLR_LIN);
			continue;
		}
		if ( Table[i].bootid == ACTIVE )
		    stat = Actvstr;
		else
		     stat = NAstr;
		switch(Table[i].systid) {
		case UNIXOS:
		     type = Ustr;
		     break;
		case DOSOS12:
		case DOSOS16:
		     type = Dstr;
		     break;
		case EXTDOS:
		     type = EDstr;
		     break;
		case DOSDATA:
		     type = DDstr;
		     break;
		case DOSHUGE:
		     type = HDstr;
		     break;
		default:
		     type = Ostr;
		     break;
		}
		t = &Table[i].bootid;
	        startcyl = (int)(((Table[i].begsect & 0xC0)*4)+Table[i].begcyl);
		length = Table[i].numsect / (long)(Drv_parm.dp_heads * Drv_parm.dp_sectors);
		if (Table[i].numsect % (long)(Drv_parm.dp_heads * Drv_parm.dp_sectors))
			length++;
	        endcyl =  startcyl + length - 1;
		percent = length * 100 / Numcyl;
		if ((remainder = (length*100 % Numcyl)) != 0) {
			if ((remainder * 100 / Numcyl) > 50) {
				/* round up */
				percent++;
			}
			/* ELSE leave percent as is since it's 
				already rounded down */
		}
		/* get sector size in MB's. */
		mb_size = (float)Table[i].numsect/(float)2048;
			
	        printf("\n    %d       %s    %-11.11s   %4d   %4d    %4d    %3d    %4.1f",
			i+1, stat, type, startcyl, endcyl, length,  percent, mb_size);
	}
	/* print warning message if table is empty */
	if (Table[0].systid == UNUSED) {
		printf(W_LINE);
		printf("THERE ARE NO PARTITIONS CURRENTLY DEFINED");
	}
	else {
		/* clr the warning line */
		printf(W_LINE);
	}
}

/* This function copies Table into Old_Table. It copies only systid, numsect,     */
/* relsect and bootid because these are the only fields needed for comparing     */
/* to determine if Table changed.                                                */

cpyoldtbl()
{
int i;
	for (i=0; i<FD_NUMPART; i++)  {
	    Old_Table[i].systid = Table[i].systid;
	    Old_Table[i].numsect = Table[i].numsect;
	    Old_Table[i].relsect = Table[i].relsect;
	    Old_Table[i].bootid = Table[i].bootid;
        }
}

nulltbl()
{
int i;
	for (i=0; i<FD_NUMPART; i++)  {
	    Table[i].systid = UNUSED;
	    Table[i].numsect = UNUSED;
	    Table[i].relsect = UNUSED;
	    Table[i].bootid = 0;
        }
}

/* This function copies the bytes from the boot record to an internal */
/* table. The entries used are shifted to the end - (see cpybtbl) */
/* - all unused are padded with zeros starting at offset 446.	  */
copytbl()
{
	int i, j;
	char *bootptr, *temp_ptr;
	unsigned char *tbl_ptr;
	int tblpos;

	tbl_ptr = &Table[0].bootid;
	bootptr = Bootblk->parts;	/* start of partition table */
	if (Bootblk->signature != MBB_MAGIC)  {
		/* signature is missing */
		nulltbl();
		memcpy(Bootblk->bootinst, Bootcod, BOOTSZ);
		return;
	}
	/* 
	 * When DOS fdisk deletes a partition, it is not recognized
	 * by the old algorithm.  The algorithm that follows looks
	 * at each entry in the Bootrec and copies all those that
	 * are valid.
	 */
	j=0;
	for (i=0; i<FD_NUMPART; i++) {
            temp_ptr = bootptr;
	    if((*temp_ptr == 0) && (*(++temp_ptr) == 0) && (*(++temp_ptr) == 0)) {
		/* null entry */
		bootptr += sizeof(struct ipart);
   	    }
	    else {
		Table[j] = *(struct ipart *)bootptr;
		j++;
		bootptr += sizeof(struct ipart);
	    }
	}
	for (i=j; i<FD_NUMPART; i++) {
	    Table[i].systid = UNUSED;
	    Table[i].numsect = UNUSED;
	    Table[i].relsect = UNUSED;
	    Table[i].bootid = 0;
	}

}

/* This function copies the table into the 512 boot record. */
/* Note that the entries unused will always be the last     */
/* ones and they are marked with 100 in sysind.		    */
/* The entries that are used are pushed to the end of the   */
/* record ( MSDOS seems to require this - although it is    */
/* not documented ) and the the unused portion of the table */
/* is padded with zeros in the bytes preceding the used     */
/* entries.						    */
cpybtbl()
{
	int i, j;
	char *boot_ptr, *parts_ptr;
	unsigned char *tbl_ptr;

	j = 0;
	for ( i=0; i<FD_NUMPART; i++)
	    if ( Table[i].systid == UNUSED )
		j++;
	for (parts_ptr = Bootblk->parts; parts_ptr < Bootblk->parts +(j*sizeof(struct ipart)); parts_ptr++)
	*parts_ptr = 0;
	tbl_ptr = &Table[0].bootid;
	for ( boot_ptr=parts_ptr; boot_ptr < (Bootblk->parts + (4*sizeof(struct ipart))); boot_ptr++) {
	    *boot_ptr = *tbl_ptr;
	    tbl_ptr++;
	}
	Bootblk->signature = MBB_MAGIC;
}

dispmenu(file)
char *file;
{
	printf(M_LINE);
	printf("SELECT ONE OF THE FOLLOWING: \n\n");
	printf("     0.   Overwrite system master boot code\n");
	printf("     1.   Create a partition\n");
	printf("     2.   Change Active (Boot from) partition\n");
	printf("     3.   Delete a partition\n");
	printf("     4.   Update (Update disk configuration and exit)\n");
	printf("     5.   Exit (Exit without updating disk configuration)");
}
pchange()
{
	char s[80];
	int i,j;

	while (1) {
		printf(Q_LINE);
		/* flush stdin if tty, but don't check err val because
 		 * it might be that stdin is a file. Therefore, just
		 * make it a void call. This is to avoid typeahead.
		 */
		(void) tcflush(0, TCIFLUSH);
		if (disk_selected > 0) {
			printf("Enter the number of the partition you want to access on the %s disk\n", diskstr);
			printf("(or enter 0 for none): ");
			}
		else {
			printf("Enter the number of the partition you want to boot from\n");
			printf("(or enter 0 for none): ");
		}
		gets(s);
		rm_blanks(s);
		if ( (s[1] != 0) || (s[0] < '0') || (s[0] > '4') ) {
			printf(E_LINE);
			printf("Illegal response, please give a number between 0 and 4\n");
		}
		else {
			break;
		}
	}
	if ( s[0] == '0' ) {	/* no active partitions */
		for ( i=0; i<FD_NUMPART; i++) {
			if (Table[i].systid != UNUSED && Table[i].bootid == ACTIVE)
		    		Table[i].bootid = 0;
	    	}
	    	printf(E_LINE);
		if (disk_selected > 0) {
	    		printf("There is currently no Active partition on the %s disk, you will not\n",diskstr);
	    		printf("be able to access filesystems created there.");
			}
		else
	    		printf("There is currently no Active partition");
	    	return(0);
		}
	else {	/* user has selected a partition to be active */
	    	i = s[0] - '1';
	    	if ( Table[i].systid == UNUSED ) {
	        	printf(E_LINE);
	        	printf("Partition does not exist");
	        	return(-1);
	    	}
		/* a DOS-DATA or EXT-DOS partition cannot be active */
		else if ((Table[i].systid == DOSDATA) || (Table[i].systid == EXTDOS)) {
			printf(E_LINE);
			printf("A DOS-DATA or EXT_DOS partition may not be made active.\n");
			printf("Select another partition.");
			return(-1);
		}
	    	Table[i].bootid = ACTIVE;
	    	for ( j=0; j<FD_NUMPART; j++) {
			if ( j != i )
		    	Table[j].bootid = 0;
	    	}
	}
	printf(E_LINE);
	if (disk_selected > 0) {
		printf("Partition %d is now the accessible partition on the %s disk.\n",i+1, diskstr);
		}
	else {
		printf("Partition %d is now the Active partition. It will be activated when you reset\n",i+1);
		printf("your computer or turn it on again");
		}
	return(1);
}
pdelete()
{
	char s[80];
	int i,j;
	char pactive;

DEL1:	printf(Q_LINE);
	/* flush stdin if tty, but don't check err val because
 	 * it might be that stdin is a file. Therefore, just
	 * make it a void call. This is to avoid typeahead.
	 */
	(void) tcflush(0, TCIFLUSH);
	printf("Enter the number of the partition you want to delete\n");
	printf(" (or enter x to exit ): ");
	gets(s);
	rm_blanks(s);
	if ( (s[0] == 'x') || (s[0] == 'X') ) {	/* exit delete cmd */
		printf(E_LINE);	/* clr error msg */
		return(1);
	}
	/* accept only a single digit between 1 and 4 */
	if (s[1] != 0 || (i=atoi(s)) < 1 || i > FD_NUMPART) {
		printf(E_LINE);
		printf("Illegal response, try again\n");
		goto DEL1;
	}
	else {		/* found a digit between 1 and 4 */
		--i;	/* structure begins with element 0 */
	}
	if ( Table[i].systid == UNUSED ) {
		printf(E_LINE);
		printf("Partition %d does not exist.",i+1);
		return(-1);
	}
	while (1) {
		/* flush stdin if tty, but don't check err val because
 		 * it might be that stdin is a file. Therefore, just
		 * make it a void call. This is to avoid typeahead.
		 */
		(void) tcflush(0, TCIFLUSH);
		printf(Q_LINE);
		printf("Do you want to delete partition %d?  This will erase all files and \n",i+1);
		printf("programs in this partition (type \"y\" or \"n\"). ");
		gets(s);
		rm_blanks(s);
		if ( (s[1] != 0) ||
		     ((s[0] != 'y') && (s[0] != 'n')))
		{
			printf(E_LINE);
			printf("Please answer \"y\" or \"n\"");
		}
		else break;
	}
	printf(E_LINE);
	if ( s[0] != 'y' && s[0] != 'Y' )
		return(1);
	if ( Table[i].bootid != 0 )
		pactive = 1;
	else
		pactive = 0;
	for ( j=i; j<3; j++ ) {
	    if(Table[j+1].systid == UNUSED) {
		Table[j].systid = UNUSED;
		break;
	    }
	    Table[j] = Table[j+1];
	}
	Table[j].systid = UNUSED;
	Table[j].numsect = UNUSED;
	Table[j].relsect = UNUSED;
        Table[j].bootid = 0;
	printf(E_LINE);
	printf("Partition %d has been deleted.",i+1);
	if ( pactive )
	    printf("  This was the active partition.");
	return(1);
}

rm_blanks(s)
char *s;
{
	register int i,j;

	for (i=0; i<CBUFLEN; i++) {
		if ((s[i] == ' ') || (s[i] == '\t'))
			continue;
		else 
			/* found 1st non-blank char of string */
			break;
	}
	for (j=0; i<CBUFLEN; j++,i++) {
		if ((s[j] = s[i]) == '\0') {
			/* reached end of string */
			return;
		}
	}
}
/* chk_ptable checks for any changes in the partition table. If there are any they */
/* are written out to the partition table on the disk. If a DOS               */
/* partition is created or changed the first sector of the partition is zeroed so  */
/* partition can be formatted.                                                     */

chk_ptable(silent)
int silent;
{
	int i, j, dos_chng, chng_flag, dos_add;
	
 	chng_flag = 0;
	dos_add = 0;	/* dos partition added */
	dos_chng = 0;
	for (i=0; i<FD_NUMPART; i++) {
		if ((Old_Table[i].systid != Table[i].systid) ||
			(Old_Table[i].relsect != Table[i].relsect) ||
			(Old_Table[i].numsect != Table[i].numsect))  {

			if ((Table[i].systid == DOSOS12) || 
				(Table[i].systid == DOSDATA) ||
				(Table[i].systid == DOSHUGE) ||
				(Table[i].systid == EXTDOS)) {
				/* check for identical partition in 
				 * another table loc due to deletes
				 */

				for (j=0; j<FD_NUMPART; j++) {
					if ((Old_Table[j].systid == Table[i].systid) &&
					   (Old_Table[j].relsect == Table[i].relsect) &&
					   (Old_Table[j].numsect == Table[i].numsect)) {
						/* same part. already present don't
						 * zero it.
						 */
						dos_chng = 1;
						break;
					}
				}

				if (dos_chng == 0) {
				/*
				 * Zero-out the new DOS partition.
				 */
					dos_add++;
					absbuf.abs_sec = Table[i].relsect;
					absbuf.abs_buf = Nullsect;
					if (ioctl(Dev, V_WRABS, &absbuf) == -1) {
						fprintf (stderr,"Fdisk: Error initializing DOS partition.\n");
						exit(1);
					}
				} else
					dos_chng = 0;
     	      		}

			/* partition changed so write it back to disk */
			chng_flag = 1;
		}
		if (Old_Table[i].bootid != Table[i].bootid)
			chng_flag = 1;
	}	/* end outer for */

        if ((chng_flag == 1) || (update_boot == 1)) {
	   cpybtbl();
		absbuf.abs_sec = 0;
		absbuf.abs_buf = Bootsect;
	        if (ioctl(Dev, V_WRABS, &absbuf) == -1) {
			fprintf(stderr,"Fdisk: error writing boot record\n");
			exit(1);
		}
		/* if update boot only no message needed */
        	if (chng_flag == 0) {
			exit(0);
		}
		if(!silent && !I_flag) {
			printf("If you have created or altered a partition, you must\n");
			printf("initialize the partition to reflect the new configuration.\n");	
			printf("For a Unix System partition run the disksetup(1m) command.\n");
			printf("For a DOS partition, run the DOS format command. Changes\n");
			printf("limited to the \"Active\" status field require\n");
			printf("no additional action.\n");
		} else
			if (!silent && I_flag && dos_add) {
				printf("You have changed or allocated a DOS partition.\n");
				printf("Run the DOS format command to initialize the partition.\n");
			}

        }
}

void
partsize_dump_parts()
{
	   int i;
	   printf("#TYPE\tHEADS\tCYLS\tSECS\tSECSZ\n");
	   printf("%u\t%u\t%u\t%u\t%u\n",
	       Drv_parm.dp_type,
	       Drv_parm.dp_heads,
	       Numcyl,
	       Drv_parm.dp_sectors,
	       Drv_parm.dp_secsiz);
	   printf("#ACTIVE\tOS\tBEGHD\tBEGSEC\tBEGCYL\tENDHD\tENDSEC\tENDCYL\tRELSEC\tNUMSEC\n");
	   for (i = 0; i < FD_NUMPART; i++) {
	      printf("%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u\n",
	          Table[i].bootid,
	          Table[i].systid,
	          Table[i].beghead,
	          Table[i].begsect&0x3F,
	          ((Table[i].begsect&0xC0)<<2)|Table[i].begcyl,
	          Table[i].endhead,
	          Table[i].endsect&0x3F,
	          ((Table[i].endsect&0xC0)<<2)|Table[i].endcyl,
	          Table[i].relsect,
	          Table[i].numsect);
	   }
}

void
partsize_unix_size()
{

	int i, unix_part, unix_size;

	unix_part = FD_NUMPART;
	for (i = 0; i<FD_NUMPART; i++) {
		if ((Table[i].systid == UNIXOS) && (Table[i].bootid == ACTIVE))
			unix_part = i;
	}
	if (unix_part == FD_NUMPART) {
		printf("0\n");
		exit(0);
	}
	unix_size = (Table[unix_part].numsect * (long)Drv_parm.dp_secsiz + 1048576/2) / 1048576;
	printf("%ld\n",unix_size);
}

