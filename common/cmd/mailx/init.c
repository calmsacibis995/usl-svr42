/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:init.c	1.4.2.7"
#ident "@(#)init.c	1.18 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * A bunch of global variable declarations lie herein.
 *
 * All global externs are declared in def.h. All variables are initialized
 * here!
 *
 * !!!!!IF YOU CHANGE (OR ADD) IT HERE, DO IT THERE ALSO !!!!!!!!
 *
 */

#include	"def.h"
#include	<grp.h>
#include	<pwd.h>
#include	<utmp.h>
#include	<sys/utsname.h>

struct hdr header[] = {
	{	"",				FALSE },
	{	"Auto-Forward-Count:",		FALSE },
	{	"Auto-Forwarded-From:",		FALSE },
	{	"Content-Length:",		TRUE },
	{	"Content-Type:",		FALSE },
	{	"Date:",			TRUE },
	{	"Default-Options:",		FALSE },
	{	"End-of-Header:",		FALSE },
	{	"From ",			TRUE },
	{	">From ",			TRUE },
	{	"From:",			TRUE },
	{	"MTS-Message-ID:",		FALSE },
	{	"Message-Type:",		FALSE },
	{	"Message-Version:",		FALSE },
	{	"Message-Service:",		TRUE },
	{	"Received:",			FALSE },
	{	"Report-Version:",		FALSE },
	{	"Status:",			FALSE },
	{	"Subject:",			TRUE },
	{	"To:",				TRUE },
	{	">To:",				FALSE },
	{	"Transport-Options:",		FALSE },
	{	"UA-Content-ID:",		FALSE },
	{	"Encoding-Type:",		TRUE },
		/*Dummy place holders for H_DAFWDFROM,*/
		/*H_DTCOPY and H_RECEIVED. Should */
		/* match above first...*/
	{	"Auto-Forwarded-From:",		FALSE },
	{	">To:",				FALSE },
	{	"Received:",			FALSE },
};

int	Fflag = 0;			/* -F option: followup */
int	Hflag = 0;			/* -H option: print headers and exit */
char	*Tflag;				/* -T option: temp file for netnews */
int	UnUUCP = 0;			/* -U flag */
char	**altnames;			/* List of alternate names for user */
int	baud;				/* Output baud rate */
int	cond;				/* Current state of conditional exc. */
int	debug;				/* Debug flag set */
char	*domain;			/* domain name, such as system.com */
struct	message	*dot;			/* Pointer to current message */
int	edit;				/* Indicates editing a file */
char	*editfile;			/* Name of file being edited */
int	escapeokay = 0;			/* Permit ~s even when reading files */
int	exitflg = 0;			/* -e for mail test */
int	fflag = 0;			/* -f: use file */
NODE	*fplist = NOFP;			/* list of FILE*'s maintained by my_fopen/my_fclose */
struct	grouphead *groups[HSHSIZE];	/* Pointer to active groups */
int	hflag;				/* Sequence number for network -h */
char	homedir[PATHSIZE];		/* Name of home directory */
char	*host;				/* Name of system (cluster name or uname) */
struct	ignret	*ignore[HSHSIZE];	/* Pointer to ignored fields */
int	image;				/* File descriptor for image of msg */
FILE	*input;				/* Current command input file */
int	intty;				/* True if standard input a tty */
int	ismail = TRUE;			/* default to program=mail */
FILE	*itf;				/* Input temp file buffer */
int	lexnumber;			/* Number of TNUMBER from scan() */
char	lexstring[STRINGLEN];		/* String from TSTRING, scan() */
int	loading;			/* Loading user definitions */
char	*lockname;			/* named used for locking in /var/mail */
#ifdef	VAR_SPOOL_MAIL
char	maildir[] = "/var/spool/mail/";	/* directory for mail files */
#else
# ifdef	USR_SPOOL_MAIL
char	maildir[] = "/usr/spool/mail/";	/* directory for mail files */
# else
#  ifdef preSVr4
char	maildir[] = "/usr/mail/";	/* directory for mail files */
#  else
char	maildir[] = "/var/mail/";	/* directory for mail files */
#  endif
# endif
#endif
char	mailname[PATHSIZE];		/* Name of /var/mail system mailbox */
off_t	mailsize;			/* Size of system mailbox */
int	maxfiles;			/* Maximum number of open files */
struct	message	*message;		/* The actual message structure */
int	msgCount;			/* Count of messages read in */
char	*mydomname;			/* My login id in user@system form */
gid_t	myegid;				/* User's effective gid */
uid_t	myeuid;				/* User's effective uid */
char	*mylocalname;			/* My login id in user@system.domain form */
char	myname[PATHSIZE];		/* My login id */
pid_t	mypid;				/* Current process id */
gid_t	myrgid;				/* User's real gid */
uid_t	myruid;				/* User's real uid */
int	newsflg = 0;			/* -I option for netnews */
char	noheader;			/* Suprress initial header listing */
int	noreset;			/* String resets suspended */
char	nosrc;				/* Don't source /etc/mail/mailx.rc */
int	numberstack[REGDEP];		/* Stack of regretted numbers */
char	origname[PATHSIZE];		/* Name of mailfile before expansion */
FILE	*otf;				/* Output temp file buffer */
int	outtty;				/* True if standard output a tty */
FILE	*pipef;				/* Pipe file we have opened */
char	*progname;			/* program name (argv[0]) */
char	*prompt = NOSTR;		/* prompt string */
int	_priv;				/* used by PRIV() macro */
int	rcvmode;			/* True if receiving mail */
int	readonly;			/* Will be unable to rewrite file */
int	regretp;			/* Pointer to TOS of regret tokens */
int	regretstack[REGDEP];		/* Stack of regretted tokens */
struct	ignret	*retain[HSHSIZE];	/* Pointer to retained fields */
int	retaincount = 0;		/* Number of retained fields. */
char	*rflag;				/* -r address for network */
int	sawcom;				/* Set after first command */
int	selfsent;			/* User sent self something */
int	senderr;			/* An error while checking */
int	sending;			/* TRUE==>sending mail; FALSE==>printing mail */
char	*sflag;				/* Subject given from non tty */
int	sourcing;			/* Currently reading variant file */
int	space;				/* Current maximum number of messages */
jmp_buf	srbuf;
/*
 * The pointers for the string allocation routines,
 * there are NSPACE independent areas.
 * The first holds STRINGSIZE bytes, the next
 * twice as much, and so on.
 */
struct strings stringdope[NSPACE];
char	*stringstack[REGDEP];		/* Stack of regretted strings */
char	tempEdit[14];			/* ???? */
char	tempMail[14];			/* ???? */
char	tempMesg[14];			/* ???? */
char	tempQuit[14];			/* ???? */
char	tempResid[PATHSIZE];		/* temp file in :saved */
char	tempSet[14];			/* ???? */
char	tempZedit[14];			/* ???? */
char	*tflag;				/* -t use To: fields to get recipients */
uid_t	uid;				/* The invoker's user id */
static struct utimbuf	utimeb;		/* ???? */
struct utimbuf	*utimep = &utimeb;	/* ???? */
struct	var	*variables[HSHSIZE];	/* Pointer to active var list */

#ifdef SVR4ES
eucwidth_t	wp;
int	maxeucw;
#endif

const	char
	appended[] = "[Appended]",
	appendedid[] = ":240",
	ateof[] = ":239:At EOF\n",
	badchdir[] = ":246:Cannot change directory to %s: %s\n",
	badexec[] = ":12:Cannot execute %s: %s\n",
	badopen[] = ":2:Cannot open %s: %s\n",
	badread1[] = ":263:Cannot read: %s\n",
#ifndef APPEND
	badread[] = ":3:Cannot read %s: %s\n",
#endif
	badwrite1[] = ":264:Cannot write: %s\n",
	badwrite[] = ":260:Cannot write %s: %s\n",
	binarysize[] = ":243:%s binary/%ld\n",
	cmdfailed[] = ":262:\"%s\" failed\n",
	errmsg[] = ":248:%s: %s\n",
	failed[] = ":238:%s() failed: %s\n",
	filedothexist[] = ":259:%s: file exists\n",
	forwardbeginid[] = ":512",
	forwardbegin[] = "-------------------- begin forwarded message --------------------",
	forwardendid[] = ":513",
	forwardend[] = "-------------------- end of forwarded message --------------------",
	hasinterrupted[] = ":254:Interrupt\n",
	hasnomail[] = ":141:No mail.\n",
	hasnomailfor[] = ":250:No mail for %s\n",
	heldmsgs[] = ":257:Held %d messages in %s\n",
	heldonemsg[] = ":256:Held 1 message in %s\n",
	illegalmsglist[] = ":252:Illegal use of \"message list\"\n",
	inappropmsg[] = ":255:%d: Inappropriate message\n",
	msgbcc[] = "Bcc: ",
	msgbccid[] = ":268",
	msgcc[] = "Cc: ",
	msgccid[] = ":267",
	msgsubject[] = "Subject: ",
	msgsubjectid[] = ":265",
	msgto[] = "To: ",
	msgtoid[] = ":266",
	newfile[] = "[New file]",
	newfileid[] = ":241",
	newmailarrived[] = ":251:New mail has arrived.\n",
	noapplicmsgs[] = ":253:No applicable messages\n",
	nofieldignored[] = ":244:No fields currently being ignored.\n",
	nofieldretained[] = ":504:No fields currently being retained.\n",
	nohelp[] = ":245:No help just now.\n",
	nomatchingif[] = ":261:\"%s\" without matching \"if\"\n",
	nomem[] = ":10:Out of memory: %s\n",
	textsize[] = ":242:%s %ld/%ld\n",
	toolongtoedit[] = ":258:Too long to edit\n",
	unexpectedEOF[] = ":249:(Unexpected end-of-file).\n",
	usercont[] = "(continue)\n",
	usercontid[] = ":247",
	usercontinue[] = ":247:(continue)\n";
