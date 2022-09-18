/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)echo:echo.c	1.3.1.4"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/echo/echo.c,v 1.1 91/02/28 16:56:03 ccs Exp $"

main(argc, argv)
char **argv;
{
	register char	*cp;
	register int	i, wd;
	int	j;
	int fnewline = 1;

	if(--argc == 0) {
		putchar('\n');
		exit(0);
	} else if (!strcmp(argv[1], "-n")) {
		fnewline = 0;
		++argv;
		--argc;
	}
	for(i = 1; i <= argc; i++) {
		for(cp = argv[i]; *cp; cp++) {
			if(*cp == '\\')
			switch(*++cp) {
				case 'b':
					putchar('\b');
					continue;

				case 'c':
					exit(0);

				case 'f':
					putchar('\f');
					continue;

				case 'n':
					putchar('\n');
					continue;

				case 'r':
					putchar('\r');
					continue;

				case 't':
					putchar('\t');
					continue;

				case 'v':
					putchar('\v');
					continue;

				case '\\':
					putchar('\\');
					continue;
				case '0':
					j = wd = 0;
					while ((*++cp >= '0' && *cp <= '7') && j++ < 3) {
						wd <<= 3;
						wd |= (*cp - '0');
					}
					putchar(wd);
					--cp;
					continue;

				default:
					cp--;
			}
			putchar(*cp);
		}
	        if ( i < argc )
                        putchar(' ');
                else
                        if ( fnewline )
                        putchar('\n');
	}
	exit(0);
}
