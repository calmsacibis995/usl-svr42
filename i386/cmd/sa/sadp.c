/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sa:i386/cmd/sa/sadp.c	1.5"
#ident	"$Header: $"

/*
 *	sadp profiles the SCSI disk queues maintained by the SD01 driver 
 *	or the generic device interface.
 *
 *	usage : sadp [-th][-d device[-drive]] s [n]
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <sys/buf.h>
#include <sys/elog.h>
#include <sys/ksym.h>
#include <sys/iobuf.h>

#include <sys/vtoc.h>
#include <sys/altsctr.h>
#include <sys/sdi_edt.h>
#include <sys/sdi.h>
#include <sys/scsi.h>
#include <sys/sd01.h>

#include <time.h>
#include <sys/utsname.h>
#include <sys/var.h>
#include <ctype.h>

#include <stdio.h>

/* cylinder profiling */
#define BLANK ' '
#define BLOB '*'
#define TRACE '.'
#define BRK '='
#define FOOT '-'
#define CYLNO   1
#define SEEKD   2
#define	CHPERCYL	103	/*the number of 8 cylinder chunks on a disk*/


#define cylin b_resid;
#define NDRIVE	3
#define SNDRIVE	56
#define SD01	"sd01_diskcnt"
#define L_SCSI 	"Sd01_dp"

#define	IDNDRV	2

/* devnm used to have 3 disk type strings:
 *	"hdsk" occupied position 0
 *	"fdsk" occupied position 2
 * sadp now only supports sd01 drivers and generic device interface
 */
char devnm[1][5] = {
	"sd01",			/* SCSI disk */
};
#define DEVNM_MSG "Only the sd01 device is supported"
#define CHUNKS	100
#define CYLS_PER_CHUNK	20

#define GETINFO(fd,name,buf,buflen)	ginfo(fd,name,buf,buflen,MIOC_READKSYM)
#define GETIINFO(fd,name,buf,buflen)	ginfo(fd,name,buf,buflen,MIOC_IREADKSYM)

int	Sd01_diskcnt = 0;
struct	last_time_info {
	int	end_index;
	long	b_blkno;
};

struct	drive_information {
	int	monitored;
	long	io_ops;
	struct	last_time_info last_time;
	long	chunk_accessed[CHUNKS];
	long	seek_dist[CHUNKS];
	long	total_accesses;
	long	sum_seeks;
};
struct drive_information *drive_info;
struct disk **dsd01_p;
struct disk *dsd01;

int fflg,dflg,tflg,hflg,errflg;
int s,n,ct;
int dev =0;
int temp;
int f;
int i;
int n1,dleng,dashb,k;
int dashf;
int dn;
int Sdrvlist[SNDRIVE];	/* SCSI */
struct HISTDATA {
	long    hdata[CHPERCYL];
};
 
char *nopt;
char empty[30];
char drive[30];
char *malloc();
int  ALL;
 
main(argc,argv)
int argc;
char **argv;
{
	extern  int     optind;
	extern  char    *optarg;
	int c,j;
	void	exit1(), extmsg(), SD01_profiler();

	while ((c = getopt(argc,argv,"thd:")) != EOF)
		switch(c) {
		case 't':
			tflg++;
			break;
		case 'h':
			hflg++;
			break;
		case 'd':
			dleng = strlen(optarg);
			if (dleng == 5){
				extmsg(DEVNM_MSG);
			}
			if ( strncmp(optarg,"sd01",4) != 0)
				extmsg(DEVNM_MSG);
			if (dleng > 4){
			for (i=5,n1=5;i<dleng;i++){
				if (optarg[i] == ','){
					if (n1 == i){
					errflg++;
					break;
					}
					if (getdrvn() != 0) {
						errflg++;
						break;
					}
					if (dashf != 0) {
						if (dashb >= dn){
							errflg++;
							break;
						}
						for (j=dashb;j<dn+1;j++){
							 Sdrvlist[j] = 1;
						}
						dashb = 0;
						dashf = 0;
					}
					else
					{
						Sdrvlist[dn] = 1;
					}
				n1 = i+1;
				}
				else {
				if (optarg[i] == '-'){
					if (dashf != 0) {
						errflg++;
						break;
					}
					if (getdrvn() != 0) {
						errflg++;
						break;
					}
					Sdrvlist[dn] = 1;
					dashb = dn;
					dashf = 1;
					n1=i+1;
				}
				else { 
					if (i == dleng-1){
					i++;
					if (getdrvn() != 0) {
						errflg++;
						break;
					}
					if (dashf != 0)
						for (j=dashb;j<dn+1;j++){
							Sdrvlist[j] =1;
						}
					else
					{
						Sdrvlist[dn] = 1;
					}
					}
				}
				}
			}
			}
			else {
				if (dleng != 4){
					extmsg(DEVNM_MSG);
				}
				ALL++;
			}
			if (errflg)
				break;
			temp = 3;

			if (strncmp(optarg,devnm[0],4) != 0){
				extmsg(DEVNM_MSG);
			}
			dflg++;
			break;
		case '?':
			errflg++;
			break;
		}
	if (errflg)
		exit1();

/*      if no frequency arguments present, exit */
	if (optind == argc)
		exit1();
/*      if a non-dash field is presented as an argument,
	check if it is a numerical arg.
*/
	nopt = argv[optind];
	if (tstdigit(nopt) != 0)
		exit1();
/*      for frequency arguments, if only s is presented , set n to 1
*/
	if ((optind +1) == argc) {
		s = atoi(argv[optind]);
		n = 1;
	}
/*      if both s and n are specified, check if 
	arg n is numeric.
*/
	if ((optind +1) < argc) {
		nopt = argv[optind + 1];
		if (tstdigit(nopt) != 0)
			exit1();
		s = atoi(argv[optind]);
		n = atoi(argv[optind+1]);
	}
	if ( s <= 0 )
		extmsg("bad value of s");
	if ( n <= 0 )
		extmsg("bad value of n");
	ct = s;

/*      open /dev/kmem  */

	if((f= open("/dev/kmem",0)) == -1)
		extmsg("cannot open /dev/kmem");

#ifdef DEBUG
fprintf (stderr, "dev = %d	dflg = %d\n", dev, dflg);
#endif
	if (dflg == 0){

              Sdrvlist[0] = 1;
	}

        SD01_profiler();
}

 
/*      get drive number routine	*/
int
getdrvn()
{
	extern char *optarg;
	char *strcpy();
	char *strncat();
 
	strcpy(drive,empty);
	strncat(drive,&optarg[n1],i-n1);
	if (tstdigit(drive) != 0)
		return (-1);
	dn =atoi(drive);
	if (dn >= SNDRIVE)
		return(-1);
	return(0);
}

void
exit1()
{
	fprintf(stderr,"usage:  sadp [-th][-d device[-drive]] s [n]\n");
	exit(1);
}

void
extmsg(msg)
char	*msg;
{
	fprintf(stderr, "sadp: %s\n", msg);
	exit(4);
}

int
tstdigit(ss)
char *ss;
{
	int kk,cc;
	kk=0;
	while ((cc = ss[kk]) != '\0' ){
		if (isdigit(cc) == 0)
			return(-1);
		kk++;
	}
	return(0);
}

/*      the following routines are obtained from iostat */

/*.s'prthist'Print Histogram'*/

void
prthist(array, mrow, scale, gmax)
	long array[], scale, gmax;
register mrow;
{
	long    line;
	void	pline();

	line = 50;
	/* handle overflow in scaling */
	if(gmax > 51) {
		line = 52;
		printf("\n%2ld%% -|", gmax/scale);
		pline(line--, array, mrow, BLOB);
		printf("\n     %c", BRK);
		pline(line--, array, mrow, BRK);
	} 
	else if ( gmax = 51 )
		line = 51;
	while( line > 0) {
		if((line & 07) == 0) {
			printf("\n%2ld%% -|", line/scale);
		} 
		else {
			printf("\n     |");
		}
		pline(line--, array, mrow, BLOB);
	}
	printf("\n 0%% -+");
	line = -1;
	pline( line, array, mrow, FOOT);
}


/*.s'pline'Print Histogram Line'*/
void
pline(line, array, mrow, dot)
	long line, array[];
int mrow;
char dot;
{
	register ii;
	register char *lp;
	char lbuff[132];

	lp = lbuff;
	for(ii = 0; ii < mrow; ii++)
		if(array[ii] < line)
			if(line == 1 && array[ii] == 0)
				*lp++ = TRACE;
			else
				*lp++ = BLANK;
		else
			*lp++ = dot;
	*lp++ = 0;
	printf("%s", lbuff);
}

/*
 * SD01_profiler() called by main to profile the SCSI disk
 */
void
SD01_profiler()
{
        void populate(); 
	void initialize_info();
	void extract_info();
	void print_results();
	unsigned sleep();

	register int j;			/* Scratch loop counter.	*/
	register int time_left;		/* Time left in this interval.	*/

	while (n--)	{
		initialize_info();
		time_left = s;
		while(time_left--) {
			populate();
			extract_info();
		if (time_left)
			sleep(1);
		}

		/*
		 * At the end of the interval, get the present I/O count
		 * and subtract from it the I/O count of the beginning of
		 * the interval to get the number of blocks transferred
		 * to and from each disk drive during the interval.
		 */
		populate();
		for (j = 0; j < Sd01_diskcnt; j++)	{
			if (Sdrvlist[j] == 0)
				continue;	/* Skip this drive. */
			drive_info[j].io_ops = dsd01[j].dk_stat.ios.io_ops - drive_info[j].io_ops;
		}

		print_results();
	}
	exit(0);
}


/*
 * initialize_info()
 */

void
initialize_info()

{
	register int  j, k;   /* Loop counters 	*/
	register int index;	/* Index into ptrackq.	*/
	void populate();
	long lseek();

	if (GETINFO(f,SD01,&Sd01_diskcnt,sizeof(Sd01_diskcnt)) == 0) {
		drive_info = (struct drive_information *) malloc(sizeof(struct drive_information) * Sd01_diskcnt);
			if(drive_info == NULL)
				extmsg("cannot malloc");

		/*
		 * allocate space for pointers to disk structures
		 * then get the pointers
		 * Note: PDI won't invalidate any of the pointers we get
		 * here.  The worst that could happen is that HBAs could
		 * be loaded while sadp is running and we just wouldn't
		 * see them.
		 */
		dsd01_p = (struct disk **) malloc(sizeof (struct disk *) * Sd01_diskcnt);
				if (dsd01_p == NULL)
					extmsg("cannot malloc");
		if (GETINFO(f,L_SCSI,dsd01_p,sizeof(struct disk *) * Sd01_diskcnt) != 0){
			extmsg("cannot read SD01 data structure pointers");
		}

		/*
		 * allocate space for disk structures
		 */
		dsd01 = (struct disk *) malloc(sizeof (struct disk) * Sd01_diskcnt);
				if (dsd01 == NULL)
					extmsg("cannot malloc");
}

		populate();
		for (j = 0; j < Sd01_diskcnt; j++)	{
			if (Sdrvlist[j] == 0)
				continue;   /* Not interested in this drive.*/
			for (k = 0; k < CHUNKS; k++)	{
				drive_info[j].chunk_accessed[k] = 0;
				drive_info[j].seek_dist[k] = 0;
			}
			drive_info[j].total_accesses = 0;
			drive_info[j].sum_seeks = 0;

			drive_info[j].io_ops = dsd01[j].dk_stat.ios.io_ops;
			/*
			 * Calculate the index of the last-used entry (the
			 * entry immediately before the head of the queue)
			 * in the performance tracking queue.  The head
			 * of the queue can be at index 0 iff no access
			 * has been made to the disk since the last boot.
			 */
			index = dsd01[j].dk_stat.pttrack - dsd01[j].dk_stat.ptrackq - 1;	
			if (index < 0)	/* Fresh queue.	*/
				index = 0;
			drive_info[j].last_time.end_index = index;
			drive_info[j].last_time.b_blkno = dsd01[j].dk_stat.ptrackq[index].b_blkno;

		}
}


/*
 * populate()
 *
 * Get the disk performance structure from /dev/kmem
 * and adjust the pointers within that structure.
 */

void
populate()

{
	int delta,i;
	struct disk *dp;
	long lseek();

	/*
	 * read the individual disk structures
	 */
	for (i=0; i < Sd01_diskcnt; i++) {
		lseek(f, *(dsd01_p + i), 0);
		if (read(f, dsd01 + i, sizeof(struct disk)) == -1) {
			extmsg("cannot read SD01 data structure");
		}
	}


	/*
	 * The disk performance queue is managed by pttrack, which
	 * points to the next entry to be used.  After the pdi
	 * structure is read from /dev/kmem
	 * pttrack is garbage since it points back to a
	 * location in the kernel data space.  Another pointer,
	 * endptrack, has been provided to assist in calculating
	 * the new value of pttrack.  Endptrack points to one element
	 * past the last element of ARRAY ptrackq (NOT the logical
	 * end of the CICULAR QUEUE that is based on that array).  To
	 * update pttrack, compute the difference between the new and
	 * old endptrack and increase pttrack by this delta amount.
	 */

	if (ALL) {
		for (i=0; i < Sd01_diskcnt; i++) {
			if (dsd01[i].dk_state > 0)
				Sdrvlist[i] = 1;
		}
	}

	for (i=0; i < SNDRIVE; i++) {
		if ( (Sdrvlist[i] == 1) && ( i < Sd01_diskcnt) ) {
			if (dsd01[i].dk_state == 0) {
				fprintf(stderr,"sd01-%d is not equipped\n",i);
				exit(4);
			}
		}
		else 
		{
		if ( Sdrvlist[i] == 1) {
			fprintf(stderr,"sd01-%d is not equipped\n",i);
			exit(4);
		}
		}
	}

	for (i=0; i < Sd01_diskcnt; i++) {
		if (Sdrvlist[i] == 0)
			continue;
	delta =  ((long) &(dsd01[i].dk_stat.ptrackq[NTRACK]) - (long)dsd01[i].dk_stat.endptrack);
	dsd01[i].dk_stat.pttrack = (scsiptrk_t *)(((int)dsd01[i].dk_stat.pttrack) + delta);
	}
}


/*
 * extract_info()
 *
 * Sample the disk performance data and save it in the drive_info structure.
 */

void
extract_info()

{
	register long b_blkno;	/* Number of block that was accessed.	*/
	long cylinder;	/* Cylinder number calculated from b_blkno.	*/
	long prev_cyl;	/* The cylinder number of the previous entry;	*/
			/* used in calculating the seek distance.	*/
	int seek_dist;	/* Calculated seek distance.			*/
	int all_new_entries;	/* Are all queue entries new?		*/
	register int index;	/* Index into the ptrackq array		*/
	int front_index;	/* Index of the front of the queue	*/
	int i;

	for (i=0; i < Sd01_diskcnt; i++) {
		if (Sdrvlist[i] == 0)
			continue;

	front_index = dsd01[i].dk_stat.pttrack - dsd01[i].dk_stat.ptrackq;
	
	/* If the queue is still empty, skip this drive. */
	if (front_index == 0)
		continue;
	
	/*
	 * If the contents of the entry at the previous end of the
   	 * queue have been overwritten since our last probe, then
	 * the queue has completely wrapped around and all NTRACK
	 * entries are new.
	 */
	if (drive_info[i].last_time.b_blkno != dsd01[i].dk_stat.ptrackq[drive_info[i].last_time.end_index].b_blkno)
	{
		/*
		 * The contents of the previous end of the queue have changed.
		 * All entries are new.  Start at the present front of the
		 * queue and probe all NTRACK entries.
		 */
		index = front_index % NTRACK;
		all_new_entries = 1;
	}
	else	/* The contents of the last end of the queue are unchanged.  */
		if (front_index == (drive_info[i].last_time.end_index + 1))
			/*
			* The front of the queue is still at the same
			* index.  So, no new entries have been added.
			*/
			continue;	/* Skip this drive. */
		else	{
			/*
			 * Some entries have been added but not enough to over-
			 * write the whole queue. Start at the first new entry.
			 */
			index = drive_info[i].last_time.end_index;
			all_new_entries = 0;
		}
	/*
	 * Index now points to the last probed entry of last time.
	 * Find out the cylinder number of that entry so we can calculate
	 * the seek distance to first entry to be probed this time.
	 */
	b_blkno = dsd01[i].dk_stat.ptrackq[index].b_blkno;
	/*
	 * Di_sectors contains the number of sectors (blocks) per
	 * track and di_tracks has the number of tracks per cylinder.
	 */
	prev_cyl = (b_blkno / dsd01[i].dk_pdsec.sectors) / dsd01[i].dk_pdsec.tracks;

	/*
	 * Normally the front of the queue is an entry that is either
	 * already probed or is invalid.  But if all the entries are new,
	 * the front of the queue points to a valid entry that has not been
	 * seen.  So, increment the access count of the cylinder in the
	 * entry at the front of the queue even though we cannot calculate
	 * a seek distance to it since there is no valid previous entry for it.
	 */
	if (all_new_entries)
		drive_info[i].chunk_accessed[prev_cyl / CYLS_PER_CHUNK]++;
	
	while (++index != front_index)	{
		index = index % NTRACK;
		b_blkno = dsd01[i].dk_stat.ptrackq[index].b_blkno;
		cylinder = (b_blkno / dsd01[i].dk_pdsec.sectors) / dsd01[i].dk_pdsec.tracks;
		drive_info[i].chunk_accessed[cylinder / CYLS_PER_CHUNK]++;
		seek_dist = cylinder - prev_cyl;
		if (seek_dist < 0)
			seek_dist = -seek_dist;
		drive_info[i].seek_dist[(seek_dist + CYLS_PER_CHUNK - 1) / CYLS_PER_CHUNK]++;
		prev_cyl = cylinder;
	}
	drive_info[i].last_time.end_index = index - 1;
	drive_info[i].last_time.b_blkno = b_blkno;
	}
}


/*
 * print_results()
 *
 * Print the results of monitoring in histogram or tabular form.  The
 * report has two parts: cylinder profile and seeking distance profile.
 */

void
print_results()

{

	int uname();
	char *ctime();
	long time();
	void Scylhdr(), Scylftr();
	
	long current_time;
	struct utsname name;
	int max_chunk;		/* The highest chunk number (or seek	*/					/* distance) for which data exists.	*/
	register struct drive_information *drive_ptr;
	register int j, k;   /* Loop counters for PEs, drives, and	*/
				/* chunks of cylinders, respectively.	*/

	current_time = time((long *) 0);
	printf("\n\n%s\n",ctime(&current_time));
	if (uname(&name) == -1)
		extmsg("Cannot get the name of the system from uname()");
	else
		printf("%s %s %s %s %s\n", name.sysname, name.nodename, name.release, name.version, name.machine);
	/*
	 * Sum up the number of accesses and seek distances for all
	 * chunks of each drive under investigation.
	 */
	for (j = 0; j < Sd01_diskcnt; j++)	{
		if (Sdrvlist[j] == 0)
			continue;   /* Not interested in this drive.*/
		for (k = 0; k < CHUNKS; k++)	{
			drive_info[j].total_accesses += drive_info[j].chunk_accessed[k];
			drive_info[j].sum_seeks += drive_info[j].seek_dist[k];
			}
	}
	if ((tflg == 0) && (hflg == 0))
		tflg = 1;
	if (tflg)	{
		printf("\nCYLINDER ACCESS PROFILE\n");
		for (j = 0; j < Sd01_diskcnt; j++)	{
			if (Sdrvlist[j] == 0)
				continue;
			drive_ptr = &(drive_info[j]);
			if (drive_ptr->total_accesses != 0)	{
				printf("\n%s-%1d:\n", devnm[0], j);
				printf("Cylinders\tTransfers\n");
				for (k = 0; k < CHUNKS; k++)
					if (drive_ptr->chunk_accessed[k] > 0)
						printf("%3d - %3d\t%ld\n", k * CYLS_PER_CHUNK, k * CYLS_PER_CHUNK + CYLS_PER_CHUNK - 1, drive_ptr->chunk_accessed[k]);
				printf("\nSampled I/O = %ld, Actual I/O = %ld\n",
					drive_ptr->total_accesses,drive_ptr->io_ops);
				if (drive_ptr->io_ops > 0)
					printf("Percentage of I/O sampled = %2.2f\n",
					((float)drive_ptr->total_accesses /(float)drive_ptr->io_ops) * 100.0);
				}
			}

		printf("\n\n\nSEEK DISTANCE PROFILE\n");
		for (j = 0; j < Sd01_diskcnt; j++)	{
			if (Sdrvlist[j] == 0)
				continue;
			drive_ptr = &(drive_info[j]);
			if (drive_ptr->sum_seeks != 0)	{
				printf("\n%s-%1d:\n", devnm[0], j);
				printf("Seek Distance\tSeeks\n");
				for (k = 0; k < CHUNKS; k++)
					if (drive_ptr->seek_dist[k] > 0)	{
						if (k == 0)
							printf("        0\t%ld\n", drive_ptr->seek_dist[k]);
						else
							printf("%3d - %3d\t%ld\n", k * CYLS_PER_CHUNK - (CYLS_PER_CHUNK - 1), k * CYLS_PER_CHUNK, drive_ptr->seek_dist[k]);
					}
				printf("Total Seeks = %ld\n",drive_ptr->sum_seeks);
				}
			}
	}
	if (hflg)	{
		/* Print a histogram of the results.	*/
		for (j = 0; j < Sd01_diskcnt; j++)	{
			if (Sdrvlist[j] == 0)
				continue;
			drive_ptr = &(drive_info[j]);
			if (drive_ptr->total_accesses != 0) 	{
				Scylhdr(CYLNO, drive_ptr->total_accesses, j);
				max_chunk = Scylhist(drive_ptr->total_accesses, drive_ptr->chunk_accessed);
				Scylftr(CYLNO, max_chunk);
			}
		}
		for (j = 0; j < Sd01_diskcnt; j++)	{
			if (Sdrvlist[j] == 0)
				continue;
			drive_ptr = &(drive_info[j]);
			if (drive_ptr->sum_seeks != 0)	{
				Scylhdr(SEEKD, drive_ptr->sum_seeks, j);
				max_chunk = Scylhist(drive_ptr->sum_seeks, drive_ptr->seek_dist);
				Scylftr(SEEKD, max_chunk);
			}
		}
	}
}


struct SHISTDATA	{
	long    hdata[CHUNKS];
};	/* Used by the histogram printing functions.	*/

/*      the following routines are obtained from iostat */
/* cylinder profiling histogram */
/*.s'cylhist'Output Cylinder Histogram'*/
int
Scylhist(at, dp)
long at;
register struct SHISTDATA *dp;

{
	register ii;
	int maxrow;
	long graph[CHUNKS];
	long    max, max2;
	long    data;
	long    scale;
	void	Sprthist();

	max = 0;
	for (ii = 0; ii < CHUNKS; ii++)	{
		if (data = dp->hdata[ii])	{
			maxrow = ii;
			if (data > max)	{
				max2 = max;
				max = data;
			} 
			else if (data > max2 && data != max)
				max2 = data;
		}
	}
	maxrow++;

	/* determine scaling */
	scale = 1;
	if ( max2 )	{
		scale = at / ( max2 * 2 );
		if ( scale > 48 )
			scale = 48;
		}

	for (ii = 0; ii < maxrow; ii++)	{
		if (dp->hdata[ii])
			graph[ii] = (scale * 100 * dp->hdata[ii]) / at;
		else
			graph[ii] = -1;
	}

	Sprthist(graph, maxrow, scale, (long) (max*100*scale/at));
	return (maxrow);
}


/*.s'prthist'Print Histogram'*/

void
Sprthist(array, mrow, scale, gmax)
	long array[], scale, gmax;
register mrow;

{
	long    line;

	line = 50;
	/* handle overflow in scaling */
	if (gmax > 51)	{
		line = 52;
		printf("\n%2ld%% -|", gmax/scale);
		pline(line--, array, mrow, BLOB);
		printf("\n     %c", BRK);
		pline(line--, array, mrow, BRK);
	} 
	else if ( gmax = 51 )
		line = 51;
	while ( line > 0)	{
		if ((line & 07) == 0)	{
			printf("\n%2ld%% -|", line/scale);
		} 
		else	{
			printf("\n     |");
		}
		pline(line--, array, mrow, BLOB);
	}
	printf("\n 0%% -+");
	line = -1;
	pline( line, array, mrow, FOOT);
}


/*.s'cylhdr'Print Cylinder Profiling Headers'*/

void
Scylhdr( flag, total, drive_num)
int flag;
long total;
int drive_num;

{
	if ( flag == CYLNO)
		printf("\nCYLINDER ACCESS HISTOGRAM\n");
	if (flag == SEEKD)
		printf("\nSEEK DISTANCE HISTOGRAM\n");
	printf("\n%s-%1d:\n", devnm[0], drive_num);
	printf("Total %s = %ld\n", flag == CYLNO ? "transfers" : "seeks", total);
}
/*.s'cylftr'Print Histogram Footers'*/

void
Scylftr(flag, max_chunk)
register int flag, max_chunk;

{
	register int i;
	char number[5];

	if (flag == CYLNO)
		printf("\n      \t\t\tCylinder number, granularity=%d", CYLS_PER_CHUNK);
	else	{
		printf("\n      =<<  <");
		for (i = 200; i <= max_chunk * CYLS_PER_CHUNK; i += 100)
			printf("    <");
		}
	/*
	 * The following code works only for CYLS_PER_CHUNK = 20 and
	 * has to be slightly modified for other CYLS_PER_CHUNK sizes.
	 */
	printf("\n      024  1");
	for (i = 200; i <= max_chunk * CYLS_PER_CHUNK; i += 100)	{
		sprintf(number, "%d", i);
		printf("    %c", number[0]);
	}
	printf("\n       00  0");
	for (i = 200; i <= max_chunk * CYLS_PER_CHUNK; i += 100)	{
		sprintf(number, "%d", i);
		printf("    %c", number[1]);
	}
	printf("\n           0");
	for (i = 200; i <= max_chunk * CYLS_PER_CHUNK; i += 100)	{
		sprintf(number, "%d", i);
		printf("    %c", number[2]);
	}
	printf("\n            ");
	for (i = 200; i <= max_chunk * CYLS_PER_CHUNK; i += 100)	{
		sprintf(number, "%d", i);
		printf("    %c", (number[3] == '\0') ? ' ' : number[3]);
	}
	printf("\n");
}

/*
 * ginfo: read info from kernel using ioctl on /dev/kmem
 * Restrictions:
 *		ioctl(2) none
 */
int
ginfo(fd,name,buf,buflen,cmd)
int fd,cmd;
size_t buflen;
void *buf;
char *name;
{
	struct mioc_rksym rks;
	rks.mirk_symname = name;
	rks.mirk_buf = buf;
	rks.mirk_buflen = buflen;
	return(ioctl(fd,cmd,&rks));
}
