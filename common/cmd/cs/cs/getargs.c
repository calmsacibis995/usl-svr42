/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/getargs.c	1.1.2.2"
#ident  "$Header: getargs.c 1.2 91/06/26 $"

#include "uucp.h"

/*
 * generate a vector of pointers (arps) to the
 * substrings in string "s".
 * Each substring is separated by blanks and/or tabs.
 *	s	-> string to analyze -- s GETS MODIFIED
 *	arps	-> array of pointers -- count + 1 pointers
 *	count	-> max number of fields
 * returns:
 *	i	-> # of subfields
 *	arps[i] = NULL
 */

GLOBAL int
getargs(s, arps, count)
register char *s, *arps[];
register int count;
{
	register int i = 0;
	int	 slen;

	char **listp, **strtoargv(char *);

        /* strip off trailing newline if it is still there */

        slen = strlen(s);
        if ((slen > 0) && (s[slen-1] == '\n'))
                s[slen-1] = '\0';


	/* dynamically create a list of arbitrary size */

	listp = strtoargv(s);

	if (listp) {
		/* only send back as many as there is room for */

		while ( i<count ) {
			if (listp[i] == NULL)
				break;
			arps[i] = listp[i];
			i++;
		}
		free(listp);
	}
	arps[i] = NULL;
	return(i);
}

/*
 *      bsfix(args) - remove backslashes from args
 *
 *      \123 style strings are collapsed into a single character
 *	\000 gets mapped into \N for further processing downline.
 *      \ at end of string is removed
 *	\t gets replaced by a tab
 *	\n gets replaced by a newline
 *	\r gets replaced by a carriage return
 *	\b gets replaced by a backspace
 *	\s gets replaced by a blank 
 *	any other unknown \ sequence is left intact for further processing
 *	downline.
 */

GLOBAL void
bsfix (args)
char **args;
{
	register char *str, *to, *cp;
	register int num;

	for (; *args; args++) {
		str = *args;
		for (to = str; *str; str++) {
			if (*str == '\\') {
				if (str[1] == '\0')
					break;
				switch (*++str) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					for ( num = 0, cp = str
					    ; cp - str < 3
					    ; cp++
					    ) {
						if ('0' <= *cp && *cp <= '7') {
							num <<= 3;
							num += *cp - '0';
						}
						else
						    break;
					}
					if (num == 0) {
						*to++ = '\\';
						*to++ = 'N';
					} else
						*to++ = (char) num;
					str = cp-1;
					break;

				case 't':
					*to++ = '\t';
					break;

				case 's':	
					*to++ = ' ';
					break;

				case 'n':
					*to++ = '\n';
					break;

				case 'r':
					*to++ = '\r';
					break;

				case 'b':
					*to++ = '\b';
					break;

				default:
					*to++ = '\\';
					*to++ = *str;
					break;
				}
			}
			else
				*to++ = *str;
		}
		*to = '\0';
	}
	return;
}


/*
 *	Miscellaneous command string/argument list manipulation
 *	routines stolen from IAF. If these are added to a public
 *	library, remove this code and use that interface.
 */

/*
 * strtoargv:    Given a pointer to a command line string,
 *	parse it into an argv array.
 *
 */

#define BUNCH 25
const char *delim = " \t'\"\n";	/* delimiters */

char **
strtoargv(char *cmdp)
{
    int nargs = 0;
    char **cmdargv = NULL;
    int i = 0;
    char delch;
    register char *cp;

    if (!cmdp)
	return(NULL);

    /* skip leading white space */
    while (isspace(*cmdp))
	cmdp++;

    if (!nargs) {
	nargs += BUNCH;
    	cmdargv = (char **) malloc(sizeof(char *) * (nargs + 1));
    	if (cmdargv == NULL)
		return(NULL);
    }

    while (*cmdp) {
	if ( i >= nargs ) {
	    nargs += BUNCH;
	    cmdargv = (char **) realloc(cmdargv, sizeof(char *) * (nargs + 1));
	    if (cmdargv == NULL)
		return(NULL);
	}
	    
	cmdargv[i] = cmdp;
	if (cmdp = (char *) strpbrk(cmdp, delim)) {
	    switch (*cmdp) {
	    /* normal separators, space and tab */
	    case ' ':
	    case '\t':
	    case '\n':
		*cmdp++ = '\0';
		break;
	    /* quoted arguments using " or ' */
	    case '"':
	    case '\'':
		delch = *cmdp; /* remember the delimiter */
		if ( cmdargv[i] != cmdp )
			break;	/* quote in the middle means new token	*/

		*cmdp = '\0';	/* clear the delimiter	*/
		cmdargv[i] = ++cmdp; /* skip the quote char */

		/* we must skip escaped quote characters
		 * i.e. \" or \'. cp is the pointer to the
		 * the real (i.e. edited) string position.
		 */

		for (cp = cmdp;;) {
		    if (*cmdp == '\0')
			return(NULL);
		    if (*cmdp == delch) {
			if (*(cp - 1) == '\\') {
			    /* got \" or \' */
			    *(cp - 1) = *cmdp;
			    cmdp++;
		        } else { /* end of string */
			    *cp = '\0';
			    cmdp++;
			    break;
			}
		    } else {
			*cp++ = *cmdp++;
		    }
		}
		break;

	    default:
		return(NULL);
	    }
	}
	i++;

	/* skip trailing white space */

	while (isspace(*cmdp))
	    cmdp++;
    }

    cmdargv[i] = NULL;

    return(cmdargv);
}
