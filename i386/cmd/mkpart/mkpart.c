/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)mkpart:i386/cmd/mkpart/mkpart.c	1.1.3.2"
#ident	"$Header: mkpart.c 1.2 91/07/10 $"

/*
 *	Mkpart - includes support for the SCSI Support package
 *
 *	Mkpart is a maintenance program for UNIX System V.3 disks, disk vtocs,
 *	and partitions.  The program allows the user to list and update the
 *	volume table of contents (vtoc), partitions, alternate blocks
 *	table, write the boot block, OR format a given disk.
 *
 *	The command line specifies which disk device the program operates on,
 *	and what functions are to be performed.  The description of the device
 *	characteristics, the boot program (if any), and the partitions and their
 *	characteristics are obtained from the partition description file.  For
 * 	a description of the command line parameters, read giveusage().
 *
 *      The partition file is a parameterized english-like description of
 *	characteristics.  They are grouped together in named stanzas, and can
 *	reference other stanzas.  Their syntax is described by the YACC grammar
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fs/s5param.h>
#include <fcntl.h>
#include "mkpart.h"
#include "parse.h"
#include <sys/vtoc.h>
#include <sys/fdisk.h>

char		*devicestanza;		/* physical device stanza name */

static	char	bootonly = 0;		/* -[bB] flag	*/
char	format = 0;			/* -F format device flag */
char	initialize = 0;			/* -i flag	*/
char	intlv;				/* interleave factor from -F flag */
char	targ = 0;				/* -t subflag	*/
char	verify = 0;			/* -v flag	*/

char	*bootfile;			/* name of bootstrap file */
char	*partfile = PARTFILE;		/* default partition file */
static	char	*wpfile;		/* file to write for -x flag */

#define TF_ALTS         0x01            /* list alternates table */
#define TF_PARTS	0x04		/* list partition info */
#define TF_VTOC		0x08		/* list vtoc info */
#define TF_WPART        0x10            /* generate a partitions file */

FILE	*input;				/* partition file input stream */

int    	MaxUseDepth;			/* Limit for mutual USE checking */

extern devstanza 	*mydev;		/* Device stanza that we work on */
extern int	devfd;			/* ...and its file descriptor */

extern	struct pdinfo	pdinfo;		/* physical device info area */
struct vtoc	vtoc;			/* table of contents */

node	*addparts,			/* list of partitions to add */
	*subparts,			/* list of partitions to remove */
	*badsecs;			/* List of bad sectors to add */

/** The opening of the device was moved from getdevice() to a new **/
/** subroutine called mkpartopen(). When the dev is open it sets  **/
/** this flag (dev_fd_open) to 1. Additionally, mkpartclose will set the **/
/** the flag to 0 to indicate the device is closed.		  **/

int dev_fd_open = 0;

#define SCSIFORMAT "scsiformat"
#define SCSIVERIFY "scsiformat"
#define SCSIALTS   "scsihdfix"
#define SCSICMDS   "/etc/scsi"

/** "is_scsi_dev" is set to 1 if the device is a scsi **/
/** This is set in the getdevice() routine **/

int is_scsi_dev = 0;

/* the variable not_for_scsi is set whenever the command line
 * contains an option illegal for scsi implementation
 */
int	not_for_scsi	= 0;

void	updatealts();
void	verifydevice();

main(argc, argv)
int	argc;
char	**argv;
{
	symbol	*dev;
	int	hasvtoc = 0;
	int	hasboot = 0;
	char    options[] = "p:P:bB:f:t:F:A:ivVx:";
	int	c;
	extern	char	*optarg;
	extern	int	optind;

	initnodes();	/* init first allocation of parse nodes */

/*
 *	 Mkpart recognizes the following command line arguments:
 *
 *	 mkpart [-f file] {-p part}* {-P part}* [-b] [-B file] [-F interleave]
 *	      {-A sector}* [-i] [-v] [-t [vpab]] dev
 *	
 *	 Run with only dev specified, mkpart initializes the device with
 *	 the boot program (or none) given in dev, and a vtoc with one
 *	 partition that spans the disk.  By specifying one or more -P's,
 *	 more partitions can be added.
 *	
 *	 dev is a device stanza name in the partition file (see -f below).
 *		It specifies (and/or refers to other device stanzas that
 *		specify) all of the device characteristics.
 *	 -f specifies the partition and device specification stanza file.
 *		If not present, /etc/partitions is assumed.
 *	 -p removes a partition from the vtoc on the specified device.  It
 *		is removed by it's partition number; no comparisons are made
 *		by attribute.  Part is a stanza name that contains or refers
 *		to a stanza that contains a partno parameter.
 *	 -P adds a partition to the vtoc on the specified device.  Part is
 *		a stanza which contains and/or refers to other stanzas that
 *		contain all of the necessary parameters for a vtoc partition.
 *	 -b causes only the boot program to be updated, unless other options
 *		are specified.
 *	 -B specifies a different boot program than the one given by the
 *		device stanza.
 *	 -F causes the device to be formatted with specified interleave
 *	        factor. This flag cannot be used in conjuction with others.
 *		Care is taken to preserve bad sector/track information.
 *	 -t creates a listing of the current vtoc.  The sub parameters
 *		specify pieces to be printed: a - alternate sectors, b - bad
 *		sectors, p - partitions, and v - vtoc and related structures.
 *		The absence of sub parameters implies them all.  Sub params
 *		a and b are NOT CURRENTLY SUPPORTED.
 *	 -A adds the specified absolute sector to the alternates table
 *	        if there is a spare alternate sector to use for it, both
 *	        on the disk and in core (with ioctl).
 *	 -i causes the device to be completely initialized.  This will
 *	        ignore any existing vtoc, partitions, or alternates and
 *	        will start from scratch.
 *	 -v causes a verification pass to be run on the entire drive
 *	        looking for bad sectors.  If not specified, only bad
 *	        sectors from the device stanza or -A arguments will be
 *	        added (if necessary).  If specified, it will add any bad
 *	        sectors found to those already known.  THIS OPTION WILL
 *	        ADD CONSIDERABLE TIME TO THE RUNNING OF MKPART!
 */

	while ( (c=getopt(argc, argv, options)) != EOF ) {
		switch (c) {
		case 'B':
			bootfile = optarg;
			break;

		case 'b':
			bootonly++;
			break;

		case 'v':
			not_for_scsi = 1;
			verify |= VER_READ;
			break;

		case 'V':
			not_for_scsi = 1;
			verify |= (VER_READ | VER_WRITE);
			break;

		case 'i':
			initialize++;
			break;
		/*
		 * For adding partitions, we build a parse list of the
		 * specified partition stanza names under addparts.
		 */
		case 'P':
			if (addparts) {
				node *newpart = newnode(LIST);

				newpart->ListRef = (void *)optarg;
				newpart->ListNext = addparts;
				addparts = newpart;
			} else {
				addparts = newnode(LIST);
				addparts->ListRef = (void *)optarg;
			}
			break;

		/*
		 * For removing partitions, we build a parse list of the
		 * specified partition stanza names under subparts.
		 */
		case 'p':
			if (subparts) {
				node *newpart = newnode(LIST);

				newpart->ListRef = (void *)optarg;
				newpart->ListNext = subparts;
				subparts = newpart;
			} else {
				subparts = newnode(LIST);
				subparts->ListRef = (void *)optarg;
			}
			break;

		case 'A':
		    {
			node *newalt = newnode(LIST);
			node *newnum = newnode(NUMBER);

			newnum->Number = atol(optarg);
			newalt->ListRef = (void *)newnum;
			newalt->ListNext = badsecs;
			badsecs = newalt;

			not_for_scsi = 1;

			break;
		    }

		case 'F':
		    {
			long i = atol(optarg);
			struct stat	statbuf;

			not_for_scsi = 1;

			format++;
			if ((i < 0) || (i > 16)) {
			    fprintf(stderr,"Illegal interleave specified\n");
			    exit(65);
			}
			intlv = (char) i;
			if (stat(argv[optind],&statbuf) == -1) {
				perror(argv[optind]); 
				giveusage(1);
				exit(1);
			} else if ( !(statbuf.st_mode & S_IFCHR) ) {
				fprintf(stderr,"%s: not character special device\n",
						argv[optind]);
				giveusage(1);
				exit(1);
			}
			break;
		    }

		case 'f':
			partfile = optarg;
			break;

		case 'x':
			wpfile = optarg;
			targ |= TF_WPART;
			break;
		case 't':
			for( ; *optarg; optarg++ ) {
				switch( *optarg ) {
				case 'a':
					not_for_scsi = 1;
					targ |= TF_ALTS;	break;
				case 'p':
					targ |= TF_PARTS;	break;
				case 'v':
					targ |= TF_VTOC;	break;
				case 'd':
					not_for_scsi = 2;
					targ = TF_ALTS|TF_PARTS|TF_VTOC;
					optarg--;
					optind--;
					break;
				default:
					goto badopt;
				}
				if (targ == (TF_ALTS|TF_PARTS|TF_VTOC))
					break;
			}
			if (!targ) {
				not_for_scsi = 2;
				targ = TF_ALTS|TF_PARTS|TF_VTOC;
			}
			break;

		default:
	badopt:
			fprintf(stderr,"Invalid option '%s'\n",argv[optind]);
			giveusage(0);
			exit(1);
		}
	}

		/* get the last argument -- device stanza */
	if (argc != optind+1) {
		fprintf(stderr,"Missing or bad device stanza name\n");
		giveusage(0);
		exit(2);
	}

	devicestanza = argv[optind];

	/* Special case for low-level format.
	 * The 'device' is an actual UNIX System device name
	 * (e.g. /dev/rdsk/0s0) instead of a device stanza reference.
	 *
	 * Formatting will erase the fdisk table in abs sector 0.
	 * It is, therefore, inappropriate to do anything else
	 * after this operation, before another fdisk table
	 * is established with the 'fdisk' command.
	*/

	if (format) {
		(void)getdevice(devicestanza);
		formatdevice();
		exit(0);
	}

		/* allow some crazy to type his own stanza file at us */
	if (partfile[0] == '-' && !partfile[1]) {
		input = stdin;
	} else if ( !(input = fopen(partfile, "r")) ) {
			fprintf(stderr,"mkpart: ");
			perror(partfile);
			exit(3);
	}

		/* Parse the entire stanza file.  See partitions.y */
	if (yyparse()) {
		fprintf(stderr,"Exiting due to stanza file errors\n");
		exit(4);
	}

	/*
	 * Now we should have a definition of the device stanza.  Look it
	 * up, collect all info from refered-to stanzas into global mydev.
	 */
	if ( !( (dev = lookup(devicestanza))->flags & SY_DEFN ) ) {
		fprintf(stderr,"Device stanza '%s' not defined in file %s\n",
			devicestanza, partfile);
		exit(5);
	} else if ( ((stanza *)(dev->ref))->dev.ds_type != S_DEVICE) {
		fprintf(stderr, "Stanza '%s' is not a device stanza in file %s.\n",
				devicestanza, partfile);
		exit(5);
	} else {
		mydev = (devstanza *)newdevstanza();
		buildstanza(mydev,dev);
	}

	/*
	 * Add any bad sectors specified as 'A' options to those found
	 * in the device stanza.
	 */
	if (mydev->ds_badsec) {
		node *np = mydev->ds_badsec;

		while (np->ListNext)
			np = np->ListNext;
		np->ListNext = badsecs;
	}
	else 
		mydev->ds_badsec = badsecs;

	/*
	 * Open boot file and read in filehdr and aouthdr.  returns true if
	 * there is a valid boot program.  Filehdr.f_nscns and
	 * aouthdr.[tdb]size fields are set correctly in any event.
	 */
	hasboot = getboot();

	/*
	 * Read data structures off device to get current values for dp,
	 * [ivlab,] pdinfo, vtoc, and alttbl.  hasvtoc is true if we could get
	 * them, otherwise we assume an uninitialized device and fill in some
	 * reasonable values.
	 */
	hasvtoc = getdevice(mydev->ds_device);

	if (verify) {
		verifydevice();
	}

	if (targ) {
		printdevinfo(targ);
	}

		/* don't do anything else if only asked for a listing */
	if (!targ || addparts || subparts) {

		/* fill in drtab and boot related structs */
		builddevice(mydev);

		updateparts();  /* update vtoc partitions */

		/* only update alts if installing initial system.
		 * Otherwise, let the dynamic bad block handler
		 * mind the store.
		*/
		updatealts();

		updatedevice(); /* update pdinfo, vtoc base */

		if (hasboot) {
			writeboot();
		} else if (bootonly) {
			fprintf(stderr,"Warning: null bootstrap specified\n");
		}

		if (!bootonly || addparts || subparts) {
			writevtoc();
		}
		return(0);
	}
}


/*
 * Printdevinfo ( print option flags )
 * Print info for user.
 */
void
printdevinfo(flags)
int flags;
{
static struct Px_data {
		int     ptx_tagv;
		char    *ptx_name;
		char    *ptx_tags;
		char    *ptx_cnt;
		} pxdata [] =  {{V_BOOT, "bootx", "BOOT", 0},
				{V_ROOT, "rootx", "ROOT", 0},
				{V_DUMP, "dumpx", "DUMP", 0},
				{V_HOME, "homex", "HOME", 0},
				{V_SWAP, "swapx", "SWAP", 0},
				{V_STAND, "standx", "STAND", 0},
				{V_USR,  "usrx",  "USR",  0},
				{V_VAR,  "varx",  "VAR",  0},
				{V_ALTS, "altsx", "ALTS", 0},
				{V_ALTTRK,"trkaltx","ALTTRK",0},
				{V_OTHER,"otherx","OTHER",0}};

	if (flags & TF_WPART) {
		FILE *pfile;
		ushort  i;
		char    pxnam [8];

		if ((pfile=fopen(wpfile,"w")) == NULL)
			{
			perror("mkpart opening file for -x");
			exit(101);
			}
		fprintf(pfile,
			"diskx:\nheads = %d, cyls = %d, sectors = %d, bpsec = %d,\n",
			pdinfo.tracks, pdinfo.cyls, pdinfo.sectors, pdinfo.bytes);
		fprintf(pfile,
			"vtocsec = %ld, altsec = %ld, boot = \"%s\", device = \"%s\"\n\n",
			mydev->ds_vtocsec, mydev->ds_altsec,
			(mydev->ds_boot ? mydev->ds_boot : "/etc/boot"),
			mydev->ds_device);
		for (i=0; i<vtoc.v_nparts; i++)
			{
			struct partition *pp = &vtoc.v_part[i];

			if (pp->p_flag & V_VALID)
				{
				int j;
				int K = sizeof(pxdata) / sizeof(struct Px_data);
				int goodp=0;

				for (j=0; j < K; j++)
					{
					if (pp->p_tag == pxdata[j].ptx_tagv)
						{
						goodp = 1;
						break;
						}
					}
				if (goodp)
					{
					if (pxdata[j].ptx_cnt++)
						sprintf(pxnam,"%s%d",
							pxdata[j].ptx_name,
							pxdata[j].ptx_cnt);
					else    sprintf(pxnam,"%s",
							pxdata[j].ptx_name);
					fprintf(pfile,"%s:\n",pxnam);
					fprintf(pfile,
						"partition = %d, start = %ld, size = %ld,\n",
						i, pp->p_start, pp->p_size);
					fprintf(pfile,"tag = %s",pxdata[j].ptx_tags);
					if (pp->p_flag & V_VALID)
						fprintf(pfile,", perm = VALID");
					if (pp->p_flag & V_UNMNT)
						fprintf(pfile,", perm = NOMOUNT");
					if (pp->p_flag & V_RONLY)
						fprintf(pfile,", perm = RO");
					fprintf(pfile,"\n\n");
					}
				}
			}
		if (ferror(pfile))
			{
			perror("mkpart writing to file for -x");
			exit(101);
			}
		if (fclose(pfile) != 0)
			{
			perror("closing file for -x");
			exit(101);
			}
	}

	if (flags & TF_VTOC) {
		printf("\tDevice %s\n",mydev->ds_device);
		printf("device type:\t\t%ld\n",pdinfo.driveid);
		printf("serial number:\t\t%.12s\n",pdinfo.serial);
		printf("cylinders:\t\t%ld\t\theads:\t\t%ld\n",pdinfo.cyls,pdinfo.tracks);
		printf("sectors/track:\t\t%ld\t\tbytes/sector:\t%ld\n",pdinfo.sectors,pdinfo.bytes);
		printf("number of partitions:\t%d",vtoc.v_nparts);
		printf("\t\tsize of alts table:\t%d\n", pdinfo.alt_len);
	}
	if (flags & TF_PARTS) {
		ushort i;

		for (i = 0; i < vtoc.v_nparts; i++) {
			printf("partition %d:\t",i);
			printpart(&vtoc.v_part[i]);
		}
	}
	if (flags & TF_ALTS) {
		/** Never true for SCSI since this bit is masked off **/
		printalts();
	}
}


/*
 * Printpart ( partition entry pointer )
 * Support for printdevinfo().  Print out a formatted partition report.
 */
void
printpart(v_p)
struct partition *v_p;
{
	switch(v_p->p_tag) {
	case V_BOOT:	printf("BOOT\t\t");			break;
	case V_DUMP:	printf("DUMP\t\t");			break;
	case V_ROOT:	printf("ROOT\t\t");			break;
	case V_HOME:	printf("HOME\t\t");			break;
	case V_SWAP:	printf("SWAP\t\t");			break;
	case V_USR:	printf("USER\t\t");			break;
	case V_VAR:	printf("VAR\t\t");			break;
	case V_STAND:	printf("STAND\t\t");			break;
	case V_BACKUP:	printf("DISK\t\t");			break;
	case V_ALTS:	printf("ALTERNATES\t");			break;
	case V_ALTTRK:	printf("ALT TRACKS\t");			break;
	case V_OTHER:	printf("NONUNIX\t\t");			break;
	default:	printf("unknown 0x%x\t",v_p->p_tag);	break;
	}

	printf("permissions:\t");
	if (v_p->p_flag & V_VALID)	printf("VALID ");
	if (v_p->p_flag & V_UNMNT)	printf("UNMOUNTABLE ");
	if (v_p->p_flag & V_RONLY)	printf("READ ONLY ");
	if (v_p->p_flag & V_OPEN)	printf("(driver open) ");
	if (v_p->p_flag & ~(V_VALID|V_OPEN|V_RONLY|V_UNMNT))
					printf("other stuff: 0x%x",v_p->p_flag);
	printf("\n\tstarting sector:\t%ld (0x%lx)\t\tlength:\t%ld (0x%lx)\n",
		v_p->p_start, v_p->p_start, v_p->p_size, v_p->p_size);
}


/*
 *
 * Giveusage ()
 *
 * 	Give a (not so) concise message on how to use mkpart.
 *
 * Parameters:
 *
 *	extend != 0 -> give shortform
 *
 * no Return Values/Exit State
 *
 */
void
giveusage(extent)
int extent;
{
    if(extent) {
	fprintf(stderr,"\nmkpart [options] device\nor,\n");
	fprintf(stderr,"mkpart -F interleave_factor raw_UNIX_System_device\n\n");
	return;
    }
    fprintf(stderr,"mkpart [-P add_partition_name] [-p remove_partition_name]\n");
    fprintf(stderr,"   [-b] [-B boot_code_file] [-f partition_file]\n");
    fprintf(stderr,"   [-A absolute_sector_number] [-F interleave_factor]\n");
    fprintf(stderr,"   [-t apv] [-x filename] [-v] [-V] [-i] device\n");
    fprintf(stderr,"NOTE: multiple P and p flags may be specified.\n");
    fprintf(stderr,"-b just rewrites the boot code, as determined from the partition file.\n");
    fprintf(stderr,"-B specifies a different boot file.\n");
    fprintf(stderr,"-F formats the entire device with specified interleave.\n");
    fprintf(stderr,"\tThe last arg is a raw UNIX System device (e.g. /dev/rdsk/1s0)\n");
    fprintf(stderr,"-A marks the sector bad and assigns it an alternate.\n");
    fprintf(stderr,"-f specifies the partition file; its absence implies %s.\n",
	PARTFILE);
    fprintf(stderr,"-t asks for a listing of:\n");
    fprintf(stderr,"   a - alternate sectors table,\n");
    fprintf(stderr,"   p - partitions,\n");
    fprintf(stderr,"   v - vtoc and physical drive characteristics.\n");
    fprintf(stderr,"-x writes a complete device and partition stanza list for the\n");
    fprintf(stderr,"   specified device to file 'filename' (useful for recovery).\n");
    fprintf(stderr,"-v attempts to read every sector on the device and adds any\n");
    fprintf(stderr,"   bad ones to the alternates table.  THIS TAKES A WHILE...\n");
    fprintf(stderr,"-V does -v also, but first WRITES every sector of the drive.\n");
    fprintf(stderr,"-i initializes the device, ignoring any existing VTOC data.\n");
    fprintf(stderr,"   MUST BE USED if the device has never been formatted;\n");
    fprintf(stderr,"   may be used to re-initialize a drive.\n");
}


#define MAXUSEDEPTH     100	/* Deepest 'use' or 'usepart' nesting */

/*
 * Buildstanza ( fresh stanza pointer, symbol table entry pointer )
 * Build a new stanza from the one that the symbol table entry points at,
 * including all of the data from any other stanzas that are 'use'ed or
 * 'usepart'ed.  In particular, used stanzas are included deepest first.
 */
void
buildstanza(r,n)
stanza *r;
symbol *n;
{
stanza		*s = (stanza *)n->ref;		/* use'ed stanza, if any */
int     	usesdepth = -1;			/* current top of stanza stack*/
int     	stanzatype = s->dev.ds_type;	/* either dev or part stanza */
stanza		*uses[MAXUSEDEPTH];		/* use stanza stack */

	/*
	 * Walk through all use'ed stanzas and collect them onto the use'ed
	 * stanza stack.  Watch for stack overflows and mutual recursion.
	 */
		/* push current stanza, check for another use */
	while ( (uses[++usesdepth] = s)->dev.ds_use ) {
		s = (stanza *)s->dev.ds_use->ref; /* get next stanza */
		if( !s ) {
			fprintf(stderr,"Referenced stanza '%s' not defined\n",
				uses[usesdepth]->dev.ds_use->name);
			exit(20);
		}
		/*
		 * Check for mutual recursion.  MaxUseDepth is set during
		 * the parse of the stanza file to the number of stanzas
		 * encountered.  If we have gone through all of them, we
		 * must be reusing one now... and forever.
		 */
		if (usesdepth > MaxUseDepth) {
			fprintf(stderr,"From stanza '%s' circular USE\n",
				n->name);
			exit(20);
		} else if (usesdepth >= MAXUSEDEPTH) {
			myerror("Stanzas nested too deeply",0);
		} else if (s->dev.ds_type != stanzatype) {
			fprintf(stderr,"From stanza '%s' incompatible stanza USEd\n",
				n->name);
			exit(20);
		}
	}

/*
 * CHECKSTANZAELEM is a macro that saves a lot of paper (and keying time).
 * It checks stanza field 'f' for some condition, COND(f); if true for both
 * the current source (s) stanza and for the resultant (r) stanza that we are
 * building up, it prints the message 'm' (which should be a "ed string).
 * The implication is that we have two stanzas that specify the same value.
 * In any event, it then performs operation OP(r->f,s->f).
 */

#define CHECKSTANZAELEM(f,COND,OP,m)                                         \
	if (COND(s->f)) {                                                    \
		if (COND(r->f)) {                                            \
			fprintf(stderr,"stanza %s, warning: %s\n",n->name,m);\
		}                                                            \
		OP(r->f,s->f);                                               \
	}

/*
 * The following definitions are useful CONDs for CHECKSTANZA.  Note that these
 * are all named for the opposite condition that is actually checked (it seemed
 * like a good idea at the time... :-).
 */
#define ZERO(a)         (a)
#define UNDEFSECTOR(a)  ((a)!=UNDEFINED_SECTOR)
#define UNDEFNUMBER(a)  ((a)!=UNDEFINED_NUMBER)

/*
 * The following are OPs for CHECKSTANZA.
 */
#define ASSIGN(a,b)     ((a)=(b))

	/*
	 * For each used stanza, in stack order, pick up the stanza and
	 * check and then include its data into the result stanza.
	 */
	for (s = uses[usesdepth]; usesdepth-- >= 0; s = uses[usesdepth]) {
			/* Device stanzas processed here... */
		if (stanzatype == S_DEVICE) {
			CHECKSTANZAELEM(dev.ds_boot,ZERO,ASSIGN,"boot file redefined")
			CHECKSTANZAELEM(dev.ds_device,ZERO,ASSIGN,"device name redefined")
			CHECKSTANZAELEM(dev.ds_dserial,ZERO,ASSIGN,"serial number redefined")
			CHECKSTANZAELEM(dev.ds_heads,ZERO,ASSIGN,"number of heads redefined")
			CHECKSTANZAELEM(dev.ds_cyls,ZERO,ASSIGN,"number of cylinders redefined")
			CHECKSTANZAELEM(dev.ds_sectors,ZERO,ASSIGN,"number of sectors/track redefined")
			CHECKSTANZAELEM(dev.ds_bpsec,ZERO,ASSIGN,"number of bytes/sector redefined")
			CHECKSTANZAELEM(dev.ds_vtocsec,UNDEFSECTOR,ASSIGN,"vtoc sector redefined")
			CHECKSTANZAELEM(dev.ds_altsec,UNDEFSECTOR,ASSIGN,"alternate track table redefined")
			/*
			 * For bad sector lists, if there isn't a list in the
			 * result stanza yet, put the current one there, else
			 * merge them together.  Mergeranges checks for
			 * collisions.
			 */
			if (r->dev.ds_badsec) {
				r->dev.ds_badsec =
					mergeranges(r->dev.ds_badsec,s->dev.ds_badsec);
			} else if (s->dev.ds_badsec) {
				r->dev.ds_badsec = s->dev.ds_badsec;
			}
			if (r->dev.ds_badtrk) {
				r->dev.ds_badtrk =
					mergeranges(r->dev.ds_badtrk,s->dev.ds_badtrk);
			} else if (s->dev.ds_badtrk) {
				r->dev.ds_badtrk = s->dev.ds_badtrk;
			}
		} else {
				/* Partition stanzas processed here... */
			CHECKSTANZAELEM(part.ps_partno,UNDEFNUMBER,ASSIGN,"partition number redefined")
			CHECKSTANZAELEM(part.ps_ptag,ZERO,ASSIGN,"partition tag redefined")
			CHECKSTANZAELEM(part.ps_start,UNDEFSECTOR,ASSIGN,"starting sector redefined")
			CHECKSTANZAELEM(part.ps_size,UNDEFSECTOR,ASSIGN,"partition size redefined")
			r->part.ps_perm |= s->part.ps_perm;
		}

		/* make the result point at the last stanza processed */
		r->dev.ds_name = uses[0]->dev.ds_name;
	}
}


#ifdef _NEVER_USED
/*
 * Printstanza ( stanza pointer )
 * A debugging utility that prints a formatted stanza.
 */
void
printstanza(s)
stanza *s;
{
	if (s->dev.ds_type == S_DEVICE) {
		printf("\tDevice Stanza '%s'\n",s->dev.ds_name->name);
		printf("boot code file:\t\t'%s'\n",s->dev.ds_boot?s->dev.ds_boot:"none");
		printf("device name:\t\t'%s'\n",s->dev.ds_device?s->dev.ds_device:"none");
		printf("serial #:\t\t'%s'\n",s->dev.ds_dserial?s->dev.ds_dserial:"none");
		printf("# of heads:\t\t%d\n",s->dev.ds_heads);
		printf("# of cyls:\t\t%d\n",s->dev.ds_cyls);
		printf("# of sectors:\t\t%d\n",s->dev.ds_sectors);
		printf("# of bytes/sector:\t%d\n",s->dev.ds_bpsec);
		printf("vtoc sector:\t\t0x%x\n",s->dev.ds_vtocsec);
		printf("alt table sector:\t0x%x\n",s->dev.ds_altsec);
		{
		node *n = s->dev.ds_badsec;

		printf("bad sectors:\t\t");
		while (n) {
			if (n->ListElem->token == RANGE) {
				printf("0x%x - 0x%x, ", n->ListElem->RangeLo,n->ListElem->RangeHi);
			} else {
				printf("0x%x, ", n->ListElem->Number);
			}
			n = n->ListNext;
		}
		printf("\n");
		}
		{
		node *n = s->dev.ds_badtrk;

		printf("bad tracks:\t\t");
		while (n) {
			if (n->ListElem->token == RANGE) {
				printf("0x%x - 0x%x, ", n->ListElem->RangeLo,n->ListElem->RangeHi);
			} else {
				printf("0x%x, ", n->ListElem->Number);
			}
			n = n->ListNext;
		}
		printf("\n");
		}
	} else if (s->part.ps_type == S_PART) {
		printf("\tPartition Stanza '%s'\n",s->part.ps_name->name);
		printf("partition #:\t\t%d\n",s->part.ps_partno);
		printf("starting sector #:\t%ld\n",s->part.ps_start);
		printf("partition size:\t\t%ld\n",s->part.ps_size);
		printf("partition tag:\t\t");
		switch(s->part.ps_ptag) {
		case V_BOOT:    printf("BOOT\n");       break;
		case V_ROOT:    printf("ROOT\n");       break;
		case V_DUMP:    printf("DUMP\n");       break;
		case V_HOME:    printf("HOME\n");       break;
		case V_SWAP:    printf("SWAP\n");       break;
		case V_STAND:   printf("STAND\n");      break;
		case V_USR:     printf("USR\n");        break;
		case V_VAR:     printf("VAR\n");        break;
		case V_BACKUP:  printf("BACKUP\n");     break;
		case V_ALTS:    printf("ALTS\n");       break;
		case V_ALTTRK:	printf("ALT TRACKS\t");	break;
		case V_OTHER:   printf("OTHER\n");      break;
		defualt:        printf("Unknown type %d\n",s->part.ps_ptag);
		}
		printf("permissions:\t\t");
		{
		int p = s->part.ps_perm;

		if (p) {
			if (p&V_UNMNT) { printf("UNMOUNT ");  p&=~V_UNMNT; }
			if (p&V_RONLY) { printf("RONLY ");    p&=~V_RONLY; }
			if (p&V_OPEN)  { printf("OPEN ");     p&=~V_OPEN;  }
			if (p&V_VALID) { printf("VALID ");    p&=~V_VALID; }
			if (p) { printf("unknown bits 0x%x",p); }
		} else {
			printf("No permission bits set!");
		}
		printf("\n");
		}
	} else {
		printf("Unknown stanza type %d for stanza '%s'\n",s->part.ps_type,s->part.ps_name->name);
	}
}
#endif /* _NEVER_USED */

/*
 * mkpartopen() will open the specified device and set the dev_fd_open
 * flag to 1. This open was pulled out of getdevice() so it may be
 * called separately for determining if this is a scsi device.
 * getdevice() in turn has been changed to use this routine for opening
 * the device.
 */

int
mkpartopen(name)
char *name;
{
	struct stat statbuf;

	if(dev_fd_open)
		return(0);

	if (stat(name,&statbuf) == -1) {
		perror(name);
		return(1);
	} else if ( !(statbuf.st_mode & S_IFCHR) ) {
		fprintf(stderr,"Device %s is not character special\n",name);
		return(1);
	}

	if ((devfd = open(name, 2 )) == -1) {
		fprintf(stderr,"Opening device ");
		perror(name);
		return(1);
	}

	/** check the device type. If it's a scsi device, set the      **/
	/** is_scsi_dev flag.					       **/
	is_scsi_dev = verify_scsi();

	/** indicate dev is open **/
	dev_fd_open = 1;
	return(0);
}

/*
 * mkpartclose() is the counterpart of mkpartopen() in that it
 * closes the device and sets the dev_fd_open flag to 0 to
 * indicate that the device is not open.
 */

int
mkpartclose()
{
	close(devfd);
	dev_fd_open=0;
	return(0);
}
