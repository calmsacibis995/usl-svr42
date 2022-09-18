/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)printf:printf.c	1.2.4.1"
#ident "$Header: printf.c 1.2 91/08/12 $"

#include <stdio.h>

extern	char *strccpy();
extern	char *strchr();
extern  char *malloc();

main(argc, argv)
int	argc;
char	**argv;
{
	char	*fmt, *tmp;
	register int	i, j, toomuch = 0;
	int argint[21];

	if (argc == 1) {
		fprintf(stderr, "Usage: printf format [[[arg1] arg2] ... argn]\n");
		exit(1);
	}

	if ((fmt = malloc(strlen(argv[1] + 1))) == (char *)NULL) {
		fprintf(stderr, "malloc failed\n");
		exit(1);
	}

	strccpy(fmt, argv[1]);

	tmp = strchr(fmt, '%');
	while (tmp != (char *)0 && *(tmp+1) == '%')
		tmp = strchr(tmp+2, '%');

	for (i = 2; argv[i] != (char *)0; i++) {
		argint[i] = 0;
		if ( isdigit(argv[i][0]) &&
		     ( ( isdigit(argv[i][1]) &&
		       ( ( isdigit(argv[i][2]) && argv[i][3] == '\0')
				|| argv[i][2] == '\0') ) || argv[i][1] == '\0'))
			for (j = 0; argv[i][j] != '\0'; j++)
				argint[i] = (10 * argint[i]) + argv[i][j] - '0';
		else
			argint[i] = (int) argv[i];

		if (tmp == (char *)0)
			toomuch++;
		else
			tmp = strchr(tmp+1, '%');
		while (tmp != (char *)0 && *(tmp+1) == '%')
			tmp = strchr(tmp+2, '%');
	}

	if (tmp != (char *)0) {
		fprintf(stderr, "printf: [error]: format describes more arguments than given.\nUsage: printf format [[[arg1] arg2] ... argn]\n");
		exit(1);
	}
	if (toomuch)
		fprintf(stderr, "printf: [warning]: format describes less arguments than given.\n");

	printf(fmt, argint[2], argint[3], argint[4],  argint[5],
			argint[6], argint[7], argint[8], argint[9],
			argint[10], argint[11], argint[12], argint[13],
			argint[14], argint[15], argint[16], argint[17],
			argint[18], argint[19], argint[20]);
	exit(0);
}
