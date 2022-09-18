/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)disksetup:i386at/cmd/disksetup/scsicomm.c	1.4"



#include	"sys/types.h"
#include	"sys/mkdev.h"
#include	"sys/stat.h"
#include	"sys/sdi_edt.h"
#include	"sys/mirror.h"
#include	"fcntl.h"
#include	"errno.h"
#include	"sys/vtoc.h"
#include	"sys/sd01_ioctl.h"
#include	"string.h"
#include	"stdio.h"
#include	"sys/scsicomm.h"

extern int	errno;

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
	while ((ch = getc(fp)) == '#') {
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

/*
 *  Check to see if the device contains any mirrored partitions
 *	Inputs:  dev_name - pointer to the name of the block device.
 *	Return:  1 if device contains a mirrored partition
 *		 0 otherwise.
 *		 2 can't tell.
 */

int
ckmirror(devname)
char	*devname;
{
	int		i, j, mir, cnt, num_part;
	struct MTABLE	*mtabptr, *omtabptr;
	struct stat	statbuf, mirstat;

	if (stat(MIRROR_TABLE, &mirstat) < 0)	{
		if (errno == ENOENT)
			return(0);
		warning("stat of '%s' failed\n", MIRROR_TABLE);
		return(2);
	}

	if ((mtabptr = (struct MTABLE *) malloc(mirstat.st_size)) == NULL)    {
		errno = 0;
		warning("malloc for %s failed\n", MIRROR_TABLE);
		return(2);
	}

	if (stat(devname, &statbuf) < 0)	{
		warning("stat of '%s' failed\n", devname);
		return(2);
	}

	if ((mir = open(MIRROR_TABLE, 0)) < 0)	{
		warning("open of '%s' failed\n", MIRROR_TABLE);
		return(2);
	}

	if ((cnt = read(mir, mtabptr, mirstat.st_size)) != mirstat.st_size)   {
		warning("read of '%s' failed\n", MIRROR_TABLE);
		close(mir);
		return(2);
	}

	close(mir);
	if (cnt % sizeof(struct MTABLE))	{
		errno = 0;
		warning("size of '%s' is not correct\n", MIRROR_TABLE);
		return(2);
	}

	omtabptr = mtabptr;
	for (i=0; i < cnt/sizeof(struct MTABLE); i++)	{
		for (j=0; j<NDISKPARTS; j++)	{
			if (strlen(mtabptr->mt_dpart[j].dp_name) == 0)
				continue;
			if ((statbuf.st_rdev / num_part) == (mtabptr->mt_dpart[j].dp_dev / num_part))
				return(1);
		}
		mtabptr++;
	}
	free(omtabptr);
	return(0);
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
		fprintf(stderr, "ERROR: PD sector on '%s' is insane!",dpart);
		return(0);
	}

        voffset = pdsector->vtoc_ptr & 511;
        vtoc = (struct vtoc *) (secbuf + voffset);
	if (vtoc->v_sanity != VTOC_SANE)
	{
		fprintf(stderr, "ERROR: VTOC on '%s' is insane!",dpart);
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

void
nowarning(message, data1, data2, data3, data4, data5)
char	*message;	/* Message to be reported */
long	data1;		/* Pointer to arg	 */
long	data2;		/* Pointer to arg	 */
long	data3;		/* Pointer to arg	 */
long	data4;		/* Pointer to arg	 */
long	data5;		/* Pointer to arg	 */
{
	return;
}

/*
 *  Return the name of the block device given the name of the
 *  character device.
 *	Inputs:  rpart - pointer to the name of the character partition.
 *	         bpart - address of where to put the block device name
 *	Return:  1 if constructed the block device name
 *		 0 otherwise.
 */

int
get_blockdevice(rpart, bpart)
char	*rpart, *bpart;
{
	char	*ptr;
	struct	stat	statbuf;

	/*  Verify rpart is a character device special file.  */
	if (stat(rpart, &statbuf) == -1)
	{
		return(0);
	}

	if ((statbuf.st_mode & S_IFMT) != S_IFCHR)
	{
		return(0);
	}

	strcpy(bpart, rpart);
	if ((ptr = strchr(rpart,'r')) == NULL)
	{
		return(0);
	}
	strcpy(strchr(bpart, 'r'), ++ptr);
	return(1);
}

/*
 *  Swap bytes in a 16 bit data type.
 *	Inputs:  data - long containing data to be swapped in low 16 bits
 *	Return:  short containing swapped data
 */

short
scl_swap16(data)
unsigned long	data;
{
	unsigned short	i;

	i = ((data & 0x00FF) << 8);
	i |= ((data & 0xFF00) >> 8);

	return (i);
}

/*
 *  Swap bytes in a 24 bit data type.
 *	Inputs:  data - long containing data to be swapped in low 24 bits
 *	Return:  long containing swapped data
 */

long
scl_swap24(data)
unsigned long	data;
{
	unsigned long	i;

	i = ((data & 0x0000FF) << 16);
	i |= (data & 0x00FF00);
	i |= ((data & 0xFF0000) >> 16);

	return (i);
}

/*
 *  Swap bytes in a 32 bit data type.
 *	Inputs:  data - long containing data to be swapped
 *	Return:  long containing swapped data
 */

long
scl_swap32(data)
unsigned long	data;
{
	unsigned long	i;

	i = ((data & 0x000000FF) << 24);
	i |= ((data & 0x0000FF00) << 8);
	i |= ((data & 0x00FF0000) >> 8);
	i |= ((data & 0xFF000000) >> 24);

	return (i);
}
