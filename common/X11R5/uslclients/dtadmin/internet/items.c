/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/items.c	1.31"
#endif

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/Error.h>
#include <FList.h>
#include "inet.h"
#include "error.h"

#define	MAXHOSTNAMELEN	20
#define	MAXUUCPLEN	256
extern void			CreateBlankEntry();
extern void 			GetPermission();
extern void 			GetFlatItems();

static struct HostEntry *	GetLine();
static void 			GetDirectory();
static void			CheckMode();
static void 			ParseAddr();
static void			MergeUUCP();
static int			cmpname();
static Boolean			create_host = False;
static Boolean			create_equiv = False;
static Boolean			create_uucp = False;
#ifdef debug
void			PrintFlatItems();
#endif

/*
** Bit manipulating macros
*/
#define BIT_IS_SET(field, bit)  (((field) & (bit)) != (unsigned)0)
#define SET_BIT(field, bit)     ((field) |= (bit))
#define UNSET_BIT(field, bit)   ((field) &= ~(bit))
/*
** Masks for `stat_flags'
*/
#define	FILE_EXISTS	1
#define FILE_READABLE	2
#define FILE_WRITEABLE	4
#define ROOTUID		(uid_t) 0
#define SYSGID		(gid_t) 3


/*
 * GetFlatItems:
 *	open file
 *	fill in hf->Lines[]
 *	create flatItems arrary
 *	initialize currentItem
 */

void
GetFlatItems(filename)
char * filename;
{
	static FILE	*fd;	/* stream for reading /etc/hosts	*/
	static unsigned	stat_flags = 0;	/* Mask returned by CheckMode() */
	int		exists;		/* Non-zero if file exists */
	int		readable;	/* Non-zero if file is readable */
	int		writeable;	/* Non-zero if file is writable or */
					/* directory is writable and file */
					/* doesn't exist */
	register	i;
	HostData	*dp, *lp;
	struct HostEntry *hp;
	char		buf[BUFSIZ];
	static char	aliases [128];
	Boolean		needed = False;

#ifdef	debug
	fprintf(stderr, "Hosts file: %s", filename);
#endif
	CheckMode(filename, &stat_flags);	/* get the file stat	*/
	exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
	readable = BIT_IS_SET(stat_flags, FILE_READABLE);
	writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
#ifdef debug
	fprintf(stderr, "/etc/hosts exists = %d, readable = %d, writeable = %d\n",
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
		create_host = True;
	} else {
		for (hf->numFlatItems=0; (hp=GetLine(fd)) != NULL;
		     hf->numFlatItems++) {
			/* clear up the previously entry's aliases */
			aliases[0] = 0;
			/* allocate INCREMENT slots for the FlatList */
			if ((hf->numFlatItems % INCREMENT) == 0 &&
			   hf->numFlatItems == hf->numAllocated) {
				hf->flatItems = (FlatList *) XtRealloc (
						(char *)hf->flatItems,
						(hf->numFlatItems + INCREMENT)
						* sizeof(FlatList));
				hf->numAllocated += INCREMENT;
			}
			dp = hf->flatItems[hf->numFlatItems].pField =
				(HostData *)XtMalloc(sizeof(HostData));

			dp->f_name = (XtPointer)strdup(hp->h_name);
			dp->f_comment = (XtPointer) strdup(hp->h_comment);
			dp->f_IPaddr = (XtPointer) strdup(hp->h_addr);
			/* construct aliases list */
			for (i=0; hp->h_aliases[i]; i++) {
				strcat(aliases, hp->h_aliases[i]);
				strcat(aliases, " ");
			}
			if (*aliases)
				dp->f_aliases = (XtPointer) strdup (aliases);
			else
				dp->f_aliases = (XtPointer) strdup(BLANKLINE);
			ParseAddr(dp, hp->h_addr);
			/* copy the local internet address */
			if (!local && !strcmp(dp->f_name, hf->nodeName)) {
				local = (FlatList *) XtMalloc (sizeof(FlatList));
				lp = local->pField=
					(HostData *)XtMalloc(sizeof(HostData));
				lp->f_net1Addr = (XtPointer)strdup(dp->f_net1Addr);
				lp->f_net2Addr = (XtPointer)strdup(dp->f_net2Addr);
				lp->f_net3Addr = (XtPointer)strdup(dp->f_net3Addr);
			}
			/* start to check the permission file */
			dp->equiv = (XtPointer) strdup(BLANKLINE);
			dp->f_allow = NONE;

			dp->index = hf->numLines -1;
			dp->changed = False;
		}
	}

	if (hf->numFlatItems == 0) {
		hf->currentItem = -1;
	} else {
		hf->currentItem = 0;
#ifdef debug
		PrintFlatItems();
#endif
	}
	if (fd) fclose (fd);

} /* GetFlatItems */

#ifdef debug
void
PrintFlatItems()
{
	register i;
	HostData *fp;
	char text [128];

	for (i=0; i<hf->numFlatItems; i++) {
		fp = hf->flatItems[i].pField;
		fprintf (stdout,
			"%3d %s %s %s #%s\n",
			 fp->index,
			 fp->f_name,
			 fp->f_IPaddr,
			 fp->f_aliases,
			 fp->f_comment);
	}
} /* PrintFlatItems */
#endif

void
PutFlatItems(filename)
char *filename;
{
	static FILE	*fd, *rfd;
	HostData	*fp;
	UucpEntry	*up;
	register	i;
	char		buf [BUFSIZ*4];
	char		rhosts [BUFSIZ*4];
	char		text [128];
	char		rbuf [128];
	char 		*cp, *bp;

	rhosts[0] = NULL;
	buf[0] = NULL;
	
	/* allocate enough memory for the uucp table */
	hf->numUucp = hf->numFlatItems;
	hf->uucpItems = (UucpEntry *)XtMalloc (hf->numUucp * sizeof(UucpEntry));

	for (i=0; i<hf->numFlatItems; i++) {
		fp = hf->flatItems[i].pField;
		sprintf (text,
			"%s\t%s\t%s\t#%s\n",
			fp->f_IPaddr,
			fp->f_name,
			fp->f_aliases,
			fp->f_comment);
		
		if (fp->f_allow == SPECIFY) {
			bp = strdup(fp->equiv);
			for (cp=bp; *cp!='\0'; cp++) {
				if (*cp == '\n') {
					*cp = '\0';
					sprintf(rbuf,"%s %s\n", fp->f_name, bp);
					strcat(rhosts, rbuf);
					bp = cp + 1;
				}
			}
		} else if (fp->f_allow == ALL) {
			strcat(rhosts, fp->f_name);
			strcat(rhosts, "\n");
		}
 
		if ( fp->index == -1 ) {	/* new item added */
	
			if ((hf->numLines % INCREMENT) == 0 &&
			   hf->numLines == hf->numLinesAllocated) {
				hf->Lines = (char **) XtRealloc ((char *)hf->Lines,
				(hf->numLines + INCREMENT) * sizeof(char **));
				hf->numLinesAllocated += INCREMENT;
				/* set all to zeros */
				memset(&hf->Lines[hf->numLines], 0,
					sizeof(char *)*INCREMENT);
			}
			hf->Lines[fp->index=hf->numLines++] = strdup(text);
		} else {
			XtFree (hf->Lines[fp->index]);
			hf->Lines[fp->index] = strdup(text);
			fp->changed = False;
		}
		/* prepare entry for uucp Systems.tcp table */
		hf->uucpItems[i].u_name = strdup(fp->f_name);
		sprintf(buf, "%s%02x%02x%02x%02x\n",
			UUCPPREFIX,
			atoi(fp->f_net1Addr),
			atoi(fp->f_net2Addr),
			atoi(fp->f_net3Addr),
			atoi(fp->f_net4Addr));
		hf->uucpItems[i].u_rest = strdup(buf);
	}
	MergeUUCP();
	if ((fd = fopen(filename, "w")) == NULL) {
		sprintf (text, GGT(string_fopenWrite), filename);
		PUTMSG (text);
		return;
	}
	if (create_host == True)
		fputs("127.0.0.1	localhost loopback me\n", fd);
	for (i = 0; hf->Lines[i]; i++) {
		if (hf->Lines[i] == (char *) -1) continue; /* deleted items */
		if (*(hf->Lines[i])) fputs(hf->Lines[i], fd);
	}
	if ((rfd = fopen(permission_path, "w")) == NULL) {
		sprintf (text, GGT(string_fopenWrite), permission_path);
		PUTMSG (text);
		return;
	}
	fputs(rhosts, rfd);

	sprintf (text, "%s", GGT(string_updated));
	PUTMSG (text);
	fclose (fd);
	fclose (rfd);
	if (create_host == True) {
		chmod(filename, (mode_t) 0444);
		chown(filename, ROOTUID, SYSGID);
		create_host = False;
	}
	if (create_equiv == True) {
		chmod(permission_path, (mode_t) 0444);
		chown(permission_path, ROOTUID, SYSGID);
		create_equiv = False;
	}
} /* PutFlatItems */

void
CreateBlankEntry()
{
	HostData *dp, *lp;

	if ((hf->numFlatItems % INCREMENT) == 0 &&
	   hf->numFlatItems == hf->numAllocated) {
		hf->flatItems = (FlatList *) XtRealloc ((char *)hf->flatItems,
		(hf->numFlatItems + INCREMENT) * sizeof(FlatList));
		hf->numAllocated += INCREMENT;
	}

	if (new) FreeNew();
	new = (FlatList *) XtMalloc (sizeof(FlatList));
	dp = new->pField = (HostData *)XtMalloc(sizeof(HostData));

	dp->f_name = (XtPointer) strdup (BLANKLINE);
	if (local) {
		lp = local->pField;
		dp->f_net1Addr = (XtPointer) strdup(lp->f_net1Addr);
		dp->f_net2Addr = (XtPointer) strdup(lp->f_net2Addr);
		dp->f_net3Addr = (XtPointer) strdup(lp->f_net3Addr);
	} else {
		dp->f_net1Addr = (XtPointer) strdup("223");
		dp->f_net2Addr = (XtPointer) strdup("254");
		dp->f_net3Addr = (XtPointer) strdup("254");
	}
	dp->f_net4Addr = (XtPointer) strdup(BLANKLINE);
	/* 4*3+3+1=16					*/
	/* four octets each 3 chars (4*3)		*/
	/* 3 dots (+3)					*/
	/* terminated Null character (+1)		*/
	dp->f_IPaddr = (char *) XtMalloc(sizeof(char) * 16);
	sprintf(dp->f_IPaddr,"%s.%s.%s.%s",
				dp->f_net1Addr,
				dp->f_net2Addr,
				dp->f_net3Addr,
				dp->f_net4Addr
		);
	dp->f_comment = (XtPointer) strdup(BLANKLINE);
	dp->f_aliases = (XtPointer) strdup(BLANKLINE);
	dp->equiv = (XtPointer) strdup(BLANKLINE);
	dp->f_allow = NONE;
	dp->index = -1;
	dp->changed = False;

} /* CreateBlankEntry */

void
DeleteFlatItems()
{
	register i;
	HostData *dp;

#ifdef later
	if (local) {
		fprintf(stderr, "local = %x\n", local);
		fprintf(stderr, "local->pField = %x\n", local->pField);
		FreeHostData(local->pField);
		XtFree((XtPointer)local);
		local = (FlatList *) NULL;
	}
#endif

	for (i=0; i<hf->numFlatItems; i++) {
		dp = hf->flatItems[i].pField;
		FreeHostData(dp);
	}
#ifdef later
	if (hf->flatItems) {
		fprintf(stderr, "hf->flatItems = %x\n", hf->flatItems);
		XtFree((XtPointer)hf->flatItems);
		hf->flatItems = (FlatList *) NULL;
	}
#endif
	hf->numAllocated = 0;
	hf->numFlatItems = 0;
	hf->currentItem = -1;
	hf->changesMade = False;
} /* DeleteFlatItems */

FreeHostData(dp)
HostData *dp;
{
	XtFree (dp->f_name);
	XtFree (dp->f_IPaddr);
	XtFree (dp->f_net4Addr);
	XtFree (dp->f_net1Addr);
	XtFree (dp->f_net2Addr);
	XtFree (dp->f_net3Addr);
	XtFree (dp->f_aliases);
	XtFree (dp->f_comment);
	XtFree (dp->equiv);
	XtFree ((XtPointer)dp);
}

extern
FreeNew()
{
	FreeHostData(new->pField);
	XtFree((XtPointer)new);
	new = (FlatList *) NULL;
}

void
DeleteLineBuffer()
{
	register i;

	if (hf->Lines == NULL)
		return;
	for (i=0; i<hf->numLines; i++) {
		if (hf->Lines[i] == (char *) -1) continue; /* deleted items */
		XtFree(hf->Lines[i]);
	}
	XtFree((XtPointer)hf->Lines);
	hf->numLines = 0;
	hf->numLinesAllocated = 0;
	hf->Lines = (char **) NULL;
} /* DeleteLineBuffer */

#ifdef SYSV
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#endif SYSV

static char line[BUFSIZ+1];
static char hostaddr[MAXADDRSIZE];
static struct HostEntry host;
static char *host_aliases[MAXALIASES];
static char *host_addrs[] = {
	hostaddr,
	NULL
};

struct HostEntry *
GetLine(hostf)
FILE * hostf;
{
	register i;
	char *p;
	register char *cp, **q;

again:
	memset(line, 0, BUFSIZ+1);
	host.h_name=NULL;
	host.h_aliases=NULL;
	host.h_addr=NULL;
	host.h_comment=NULL;
	if ((p = fgets(line, BUFSIZ, hostf)) == NULL)
		return (NULL);

	for (i=0; host_aliases[i]; i++) host_aliases[i] = NULL;
	
	if ((hf->numLines % INCREMENT) == 0 &&
	   hf->numLines == hf->numLinesAllocated) {
		hf->Lines = (char **) XtRealloc ((char *)hf->Lines,
		(hf->numLines + INCREMENT) * sizeof(char **));
		hf->numLinesAllocated += INCREMENT;
		/* set all to zeros */
		memset(&hf->Lines[hf->numLines], 0,
			sizeof(char *)*INCREMENT);
	}

	hf->Lines[hf->numLines++] = strdup(p);
	if (*p == '#') { /* comment line by itself, save it */
		goto again;
	}
	cp = strpbrk(p, "\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	host.h_addr = p;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
	}
	if (strcmp(host.h_name, "me") == 0 || strcmp(host.h_name, "localhost") == 0)
		goto again;
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (*cp == '#') {
			*cp++ = '0';
			host.h_comment = cp;
			return (&host);
		}
		if (q < &host_aliases[MAXALIASES - 1])
			*q++ = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
	}
	*q = NULL;
	return (&host);
}

#define UC(b)	(((int)b)&0xff)
static void
ParseAddr (fp, cp)
HostData * fp;
register char * cp;
{
	static char b[4];
	register char * p;
	u_long theaddr;

	theaddr = inet_addr(cp);
	p = (char *) &theaddr;
	sprintf(b, "%d", UC(p[0]));
	fp->f_net1Addr = strdup(b);
	sprintf(b, "%d", UC(p[1]));
	fp->f_net2Addr = strdup(b);
	sprintf(b, "%d", UC(p[2]));
	fp->f_net3Addr = strdup(b);
	sprintf(b, "%d", UC(p[3]));
	fp->f_net4Addr = strdup( b);
} /* ParseAddr */

void
GetPermission()
{
    static FILE    *fd;    /* stream for reading /etc/hosts.equiv    */
    char *user;
    char ahost[MAXHOSTNAMELEN];
    Boolean hostmatch;
    register char *p;
    int    allocated = 0;
    Boolean        permission_flag = True;
    static unsigned    stat_flags = 0;    /* Mask returned by CheckMode() */
    int        exists;        /* Non-zero if file exists */
    int        readable;    /* Non-zero if file is readable */
    int        writeable;    /* Non-zero if file is writable or */
                    /* directory is writable and file */
                    /* doesn't exist */
    register    i;
    HostData    *fp;
    char        buf[BUFSIZ];


    CheckMode(permission_path, &stat_flags);    /* get the file stat */
    exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
    readable = BIT_IS_SET(stat_flags, FILE_READABLE);
    writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
    #ifdef debug
        fprintf(stderr, "/etc/hosts.equiv exists = %d, readable = %d, writeable = %d\n",
    	exists, readable, writeable);
    #endif
    if (!exists & !writeable) { /* somthing's serious wrong */
        fprintf(stderr, GGT(string_createFail), permission_path);
    } else
    if (!readable & exists) {
        fprintf(stderr, GGT(string_accessFail), permission_path);
    } else
    if (exists & readable & !writeable) {
        fprintf(stderr, GGT(string_writeFail), permission_path);
    }

    if ((fd = fopen(permission_path, "r")) == (FILE *)NULL) {
        /* Something's wrong here but there are some chances    */
        /* we might be able to create the file if we are the    */
        /* prviledged user.  For now just put out message    */
#ifdef debug
	fprintf(stderr, GGT(string_openFail), permission_path);
#endif
	create_equiv = True;
    } else {
        while (fgets(ahost, sizeof (ahost), fd)) {
            hostmatch = False;
            p = ahost;
            while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
                *p = isupper(*p) ? tolower(*p) : *p;
                p++;
            }
            /* there is a user specified */
            if (*p == ' ' || *p == '\t') {
                *p++ = '\0';
                while (*p == ' ' || *p == '\t')
                    p++;
                user = p;
                while(*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
                    p++;
            } else
                user = p;
            *p = '\0';
            for (i=0, fp= hf->flatItems[i].pField; i<hf->numFlatItems; i++) {
                fp = hf->flatItems[i].pField;
                if (hostmatch = !strcmp(fp->f_name, ahost)) {
                    if (user[0]) {
                        fp->f_allow = SPECIFY;
                        fp->equiv = XtRealloc(fp->equiv,
                                allocated += strlen(user)+2);
                        strcat(fp->equiv, user);
                        strcat(fp->equiv, "\n");
                    } else {
                        /* 1) no need to check further based on current
                         *    implemnetation of tcp/ip stuff.
                         * 2) release the equiv list collected so far.
                         */ 
                        if( fp->f_allow != ALL) {
                            fp->f_allow = ALL;
                            if (*fp->equiv) {
                                XtFree(fp->equiv);
                                fp->equiv = NULL;
                            }
                        }
                    }
                break;
                }
            }
        }
    }
    if (fd != NULL) fclose(fd);
} /* GetPermission */

/*
** uses CheckMode() to set bits in `stat_flags' corresponding to
** FILE_READABLE and FILE_WRITEABLE.  FILE_EXISTS is set by this function.
** if the file doesn't exist and the file's directory is stat(2)'able,
** CheckMode() is called on the directory.
*/
static void
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
				UNSET_BIT(*stat_flags, FILE_READABLE);
				break;
			default:
				*stat_flags = 0;
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

static int
cmpname(x, y)
UucpEntry *x;
UucpEntry *y;
{
	return strcoll(x->u_name, y->u_name);
} /* cmpname */

static void
MergeUUCP()
{
    static FILE    *fd;    /* stream for reading /etc/uucp/Systems.tcp    */
    int  n;
    int  numItem;
    char *cp;
    char ahost[MAXUUCPLEN];
    register char *p;
    static unsigned    stat_flags = 0;    /* Mask returned by CheckMode() */
    int        exists;        /* Non-zero if file exists */
    int        readable;    /* Non-zero if file is readable */
    int        writeable;    /* Non-zero if file is writable or */
                    /* directory is writable and file */
                    /* doesn't exist */
    register    i;
    HostData    *fp;
    char        buf[BUFSIZ];


    CheckMode(uucp_path, &stat_flags);    /* get the file stat */
    exists = BIT_IS_SET(stat_flags, FILE_EXISTS);
    readable = BIT_IS_SET(stat_flags, FILE_READABLE);
    writeable = BIT_IS_SET(stat_flags, FILE_WRITEABLE);
    #ifdef debug
        fprintf(stderr,
        "/etc/uucp/Systems.tcp exists = %d, readable = %d, writeable = %d\n",
    	exists, readable, writeable);
    #endif
    if (!exists & !writeable) { /* somthing's serious wrong */
        fprintf(stderr, GGT(string_createFail), uucp_path);
	return;
    } else
    if (!readable & exists) {
        fprintf(stderr, GGT(string_accessFail), uucp_path);
	return;
    } else
    if (exists & readable & !writeable) {
        fprintf(stderr, GGT(string_writeFail), uucp_path);
	return;
    }

    qsort((void *) hf->uucpItems, hf->numUucp, sizeof(UucpEntry), cmpname);
    numItem = hf->numUucp;

    if ((fd = fopen(uucp_path, "r")) == (FILE *)NULL) {
        /* Something's wrong here but there are some chances    */
        /* we might be able to create the file if we are the    */
        /* prviledged user.  For now just put out message    */
#ifdef debug
	fprintf(stderr, GGT(string_openFail), uucp_path);
#endif
	create_uucp = True;
    } else {
        while (fgets(ahost, sizeof (ahost), fd)) {
	    if (*ahost == '#') continue;
	    cp = strpbrk(ahost, "\n");
	    if (cp == NULL) continue;
            p = ahost;
            while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
                p++;
            }
            if (*p == ' ' || *p == '\t') {
                *p++ = '\0';
		while (*p == ' ' || *p == '\t') p++;
	        if (strncmp(ahost, hf->uucpItems[0].u_name, n=strlen(ahost)) < 0)
		    goto allocate;
	        if (strncmp(ahost, hf->uucpItems[numItem -1].u_name, n) > 0)
		    goto allocate;
		for (i = 0; i < numItem ; i++) {
                    if (strcmp(ahost, hf->uucpItems[i].u_name) == 0) 
                        goto next;
                    if (strncmp(ahost, hf->uucpItems[i].u_name, n) < 0) {
allocate:
                        hf->uucpItems = (UucpEntry *) XtRealloc (
                                        (char *)hf->uucpItems,
                                        (hf->numUucp + 1) * sizeof(UucpEntry));
			hf->uucpItems[hf->numUucp].u_name = strdup(ahost);
			hf->uucpItems[hf->numUucp++].u_rest = strdup(p);
                        break;
                    }
		}
            } else continue;
next:;
        }
    }
    if (fd) fclose (fd);
    fd = fopen(uucp_path, "w");
    for (i = 0; i<hf->numUucp; i++) {
	sprintf(buf,"%s\t\t%s",
		hf->uucpItems[i].u_name,
		hf->uucpItems[i].u_rest);
	fputs(buf, fd);
	XtFree(hf->uucpItems[i].u_name);
	XtFree(hf->uucpItems[i].u_rest);
    }
    XtFree((XtPointer)hf->uucpItems);
    hf->uucpItems = (UucpEntry *)NULL;
    if (fd) fclose(fd);
    if (create_uucp == True) {
	chmod(uucp_path, (mode_t) 0444);
	chown(uucp_path, ROOTUID, SYSGID);
	create_equiv = False;
    }

} /* MergeUUCP */
