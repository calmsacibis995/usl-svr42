/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/mail.h	1.18.2.4"
#ident "@(#)mail.h	2.59 'attmail mail(1) command'"
 /*
  * All global externs defined here. All variables are initialized
  * in init.c
  *
  * !!!!!IF YOU CHANGE (OR ADD) IT HERE, DO IT THERE ALSO !!!!!!!!
  *
  */
#include "stdc.h"
#include "libmail.h"
#include "re/re.h"

#define CHILD		0
#define SAME		0

#define	BELL		07

/* return values from system calls */
#define CERROR		-1
#define CSUCCESS	0

#define TRUE	1
#define FALSE	0

typedef enum { TAIL, HEAD } WhereHdr;

#define	REAL	1
#define DEFAULT	0

/* findSurg() return values */
typedef enum {
	NOMATCH = -1, DELIVER, POSTDELIVER, DENY, TRANSLATE
} FindSurgRet;

/* sendsurg() return values */
typedef enum {
	FAILURE = 'f', CONTINUE = 'c', SUCCESS = 's'
} SendSurgRet;

#define	HDRSIZ	256	/* maximum length of header line */

#define E_FLGE	1	/* flge error */
#define	E_REMOTE 1	/* unknown remote */
#define E_FILE	2	/* file error */
#define E_SPACE	3	/* no space */
#define E_FRWD	4	/* cannot forward */
#define E_SYNTAX 5      /* syntax error */
#define E_FRWL	6	/* forwarding loop */
#define E_SNDR  7	/* invalid sender */
#define E_USER  8	/* invalid user */
#define E_FROM  9	/* too many From lines */
#define E_PERM  10 	/* bad permissions */
#define E_MBOX  11 	/* mbox problem */
#define E_TMP	12 	/* temporary file problem */
#define E_DEAD  13 	/* Cannot create dead.letter */
#define E_UNBND 14 	/* Unbounded forwarding */
#define E_LOCK  15 	/* cannot create lock file */
#define E_GROUP	16	/* no group id of 'mail' */
#define	E_MEM	17	/* malloc failure */
#define E_FORK	18	/* could not fork */
#define	E_PIPE	19	/* could not pipe */
#define	E_OWNR	20	/* invoker does not own mailfile */
#define	E_DENY	21	/* permission denied by mailsurr file */
#define E_SURG	22	/* surrogate command failed */
#define E_NBNRY	23	/* disallowed from sending binary to remote */
#define E_TRAN	24	/* translation command failed */

#define H_FROM			1	/* "From "			*/
#define H_RFROM			2	/* "From ... remote from ..."	*/
#define H_FROM1			3	/* ">From "			*/
#define H_MVERS			4	/* "Message-Version:"		*/
#define H_RVERS			5	/* "Report-Version:"		*/
#define H_TCOPY			6	/* ">To:"			*/
#define H_DATE			7	/* "Date:"			*/
#define H_ODATE			8	/* "Original-Date:"		*/
#define H_FROM2			9	/* "From:"			*/
#define H_RECEIVED		10	/* "Received:"			*/
#define H_AFWDFROM		11	/* "Auto-Forwarded-From:"	*/
#define H_OAFWDFROM		12	/* "Original-Auto-Forwarded-From:"*/
#define H_AFWDCNT		13	/* "Auto-Forward-Count:"	*/
#define H_MTSID			14	/* "MTS-Message-ID:"		*/
#define H_CMTSID		15	/* "Confirming-MTS-Message-ID:"	*/
#define H_UAID			16	/* "UA-Content-ID:"		*/
#define H_CUAID			17	/* "Confirming-UA-Content-ID:"	*/
#define H_DLVRTO		18	/* "Delivered-To:"		*/
#define H_NDLVRTO		19	/* "Not-Delivered-To:"		*/
#define H_ENROUTE		20	/* "En-Route-To:"		*/
#define H_EOH			21	/* "End-of-Header:"		*/
#define H_EVERS			22	/* "EMail-Version:"		*/
#define H_PHONE			23	/* "Phone:"			*/
#define H_FAXPHONE		24	/* "Fax-Phone:"			*/
#define H_SUBJ			25	/* "Subject:"			*/
#define H_OSUBJ			26	/* "Original-Subject:"		*/
#define H_UAMID			27	/* "UA-Message-ID:"		*/
#define H_TO			28	/* "To:"			*/
#define H_REPLYTO		29	/* "Reply-To:"			*/
#define H_CC			30	/* "Cc:"			*/
#define H_BCC			31	/* "Bcc:"			*/
#define H_EOP			32	/* "End-of-Protocol:"		*/
#define H_MTYPE			33	/* "Message-Type:"		*/
#define H_MSVC			34	/* "Message-Service:"		*/
#define H_OMSVC			35	/* "Original-Message-Service:"	*/
#define H_DEFOPTS		36	/* "Default-Options:"		*/
#define H_TROPTS		37	/* "Transport-Options:"		*/
#define H_ERRTO			38	/* "Errors-To:"			*/
#define H_CLEN			39	/* "Content-Length:"		*/
#define H_CTYPE			40	/* "Content-Type:"		*/
#define H_ENCDTYPE		41	/* "Encoding-Type:"		*/
#define H_SENDER		42	/* "Sender:"			*/
#define H_PRECEDENCE		43	/* "Precedence:"		*/
/* The following three must be last */
#define H_NAMEVALUE		44	/* "Name-Value:"		*/
#define H_CONT			45	/* "Continue:"			*/
#define	H_MAX			46	/* The number of recognized header lines */

/* MTA Transport Options */
#define	DELIVERY	001
#define	NODELIVERY	002
#define	REPORT		010
#define	RETURN		020
#define	IGNORE		040

typedef enum	/* copylet() and mcopylet() flags */
{
	REMOTE,		/* remote mail, add rmtmsg */
	ORDINARY,	/* local mail delivery */
	ZAP,		/* zap header and trailing empty line */
	TTY		/* suppress non-text to tty */
} CopyLetFlags;

typedef enum		/* Content-Type: */
{
	C_Text,		/*     7-bit ASCII */
	C_GText,	/*     Printable in current locale: generic-text */
	C_Binary	/*     has non-printable characters */
} t_Content;

#define	LSIZE		BUFSIZ		/* maximum size of a line */
#define	MAXLET		1000		/* maximum number of letters */
#define FWRDLEVELS	20		/* maximum number of forwards */
#define FWRDBUFSIZE	1024		/* maximum size of Forward to line, see mail(1) */
#ifdef FILENAME_MAX
# define MAXFILENAME	FILENAME_MAX	/* max length of a filename */
#else
# define MAXFILENAME	1024		/* max length of a filename */
#endif
#define DEADPERM	0600		/* permissions of dead.letter */
#define DEADMASK	077		/* permissions of dead.letter */
#define MAILMASK	07		/* normal umask for mail */
#define RE_NBRAK	9		/* number of () referenced in regular expression library */
#define RE_OFFSET	1		/* does match[0] or match[1] refer to 1st () */
#define BATCH_STRING	(-2)		/* Translation command is replacement string */
#define BATCH_OFF	(-1)		/* Translation,Delivery,etc. command is unbatched */
#define SURG_RC_DEF	(-1003)		/* return code if no surrogate run */
#define SURG_RC_FORK	(-1002)		/* return code if surrogate fork failed */
#define SURG_RC_ERR	(-1001)		/* return code if surrogates failed */

#ifdef DEBUG
#  define PIPER		"./mail_pipe"
#  define MAILSURR	"./Nmailsurr"
#  define CMAILSURR	"./Cmailsurr"
#  define TMAILSURR	"./Tmailsurr"
#  define MAILCNFG	"./Nmailcnfg"
#else
#  define PIPER		"/usr/lib/mail/mail_pipe"
#  define MAILSURR	"/etc/mail/mailsurr"
#  define CMAILSURR	"/etc/mail/Cmailsurr"
#  define TMAILSURR	"/etc/mail/Tmailsurr"
#  define MAILCNFG	"/etc/mail/mailcnfg"
#endif

/* All temporary files are created as a Tmpfile. */
/* They are also kept on a global list. */
typedef struct Tmpfile Tmpfile;
struct Tmpfile {
    char *lettmp;		/* pointer to tmp filename */
    FILE *tmpf;			/* file pointer for temporary files */
    Tmpfile *next;		/* linked list of temp files for cleanup */
};

/* All known header types have known tags with known spellings. */
/* Whether they are displayed by default is also known per header type. */
typedef struct {
	const char *tag;
	int default_display;
} Hdr;

/* Headers are kept in a doubly-linked list of the headers of that particular type. */
/* In addition, each header field may have additional continuation lines. */
typedef struct Hdrs Hdrs;
struct Hdrs {
	Hdrs		*next;	/* Link pointer in doubly-linked list */
	Hdrs		*prev;	/* Link pointer in doubly-linked list */
	Hdrs		*cont;	/* Continuation lines */
	const char	*name;	/* The name before the ":" */
	char		*value;	/* The info after the ":" */
	int		hdrtype;/* type of header, such as H_FROM */
};

/* Each Let represents one letter in a mailbox. */
typedef struct {
	long		adr;		/* offset in mailfile of letter n */
	char		change;		/* disposition status of letter n */
	t_Content	binflag;	/* content ==> text, generic-text, binary. */
					/* This is determined INDEPENDENTLY of what
					 * the Content-type, if present, says...
					 */
	char		*encoding_type;	/* locale of letter if Generic-Text. */
} Let;

/* The headers for a given message or letter are kept in Hdrinfo. */
typedef struct {
    Hdrs *hdrhead;		/* first header in list */
    Hdrs *hdrtail;		/* last header in list */
    Hdrs *hdrs[H_MAX];		/* header information */
    int orig_aff;		/* orig. msg. contained H_AFWDFROM lines */
    int orig_rcv;		/* orig. msg. contained H_RECEIVED lines */
    int orig_tcopy;		/* orig. msg. contained H_TCOPY lines */
    int affcnt;			/* number of H_AFWDFROM lines */
    int fnuhdrtype;		/* type of first non-UNIX header line */
    Hdrs *last_hdr;		/* last header seen when reading message */
} Hdrinfo;

/* All information about a mailbox is kept in Letinfo. */
typedef struct {
    Let	let[MAXLET];		/* where messages to be read are kept */
    Hdrinfo hdrinfo;		/* the headers */
    int	nlet;			/* current number of letters in mailfile */
    int	onlet;			/* number of letters in mailfile at startup*/
    int	changed;		/* > 0 says mailfile has changed */
    Tmpfile tmpfile;		/* Visible temporary file */
} Letinfo;

/* A recipient is kept in a list of Recip's, pointed to by a Reciplist */
typedef struct Recip Recip;
struct Recip {
	Recip	*next;		/* Recips are kept in a chain */
	string	*name;		/* the name for this recipient */
	string	*cmdl;		/* expanded surrogate command kept here */
	string	*cmdr;		/* and right side for batched commands */
	Recip	*parent;	/* name translated from */
	string	*SURRcmd;	/* saved surrogate command after CONTINUE */
	string	*SURRoutput;	/* saved surrogate output after CONTINUE */
	int	SURRrc;		/* saved return code after CONTINUE */
	int	accepted;	/* Accept seen */
	int	local;		/* is the user name local? */
	uid_t	useruid;	/* the local uid if so */
	int	translated;	/* Has the name been translated? */
	int	fullyresolved;	/* A translation has completely translated the name */
	int	lastsurrogate;	/* the last surrogate this recipient was on before translation */
};

/* Each Reciplist is a queue on which recipients are placed. */
/* There is one of these for each surrogate command, plus a few. */
typedef struct {
	Recip *last_recip;	/* The last recipient on the list. */
	Recip recip_list;	/* A null recipient to be the head of the list. */
} Reciplist;

typedef enum {
	Msg_msg, Msg_forward, Msg_deliv, Msg_nondeliv
} Msg_type;

typedef struct Msg Msg;
struct Msg {
    t_Content binflag;		/* text, generic-text or binary? */
    int delivopts;		/* /delivery/nodelivery /report/return/ignore */
    Hdrinfo hdrinfo;		/* the headers */
    long msgsize;		/* size of message file */
    string *orig;		/* originator of message: Rpath or forwarder */
    Msg *parent;		/* associated message for Forwarded and *Delivery */
    Reciplist *preciplist;	/* recipient lists: 0..surr_len+local+success+failed+temps */
    int ret_on_error;		/* send return mail on error? 0->no, 1->yes */
    string *Rpath;		/* return path to sender of message == sys!...!user or user */
    int surg_rc;		/* exit code from surrogate command */
    FILE *SURRerrfile;		/* file pointer for errors from surrogate entries */
    FILE *SURRoutfile;		/* file pointer for output from surrogate entries */
    string *SURRcmd;		/* the surrogate command run */
    Tmpfile tmpfile;		/* Visible temporary file */
    Tmpfile SURRinput;		/* Temp file for input to surrogates */
    Msg_type type;		/* type of message: Regular, Forwarded, Delivery, Nondelivery */
    Msg *errmsg;		/* error message being built up for return */
};

/* numbers added to surr_len to access Msg.preciplist */
#define RECIPS_LOCAL	0
#define RECIPS_SUCCESS	1
#define RECIPS_FAILURE	2
#define RECIPS_TEMP	3
#define RECIPS_TEMP2	4
#define RECIPS_TEMP3	5
#define RECIPS_DONE	6
#define RECIPS_MAX	7
#define recips_exist(pmsg, i)	(pmsg->preciplist[i].recip_list.next != 0)
#define recips_head(pmsg, i)	(&pmsg->preciplist[i].recip_list)

/* The types of commands which a surrogate file may contain. */
typedef enum
{
    t_eof, t_transport = '<',
    t_accept_name = 'a', t_deny_name = 'd',
    t_translate = 't', t_postprocess = '>',
    t_quit = 'q', t_error = 'e'
} t_surrtype;

/* permit some values in struct t_surrfile to have their space shared */
typedef union {
    string *_cmd_left;
    string *_deny_msg;
} _t_sf_union;

typedef union {
    int _nowait4postprocess;
    int _quit_translate;
} _t_sf_union2;

/* The surrogate file is compiled into an array of t_surrfile. */
typedef struct t_surrfile
{
    string *orig_pattern;	/* originator's regular expression */
    re_re *orig_regex;		/* compiled version */
    int orig_nbra;		/* number of () pairs in regular expression */

    string *recip_pattern;	/* recipient's regular expression */
    re_re *recip_regex;		/* compiled version */
    int recip_nbra;		/* number of () pairs in regular expression */

    t_surrtype surr_type;	/* the type of the command string */

				/* Which surrogate commands use this information?	*/
    char *statlist;		/* transport	translate				*/
    _t_sf_union _t_sf_union;
      /* _cmd_left */		/* transport	translate	postprocess		*/
      /* _deny_msg */		/*						deny	*/
    string *cmd_right;		/* transport	translate	postprocess		*/
    int batchsize;		/* transport	translate	postprocess		*/
    _t_sf_union2 _t_sf_union2;
      /* _nowait4postprocess */	/*			 	postprocess		*/
      /* _quit_translate */	/*		translate				*/
    int fullyresolved;		/*		translate				*/
} t_surrfile;

/* Share the space for these through a union. */
#define cmd_left		_t_sf_union._cmd_left
#define deny_msg		_t_sf_union._deny_msg
#define nowait4postprocess	_t_sf_union2._nowait4postprocess
#define quit_translate		_t_sf_union2._quit_translate

extern	void	Dout ARGS((const char *subname, int level, const char *fmt, ...));
extern	void	Tout ARGS((const char *subname, const char *msg, ...));
extern	void	add_retmail ARGS((Msg *pmsg, int wherefrom, int rc));
extern	int	add_recip ARGS((Msg *pmsg, char *name, int checkdups, Recip *parent, int fullyresolved, int stripbangs, int wherefrom, int wheretemp, int whereto));
extern	int	areforwarding ARGS((char *fwrdfile, char *fwrdbuf, unsigned n));
extern	void	cat ARGS((char*, const char*, const char*));
extern	int	ckdlivopts ARGS((Msg *pmsg));
extern	void	cksaved ARGS((char *user));
extern	int	cksvdir ARGS((void));
extern	SendSurgRet	cksurg_rc ARGS((int surr_num, int rc));
extern	void	cleansurrinfo ARGS((Msg *pmsg));
extern	void	clr_hdrinfo ARGS((Hdrinfo *phdrinfo));
extern	string	*cmdexpand ARGS((Msg *pmsg, Recip *r, string *instr, char **lbraslist, char **lbraelist, string *outstr));
extern	void	copyback ARGS((Letinfo *pletinfo));
extern	int	copylet ARGS((Letinfo *pletinfo, int letnum, FILE *f, CopyLetFlags type, int pflg, int Pflg));
extern	void	copymt ARGS((Letinfo *pletinfo, FILE *f1, FILE *f2));
extern	void	createmf ARGS((uid_t uid, char *file));
extern	void	del_Hdrs ARGS((Hdrs *old));
extern	void	del_Msg ARGS((Msg *old));
extern	void	del_Recip ARGS((Recip *plist));
extern	void	delete ARGS((int));
extern	int	dlock ARGS((char *mailfile, int sending));
extern	void	doeopt ARGS((void));
extern	void	doFopt ARGS((void));
extern	void	done ARGS((int));
extern	FILE	*doopen ARGS((char *file, char *type, int errnum));
extern	int	dowait ARGS((pid_t pidval));
extern	void	dumpaff ARGS((Hdrinfo *phdrinfo, CopyLetFlags type, int htype, int *didafflines, int *suppress, FILE *f, int Pflg));
extern	void	dumprcv ARGS((Hdrinfo *phdrinfo, CopyLetFlags type, int htype, int *didrcvlines, int *suppress, FILE *f, int Pflg));
extern	void	errmsg ARGS((int error_value, const char *error_message, ...));
extern	void	fini_Letinfo ARGS((Letinfo *pletinfo));
extern	void	fini_Msg ARGS((Msg *pmsg));
extern	void	fini_Reciplist ARGS((Reciplist *list));
extern	void	fini_Tmpfile ARGS((Tmpfile *ptmpfile));
extern	void	gendeliv ARGS((Msg *parent, int wherefrom));
extern	char	*getarg ARGS((char *s, char *ret));
extern	int	getcomment ARGS((char *s, char *q));
extern	Hdrs	*getlasthdr ARGS((Hdrs *hdr));
extern	int	gethead ARGS((Letinfo *pletinfo, int current, int all));
extern	int	getline ARGS((char *ptr2line, int max, FILE *f));
extern	int	getnumbr ARGS((Letinfo *pletinfo, const char *s));
extern	int	getsurr ARGS((FILE *fp, string *buf, int firstfield));
extern	int	goback ARGS((Letinfo *pletinfo, int letnum, char *others, int usefrom, int readtty, int sendmsg));
extern	void	init ARGS((void));
extern	void	init_altenviron ARGS((void));
extern	void	init_Hdrinfo ARGS((Hdrinfo *phdrinfo));
extern	void	init_Letinfo ARGS((Letinfo *pletinfo));
extern	void	init_Msg ARGS((Msg *pmsg));
extern	void	init_Reciplist ARGS((Reciplist *list));
extern	void	init_Tmpfile ARGS((Tmpfile *ptmpfile));
extern	void	init_retmail ARGS((Msg *parent, Msg *pmsg, int rc));
extern	void	initsurrfile ARGS((void));
extern	int	isheader ARGS((char *lp, int *ctfp, int sending, int fnuhdrtype));
extern	int	isit ARGS((char *lp, int type));
extern	t_Content	istext ARGS((unsigned char *s, int size, t_Content cur));
extern	int	legal ARGS((char *file));
extern	int	lock ARGS((char *user, int sending));
extern	int	madd_recip ARGS((Msg *pmsg, char *name, int checkdups, Recip *parent, int fullyresolved, int stripbangs, int wherefrom, int wheretemp, int whereto));
extern	re_re	*mailcompile ARGS((string *pattern, int *retnbra));
extern	int	matchsurr ARGS((string *lname, re_re *pattern, char **lbraslist, char **lbraelist, int nbra));
extern	int	maxbatchsize ARGS((string *B));
extern	int	mcopylet ARGS((Msg *pmsg, FILE *f, CopyLetFlags remote));
extern	void	mkdate ARGS((char *datestring));
extern	void	mkdead ARGS((Msg *pmsg));
extern	void	mktmp ARGS((Tmpfile *ptmpfile));
extern	char	**msetup_exec ARGS((Msg *pmsg, int whereexec));
extern	char	*mta_ercode ARGS((int));
extern	Hdrs	*new_Hdrs ARGS((int hdrtype, const char *name, const char *val));
extern	Msg	*new_Msg ARGS(());
extern	Recip	*new_Recip ARGS((char *name, Recip *parent, int fullyresolved));
extern	int	nw_sendlist ARGS((Msg *pmsg));
extern	void	oldforwarding ARGS((char*));
extern	int	parse ARGS((int argc, char **argv, int *parse_err));
extern	int	pckaffspot ARGS((Hdrinfo *phdrinfo, int));
extern	int	pckrcvspot ARGS((Hdrinfo *phdrinfo));
extern	void	pickFrom ARGS((char *lineptr, string **fromU, string **fromS));
extern	void	pipeletter ARGS((Msg *pmsg, int whereexec, int nowait4process));
extern	void	pophdrlist ARGS((Hdrinfo *phdrinfo, Hdrs *hdr2rm));
extern	int	printhdr ARGS((int type, int hdrtype, Hdrs *hptr, FILE *fp, int Pflg));
extern	void	printmail ARGS((void));
extern	Recip	*recip_parent ARGS((Recip *r));
extern	void	retmail ARGS((Msg *parent, int wherefrom, int rc, const char *fmt, ...));
extern	void	savdead ARGS((void));
extern	void	save_a_hdr ARGS((Hdrinfo *phdrinfo, const char *val, int hdrtype, const char *name));
extern	void	save_mta_hdr ARGS((Hdrinfo *phdrinfo, const char *val, int hdrtype, const char *name));
extern	void	save_cont_hdr ARGS((Hdrinfo *phdrinfo, const char *val));
extern	void	save_a_txthdr ARGS((Hdrinfo *phdrinfo, char *val, int hdrtype));
extern	int	sel_disp ARGS((CopyLetFlags type, int hdrtype, const char *s, int Pflg));
extern	void	send2accept ARGS((Msg *pmsg, int surr_num));
extern	void	send2clean ARGS((Msg *pmsg, int osurr_num, int nsurr_num));
extern	void	send2d_p ARGS((Msg *pmsg, int surr_num));
extern	void	send2deliv ARGS((Msg *pmsg, int surr_num));
extern	void	send2deny ARGS((Msg *pmsg, int surr_num));
extern	void	send2exec ARGS((Msg *pmsg, int whereexec, int nowait4process));
extern	Recip	*send2findleft ARGS((Msg *pmsg, int wherefrom, int whereexec, int surr_num, char **lbraslist, char **lbraelist, int nbra));
extern	SendSurgRet	send2findright ARGS((Msg *pmsg, int batchsize, Recip *curmsg, int wherefrom, int whereexec, int wheretemp, int surr_num, char **lbraslist, char **lbraelist, char *cmdname, const char *pn, SendSurgRet execcmd, int ckrc, int nowait4process));
extern	void	send2local ARGS((Msg *pmsg, int surr_num, int level));
extern	void	send2move ARGS((Msg *pmsg, int osurr_num, int nsurr_num));
extern	void	send2bmvrecip ARGS((Msg *pmsg, int osurr_num, int nsurr_num));
extern	void	send2mvrecip ARGS((Msg *pmsg, int osurr_num, int nsurr_num));
extern	void	send2post ARGS((Msg *pmsg, int wherefrom, t_surrtype surr_type));
extern	void	send2quit ARGS((Msg *pmsg, int surr_num));
extern	int	send2tran ARGS((Msg *pmsg, int surr_num));
extern	int	sendlist ARGS((Msg *pmsg, int level));
extern	void	sendmail ARGS((int argc, char **argv));
extern	void	send_retmail ARGS((Msg *pmsg));
extern	void	setletr ARGS((Letinfo *pletinfo, int letter, int status));
extern	int	setmailfile ARGS((void));
extern	void	(*setsig ARGS((int i, void(*f)())))();
extern	void	setsurg_bw ARGS((string *st, int *pbatchsize, int *presolved));
extern	char	*setsurg_rc ARGS((string *st, int defreal, t_surrtype, int *pbatchsize, int *pfulltrans, int *pquit_translate, int *premove_exclams));
extern	char	**setup_exec ARGS((char*));
extern	long	sizeheader ARGS((Msg *pmsg));
extern	int	splitname ARGS((char *buf, unsigned n, char *name, char *ins));
extern	void	stamp ARGS((struct utimbuf *utimep));
extern	int	systm ARGS((char *s));
extern	string	*tokdef ARGS((string *fld, string *tok, char *name));
extern	void	unlock ARGS((void));
extern	int	validmsg ARGS((Letinfo *pletinfo, int));
extern	void	vDout ARGS((const char *subname, int level, const char *fmt, va_list args));

extern	const char Binary[];	/* "binary" */
extern	const char GenericText[];/* "generic-text" */
extern	const char Text[];	/* "text" */
extern	char	**altenviron;	/* alternate environment for surrogate commands */
extern	char	dbgfname[20];	/* name of file for debugging output */
extern	FILE	*dbgfp;		/* FILE* for debugging output */
extern	int	debug;		/* Controls debugging level. 0 ==> no debugging */
extern	const char *errlist[];	/* Words to go along with E_* error numbers */
extern	int	error;		/* Local value for error */
extern	char	*failsafe;	/* $FAILSAFE */
extern	int	flgd;		/*  1 ==> 'd' test option -- show work in progress */
extern	int	flge;		/*  1 ==> 'e'      option -- check existence of mail */
extern	char	*flgF;		/* !0 ==> 'F'      option -- Installing/Removing Forwarding, holds value */
extern	char	*flgf;		/* !0 ==> 'f' read option -- mbox to use, holds value */
extern	int	flgh;		/*  1 ==> 'h' read option -- start mail printing with header list */
extern	char	*flgm;		/* !0 ==> 'm' send option -- holds message type */
extern	int	flgp;		/*  1 ==> 'p' read option -- no prompts when reading mail */
extern	int	flgP;		/*  1 ==> 'P' read option -- always print binary mail */
extern	int	flgq;		/*  1 ==> 'q' read option -- typing interrupts exit while reading mail */
extern	int	flgr;		/*  1 ==> 'r' read option -- print in fifo order */
extern	int	flgt;		/*  1 ==> 't' send option -- add To: line to letter */
extern	int	flgT;		/*  1 ==> 'T' test option -- surrogate file to debug */
extern	int	flgw;		/*  1 ==> 'w' send option -- don't wait on delivery programs */
extern	int	flgx;		/* !0 ==> 'x' test option -- dump debug messages */
extern	int	flglb;		/*  1 ==> '#'      option -- show command to be run */
extern	const Hdr header[];	/* H_* #define's used to index into array */
extern	const char *help[];	/* help messages during printmail() */
extern	char	*hmbox;		/* pointer to $HOME/mbox */
extern	char	*home;		/* pointer to $HOME */
extern	int	interrupted;	/* some signal came in */
extern	int	ismail;		/* default to program=mail */
extern	char	*mailfile;	/* pointer to mailfile */
extern	gid_t	mailgrp;	/* numeric id of group 'mail' */
extern	char	*mailsurr;	/* surrogate file name */
extern	int	maxerr;		/* largest value of error */
extern	const char mbox[];	/* name for mbox */
extern	uid_t	mf_uid;		/* uid of user's mailfile */
extern	gid_t	mf_gid;		/* gid of user's mailfile */
extern	char	my_name[20];	/* user's name who invoked this command */
extern	uid_t	my_euid;	/* user's euid */
extern	gid_t	my_egid;	/* user's egid */
extern	uid_t	my_uid;		/* user's uid */
extern	gid_t	my_gid;		/* user's gid */
extern	char	*remotefrom;	/* Holds name of the system to use in "remote from" */
extern	int	sav_errno;	/* errno from writing to files */
extern	void	(*saveint)();	/* saved signal from SIG_INT */
extern	const char *seldisp[];	/* selective display header field prefixes */
extern	jmp_buf	sjbuf;		/* Where to longjmp for signals */
extern	t_surrfile *surrfile;	/* the compiled surrogate file */
extern	int	surr_len;	/* # entries in surrogate file */
extern	char	*thissys;	/* Holds name of the system we are on */
extern	char	tmpdir[];	/* default directory for tmp files */
extern	Msg	*topmsg;	/* Current message being worked on */
extern	Tmpfile	*toptmpfile;	/* List of the temp files being used. */
extern	mode_t	umsave;		/* saved umask */

#ifdef DEBUG_OPEN_CLOSE
#  define open(s,n) my_open(s,n)
#  define close(n) my_close(n)
   extern FILE	*my_fopen();
#  define fopen(s,t) my_fopen(s,t)
#  define fclose(n) my_fclose(n)
#endif
