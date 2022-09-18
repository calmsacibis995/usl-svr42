/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386at/cmd/crash/memsize.c	1.2"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <sys/fcntl.h>
#include <stdio.h>

/*
 *	memsize -- print the memory size of the active system or a core dump
 *
 *	memsize takes an optional argument which is the name of a dump file;
 *	by default it uses /dev/mem (for active system).
 *
 *	memsize computes the memory size by looking for the bootinfo structure
 *	and adding up the sizes of all of the memory segments.
 */

void
main(argc, argv)
	int	argc;
	char	*argv[];
{
	char	*fname = "/dev/mem";
	int	fd, i;
	char	buffer[BOOTINFO_LOC+512];
	struct 	bootmem *memavail;
	int	memavailcnt;
	unsigned long	memsize;

	if (argc > 2) {
		fprintf(stderr, "usage:  memsize [ dumpfile ]\n");
		exit(1);
	}
	if (argc == 2)
		fname = argv[1];

	if ((fd = open(fname, O_RDONLY)) < 0) {
		fprintf(stderr, "memsize: Can't open %s\n", fname);
		exit(1);
	}
	if (read(fd, buffer, sizeof(buffer)) != sizeof(buffer)) {
		fprintf(stderr, "memsize: Read of %s failed\n", fname);
		exit(1);
	}
	close(fd);

	memavail = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavail;
	memavailcnt = ((struct bootinfo *)(buffer + BOOTINFO_LOC))->memavailcnt;

	for (memsize = i = 0; i < memavailcnt; ++i)
		memsize += (memavail++)->extent;

	printf("%ld\n", memsize);
	exit(0);
}
