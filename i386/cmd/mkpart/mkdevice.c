/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mkpart:i386/cmd/mkpart/mkdevice.c	1.1.1.4"
#ident "$Header: mkdevice.c 1.2 91/07/10 $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/alttbl.h>
#include <sys/fdisk.h>
#include <sys/vtoc.h>
#include "mkpart.h"
#include "parse.h"
#include <sys/sdi_edt.h>

#define SCSIFORMAT "scsiformat"
#define SCSIVERIFY "scsiformat"
#define SCSIALTS   "scsihdfix"
#define SCSICMDS   "/etc/scsi"

/* First_alt - Flag.
 *	If value is 1, we are establishing the alternates table
 *	for the first time.  in that case, DON'T use the V_ADDBAD
 *	ioctl, but DO update table on disk, directly, and use ioctl
 *	with V_REMOUNT to inform the driver to pick up the new table.
*/
short	int	First_alt=0;

struct	disk_parms	dp;	/* device parameters	*/
struct	ipart	*unix_part,	/* fdisk partition table	*/
		fd_parts[FD_NUMPART];
struct	pdinfo	pdinfo;		/* physikal device info array	*/
struct	vtoc	vtoc;		/* table of contents	*/

struct	alt_info alttbl;	/* alternate sector and track tables */
static	daddr_t	verbads[MAX_ALTENTS]; /* bad sectors found in verify	*/

static	daddr_t	pd_sector;	/* sector from which pdinfo is read/written */
	struct	absio	absbuf;	/* for RDABS and WRABS ioctl calls	*/
struct	mboot	mboot;		/* fdisk partition table	*/

extern	node	*addparts,	/* list of partitions to add	*/
		*subparts;	/* list of partitions to remove */

devstanza	*mydev;		/* Device stanza that we work on */
int	devfd;			/* .. and its file descriptor    */

extern	char	*devicestanza;	/* physical device stanza name	*/
extern	char	format;		/* -F flag	*/
extern	char	intlv;		/* interleave factor from cmd line -F flag */
extern	char	initialize;	/* -i flag	*/
extern	char	*partfile;	/* default partition file	*/
extern	char	targ;		/* -t subflag	*/
extern	char	verify;		/* -f flag	*/
extern	int	dev_fd_open;	/* open device flag	*/
extern	int	is_scsi_dev;	/* scsi flag	*/
extern	int	not_for_scsi;	/* not for scsi flag	*/

static	void	addbad();
static  void	remount();
static	int	sec_verify();

/*
 *
 * verifydevice()
 *
 *	verify every sector of the device
 *
 * Parameters: none
 *
 * Return Values: none
 *
 * Exit State:
 *
 *	71: error while seeking sectors
 *
 * Description:
 *
 *	Attempt to read every sector of the drive and add bad sectors found to
 *	list in mydev->ds_badsec.
 *
 */
void
verifydevice()
{
	int	trksiz = dp.dp_secsiz*dp.dp_sectors;
	uchar_t	*verbuf = (uchar_t *)malloc(dp.dp_secsiz*dp.dp_sectors);
	daddr_t cursec;
	daddr_t	lastusec = unix_part->numsect;	/* last UNIX System sector */
	int	dotpos = 0;
	int	badcnt = 0;
	int	i;
	uchar_t	*bptr;
	char	*rptr;
	char	resp[10];

	bptr = verbuf;
	for (i = 0; i < trksiz; i++) 
		*bptr++ = 0xe5;

	if ((verify & VER_WRITE) == 0) 
		goto do_readonly;

	printf("\nCAUTION: ABOUT TO DO DESTRUCTIVE WRITE ON ENTIRE UNIX SYSTEM PARTITION.\n");
	printf("THIS WILL DESTROY ANY DATA ON %s.  Continue (y/n)? ", mydev->ds_device);
	rptr = gets(resp);
	if (!rptr ||  !((resp[0] == 'Y') || (resp[0] == 'y'))) 
		goto do_readonly;

	/* start at 0, relative to begining of UNIX System Partition */
	for (cursec = 0; cursec < lastusec; cursec += dp.dp_sectors) {
		if (lseek(devfd, cursec * dp.dp_secsiz, 0) == -1) {
			fprintf(stderr, "Error seeking sector %ld!\n", cursec);
			exit(71);
		}
		if (write(devfd, verbuf, trksiz) != trksiz) {
			daddr_t tmpsec;
			daddr_t tmpend = cursec + dp.dp_sectors;
			for (tmpsec = cursec; tmpsec < tmpend; tmpsec++) {
				int	tmptry;
				for (tmptry = 0; tmptry < 5; tmptry++) {
					if (lseek(devfd, tmpsec * dp.dp_secsiz, 0) == -1) {
						fprintf(stderr, "Error seeking sector %ld!\n",
						    tmpsec);
						exit(71);
					}
					if (write(devfd, verbuf, dp.dp_secsiz) != dp.dp_secsiz) {
						addbad(&badcnt, unix_part->relsect + tmpsec);
						break;
					}
				}
			}
		}
		fprintf(stderr, "%6ld\b\b\b\b\b\b", cursec);
	}
	fprintf(stderr, "\n");
	dotpos = 0;

do_readonly:
	for (cursec = 0; cursec < lastusec; cursec += dp.dp_sectors) {
		if (lseek(devfd, cursec * dp.dp_secsiz, 0) == -1) {
			fprintf(stderr, "Error seeking sector %ld!\n", cursec);
			exit(71);
		}
		if (read(devfd, verbuf, trksiz) != trksiz) {
			daddr_t tmpsec;
			daddr_t tmpend = cursec + dp.dp_sectors;
			for (tmpsec = cursec; tmpsec < tmpend; tmpsec++) {
				int	tmptry;
				for (tmptry = 0; tmptry < 5; tmptry++) {
					if (lseek(devfd, tmpsec * dp.dp_secsiz, 0) == -1) {
						fprintf(stderr, "Error seeking sector %ld!\n",
						    tmpsec);
						exit(71);
					}
					if (read(devfd, verbuf, dp.dp_secsiz) != dp.dp_secsiz) {
						addbad(&badcnt, unix_part->relsect + tmpsec);
						break;
					}
				}
			}
		}
		fprintf(stderr, "%6ld\b\b\b\b\b\b", cursec);
	}
	fprintf(stderr, "\n");
	if (badcnt > 0) {
		printf("Bad sectors found during verify pass are:\n");
		for (i = 0; i < badcnt; ) {
			printf("\t%ld", verbads[i]);
			if (((i++ % 4) == 0) && (i != 1))
				printf("\n");
		}
		printf("\nVerify complete.\n");
	}
}


/*
 *
 * addbad ()
 *
 *	adds sector to verbads and list from mydev->ds_badsec IFF it currently
 * 	isn't in verbads.
 *
 * Parameters:
 *
 *	cntptr:	counter of bad blocks of the disk
 *	secno:	sector number of the bad block
 *
 * Return Values: none
 *
 * Exit State:
 *
 *	72: to many bad sectors on drive
 *
 */

static	void
addbad(cntptr, secno)
int	*cntptr;
daddr_t secno;
{
	node    * lp, *np;
	int	i;

	for (i = 0; i < *cntptr; i++) {
		if (verbads[i] == secno)
			return;
	}
	if (*cntptr > MAX_ALTENTS) {
		fprintf(stderr, "TOO MANY BAD SECTORS ON DRIVE!\n");
		exit(72);
	}
	verbads[*cntptr++] = secno;
	lp = newnode(LIST);
	np = newnode(NUMBER);
	lp->ListRef = (void * )np;
	np->Number = secno;
	lp->ListNext = mydev->ds_badsec;
	mydev->ds_badsec = lp;
}


/*
 *
 * updatealts ()
 *
 *	update the Alternate Sector Table,
 *	adds or changes bad block records
 *
 * Parameters: none
 *
 * Return Values: none
 *
 *	If alternate table already is established or device is a scsi
 *	device, do nothing.
 *
 * Exit States:
 *
 *	54: V_ADDBAD ioctl call failed
 *	61: Range is illegal for bad sector specification
 *	62: The bad sector os an assigned alternate
 *	63: Insufficient alternates available for a bad sector
 *	69: The bad sector is past the end of the drive
 * 
 * Description:
 *
 *	UpdateAlts
 *	Update the Alternate Sector Table (alttbl).  At this point, 
 *	mydev->ds_badsec *	points to a list of bad sectors.  These come 
 *	from 3 places:
 *	  - from the device stanza,
 *	  - from -A arguments, and
 *	  - from any sectors found defective during the 'format' surface 
 *	analysis. Alttbl has any entries which were in the alternates table as 
 *	read from the disk (if any) plus sectors from any alternates partitions 
 *	which were added during this run. These new ones (at least) have not 
 *	been verified as being useable. We first look through the bad sector 
 *	list to see if any show up as being alternates. If we find one and it 
 *	isn't assigned yet, just remove it from the table (this may result in 
 *	an incompletely filled alternates table, but rarely -- big deal). If we
 *	find one which is already assigned, we complain (the only solution to 
 *	this is to reformat and re-build everything).  After getting a good 
 *	list of alternates, we look through the bad sector list again. If we've
 *	already assigned an alternate for a bad one, all's ok.  If not, assign 
 *	the next available alternate (if none left, complain).  Then try the 
 *	V_ADDBAD call to add the baddy to the incore alternates table.  This 
 *
 *	may not work if we've just added the first alternates partition or a 
 *	previous one was full.  In this case, warn the user that he should 
 *	reboot if the V_REMOUNT call fails.
 *	We also initialize the alternate track table from mydev->ds_badtrk;
 *	this list was read in from the device stanza.
 *
 *	For MX300I non scsi devices getblockzero returns the first block 
 *	of the named hard disk. hd_serve prints and/or changes a bad block 
 *	record for a pack (at an Interphasse Storager). 
 *	Use this part of code only in single user mode.
 *	
 */

void
updatealts()
{
	node * badptr;
	daddr_t curbad;
	ushort	i;

	/** NOTE: NEVER CALLED FOR SCSI! **/
	long	maxsec = (long)dp.dp_heads *dp.dp_cyls *dp.dp_sectors;

	if (!First_alt || is_scsi_dev)
		return;

	if ((alttbl.alt_sec.alt_reserved == 0) && mydev->ds_badsec) {
		fprintf(stderr,
		    "Warning: No alternates partition in VTOC for device %s\n",
		    mydev->ds_device);
		fprintf(stderr, "         Bad sectors will not be marked!\n");
		goto do_tracks;
	}
	for (badptr = mydev->ds_badsec; badptr; badptr = badptr->ListNext) {
		if (badptr->ListElem->token == RANGE) {
			fprintf(stderr, "RANGE illegal for bad sector specification\n");
			exit(61);
		}
		curbad = badptr->ListElem->Number;
		if (!First_alt) {
			union io_arg new_badblk,
			*newbad = &new_badblk;

			newbad->ia_abs.bad_sector = curbad;
			if (ioctl(devfd, V_ADDBAD, newbad) == -1) {
				fprintf(stderr,
				    "Failed V_ADDBAD on bad block %#lx\n",
				    curbad);
				perror("Unable to assign alternate");
				exit(54);
			}
#ifdef DEBUG
			printf("Bad sector: %#lx assigned to alternate: %#lx.\n",
			    curbad, newbad->ia_abs.new_sector);
#endif
			break;	/* driver will do all the checking */
		}
		if (curbad >= maxsec) {
			fprintf(stderr,
			    "Bad sector %ld is past the end of the drive.\n",
			    curbad);
			exit(69);
		}
		/* if unused alternate is bad, then excise it from the list. */
		for (i = alttbl.alt_sec.alt_used; i < alttbl.alt_sec.alt_reserved; 
		    i++) {
			if (alttbl.alt_sec.alt_base + i == curbad) {
				alttbl.alt_sec.alt_bad[i] = -1;
				if (i == alttbl.alt_sec.alt_used) {
					while (++alttbl.alt_sec.alt_used
					     < alttbl.alt_sec.alt_reserved)
						if (alttbl.alt_sec.alt_bad[alttbl.alt_sec.alt_used]
						     != -1)
							break;
				}
				break;
			}
		}
		/* if used alternate is bad, give up */
		for (i = 0; i < alttbl.alt_sec.alt_used; i++) {
			if (alttbl.alt_sec.alt_base + i == curbad) {
				fprintf(stderr,
				    "Bad sector %ld is an assigned alternate!\n",
				    curbad);
				exit(62);
			}
		}
	}

	if (!First_alt)
		return;	/* bad blocks already submitted via ioctl() */

	for (badptr = mydev->ds_badsec; badptr; badptr = badptr->ListNext) {
		curbad = badptr->ListElem->Number;
		/* don't map alternates */
		if (curbad >= alttbl.alt_sec.alt_base && curbad < 
		    alttbl.alt_sec.alt_base + alttbl.alt_sec.alt_reserved)
			continue;
		/* check if bad block already mapped (already in list) */
		for (i = 0; i < alttbl.alt_sec.alt_used; i++)
			if (alttbl.alt_sec.alt_bad[i] == curbad) 
				break;
		if (i == alttbl.alt_sec.alt_used) {	/* this is a new bad block */
			if (alttbl.alt_sec.alt_used >= alttbl.alt_sec.alt_reserved) {
				fprintf(stderr,
				    "Insufficient alternates available for bad sector %ld!\n",
				    curbad);
				exit(63);
			}
			alttbl.alt_sec.alt_bad[alttbl.alt_sec.alt_used] = curbad;
			while (++alttbl.alt_sec.alt_used < alttbl.alt_sec.alt_reserved)
				if (alttbl.alt_sec.alt_bad[alttbl.alt_sec.alt_used]
				     != -1)
					break;
			/* TRY THE V_ADDBAD HERE */
		}
	}

do_tracks:
	/* Put bad tracks into bad track table */
	if ((alttbl.alt_trk.alt_reserved == 0) && mydev->ds_badtrk) {
		fprintf(stderr,
		    "Warning: No alternate track partition in VTOC for device %s\n",
		    mydev->ds_device);
		fprintf(stderr, "         Bad tracks will not be marked!\n");
		return;
	}
	for (badptr = mydev->ds_badtrk; badptr; badptr = badptr->ListNext) {
		if (badptr->ListElem->token == RANGE) {
			fprintf(stderr,
			    "RANGE illegal for bad track specification\n");
			exit(61);
		}
		curbad = badptr->ListElem->Number;
		if (curbad >= maxsec / dp.dp_sectors) {
			fprintf(stderr,
			    "Bad track %ld is past the end of the drive.\n",
			    curbad);
			exit(69);
		}
		/* if unused alternate is bad, then excise it from the list. */
		for (i = alttbl.alt_trk.alt_used; i < alttbl.alt_trk.alt_reserved; i++) {
			if (alttbl.alt_trk.alt_base / dp.dp_sectors+i == curbad) {
				alttbl.alt_trk.alt_bad[i] = -1;
				if (i == alttbl.alt_trk.alt_used) {
					while (++alttbl.alt_trk.alt_used
					     < alttbl.alt_trk.alt_reserved)
						if (alttbl.alt_trk.alt_bad[alttbl.alt_trk.alt_used]
						     != -1)
							break;
				}
				break;
			}
		}
		/* if used alternate is bad, give up */
		for (i = 0; i < alttbl.alt_trk.alt_used; i++) {
			if (alttbl.alt_trk.alt_base / dp.dp_sectors+i == curbad) {
				fprintf(stderr,
				    "Bad track %ld is an assigned alternate!\n",
				    curbad);
				exit(62);
			}
		}
	}

	for (badptr = mydev->ds_badtrk; badptr; badptr = badptr->ListNext) {
		curbad = badptr->ListElem->Number;
		/* don't map alternates */
		if (curbad >= alttbl.alt_trk.alt_base / dp.dp_sectors && 
		    curbad < (alttbl.alt_trk.alt_base / dp.dp_sectors
		     + alttbl.alt_trk.alt_reserved))
			continue;
		/* check if bad track already mapped (already in list) */
		for (i = 0; i < alttbl.alt_trk.alt_used; i++)
			if (alttbl.alt_trk.alt_bad[i] == curbad) 
				break;
		if (i == alttbl.alt_trk.alt_used) {	/* this is a new bad track */
			if (alttbl.alt_trk.alt_used >= alttbl.alt_trk.alt_reserved) {
				fprintf(stderr,
				    "Insufficient alternates available for bad track %ld!\n",
				    curbad);
				exit(63);
			}
			alttbl.alt_trk.alt_bad[alttbl.alt_trk.alt_used] = curbad;
			while (++alttbl.alt_trk.alt_used < alttbl.alt_trk.alt_reserved)
				if (alttbl.alt_trk.alt_bad[alttbl.alt_trk.alt_used]
				     != -1)
					break;
		}
	}
}


/*
 *
 * getdevice (name)
 *
 *	gets pdinfo, vtoc and alttbl data from the device
 *
 *
 * Parameters:
 *	
 *	name: name of the device
 *
 * Return Values:
 *
 *	true:  could get valid disk structs of device
 *	false: un-vtoc-ed device
 *
 * Exit State:
 *
 *	66: configure drive request failed
 *	67: second GETPARMS ioctl on drive failed
 *	69: malloc of buffer failed
 *
 * Description:
 *
 *	Verifies that the device specified is a character special.  Then it
 *	gets device info from V_GETPARMS ioctl into global dp.
 *	Read data into global structs pdinfo, vtoc, and alttbl.  If the device 
 *	isn't char special or the V_GETPARMS fails, give a message and quit.  
 *	Otherwise, if a seek or read fails, fill in all of the structs with 
 *	default values built (or calculated) from dp.
 *
 */

int
getdevice(name)
char	*name;
{
	int	i;
	char	*buf;

	if (!dev_fd_open) {
		if (mkpartopen(name))
			exit(1);
	}

	if (ioctl(devfd, V_GETPARMS, &dp) == -1) {
		fprintf(stderr, "GETPARMS on ");
		perror(name);
		exit(1);
	}

	/*
	 * if the parameters returned from the controller don't agree with
	 * the values in the device stanza, attempt to V_CONFIG the device
	 * and re-get parameters.
	 * NOTE: If we're going to create a partitions-type file (-x flag)
	 * don't do any of this, 'cuz user may not be sure what things
	 * really look like!
	 */
	if (!format && !(targ & TF_WPART) && 
	    ((mydev->ds_heads != dp.dp_heads) || 
	    (mydev->ds_cyls != dp.dp_cyls) || 
	    (mydev->ds_sectors != dp.dp_sectors) || 
	    (mydev->ds_bpsec != dp.dp_secsiz))) {
		union io_arg ia;

		fprintf(stderr,
		    "WARNING: device stanza parameters do not match driver values!\n");
		fprintf(stderr, "Attempting to reconfigure drive.\n");
		ia.ia_cd.ncyl = mydev->ds_cyls;
		ia.ia_cd.nhead = mydev->ds_heads;
		ia.ia_cd.nsec = mydev->ds_sectors;
		ia.ia_cd.secsiz = mydev->ds_bpsec;
		if (ioctl(devfd, V_CONFIG, &ia) == -1) {
			fprintf(stderr,
			    "Configure Drive request FAILED!\n");
			exit(66);
		}
		if (ioctl(devfd, V_GETPARMS, &dp) == -1) {
			fprintf(stderr, "second GETPARMS on ");
			perror(name);
			exit(67);
		}
	}

	/*
	 * If we're supposed to format the disk, we have all the info
	 * we need, (V_PARM), so just return.
	*/
	if (format) 
		return 0;

	if ((buf = (char *)malloc(dp.dp_secsiz)) == NULL) {
		fprintf(stderr, "mkpart: malloc of buffer failed\n");
		exit(69);
	}

	pd_sector = mydev->ds_vtocsec;
	absbuf.abs_sec = 0;
	absbuf.abs_buf = (char *) & mboot;
	if ( ((i = ioctl(devfd, V_RDABS, &absbuf)) == -1) || 
	    (mboot.signature != MBB_MAGIC) ) {
		if (i >= 0)
			fprintf(stderr, "Error: invalid fdisk partition table found\n");
		else {
			fprintf(stderr, "Reading fdisk partition table: ");
			perror(name);
		}
		exit(1);
	}
	/* copy the partition stuff into fd_parts */
	COPY(fd_parts[0], mboot.parts[0], sizeof(struct ipart ) * FD_NUMPART);

	/* find an active UNIX System partition */
	unix_part = NULL;
	for (i = 0; i < FD_NUMPART; i++) {
		if (fd_parts[i].systid == UNIXOS) {
			if (fd_parts[i].bootid == ACTIVE || unix_part == NULL)
				unix_part = (struct ipart *) & fd_parts[i];
		}
	}
	if (unix_part == NULL) {
		fprintf(stderr, "No UNIX System partition in fdisk table!\n");
		exit(1);
	}

	if (initialize) {	/* Make sure we don't have an old pdinfo */
		absbuf.abs_sec = unix_part->relsect + pd_sector;
		((struct pdinfo *)absbuf.abs_buf)->sanity = 0;
		ioctl(devfd, V_WRABS, &absbuf);
		remount();	/* make sure driver knows about it */
	}

	/*
	 * If we're supposed to initialize drive, don't even bother trying
	 * to read structures from the disk.
	 */

	if (initialize) 
		goto makedflt;

	if ( (lseek(devfd, dp.dp_secsiz * pd_sector, 0) == -1) || 
	    (read(devfd, buf, dp.dp_secsiz) == -1)	) {
		fprintf(stderr, "Warning: seeking/reading pdinfo: ");
		perror(name);
		goto makedflt;
	}
	COPY(pdinfo, buf[0], sizeof(pdinfo));
	if (pdinfo.sanity != VALID_PD) {
		fprintf(stderr, "Warning: invalid pdinfo block found -- initializing.\n");
		goto makedflt;
	}
	if ( (lseek(devfd, (pdinfo.vtoc_ptr / dp.dp_secsiz) * dp.dp_secsiz, 0) == -1) || 
	    (read(devfd, buf, dp.dp_secsiz) == -1)	) {
		fprintf(stderr, "Warning: seeking/reading VTOC: ");
		perror(name);
		goto makedflt;
	}
	COPY(vtoc, buf[pdinfo.vtoc_ptr%dp.dp_secsiz], sizeof(vtoc));
	if (vtoc.v_sanity != VTOC_SANE) {
		fprintf(stderr, "Warning: invalid VTOC found -- initializing.\n");
		goto makedflt;
	}

	/* The following is okay for both scsi and esdi */
	/* enlarge 'buf' to hold largest possible alternates table */
	if ((buf = (char *)realloc(buf, sizeof(alttbl))) == NULL) {
		fprintf(stderr, "mkpart: realloc of buffer failed\n");
		exit(69);
	}

	/** Do not attempt any alt handling for Scsi. Instead, NULL the **/
	/** ptr and set the altlen to 0.			  	**/

	if (!is_scsi_dev) {
		if ( (lseek(devfd, (pdinfo.alt_ptr / dp.dp_secsiz) * dp.dp_secsiz, 0) == -1) || 
		    (read(devfd, buf, sizeof(alttbl)) == -1)	) {
			fprintf(stderr, "Warning: seeking/reading alternates table: ");
			perror(name);
			goto makedflt;
		}
		COPY(alttbl, buf[pdinfo.alt_ptr%dp.dp_secsiz], sizeof(alttbl));
		if (alttbl.alt_sanity != ALT_SANITY || 
		    alttbl.alt_version != ALT_VERSION) {
			fprintf(stderr, "Warning: invalid alternates table found -- initializing.\n");
			goto makedflt;
		}
	} else
	{
		/** SCSI **/
		pdinfo.alt_ptr = (long)0;
		pdinfo.alt_len = 0;
	}
	return 1;	/* Successfully read all structures */

makedflt:
	/*
	 * Fill in all of the data structures as best as can be done.  Since
	 * the device has no info out there, we assume that the user will
	 * be specifying this stuff later on from device and partition stanzas.
	 */

	pd_sector = mydev->ds_vtocsec;
	/* Initialize pdinfo structure */
	pdinfo.driveid = 0;		/* reasonable default value	*/
	pdinfo.sanity = VALID_PD;
	pdinfo.version = V_VERSION;
	strncpy(pdinfo.serial, "            ", sizeof(pdinfo.serial));
	pdinfo.cyls = dp.dp_cyls;
	pdinfo.tracks = dp.dp_heads;
	pdinfo.sectors = dp.dp_sectors;
	pdinfo.bytes = dp.dp_secsiz;
	pdinfo.logicalst = dp.dp_pstartsec;
	pdinfo.vtoc_ptr = dp.dp_secsiz * pd_sector + sizeof(pdinfo);
	pdinfo.vtoc_len =  sizeof(vtoc);


	/** Zero the alt_ptr and alt_len for scsi **/
	if (!is_scsi_dev) {
		pdinfo.alt_ptr = dp.dp_secsiz * (pd_sector + 1);
		pdinfo.alt_len = sizeof(alttbl);
	} else
	{
		pdinfo.alt_ptr = (long)0;
		pdinfo.alt_len = 0;
	}

	/* Initialize vtoc */
	vtoc.v_sanity = VTOC_SANE;
	vtoc.v_version = V_VERSION;
	strncpy(vtoc.v_volume, mydev->ds_device, sizeof(vtoc.v_volume));
	vtoc.v_nparts = 1;
	vtoc.v_part[0].p_tag = V_BACKUP;
	vtoc.v_part[0].p_flag = V_UNMNT | V_VALID;
	vtoc.v_part[0].p_start = unix_part->relsect;
	vtoc.v_part[0].p_size = unix_part->numsect;

	/** Zero the new SCSI timstamp element **/
	/** Do it for both scsi and esdi in    **/
	/** case esdi wants to do something    **/
	/** with it later.		       **/

	vtoc.timestamp[0] = (long)0;

	/* Build empty alternates table for ESDI ONLY */

	if (!is_scsi_dev) {
		memset((char *) & alttbl, 0, sizeof(alttbl));
		alttbl.alt_sanity = ALT_SANITY;
		alttbl.alt_version = ALT_VERSION;
	}
	return 0;	/* couldn't get real data, built defaults */
}


/*
 *
 * formatdevice()
 *
 *	formats the device
 *
 * Parameters: none
 *
 * Return Values: none
 *
 * Exit State: none
 *
 * Description:
 *
 *	Special case for low-level format. The 'device' is an actual
 *	UNIX System device name (e.g. /dev/rdsk/0s0) instead of a device
 *	stanza reference.
 *	Formatting will erase the fdisk table in abs sector 0. 
 *
 */

void
formatdevice()
{
	long	numtrks = dp.dp_heads *dp.dp_cyls;
	long	curtrk;
	daddr_t cursec;
	union io_arg ia;
	int	retrycnt;
	ushort	i;
	char	resp[10];
	char	*rptr;

	short	knt_bad = 0,
	knt_marked = 0;

	struct {
		unsigned	track : 1;	/* bad track flag */
		unsigned	sector : 1;	/* bad sector flag */
	} bad;

	printf("\nABOUT TO FORMAT ENTIRE DRIVE.  THIS WILL DESTROY ANY DATA\n");
	printf("on %s.  Continue (y/n)? ", devicestanza);
	rptr = gets(resp);
	if (!rptr || !((resp[0] == 'Y') || (resp[0] == 'y'))) 
		return;

	fprintf(stderr, "\nAttempting to format %ld tracks.\n\nFormatting ", numtrks);

	ia.ia_fmt.num_trks  = 1;	/* # sectors to format */
	ia.ia_fmt.intlv = intlv;	/* set interleave */

	for (curtrk = 0; curtrk < numtrks; curtrk++) {
		ia.ia_fmt.start_trk = (ushort) curtrk;

		cursec = curtrk * dp.dp_sectors;

		bad.track = 0;

		/* check for sectors that are marked bad */
		if (sec_verify(cursec, dp.dp_sectors, 1) != 0) {

			/* check for a marked bad block */
			for (bad.track = 1, bad.sector = 0, i = 0; i < dp.dp_sectors; i++) {
				if (sec_verify(cursec + i, 1, 1) != BAD_BLK)
					bad.track = 0;
				else
					bad.sector = 1;

				if ( !bad.track && bad.sector)
					break;
			}
			if (bad.track) {
				++knt_bad;
				fprintf(stderr, "\rTrack %6ld bad        \nFormatting ",
				    curtrk);
			} else if (bad.sector) {
				++knt_marked;
				fprintf(stderr, "\rMarking track %6ld bad\nFormatting ",
				    curtrk);
			}

			if (!bad.track && bad.sector && ioctl(devfd, FMTBAD, &ia) != -1) {
				fprintf(stderr, "\nFormat bad FAILED at track %ld.\n", curtrk);
				bad.track = 1;
			}
		}
		/* else - no bad sectors on track */

		for (retrycnt = 0; !bad.track && retrycnt < 5; retrycnt++) {
			if (ioctl(devfd, V_FORMAT, &ia) != -1)
				break;
		}
		if (retrycnt == 5) {
			fprintf(stderr,
			    "\rFormat request FAILED 5 times at track %ld.\n", curtrk);
		}
		if (retrycnt != 0)
			fprintf(stderr, "Continuing... ");
		fprintf(stderr, "%6ld\b\b\b\b\b\b", curtrk);
	}

	if (knt_bad || knt_marked) {
		fprintf(stderr, "\n\nFound %d complete and %d partial bad tracks. ",
		    knt_bad, knt_marked);
		fprintf(stderr, "Total bad tracks = %d\n", knt_bad + knt_marked);
	}

	fprintf(stderr, "\nFormat complete.\n");
}


/*
 *
 * sec_verify (start_sec, n_sec, n_tries)
 *
 * 	verify sectors for marked bad using V_VERIFY ioctl
 *
 * Parameters:
 *
 *	start_sec: first sector to verify
 *	n_sec:     number of sectors to verify
 *	n_tries:   number of times (if all succeed)
 *
 * Return Values:
 *	returns 0 for success, else
 *	return the error-code from the driver.
 *
 * Exit State:
 *
 *	81: ioctl failed
 *
 */

static	int
sec_verify(start_sec, n_sec, n_tries)
daddr_t	start_sec;
int	n_sec;
int	n_tries;
{
	union vfy_io vfy;

	while (n_tries-- > 0) {
		vfy.vfy_in.abs_sec = start_sec;
		vfy.vfy_in.num_sec = n_sec;
		vfy.vfy_in.time_flg = 0;
		if (ioctl(devfd, V_VERIFY, &vfy) != 0) {
			fprintf(stderr, "\n");
			perror("Verify operation failed");
			exit(81);
		}
		if (vfy.vfy_out.err_code)
			return vfy.vfy_out.err_code;
	}
	return 0;
}


/*
 *
 * builddevice (dev)
 *
 *	Pull all of the values gotten from the user's device stanza(s) into the
 *	various disk structures.
 *
 * Parameters:
 *
 *	dev: device stanza 
 *
 * Return Values: none
 *
 * Exit State: none
 *
 * Remarks:
 *
 *	CURRENTLY WE DO VERY LITTLE VERIFICATION.  Builddevice should do lots of
 * 	validation to keep the user from shooting him/herself in the foot.
 *
 */

void
builddevice(dev)
devstanza *dev;
{
	if (dev->ds_dserial) {
		strncpy(pdinfo.serial, dev->ds_dserial, sizeof(pdinfo.serial));
	}
	pdinfo.tracks = dev->ds_heads;
	dp.dp_cyls = pdinfo.cyls = dev->ds_cyls;
	dp.dp_secsiz = pdinfo.bytes = dev->ds_bpsec;

	pdinfo.vtoc_ptr = dev->ds_vtocsec * dp.dp_secsiz + sizeof(pdinfo);
	pdinfo.vtoc_len = sizeof(vtoc);

	/** Zero the alt variables for SCSI **/

	if (!is_scsi_dev) {
		pdinfo.alt_ptr = dev->ds_altsec * dp.dp_secsiz;
		pdinfo.alt_len = sizeof(alttbl);
	} else
	{
		pdinfo.alt_ptr = (long)0;
		pdinfo.alt_len = 0;
	}
}


/*
 *
 * updatedevice()
 *
 *	dummy function to satisfy mkpart.c
 *
 * Remarks:
 *
 *	Generally, any info that needs to be "bubbled up" from the partitions 
 *	or alternates table should be handled here. Currently, we don't do 
 *	anything.
 */

void
updatedevice()
{
}


/*
 *
 * writevtoc ()
 *
 * 	Write out the updated volume label, pdinfo, vtoc, and alternate table.  
 *
 * Parameters: none
 *
 * Return Values: none
 *
 * Exit State:
 *
 *	51: writing new pdinfo failed
 *	53: writing new alternate block table failed
 *	69: can't allocate buffer
 *
 * Description:
 *
 *	We assume that the pdinfo and vtoc, together, will fit into a single 
 *	BSIZE'ed block. (This is currently true on even 512 byte/block systems;
 *	this code may need fixing if a data structure grows). We are careful 
 *	to read the block that the volume label resides in, and overwrite the 
 *	label at its offset; writeboot() should have taken care of leaving this 
 *	hole.
 *
 */

void
writevtoc()
{
	char	*buf;

	/* We allocate a buffer large enough to accomodate
	 *		the largest object, the alternates list.
	 */

	if ((buf = (char *)malloc(sizeof(alttbl))) == NULL)
	{
		fprintf(stderr, "writevtoc -- can't allocate buffer\n");
		exit(69);
	}
	/*
	 * If we're doing a floppy, we're all done.  Otherwise, put out the
	 * other structs:  pdinfo, vtoc, and alttbl.
	 */
	if ((dp.dp_type == DPT_WINI) || (dp.dp_type == DPT_SCSI_HD) || 
	    (dp.dp_type == DPT_SCSI_OD)) {
		/* put pdinfo & vtoc into the same sector */
		*((struct pdinfo *)buf) = pdinfo;
		*((struct vtoc *) & buf[pdinfo.vtoc_ptr%dp.dp_secsiz]) = vtoc;
		absbuf.abs_sec = unix_part->relsect + pd_sector;
		absbuf.abs_buf = buf;
		if (ioctl(devfd, V_WRABS, &absbuf) != 0) {
			perror("Writing new pdinfo");
			exit(51);
		}

	/*					*/
	/*	now do the alternate table	*/
	/*					*/
	/* With Maria's implementation of automatic BBH, it becomes imperative
	 * that we use the V_ADDBAD ioctl, for non-initialization situations,
	 * instead of the write(), as the write() may cause the disk
	 * and kernel versions of the alttbl to be out of sync.
	 */

		/* only write table to disk on initialization */
		/* DON'T DO THIS FOR SCSI DISKS **/

		if ((First_alt || initialize) && !is_scsi_dev) {		
			/* - otherwise, dynamic bbh takes care of things */
			if ((lseek(devfd, pdinfo.alt_ptr, 0) == -1) || 
			    (write(devfd, (char *) & alttbl, sizeof(alttbl)) != sizeof(alttbl))) {
				perror("Writing new alternate block table");
				exit(53);
			}
		}
	}

	/* make sure things are clean! */
	remount();
	sync();
}


/*
 *
 * remount ()
 *
 *	attempt to remount the device
 *
 * Parameters: none
 *
 * Return Values: none
 *
 * Exit State:
 *
 *	54: remounting failed
 *
 * Description:
 *
 * 	Attempt to remount the device.  Generally, one can expect this to
 * 	fail if somebody has one of the partitions open.  This means that
 * 	the user won't be able to use the new structures until reboot time.
 *
 * Remarks:
 *
 *	Make sure changes to special sectors are recognized by the disk driver.
 */

static	void
remount()
{
	if (ioctl(devfd, V_REMOUNT, NULL) == -1) {
		fprintf(stderr, "Warning:  V_REMOUNT io control failed.  This may result from\n");
		fprintf(stderr, "one or more of the partitions on the disk being open.\n");
		fprintf(stderr, "Disk must be manually re-mounted or system re-booted for\n");
		fprintf(stderr, "any changes in partitions or alternate sectors to be useable.\n");
		exit(54);
	}
	mkpartclose();
	if (mkpartopen(mydev->ds_device)) {
		perror("Can't re-open disk device");
		exit(54);
	}
}


/*
 *
 * updateparts ()
 *
 *	update partitions
 *
 * Return Values: none
 *
 * Exit State: 
 *
 *	6 : The partition stanza is not defined
 *	7 : The partition stanza is not defined
 *	8 : The partition stanza has no partition number
 *	64: Removal of ALTS partition not supported
 *
 * Description:
 *
 * 	Walk through all of the partitions to be removed and remove them.  Alter
 * 	the partition count to reflect those removed at the end.  Walk through
 * 	partitions to be added and fill them in -- regardless of the info being
 * 	overlaid!
 */

void
updateparts()
{
	struct partition *p;
	partstanza * mypart;
	node * n;
	symbol * part;

	long	*pt;	/** *pt is used like *p for the timestamp array **/

	for (n = subparts; n; n = n->ListNext) {	
		/* For each removed partition */
		/* look up the name; make sure it was defined */
		if ( !((part = lookup(n->ListRef))->flags & SY_DEFN) ) {
			fprintf(stderr, "Partition stanza '%s' not defined in file %s\n",
			    (char *)n->ListRef, partfile);
			exit(6);
		}

		/*
		 * Build a stanza combining any other referenced stanzas
		 * into one.  Replace the name on the partition list with its
		 * stanza (this last operation is not needed, but is
		 * conceivably useful in some future scenarios).
		 */
		mypart = (partstanza * )newpartstanza();
		buildstanza(mypart, part);
		n->ListRef = (void * )mypart;

		/* Validate partition number from stanza */
		if (mypart->ps_partno == UNDEFINED_NUMBER) {
			fprintf(stderr, "Stanza %s has no partition number\n", mypart->ps_name->name);
			exit(8);
		}
		/* Clear partition entry */
		p = &vtoc.v_part[mypart->ps_partno];
		if (p->p_tag == V_ALTS) {
			fprintf(stderr, "Removal of ALTS partition not supported\n");
			exit(64);
		}
		p->p_tag = p->p_flag = p->p_start = p->p_size = 0;

		/** Clear the timestamp **/
		vtoc.timestamp[mypart->ps_partno] = (long)0;
	}

	/*
	 * Reset vtoc.v_nparts.  After deleting partitions, walk back from the
	 * original last partition until we find one that is still valid and
	 * call it the last partition.
	 */
	for (p = &vtoc.v_part[vtoc.v_nparts-1], pt = &vtoc.timestamp[vtoc.v_nparts-1]; 
	    p >= vtoc.v_part && !(p->p_flag & V_VALID); p--, pt--) {
		continue;
	}
	vtoc.v_nparts = p - vtoc.v_part + 1;


	/*
	 * Add new partitions.  Loop around for each one, build it up from
	 * any other referenced stanzas, stuff the data into the partition,
	 * fix up the number of partitions if this is beyond the previous
	 * last partition.  CURRENTLY, PARTITION VALUES ARE NOT SANITY CHECKED.
	 * We ought to at least ensure that the numbers are reasonable.
	 */
	for (n = addparts; n; n = n->ListNext) {
		if ( !((part = lookup(n->ListRef))->flags & SY_DEFN) ) {
			fprintf(stderr, "Partition stanza '%s' not defined in file %s\n",
			    (char *)n->ListRef, partfile);
			exit(7);
		}
		mypart = (partstanza * )newpartstanza();
		buildstanza(mypart, part);
		n->ListRef = (void * )mypart;
		p = &vtoc.v_part[mypart->ps_partno];
		p->p_tag = mypart->ps_ptag;
		p->p_flag = mypart->ps_perm | V_VALID; /* by definition */
		p->p_start = mypart->ps_start;
		p->p_size = mypart->ps_size;
		if (mypart->ps_partno >= vtoc.v_nparts) {
			vtoc.v_nparts = mypart->ps_partno + 1;
		}
		/** Initialize the new timestamp variable **/

		vtoc.timestamp[mypart->ps_partno] = (long)0;

		/** ONLY DO ALT HANDLING FOR ESDI **/
		if (!is_scsi_dev) {

			if (p->p_tag == V_ALTS && alttbl.alt_sec.alt_reserved == 0) {
				alttbl.alt_sec.alt_reserved = p->p_size;
				alttbl.alt_sec.alt_base = p->p_start;

				/* Set flag - Adding an ALTS slice for the very
				 *	first time. With dynamic BBH, it is no longer
				 * appropriate to add additional ALTS areas. One must
				 * allocate all ALTS at installation time.
				*/
				First_alt++;
			}
			if (p->p_tag == V_ALTTRK && alttbl.alt_trk.alt_reserved == 0) {
				alttbl.alt_trk.alt_reserved = p->p_size / dp.dp_sectors;
				alttbl.alt_trk.alt_base = p->p_start;

				/* Adding an ALTTRK slice for the first time. */
				First_alt++;
			}
		}
	}
}

/*
 *
 * printalts()
 *
 *	print out a formatted alternate table report
 *
 * Parameters: none
 *
 * Return Values: none
 *
 * Exit State: none
 *
 */
void
printalts()
{
	
	/** Never true for SCSI since the TF_ALTS bit is masked off **/

	int	i, j;
	int	sectors;
	struct	alt_table *altptr;

	for (sectors = 1; sectors >= 0; sectors--) {
		altptr = sectors ? &alttbl.alt_sec : &alttbl.alt_trk;

		printf("\nALTERNATE %s TABLE: %d alternates available, %d used\n",
		    sectors ? "SECTOR" : "TRACK",
		    altptr->alt_reserved, altptr->alt_used);

		if (altptr->alt_used > 0) {
			printf("\nAlternates are assigned for the following bad %ss:\n",
			    sectors ? "sector" : "track");
			for (i = j = 0; i < altptr->alt_used; ++i) {
				if (altptr->alt_bad[i] == -1)
					continue;
				printf("\t%ld -> %ld", altptr->alt_bad[i],
				    sectors ? altptr->alt_base + i
				     : altptr->alt_base / dp.dp_sectors + i);
				if ((++j % 3) == 0) 
					printf("\n");
			}
			printf("\n");
		}
		if (altptr->alt_used != altptr->alt_reserved) {
			printf("\nThe following %ss are available as alternates:\n",
			    sectors ? "sector" : "track");
			for (i = altptr->alt_used, j = 0; i < altptr->alt_reserved; ++i) {
				if (altptr->alt_bad[i] == -1)
					continue;
				printf("\t\t%ld",
				    sectors ? altptr->alt_base + i
				     : altptr->alt_base / dp.dp_sectors + i);
				if ((++j % 4) == 0) 
					printf("\n");
			}
			printf("\n");
		}
	}
}

/*
 * verify_scsi()
 *
 *	checks the device type, if it's a scsi device.
 *
 * Parameters: none
 *
 * Return Values:
 *
 *	1	it's a scsi device
 *	0	no scsi device
 *
 * Exit State: 
 *
 *	1	the command line contains an option illegal for scsi devices
 *
 */
verify_scsi()
{
	/** used for determining if this is a scsi disk **/
	struct	bus_type bus_type;

	/** If the B_GETTYPE ioctl succeeds and the bus_name is "scsi" **/
	/** Then it's time to see if the options given on the command  **/
	/** line are valid for scsi disks. If they're not valid, give  **/
	/** advice and handle the alts flag, otherwise continue on.    **/

	if(ioctl(devfd, B_GETTYPE, &bus_type) >= 0 &&
		!strncmp(bus_type.bus_name, "scsi", 4))
	{
		if(not_for_scsi == 1)
		{
fprintf(stderr,"mkpart [-P add_partition_name] [-p remove_partition_name]\n");
fprintf(stderr,"   [-b] [-B boot_code_file] [-f partition_file]\n");
fprintf(stderr,"   [-t vp] [-x filename] [-i] device\n");
fprintf(stderr,"\n");
fprintf(stderr,"NOTE: multiple P and p flags may be specified.\n");
fprintf(stderr,"-b just rewrites the boot code, as determined from the partition file.\n");
fprintf(stderr,"-B specifies a different boot file.\n");
fprintf(stderr,"\tThe last arg is a raw UNIX System device (e.g. /dev/rdsk/1s0)\n");
fprintf(stderr,"-f specifies the partition file; its absence implies %s.\n",
	PARTFILE);
fprintf(stderr,"-t asks for a listing of:\n");
fprintf(stderr,"   p - partitions,\n");
fprintf(stderr,"   v - vtoc and physical drive characteristics.\n");
fprintf(stderr,"-x writes a complete device and partition stanza list for the\n");
fprintf(stderr,"   specified device to file 'filename' (useful for recovery).\n");
fprintf(stderr,"-i initializes the device, ignoring any existing VTOC data.\n");
fprintf(stderr,"   MUST BE USED if the device has never been formatted;\n");
fprintf(stderr,"   may be used to re-initialize a drive.\n");
fprintf(stderr,"\n");
fprintf(stderr,"NOTE: The options; -A, -F, -ta, -V, and -v are not valid for SCSI disks.\n");
fprintf(stderr,"      For the following mkpart options, use the designated SCSI command:\n\n");
fprintf(stderr,"      -A:	%s/%s\n",SCSICMDS,SCSIALTS);
fprintf(stderr,"      -F:	%s/%s\n",SCSICMDS,SCSIFORMAT);
fprintf(stderr,"      -ta:	%s/%s\n",SCSICMDS,SCSIALTS);
fprintf(stderr,"      -V:	%s/%s\n",SCSICMDS,SCSIVERIFY);
fprintf(stderr,"      -v:	%s/%s\n",SCSICMDS,SCSIVERIFY);
			
			mkpartclose();
			exit(1);
		}
		else if(not_for_scsi == 2)
		{
			/** Remove the TF_ALTS flag from targ **/
			/** This way the default for scsi   **/
			/** disks will be TF_PARTS and TF_VTOC **/

			targ &= ~TF_ALTS;
		}
		return(1);
	}
	return(0);
}
