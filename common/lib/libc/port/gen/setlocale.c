/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/setlocale.c	1.12"
/*
* setlocale - set and query function for all or parts of a program's locale.
*/
#include "synonyms.h"
#include "shlib.h"
#include <locale.h>
#include "_locale.h"	/* internal to libc locale data structures */
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static char *set_cat();

char *
setlocale(cat, loc)
int cat;
const char *loc;
{
	char part[LC_NAMELEN];
	char *lp;
	int prev_seen;

	if (loc == 0)	/* query */
	{

#if DSHLIB
		static char *ans;

		if (!ans && (ans = malloc(LC_ALL * LC_NAMELEN + 1)) == 0)
			return 0;
#else
		static char ans[LC_ALL * LC_NAMELEN + 1];
#endif

		if (cat != LC_ALL)
			(void)strcpy(ans, _cur_locale[cat]);
		else
		{
			register char *p, *q;
			register int flag = 0;
			register int i;

			/*
			* Generate composite locale description.
			*/
			p = ans;
			for (i = LC_CTYPE; i < LC_ALL; i++)
			{
				*p++ = '/';
				q = _cur_locale[i];
				(void)strcpy(p, q);
				p += strlen(q);
				if (!flag && i > LC_CTYPE)
					flag = strcmp(q, _cur_locale[i - 1]);
			}
			if (!flag)
				return q;
		}
		return ans;
	}
	/*
	* Handle LC_ALL setting specially.
	*/
	if (cat == LC_ALL)
	{
		static int reset = 0;
		register const char *p;
		const char *lang;
		register int i,compatflag;
		register char *sv_loc;
		static const char *name[LC_ALL] = 
			{ "LC_CTYPE", 
			  "LC_NUMERIC",
			  "LC_TIME",
			  "LC_COLLATE",
			  "LC_MONETARY",
			  "LC_MESSAGES"
 			};

		if (!reset)
			sv_loc = setlocale(LC_ALL, NULL);
		cat = LC_CTYPE;
		if ((!(lang=getenv("LANG"))) && (getenv("CHRCLASS") || getenv("LANGUAGE")))
			compatflag=1;
		else {
			compatflag=0;
		}
		if ((p = loc)[0] != '/')	/* simple locale */
		{
			loc = strncpy(part, p, LC_NAMELEN - 1);
			part[LC_NAMELEN - 1] = '\0';
		}
		do	/* for each category other than LC_ALL */
		{
			if (p[0] == '/')	/* piece of composite locale */
			{
				i = strcspn(++p, "/");
				(void)strncpy(part, p, i);
				part[i] = '\0';
				p += i;
			}
			prev_seen=0;
			if (part[0]=='\0') {
				if (compatflag)
					lp=_nativeloc(cat);
				else
					if ((lp = getenv(name[cat])) == NULL)
						lp = (char *)lang;
				if (!lp)
					lp="C";
			}
			else
				lp = part;
			if (cat > LC_NUMERIC)
				for(i=0;i<cat;i++) {
					if (strcmp(_cur_locale[i],lp)==0) {
						prev_seen=1;
						break;
					}
				}
			if (prev_seen) {
				(void) strcpy(_cur_locale[cat++],lp);
			}else {
				if (set_cat(cat++, lp) == 0) {
					reset = 1;
					(void) setlocale(LC_ALL, sv_loc);
					reset = 0;
					return 0;
				}
			}
		} while (cat < LC_ALL);
		return setlocale(LC_ALL, NULL);
	}
	return(set_cat(cat, loc));
}

static char *
set_cat(cat, loc)
int cat;
const char *loc;
{
	char part[LC_NAMELEN];

	/*
	* Set single category's locale.  By default,
	* just note the new name and handle it later.
	* For LC_CTYPE and LC_NUMERIC, fill in their
	* tables now.
	*/
	if (loc[0] == '\0')
		loc = _nativeloc(cat);
	else
	{
		loc = strncpy(part, loc, LC_NAMELEN - 1);
		part[LC_NAMELEN - 1] = '\0';
	}
	if (cat <= LC_NUMERIC)
	{
		if (strcmp(loc, _cur_locale[cat]) != 0 && _set_tab(loc, cat) != 0)
			return 0;
	}
	else {
		int fd;
		static const char *name[LC_ALL] = 
			{ "LC_CTYPE", 
			  "LC_NUMERIC",
			  "LC_TIME",
			  "LC_COLLATE",
			  "LC_MONETARY",
			  "LC_MESSAGES"
 			};
		if (strcmp(loc, _cur_locale[cat]) != 0) {
			if ((fd = open(_fullocale(loc, name[cat]), O_RDONLY)) == -1)
				return 0;
			(void)close(fd);
		}
	}
	return strcpy(_cur_locale[cat], loc);
}
