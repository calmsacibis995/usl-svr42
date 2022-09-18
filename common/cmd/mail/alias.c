/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/alias.c	1.12.2.7"
#ident "@(#)alias.c	1.26 'attmail mail(1) command'"
/*
    NAME
	mailalias - look up alias names

    SYNOPSIS
	mailalias [-s | -p] [-v] [-r] [-l] [-R] [-S suffix] [-P prefix] [-L prefix%Usuffix] name ...

    DESCRIPTION
	Look up the name in the alias files. First look in
	$HOME/lib/names, then in the files listed in
	/etc/mail/namefiles. For each name found at the
	beginning of a line, output the remainder of the line.

	For each file X listed in /etc/mail/namefiles, the sorted
	file X.s will be binary searched for the alias. If X.s
	does not exist, it will be created.

	If -s is not specified and multiple names are requested,
	each line is prefixed with the name being looked up. If
	-p is specified, the prefix is always printed.

	If -v is specified, debugging output is given.

	If -r is specified, names will be recursively retranslated.

	If -l is specified, the sorted files will not be used.

	If -R is specified, the sorted files will not be recreated.

	If -P is specified, the given prefix is stripped off the name
	before being printed. Up to 50 prefixes may be specified.

	If -S is specified, the given suffix is stripped off the name
	before being printed. Up to 50 suffixes may be specified.

	If -L is specified, names are looked up in /etc/passwd. If
	they are local user names, then the argument prefix%Usuffix
	is output, with %U replaced with the user name.
*/
#include <ctype.h>
#include <stdio.h>
#include "libmail.h"
#include <dirent.h>

#define UNCHECKED -1
#define NOTTHERE  -2
#define HASHSIZE 263

#define lower(c)	( isupper(c) ? _tolower(c) : c )

typedef struct _deque deque;	/* double-ended queue */
struct _deque {
	deque *next;
	deque *prev;
	deque *curr;
	char *key;
	char *object;
};

typedef struct _hdeque hdeque;	/* hashed set of double-ended queues */
struct _hdeque {
	int curr;
	deque arr[HASHSIZE];
};

typedef struct alias_list alias_list;
struct alias_list {
	string *key;
	string *alias;
	int translated;
};

typedef struct _dbfiles dbfiles;
struct _dbfiles {
	string *fname;
	string *sname;
	int libfile;
	int (*search)();
	struct stat *st;
	string *key;
	hdeque *incore;
};

typedef struct _tree_node tree_node;
struct _tree_node {
	tree_node *par;
	string *str;
};

#define compare(s1, p2)	cascmp(s_to_c(s1), p2)

/* predeclared */
static void add_dir ARGS((string *file));
static void add_file ARGS((string *file, struct stat *st, int libfile, char *subfile_key));
static int b_lookup ARGS((char *name, dbfiles *file, string *alias));
static void cfclose ARGS((FILE *fp));
static FILE *cfopen ARGS((char *path, char *mode));
static void clear_deque  ARGS((deque *alist));
static void clear_hdeque ARGS((hdeque *alistv));
static int d_lookup ARGS((char *name, dbfiles *file, string *alias));
static char *find_another ARGS((hdeque *alistv, char *key, char *clue));
static char *first ARGS((deque *alist));
/* static char *get ARGS((void)); /* not used here */
static void getdbfiles ARGS((void));
static int hashfunc ARGS((char *key));
static int i_lookup ARGS((char *name, dbfiles *file, string *alias));
static int is_path ARGS((tree_node *parent, tree_node *child));
static int l_lookup ARGS((char *name, dbfiles *file, string *alias));
static int lookup ARGS((char *name, dbfiles *file, string *alias));
static deque *new_deque ARGS((void));
static hdeque *new_hdeque ARGS((void));
static char *next ARGS((deque *alist));
static char *pop ARGS((deque *alist));
/* static char *prev ARGS((deque *alist)); /* not used here */
static void print_list ARGS((string *parent, hdeque *alistv));
static void print_local ARGS((const char *name));
static void print_name ARGS((const char *name));
static void push  ARGS((deque *alist, char *aentry));
static void put ARGS((deque *alist, char *aentry));
static int readlong ARGS((unsigned long *pl, FILE *infp));
static int recreate ARGS((string *file, string *sfile));
static char *search ARGS((hdeque *alistv, char *name, char *atarg));
static void store ARGS((hdeque *alistv, char *ckey, char *object));
static void translate_list ARGS((int argc, char **argv));
static void translate_name ARGS((char *name, string *alias));
static char *xmalloc ARGS((int n));

#define NEW(X)	((X *)xmalloc(sizeof(X)))

static void usage  ARGS((void));

typedef struct prefixsuffix {
	char *name;
	int length;
} prefixsuffix;

const char *progname = "";		/* argv[0] */
static int verbose;		/* -v option */
static int simple;		/* -s option */
static int useprefix;		/* -p option */
static int recurse;		/* -r option */
static int linear;		/* -l option */
static int frecreate;		/* -R option */
static int dumpdirs;		/* -d option */
static int printlocal;		/* -L option */
static char *Lprefix, *Lsuffix;	/* used by -L option */
static int stripprefixes;	/* -P option */
#define MAX_PREFIXES 50
static prefixsuffix Pprefix[MAX_PREFIXES];
static int stripsuffixes;	/* -S option */
#define MAX_SUFFIXES 50
static prefixsuffix Ssuffix[MAX_SUFFIXES];
static int wdepth;		/* depth of working recursion */
static deque *aliases;		/* list of dbfiles */

static void usage()
{
    (void) pfmt(stdout, MM_ERROR, ":120:Incorrect usage\n");
    (void) pfmt(stdout, MM_ACTION, ":516:Usage: %s [-s] [-p] [-r] [-l] [-R] [-d] [-v] [-P prefix] [-suffix S] [-L prefix/suffix] name ...\n", progname);
    (void) pfmt(stdout, MM_ACTION, ":415:\t-s\tdo not print alias name on each line\n");
    (void) pfmt(stdout, MM_ACTION, ":416:\t-p\talways print alias name on each line\n");
    (void) pfmt(stdout, MM_ACTION, ":417:\t-r\trecursively translate aliases\n");
    (void) pfmt(stdout, MM_ACTION, ":418:\t-l\talways use linear search for aliases\n");
    (void) pfmt(stdout, MM_ACTION, ":419:\t-R\tdo not recreate sorted alias files\n");
    (void) pfmt(stdout, MM_ACTION, ":359:\t-d\toutput complete alias files from alias directories\n");
    (void) pfmt(stdout, MM_ACTION, ":420:\t-v\tprint debugging output\n");
    (void) pfmt(stdout, MM_ACTION, ":518:\t-P\tprefix to be removed\n");
    (void) pfmt(stdout, MM_ACTION, ":519:\t-S\tsuffix to be removed\n");
    (void) pfmt(stdout, MM_ACTION, ":493:\t-L\tprefix/suffix to print around local user names (%U replaced with user name)\n");
    exit(1);
    /* NOTREACHED */
}

/* resolve mail aliases */
main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	progname = argv[0];
	(void) setlocale(LC_ALL, "");
	(void) setcat("uxemail");
	(void) setlabel("UX:mailalias");

	while ((i = getopt(argc, argv, "dlL:pP:rRsS:v?")) != -1)
		switch (i) {
		case 'd': dumpdirs++; break;
		case 'l': linear++; break;
		case 'L': printlocal++;
			Lprefix = optarg;
			Lsuffix = strstr(optarg, "%U");
			if (Lsuffix) {
				*Lsuffix = '\0';
				Lsuffix += 2;
			} else
				Lsuffix = "";
			break;
		case 'p': useprefix++; break;
		case 'P':
			if (stripprefixes >= MAX_PREFIXES) {
				(void) pfmt(stdout, MM_ERROR, ":514:Only %d prefixes permitted\n", MAX_PREFIXES);
				exit(1);
			}
			Pprefix[stripprefixes].name = optarg;
			Pprefix[stripprefixes].length = strlen(optarg);
			stripprefixes++;
			break;
		case 'r': recurse++; break;
		case 'R': frecreate++; break;
		case 's': simple++; break;
		case 'S':
			if (stripsuffixes >= MAX_SUFFIXES) {
				(void) pfmt(stdout, MM_ERROR, ":515:Only %d suffixes permitted\n", MAX_SUFFIXES);
				exit(1);
			}
			Ssuffix[stripsuffixes].name = optarg;
			Ssuffix[stripsuffixes].length = strlen(optarg);
			stripsuffixes++;
			break;
		case 'v': verbose++; break;
		case '?': usage();
		}

	/* get environmental info */
	getdbfiles();

	/* loop through the names to be translated (from standard input) */
	if (optind == (argc-1))
		simple++;

	if (recurse) {
		translate_list(argc-optind, argv+optind);
	} else {
		string *alias = s_new();
		for (i = optind; i < argc; i++) {
			s_reset(alias);
			translate_name(argv[i], alias);
			if (!simple || useprefix)
				printf("%s\t", argv[i]);
			if (*s_to_c(alias) == '\0')
				print_local(argv[i]);
			else {
				print_name(s_to_c(alias));
			}
			fflush(stdout);
		}
	}
	return 0;
}

/* add the named file to the list of dbfiles */

static void add_file(file, st, libfile, subfile_key)
string *file;
struct stat *st;
int libfile;
char *subfile_key;
{
	dbfiles *n = (dbfiles *)xmalloc(sizeof(dbfiles));

	if (verbose > 1)
		printf("add_file('%s', %d)\n", s_to_c(file), libfile);

	n->st = (struct stat *)xmalloc(sizeof(*n->st));

	n->fname   = s_dup(file);
	n->sname   = NULL;
	n->libfile = libfile;
	*n->st     = *st;
	n->key     = subfile_key ? s_copy(subfile_key) : NULL;
	n->incore  = NULL;

	if (dumpdirs && (libfile == 2)) {
		n->search = d_lookup;
	} else {
		n->search = NULL;
	}

	put((deque*)aliases, (char*)n);
}

/* get the list of dbfiles to search */
/* dfiles file names are taken from ~/lib/names and libdir/sysalias
/* if one of the files named in libdir/sysalias is a directory,
/* then include all of the files in that directory as secondary alias files
/* which will be searched only if their basename matches the alias "name"
/* that is being resolved.
/* */

static void getdbfiles()
{
	char *home = getenv("HOME");
	FILE *fp;
	struct stat st[1];
	string *file = s_new();

	if (verbose)
		printf("getdbfiles()\n");

	aliases = new_deque();

	if (home != NULL) {
		file = s_xappend(file, home, useralias, (char*)0);
		if ((stat(s_to_c(file), st) == 0)
		&& ((st->st_mode & S_IFMT) == S_IFREG))
			add_file(file, st, 0, NULL);
	}
	s_reset(file);

	/* system wide aliases */
	if (chdir(libdir) < 0) {
		pfmt(stderr, MM_ERROR, ":9:Cannot change directory to \"%s\": %s\n",
			libdir, strerror(errno));
		return;
	}

	if ((fp = fopen(sysalias, "r")) != NULL) {
		for (s_reset(file); s_gettoken(fp, file) != NULL; s_reset(file)){
			if (stat(s_to_c(file), st) == 0) {
				switch(st->st_mode & S_IFMT) {
				case S_IFREG:
					add_file(file, st, 1, NULL);
					break;
				case S_IFDIR:
					add_dir(file);
					break;
				default:
					break;
				}
			}
		}
		fclose(fp);
	}
}

/* add files in named directory to list of dbfiles as secondary alias files */

static void add_dir(file)
string *file;
{
	DIR *dirp;
	struct dirent *sub;
	struct stat st[1];
	char buf[BUFSIZ];
	int len;

	if (dirp = opendir(s_to_c(file))) {
		while (sub = readdir(dirp)) {
			len = strlen(sub->d_name);

			if ((len > 2)
			&& (strcmp(sub->d_name + len - 2, ".s") == 0))
				continue;

			sprintf(buf, "%s/%s", s_to_c(file), sub->d_name);

			if (stat(buf, st) < 0)
				continue;

			if ((st->st_mode & S_IFMT) == S_IFREG)
				add_file(s_copy(buf), st, 2, sub->d_name);
		}
		closedir(dirp);
	}
}

/* loop through the translation files until a lookup() succeeds
/* or all files have been examined
/* */

static void translate_name(name, alias)
	char *name;		/* name to translate */
	string *alias;		/* where to put the alias */
{
	string *file = s_new();
	dbfiles *pfiles;

	if (verbose)
		printf("translate_name(%s)\n", name);

	/* look at the names */
	s_restart(file);
	for (pfiles = (dbfiles *)first(aliases); pfiles; pfiles = (dbfiles *)next(aliases)) {
		if (lookup(name, pfiles, alias) == 0) {
			s_free(file);
			return;
		}
	}
	return;
}

/*  Loop through the entries in a translation file looking for a match.
 *  Return 0 if found, -1 otherwise.
 */
static int lookup(name, f, alias)
	char *name;
	dbfiles *f;
	string *alias;	/* returned string */
{

	if (f->key && strcmp(name, s_to_c(f->key))) return (-1);

	if (verbose > 1)
		printf("lookup(%s, %s)\n", name, s_to_c(f->fname));

	/* if we've never searched this dbfile, determine search method */
	if (f->search == NULL) {
		f->search = l_lookup;
		if (f->st->st_size < 2*BUFSIZ) {
			f->search = i_lookup;
		} else if (!linear && f->libfile ) {
			f->sname = s_copy(s_to_c(f->fname));
			s_append(f->sname, ".s");

			if ((newer(s_to_c(f->sname), s_to_c(f->fname)))
			|| (!frecreate && recreate(f->fname, f->sname))) {
				f->search = b_lookup;
			}
		}
	}

	return ((*f->search)(name, f, alias));
}

/* Check to see if two names point to the same file */
static int samefile(f1, f2)
char *f1;
char *f2;
{
	struct stat s1, s2;
	if (stat(f1, &s1) == -1)
		return 0;
	if (stat(f2, &s2) == -1)
		return 0;
	return ((s1.st_ino == s2.st_ino) &&
		(s1.st_dev == s2.st_dev));
}

/* Check if we're being run with gid==mail */
static int runbymail()
{
	static const char pn[] = "runbymail";
	static int ret = UNCHECKED;
	if (ret == UNCHECKED) {
		uid_t egid = getegid();
		struct group *gr = getgrnam("mail");
		if (verbose > 6)
			printf("%s: gr->gr_gid=%ld, egid=%ld\n", pn,
				(long)(gr ? gr->gr_gid : -1), (long)egid);
		ret = gr ? (gr->gr_gid == egid) : 0;
		endgrent();
	}
	return ret;
}

/* Recreate the sorted version of an alias file */
static int recreate(file, sfile)
string *file;
string *sfile;
{
	static const char pn[] = "recreate";
	char *filename = s_to_c(file);
	char *sfilename = s_to_c(sfile);
	char *slash = strrchr(filename, '/');
	char *base = slash ? slash++ : filename;
	char dir[FILENAME_MAX];
	string *tfile;
	char *tfilename;
	int ret, lret;

	if (verbose > 2)
		printf("%s(%s, %s)\n", pn, filename, sfilename);

	/* extract the directory name */
	if (slash) {
		int len = slash - filename;
		strncpy(dir, filename, len);
		dir[len] = '\0';
	} else {
		strcpy(dir, ".");
	}

	/* make sure X.s doesn't get truncated to X */
	if (samefile(filename, sfilename)) {
		if (verbose > 5)
			printf("%s: %s and %s are the same file\n", pn, filename, sfilename);
		return 0;
	}

	/* Only rewrite system files if run by mail */
	if (!runbymail()) {
		if (verbose > 5)
			printf("%s: must be run by mail to sort the files\n", pn);
		return 0;
	}

	/* Make certain that files which get created are group writable. */
	(void) umask(02);

	/* Lock the file to make certain that no one else touches it. */
	if ((lret = maildlock(base, 1, dir, 0)) != L_SUCCESS) {
		if (verbose > 5)
			printf("%s: maildlock() returned %d\n", pn, lret);
		return 0;
	}

	tfile = s_dup(file);
	tfile = s_append(tfile, ".t");
	tfilename = s_to_c(tfile);

	/* Check if the window was caught before we locked the file. */
	if ((lret = newer(sfilename, filename)) != 0) {
		if (verbose > 5)
			printf("%s: newer() returned %d\n", pn, lret);
		ret = 1;
	}

	/* Create the new sorted alias file. */
	else if ((lret = sortafile(filename, tfilename)) != 0) {
		if (verbose > 5)
			printf("%s: sortafile() returned %d\n", pn, lret);
		if (((chown(tfilename, (uid_t)2, getegid()) == -1) &&
		     (posix_chown(tfilename) == -1)) ||
		    (rename(tfilename, sfilename) == -1))
			ret = 0;
		else
			ret = 1;
	}

	else {
		if (verbose > 5)
			printf("%s() failed\n", pn);
		ret = 0;
	}

	s_free(tfile);
	mailunlock();
	return ret;
}

/* Do a binary search lookup of the name in the given file */
/* if we see that there are lots of aliases to resolve,
/* switch to using i_lookup() for this file.
/* */
/* This routine is based on a similar routine in the PD program smail 2.
/* */

/*
** "Binary search routines are never written right the first time around."
** - Robert G. Sheldon.
**
** "...or even the third or fourth time."
** - Tony L. Hansen
*/
static int b_lookup(name, f, alias)
dbfiles *f;
char *name;
string *alias;	/* returned string */
{
	static const char pn[] = "b_lookup";
	FILE *fp;
	string *file = f->sname;
	int rv = -1;
	register int c, flag;
	register char *s;
	long lo, hi, start, end;

	if (verbose > 2)
		printf("%s(%s, %s)\n", pn, name, s_to_c(file));

	if (wdepth > 10) {
		if (verbose > 2) {
			printf("b_lookup() switching to i_lookup(): ",
				s_to_c(file));
			printf("working stack depth = %d\n", wdepth);
		}

		f->search = i_lookup;
		return ((*f->search)(name, f, alias));
	}

	if ((fp = cfopen(s_to_c(file), "r")) != NULL) {
		c = lower(*name);
		if ((fseek(fp, c*sizeof(long), 0) == EOF) ||
		    !readlong((unsigned long*)&start, fp) ||
		    !readlong((unsigned long*)&end, fp) ||
		    (start == end))
			goto ret;
		for (lo = start, hi = end;;) {
			/* find midpoint */
			long middle = ( hi+lo+1 )/2;
			register long hmiddle = middle;
			if (verbose > 5)
				printf("%s: lo=%ld, hi=%ld, middle=%ld\n", pn, lo, hi, middle);
			if (fseek(fp, middle, 0) == -1)
				goto ret;

			/* go to beginning of next line */
			if (hmiddle > start) {
				if (verbose > 5)
					printf("%s: looking for end of line\n", pn);
				while (((c = getc(fp)) != EOF) && (c != '\n'))
					hmiddle++;
				if (c == EOF) {		/* end of file */
					if (verbose > 5)
						printf("%s: EOF found\n", pn);
					goto ret;
				}
			}

			/* check 1st token on line for match */
			for (flag = 0, s = name; flag == 0; s++ ) {
				c = getc(fp);
				if (verbose > 6)
					if (c == EOF)
						printf("%s: EOF found\n", pn);
					else
						printf("%s, c='%c'\n", pn, c);
				if ( *s == '\0' ) {	/* end of token */
					if ((c == ' ') || (c == '\t') || (c == '\n')) {
						string *line = s_new();
						string *token = s_new();
						if (c == '\n')
							ungetc(c, fp);
						s_reset(alias);
						if (s_getline(fp, line)) {
							s_restart(line);
							while (s_parse(line, s_restart(token))) {
								/* avoid definition loops */
								alias = (compare(token, name)==0) ?
									s_append(alias, name) :
									s_append(alias, s_to_c(token));
								alias = s_append(alias, " ");
							}
						}
						if (verbose > 5)
							printf("%s: line='%s'\n", pn, s_to_c(alias));
						s_free(line);
						s_free(token);
						rv = 0;
						goto ret;
					}
				}
				if (c == EOF)		/* end of file */
					flag = 1;
				else
					flag = lower(c) - lower(*s);
			}

			/* failure? */
			if (lo >= middle)
				goto ret;

			/* close the window */
			if ((c != EOF) && (flag < 0)) {
				lo = (hmiddle > middle ? hmiddle-1 : middle);
			} else {
				hi = middle - 1;
			}
		}
	ret:
		cfclose(fp);
	}

	return rv;
}

/* read in a semi-portable long from the output file */
static int readlong(pl, infp)
unsigned long *pl;
FILE *infp;
{
    unsigned char s[sizeof(long)];
    register unsigned long l = 0;
    register int i;
    if (fread(s, sizeof(s), 1, infp) != 1)
	return 0;
    for (i = sizeof(long); --i >= 0; )
	l = (l << 8) + s[i];
    *pl = l;
    return 1;
}

/* do a linear lookup of the name in the given file
/* if we see that there are lots of aliases to resolve,
/* switch to using i_lookup() for this file.
/* */

static int l_lookup(name, f, alias)
char *name;
dbfiles *f;
string *alias;	/* returned string */
{
	FILE *fp;
	string *file = f->fname;
	int rv = -1;

	if (verbose > 2)
		printf("l_lookup(%s, %s)\n", name, s_to_c(file));

	if (wdepth > 10) {
		if (verbose > 2) {
			printf("l_lookup() switching to i_lookup(): ",
				s_to_c(file));
			printf("working stack depth = %d\n", wdepth);
		}

		f->search = i_lookup;
		return ((*f->search)(name, f, alias));
	}
	if ((fp = cfopen(s_to_c(file), "r")) != NULL) {
		/* look for a match */
		string *line = s_new();
		string *token = s_new();
		s_reset(alias);

		while (s_getline(fp, s_restart(line))!=NULL) {
			if (s_parse(s_restart(line), s_restart(token))==NULL)
				continue;

			if (compare(token, name)!=0)
				continue;

			/* match found, get the alias */
			while (s_parse(line, s_restart(token))!=NULL) {
				/* avoid definition loops */
				alias = (compare(token, name)==0) ?
					s_append(alias, name) :
					s_append(alias, s_to_c(token));
				alias = s_append(alias, " ");
			}
			rv = 0;
			break;
		}

		cfclose(fp);
		s_free(line);
		s_free(token);
	}

	return rv;
}

/* dump the aliases in the given file */

static int d_lookup(name, f, alias)
char *name;
dbfiles *f;
string *alias;	/* returned string */
{
	FILE *fp;
	string *file = f->fname;
	int rv = -1;

	if (verbose > 1)
		printf("d_lookup(%s, %s)\n", name, s_to_c(file));
	if ((fp = fopen(s_to_c(file), "r")) != NULL) {
		/* look for a match */
		string *line = s_new();
		string *token = s_new();
		s_reset(alias);

		while (s_getline(fp, s_restart(line))!=NULL) {
			if (s_parse(s_restart(line), s_restart(token))==NULL)
				continue;

			/* if 1st token is same as alias name, skip it */
			if (compare(token, name) != 0) {
				alias = s_append(alias, s_to_c(token));
				alias = s_append(alias, " ");
			}

			/* get the rest of the alias */
			while (s_parse(line, s_restart(token))!=NULL) {
				/* avoid definition loops */
				alias = (compare(token, name)==0) ?
					s_append(alias, name) :
					s_append(alias, s_to_c(token));
				alias = s_append(alias, " ");
			}
			rv = 0;
		}

		fclose(fp);
		s_free(line);
		s_free(token);
	}

	return rv;
}

/* lookup the "name" for the dbfile "f" provided.
/* if this is the first i_lookup() for a given file,
/* store the file contents in a memory resident hashed deque
/* linked to the dfiles pointer "incore".
/* */

static int i_lookup(name, f, alias)
char *name;
dbfiles *f;
string *alias;	/* returned string */
{
	FILE *fp;
	string *file = f->fname;
	alias_list *p;

	if (verbose > 2)
		printf("i_lookup(%s, %s)\n", name, s_to_c(file));

	if ((f->incore == NULL)
	&& ((fp = fopen(s_to_c(file), "r")) != NULL)) {
		string *line  = s_new();
		string *token = s_new();

		f->incore = new_hdeque();

		s_reset(alias);

		while (s_getline(fp, s_restart(line))!=NULL) {
			alias_list *a;
			string *tmp = NULL;

			if (s_parse(s_restart(line), s_restart(token))==NULL)
				continue;

			a = NEW(alias_list);

			a->key = s_copy(s_to_c(token));

			while (s_parse(line, s_restart(token))!=NULL) {
				if (!tmp) {
					tmp = s_new();
					s_reset(tmp);
				}
				s_append(tmp, s_to_c(token));
				s_append(tmp, " ");

			}

			a->alias = tmp;

			(void) search(f->incore, s_to_c(a->key), (char*)a);
		}

		fclose(fp);
		s_free(line);
		s_free(token);
	}

	if (p = (alias_list *)search(f->incore, name, NULL)) {
		*alias = *p->alias;
		return (0);
	}

	return (-1);
}

/* The recursive form of the aliasing algorithm works like this:
/*
/* translate_list(argc, argv):
/*
/* 	for (target = each address in 'argv') {
/* 		make a one-node tree with 'target' as the root node.
/* 		translate_alias(target);
/* 		print all terminal nodes in the tree
/* 	}
/*
/* translate_alias(target):
/*
/* 	check the alias files to see if 'target' has an alias
/* 	if not, you're done with this 'target' address; return;
/*
/* 	add the aliases of 'target' to the tree as children of 'target'
/*
/* 	for(child = each child of node 'target') {
/* 		for(other = each node in the tree with the same
/* 				alias name as 'child') {
/* 			if('other' is an ancestor of 'child') {
/* 				make 'child' a terminal node
/* 			} else {
/* 				if('child' is not terminal)
/* 					make 'child' a dead node
/* 			}
/* 		}
/* 	}
/* 	for(child = each child of node 'target') {
/* 		if('child' is not 'dead' and not 'terminal')
/* 			translate_alias('child');
/* 	}
/*
/*
/* The code for translate_list() implements a non-recursive form of
/* this algorithm, using:
/*
/* 	deque 'w' as the list of addresses that require resolution,
/* 	deque 'd' as the list of terminal node names,
/* 	deque 'l' as the list of children of 'target'
/* 	deque 'r' as the 'tree'.
/*
/* 	Note, the 'tree' needs to store only an alias's
/* 	name and its parent, since this is sufficient
/* 	to determine ancestry.
/* */

static void translate_list(argc, argv)
int argc;
char **argv;
{
	string *alias = s_new();
	int c;
	/* w = objects of type "tree_node"
	/*     "w"orking list of alises to be resolved
	/* l = objects of type "tree_node"
	/*     list of aliases for a given address
	/* d = objects of type "string"
	/*     hashed list of addresses that are fully resolved
	/*     ("d"one) - string
	/* r = objects of type "tree_node"
	/*     hashed list of aliases encountered while resolving current alias
	/*     the information in the objects forms a tree where the only
	/*     links are from a child node to its parent node.
	/* */

	deque *w = new_deque();
	deque *l = new_deque();
	hdeque *d = new_hdeque();
	hdeque *r = new_hdeque();

	string *p, *parent, *tok;
	tree_node *sp, *pp;

	if (verbose)
		printf("translate_list()\n");

	for (c=0; c < argc; c++) {

		clear_deque(w);
		clear_deque(l);
		clear_hdeque(d);
		clear_hdeque(r);

		parent = s_copy(argv[c]);

		sp = NEW(tree_node);
		sp->par = NULL;
		sp->str = parent;

		put(w, (char *)sp);
		store(r, s_to_c(parent), (char *)sp);

		wdepth++;

		while (sp = (tree_node *)pop(w)) {
			wdepth--;
			if (verbose)
				printf("aliasing '%s'\n", s_to_c(sp->str));

			s_reset(alias);

			translate_name(s_to_c(sp->str), alias);

			if (*s_to_c(alias) == NULL) {
				if (verbose)
					printf("aliasing completed for '%s'\n", s_to_c(sp->str));
				(void) search(d, s_to_c(sp->str), (char*)sp->str);
				continue;
			}

			if (verbose)
				printf("'%s' -> '%s'\n",
					s_to_c(sp->str), s_to_c(alias));

			s_restart(alias);

			while (tok = s_tok(alias, " \t\n")) {
				pp = NEW(tree_node);
				p = s_copy(s_to_c(tok));

				pp->par = sp;	/* parent */
				pp->str = p;	/* child  */

				put(l, (char *)pp);
				store(r, s_to_c(p), (char*)pp);
			}

			while (pp = (tree_node *)pop(l)) {
				tree_node *x = NULL;
				int instash = 0;
				while (x = (tree_node *)find_another(r, s_to_c(pp->str), (char *)x)) {
					if (x != pp) {
						instash = 1;
						if (is_path(x, pp)) {
							/* terminal */
							(void) search(d, s_to_c(pp->str), (char *)pp->str);
							break;
						}
					}
				}
				if (instash == 0) {
					push(w, (char *)pp);
					wdepth++;
				}
			}
		}
		print_list(parent, d);
	}
}

/* put - an object on one end of a dequeue */

static void push(alist, object)
deque *alist;
char *object;
{
	deque *aentry = NEW(deque);

	aentry->object = object;
	aentry->prev = alist;
	aentry->next = alist->next;

	alist->next->prev = aentry;
	alist->next  = aentry;
}

/* put - put an object on the other end of a deque than that used by push */

static void put(alist, object)
deque *alist;
char *object;
{
	deque *aentry = NEW(deque);

	aentry->object = object;
	aentry->next = alist;
	aentry->prev = alist->prev;

	alist->prev->next = aentry;
	alist->prev = aentry;
}

/* pop - get an object off of the same end of a deque used by push */

static char *pop(alist)
deque *alist;
{
	deque *ret = alist->next;
	char *object;

	if (ret != alist) {
		object = ret->object;
		alist->next = ret->next;
		ret->next->prev = alist;
		free(ret);
		return (object);
	}
	return (NULL);
}

#ifdef NOTDEF	/* not used here */
static char *get(alist)
deque *alist;
{
	deque *ret = alist->prev;
	char *object;

	if (ret != alist) {
		object = ret->object;
		alist->prev = ret->prev;
		ret->prev->next = alist;
		free(ret);
		return (object);
	}
	return (NULL);
}
#endif	/* NOTDEF */

/* return the 'first' object on a hashed deque
/* note that since hashing introduces lots of randomness
/* in the ordering of things in a hashed structure, the
/* notion of 'first' and 'next' are kind of fuzzy.
/*
/* suffice it to say that if you start by examining the 'first' object,
/* and follow by examining enough of the 'next' objects, you will have
/* examined all of the objects exactly once.
/* */

static char *firstv(alistv)
hdeque *alistv;
{
	char *obj;

	for (alistv->curr=0; alistv->curr < HASHSIZE; alistv->curr++) {
		deque *alist = alistv->arr + alistv->curr;
		if (obj = first(alist)) return (obj);
	}
	return (NULL);
}

/* return the 'next' object on a hashed deque */

static char *nextv(alistv)
hdeque *alistv;
{
	char *obj;
	deque *alist = alistv->arr + alistv->curr;

	while (alistv->curr < HASHSIZE) {

		if (obj = next(alist)) return (obj);

		alistv->curr++;
		if (alistv->curr < HASHSIZE) {
			alist++;
			if (obj = first(alist)) return (obj);
		}
	}
	return (NULL);
}

/* first - return the 'first' object on the deque */

static char *first(alist)
deque *alist;
{
	alist->curr = alist->next;
	if (alist->curr == alist) return (NULL);
	return (alist->curr->object);
}

/* return the 'next' object on a deque */

static char *next(alist)
deque *alist;
{
	alist->curr = alist->curr->next;
	if (alist->curr == alist) return (NULL);
	return (alist->curr->object);
}

#ifdef NOTDEF	/* not used here */
static char *prev(alist)
deque *alist;
{
	alist->curr = alist->curr->prev;
	if (alist->curr == alist) return (NULL);
	return (alist->curr->object);
}
#endif	/* NOTDEF */

/* strsav - return a pointer to a dynamically allocated copy of 's' */

static char *strsav(s)
char *s;
{
	char *ret;
	ret = xmalloc(strlen(s)+1);
	strcpy(ret, s);
	return (ret);
}

/* search - look for an object with key "ckey" in a hashed deque
/*	if an object with key "ckey" is found, return the object
/*
/*	if the caller provides a non-NULL "object", and no objects
/*	match "ckey", install the object provided with key "ckey"
/*
/*	return NULL.
/* */
static char *search(alistv, ckey, object)
hdeque *alistv;
char *ckey;
char *object;
{
	deque *p, *alist;

	alist = alistv->arr + hashfunc(ckey);

	for (p=alist->next; p != alist; p=p->next) {
		if (cascmp(p->key, ckey) == 0) {
			return (p->object);
		}
	}

	if (object) {
		put(alist, object);
		alist->prev->key = strsav(ckey);
	}

	return (NULL);
}

/* store - add an object to a hashed deque */

static void store(alistv, ckey, object)
hdeque *alistv;
char *ckey;
char *object;
{
	deque *alist;

	alist = alistv->arr + hashfunc(ckey);
	put(alist, object);

	alist->prev->key = strsav(ckey);
}

static int rvals[0x20] = {
	6806,	28022,	28540,	2313,	15939,	30532,	8778,	27444,
	24757,	10999,	20978,	24771,	9519,	2228,	19669,	30872,
	27822,	6097,	20224,	11081,	15733,	6362,	29568,	7652,
	14792,	5693,	17903,	6631,	20888,	27590,	6562,	28809,
};

/* hashfunc - given a character string, calculate a number that can be
/*            used as a hash-key for a hashed deque.
/*
/*	the function uses the table of random numbers above to increase
/*	the dispersion of the generated hash-key values.
/* */

static int hashfunc(s)
register char *s;
{
	register char c;
	register int key;

	key = 0;

	while (c = *s++) key += rvals[lower(c) & 0x1f];
	key %= HASHSIZE;
	return (key);
}

/* new_hdeque - allocate space and initialize the structure for a hdeque */

static hdeque *new_hdeque()
{
	hdeque *ret = NEW(hdeque);
	int i;

	for (i=0; i < HASHSIZE; i++) {
		deque *alist = ret->arr + i;
		alist->next = alist->prev = alist;
	}
	ret->curr = 0;
	return (ret);
}

/* new_deque - allocate space and initialize the structure for a deque */

static deque *new_deque()
{
	deque *ret = NEW(deque);
	ret->next = ret->prev = ret;
	return (ret);
}

/* clear_hdeque - remove all nodes from the named hdeque */

static void clear_hdeque(alistv)
hdeque *alistv;
{
	int i;

	for (i=0; i < HASHSIZE; i++) clear_deque(alistv->arr + i);
	alistv->curr = 0;
}

/* clear_deque - remove all nodes from the named deque */

static void clear_deque(alist)
deque *alist;
{
	while (pop(alist))
		/* nothing */;
}

/* print_list - print every string in the named hashed deque "alist"
/*	when necessary, prefix each string with "parent" as itsprefix
/* */

static void print_list(parent, alistv)
string *parent;
hdeque *alistv;
{
	string *p;
	char *name;

	for (p = (string *)firstv(alistv); p ; p = (string *)nextv(alistv)) {
		name = s_to_c(p);
		if (*name) {
			if (!simple || useprefix) {
				printf("%s\t", s_to_c(parent));
			}
			if (p == parent)
				print_local(name);
			else
				print_name(name);
		}
		free(name);
	}
}

/* xmalloc - error checking malloc() - exits on failure */

static char *xmalloc(n)
int n;
{
	char *ret = malloc(n);
	if (!ret) {
		(void) pfmt(stderr, MM_ERROR,
			":10:Out of memory: %s\n", strerror(errno));
		exit(1);
	}
	return (ret);
}

#define FPCACHE 10

struct fpcache {
	char *path;
	FILE *fp;
};

static struct fpcache fc[FPCACHE];

/* cfopen - cache open file pointers to avoid repeated fopen()'s
/*
/*	look for a file pointer which matches the "path"
/*	if found, rewind the file and return the file pointer
/*
/*	get a file pointer for "path" via fopen()
/*
/*	if an empy slot was found,
/*		add the file pointer and "path"	to the cache
/*
/*	return the file pointer
/* */

static FILE *cfopen(path, mode)
char *path, *mode;
{
	register struct fpcache *f, *e;
	FILE *fp;

	for (e = NULL, f = fc; f < fc + FPCACHE; f++) {
		if (f->fp) {
			if (strcmp(path, f->path) == 0) {
				rewind(f->fp);
				return (f->fp);
			}
		} else {
			if (!e) e = f;
		}
	}

	fp = fopen(path, mode);

	if (fp && e) {
		e->path = path;
		e->fp = fp;
	}

	return (fp);
}

/* cached fclose - avoid closing files which are likely to be needed again
/*
/*	if the file pointer 'fp' matches a file pointer in the cache
/*		don't fclose the file.
/*	otherwise,
/*		fclose() the file associated with 'fp'
/* */

static void cfclose(fp)
register FILE *fp;
{
	register struct fpcache *f;
	for (f = fc; f < fc + FPCACHE; f++) {
		if (f->fp == fp) {
			return;
		}
	}
	fclose(fp);
}

/* find_another() -
/*	finds nodes in a hashed deque with key "ckey"
/*	if "clue" is NULL,
/*		the search starts at one end of the deque
/*		and works its way towards the other end.
/*	othewise,
/*		it assumes that a previous search succeeded (returning
/*		the object) and it continues from where it left off.
/*
/*	Thus, successive calls to find_another() can be used to find
/*	all nodes in a hashed list which match a given key.
/*
/* */

static char *find_another(alistv, ckey, clue)
hdeque *alistv;
char *ckey;
char *clue;
{
	static deque *alist, *probe;

	if (clue == NULL) {
		probe = alist = alistv->arr + hashfunc(ckey);
	}

	for (probe=probe->next; probe != alist; probe = probe->next) {
		if (cascmp(probe->key, ckey) == 0) {
			return (probe->object);
		}
	}

	return (NULL);
}

/* is_path(parent, child)
/*   return 1 - if parent is an ancestor of the child
/*   return 0 - otherwise
/* */

static int is_path(parent, child)
register tree_node *parent, *child;
{
	while (child->par) {
		if (child->par == parent) return (1);
		child = child->par;
	}
	return (0);
}

/* Check if a name which has no alias is local. */
/* If so and -L has been specified, print the */
/* name using the given prefix and suffix. */
static void print_local(name)
const char *name;
{
	if (printlocal && islocal(name, (uid_t*)0))
		printf("%s%s%s\n", Lprefix, name, Lsuffix);
	else
		print_name(name);
}

/* Strip off any specified prefixes or suffixes and print the name. */
static void print_name(name)
const char *name;
{
	register int i;
	if (stripprefixes > 0) {
		for (i = 0; i < stripprefixes; i++) {
			const char *pname = Pprefix[i].name;
			if ((name[0] == pname[0]) &&
			    (strncmp(name+1, pname+1, Pprefix[i].length-1) == 0))
				name += Pprefix[i].length;
		}
	}
	if (stripsuffixes > 0) {
		register int length = strlen(name);
		for (i = 0; i < stripsuffixes; i++) {
			register int suflength = Ssuffix[i].length;
			if ((length > suflength) &&
			    (strncmp(name + length - suflength, Ssuffix[i].name, suflength) == 0))
				length -= Ssuffix[i].length;
		}
		for (i = 0; i < length; i++)
			putchar(name[i]);
		putchar('\n');
	} else {
		(void) printf("%s\n", name);
	}
}
