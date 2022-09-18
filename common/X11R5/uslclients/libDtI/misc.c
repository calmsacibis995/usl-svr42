/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)libDtI:misc.c	1.11" */

#include <X11/Intrinsic.h>
#include "DtI.h"

extern char *strndup();
extern char *getenv();
#ifndef MEMUTIL
extern char *malloc();
#endif /* MEMUTIL */

/****************************procedure*header*****************************
    Dm__MakePath- make fully qualified path for 'name' by concatenating
	'path' and 'name'.  CAUTION: result is put in (static) Dm__buffer.

	ASSUMES 'path' begin with '/' and if path is NULL or "", 'name' MUST
	be full path.
*/
char *
Dm_MakePath(char * path, char * name, char * buf)
{
    register int len;

    if ((path == NULL) || (path[0] == '\0'))
    {
	len = 0;

    } else
    {
	/* Add '/' separator only if 'path' is not "/" */

	if ( (len = strlen(path)) > 1 )
	    (void)strcpy(buf, path);
	else
	    len = 0;

	buf[len++] = '/';		/* Add '/' separator */
    }

    (void)strcpy(buf + len, name);
    return(buf);
}					/* end of Dm__MakePath */

/*
 * supports simple ${env} expansion. 
 * supports simple %{prop} expansion (property values, if op is !NULL). 
 * - does support recursive expansion via the expand_proc.
 * - does not support conditional parameter expansion.
 * - does not modify the original string.
 * - returns malloced space. Thus the caller should free the returned string
 *   when done with it.
 * - no limit on the length of expanded value.
 */
char *
Dm__expand_sh(str, expand_proc, client_data)
char *str;
char *(*expand_proc)();
XtPointer client_data;
{
	register char *p = str;
	char *start;
	char *name;
	char *end;
	char *value;
	char *ret;
	char *free_this = NULL;
	int size;
	int notfirst = 0;
	int enclosed;

    while (1) {
	while (*p && ((*p != '$') && (!expand_proc || (*p != '%')))) p++;
	if (!*p) {
		/* nothing to expand */
		if (notfirst)
			return(str);
		else
			return(strdup(str));
	}

	start = p;
	if (*++p == '{') {
		/* look for matching '}' */
		name = ++p;
		while (*p && (*p != '}')) p++;
		enclosed = 1;
	}
	else {
		name = p;
		while (*p && (isalpha(*p) || isdigit(*p) || (*p == '_'))) p++;
		enclosed = 0;
	}
	/* now, p points to the end of env name */
	end = p;

	name = strndup(name, p - name);

	switch (*start) {
	case '$':
		value = getenv(name);
		break;
	case '%':
		value = (*expand_proc)(name, client_data);
		free_this = value;
		break;
	}
	free(name);

	size = strlen(str) + 1 + (value ? strlen(value) : 0);
	if (!(ret = (char *)malloc(size)))
		return(NULL);
	memcpy(ret, str, start - str);
	p = ret + (start - str);
	if (value) {
		strcpy(p, value);
		p += strlen(value);
	}
	if (free_this) {
		free(free_this);
		free_this = NULL;
	}

	if (enclosed)
		++end;
	strcpy(p, end);
	if (notfirst)
		free(str);
	str = ret;
	notfirst = 1;
    } /* while */
}

int
Dm__strnicmp(str1, str2, len)
register const char *str1;
register const char *str2;
register int len;
{
	register int c1;
	register int c2;

	while ((--len >= 0) &&
		((c1 = toupper(*str1)) == (c2 = toupper(*str2++))))
		if (*str1++ == '\0')
			return(0);
	return(len < 0 ? 0 : (c1 - c2));
}

int
Dm__stricmp(str1, str2)
register const char *str1;
register const char *str2;
{
	register int c1;
	register int c2;

	while (*str1 && *str2 &&
	       ((c1 = toupper(*str1)) == (c2 = toupper(*str2++)))) {
		str1++;
		str2++;
	}

	if (c1 != c2)
		return(c1 - c2);
	else
		return((*str1 == '\0') - (*str2 == '\0'));
}

#ifdef USE_REGCMP
char *
CvtToRegularExpression(expression)
char * expression;
{
	char *ret;

	if (expression == NULL || *expression == '\0')
   		ret = strdup(".*$");
	else {
   		register char * i;
   		register char * j = ret = malloc(strlen(expression) * 2 + 3);

   		*j++ = '^';
   		for (i = expression; *i; i++)
      		switch(*i) {
         	case '?':
			 *j++ = '.';
                   	break;
         	case '.':
			*j++ = '\\';
                   	*j++ = '.';
                   	break;
         	case '*':
			*j++ = '.';
                   	*j++ = '*';
                   	break;
         	default:
			*j++ = *i;
                   	break;
         	}
   		*j++ = '$';
   		*j   = '\0';
   	}

	return(ret);

} /* end of CvtToRegularExpression */
#endif
