#ident	"@(#)xauth:process.c	1.1"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * $XConsortium: process.c,v 1.26 89/01/03 11:54:21 jim Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;			/* for stupid errno.h files */
#include <signal.h>
#include "xauth.h"
#include <X11/X.h>			/* for Family constants */

#if !defined(SYSV) && !defined(SVR4)
extern char *get_hostname();
extern Bool nameserver_timedout;
#endif

#define ATT_PROTOCOL_ABBREV ":"
#define ATT_PROTOCOL "AT&T-MAGIC-COOKIE-1"

#ifndef DEFAULT_PROTOCOL_ABBREV		/* to make add command easier */
#define DEFAULT_PROTOCOL_ABBREV "."
#endif
#ifndef DEFAULT_PROTOCOL		/* for protocol abbreviation */
#define DEFAULT_PROTOCOL "MIT-MAGIC-COOKIE-1"
#endif

#define XAUTH_DEFAULT_RETRIES 2		/* just a few times */
#define XAUTH_DEFAULT_TIMEOUT 2		/* in seconds, be quick */
#define XAUTH_DEFAULT_DEADTIME 600L	/* 10 minutes in seconds */

typedef struct _AuthList {		/* linked list of entries */
    struct _AuthList *next;
    Xauth *auth;
} AuthList;

#define add_to_list(h,t,e) {if (t) (t)->next = (e); else (h) = (e); (t) = (e);}

typedef struct _CommandTable {		/* commands that are understood */
    char *name;				/* full name */
    int minlen;				/* unique prefix */
    int maxlen;				/* strlen(name) */
    int (*processfunc)();		/* handler */
    char *helptext;			/* what to print for help */
} CommandTable;

struct _extract_data {			/* for iterating */
    FILE *fp;				/* input source */
    char *filename;			/* name of input */
    Bool used_stdout;			/* whether or not need to close */
    Bool numeric;			/* format in which to write */
    int nwritten;			/* number of entries written */
    char *cmd;				/* for error messages */
};

struct _list_data {			/* for iterating */
    FILE *fp;				/* output file */
    Bool numeric;			/* format in which to write */
};


/*
 * private data
 */
static char *stdin_filename = "(stdin)";  /* for messages */
static char *stdout_filename = "(stdout)";  /* for messages */
static char *Yes = "yes";		/* for messages */
static char *No = "no";			/* for messages */

static int do_list(), do_merge(), do_extract(), do_add(), do_remove();
static int do_help(), do_source(), do_info(), do_exit();
static int do_quit(), do_questionmark();

CommandTable command_table[] = {	/* table of known commands */
    { "add",      2, 3, do_add,
	"add dpyname protoname hexkey   add entry" },
    { "exit",     3, 4, do_exit,
	"exit                           save changes and exit program" },
    { "extract",  3, 7, do_extract,
	"extract filename dpyname...    extract entries into file" },
    { "help",     1, 4, do_help,
	"help [topic]                   print help" },
    { "info",     1, 4, do_info,
	"info                           print information about entries" },
    { "list",     1, 4, do_list,
	"list [dpyname...]              list entries" },
    { "merge",    1, 5, do_merge,
	"merge filename...              merge entries from files" },
    { "nextract", 2, 8, do_extract,
	"nextract filename dpyname...   numerically extract entries" },
    { "nlist",    2, 5, do_list,
	"nlist [dpyname...]             numerically list entries" },
    { "nmerge",   2, 6, do_merge,
	"nmerge filename...             numerically merge entries" },
    { "quit",     1, 4, do_quit,
	"quit                           abort changes and exit program" },
    { "remove",   1, 6, do_remove,
	"remove dpyname...              remove entries" },
    { "source",   1, 6, do_source,
	"source filename                read commands from file" },
    { "?",        1, 1, do_questionmark,
	"?                              list available commands" },
    { NULL,       0, 0, NULL, NULL },
};

#define COMMAND_NAMES_PADDED_WIDTH 10	/* wider than anything above */


static Bool okay_to_use_stdin = True;	/* set to false after using */

static char *hex_table[] = {		/* for printing hex digits */
    "00", "01", "02", "03", "04", "05", "06", "07", 
    "08", "09", "0a", "0b", "0c", "0d", "0e", "0f", 
    "10", "11", "12", "13", "14", "15", "16", "17", 
    "18", "19", "1a", "1b", "1c", "1d", "1e", "1f", 
    "20", "21", "22", "23", "24", "25", "26", "27", 
    "28", "29", "2a", "2b", "2c", "2d", "2e", "2f", 
    "30", "31", "32", "33", "34", "35", "36", "37", 
    "38", "39", "3a", "3b", "3c", "3d", "3e", "3f", 
    "40", "41", "42", "43", "44", "45", "46", "47", 
    "48", "49", "4a", "4b", "4c", "4d", "4e", "4f", 
    "50", "51", "52", "53", "54", "55", "56", "57", 
    "58", "59", "5a", "5b", "5c", "5d", "5e", "5f", 
    "60", "61", "62", "63", "64", "65", "66", "67", 
    "68", "69", "6a", "6b", "6c", "6d", "6e", "6f", 
    "70", "71", "72", "73", "74", "75", "76", "77", 
    "78", "79", "7a", "7b", "7c", "7d", "7e", "7f", 
    "80", "81", "82", "83", "84", "85", "86", "87", 
    "88", "89", "8a", "8b", "8c", "8d", "8e", "8f", 
    "90", "91", "92", "93", "94", "95", "96", "97", 
    "98", "99", "9a", "9b", "9c", "9d", "9e", "9f", 
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", 
    "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af", 
    "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", 
    "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf", 
    "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", 
    "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf", 
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", 
    "d8", "d9", "da", "db", "dc", "dd", "de", "df", 
    "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", 
    "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef", 
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", 
    "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff", 
};

static unsigned int hexvalues[256];	/* for parsing hex input */

static int original_umask = 0;		/* for restoring */


/*
 * private utility procedures
 */

static void prefix (fn, n)
    char *fn;
    int n;
{
    fprintf (stderr, "%s: %s:%d:  ", ProgramName, fn, n);
}

static void baddisplayname (dpy, cmd)
    char *dpy, *cmd;
{
    fprintf (stderr, "bad display name \"%s\" in \"%s\" command\n",
	     dpy, cmd);
}

static void badcommandline (cmd)
    char *cmd;
{
    fprintf (stderr, "bad \"%s\" command line\n", cmd);
}

static int parse_boolean (s)
    char *s;
{
    static char *true_names[] = { "on", "t", "true", "yes", "y", NULL };
    static char *false_names[] = { "off", "f", "false", "no", "n", NULL };
    register char **cpp;

    if (s) {
	for (cpp = false_names; *cpp; cpp++) {
	    if (strcmp (s, *cpp) == 0) return 0;
	}
	for (cpp = true_names; *cpp; cpp++) {
	    if (strcmp (s, *cpp) == 0) return 1;
	}
    }
    return -1;
}

static char *skip_space (s)
    register char *s;
{
    register char c;

    if (!s) return NULL;

    for (; (c = *s) && isascii(c) && isspace(c); s++) ;
    return s;
}


static char *skip_nonspace (s)
    register char *s;
{
    register char c;

    if (!s) return NULL;

    /* put quoting into loop if need be */
    for (; (c = *s) && isascii(c) && !isspace(c); s++) ;
    return s;
}

static char **split_into_words (src, argcp)  /* argvify string */
    char *src;
    int *argcp;
{
    char *word;
    char savec;
    char **argv;
    int cur, total;

    *argcp = 0;
#define WORDSTOALLOC 4			/* most lines are short */
    argv = (char **) malloc (WORDSTOALLOC * sizeof (char *));
    if (!argv) return NULL;
    cur = 0;
    total = WORDSTOALLOC;

    /*
     * split the line up into separate, nul-terminated tokens; the last
     * "token" will point to the empty string so that it can be bashed into
     * a null pointer.
     */

    do {
	word = skip_space (src);
	src = skip_nonspace (word);
	savec = *src;
	*src = '\0';
	if (cur == total) {
	    total += WORDSTOALLOC;
	    argv = (char **) realloc (argv, total * sizeof (char *));
	    if (!argv) return NULL;
	}
	argv[cur++] = word;
	if (savec) src++;		/* if not last on line advance */
    } while (word != src);

    argv[--cur] = NULL;			/* smash empty token to end list */
    *argcp = cur;
    return argv;
}


static FILE *open_file (filenamep, mode, usedstdp, srcfn, srcln, cmd)
    char **filenamep;
    char *mode;
    Bool *usedstdp;
    char *srcfn;
    int srcln;
    char *cmd;
{
    FILE *fp;

    if (strcmp (*filenamep, "-") == 0) {
	*usedstdp = True;
					/* select std descriptor to use */
	if (mode[0] == 'r') {
	    if (okay_to_use_stdin) {
		okay_to_use_stdin = False;
		*filenamep = stdin_filename;
		return stdin;
	    } else {
		prefix (srcfn, srcln);
		fprintf (stderr, "%s:  stdin already in use\n", cmd);
		return NULL;
	    }
	} else {
	    *filenamep = stdout_filename;
	    return stdout;		/* always okay to use stdout */
	}
    }

    fp = fopen (*filenamep, mode);
    if (!fp) {
	prefix (srcfn, srcln);
	fprintf (stderr, "%s:  unable to open file %s\n", cmd, *filenamep);
    }
    return fp;
}

static int getinput (fp)
    FILE *fp;
{
    register int c;

    while ((c = getc (fp)) != EOF && isascii(c) && c != '\n' && isspace(c)) ;
    return c;
}

static int get_short (fp, sp)		/* for reading numeric input */
    FILE *fp;
    unsigned short *sp;
{
    int c;
    int i;
    unsigned short us = 0;

    /*
     * read family:  written with %04x
     */
    for (i = 0; i < 4; i++) {
	switch (c = getinput (fp)) {
	  case EOF:
	  case '\n':
	    return 0;
	}
	if (c < 0 || c > 255) return 0;
	us = (us * 16) + hexvalues[c];	/* since msb */
    }
    *sp = us;
    return 1;
}

static int get_bytes (fp, n, ptr)	/* for reading numeric input */
    FILE *fp;
    unsigned int n;
    char **ptr;
{
    char *s;
    register char *cp;
    char buf[2];

    cp = s = malloc (n);
    if (!cp) return 0;

    while (n > 0) {
	if ((buf[0] = getinput (fp)) == EOF || buf[0] == '\n' ||
	    (buf[1] = getinput (fp)) == EOF || buf[1] == '\n') {
	    free (s);
	    return 0;
	}
	*cp = (char) ((hexvalues[(unsigned int)buf[0]] * 16) + 
		      hexvalues[(unsigned int)buf[1]]);
	cp++;
	n--;
    }

    *ptr = s;
    return 1;
}


static Xauth *read_numeric (fp)
    FILE *fp;
{
    Xauth *auth;

    auth = (Xauth *) malloc (sizeof (Xauth));
    if (!auth) goto bad;
    auth->family = 0;
    auth->address = NULL;
    auth->address_length = 0;
    auth->number = NULL;
    auth->number_length = 0;
    auth->name = NULL;
    auth->name_length = 0;
    auth->data = NULL;
    auth->data_length = 0;

    if (!get_short (fp, (unsigned short *) &auth->family))
      goto bad;
    if (!get_short (fp, (unsigned short *) &auth->address_length))
      goto bad;
    if (!get_bytes (fp, (unsigned int) auth->address_length, &auth->address))
      goto bad;
    if (!get_short (fp, (unsigned short *) &auth->number_length))
      goto bad;
    if (!get_bytes (fp, (unsigned int) auth->number_length, &auth->number))
      goto bad;
    if (!get_short (fp, (unsigned short *) &auth->name_length))
      goto bad;
    if (!get_bytes (fp, (unsigned int) auth->name_length, &auth->name))
      goto bad;
    if (!get_short (fp, (unsigned short *) &auth->data_length))
      goto bad;
    if (!get_bytes (fp, (unsigned int) auth->data_length, &auth->data))
      goto bad;
    
    switch (getinput (fp)) {		/* get end of line */
      case EOF:
      case '\n':
	return auth;
    }

  bad:
    if (auth) XauDisposeAuth (auth);	/* won't free null pointers */
    return NULL;
}

static int read_auth_entries (fp, numeric, headp, tailp)
    FILE *fp;
    Bool numeric;
    AuthList **headp, **tailp;
{
    Xauth *((*readfunc)()) = (numeric ? read_numeric : XauReadAuth);
    Xauth *auth;
    AuthList *head, *tail;
    int n;

    head = tail = NULL;
    n = 0;
					/* put all records into linked list */
    while ((auth = ((*readfunc) (fp))) != NULL) {
	AuthList *l = (AuthList *) malloc (sizeof (AuthList));
	if (!l) {
	    fprintf (stderr,
		     "%s:  unable to alloc entry reading auth file\n",
		     ProgramName);
	    exit (1);
	}
	l->next = NULL;
	l->auth = auth;
	if (tail) 			/* if not first time through append */
	  tail->next = l;
	else
	  head = l;			/* first time through, so assign */
	tail = l;
	n++;
    }
    *headp = head;
    *tailp = tail;
    return n;
}

static Bool get_displayname_auth (displayname, auth)
    char *displayname;
    Xauth *auth;			/* fill in */
{
    int family;
    char *host = NULL, *rest = NULL;
    int dpynum, scrnum;
    char *cp;
    int len;
#if !defined(SYSV) && !defined(SVR4)
    extern char *get_address_info();
#endif
    Xauth proto;
    int prelen = 0;

    /*
     * check to see if the display name is of the form "host/unix:"
     * which is how the list routine prints out local connections
     */
    cp = index (displayname, '/');
    if (cp && strncmp (cp, "/unix:", 6) == 0)
      prelen = (cp - displayname);

    if (!parse_displayname (displayname + ((prelen > 0) ? prelen + 1 : 0),
			    &family, &host, &dpynum, &scrnum, &rest)) {
	return False;
    }

#if !defined(SYSV) && !defined(SVR4)
    proto.family = family;
    proto.address = get_address_info (family, displayname, prelen, host, &len);
#else
    if(host){
    	proto.family = FamilyWild;
    	len = strlen(host);
    	proto.address = copystring(host, len);
     }
     else proto.address = NULL;
#endif
    if (proto.address) {
	char buf[40];			/* want to hold largest display num */

	proto.address_length = len;
	buf[0] = '\0';
	sprintf (buf, "%d", dpynum);
	proto.number_length = strlen (buf);
	if (proto.number_length <= 0) {
	    free (proto.address);
	    proto.address = NULL;
	} else {
	    proto.number = copystring (buf, proto.number_length);
	}
    }

    if (host) free (host);
    if (rest) free (rest);

    if (proto.address) {
	auth->family = proto.family;
	auth->address = proto.address;
	auth->address_length = proto.address_length;
	auth->number = proto.number;
	auth->number_length = proto.number_length;
	auth->name = NULL;
	auth->name_length = 0;
	auth->data = NULL;
	auth->data_length = 0;
	return True;
    } else {
	return False;
    }
}

static int cvthexkey (hexstr, ptrp)	/* turn hex key string into octets */
    char *hexstr;
    char **ptrp;
{
    int i;
    int len = 0;
    char *retval, *s;
    unsigned char *us;
    char savec;
    int whichchar;
    static char *hexdigits = "0123456789abcdef";

    /* convert to lower case and count */
    for (s = hexstr; *s; s++) {
	if (!isascii(*s)) return -1;
	if (isspace(*s)) continue;
	if (!isxdigit(*s)) return -1;
	len++;
    }

    /* if odd then there was an error */
    if ((len & 1) == 1) return -1;


    /* now we know that the input is good */
    len >>= 1;
    retval = malloc (len);
    if (!retval) {
	fprintf (stderr, "%s:  unable to allocate %d bytes for hexkey\n",
		 ProgramName, len);
	return -1;
    }

    whichchar = 0;
    for (us = (unsigned char *) retval, i = len; i > 0; hexstr++) {
	if (isspace(*hexstr)) continue;	 /* already know it is ascii */
	if (whichchar) {
#define atoh(c) ((c) - (((c) >= '0' && (c) <= '9') ? '0' : ('a' - 10)))
	    *us = (unsigned char)((atoh(savec) << 4) + atoh(*hexstr));
#undef atoh
	    whichchar = 0;		/* ready for next character */
	    us++;
	    i--;
	} else {
	    savec = *hexstr;
	    whichchar = 1;
	}
    }
    *ptrp = retval;
    return len;
}

static int dispatch_command (inputfilename, lineno, argc, argv, tab, statusp)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
    CommandTable *tab;
    int *statusp;
{
    CommandTable *ct;
    char *cmd;
    int n;
					/* scan table for command */
    cmd = argv[0];
    n = strlen (cmd);
    for (ct = tab; ct->name; ct++) {
					/* look for unique prefix */
	if (n >= ct->minlen && n <= ct->maxlen &&
	    strncmp (cmd, ct->name, n) == 0) {
	    *statusp = (*(ct->processfunc))(inputfilename, lineno, argc, argv);
	    return 1;
	}
    }

    *statusp = 1;
    return 0;
}


static AuthList *xauth_head = NULL;	/* list of auth entries */
static Bool xauth_existed = False;	/* if was present at initialize */
static Bool xauth_modified = False;	/* if added, removed, or merged */
static Bool xauth_allowed = True;	/* if allowed to write auth file */
static char *xauth_filename = NULL;
static Bool dieing = False;

static die ()
{
    dieing = True;
    exit (auth_finalize ());
    /* NOTREACHED */
}

static catchsig (sig)
    int sig;
{
#if defined(SYSV) || defined(SVR4)
    if (sig > 0) signal (sig, (void(*)())die);	/* re-establish signal handler */
#endif
    if (verbose && xauth_modified) printf ("\r\n");
    die ();
    /* NOTREACHED */
}

static void register_signals ()
{
    signal (SIGINT, (void(*)())catchsig);
    signal (SIGTERM, (void(*)())catchsig);
    signal (SIGHUP, (void(*)())catchsig);
    return;
}


/*
 * public procedures for parsing lines of input
 */

int auth_initialize (authfilename)
    char *authfilename;
{
    int n;
    AuthList *head, *tail;
    FILE *authfp;
    Bool exists;

    register_signals ();

    bzero ((char *) hexvalues, sizeof hexvalues);
    hexvalues['0'] = 0;
    hexvalues['1'] = 1;
    hexvalues['2'] = 2;
    hexvalues['3'] = 3;
    hexvalues['4'] = 4;
    hexvalues['5'] = 5;
    hexvalues['6'] = 6;
    hexvalues['7'] = 7;
    hexvalues['8'] = 8;
    hexvalues['9'] = 9;
    hexvalues['a'] = hexvalues['A'] = 0xa;
    hexvalues['b'] = hexvalues['B'] = 0xb;
    hexvalues['c'] = hexvalues['C'] = 0xc;
    hexvalues['d'] = hexvalues['D'] = 0xd;
    hexvalues['e'] = hexvalues['E'] = 0xe;
    hexvalues['f'] = hexvalues['F'] = 0xf;

    exists = (access (authfilename, F_OK) == 0);
    if (exists && access (authfilename, W_OK) != 0) {
	fprintf (stderr,
	 "%s:  %s not writable, changes will be ignored\n",
		 ProgramName, authfilename);
	xauth_allowed = False;
    }

    if (break_locks && verbose) {
	printf ("Attempting to break locks on authority file %s\n",
		authfilename);
    }

    if (ignore_locks) {
	if (break_locks) XauUnlockAuth (authfilename);
    } else {
	n = XauLockAuth (authfilename, XAUTH_DEFAULT_RETRIES,
			 XAUTH_DEFAULT_TIMEOUT, 
			 (break_locks ? 0L : XAUTH_DEFAULT_DEADTIME));
	if (n != LOCK_SUCCESS) {
	    char *reason = "unknown error";
	    switch (n) {
	      case LOCK_ERROR:
		reason = "error";
		break;
	      case LOCK_TIMEOUT:
		reason = "timeout";
		break;
	    }
	    fprintf (stderr, "%s:  %s in locking authority file %s\n",
		     ProgramName, reason, authfilename);
	    return -1;
	}
    }

    original_umask = umask (0077);	/* disallow non-owner access */

    authfp = fopen (authfilename, "r");
    if (!authfp) {
	int olderrno = errno;

					/* if file there then error */
	if (access (authfilename, F_OK) == 0) {	 /* then file does exist! */
	    errno = olderrno;
	    return -1;
	}				/* else ignore it */
	fprintf (stderr, 
		 "%s:  creating new authority file %s\n",
		 ProgramName, authfilename);
    } else {
	xauth_existed = True;
	n = read_auth_entries (authfp, False, &head, &tail);
	(void) fclose (authfp);
	if (n < 0) {
	    fprintf (stderr,
		     "%s:  unable to read auth entries from file \"%s\"\n",
		     ProgramName, authfilename);
	    return -1;
	}
	xauth_head = head;
    }

    n = strlen (authfilename);
    xauth_filename = malloc (n + 1);
    if (xauth_filename) strcpy (xauth_filename, authfilename);
    xauth_modified = False;

    if (verbose) {
	printf ("%s authority file %s\n", 
		ignore_locks ? "Ignoring locks on" : "Using", authfilename);
    }
    return 0;
}

static int write_auth_file (tmpnam)
    char *tmpnam;
{
    FILE *fp;
    AuthList *list;

    /*
     * xdm and auth spec assumes auth file is 12 or fewer characters
     */
    strcpy (tmpnam, xauth_filename);
    strcat (tmpnam, "-n");		/* for new */
    (void) unlink (tmpnam);
    fp = fopen (tmpnam, "w");		/* umask is still set to 0077 */
    if (!fp) {
	fprintf (stderr, "%s:  unable to open tmp file \"%s\"\n",
		 ProgramName, tmpnam);
	return -1;
    } 

    for (list = xauth_head; list; list = list->next) {
	XauWriteAuth (fp, list->auth);
    }

    (void) fclose (fp);
    return 0;
}

int auth_finalize ()
{
    char tmpnam[1024];			/* large filename size */

    if (xauth_modified) {
	if (dieing) {
	    if (verbose) {
		printf ("Aborting changes to authority file %s\n",
			xauth_filename);
	    }
	} else if (!xauth_allowed) {
	    fprintf (stderr, 
		     "%s:  %s not writable, changes ignored\n",
		     ProgramName, xauth_filename);
	} else {
	    if (verbose) {
		printf ("%s authority file %s\n", 
			ignore_locks ? "Ignoring locks and writing" :
			"Writing", xauth_filename);
	    }
	    tmpnam[0] = '\0';
	    if (write_auth_file (tmpnam) == -1) {
		fprintf (stderr,
			 "%s:  unable to write authority file %s\n",
			 ProgramName, tmpnam);
	    } else {
		(void) unlink (xauth_filename);
		if (link (tmpnam, xauth_filename) == -1) {
		    fprintf (stderr,
		     "%s:  unable to link authority file %s, use %s\n",
			     ProgramName, xauth_filename, tmpnam);
		} else {
		    (void) unlink (tmpnam);
		}
	    }
	}
    }

    if (!ignore_locks) {
	XauUnlockAuth (xauth_filename);
    }
    (void) umask (original_umask);
    return 0;
}

int process_command (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    CommandTable *ct;
    int n, status;

    if (argc < 1 || !argv || !argv[0]) return 1;

    if (dispatch_command (inputfilename, lineno, argc, argv,
			  command_table, &status))
      return status;

    prefix (inputfilename, lineno);
    fprintf (stderr, "unknown command \"%s\"\n", argv[0]);
    return 1;
}


/*
 * utility routines
 */

static void fprintfhex (fp, len, cp)
    register FILE *fp;
    int len;
    char *cp;
{
    unsigned char *ucp = (unsigned char *) cp;

    for (; len > 0; len--, ucp++) {
	register char *s = hex_table[*ucp];
	putc (s[0], fp);
	putc (s[1], fp);
    }
    return;
}

dump_numeric (fp, auth)
    register FILE *fp;
    register Xauth *auth;
{
    fprintf (fp, "%04x", auth->family);  /* unsigned short */
    fprintf (fp, " %04x ", auth->address_length);  /* short */
    fprintfhex (fp, auth->address_length, auth->address);
    fprintf (fp, " %04x ", auth->number_length);  /* short */
    fprintfhex (fp, auth->number_length, auth->number);
    fprintf (fp, " %04x ", auth->name_length);  /* short */
    fprintfhex (fp, auth->name_length, auth->name);
    fprintf (fp, " %04x ", auth->data_length);  /* short */
    fprintfhex (fp, auth->data_length, auth->data);
    putc ('\n', fp);
    return;
}

#if defined(SYSV) || defined(SVR4)
static void allign_column(fp, len, total, pat_ch)
FILE *fp;
int len, total;
char pat_ch;
{
	register i;
	for(i=len; i<total; i++)
		putc (pat_ch, fp);
}
#endif

static int dump_entry (inputfilename, lineno, auth, data)
    char *inputfilename;
    int lineno;
    Xauth *auth;
    char *data;
{
    struct _list_data *ld = (struct _list_data *) data;
    FILE *fp = ld->fp;

    if (ld->numeric) {
	dump_numeric (fp, auth);
    } else {
#if !defined(SYSV) && !defined(SVR4)
	char *dpyname = NULL;
	char numbuf[10];

	switch (auth->family) {
	  case FamilyLocal:
	    fwrite (auth->address, sizeof (char), auth->address_length, fp);
	    fprintf (fp, "/unix");
	    break;
	  case FamilyInternet:
	  case FamilyDECnet:
	    dpyname = get_hostname (auth);
	    if (dpyname) {
		fprintf (fp, "%s", dpyname);
		break;
	    }
	    /* else fall through to default */
	  default:
	    fprintf (fp, "#%04x#", auth->family);
	    fprintfhex (fp, auth->address_length, auth->address);
	    putc ('#', fp);
	}
#else
	fwrite (auth->address, sizeof (char), auth->address_length, fp);
#endif
	putc (':', fp);
	fwrite (auth->number, sizeof (char), auth->number_length, fp);
#if defined(SYSV) || defined(SVR4)
	allign_column(fp, auth->number_length+auth->address_length+1, 25, ' ');
#else
	putc (' ', fp);
	putc (' ', fp);
#endif
	fwrite (auth->name, sizeof (char), auth->name_length, fp);
#if defined(SYSV) || defined(SVR4)
	allign_column(fp, auth->name_length, 32, ' ');
#else
	putc (' ', fp);
	putc (' ', fp);
#endif
	fprintfhex (fp, auth->data_length, auth->data);
	putc ('\n', fp);
    }
    return 0;
}

static int extract_entry (inputfilename, lineno, auth, data)
    char *inputfilename;
    int lineno;
    Xauth *auth;
    char *data;
{
    struct _extract_data *ed = (struct _extract_data *) data;

    if (!ed->fp) {
	ed->fp = open_file (&ed->filename, "w", &ed->used_stdout,
			    inputfilename, lineno, ed->cmd);
	if (!ed->fp) {
	    prefix (inputfilename, lineno);
	    fprintf (stderr,
		     "unable to open extraction file \"%s\"\n",
		     ed->filename);
	    return -1;
	}
    }
    (*(ed->numeric ? dump_numeric : XauWriteAuth)) (ed->fp, auth);
    ed->nwritten++;

    return 0;
}


static int match_auth (a, b)
    register Xauth *a, *b;
{
    return ((a->family == b->family &&
	     a->address_length == b->address_length &&
	     a->number_length == b->number_length &&
	     bcmp (a->address, b->address, a->address_length) == 0 &&
	     bcmp (a->number, b->number, a->number_length) == 0) ? 1 : 0);
}


static int merge_entries (firstp, second, nnewp, nreplp)
    AuthList **firstp, *second;
    int *nnewp, *nreplp;
{
    AuthList *a, *b, *first, *tail;
    int n = 0, nnew = 0, nrepl = 0;

    if (!second) return 0;

    if (!*firstp) {			/* if nothing to merge into */
	*firstp = second;
	for (tail = *firstp, n = 1; tail->next; n++, tail = tail->next) ;
	*nnewp = n;
	*nreplp = 0;
	return n;
    }

    first = *firstp;
    /*
     * find end of first list and stick second list on it
     */
    for (tail = first; tail->next; tail = tail->next) ;
    tail->next = second;

    /*
     * run down list freeing duplicate entries; if an entry is okay, then
     * bump the tail up to include it, otherwise, cut the entry out of
     * the chain.
     */
    for (b = second; b; ) {
	AuthList *next = b->next;	/* in case we free it */

	a = first;
	while (1) {
	    if (match_auth (a->auth, b->auth)) {  /* found a duplicate */
		AuthList tmp;		/* swap it in for old one */
		tmp = *a;
		*a = *b;
		*b = tmp;
		a->next = b->next;
		XauDisposeAuth (b->auth);
		free ((char *) b);
		b = NULL;
		tail->next = next;
		nrepl++;
		nnew--;
		break;
	    }
	    if (a == tail) break;	/* if have looked at left side */
	    a = a->next;
	}
	if (b) {			/* if we didn't remove it */
	    tail = b;			/* bump end of first list */
	}
	b = next;
	n++;
	nnew++;
    }

    *nnewp = nnew;
    *nreplp = nrepl;
    return n;

}


static int iterdpy (inputfilename, lineno, start,
		    argc, argv, yfunc, nfunc, data)
    char *inputfilename;
    int lineno;
    int start;
    int argc;
    char *argv[];
    int (*yfunc)(), (*nfunc)();
    char *data;
{
    int i;
    int status;
    int errors = 0;
    Xauth proto;
    AuthList *l;

    /*
     * iterate
     */
    for (i = start; i < argc; i++) {
	char *displayname = argv[i];
	proto.address = proto.number = NULL;
	if (!get_displayname_auth (displayname, &proto)) {
	    prefix (inputfilename, lineno);
	    baddisplayname (displayname, argv[0]);
	    errors++;
	    continue;
	}
	status = 0;
	for (l = xauth_head; l; l = l->next) {
	    if (match_auth (&proto, l->auth)) {
		if (yfunc) {
		    status = (*yfunc) (inputfilename, lineno,
				       l->auth, data);
		    if (status < 0) break;
		}
	    } else {
		if (nfunc) {
		    status = (*nfunc) (inputfilename, lineno,
				       l->auth, data);
		    if (status < 0) break;
		}
	    }
	}
	if (proto.address) free (proto.address);
	if (proto.number) free (proto.number);
	if (status < 0) {
	    errors -= status;		/* since status is negative */
	    break;
	}
    }

    return errors;
}

static int remove_entry (inputfilename, lineno, auth, data)
    char *inputfilename;
    int lineno;
    Xauth *auth;
    char *data;
{
    int *nremovedp = (int *) data;
    AuthList **listp = &xauth_head;
    AuthList *prev = NULL, *list = (*listp);
    int removed = 0, notremoved = 0;

    if (!list) {
	*nremovedp = 0;
	return 1;			/* if nothing to remove */
    }

    /*
     * run through list removing any records that match
     */
    while (list) {
	if (match_auth (list->auth, auth)) {
	    AuthList *next = list->next;	      /* next one to look at */
	    if (prev) {
		prev->next = next;		       /* unlink current one */
	    } else {
		*listp = next;			       /* bump start of list */
	    }
	    XauDisposeAuth (list->auth);                    /* free the auth */
	    free (list);				    /* free the link */
	    list = next;			  /* go look at the next one */
	    removed++;
	    xauth_modified = True;
	} else {
	    notremoved++;
	    prev = list;
	    list = list->next;
	}
    }

    if (notremoved == 0) {		/* if nothing left */
	*listp = NULL;			/* then null out list */
    }
    *nremovedp = removed;
    return 0;
}

/*
 * action routines
 */

/*
 * help
 */
int print_help (fp, cmd, prefix)
    FILE *fp;
    char *cmd;
    char *prefix;
{
    CommandTable *ct;
    int n = 0;

    if (!prefix) prefix = "";

    if (!cmd) {				/* if no cmd, print all help */
	for (ct = command_table; ct->name; ct++) {
	    fprintf (fp, "%s%s\n", prefix, ct->helptext);
	    n++;
	}
    } else {
	int len = strlen (cmd);
	for (ct = command_table; ct->name; ct++) {
	    if (strncmp (cmd, ct->name, len) == 0) {
		fprintf (fp, "%s%s\n", prefix, ct->helptext);
		n++;
	    }
	}
    }
	
    return n;
}

static int do_help (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    char *cmd = (argc > 1 ? argv[1] : NULL);
    int n;

    n = print_help (stdout, cmd, "    ");  /* a nice amount */

    if (n < 0 || (n == 0 && !cmd)) {
	prefix (inputfilename, lineno);
	fprintf (stderr, "internal error with help");
	if (cmd) {
	    fprintf (stderr, " on command \"%s\"", cmd);
	}
	fprintf (stderr, "\n");
	return 1;
    }

    if (n == 0) {
	prefix (inputfilename, lineno);
	/* already know that cmd is set in this case */
	fprintf (stderr, "no help for noexistent command \"%s\"\n", cmd);
    }

    return 0;
}

/*
 * questionmark
 */
static int do_questionmark (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    CommandTable *ct;
    int i;
#define WIDEST_COLUMN 72
    int col = WIDEST_COLUMN;

    printf ("Commands:\n");
    for (ct = command_table; ct->name; ct++) {
	if ((col + ct->maxlen) > WIDEST_COLUMN) {
	    if (ct != command_table) {
		putc ('\n', stdout);
	    }
	    fputs ("        ", stdout);
	    col = 8;			/* length of string above */
	}
	fputs (ct->name, stdout);
	col += ct->maxlen;
	for (i = ct->maxlen; i < COMMAND_NAMES_PADDED_WIDTH; i++) {
	    putc (' ', stdout);
	    col++;
	}
    }
    if (col != 0) {
	putc ('\n', stdout);
    }

    /* allow bad lines since this is help */
    return 0;
}

/*
 * list [displayname ...]
 */
static int do_list (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    struct _list_data ld;

    ld.fp = stdout;
    ld.numeric = (argv[0][0] == 'n');

    if (argc == 1) {
	register AuthList *l;

	if (xauth_head) {
#if defined(SYSV) || defined(SVR4)
	    char *d = "DISPLAY NAME";
	    char *p = "PROTOCOL NAME";
	    char *k = "DISPLAY KEY";

	    fprintf(stdout, d); 
	    allign_column(stdout, strlen(d), 25, ' ');

	    fprintf(stdout, p); 
	    allign_column(stdout, strlen(p), 32, ' ');

	    fprintf(stdout, "%s\n", k); 
	    allign_column(stdout, 0, strlen(d), '=');
	    allign_column(stdout, strlen(d), 25, ' ');
	    allign_column(stdout, 0, strlen(p), '=');
	    allign_column(stdout, strlen(p), 32, ' ');
	    allign_column(stdout, 0, strlen(k), '=');
	    fprintf(stdout, "\n\n");
#endif
	    for (l = xauth_head; l; l = l->next) {
		dump_entry (inputfilename, lineno, l->auth, (char *) &ld);
	    }
#if defined(SYSV) || defined(SVR4)
	    fprintf(stdout, "\n");
#endif
	}
	return 0;
    }

    return iterdpy (inputfilename, lineno, 1, argc, argv,
		    dump_entry, NULL, (char *) &ld);
}

/*
 * merge filename [filename ...]
 */
static int do_merge (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    int i;
    int errors = 0;
    AuthList *head, *tail, *listhead, *listtail;
    int nentries, nnew, nrepl;
    Bool numeric = False;

    if (argc < 2) {
	prefix (inputfilename, lineno);
	badcommandline (argv[0]);
	return 1;
    }

    if (argv[0][0] == 'n') numeric = True;
    listhead = listtail = NULL;

    for (i = 1; i < argc; i++) {
	char *filename = argv[i];
	FILE *fp;
	Bool used_stdin = False;

	fp = open_file (&filename, "r", &used_stdin, inputfilename, lineno,
			argv[0]);
	if (!fp) {
	    errors++;
	    continue;
	}

	head = tail = NULL;
	nentries = read_auth_entries (fp, numeric, &head, &tail);
	if (nentries == 0) {
	    prefix (inputfilename, lineno);
	    fprintf (stderr, "unable to read any entries from file \"%s\"\n",
		     filename);
	    errors++;
	} else {			/* link it in */
	    add_to_list (listhead, listtail, head);
 	}

	if (!used_stdin) (void) fclose (fp);
    }

    /*
     * if we have new entries, merge them in (freeing any duplicates)
     */
    if (listhead) {
	nentries = merge_entries (&xauth_head, listhead, &nnew, &nrepl);
	if (verbose) 
	  printf ("%d entries read in:  %d new, %d replacement%s\n", 
	  	  nentries, nnew, nrepl, nrepl != 1 ? "s" : "");
	if (nentries > 0) xauth_modified = True;
    }

    return 0;
}

/*
 * extract filename displayname [displayname ...]
 */
static int do_extract (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    int errors;
    struct _extract_data ed;

    if (argc < 3) {
	prefix (inputfilename, lineno);
	badcommandline (argv[0]);
	return 1;
    }

    ed.fp = NULL;
    ed.filename = argv[1];
    ed.numeric = (argv[0][0] == 'n');
    ed.nwritten = 0;
    ed.cmd = argv[0];

    errors = iterdpy (inputfilename, lineno, 2, argc, argv, 
		      extract_entry, NULL, (char *) &ed);

    if (!ed.fp) {
	fprintf (stderr, 
		 "No matches found, authority file \"%s\" not written\n",
		 ed.filename);
    } else {
	if (verbose) {
	    printf ("%d entries written to \"%s\"\n", 
		    ed.nwritten, ed.filename);
	}
	if (!ed.used_stdout) {
	    (void) fclose (ed.fp);
	}
    }

    return errors;
}


/*
 * add displayname protocolname hexkey
 */
static int do_add (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{ 
    int n, nnew, nrepl;
    int len;
    char *dpyname;
    char *protoname;
    char *hexkey;
    char *key;
    Xauth *auth;
    AuthList *list;

    if (argc != 4 || !argv[1] || !argv[2] || !argv[3]) {
	prefix (inputfilename, lineno);
	badcommandline (argv[0]);
	return 1;
    }

    dpyname = argv[1];
    protoname = argv[2];
    hexkey = argv[3];

    len = cvthexkey (hexkey, &key);
    if (len < 0) {
	prefix (inputfilename, lineno);
	fprintf (stderr,
		 "key contains odd number of or non-hex characters\n");
	return 1;
    }

    auth = (Xauth *) malloc (sizeof (Xauth));
    if (!auth) {
	prefix (inputfilename, lineno);
	fprintf (stderr, "unable to allocate %d bytes for Xauth structure\n",
		 sizeof (Xauth));
	free (key);
	return 1;
    }

    if (!get_displayname_auth (dpyname, auth)) {
	prefix (inputfilename, lineno);
	baddisplayname (dpyname, argv[0]);
	free (auth);
	free (key);
	return 1;
    }

    /*
     * allow an abbreviation for common protocol names
     */
    if (strcmp (protoname, DEFAULT_PROTOCOL_ABBREV) == 0) {
	protoname = DEFAULT_PROTOCOL;
    }
    else if (strcmp (protoname, ATT_PROTOCOL_ABBREV) == 0) {
	protoname = ATT_PROTOCOL;
    }

    auth->name_length = strlen (protoname);
    auth->name = copystring (protoname, auth->name_length);
    if (!auth->name) {
	prefix (inputfilename, lineno);
	fprintf (stderr, "unable to allocate %d character protocol name\n",
		 auth->name_length);
	free (auth);
	free (key);
	return 1;
    }
    auth->data_length = len;
    auth->data = key;

    list = (AuthList *) malloc (sizeof (AuthList));
    if (!list) {
	prefix (inputfilename, lineno);
	fprintf (stderr, "unable to allocate %d bytes for auth list\n",
		 sizeof (AuthList));
	free (auth);
	free (key);
	free (auth->name);
	return 1;
    }

    list->next = NULL;
    list->auth = auth;

    /*
     * merge it in; note that merge will deal with allocation
     */
    n = merge_entries (&xauth_head, list, &nnew, &nrepl);
    if (n <= 0) {
	prefix (inputfilename, lineno);
	fprintf (stderr, "unable to merge in added record\n");
	return 1;
    }

    xauth_modified = True;
    return 0;
}

/*
 * remove displayname
 */
static int do_remove (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    int nremoved = 0;
    int errors;

    if (argc < 2) {
	prefix (inputfilename, lineno);
	badcommandline (argv[0]);
	return 1;
    }

    errors = iterdpy (inputfilename, lineno, 1, argc, argv,
		      remove_entry, NULL, (char *) &nremoved);
    if (verbose) printf ("%d entries removed\n", nremoved);
    return errors;
}

/*
 * info
 */
static int do_info (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    int n;
    AuthList *l;

    if (argc != 1) {
	prefix (inputfilename, lineno);
	badcommandline (argv[0]);
	return 1;
    }

    for (l = xauth_head, n = 0; l; l = l->next, n++) ;

    printf ("Authority file:       %s\n",
	    xauth_filename ? xauth_filename : "(none)");
    printf ("File new:             %s\n", xauth_existed ? No : Yes);
    printf ("File locked:          %s\n", ignore_locks ? No : Yes);
    printf ("Number of entries:    %d\n", n);
    printf ("Changes honored:      %s\n", xauth_allowed ? Yes : No);
    printf ("Changes made:         %s\n", xauth_modified ? Yes : No);
    printf ("Current input:        %s:%d\n", inputfilename, lineno);
    return 0;
}


/*
 * exit
 */
static Bool alldone = False;

static int do_exit (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    /* allow bogus stuff */
    alldone = True;
    return 0;
}

/*
 * quit
 */
static int do_quit (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    /* allow bogus stuff */
    die (0);
    /* NOTREACHED */
}


/*
 * source filename
 */
static int do_source (inputfilename, lineno, argc, argv)
    char *inputfilename;
    int lineno;
    int argc;
    char **argv;
{
    char *script;
    char buf[BUFSIZ];
    FILE *fp;
    Bool used_stdin = False;
    int len;
    int errors = 0, status;
    int sublineno = 0;
    char **subargv;
    int subargc;
    Bool prompt = False;		/* only true if reading from tty */

    if (argc != 2 || !argv[1]) {
	prefix (inputfilename, lineno);
	badcommandline (argv[0]);
	return 1;
    }

    script = argv[1];

    fp = open_file (&script, "r", &used_stdin, inputfilename, lineno, argv[0]);
    if (!fp) {
	return 1;
    }

    if (verbose && used_stdin && isatty (fileno (fp))) prompt = True;

    while (!alldone) {
	buf[0] = '\0';
	if (prompt) {
	    printf ("xauth> ");
	    fflush (stdout);
	}
	if (fgets (buf, sizeof buf, fp) == NULL) break;
	sublineno++;
	len = strlen (buf);
	if (len == 0 || buf[0] == '#') continue;
	if (buf[len-1] != '\n') {
	    prefix (script, sublineno);
	    fprintf (stderr, "line too long\n");
	    errors++;
	    break;
	}
	buf[--len] = '\0';		/* remove new line */
	subargv = split_into_words (buf, &subargc);
	if (subargv) {
	    status = process_command (script, sublineno, subargc, subargv);
	    free ((char *) subargv);
	    errors += status;
	} else {
	    prefix (script, sublineno);
	    fprintf (stderr, "unable to break line into words\n");
	    errors++;
	}
    }

    if (!used_stdin) {
	(void) fclose (fp);
    }
    return errors;
}
