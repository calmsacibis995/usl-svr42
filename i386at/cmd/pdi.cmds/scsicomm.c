/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)pdi.cmds:scsicomm.c	1.4.5.5"
#ident  "$Header: miked 1/22/92$"

#include	<sys/types.h>
#include	<sys/mkdev.h>
#include	<sys/stat.h>
#include	<sys/sdi_edt.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/vtoc.h>
#include	<sys/sd01_ioctl.h>
#include	<string.h>
#include	<stdio.h>
#include	<sys/scsicomm.h>

#define	SD01_DEV	"/dev/rdsk/c0t0d0s0"
extern int	errno;
extern int	Debug;

/*
 *  Return the SCSI Equipped Device Table
 *	Inputs:  hacnt - pointer to integer to place the number of HA's.
 *	Return:  address of the EDT
 *	         0 if couldn't read the EDT
 */

struct scsi_edt *
readedt(hacnt)
int	*hacnt;
{
	struct	drv_majors *ptr, *GetDriverMajors();
	struct	scsi_edt *xedt;
	int	fd, sdi_fd, ha_count;
	char 	tempnode[20], *mktemp();
	char 	sditempnode[20];
	int	scsi_via_mdevice = TRUE;
	dev_t	sdi_dev;

	setuid (0);

	if ((ptr = GetDriverMajors("sd01", &ha_count)) == NULL)	{
		scsi_via_mdevice = FALSE;
	} else {
		strcpy(tempnode, TEMPNODE);
		mktemp(tempnode);
		if (mknod(tempnode, (S_IFCHR | S_IREAD), makedev(ptr->c_maj,0x00)) < 0)	{
			scsi_via_mdevice = FALSE;
			warning("mknod failed for '%s'! Will try /dev/rdsk/c0t0d0s0.\n", tempnode);
		}
	}

	*hacnt = 0;
	if (scsi_via_mdevice == TRUE) {
		if ((fd = open (tempnode, O_RDONLY)) < 0)	{
			unlink(tempnode);
			warning("Cannot open '%s'! Will try /dev/rdsk/c0t0d0s0.\n", tempnode);
			scsi_via_mdevice = FALSE;
		}
	}
	if (scsi_via_mdevice == FALSE) {
		if ((fd = open (SD01_DEV, O_RDONLY)) < 0)	{
			error("Cannot open %s.\n", SD01_DEV);
			return(0);
		}
	}

	/* get device for sdi */
	if (ioctl(fd, B_GETDEV, &sdi_dev) < 0) {
		(void) close (fd);
		unlink (tempnode);
		error ("ioctl(B_GETDEV) failed.\n");
		return(0);
	}

/*
	sdi_dev += 28;
*/
	(void)close(fd);
	unlink(tempnode);

	strcpy(sditempnode, TEMPNODE);
	mktemp(sditempnode);

	if (mknod(sditempnode, (S_IFCHR | S_IREAD), sdi_dev) < 0) {
		error("mknod failed for sdi temp device\n");
		return(0);
	}

/*
 *	This has been changed to allow retries when the open of
 *	the SDI device fails because it is busy.  It can be busy
 *	in multi-user because of other normal disk activity.  Since
 *	I require pass-thru to the device, no one else can be using
 *	it while I am.
 */
	errno = 0;
	while ((sdi_fd = open(sditempnode, O_RDONLY)) < 0) {
		if ( errno != EBUSY ) {
			unlink (sditempnode);
			error("Cannot open sdi device: %s\n", sditempnode);
		}
	}

	/*  Get the Number of HA's in the system  */
	ha_count = 0;
	if (ioctl(sdi_fd, B_HA_CNT, &ha_count) < 0)  {
		(void) close(sdi_fd);
		unlink(sditempnode);
		error("ioctl(B_HA_CNT) failed\n"); 
		return(0);
	}

	if (ha_count == 0)	{
		(void) close(sdi_fd);
		unlink(sditempnode);
		errno = 0;
		error("Unable to determine the number of HA boards.\n");
		return(0);
	}

	*hacnt = ha_count;
	/*  Allocate space for SCSI EDT  */
	if ((xedt = (struct scsi_edt *) calloc(1, sizeof(struct scsi_edt) * MAX_TCS * ha_count)) == NULL)	{
		(void) close(fd);
		unlink(tempnode);
		errno = 0;
		error("Calloc for EDT structure failed\n");
		return(0);
	}

	/*  Read in the SCSI EDT  */
	if (ioctl(sdi_fd, B_REDT, xedt) < 0)  {
		(void) close(sdi_fd);
		unlink(sditempnode);
		error("ioctl(B_REDT) failed\n"); 
		return(0);
	}

	(void) close(sdi_fd);
	unlink(sditempnode);
	return(xedt);
}

/*
 *  Look in the mdevice file to get driver major numbers 
 *	Inputs:  drvrname - pointer to driver name.
 *	         count - pointer where to put the number of major numbers
 *	Return:  address of drv_majors structure
 *		 0 otherwise.
 */

struct drv_majors *
GetDriverMajors(drvrname, count)
char	*drvrname;
int	*count;
{
	int	i, firstblk, lastblk, firstchr, lastchr;
	int	block_mode_flag;
	FILE	*mdevicefp;
	char	*ptr, tmpdrvname[NAME_LEN], field[MFIELDS][MAXFIELD];
	struct	drv_majors *drvptr, drv_maj[MAXMAJNUMS];
	char	mdevice[256];
	int	mult_maj;

	for (i=0; i<NAME_LEN; i++)	{
		if ((*(drvrname + i) > 'A') && ((*(drvrname + i) < 'Z')))   {
			tmpdrvname[i] = *(drvrname + i) + 0x20;
		} else	{
			tmpdrvname[i] = *(drvrname + i);
		}
	}

	sprintf(mdevice,"/etc/conf/mdevice.d/%s",drvrname);
	if (Debug) fprintf(stderr, "GetDriverMajors(%s  %s)\t",drvrname, tmpdrvname);

	/*  open the mdevice file  */
	if ((mdevicefp = fopen(mdevice,"r")) == NULL) { 
		errno = 0;
		if (Debug)
			warning("Could not open %s\n",mdevice);
		return((struct drv_majors *) NULL);
	}

	*count = 0;
	mult_maj = 0;
	/*  look for driver in each of the lines in the mdevice file  */
	if (ParseLine1(field,mdevicefp,MFIELDS) == MFIELDS)	{
		block_mode_flag = (int) strchr(field[2], 'b');

		if (block_mode_flag)	{
			firstblk = atoi(field[4]);
			if (ptr = strchr(field[4],'-')) {
				*ptr++ = '\0';
				mult_maj = 1;
				lastblk = atoi(ptr);
			}
		}

		firstchr = atoi(field[5]);
		if (ptr = strchr(field[5],'-')) {
			*ptr++ = '\0';
			mult_maj = 1;
			lastchr = atoi(ptr);
		}
		if (mult_maj) {
			if (block_mode_flag && ((lastblk - firstblk) != (lastchr - firstchr)))	{
				errno = 0;
				warning("number of block major numbers does not equal number of character major numbers\n");
				fclose(mdevicefp);
				return((struct drv_majors *) NULL);
			}
		}
		else 
				lastchr = atoi(field[5]);

		for (i=firstchr; i <= lastchr; i++, (*count)++)     {
			if (block_mode_flag)	
				drv_maj[*count].b_maj = firstblk++;
			else	
				drv_maj[*count].b_maj = NO_MAJOR;
			drv_maj[*count].c_maj = i;
			if (Debug) fprintf(stderr, "Driver %s\t:  cmaj=%d,\tbmaj=%d\n",tmpdrvname, drv_maj[*count].c_maj, drv_maj[*count].b_maj);
		}

/*
			break;
*/
	fclose(mdevicefp);
	}
	else {
		fclose(mdevicefp);
		return((struct drv_majors *) NULL);
	}

	if (*count == 0)
		return((struct drv_majors *) NULL);

	/*  Allocate space for drv_majors structure  */
	if ((drvptr = (struct drv_majors *) calloc(1, sizeof(struct drv_majors) * (*count))) == NULL)	{
		errno = 0;
		warning("calloc for major number structure failed\n");
		fclose(mdevicefp);
		return((struct drv_majors *) NULL);
	}
	memcpy(drvptr, drv_maj, sizeof(struct drv_majors) * (*count));
	return(drvptr);
}

/*
 * read the next line from a file, skipping over white space
 * and comments and place each field into a character array. 
 * Returns the number of fields read or EOF.
 */

int
ParseLine(array,fp, numfields)
char array[][MAXFIELD];
FILE *fp;
int numfields;
{
	int	i, j;
	char	ch;

	/*  inititalize the array to null strings  */
	for (i=0; i<numfields; i++) 
		array[i][0] = '\0';

	/*  skip over comment lines , reading the remainder of the line  */
	while ((ch = getc(fp)) == '#' ) {
		fscanf(fp,"%*[^\n]%*[\n]");
	}
	if (ungetc(ch,fp) == EOF)
		return(EOF);

	j = 0;
	for (i=0; i<numfields; i++) {
		if(fscanf(fp,"%s",array[i]) == EOF) {
			j = 0;
			break;
		}
		j++;
	}
	/*  read the remainder of the line  */
	fscanf(fp,"%*[^\n]%*[\n]");
	if (j == 0) {
		return(EOF);
	} else {
		if (j != numfields) {
			errno = 0;
			warning("Number of columns incorrect in file.\n");
		}
		return(j);
	}
}

/* a patch routine to parse /etc/conf/mdevice.d/module files
/* invoked in GetDriverMajors. Special chars in those files
/* have to be skipped.
*/
int
ParseLine1(array,fp, numfields)
char array[][MAXFIELD];
FILE *fp;
int numfields;
{
	int	i, j;
	char line[BUFSIZ];
	char *linep;
	char a[MAXFIELD];

	/*  inititalize the array to null strings  */
	for (i=0; i<numfields; i++) 
		array[i][0] = '\0';

	/*  skip over comment lines */
	j = 0;
	while (fgets(line, BUFSIZ, fp)) {
		linep = line;
		if ((*linep == '#') || (*linep == '*') || (*linep == '$')) 
			for (++linep; *linep != '\0'; ++linep);
		else {
			while ((*linep != '\n') && (*linep != '\0')) {
				while (isspace(*linep)) linep++;
				for (i=0;i < 20; i++)
					a[i] = '\0';
				for (i=0; !isspace(*linep); i++, linep++)
					a[i] = *linep;
				strcpy (array[j++], a);
			}
		}
	}
	if (j != numfields) {
		errno = 0;
		warning("Number of columns incorrect in file.\n");
	}
	return(j);
}

/*
 *  Routine to read in the pdsector and VTOC.
 *	Inputs:  devname - pointer to the device name
 *	         vtocptr - pointer to where to put the VTOC
 *	Return:  1 if pdsector VTOC was read in
 *		 0 otherwise.
 */

int
rd_vtoc(dpart,vbuf)
char	*dpart;		/* Disk partition name */
struct	vtoc	*vbuf;	/* buf for VTOC data */
{
	int		fd;		/* Disk Partition File Descriptor */
	struct vtoc	*vtoc;
	struct pdinfo	*pdsector;
	unsigned	voffset;
	char		secbuf[512];
	struct phyio	args;		/* IO command buffer */

	/*  Open the Disk Partition.  */
	if ((fd = open(dpart,O_RDONLY)) == -1)
	{
		fprintf(stderr,"ERROR: Cannot open disk partition '%s'!\n",dpart);
		perror("system call error is");
		return(0);
	}

	/*
	*  Read the disk Physical Descriptor (PD) block.
	*  (Got the following stuff from prtvtoc)
	*/
	args.sectst = 0;
	args.memaddr = (unsigned long) &secbuf;
	args.datasz = 512;
	if (ioctl(fd,V_PDREAD,&args) == -1)
	{
		fprintf(stderr,"ERROR: V_PDREAD ioctl to %s failed:\n", dpart);
		perror("system call error is");
		close(fd);
		return(0);
	}

	close(fd);
	if (args.retval != 0)
	{
		fprintf(stderr,"ERROR: Bad return value from V_PDREAD ioctl to %s:\n", dpart);
		return(0);
	}

	pdsector = (struct pdinfo *) secbuf;
	if (pdsector->sanity != VALID_PD)
	{
		fprintf(stderr, "ERROR: PD sector on '%s' is insane!\n",dpart);
		return(0);
	}

        voffset = pdsector->vtoc_ptr & 511;
        vtoc = (struct vtoc *) (secbuf + voffset);
	if (vtoc->v_sanity != VTOC_SANE)
	{
		fprintf(stderr, "ERROR: VTOC on '%s' is insane!\n",dpart);
		return(0);
	}

	*vbuf = *vtoc;
	close(fd);
	return(1);
}

/*
 *  Routine to print out error information.
 *	Inputs:  message - pointer to the string of the error message
 *	         data1...data5 - pointers to additional arguments
 *
 *	NOTE:  This routine does not return.  It exits.
 */

void
error(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	(void) fflush(stdout);
	(void) fprintf(stderr, "ERROR: ");
	(void) fprintf(stderr, message, data1, data2, data3, data4, data5);
	if (errno)
		perror("system call error is");

	exit(ERREXIT);
}

/*
 *  Routine to print out warning information.
 *	Inputs:  message - pointer to the string of the warning message
 *	         data1...data5 - pointers to additional arguments
 */

void
warning(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	(void) fflush(stdout);
	(void) fprintf(stderr, "WARNING: ");
	(void) fprintf(stderr, message, data1, data2, data3, data4, data5);
	if (errno) {
		perror("system call error is");
		errno = 0;
	}
}
