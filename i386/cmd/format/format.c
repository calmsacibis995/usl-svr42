/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)format:i386/cmd/format/format.c	1.1.6.3"
#ident  "$Header: format.c 1.4 91/08/08 $"

char formatcopyright[] = "Copyright 1986 Intel Corp.";

/*
 * format.c
 *	Format utility for disks.  uses iSBC214-style ioctl calls.
 *	supportted by: iSBC 214
 *	This code is very dependent on the ioctl call behaviour.
 */

#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/iobuf.h>
#include <sys/vtoc.h>
#include <stdio.h>
#include <errno.h>

long	lseek();
long	atol();
void	dev_check();

extern int	opterr;
extern char	*optarg;
extern int	optind;

extern	errno;

int	qflag		= 0;		/* "quiet" flag   */
int	vflag		= 0;		/* "verbose" flag */
int	verifyflag	= 0;		/* sector r/w verify flag */
int	exhaustive	= 0;		/* verify is exhaustive? */
int	write_bb	= 0;		/* need to write bb info?? */
int	dot_column;			/* this value is used to keep
					track of which column we will
					print the next dot in so that 
					if we are running the -v option,
					the dots still come out aligned.
					*/

char	*device;			/* The name of the device. */
int	devfd;				/* File descriptor for above. */
struct  disk_parms dp;                  /* Device description from driver */

daddr_t	numtrk;				/* number of cylinders */

daddr_t	first_track = 0;			/* First track to format. */
daddr_t	last_track = -1;			/* Last track to format. */
ushort	interleave = 2;

/* Exit Codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* usage */
#define	RET_STAT	2	/* can't stat device */
#define	RET_OPEN	3	/* can't open device */
#define	RET_CHAR	4	/* device not character device */
#define	RET_FLOPPY	5	/* device not a floppy disk */
#define	RET_FORM0	6	/* formatted 0 tracks */

giveusage()
{
	fprintf(stderr, "Usage: format [-f first] [-l last] [-i interleave]\n");
	fprintf(stderr, "              [-q] [-v] [-V] [-E] <dev>\n");
	fprintf(stderr, "       NOTES: -E implies -V\n");
	exit(RET_USAGE);
}

main(argc, argv)
int	argc;
char	**argv;
{

	register daddr_t track,tracks;
	int	leave_loop;
	ushort	sector;
	char	c;

	if (argc == 1)
		giveusage();

	/*
	 * Decode/decipher the arguments.
	 */

	opterr = 0;
	while ((c = getopt(argc,argv,"f:l:i:qvVE")) != EOF)
		switch(c) {

		case 'f':	/* first track to format */
			first_track = atol (optarg);
			continue;
		case 'l':	/* last track to format */
			last_track = atol (optarg);
			continue;
		case 'i':	/* interleave */
			interleave = atol (optarg);
			continue;

		case 'q':	/* quiet mode */
			++qflag;
			continue;
			/*
			 * Verify/verbosity and other aberations.
			 */
		case 'v':			/* verbose */
			++vflag;
			break;
		case 'V':			/* sector r/w verify 'on' */
			++verifyflag;
			continue;
		case 'E':			/* Exhaustive verify */
			++verifyflag;		/* assumes verify */
			++exhaustive;
			continue;
		default:
			giveusage() ;
		}

	if (opterr)
		giveusage();

	if (optind < argc)
		device = argv[optind++];
	else
		giveusage();

	if (optind < argc)
		giveusage();

	/*
	 * Ask the driver what the device looks like.
	 */
	if ((devfd = open(device, 2 )) == -1) {
		fprintf(stderr, "format: Can't open %s.\n", device);
		exit(RET_OPEN);
	}

	if(ioctl(devfd, V_GETPARMS, &dp) == -1) {
		fprintf(stderr,"format: Device must be a character device.\n");
		exit(RET_CHAR);
	}

	if (dp.dp_type != DPT_FLOPPY) {
		fprintf(stderr, "format: %s is not a floppy disk.\n", device);
		exit(RET_FLOPPY);
	}

	/* entry point for machine dependent device checks */
	dev_check();

	numtrk = dp.dp_cyls * dp.dp_heads;
	tracks = numtrk - (dp.dp_pstartsec/dp.dp_sectors);

	/*
	 * If the last track was not specified, compute it.
	 */
	if (last_track == -1)
		last_track = dp.dp_pnumsec/dp.dp_sectors - 1;
	if (last_track > (tracks-1)) {
		fprintf(stderr, "format: format limited to track: %lu\n",tracks);
		last_track = tracks;
	}

	sector = 0;
	if (!qflag)
		printf ("formatting");
	dot_column = 10;
	for(track=first_track; track<=last_track; track++) {
		if (!qflag)
			printdot(track);
		if(!fmtrack(track))
			break;
		if (verifyflag) {
			if (!track_ok(track, &sector))
				break;
		}
	}

	/*
	 * Tell him what happened.
	 */

	tracks = track - first_track;
	if(!qflag)
		printf ("\n");
	if (tracks == 1) {
		if(!qflag)
			printf("Formatted 1 track: %lu, interleave %d.\n",
				first_track, interleave);
	}
	else if (tracks == 0) {
		fprintf(stderr,"format: Formatted 0 track: Please check floppy density\n");
		close(devfd);
		exit(RET_FORM0);
	}
	else if(!qflag)
		printf("Formatted %lu tracks: %lu thru %lu, interleave %d.\n",
				tracks, first_track, track-1, interleave);

	close(devfd);
	exit(RET_OK);
}
/**************************************************************************
 *		main ends here.  below are subroutines			  *
 **************************************************************************
 */

/*
* fmtrack
*       Format a track.
*
*/

fmtrack(track)
daddr_t	track;					/* partition relative */
{
	union io_arg  fmt;
	int fmtval;

	fmt.ia_fmt.start_trk = track;
	fmt.ia_fmt.num_trks = 1;
	fmt.ia_fmt.intlv = interleave;
	fmtval = ioctl(devfd, V_FORMAT, &fmt);
	return(fmtval == 0);
}


/*
 *	track_ok writes to a sector, reads from it, compares the values,
 *		if values mis-compare, return 0;
 *	Does single, random-sector testing when
 *		exhaustive is set to 0.
 */
track_ok (track_no, sector_p)
daddr_t	track_no;
ushort	*sector_p;
{

#define	BSIZE	BUFSIZ

	char	buffer[BSIZE];
	long	present;
	daddr_t	start, end;
	int	i;
	long	junk;
	register ushort secsiz;


	secsiz = dp.dp_secsiz;
	start = (daddr_t)track_no * dp.dp_sectors;

	if (!exhaustive) {
		start = start + (int) (rand() % dp.dp_sectors);
		end = start + 1;
	}
	else {
		end = start + dp.dp_sectors;
	}

	for (present = start; present < end; present++) {
		for (i=0; i < secsiz; i++) {
			buffer[i] = (char)0xa5;
		}
		junk = lseek (devfd, (long) (present * secsiz), 0);
		if ((junk = write (devfd, buffer, secsiz)) != secsiz) {
			*sector_p = present - start;
			return(0);
		}
		for (i=0; i < secsiz; i++)
			buffer[i] = (char)0x5a;
		junk = lseek (devfd, (long) (present * secsiz), 0);
		if (read (devfd, buffer, secsiz) != secsiz) {
			*sector_p = present - start;
			return(0);
		}
		for (i=0; i < secsiz; i++) {
			if (buffer[i] != (char)0xa5) {
				*sector_p = present - start;
				return(0);
			}
		}
	}
	return (1);
}


/*
 * printdot
 *	Put a dot on the screen
 */
printdot(track)
daddr_t	track;
{
	if (((track % ((dp.dp_heads * 5) * 40)) == 0) && (track != first_track)) {
		printf ("\n          ");
		dot_column = 10;
	}
	if ((track % (dp.dp_heads * 5)) == 0) {
		printf (".");
		dot_column++;
		fflush(stdout);
	}
}
