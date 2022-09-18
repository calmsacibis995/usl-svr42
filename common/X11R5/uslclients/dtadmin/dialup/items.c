/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:dialup/items.c	1.23"
#endif

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <FList.h>
#include "uucp.h"
#include "error.h"

extern void	CreateBlankEntry();
extern void	FreeHostData();
extern void	FreeNew();
extern void	AddLineToBuffer();
extern void	DeleteLineFromBuffer();
extern void	CheckMode();

static void 	GetDirectory();

static		exists;		/* Non-zero if file exists */
static		readable;	/* Non-zero if file is readable */
static		writeable;	/* Non-zero if file is writable or */
				/* directory is writable and file */
				/* doesn't exist */
LinePtr			GetLine();
#ifdef debug
void			PrintFlatItems();
#endif

static FILE *fd;

/*
 * This routine reads the /etc/uucp/Systems file and populates the info 
 * into the internal data structure that will be used in both the flatlist
 * and the property sheet.
 * The update flag will be used by other routines to make the apply, delete
 *
 * dp -- pointer to the default values (since last apply) of the property
 * sheet.
 */

 
void
GetFlatItems(filename)
char * filename;
{
	static unsigned	stat_flags = 0;	/* Mask returned by CheckMode() */
	int		n = 0;
	HostData	*dp;
	LinePtr		lp;
	char		*begin, *end;
	unsigned	offset, length;
	char		*token;
	char		*f_name;
	char		*f_time;
	char		*f_type;
	char		*f_class;
	char		*f_phone;
	char		*f_expect[F_MAX/2];
	char		*f_respond[F_MAX/2];
	char		buf[BUFSIZ];

	CheckMode(filename, &stat_flags);	/* get the file stat	*/
	exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
	readable = BIT_IS_SET(stat_flags, FILE_READABLE);
	writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
#ifdef debug
	fprintf(stderr, "/etc/uucp/System exists = %d, readable = %d, writeable = %d\n",
			exists, readable, writeable);
#endif
	if (!exists & !writeable) { /* somthing's serious wrong */
#ifdef debug
		fprintf(stderr, GGT(string_createFail), filename);
		exit(1);
#endif
		sprintf(buf, GGT(string_noFile), filename);
		rexit(1, buf, "");
	} else
	if (!readable & exists) {
#ifdef debug
		fprintf(stderr, GGT(string_accessFail), filename);
		exit(2);
#endif
		sprintf(buf, GGT(string_accessFail), filename);
		rexit(2, buf, "");
	} else
	if (exists & readable & !writeable) {
		fprintf(stderr, GGT(string_writeFail), filename);
		/*
		** Update appropriate widgets to reflect the new state
		*/

	}
	if ((fd = fopen(filename, "r")) == (FILE *)NULL) {
		/* Something's wrong here but there are some chances	*/
		/* we might be able to create the file if we are the	*/
		/* prviledged user.  For now just put out message	*/
#ifdef debug
		fprintf(stderr, GGT(string_openFail), filename);
#endif
		;
	} else {
		for (sf->numFlatItems=0; (lp = GetLine (buf, BUFSIZ, fd)) != NULL;
		     sf->numFlatItems++) {
			if ((sf->numFlatItems % INCREMENT) == 0 &&
			   sf->numFlatItems == sf->numAllocated) {
				sf->flatItems = (FlatList *) XtRealloc (
				(char *) sf->flatItems,
				(sf->numFlatItems + INCREMENT) * sizeof(FlatList));
				sf->numAllocated += INCREMENT;
			}
			buf[strlen(buf)-1] = '\0';
			begin = &buf[0];
			if ((token = strtok(buf, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_name = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_time = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_type = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_class = token;
			if ((token = strtok((char *) NULL, " \t\n")) == NULL) {
				sf->numFlatItems--;
				continue;
			} else
				f_phone = token;
			dp = sf->flatItems[sf->numFlatItems].pField =
				(HostData *)XtMalloc(sizeof(HostData));
			dp->f_name = (XtPointer)strdup(f_name);
			dp->f_time = (XtPointer)strdup(f_time);
			dp->f_type = (XtPointer)strdup(f_type);
			dp->f_class = (XtPointer)strdup(f_class);
			dp->f_phone = (XtPointer)strdup(f_phone);
			dp->changed = False;
			dp->lp = lp;
			for (n =0;
			     (f_expect[n] = strtok(NULL, " \t\n")) != NULL &&
			     n < F_MAX/2;
			     n ++)
			     if((f_respond[n] = strtok(NULL, " \t\n")) == NULL) {
				n++;
				break;
			     }
			switch (n) {
				case 0: /* no exepect, respond string */
				case 1: /* one exepect, respond string */
					dp->f_expect = (XtPointer)strdup("");
					if (f_expect[0]) {
						dp->f_expect1 =
							(XtPointer)strdup(f_expect[0]);
						if (f_respond[0] == NULL)
							dp->f_login =
							(XtPointer)strdup("");
						else
							dp->f_login =
							(XtPointer)strdup(f_respond[0]);
					} else {
						dp->f_expect1 = (XtPointer)strdup("");
						dp->f_login = (XtPointer)strdup("");
					}
					dp->f_expect2 = (XtPointer)strdup("");
					dp->f_passwd = (XtPointer)strdup("");
					break;
				case 2: /* two expect, respond strings */
					dp->f_expect = (XtPointer)strdup("");
					dp->f_expect1 = (XtPointer)strdup(f_expect[0]);
					dp->f_login = (XtPointer)strdup(f_respond[0]);
					dp->f_expect2 = (XtPointer)strdup(f_expect[1]);
					if(f_respond[1] == NULL)
						dp->f_passwd = (XtPointer)strdup("");
					else
						dp->f_passwd =
							(XtPointer)strdup(f_respond[1]);
					break;
				default:
					end = f_expect[0];
					offset = end - begin;
					length = f_expect[n-2] - f_expect[0];
					dp->f_expect = malloc(length);
					strncpy(dp->f_expect, lp->text+offset, length-1);
					*((char *)dp->f_expect + length - 1) = '\0';
					dp->f_expect1 = (XtPointer)strdup(f_expect[n-2]);
					dp->f_login = (XtPointer)strdup(f_respond[n-2]);
					dp->f_expect2 = (XtPointer)strdup(f_expect[n-1]);
					if(f_respond[n-1] == NULL)
						dp->f_passwd = (XtPointer)strdup("");
					else
						dp->f_passwd =
							(XtPointer)strdup(f_respond[n-1]);
					break;
			}
		}
	}
	if (sf->numFlatItems == 0)
		sf->currentItem = -1;
	else {
		sf->currentItem = 0;
#ifdef debug
		PrintFlatItems();
#endif
	}
	if (fd != NULL)
		fclose (fd);
} /* GetFlatItems */

#ifdef debug
void
PrintFlatItems()
{
	register i;
	HostData *fp;
	char text [BUFSIZ];

	for (i=0; i<sf->numFlatItems; i++) {
		fp = sf->flatItems[i].pField;
		fprintf (stdout,
			"%s %s %s %s %s %s %s %s %s %s\n",
			 fp->f_name,
			 fp->f_time,
			 fp->f_type,
			 fp->f_class,
			 fp->f_phone,
			 fp->f_expect,
			 fp->f_expect1,
			 fp->f_login,
			 fp->f_expect2,
			 fp->f_passwd);
	}
} /* PrintFlatItems */
#endif

void
PutFlatItems(filename)
char *filename;
{
	register LinePtr	endp;
	HostData *fp;
	register i;
	char text [BUFSIZ*4];
	char buf [BUFSIZ];

	buf[0] = NULL;

	for (i=0; i<sf->numFlatItems; i++) {
		fp = sf->flatItems[i].pField;
		if (fp->changed == False) continue;
		sprintf (text,
			"%s %s %s %s %s %s %s %s %s %s\n",
			fp->f_name,
			fp->f_time,
			fp->f_type,
			fp->f_class,
			fp->f_phone,
			fp->f_expect,
			fp->f_expect1,
			fp->f_login,
			fp->f_expect2,
			fp->f_passwd);
		XtFree (fp->lp->text);
		fp->lp->text = strdup(text);
		fp->changed = False;
	}
	if ((fd = fopen(filename, "w")) == NULL) {
		sprintf (buf, GGT(string_fopenWrite), filename);
		PUTMSG (buf);
		return;
	}
	for (endp = sf->lp; endp; endp = endp->next)
		if (*(endp->text)) fputs(endp->text, fd);

	PUTMSG (GGT(string_updated));
	fclose (fd);
	if (!exists) {
		chmod(filename, (mode_t) 0640);
		chown(filename, UUCPUID, UUCPGID);
		exists = 1;
	}
} /* PutFlatItems */

void
CreateBlankEntry()
{
	HostData *dp;

	if ((sf->numFlatItems % INCREMENT) == 0 &&
	   sf->numFlatItems == sf->numAllocated) {
		sf->flatItems = (FlatList *) XtRealloc ((char *)sf->flatItems,
		(sf->numFlatItems + INCREMENT) * sizeof(FlatList));
		sf->numAllocated += INCREMENT;
	}

	if (new) FreeNew();
	new = (FlatList *) XtMalloc (sizeof(FlatList));
	dp = new->pField = (HostData *)XtMalloc(sizeof(HostData));

	dp->f_name = (XtPointer) strdup (BLANKLINE);
	dp->f_time = (XtPointer) strdup("Any");
	dp->f_type = (XtPointer) strdup("ACU");
	dp->f_class = (XtPointer) strdup("Any");
	dp->f_phone = (XtPointer) strdup(BLANKLINE);
	dp->f_expect = (XtPointer) strdup("");
	dp->f_expect1 = (XtPointer) strdup("in:--in:");
	dp->f_login = (XtPointer) strdup("nuucp");
	dp->f_expect2 = (XtPointer) strdup("word:");
	dp->f_passwd = (XtPointer) strdup(BLANKLINE);
	dp->lp = (LinePtr)0;
	dp->changed = False;

} /* CreateBlankEntry */

void
DeleteFlatItems()
{
	HostData *dp;
	int i;

	for (i=0; i<sf->numFlatItems; i++) {
		dp = sf->flatItems[i].pField;
		FreeHostData (dp);
	}
} /* DeleteFlatItems */

LinePtr
GetLine(buf, len, fd)
char *buf;
int len;
FILE *fd;
{
	register char *p;
	LinePtr	lp;
	while ((p = fgets(buf, len, fd)) != NULL) {
		lp = (LinePtr) XtMalloc (sizeof(LineRec));
		lp->next = (LinePtr) NULL;
		lp->text = strdup(p);
		AddLineToBuffer((LinePtr) NULL, lp);
		if (buf[0] == ' ' || buf[0] == '\t' ||  buf[0] == '\n'
		    || buf[0] == '\0' || buf[0] == '#')
			continue;
		return(lp);
	}
	return((LinePtr) NULL);
} /* GetLine */

/*
 * add lp to the position right after ap in the buffer if ap is not null
 * otherwise, add it to the end of the buffer
 */
void
AddLineToBuffer(ap, lp)
LinePtr	ap, lp;
{
	if( sf->lp == NULL)
		sf->lp = lp;
	else {
		if (ap == NULL) {
			LinePtr endp;
			for (endp=sf->lp; endp->next; endp=endp->next);
			endp->next = lp;
		} else {
			lp->next = ap->next;
			ap->next = lp;
		}
	}
} /* AddLineToBuffer */

void
DeleteLineFromBuffer(tp)
LinePtr tp;
{
	register LinePtr lp = sf->lp;
	if (lp == NULL) return;
	if (lp == tp) /* lucky, found it */
		sf->lp = tp->next;
	else
		for (; lp->next; lp = lp->next)
			if (lp->next == tp) {
				lp->next =tp->next;
				break;
			}
	XtFree(tp->text);
	XtFree((char *)tp);
} /* DeleteLineFromBuffer */

void
FreeHostData(dp)
HostData *dp;
{
	XtFree (dp->f_name);
	XtFree (dp->f_time);
	XtFree (dp->f_type);
	XtFree (dp->f_class);
	XtFree (dp->f_phone);
	XtFree (dp->f_expect);
	XtFree (dp->f_expect1);
	XtFree (dp->f_login);
	XtFree (dp->f_expect2);
	XtFree (dp->f_passwd);
	if (dp->lp != NULL)
		DeleteLineFromBuffer(dp->lp);
	XtFree ((XtPointer)dp);
} /* Free */

/* free the associated data structure for the new item */

void
FreeNew()
{
	FreeHostData(new->pField);
	XtFree((XtPointer)new);
	new = (FlatList *) NULL;
} /* FreeNew */

/*
** uses access() to set bits in `stat_flags' corresponding to
** FILE_READABLE and FILE_WRITEABLE.  FILE_EXISTS is set by this function.
** if the file doesn't exist and the file's directory is accessible,
** access() is called on the directory.
*/
void
CheckMode(pathname, stat_flags)
char		*pathname;
unsigned	*stat_flags;
{
	char		dirname[128];
	unsigned short	useruid;
	unsigned short	usergid;
	struct stat	statbuf;

	errno = 0;
	/* for tfadmin */
	if (access(pathname, W_OK) == 0) {
		SET_BIT(*stat_flags, FILE_WRITEABLE | FILE_EXISTS);
	}
	if (access(pathname, R_OK) == 0) {
		SET_BIT(*stat_flags, FILE_READABLE | FILE_EXISTS);
		return;
	} else {
		switch (errno) {
			case ENOENT:	/* File named by `name' doesn't */
					/* exist; check the directory */
				UNSET_BIT(*stat_flags, FILE_EXISTS);
				GetDirectory(pathname, dirname);
				if (access(dirname, W_OK) == -1)
				/* something's wrong here */
					*stat_flags = 0;
				else
					SET_BIT(*stat_flags, FILE_WRITEABLE);
				break;
			case EACCES:
			case EMULTIHOP:
			case ENOLINK:
			case ENOTDIR:
				*stat_flags = 0;
				break;

			default:
				exit(1);
				break;
		}
	}
} /* CheckMode */

/*
** Returns the result of stripping the last component in the pathname in
** dirname.
*/
static void
GetDirectory(pathname, dirname)
char	*pathname;
char	*dirname;
{
	char	*cp;
  
	/*
	** find the last occurance of '/' in pathname
	*/
	cp = strrchr(pathname, '/');

	if (cp) {
		cp++;
		while (pathname != cp)
			*dirname++ = *pathname++;
	} else
		*dirname++ = '.';

	*dirname = '\0';

} /* GetDiretory */
