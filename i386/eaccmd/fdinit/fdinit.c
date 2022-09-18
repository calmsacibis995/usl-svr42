/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)eac:i386/eaccmd/fdinit/fdinit.c	1.2"
#include	<sys/cram.h>
#include	<stdio.h>
#include	<errno.h>
#include	<fcntl.h>

#define DRV_5H	  0x02
#define DRV_3H	  0x04

main(argc, argv)
int argc;
char **argv;

{
	int c, drive;
	extern char *optarg;
	extern int optind;

	if (argc < 2 || argc > 3){
		usage(argv[0]);
		}
	while ((c = getopt(argc, argv, "f:")) != -1 )
		switch (c) {
		case 'f':
			drive = atoi(optarg);
			if ( drive > 1 || drive < 0 ) {
				fprintf(stderr, "Invalid Drive %d\n",drive);
				exit(1);
			}
			break;
		case '?':
			usage(argv[0]);
		}

	printf("%d\n", sysconfig(drive));
}

usage(s)
char *s;
{

	fprintf(stderr, "Usage: %s -f <drive> \n",s);
	exit(1);
}

sysconfig(drive)
int drive;
{
	int fd, type;
	unsigned char buf[2];
	unsigned char fddrive[2];

	errno=0;
	if ( (fd=open("/dev/cram",O_RDONLY)) == -1 ) {
		perror("");
		return(0);
	}

	buf[0]=0x10;
	if ( ioctl(fd, CMOSREAD, buf) == -1 ) {
		perror("");
		return(0);
	}
	fddrive[0]=(buf[1] >> 4 & 0x0F);
	fddrive[1]=(buf[1] & 0x0F);
	switch (fddrive[drive]) {
	case DRV_5H:
		return(5);
	case DRV_3H:
		return(3);
	default:
		return(0);
	}
}
