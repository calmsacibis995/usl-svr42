/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ar:common/ar.c	1.23"
/* ar: UNIX Archive Maintainer */


#include <stdio.h>
#include <sys/param.h>
#include <ar.h>
#include <locale.h>

#ifndef	UID_NOBODY
#define UID_NOBODY	60001
#endif

#ifndef GID_NOBODY
#define GID_NOBODY      60001
#endif

#ifdef __STDC__
#include <stdlib.h>
#endif

#include "libelf.h"
#include <ccstypes.h>

#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pfmt.h>
#include "paths.h"
#include "sgs.h"

#ifdef __STDC__
#include <time.h>
#include <locale.h>
#endif

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000

#define FLAG(ch)	(flg[ch - 'a'])
#define CHUNK		500
#define SYMCHUNK	1000
#define SNAME		16
#define ROUNDUP(x)	(((x) + 1) & ~1)

#define LONGDIRNAME	"//              "
#define SYMDIRNAME	"/               "	/* symbol directory filename */
#define FORMAT		"%-16s%-12ld%-6u%-6u%-8o%-10ld%-2s"
#define DATESIZE	60

static	struct stat	stbuf;


typedef struct arfile ARFILE;

static	char	ptr_index[SNAME];	/* holds the string that corresponds */
					/* to the filename's index in table  */
struct arfile
{
	char	ar_name[SNAME];		/* info from archive member header */
	long	ar_date;
	int	ar_uid;
	int	ar_gid;
	unsigned long	ar_mode;
	long	ar_size;
	char    *longname;
	long	offset;
	char	*pathname;
	char	*contents;
	ARFILE	*next;
};

static long	nsyms, *symlist;
static long	sym_tab_size, long_tab_size;
static long *sym_ptr;
static long *nextsym = NULL;
static int syms_left = 0;


static ARFILE	*listhead, *listend;

static FILE     *outfile;
static int	fd;

static Elf	*elf, *arf;

static char	flg[26];
static char	**namv;
static char	*arnam;
static char	*ponam;
static char	*gfile;
static char	*str_base,	/* start of string table for names */
		*str_top;	/* pointer to next available location */

static char	*str_base1,
		*str_top1;
static	int	longnames = 0;

static int      signum[] = {SIGHUP, SIGINT, SIGQUIT, 0};
static int      namc;
static int      modified;
static int	Vflag=0;

static	int	m1[] = { 1, ROWN, 'r', '-' };
static	int	m2[] = { 1, WOWN, 'w', '-' };
static	int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
static	int	m4[] = { 1, RGRP, 'r', '-' };
static	int	m5[] = { 1, WGRP, 'w', '-' };
static	int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
static	int	m7[] = { 1, ROTH, 'r', '-' };
static	int	m8[] = { 1, WOTH, 'w', '-' };
static	int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

static	int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

static	int	notfound(),	qcmd(),
		rcmd(),		dcmd(),		xcmd(),
		pcmd(),		mcmd(),		tcmd();

static	void	setcom(),	usage(),	sigexit(),	
		cleanup(),	movefil(),	 mesg(),		
		select(),	mksymtab(),	getaf(),
		savename(),     writefile(),	search_sym_tab(),
		sputl(),	writesymtab(),	mklong_tab();

static	char	*trim(),	*match(),	*trimslash();

static	ARFILE	*getfile(),	*newfile();

static	FILE	*stats();
static  int     (*comfun)();

extern	char	*tempnam(),	*ctime();
extern	long	time(), lseek();
extern	void	exit(),		free();
extern  int	creat(),	write(),	close(),
		access(),	unlink(),	stat(),
		read();

static char badposnam[] = ":1:Posname, %s, not found\n";
static char nomem[] = ":2:Out of memory\n";
static char malformar[] = ":3:%s: malformed archive (at %ld)\n";
static char badopen[] = ":4:Cannot open %s: %s\n";
static char badread[] = ":5:Cannot read %s: %s\n";
static char badlibelf[] = ":6:libelf error: %s\n";
static char nostring[] = ":7:%s cannot get string table space\n";
static char nogrowstring[] = ":8:%s cannot grow string table\n";
static char creating[] = ":9:Creating %s\n";
static char badcreate[] = ":10:Cannot create %s: %s\n";

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *cp;
	int argv_len=0;
	char label[20];


	for (i = 0; signum[i]; i++)
		if (signal(signum[i], SIG_IGN) != SIG_IGN)
			(void) signal(signum[i], sigexit);

	(void)setlocale(LC_ALL, "");
	(void)sprintf(label, "UX:%sar", SGS);
	(void) setlabel(label);
	(void) setcat("uxar");

	if (argc < 2){
		pfmt(stderr, MM_ERROR, ":11:Incorrect usage\n");
		usage();
	}

	cp = argv[1];
	argv_len = strlen(argv[1]);

	if (*cp == '-')	/* skip a '-', make ar more like other commands */
		cp++;
	for (; *cp;)
	{
		switch (*cp)
		{
		case 'l':
			(void) pfmt(stderr, MM_WARNING, 
		":12:The l option will be removed in the next release\n");
			cp++;
			continue;
		case 'v':
		case 'u':
		case 'a':
		case 'b':
		case 'c':
		case 'i':
		case 's':
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'r':
			setcom(rcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'd':
			setcom(dcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'x':
			setcom(xcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 't':
			setcom(tcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'p':
			setcom(pcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'm':
			setcom(mcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'q':
			setcom(qcmd);
			flg[*cp - 'a']++;
			cp++;
			continue;
		case 'V':
			(void) pfmt(stderr, MM_INFO|MM_NOGET, "%s %s\n",
				CPL_PKG, CPL_REL);
			if (argv_len <= 2 && argc-Vflag > 2) {
				argv_len = strlen(argv[2+Vflag]);
				cp = argv[2+Vflag];
				if ( *cp == '-')
					cp++;
				Vflag++;	/* how many -V options */
				}
			else
				usage();
			continue;
		default:
			(void) pfmt(stderr, MM_ERROR, "uxlibc:1:Illegal option -- %c\n"
				, *cp);
			usage();
		}
	}
	
	if (argc - Vflag < 3)
		usage();

	if (FLAG('i'))
		FLAG('b')++;
	if (FLAG('a') || FLAG('b'))
	{
		ponam = trim(argv[2+Vflag]);
		argv++;
		argc--;
		if (argc-Vflag < 3)
			usage();
	}

	arnam = argv[2+Vflag];
	namv = argv + 3 + Vflag;
	namc = argc - (3 + Vflag);

	if (comfun == 0)
	{
		if ( !(FLAG('d') || FLAG('r') || FLAG('q') || FLAG('t') || FLAG('p') || FLAG('m') || FLAG('x') ) )
		{
			(void) pfmt(stderr, MM_ERROR,
				":13:One of [drqtpmx] must be specified\n");
			exit(1);
		}
	}

	modified = FLAG('s');
	getaf(); 

	if ( (fd == -1) && (FLAG('d') || FLAG('t') || FLAG('p')
                              || FLAG('m') || FLAG('x') || 
			      (FLAG('r') && (FLAG('a') || FLAG('i') || 
			       FLAG('b') ) ) )   ) 
		{
                   (void) pfmt(stderr, MM_ERROR,":14:Archive, %s, not found\n"
			, arnam);
                   exit(1);
                }

	(*comfun)();
	if (modified)	/* make archive symbol table */
		writefile();
	(void) close(fd);
	return(notfound());
}


static void
setcom(fun)
	int (*fun)();
{

	if (comfun != 0)
	{
		(void) pfmt(stderr, MM_ERROR, ":15:Only one of [drqtpmx] allowed\n"
			);
		exit(1);
	}
	comfun = fun;
}


static	int
rcmd()
{
	register FILE *f;
	register ARFILE *fileptr;
	register ARFILE	*abifile = NULL;
	register ARFILE	*backptr = NULL;
	ARFILE	*endptr;
	ARFILE	*moved_files;

	for ( fileptr = getfile(); fileptr; fileptr = getfile())
	{
		if ( !abifile && ponam && strcmp( fileptr->longname, ponam ) == 0)
			abifile = fileptr;
		else if ( !abifile )
			backptr = fileptr;


		if (namc == 0 || match(fileptr->longname) != NULL )
		{
			f = stats( gfile );
			if (f == NULL)
			{
				if (namc)
					(void) pfmt(stderr, MM_ERROR, badopen,
						gfile, strerror(errno));
				mesg('c', gfile);
			}
			else
			{
				if (FLAG('u') && stbuf.st_mtime <= fileptr->ar_date)
				{
					(void) fclose(f);
					continue;
				}
				mesg('r', fileptr->longname);
				movefil( fileptr );
				free( fileptr->contents );
				if ((fileptr->contents = (char *)malloc( ROUNDUP( stbuf.st_size ))) == NULL)
				{
					(void) pfmt( stderr, MM_ERROR, nomem);
					exit(1);
				}
				if (fread( fileptr->contents, sizeof(char), stbuf.st_size, f ) != stbuf.st_size)
				{
					(void) pfmt( stderr, MM_ERROR, badread,
					fileptr->longname, strerror(errno));
					exit(1);
				}
				if ( fileptr->pathname != NULL)
					free(fileptr->pathname);
				if ((fileptr->pathname= (char *)malloc( strlen(gfile) * sizeof(char *)  )) == NULL)
                                {
                                        (void) pfmt( stderr, MM_ERROR, nomem);
                                        exit(1);
                                }
				(void) strcpy(fileptr->pathname, gfile);
				fileptr->offset = 0;

				(void) fclose(f);
				modified++;
			}
		}
		else
			mesg( 'c', fileptr->longname);
	}

	endptr = listend;
	cleanup();
	if (ponam && endptr && (moved_files = endptr->next))
	{
		if (!abifile)
		{
			(void) pfmt( stderr, MM_ERROR, badposnam, ponam);
			exit(2);
		}
		endptr->next = NULL;

		if (FLAG('b'))
			abifile = backptr;

		if (abifile)
		{
			listend->next = abifile->next;
			abifile->next = moved_files;
		}
		else
		{
			listend->next = listhead;
			listhead = moved_files;
		}
		listend = endptr;
	}
	else if (ponam && !abifile)
                (void) pfmt(stderr, MM_ERROR, badposnam, ponam);
	return(0);
}

static	int
dcmd()
{
	register ARFILE	*fptr;
	register ARFILE *backptr = NULL;

	for( fptr = getfile(); fptr; fptr = getfile())
	{
		if (match( fptr->longname) != NULL)
		{
			mesg('d', fptr->longname);
			if (backptr == NULL)
				listhead = NULL;
			else
			{
				backptr->next = NULL;
				listend = backptr;
			}
			modified = 1;
		}
		else
		{
			mesg('c', fptr->longname);
			backptr = fptr;
		}
	}
	return(0);
}

static	int
xcmd()
{
	register int f;
	register ARFILE *next;

	for( next = getfile(); next; next = getfile())
	{
		if (namc == 0 || match( next->longname) != NULL)
		{
			f = creat( next->longname, (mode_t)next->ar_mode & 0777);
			if (f < 0)
			{
				(void) pfmt(stderr, MM_ERROR, badcreate,
					next->longname, strerror(errno));
				mesg('c', next->longname);
			}
			else
			{
				mesg( 'x', next->longname);
				if (write( f, next->contents, (unsigned)next->ar_size ) != next->ar_size)
				{
					(void) pfmt( stderr, MM_ERROR, 
					":16:Cannot write %s: %s\n",
					next->longname,strerror(errno));
					exit(1);
				}
				(void) close(f);
			}
		}
	}
	return(0);
}

static	int
pcmd()
{
	register ARFILE	*next;

	for( next = getfile(); next; next = getfile())
	{
		if (namc == 0 || match( next->longname) != NULL)
		{
			if (FLAG('v'))
			{
				(void) fprintf(stdout, "\n<%s>\n\n", next->longname);
				(void) fflush(stdout);
			}
			(void) fwrite( next->contents, sizeof(char), next->ar_size, stdout );
		}
	}
	return(0);
}

static	int
mcmd()
{
	register ARFILE	*fileptr;
	register ARFILE	*abifile = NULL;
	register ARFILE	*tmphead = NULL;
	register ARFILE	*tmpend = NULL;
	ARFILE	*backptr1 = NULL;
	ARFILE	*backptr2 = NULL;

	for( fileptr = getfile(); fileptr; fileptr = getfile() )
	{
		if (match( fileptr->longname) != NULL)
		{
			mesg( 'm', fileptr->longname);
			if ( tmphead )
				tmpend->next = fileptr;
			else
				tmphead = fileptr;
			tmpend = fileptr;

			if (backptr1)
			{
				listend = backptr1;
				listend->next = NULL;
			}
			else
				listhead = NULL;
			continue;
		}

		mesg( 'c', fileptr->longname);
		backptr1 = fileptr;
		if ( ponam && !abifile )
		{
			if ( strcmp( fileptr->longname, ponam ) == 0)
				abifile = fileptr;
			else
				backptr2 = fileptr;
		}
	}

	if ( !tmphead )
		return(1);

	if ( !ponam )
		listend->next = tmphead;
	else
	{
		if ( !abifile )
		{
			(void) pfmt( stderr, MM_ERROR, badposnam, ponam);
			exit(2);
		}
		if (FLAG('b'))
			abifile = backptr2;

		if (abifile)
		{
			tmpend->next = abifile->next;
			abifile->next = tmphead;
		}
		else
		{
			tmphead->next = listhead;
			listhead = tmphead;
		}
	}

	modified++;
	return(0);
}

static	int
tcmd()
{
	register ARFILE	*next;
	register int	**mp;
#ifdef __STDC__
	char   buf[DATESIZE];
#else
	register char	*cp;
#endif

	for( next = getfile(); next; next = getfile() )
	{
		if (namc == 0 || match( next->longname) != NULL)
		{
			if (FLAG('v'))
			{
				for (mp = &m[0]; mp < &m[9];)
					select(*mp++, next->ar_mode);

				(void) fprintf(stdout, "%6d/%6d", next->ar_uid, 
					next->ar_gid);
				(void) fprintf(stdout, "%7ld", next->ar_size);
#ifdef __STDC__
				(void)setlocale(LC_TIME, "");
				if ( (strftime(buf,DATESIZE,
					gettxt(":17", "%b %d %H:%M %Y"),
					localtime( &(next->ar_date) )) ) == 0 ) 
				{
				    (void) pfmt(stderr, MM_ERROR, 
			":18:Not enough space to store the date\n");
					exit(1);
				}
			
				(void) fprintf(stdout, " %s ", buf);	
#else
				cp = ctime( &(next->ar_date));
                                (void) fprintf(stdout, " %-12.12s %-4.4s ", cp+4, cp + 20);
#endif


			}
			(void) fprintf(stdout, "%s\n", trim(next->longname));
		}
	}
	return(0);
}

static	int
qcmd()
{
	register ARFILE *fptr;

	if (FLAG('a') || FLAG('b'))
	{
		(void) pfmt(stderr, MM_ERROR, ":19:abi not allowed with q\n");
		exit(1);
	}

	for ( fptr = getfile(); fptr; fptr = getfile())
		;
	cleanup();
	return(0);
}



static void
getaf()
{
        Elf_Cmd cmd;

        if (elf_version(EV_CURRENT) == EV_NONE){
                (void) pfmt(stderr, MM_ERROR, ":20:libelf.a out of date\n");
		exit(1);
		}

        if ( (fd  = open(arnam, O_RDONLY)) == -1) {
                return; /* archive does not exist yet, may have to create one*/
                }

	
	cmd = ELF_C_READ;
	arf = elf_begin(fd, cmd, (Elf *)0);

	if (elf_kind(arf) != ELF_K_AR){
		(void) pfmt(stderr, MM_ERROR, ":21:%s not in archive format\n"
			, arnam);
		if (FLAG('a') || FLAG('b'))
		    (void) pfmt(stderr, MM_ERROR,
		    ":22:%s taken as mandatory 'posname' with keys 'abi'\n"
				,ponam);
		exit(1);
	}
	
}

static	ARFILE *
getfile()
{
	Elf_Arhdr *mem_header;
	register ARFILE	*file;

	if ( fd == -1 )
		return( NULL ); /* the archive doesn't exist */

	if ( (elf = elf_begin(fd, ELF_C_READ, arf) ) == 0 )
		return( NULL );  /* the archive is empty or have hit the end */

 	if ( ( mem_header = elf_getarhdr(elf) ) == NULL) {
                (void) pfmt( stderr, MM_ERROR, malformar,
                         arnam, elf_getbase(elf) );
                exit(1);
        }


	/* zip past special members like the symbol and string table members */
	while ( strncmp(mem_header->ar_name,"/",1) == 0 ||
	     	strncmp(mem_header->ar_name,"//",2) == 0 ) 
	{
			(void) elf_next(elf);
			(void) elf_end(elf);
			if ((elf = elf_begin(fd, ELF_C_READ, arf)) == 0)
                		return( NULL );  /* the archive is empty or have hit the end */
                        if ( (mem_header = elf_getarhdr(elf) ) == NULL) 
			{
                                (void) pfmt( stderr, MM_ERROR, malformar,
					arnam, elf_getbase(elf) );
                                exit(0);
			}
       }

	file = newfile();

	(void) strncpy( file->ar_name, mem_header->ar_name, SNAME );

	if ((file->longname = (char *) malloc( strlen(mem_header->ar_name) * sizeof(char *) )) == NULL) {
		(void) pfmt( stderr, MM_ERROR, nomem);
		exit(1);
		}
	(void) strcpy( file->longname, mem_header->ar_name);

		
	file->ar_date = mem_header->ar_date;
	file->ar_uid  = mem_header->ar_uid;
	file->ar_gid  = mem_header->ar_gid;
	file->ar_mode = (unsigned long) mem_header->ar_mode;
	file->ar_size = mem_header->ar_size;
	file->offset = elf_getbase(elf); 

	/* reverse logic */
	if ( !(FLAG('t') && !FLAG('s')) ){
		if ((file->contents = (char *)malloc( ROUNDUP( file->ar_size ))) == NULL) {
			(void) pfmt( stderr, MM_ERROR, nomem);
			exit(1);
		}

		if ( lseek(fd, file->offset, 0) != file->offset ){
			(void) pfmt( stderr, MM_ERROR, ":23:lseek() failed: %s\n",
				strerror(errno));
			exit(1);
		}

		if ( read(fd, file->contents, (unsigned) ROUNDUP(file->ar_size ) ) == -1 ){
			(void) pfmt( stderr, MM_ERROR, badread, arnam,
				strerror(errno));
			exit(1);
		}
	}

	(void) elf_next(elf);
	(void) elf_end(elf);
	return (file);
}

static	ARFILE *
newfile()
{
	static ARFILE	*buffer =  NULL;
	static int	count = 0;
	register ARFILE	*fileptr;

	if (count == 0)
	{
		if ((buffer = (ARFILE *) calloc( CHUNK, sizeof( ARFILE ))) == NULL)
		{
			(void) pfmt( stderr, MM_ERROR, nomem);
			exit(1);
		}
		count = CHUNK;
	}
	count--;
	fileptr = buffer++;

	if (listhead)
		listend->next = fileptr;
	else
		listhead = fileptr;
	listend = fileptr;
	return( fileptr );
}

static void
usage()
{
	(void) pfmt(stderr, MM_ACTION, ":24:Usage: ar [-V] key[vcs] [posname] archive file [name] ...\n\t where key is one of the following: r[uabi], m[abi], d, q, t, p, x\n" );
	exit(1);
}


/*ARGSUSED0*/
static void
sigexit(i)
	int i;
{
	if (outfile)
		(void) unlink( arnam );
	exit(100);
}

/* tells the user which of the listed files were not found in the archive */

static int
notfound()
{
	register int i, n;

	n = 0;
	for (i = 0; i < namc; i++)
		if (namv[i])
		{
			(void) pfmt(stderr, MM_ERROR, 
				":25:%s not found\n", namv[i]);
			n++;
		}
	return (n);
}


/* puts the file which was in the list in the linked list */

static void
cleanup()
{
	register int i;
	register FILE	*f;
	register ARFILE	*fileptr;

	for (i = 0; i < namc; i++)
	{
		if (namv[i] == 0)
			continue;
		mesg('a', namv[i] );
		f = stats( namv[i] );
		if (f == NULL)
			(void) pfmt(stderr, MM_ERROR, badopen, namv[i],
				strerror(errno));
		else
		{
			fileptr = newfile();
			/* if short name */
			(void) strcpy( fileptr->ar_name, trim( namv[i] ));

			if ((fileptr->longname = (char *)malloc( strlen(trim( namv[i] )) * sizeof(char *)) ) == NULL)
                        {
                                (void) pfmt( stderr, MM_ERROR, nomem);
                                exit(1);
                        }	

			(void) strcpy( fileptr->longname,  trim(namv[i]) );

			if ((fileptr->pathname = (char *)malloc( strlen(namv[i]) * sizeof(char *)) ) == NULL) {
                                (void) pfmt( stderr, MM_ERROR, nomem);
                                exit(1);
                        }

                        (void) strcpy( fileptr->pathname,  namv[i] );


			
			movefil( fileptr );
			if ((fileptr->contents = (char *)malloc( ROUNDUP( stbuf.st_size ))) == NULL)
			{
				(void) pfmt( stderr, MM_ERROR, nomem);
				exit(1);
			}
			if (fread( fileptr->contents, sizeof(char), stbuf.st_size, f ) != stbuf.st_size )
			{
				(void) pfmt( stderr, MM_ERROR, badread,
					fileptr->longname, strerror(errno));
				exit(1);
			} 

			(void) fclose(f);
			modified++;
		        namv[i] = 0;
		}
	}
}

/*
* insert the file 'file' into the temporary file
*/

static void
movefil( fileptr )
	register ARFILE *fileptr;
{
	fileptr->ar_size = stbuf.st_size;
	fileptr->ar_date = stbuf.st_mtime;

	if (stbuf.st_uid > 60000 )
		fileptr->ar_uid = UID_NOBODY;
	else
		fileptr->ar_uid = stbuf.st_uid;

	if (stbuf.st_gid > 60000 )
		fileptr->ar_uid = GID_NOBODY;
	else
		fileptr->ar_gid = stbuf.st_gid;

	fileptr->ar_mode = stbuf.st_mode;
}


static FILE *
stats( file )
	register char *file;
{
	register FILE *f;

	f = fopen(file, "r");
	if (f == NULL)
		return(f);
	if (stat(file, &stbuf) < 0)
	{
		(void) fclose(f);
		return(NULL);
	}
	return (f);
}


static char *
match( file )
	register char	*file;
{
	register int i;

	for (i = 0; i < namc; i++)
	{
		if (namv[i] == 0)
			continue;
		if (strcmp(trim(namv[i]), file) == 0)
		{
			gfile = namv[i];
			file = namv[i];
			namv[i] = 0;
			return (file);
		}
	}
	return (NULL);
}



static void
mesg(c, file)
	int	c;
	char	*file;
{
	if (FLAG('v'))
		if (c != 'c' || FLAG('v') > 1)
			(void) fprintf(stdout, "%c - %s\n", c, file);
}



static char *
trimslash(s)
	char *s;
{
	static char buf[SNAME];

	(void)strncpy(buf, trim(s), SNAME - 2);
	buf[SNAME - 2] = '\0';
	return (strcat(buf, "/"));
}


static char *
trim(s)
	char *s;
{
	register char *p1, *p2;

	for (p1 = s; *p1; p1++)
		;
	while (p1 > s)
	{
		if (*--p1 != '/')
			break;
		*p1 = 0;
	}
	p2 = s;
	for (p1 = s; *p1; p1++)
		if (*p1 == '/')
			p2 = p1 + 1;
	return (p2);
}

static void
select(pairp, mode)
	int	*pairp;
	unsigned long	mode;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n >= 0 && (mode & *ap++) == 0)
		ap++;
	(void) putchar(*ap);
}


static void
mksymtab()
{
	register ARFILE	*fptr;
	long	mem_offset = 0;

	Elf_Scn	*scn;
	Elf32_Shdr *shdr;
	int newfd;


	for( fptr = listhead; fptr; fptr = fptr->next ) {
		newfd = -1;
	
		/* determine if file is coming from the archive or not */

		if ( (fptr->offset > 0) && (fptr->pathname == NULL) ) {

			if (elf_rand(arf, fptr->offset - sizeof(struct ar_hdr) )
				!= fptr->offset - sizeof(struct ar_hdr) ) {
				(void) pfmt(stderr, MM_ERROR, badlibelf,
					elf_errmsg(-1));
				exit(1);
			}

			if ((elf = elf_begin(fd, ELF_C_READ, arf)) == 0) {
				(void) pfmt(stderr, MM_ERROR,
					":26:Hit end of archive\n");
				break;
				}
			}
		else
		 if ( (fptr->offset == 0) && (fptr->pathname != NULL) ){
			if ( (newfd  = open(fptr->pathname, O_RDONLY)) == -1)
			{
				(void) pfmt(stderr, MM_ERROR, badopen,
					fptr->pathname, strerror(errno));
				exit(1);
			}

			if ((elf = elf_begin(newfd, ELF_C_READ, (Elf *)0)) == 0)
			{
				(void) pfmt(stderr, MM_ERROR, badlibelf,
                                        elf_errmsg(-1));
                                exit(1);
			}
			if (elf_kind(elf) == ELF_K_AR){
				(void) pfmt(stderr, MM_ERROR, 
	":27:%s is in archive format - embedded archives are not allowed\n"
				,fptr->pathname);
				exit(1);
				}
			}
		else{
			(void) pfmt(stderr, MM_ERROR, ":28:Internal error - cannot tell whether file is included in archive or not\n");
			exit(1);
			}
			
				
		if (elf_kind(elf) == ELF_K_COFF) {
			if (elf_update(elf, ELF_C_NULL) == -1) {
				(void) pfmt(stderr, MM_ERROR, 
				":29:Trouble translating COFF file %s: %s\n"
				,fptr->pathname, elf_errmsg(-1));
				exit(1);
			}
		}

		if ((elf32_getehdr(elf)) != NULL)
		{
			/* loop through sections to find symbol table */
			scn = 0;
			while( (scn = elf_nextscn(elf,scn)) != 0)
			{
				if ((shdr = elf32_getshdr(scn)) == NULL)
				{
        			        (void) pfmt(stderr, MM_ERROR, badlibelf,
					elf_errmsg(-1));
			                break;
        			}
				if ( shdr->sh_type == SHT_SYMTAB)
					search_sym_tab(elf,shdr,scn,mem_offset); 
			}
		}

		mem_offset += sizeof( struct ar_hdr ) + ROUNDUP( fptr->ar_size);
		(void) elf_end(elf);
		if (newfd >= 0)
			(void) close(newfd);
			
		} /* for */
}

static void
writesymtab( tf )
	register FILE	*tf;
{
	long	offset;
	char	buf1[sizeof( struct ar_hdr ) + 1];
	register char	*buf2, *bptr;
	int	i, j;
	long	*ptr;

	/*
	* patch up archive pointers and write the symbol entries
	*/
	while ((str_top - str_base) & 0x3)	/* round up string table */
		*str_top++ = '\0';

	sym_tab_size = (nsyms +1) * 4 + sizeof(char) * (str_top -str_base);

	offset = (nsyms + 1) * 4 + sizeof(char) * (str_top - str_base)
		+ sizeof(struct ar_hdr) + SARMAG;

	(void) sprintf(buf1, FORMAT, SYMDIRNAME, time(0), (unsigned)0, (unsigned)0, (unsigned)0, (long)sym_tab_size, ARFMAG);

	if (longnames)
		offset += long_tab_size + sizeof(struct ar_hdr);
	

	if ( strlen(buf1) != sizeof(struct ar_hdr))
	{
		(void) pfmt(stderr, MM_ERROR,	
			":30:Internal header generation error\n");
		exit(1);
	}

	if ( (buf2 = (char *)malloc( 4 * (nsyms + 1))) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, 
			":31:Cannot get space for number of symbols\n");
		exit(1);
	}
	sputl(nsyms, buf2);
	bptr = buf2 + 4;

	for (i = 0, j = SYMCHUNK, ptr = symlist; i < nsyms; i++, j--, ptr++)
	{
		if ( !j ) {
			j = SYMCHUNK;
			ptr = (long *) *ptr;
			}
		*ptr += offset;
		sputl( *ptr, bptr );
		bptr += 4;
	}


	(void) fwrite( buf1, 1, sizeof(struct ar_hdr), tf );
	(void) fwrite( buf2, 1, (nsyms  + 1) * 4, tf );
	(void) fwrite( str_base, 1,  sizeof(char) * (str_top - str_base), tf );
}


static void
savename(symbol)
	char    *symbol;
{
	static int str_length = BUFSIZ * 5;
	register char *p, *s;
	register unsigned int i;
	int diff;

	diff = 0;
	if (str_base == (char *)0)	/* no space allocated yet */
	{
		if ((str_base = (char *)malloc((unsigned)str_length)) == NULL)
		{
			(void) pfmt(stderr, MM_ERROR, nostring, arnam);
			exit(1);
		}
		str_top = str_base;
	}


	p = str_top;
		str_top += strlen(symbol) + 1;

	if (str_top > str_base + str_length)
	{
		char *old_base = str_base;

		str_length += BUFSIZ * 2;
		if ((str_base = (char *)realloc(str_base, str_length)) == NULL)
		{
			(void) pfmt(stderr, MM_ERROR, nogrowstring, arnam);
				exit(1);
		}
		/*
		* Re-adjust other pointers
		*/
		diff = str_base - old_base;
		p += diff;
	}

		for (i = 0, s = symbol;
			i < strlen(symbol) && *s != '\0'; i++)
		{
			*p++ = *s++;
		}
		*p++ = '\0';
		str_top = p;
}

static void
savelongname(fptr)
ARFILE	*fptr;
{
        static int str_length = BUFSIZ * 5;
        register char *p, *s;
        register unsigned int i;
        int diff;
	static int bytes_used;
	int index;
	char	ptr_index1[SNAME-1];

        diff = 0;
        if (str_base1 == (char *)0)      /* no space allocated yet */
        {
                if ((str_base1 = (char *)malloc((unsigned)str_length)) == NULL)
                {
                        (void) pfmt(stderr, MM_ERROR, nostring, arnam);
                        exit(1);
                }
                str_top1 = str_base1;
        }


        p = str_top1;
	str_top1 += strlen(fptr->longname) + 2;

	index = bytes_used;
	(void) sprintf(ptr_index1, "%d", index); /* holds digits */
	(void) sprintf(ptr_index, "%-16s", SYMDIRNAME);
	ptr_index[1] = '\0';
	(void) strcat(ptr_index,ptr_index1);


	(void) strcpy( fptr->ar_name, ptr_index);

	bytes_used += strlen(fptr->longname) + 2;

        if (str_top1 > str_base1 + str_length)
        {
                char *old_base = str_base1;

                str_length += BUFSIZ * 2;
                if ((str_base1 = (char *)realloc(str_base1, str_length)) == NULL)
                {
                        (void) pfmt(stderr, MM_ERROR, nogrowstring, arnam);
                                exit(1);
                }
                /*
                * Re-adjust other pointers
                */
                diff = str_base1 - old_base;
                p += diff;
        }

                for (i = 0, s = fptr->longname;
                        i < strlen(fptr->longname) && *s != '\0'; i++)
                {
                        *p++ = *s++;
                }
                *p++ = '/';
		*p++ = '\n';
                str_top1 = p;

	return;
}

static void
writefile()
{
	register ARFILE	*fptr;
	char		buf[ sizeof( struct ar_hdr ) + 1 ];
	char	       	buf11[sizeof( struct ar_hdr) + 1 ];
	register int i;

	mklong_tab();
	mksymtab();

	for (i = 0; signum[i]; i++) /* started writing, cannot interrupt */
                (void) signal(signum[i], SIG_IGN);
	
	/* Unless c flag, print message when archive is produced */
        if (!FLAG('c') && (access (arnam,(mode_t)00) != 0))
                (void) pfmt(stderr, MM_INFO, creating, arnam);

	if ((outfile = fopen( arnam, "w" )) == NULL)
	{
		(void) pfmt( stderr, MM_ERROR, badcreate, arnam, strerror(errno));
		exit(1);
	}

	(void) fwrite( ARMAG, sizeof(char), SARMAG, outfile );

	if ( nsyms )
		writesymtab( outfile ); 

	if (longnames)
	{
		(void) sprintf(buf11, FORMAT, LONGDIRNAME, time(0), (unsigned)0, (unsigned)0, (unsigned)0, (long)long_tab_size, ARFMAG);
		(void) fwrite(buf11, 1, sizeof(struct ar_hdr), outfile);
		(void) fwrite(str_base1, 1, sizeof(char) * (str_top1 - str_base1), outfile);
	}

	for ( fptr = listhead; fptr; fptr = fptr->next )
	{
	   if ( strlen(fptr->longname) <= (unsigned)SNAME-2)
		(void) sprintf( buf, FORMAT, trimslash( fptr->longname), fptr->ar_date, (unsigned)fptr->ar_uid, (unsigned)fptr->ar_gid, (unsigned)fptr->ar_mode, fptr->ar_size, ARFMAG );

	   else
		(void) sprintf( buf, FORMAT, fptr->ar_name, fptr->ar_date, (unsigned)fptr->ar_uid, (unsigned)fptr->ar_gid, (unsigned)fptr->ar_mode, fptr->ar_size, ARFMAG );


		if (fptr->ar_size & 0x1)
			fptr->contents[ fptr->ar_size ] = '\n';

		(void) fwrite( buf, sizeof( struct ar_hdr ), 1, outfile );
		(void) fwrite( fptr->contents, ROUNDUP( fptr->ar_size ), 1, outfile );
	}

	if ( ferror( outfile ))
	{
		(void) pfmt( stderr, MM_ERROR, ":32:Cannot write archive: %s\n",
			strerror(errno));
		(void) unlink( arnam );
		exit(2);
	}

	(void) fclose( outfile );
}

static void
mklong_tab()
{
	ARFILE  *fptr;
	for ( fptr = listhead; fptr; fptr = fptr->next )
        {
		if ( strlen(fptr->longname) >= (unsigned)SNAME-1)
		{
			longnames++;
			savelongname(fptr);
			(void) strcpy( fptr->ar_name, ptr_index);

		}
	}
	if (longnames)
	{
		/* round up table that keeps the long filenames */
		/* while (not a multiple of 4) 			*/
		while ((str_top1 - str_base1) & 0x3) {
			*str_top1++ = '\n';
			}

		long_tab_size = sizeof(char) * (str_top1 - str_base1);
	}
	return;

}

static
void sputl(n,cp)
	long n;
	char *cp;
{
	*cp++ =n/(256*256*256);
	*cp++ =n/(256*256);
	*cp++ =n/(256);
	*cp++ =n&255;
}

static void
search_sym_tab(elf, shdr, scn, mem_offset)
Elf *elf;
Elf32_Shdr *shdr;
Elf_Scn *scn;
long mem_offset;
{
	Elf_Data 	*str_data, *sym_data; /* string table, symbol table */
	Elf_Scn  	*str_scn;
	Elf32_Shdr	*str_shdr;
	int 		no_of_symbols, counter;
	char 		*symname;
	Elf32_Sym 	*p;
	int 		symbol_bind;
       	unsigned int 	index;

	if ((str_scn = elf_getscn(elf,shdr->sh_link)) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, badlibelf, elf_errmsg(-1));
		return;
	}
	if ((str_shdr = elf32_getshdr(str_scn)) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, badlibelf, elf_errmsg(-1));
                return;
        }

	if (str_shdr->sh_type != SHT_STRTAB) 
	{
		(void) pfmt(stderr, MM_ERROR, ":33:No string table for symbol names\n");
		return;	
       	}

	str_data = 0;
	if ((str_data = elf_getdata(str_scn,str_data)) == 0 )
	{
		(void) pfmt(stderr, MM_ERROR,":34:No data in string table\n");
		return;
	}
	if ( str_data->d_size == 0)
	{
		(void) pfmt(stderr, MM_ERROR,":35:No data in string table - size is 0\n");
		return;
       	}
	
	if (shdr->sh_entsize)
		no_of_symbols = shdr->sh_size/shdr->sh_entsize;
	else 
	{
		(void) pfmt(stderr, MM_ERROR,
		":36:A symbol table entry size of zero is invalid!\n");

		return;
	}

	sym_data = 0;
	if ((sym_data = elf_getdata(scn, sym_data)) == NULL)
	{
		(void) pfmt(stderr, MM_ERROR, badlibelf, elf_errmsg(-1));
                return;
        }

	p = (Elf32_Sym *)sym_data->d_buf;
	p++; /* the first symbol table entry must be skipped */

	for (counter = 1; counter<(no_of_symbols); counter++, p++) {

		symbol_bind = ELF32_ST_BIND(p->st_info);
		index = p->st_name;
		symname = (char *)(str_data->d_buf) + index;

		if ( ((symbol_bind == STB_GLOBAL) || (symbol_bind == STB_WEAK)) && ( p->st_shndx != SHN_UNDEF)  ) {
           		if ( !syms_left ) {
		                sym_ptr = (long *) malloc((SYMCHUNK + 1) * sizeof(long));
			         syms_left = SYMCHUNK;
		                if ( nextsym )
               			         *nextsym = (long) sym_ptr;
		                else
               			         symlist = sym_ptr;
		                nextsym = sym_ptr;
           		}

		        sym_ptr = nextsym;
		        nextsym++;
		        syms_left--;
		        nsyms++;
		        *sym_ptr = mem_offset;
			savename(symname); /* put name in the archiver's 
					      symbol table string table */
		}
	}
}
