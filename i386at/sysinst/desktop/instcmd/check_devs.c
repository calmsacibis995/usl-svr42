/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:desktop/instcmd/check_devs.c	1.5"
#ident	"$Header: $"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/cram.h>
#include <sys/types.h>
#include <sys/fd.h>

extern int errno;

void usage();
int get_floppy_info();
int get_tape_info();
char *arg0save;


main(argc, argv)
int argc;
char **argv;
{
  extern int opterr, optind;
  extern char *optarg;
  int err[2];
  int i, j, arg;

  opterr = 0;		/* turn off the getopt error messages */

  arg0save = argv[0];

  while ((arg = getopt(argc,argv,"s:f:t:")) != EOF) {

     switch (arg) {

     case 'f': { /* check for existence/type of floppy device */

	int flopnum;

	flopnum=atoi(optarg);
	return (get_floppy_info(flopnum));
     }

     case 's': { /* check for existence/type of serial port */
	char *serialpath;

	serialpath = optarg;
	return (get_serial_info(serialpath));
     }

     case 't': { /* check for existence of tape */
	
	char *tapepath;

	tapepath = optarg;
	return (get_tape_info(tapepath));
     }
     case '?' : /* Incorrect argument found */

	usage();
	return (99);
     } /* switch arg */
  } /* while */
  usage();
  return (99);
}

void
usage()
{
	fprintf(stderr,"usage: %s [-f <1|2>] [-t tape_device_special_file\n",
		arg0save);
}

int
get_floppy_info(n)
int n;
{
	int	fd;
	unsigned char buf[2];

	

	if ((n<1) || (n>NUMDRV)) /* NUMDRV defined in fd.h */
		return (99); /* not a valid drive number */

	buf[0] = DDTB;
	if ((fd = open("/dev/cram", O_RDONLY)) < 0) {
		fprintf(stderr,"%s: errno %d on open /dev/cram\n", arg0save,
		   errno);
		return (99);
	}
	if (ioctl(fd, CMOSREAD, buf) < 0) {
		fprintf(stderr,"%s: errno %d on ioctl of /dev/cram\n",
		   arg0save, errno);
		return (99);
	}
	close(fd);

	if (n==1)
	  return ((buf[1] >> 4) & 0x0F);
	return (buf[1] & 0x0F);
	/* 				0 for nothing	*/
	/* 				2 for 5 1/4"	*/
	/* 				4 for 3 1/2"	*/
}

int
get_tape_info(path)
char *path;
{
	int fd, err;

	if (access(path,R_OK) < 0) { /* validate path, ability to open */
		fprintf(stderr,"%s: errno %d on open /dev/cram\n",
		   arg0save, errno);
		return (99); /* not a valid path wrong! */
	}

	errno=0;
	fd=open(path,O_RDONLY); /* access should've insured only errors will
				 * be from the driver
				 */
	if ((err=errno) == ENXIO)
		return (99);	/* indicates no tape controller */

	return (0); /* tape controller is alive; tape may not be
		     * inserted, though
		     */
}

/*
 * determine presence of serial port given by arg path
 */
int
get_serial_info(path)
char *path;
{
	int fd;

	/* open serial port with NDELAY turned on so we
	 * don't hang waiting for CARRIER_DETECT to be raised
	 */
	fd= open(path, O_RDONLY|O_NDELAY, 0 );

	/* fail command if ENXIO -- indicates controller not there
	 * or ENOENT -- indicates file not found
	 */
	if ((fd == -1) && ((errno == ENOENT) | (errno == ENXIO)))
		return (99);
	return (0);
}
