/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/retest.c	1.1"
#ident "@(#)retest.c	1.1 'attmail mail(1) command'"
#include <stdio.h>
#include "re.h"

extern char *malloc();

main(argc, argv)
char **argv;
{
    unsigned char re_map[256];
    char buf[1024];
    re_re **p_pattern;
    char *p;
    int i, pat;
    int *p_nbrak;
    char *match[10][2];

    if (argc < 2)
	{
	(void) fprintf (stderr, "Usage: %s regular-expression ...\n", argv[0]);
	return 1;
	}

    p_pattern = (re_re**) malloc((argc - 1) * sizeof(re_re));
    p_nbrak = (int*) malloc((argc - 1) * sizeof(int));

    if (!p_pattern || !p_nbrak)
	{
	(void) fprintf (stderr, "%s: malloc failed!\n", argv[0]);
	return 1;
	}

    for (i = 0; i < 256; i++)
	re_map[i] = (char)i;

    /* Map upper case letters to lower case. */
    /* This works even on EBCDIC systems. */
    for (i = 'A'; i <= 'Z'; i++)
	re_map[i] = tolower(i);
    for (pat = 1; pat < argc; pat++)
	{
	p_pattern[pat-1] = re_recomp(argv[pat], argv[pat] + strlen(argv[pat]), re_map);
	if (!p_pattern[pat-1])
	    {
	    (void) fprintf (stderr, "%s: compilation of pattern %d failed!\n", argv[0], pat);
	    return 1;
	    }
	p_nbrak[pat-1] = re_paren(p_pattern[pat-1]);
	(void) printf("Number of brackets for pattern %d = '%d'\n", pat, p_nbrak[pat-1]);
	}


    while (gets(buf))
	{
	(void) printf("looking at '%s'\n", buf);
	for (pat = 1; pat < argc; pat++)
	    {
	    (void) printf("\tpattern = '%s'\n", argv[pat]);
	    if (!re_reexec(p_pattern[pat-1], buf, buf + strlen(buf), match))
		(void) printf("\t\tno match\n");

	    else
		{
		for (i = 0; i <= p_nbrak[pat-1]; i++)
		    {
		    (void) printf("\t\tbracket #%d = %#lx,%#lx = '", i, (long)match[i][0], (long)match[i][1]);
		    if (match[i][0])
			for (p = match[i][0]; p < match[i][1]; p++)
			    {
			    (void) putchar(*p);
			    }
		    else
			(void) printf("(NULL)");
		    (void) printf("'\n");
		    }
		}
	    }
	}
    return 0;
}
