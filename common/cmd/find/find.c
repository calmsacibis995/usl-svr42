/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* 	Portions Copyright(c) 1988, Sun Microsystems Inc.	*/
/*	All Rights Reserved					*/

#ident	"@(#)find:find.c	4.46.1.11"
#ident  "$Header: find.c 1.2 91/06/26 $"
/***************************************************************************
 * Command : find
 * Inheritable Privileges : P_DACREAD,P_DACWRITE,P_MACREAD,P_MACWRITE,P_COMPAT
 *       Fixed Privileges : None
 *
 * Notes:
 *
 *
 * Rewrite of find program to use nftw(new file tree walk) library function
 * This is intended to be upward compatible to System V release 3.
 * There is one additional feature:
 *	If the last argument to -exec is {} and you specify + rather
 *	than ';', the command will be invoked fewer times with {}
 *	replaced by groups of pathnames. 
 ***************************************************************************/


#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/param.h>
#include <ftw.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/mac.h>
#include <ctype.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>
#include <priv.h>
#include <sys/secsys.h>
#include <sys/time.h>
#include <sys/resource.h>


#define A_DAY		(long)(60*60*24)	/* a day full of seconds */
#define BLKSIZ		512
#define round(x,s)	(((x)+(s)-1)&~((s)-1))
#ifndef FTW_SLN
#define FTW_SLN		7
#endif

/*
 * This is the list of operations
 */
enum Command
{
	PRINT, DEPTH, LOCAL, MOUNT, ATIME, MTIME, CTIME, NEWER,
	NAME, USER, GROUP, INUM, SIZE, LINKS, PERM, EXEC, OK, CPIO, NCPIO,
	TYPE, AND, OR, NOT, LPAREN, RPAREN, CSIZE, VARARGS, FOLOW,
	PRUNE, NOUSER, NOGRP, FSTYPE, LEVEL
};

enum Type
{
	Unary, Id, Num, Str, Exec, Cpio, Op
};

struct Args
{
	char		name[10];
	enum Command	action;
	enum Type	type;
};

/*
 * Except for pathnames, these are the only legal arguments
 */
struct Args commands[] =
{
	"!",		NOT,	Op,
	"(",		LPAREN,	Unary,
	")",		RPAREN,	Unary,
	"-a",		AND,	Op,
	"-atime",	ATIME,	Num,
	"-cpio",	CPIO,	Cpio,
	"-ctime",	CTIME,	Num,
	"-depth",	DEPTH,	Unary,
	"-exec",	EXEC,	Exec,
	"-follow",	FOLOW,  Unary,
	"-group",	GROUP,	Num,
	"-inum",	INUM,	Num,
	"-level",	LEVEL,	Id, 	/* TI/E */
	"-links",	LINKS,	Num,
	"-local",	LOCAL,	Unary,
	"-mount",	MOUNT,	Unary,
	"-mtime",	MTIME,	Num,
	"-name",	NAME,	Str,
	"-ncpio",	NCPIO,  Cpio,
	"-newer",	NEWER,	Str,
	"-o",		OR,	Op,
	"-ok",		OK,	Exec,
	"-perm",	PERM,	Num,
	"-print",	PRINT,	Unary,
	"-size",	SIZE,	Num,
	"-type",	TYPE,	Num,
	"-xdev",	MOUNT,	Unary,
	"-user",	USER,	Num,
	"-prune",	PRUNE,	Unary,
	"-nouser",	NOUSER,	Unary,
	"-nogroup",	NOGRP,	Unary,
	"-fstype",	FSTYPE,	Str,
	0,
};

union Item
{
	struct Node	*np;
	struct Arglist	*vp;
	time_t		t;
	char		*cp;
	char		**ap;
	level_t	    	v;  	/* TI/E - for level comparison */
	long		l;
	int		i;
};

struct Node
{
	struct Node	*next;
	enum Command	action;
	union Item	first;
	union Item	second;
};

/*
 * Prototype variable size arglist buffer
 */

struct Arglist
{
	struct Arglist	*next;
	char		*end;
	char		*nextstr;
	char		**firstvar;
	char		**nextvar;
	char		*arglist[1];
};

static int		walkflags = FTW_CHDIR|FTW_PHYS;
static int		compile();
static int		execute();
static int		doexec();
static char		*getfstype();
static struct Args	*lookup();
static int		ok();
static struct Arglist	*varargs();

static struct Node	*topnode;
static char		*cpio[] = { "cpio", "-o", 0 };
static char		*ncpio[] = { "cpio", "-oc", 0 };
static char		*cpiol[] = { "cpio", "-oL", 0 };
static char		*ncpiol[] = { "cpio", "-ocL", 0 };
static long		now;
static FILE		*output;
static char		*dummyarg = (char*)-1;
static int		lastval;
static int		action = 0;
static int		varsize;
static struct Arglist	*lastlist;

FILE	*cmdopen();
int	cmdclose();
char	*getshell();
extern int	exec();
extern int	nftw();
extern struct	group	*getgruid();
extern time_t	time();
extern char	**environ;
extern int	getrlimit();

static const char
	incstat[] = ":239:Incomplete statement\n",
	badusage[] = ":8:Incorrect usage\n",
	badstat[] = ":5:Cannot access %s: %s\n",
	badoption[] = ":245:Illegal option -- %s\n",
	badoper[] = ":792:Illegal operator: \"%s\"\n",
	badlname[] = ":793:Invalid level name \"%s\"\n";

#define usage() pfmt(stderr,MM_ACTION,":246:Usage: find path-list predicate-list\n")
	
/* Exit Codes */
#define	RET_OK		0	/* success */
#define	RET_USAGE	1	/* incorrect usage */
#define	RET_OPTION	2	/* illegal option */
#define	RET_INSTALL	3	/* system service not installed */
#define	RET_LVL_NM	4	/* invalid level name */
#define	RET_LVL_ID	5	/* invalid level identifier */
#define	RET_TIME	6	/* time() failed */
#define	RET_INCSTAT	7	/* incomplete statement */
#define	RET_STAT	8	/* cannot access/stat */
#define	RET_OPER	9	/* illegal operator */
#define	RET_OPER2	10	/* operand follows operand */
#define	RET_FIND	11	/* cannot find <-user or -group> name */
#define	RET_ARG		12	/* invalid argument */
#define	RET_OCTAL	13	/* not octal */
#define	RET_ACTION	14	/* no action specified */
#define	RET_ABORT	15	/* abort from -ok option */

/*
 * Procedure:     main
 *
 * Restrictions:
                 nftw: none
 */
main(argc, argv)
char *argv[];
{
	register char *cp;
	register int paths;
	time_t time();
	struct rlimit rlimit;
	int fds; 

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:find");

	if (time(&now) == (time_t) -1) 
	{
		pfmt(stderr, MM_ERROR, ":240:time() failed: %s\n",
			strerror(errno));
		exit(RET_TIME);
	}
	if(argc<3)
	{
		pfmt(stderr, MM_ERROR, badusage);
		usage();
		exit(RET_USAGE);
	}
	for(paths=1; cp = argv[paths] ; ++paths)
	{
		if(*cp== '-')
			break;
		else if((*cp=='!' || *cp=='(') && *(cp+1)==0)
			break;
	}
	if(paths == 1) /* no path-list */
	{
		pfmt(stderr, MM_ERROR, badusage);
		usage();
		exit(RET_USAGE);
	}
	output = stdout;
	/* Calculate number of file descriptors that nftw is allowed to use */

	if(getrlimit(RLIMIT_NOFILE, &rlimit)) 
		fds = 4;
	else
		fds = (int) rlimit.rlim_cur / 2;

	/* allocate enough space for the compiler */
	topnode = (struct Node*)malloc(argc*sizeof(struct Node));
	compile(argv+paths,topnode);
	while(--paths)
		if (nftw(*++argv, execute, fds, walkflags)) {
			if(errno == ENAMETOOLONG)
				pfmt(stderr, MM_ERROR, ":1120:Path/File name too long\n");
			else
				pfmt(stderr, MM_ERROR, ":1121:%s %s\n",*argv,strerror(errno));
			}
	/* execute any remaining variable length lists */
	while(lastlist)
	{
		if(lastlist->end != lastlist->nextstr)
		{
			*lastlist->nextvar = 0;
			doexec((char*)0,lastlist->arglist);
		}
		lastlist = lastlist->next;
	}

	/* output not equal to stdout only for obsolete -cpio option */
	if(output != stdout)
		exit(cmdclose(output));

	exit(RET_OK);
}



/*
 * Procedure:     compile
 *
 * Restrictions:
                 getpwnam: none
                 getgrnam: P_MACREAD
                 lvlvalid: P_MACREAD
                 lvlin: P_MACREAD
                 fopen: None
                 fclose: None
                 stat(2): None
 * Notes: Compile the arguments
 */
static int
compile(argv,np)
char **argv;
register struct Node *np;
{
	register char *b;
	register char **av;
	register struct Node *oldnp = 0;
	struct Args *argp;
	char **com;
	int i;
	enum Command wasop = PRINT;
	level_t usrlvl;	    /* TI/E - user's level */
	
	for(av=argv; *av && (argp=lookup(*av)); av++)
	{
		np->next = 0;
		np->action = argp->action;
		np->second.i = 0; 
		if(argp->type==Op)
		{
			if(wasop==NOT || (wasop && np->action!=NOT))
			{
				pfmt(stderr, MM_ERROR,
					":241:Operand follows operand\n");
				exit(RET_OPER2);
			}
			if(np->action!=NOT && oldnp==0)
				goto err;
			wasop=argp->action;
		}
		else
		{
			wasop = PRINT;
			if(argp->type != Unary)
			{
				if(!(b = *++av))
				{
					pfmt(stderr, MM_ERROR, incstat);
					exit(RET_INCSTAT);
				}
				if(argp->type == Num)
				{
					if(*b=='+' || *b=='-')
					{
						np->second.i = *b; 
						b++;
					}
				}
			}
		}
		switch(argp->action)
		{
			case AND:
				continue;
			case NOT:
				break;
			case OR:
				np->first.np = topnode;
				topnode = np;
				oldnp->next = 0;
				break;
			case LPAREN:
			{
				struct Node *save = topnode;
				topnode = np+1;
				i = compile(++av,topnode);
				np->first.np = topnode;
				topnode = save;
				av += i;
				oldnp = np;
				np->next = np+i;
				np += i;
				continue;
			}
			case RPAREN:
				if(oldnp==0)
					goto err;
				oldnp->next = 0;
				return(av-argv);
			case FOLOW:
				walkflags &= ~FTW_PHYS;
				break;
			case MOUNT:
				walkflags |= FTW_MOUNT;
				break;
			case DEPTH:
				walkflags |= FTW_DEPTH;
				break;
			case LOCAL:
				np->first.l = 0;
				np->second.i = '+';
				break;
			case SIZE:
				if(b[strlen(b)-1]=='c')
					np->action = CSIZE;
			case CTIME:
			case MTIME:
			case ATIME:
			case LINKS:
			case INUM:
				np->first.l = atoi(b);
				break;
			case USER:
			case GROUP:
			{
				struct	passwd	*pw;
				struct	group *gr;
				i = -1;

				if(argp->action == USER)
				{
					if((pw=getpwnam(b))!= 0)
						i = (int)pw->pw_uid;
				}
				else
				{
					(void)procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
					if((gr=getgrnam(b))!= 0)
						i = (int)gr->gr_gid;
					(void)procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
				}

				if(i == -1)
				{
					if(!gmatch(b, "[0-9][0-9][0-9]*")
					&& !gmatch(b, "[0-9][0-9]")
					&& !gmatch(b, "[0-9]"))
					{ 
						pfmt(stderr, MM_ERROR,
							":242:Cannot find %s name\n",
							 *av);
						exit(RET_FIND);
					}
					i = atoi(b);
				}
				np->first.l = i;
				break;
			}
			case EXEC:
			case OK:
				walkflags &= ~FTW_CHDIR;
				np->first.ap = av;
				action++;
				while(1)
				{
					if((b= *++av)==0)
					{
						pfmt(stderr, MM_ERROR, incstat);
						exit(RET_INCSTAT);
					}
					if(strcmp(b,";")==0)
					{
						*av = 0;
						break;
					}
					else if(strcmp(b,"{}")==0)
						*av = dummyarg;
					else if(strcmp(b,"+")==0 &&
						av[-1]==dummyarg &&
						np->action==EXEC)
					{
						av[-1] = 0;
						np->first.vp = varargs(np->first.ap);
						np->action = VARARGS;
						break;
					}
				}
				break;
			case NAME:
				np->first.cp = b;
				break;
			case LEVEL: 	/* TI/E - option processing */
				if (lvlproc(MAC_GET, &usrlvl) == -1)
				{
				    if (errno == ENOPKG) {
				        pfmt(stderr, MM_ERROR, badoption, "-level");
				        pfmt(stderr, MM_ERROR,
				        	":794:System service not installed\n");
				        usage();
				        exit(RET_INSTALL);
				    }
				}
				switch (*b)
				{
				case '=':
				case '-':
				case '+':
				    break;
				default:
				    pfmt(stderr, MM_ERROR, badoper, b);
				    exit(RET_OPER);
				}
				np->first.cp = b++; /* skip the operator */
				if (*b == NULL)
				{
				    pfmt(stderr, MM_ERROR, ":795:Invalid argument\n");
				    exit(RET_ARG);
				}

				if (isdigit(*b))    /* must be LID */
				{
				    if (((np->second.v = atol(b)) == LONG_MAX) || (np->second.v == LONG_MIN))
				    {
							pfmt(stderr, MM_ERROR, ":796:Invalid level identifier \"%s\"\n",b);
							exit(RET_LVL_ID);
				    }

				    if (lvlvalid(&np->second.v) == -1)
				    {
						pfmt(stderr, MM_ERROR, badlname, b);
						exit(RET_LVL_NM);
				    }
				} else {    /* alias of fully qualified level name */
				    if (lvlin(b, &np->second.v) == -1)
				    {
							pfmt(stderr, MM_ERROR, badlname, b);
							exit(RET_LVL_NM);
				    }

				}

				(void)procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
				break;
				/* TI/E - end option processing code */
			case PERM:
				for(i=0; *b ; ++b)
				{
					if(*b < '0' || *b >= '8')
					{
						pfmt(stderr, MM_ERROR,
							":243:%s not octal\n",
							*av);
						exit(RET_OCTAL);
					}
					i <<= 3;
					i = i + (*b - '0');
				}
				np->first.l = i;
				break;
			case TYPE:
				i = *b;
				np->first.l = i=='d' ? S_IFDIR :
				    i=='b' ? S_IFBLK :
				    i=='c' ? S_IFCHR :
#ifdef S_IFIFO
				    i=='p' ? S_IFIFO :
#endif
				    i=='f' ? S_IFREG :
#ifdef S_IFLNK
				    i=='l' ? S_IFLNK :
#endif
				    0;
				break;
			case CPIO:
				if (walkflags & FTW_PHYS)
					com = cpio;
				else
					com = cpiol;
				goto common;
			case NCPIO: 	
			{
				FILE *fd;

				if (walkflags & FTW_PHYS)
					com = ncpio;
				else
					com = ncpiol;
			common:
				/* set up cpio */
				if((fd=fopen(b, "w")) == NULL)
				{
					pfmt(stderr, MM_ERROR,
						":148:Cannot create %s: %s\n",
						b, strerror(errno));
					exit(1);
				}
				np->first.l = (long)cmdopen("cpio",com,"w",fd);
				fclose(fd);
				walkflags |= FTW_DEPTH;
				np->action = CPIO;
			}
			case PRINT:
				action++;
				break;
			case NEWER:
			{
				struct stat statb;
				if(stat(b, &statb) < 0)
				{
					pfmt(stderr, MM_ERROR, badstat,
						b, strerror(errno));
					exit(RET_STAT);
				}
				np->first.l = statb.st_mtime;
				np->second.i = '+';
				break;
			}
			case PRUNE:
			case NOUSER:
			case NOGRP:
				break;
			case FSTYPE:
				np->first.cp = b;
				break;
		}
		oldnp = np++;
		oldnp->next = np;
	}
	if((*av) || (wasop))
		goto err;
	else if (action == 0)
		{
			pfmt(stderr, MM_ERROR, ":244:No action specified\n");
			exit(RET_ACTION);
		}
	oldnp->next = 0;
	return(av-argv);
err:	
	pfmt(stderr, MM_ERROR, badoption, *av);
	usage();
	exit(RET_OPTION);
}

/*
 * Procedure:     execute
 *
 * Restrictions:
 *               pfmt: None
 *               strerror: None
 *               lvlfile(2): None
 *               getpwuid: none
 *               getgrgid: P_MACREAD
 *               fprintf: None
 * This is the function that gets executed at each node
 */

static int
execute(name, statb, type, state)
char *name;
struct stat *statb;
struct FTW *state;
{
	register struct Node *np = topnode;
	register int val;
	time_t t;
	register long l;
	char *Fstype;
	int not = 1;
	level_t f_level,            /* TI/E - file level */
				i_level;            /* TI/E - input level */

	if(type==FTW_NS)
	{
		pfmt(stderr, MM_ERROR, badstat, name, strerror(errno));
		return(0);
	}
	else if(type==FTW_DNR)
	{
		pfmt(stderr, MM_ERROR, ":247:Cannot read dir %s: %s\n",
			 name, strerror(errno));
		return(0);
	}
	else if(type==FTW_SLN)
	{
		pfmt(stderr, MM_ERROR,
			":248:Cannot follow symbolic link %s: %s\n",
			 name, strerror(errno));
		return(0);
	}
	while(np)
	{
		switch(np->action)
		{
			case NOT:
				not = !not;
				np = np->next;
				continue;
			case OR:
			case LPAREN:
			{
				struct Node *save = topnode;
				topnode = np->first.np;
				(void)execute(name,statb,type,state);
				val = lastval;
				topnode = save;
				if(np->action==OR)
				{
					if(val)
						return(0);
					val = 1;
				}
				break;
			}
			case LOCAL:
				l = (long)statb->st_dev;
				goto num;
			case TYPE:
				l = (long)statb->st_mode&S_IFMT;
				goto num;
			case LEVEL: 	/* TI/E - level selector */
				if (np->second.v <= 0)	/* INVALID level */
				{
						val = 0;
						break;
				} else
						i_level = np->second.v;

				f_level = statb->st_level;
	
				switch (*(np->first.cp))
				{
				case '=':
						val = lvlequal(&i_level, &f_level);
						break;
				case '-':
						val = lvldom(&i_level, &f_level);
						break;
				case '+':
						val = lvldom(&f_level, &i_level);
						break;
				default:    /* this should never happen */
						pfmt(stderr, MM_ERROR, badoper, np->first.cp);
						exit(RET_OPER);
				}
				if (val == -1)  /* this should never happen */
				{
						pfmt(stderr, MM_ERROR, ":798:Level compare failed on operation: %s\n",
							np->first.cp);

						return(0);
				}
				break;
				/* TI/E - end level selector code */
			case PERM:
				l = (long)statb->st_mode&07777;
				if(np->second.i == '-')
					val = ((l&np->first.l)==np->first.l);
				else
					val = (l == np->first.l);
				break;
			case INUM:
				l = (long)statb->st_ino;
				goto num;
			case NEWER:
				l = statb->st_mtime;
				goto num;
			case ATIME:
				t = statb->st_atime;
				goto days;
			case CTIME:
				t = statb->st_ctime;
				goto days;
			case MTIME:
				t = statb->st_mtime;
			days:
				l = (now-t)/A_DAY;
				goto num;
			case CSIZE:
				l = statb->st_size;
				goto num;
			case SIZE:
				l = round(statb->st_size,BLKSIZ)/BLKSIZ;
				goto num;
			case USER:
				l = (long)statb->st_uid;
				goto num;
			case GROUP:
				l = (long)statb->st_gid;
				goto num;
			case LINKS:
				l = (long)statb->st_nlink;
			num:
				if(np->second.i == '+')
					val = (l > np->first.l);
				else if(np->second.i == '-')
					val = (l < np->first.l);
				else
					val = (l == np->first.l);
				break;
			case OK:
				val = ok(name,np->first.ap);
				break;
			case EXEC:
				val = doexec(name,np->first.ap);
				break;
			case VARARGS:
			{
				register struct Arglist *ap = np->first.vp;
				register char *cp;
				cp = ap->nextstr - (strlen(name)+1);	
				if(cp >= (char*)(ap->nextvar+3))
				{
					/* there is room just copy the name */
					val = 1;
					strcpy(cp,name);
					*ap->nextvar++ = cp;
					ap->nextstr = cp;
				}
				else
				{
					/* no more room, exec command */
					*ap->nextvar++ = name;
					*ap->nextvar = 0;
					val = doexec((char*)0,ap->arglist);
					ap->nextstr = ap->end;
					ap->nextvar = ap->firstvar;
				}
				break;
			}
			case DEPTH:
			case MOUNT:
			case FOLOW:
				val = 1;
				break;
			case NAME:
			{
				val = gmatch(name+state->base,np->first.cp);
				break;
			}
			case PRUNE:
				if (type == FTW_D)
					state->quit = FTW_PRUNE;
				val = 1;
				break;
			case NOUSER:
			{
				val = ((getpwuid(statb->st_uid))== 0);
				break;
			}
			case NOGRP:
			{
				(void)procprivl(CLRPRV,pm_work(P_MACREAD),(priv_t)0);
				val = ((getgrgid(statb->st_gid))== 0);
				(void)procprivl(SETPRV,pm_work(P_MACREAD),(priv_t)0);
				break;
			}
			case FSTYPE:
			{
				if (walkflags & FTW_CHDIR) {
				    switch (type) {
				    case FTW_D:
				    case FTW_DP:
				    case FTW_DNR:
					Fstype = getfstype(".");
					break;
				    default:
					Fstype = getfstype(name+state->base);
					break;
				    }
				} else
				    Fstype = getfstype(name);
				if ( Fstype == NULL )
					val = 0;
				else
					val = ((strcmp(np->first.cp,Fstype))?0:1);
				break;
			}
			case CPIO:
				output = (FILE *)np->first.l;
				fprintf(output, "%s\n", name);
				val = 1;
				break;
			case PRINT:
				fprintf(stdout,"%s\n",name);
				val=1;
				break;
		}
		/* evaluate 'val' and 'not' (exclusive-or)
		 * if no inversion (not == 1), return only when val==0 
		 * (primary not true). Otherwise, invert the primary 
		 * and return when the primary is true. 
		 * 'Lastval' saves the last result (fail or pass) when 
		 * returning back to the calling routine. 
		 */
		if(val^not) {
			lastval = 0;
			return(0);
		}
		lastval = 1;
		not = 1;
		np=np->next;
	}
	return(0);
}

/*
 * Procedure:     ok
 *
 * Restrictions:
 *               fflush: none
 *               pfmt: none
 *               getchar: none
 * code for the -ok option
 */

static int
ok(name,argv)
char *name;
char *argv[];
{
	int c, yes=0;

	fflush(stdout); /* flush output buffer used by -print */
	(void) pfmt(stderr, MM_NOSTD, ":249:< %s ... %s >?   ",*argv , name);
	fflush(stderr);
	if((c=getchar())=='y')
		yes = 1;
	while(c!='\n')
		if(c==EOF)
			exit(RET_ABORT);
		else
			c = getchar();
	return(yes? doexec(name,argv): 0);
}

/*
 * Procedure:     doexec
 *
 * Restrictions:
 *               execvp(2): none
 * execute argv with {} replaced by name
 */

static int
doexec(name,argv)
char *name;
register char *argv[];
{
	register char *cp;
	register char **av = argv;
	int r = 0;
	pid_t pid;

	fflush(stdout);	/* flush output buffer used by -print */
	if(name)
	{
		while (cp= *av++)
		{
			if(cp==dummyarg)
				av[-1] = name;
		}
	}
	if(pid = fork())
	{
		while(wait(&r) != pid);
	}
	else /*child*/
	{
	  int privid;
	  privid = secsys(ES_PRVID, 0);

	  if (privid < 0 ||  privid != getuid()) 
	    procprivl(CLRPRV, pm_max(P_ALLPRIVS), 0);
	  
	  execvp(argv[0], argv);
	  exit(1);	/* child */
	}
	return(!r);
}


/*
 *  Table lookup routine
 */
static struct Args*
lookup(word)
register char *word;
{
	register struct Args *argp = commands;
	register int second;
	if(word==0 || *word==0)
		return(0);
	second = word[1];
	while(*argp->name)
	{
		if(second == argp->name[1] && strcmp(word,argp->name)==0)
			return(argp);
		argp++;
	}
	return(0);
}


/*
 * Get space for variable length argument list
 */

static struct Arglist*
varargs(com)
char **com;
{
	register struct Arglist *ap;
	register int n;
	register char **ep;
	if(varsize==0)
	{
		n = 2*sizeof(char**);
		for(ep=environ; *ep; ep++)
			n += (strlen(*ep)+sizeof(char**) + 1);
		varsize = sizeof(struct Arglist)+ARG_MAX-PATH_MAX-n-1;
	}
	ap = (struct Arglist*)malloc(varsize+1);
	ap->end = (char*)ap + varsize;
	ap->nextstr = ap->end;
	ap->nextvar = ap->arglist;
	while( *ap->nextvar++ = *com++);
	ap->nextvar--;
	ap->firstvar = ap->nextvar;
	ap->next = lastlist;
	lastlist = ap;
	return(ap);
}

/*
 * Procedure:     getfstype
 *
 * Restrictions:
 *         statvfs(2): None
 *               pfmt: None
 *           strerror: None
 * Returns the file system type for the file system "file" lives in.
 */

static char *
getfstype(file)
	char *file;
{
	statvfs_t sbuf;
	static char mnttype[FSTYPSZ];

	if (statvfs(file, &sbuf) != 0) 
	{
		pfmt(stderr, MM_ERROR, ":250:statvfs() on %s failed: %s\n",
			file, strerror(errno));
		return(NULL);
	}

	strcpy(mnttype, sbuf.f_basetype);
	return(mnttype);
}

/*
 * Procedure:     cmdopen
 *
 * Restrictions:
                 dup2: none
                 execvp(2): P_MACREAD
                 execv(2): P_MACREAD
 * Notes:
 * filter command support
 * fork and exec cmd(argv) according to mode:
 *
 *	"r"	with fp as stdin of cmd (default stdin), cmd stdout returned
 *	"w"	with fp as stdout of cmd (default stdout), cmd stdin returned
 */

#define CMDERR	((1<<8)-1)	/* command error exit code		*/
#define MAXCMDS	8		/* max # simultaneous cmdopen()'s	*/

static struct			/* info for each cmdopen()		*/
{
	FILE	*fp;		/* returned by cmdopen()		*/
	pid_t	pid;		/* pid used by cmdopen()		*/
} cmdproc[MAXCMDS];

FILE*
cmdopen(cmd, argv, mode, fp)
char	*cmd;
char	**argv;
char	*mode;
FILE	*fp;
{
	register int	proc;
	register int	cmdfd;
	register int	usrfd;
	int		pio[2];

	switch (*mode)
	{
	case 'r':
		cmdfd = 1;
		usrfd = 0;
		break;
	case 'w':
		cmdfd = 0;
		usrfd = 1;
		break;
	default:
		return(0);
	}
	for (proc = 0; proc < MAXCMDS; proc++)
		if (!cmdproc[proc].fp)
			 break;
	if (proc >= MAXCMDS) return(0);
	if (pipe(pio)) return(0);
	switch (cmdproc[proc].pid = fork())
	{
	case -1:
		return(0);
	case 0:
		if (fp && fileno(fp) != usrfd)
		{
			(void)close(usrfd);
			if (dup2(fileno(fp), usrfd) != usrfd) 
				_exit(CMDERR);
			(void)close(fileno(fp));
		}
		(void)close(cmdfd);
		if (dup2(pio[cmdfd], cmdfd) != cmdfd) 
			_exit(CMDERR);
		(void)close(pio[cmdfd]);
		(void)close(pio[usrfd]);
		
		{
		int privid;
		privid = secsys(ES_PRVID, 0);

		if (privid < 0 ||  privid != getuid()) 
			procprivl(CLRPRV, pm_max(P_ALLPRIVS), 0);
		}

	
		execvp(cmd, argv);
		if (errno == ENOEXEC)
		{
			register char	**p;
			char		**v;

			/*
			 * assume cmd is a shell script
			 */

			p = argv;
			while (*p++);
			if (v = (char**)malloc((p - argv + 1) * sizeof(char**)))
			{
				p = v;
				*p++ = cmd;
				if (*argv) argv++;
				while (*p++ = *argv++);

				execv(getshell(), v);
			}
		}
		_exit(CMDERR);
		/*NOTREACHED*/
	default:
		(void)close(pio[cmdfd]);
		cmdproc[proc].fp = fdopen(pio[usrfd], mode);
		return(cmdproc[proc].fp);
	}
}


/*
 * Procedure:     cmdclose
 *
 * Restrictions:
 *               fclose: none
 *
 * close a stream opened by cmdopen()
 * -1 returned if cmdopen() had a problem
 * otherwise exit() status of command is returned
 *
 * Note: Since the -cpio option is obsolete, exit codes for 
 *	 its operation failures is not defined.
 */

int
cmdclose(fp)
FILE	*fp;
{
	register int	i;
	register pid_t	p, pid;
	int		status;

	for (i = 0; i < MAXCMDS; i++)
		if (fp == cmdproc[i].fp) break;
	if (i >= MAXCMDS) 
		return(-1);
	(void)fclose(fp);
	cmdproc[i].fp = 0;
	pid = cmdproc[i].pid;
	while ((p = wait(&status)) != pid && p != (pid_t)-1);
	if (p == pid)
	{
		status = (status >> 8) & CMDERR;
		if (status == CMDERR) 
			status = -1;
	}
	else 
		status = -1;
	return(status);
}

/*
 * Procedure:     getshell
 *
 * Restrictions:
 *                access(2): none
 *
 * return pointer to the full path name of the shell
 *
 * SHELL is read from the environment and must start with /
 *
 * if set-uid or set-gid then the executable and its containing
 * directory must not be writable by the real user
 *
 * /sbin/sh is returned by default
 */
extern char	*getenv();
extern char	*strrchr();


char*
getshell()
{
	register char	*s;
	register char	*sh;
	register uid_t	u;
	register int	j;

	if ((sh = getenv("SHELL")) && *sh == '/')
	{
		if (u = getuid())
		{
			if ((u != geteuid() || getgid() != getegid())
			   && !access(sh, 2))
				goto defshell;
			s = strrchr(sh, '/');
			*s = 0;
			j = access(sh, 2);
			*s = '/';
			if (!j) goto defshell;
		}
		return(sh);
	}
 defshell:
	return("/sbin/sh");
}
