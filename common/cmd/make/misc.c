/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)make:misc.c	1.36"

#include "defs"
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>	/* struct stat */
#include <varargs.h>
#include <ccstypes.h>

#define HASHSIZE	1024

/*	The gothead and gotf structures are used to remember
 *	the names of the files `make' automatically gets so
 *	`make' can remove them upon exit.  See get() and rmgots().
 */

typedef struct gotf {		/* a node in a list of files */
	struct gotf *gnextp;
	CHARSTAR gnamep;
} *GOTF;

typedef struct gothead {	/* head node of list of files */
	struct gotf *gnextp;
	CHARSTAR gnamep;
	GOTF endp;
} *GOTHEAD;


extern int 	errno;
extern char	touch_t[],	/* from doname.c */
		archmem[];	/* files.c */
extern VARBLOCK	firstvar;	/* from main.c */
extern int	yyparse();	/* gram.y */

typedef struct hashentry {
	NAMEBLOCK contents;
	struct hashentry *next;
} *HASHENTRY;

HASHENTRY hashtab[HASHSIZE];

/*
**	Declare local functions and make LINT happy.
*/

static CHARSTAR	colontrans();
static CHARSTAR	do_colon();
static int	getword();
static CHARSTAR	dftrans();
static CHARSTAR	do_df();
static CHARSTAR	straightrans();
static void	rm_gots();
static time_t	lookarch();
static int	sstat();
static void	insert_ar();


NAMEBLOCK
hashfind(s)		/* given string, return its NAMEBLOCK or NULL */
register CHARSTAR s;
{
	register int	hashval = 0;
	register CHARSTAR t;
	HASHENTRY	p;

	for (t = s; *t != CNULL; ++t)
		hashval += (unsigned char) *t;

	hashval %= HASHSIZE;

	for (p = hashtab[hashval]; p; p = p->next) {
		if (STREQ(s, p->contents->namep)) {
			return(p->contents);
		}
	}
	return(NULL);
}

NAMEBLOCK
hashput(b)		/* given NAMEBLOCK, put into hash table */
NAMEBLOCK b;	
{
	register int	hashval = 0;
	register CHARSTAR t;
	HASHENTRY	p;

	for (t = b->namep; *t != CNULL; ++t)
		hashval += (unsigned char) *t;

	hashval %= HASHSIZE;

	for (p = hashtab[hashval]; p; p = p->next)
		if (STREQ(b->namep, p->contents->namep)) {
			return(p->contents);
		}

	p = ALLOC (hashentry);
	p->next = hashtab[hashval];
	p->contents = b;
	hashtab[hashval] = p;

	return(p->contents);
}

NAMEBLOCK
makename(s)		/* make a name entry; `s' is assumed previously saved */
register CHARSTAR s;
{
	register NAMEBLOCK p;
	NAMEBLOCK	hashput();

	p = ALLOC(nameblock);
	p->nextname = firstname;
	p->backname = NULL;
	p->namep = s;
	p->linep = NULL;
	p->done = D_INIT;
	p->septype = 0;
	p->rundep = 0;
	p->modtime = 0L;
	p->shenvp = NULL;
	p->tbfp = NULL;

	(void) hashput( p );
	firstname = p;

	return( p );
}

#define	SHASHSIZE	73

FSTATIC struct string {
	struct string *str_p;
	char	*str_s;
} *strtab[SHASHSIZE];


CHARSTAR
copys(s)
register char *s;
{
	register char *t;
	register int	len, hashval = 0;
	register struct string *p;

	for (len = 0, t = s; *t != CNULL; len++, t++)
		hashval += (unsigned char) *t;

	len++;
	hashval %= SHASHSIZE;

	for (p = strtab[hashval]; p; p = p->str_p)
		if (STREQ(s, p->str_s))
			return(p->str_s);

	if ( !(p = ALLOC(string)) )
fat:		fatal("cannot alloc memory");

	if ( !(p->str_s = (CHARSTAR) calloc((unsigned) len, sizeof(*p->str_s))) )
		goto fat;

	p->str_p = strtab[hashval];
	strtab[hashval] = p;
	t = p->str_s;
	while (*t++ = *s++)
		;
	return(p->str_s);
}


CHARSTAR
concat(a, b, c)			/* c = concatenation of a and b */
register CHARSTAR a, b;
CHARSTAR c;
{
	register CHARSTAR t = c;
	while (*t = *a++)
		t++;
	while (*t++ = *b++)
		;
	return(c);
}


int
suffix(a, b, p)		/* if b is the suffix of a, set p = prefix */
register CHARSTAR a, b, p;
{
	if (! (a && b) )
		return(0);
	else {
		CHARSTAR a0 = a,
			 b0 = b;

		while (*a++) ;
		while (*b++) ;

		if ( (a - a0) < (b - b0) )
			return(0);
	
		while (b > b0)
			if (*--a != *--b)
				return(0);

		while (a0 < a)
			*p++ = *a0++;
		*p = CNULL;
		return(1);
	}
}


int	*
intalloc(n)
int	n;
{
	int *p;

	if ( p = (int *) calloc(1, (unsigned) n) )
		return(p);

	fatal("out of memory");
	/*NOTREACHED*/
}


CHARSTAR
subst(a, b)	/* copy string a into b, substituting for arguments */
register CHARSTAR a, b;
{
	register CHARSTAR s;
	static depth = 0;
	char	vname[OUTMAX], closer;
	int	amatch();
	
	if (++depth > 100)
		fatal("infinitely recursive macro?");
	if (a)
		while (*a)
			if (*a != DOLLAR)
				*b++ = *a++;
			else if (*++a == CNULL || *a == DOLLAR)
				*b++ = *a++;
			else {
				s = vname;
				if ( *a == LPAREN || *a == LCURLY ) {
					closer = (*a == LPAREN ? RPAREN : RCURLY);
					++a;
					while (*a == BLANK)
						++a;
					while ((*a != BLANK) &&
					       (*a != closer) && 
					       (*a != CNULL))
						*s++ = *a++;
					while (*a != closer && *a != CNULL)
						++a;
					if (*a == closer)
						++a;
				} else
					*s++ = *a++;

				*s = CNULL;
				if (amatch(&vname[0], "*:*=*"))
					b = colontrans(b, vname);

				else if (ANY("@*<%?", vname[0]) && vname[1])
					b = dftrans(b, vname);
				else
					b = straightrans(b, vname);
				s++;
			}

	*b = CNULL;
	--depth;
	return(b);
}


static CHARSTAR
colontrans(b, vname)	/* Translate the $(name:*=*) type things. */
register CHARSTAR b;
char	vname[];
{
	register CHARSTAR p, q = 0;
	char	dftype = 0;
	CHARSTAR pcolon;
	VARBLOCK vbp, srchvar();
	void	cfree();

	/* Mark off the name (up to colon),
	 *	 the from expression (up to '='),
	 *	 and the to expresion (up to CNULL). */

	for (p = &vname[0]; *p  && *p != KOLON; p++)
		;
	pcolon = p;
	*pcolon = CNULL;

	if (ANY("@*<%?", vname[0])) {
		dftype = vname[1];
		vname[1] = CNULL;
	}
	if ( !(vbp = srchvar(vname)) )
		return(b);

	p = vbp->varval.charstar;
	if (dftype) {
		if ( !(q = (CHARSTAR) calloc((unsigned) (strlen(p) + 2), 1)) )
			fatal("cannot alloc mem");
		(void)do_df(q, p, vname[1]);	/*D/F trans gets smaller*/
		p = q;
	}
	if (p && *p)
		b = do_colon(b, p, pcolon + 1);
	*pcolon = KOLON;
	if (dftype)
		vname[1] = dftype;
	if (q)
		cfree(q);
	return(b);
}
static char *subStringOccurs(string, subStr)
	char *string, *subStr;
{
	int len = strlen(subStr);
	while(*string){
		if(strncmp(string, subStr, len)==0)
			return string;
		string++;
	}
	return 0;
}

static CHARSTAR
do_colon(to, from, trans)	/* Translate $(name:*=*) type things. */
register CHARSTAR to, from;
CHARSTAR trans;
{
	register int	i = 0, leftlen;
	register CHARSTAR p;
	CHARSTAR pbuf, trysccs(), strshift(), sdot();
	char	left[30], right[70], buf[128];
	int	lwig = 0, rwig = 0, len;
	int	sindex();
	
/* Mark off the name (up to colon), the from expression ( up to '=')
 *	and the to expression (up to CNULL)
 */
	while (*trans != EQUALS)
		left[i++] = *trans++;
	if (left[i-1] == WIGGLE) {
		lwig++;
		--i;
	}
	left[i] = CNULL;
	leftlen = i;
	i = 0;
	while (*++trans)
		right[i++] = *trans;
	if (right[i-1] == WIGGLE) {
		rwig++;
		--i;
	}
	right[i] = CNULL;

	/*	Now, translate.	 */
#if 0
	for (; len = getword(from, buf); from += len) {
		pbuf = buf;
		if ((i = sindex(pbuf, left)) >= 0 &&
		    (pbuf[i+leftlen] == CNULL) &&
		    (lwig ? ((p = sdot(pbuf)) != NULL) : 1)) {
			pbuf[i] = CNULL;
			if (!lwig && rwig)
				(void)trysccs(pbuf);
			else if (lwig && !rwig)
				(void)strshift(p, -2);
			to = copstr(to, pbuf);
			to = copstr(to, right);
		} else
			to = copstr(to, pbuf);
	}
#endif
	for (; len = getword(from, buf); from += len) {
		char *occurence;
		pbuf = buf;
		while(occurence=subStringOccurs(pbuf,left)){
			char saveC = *occurence;
			*occurence = CNULL;
			to = copstr(to, pbuf);
			to = copstr(to, right);
			*occurence = saveC;
			/* Jump over the substituted string */
			pbuf = occurence+leftlen;
		}
		/* copy the rest of trailing stuff */
		to = copstr(to,pbuf);
	}
	return(to);
}


static int
getword(from, buf)
register CHARSTAR from, buf;
{
	register int	i = 0;
	if (*from == TAB || *from == BLANK)
		while (*from == TAB || *from == BLANK) {
			*buf++ = *from++;
			++i;
		}
	else
		while (*from && *from != TAB && *from != BLANK) {
			*buf++ = *from++;
			++i;
		}
	*buf = CNULL;
	return(i);
}


static CHARSTAR
dftrans(b, vname)		/* Do the $(@D) type translations. */
register CHARSTAR b;
CHARSTAR vname;
{
	register VARBLOCK vbp;
	VARBLOCK srchvar();
	char	c1 = vname[1];

	vname[1] = CNULL;
	vbp = srchvar(vname);
	vname[1] = c1;
	if (vbp && *vbp->varval.charstar)
		b = do_df(b, vbp->varval.charstar, c1);
	return(b);
}


static CHARSTAR
do_df(b, str, type)
register CHARSTAR b;
char	str[], type;
{
	register int	i;
	register CHARSTAR p;
	char	buf[128];

#define LASTSLASH(a)		strrchr(a, SLASH)

	*b = CNULL;
	for (; i = getword(str, buf); str += i) {
		if (buf[0] == BLANK || buf[0] == TAB) {
			b = copstr(b, buf);
			continue;
		}
		if (p = LASTSLASH(buf)) {
			*p = CNULL;
			b = copstr(b, type == 'D' ? (buf[0] == CNULL ? "/" : buf):
						    p + 1);
			*p = SLASH;
		} else
			b = copstr(b, type == 'D' ? "./" : buf);
	}
	return(b);
}


static CHARSTAR
straightrans(b, vname)		/* Standard trnaslation, nothing fancy. */
register CHARSTAR b;
char	vname[];
{
	register CHAIN pchain;
	register NAMEBLOCK pn;
	register VARBLOCK vbp;
	VARBLOCK srchvar();

	if ( vbp = srchvar(vname) ) {
		if (vbp->v_aflg  && vbp->varval.chain) {
			pchain = (vbp->varval.chain);
			for (; pchain; pchain = pchain->nextchain) {
				pn = pchain->datap.nameblock;
				if (pn->alias)
					b = copstr(b, pn->alias);
				else
					b = copstr(b, pn->namep);
				*b++ = BLANK;
			}
			vbp->used = YES;
		} else if (*vbp->varval.charstar)
			b = subst(vbp->varval.charstar, b);

		vbp->used = YES;
	}
	return(b);
}


/* copy s into t, return the location of the next free character in s */
CHARSTAR
copstr(s, t)
register CHARSTAR s, t;
{
	if ( !t )
		return(s);
	while (*t)
		*s++ = *t++;
	*s = CNULL;
	return(s);
}


void
setvar(v, s)
register CHARSTAR v, s;
{
	VARBLOCK srchvar();
	register VARBLOCK p = srchvar(v);


	if ( !p )
		p = varptr(v);
	if ( !s )
		s = Nullstr;
	if ( !(p->noreset) ) {
		if (IS_ON(EXPORT))
			p->envflg = YES;
		p->varval.charstar = s;
		if (IS_ON(INARGS) || IS_ON(ENVOVER))
			p->noreset = YES;
		else
			p->noreset = NO;
#ifdef MKDEBUG
		if (IS_ON(DBUG)) {
			printf("setvar: %s = %s, noreset = %d envflg = %d Mflags = 0%o\n",
			    v, p->varval.charstar, p->noreset, p->envflg, Mflags);

		}
		if (p->used && !amatch(v, "[@*<?!%]") ){
			if(IS_OFF(WARN))
				fprintf(stderr, "Warning: %s changed after being used\n", v);
		}
#endif
	}
}


VARBLOCK
varptr(v)
register CHARSTAR v;
{
	VARBLOCK srchvar();
	register VARBLOCK vp = srchvar(v);

	if ( vp )
		return(vp);

	vp = ALLOC(varblock);
	vp->nextvar = firstvar;
	firstvar = vp;
	vp->varname = copys(v);
	vp->varval.charstar = 0;
	return(vp);
}


VARBLOCK
srchvar(vname)
register CHARSTAR vname;
{
	register VARBLOCK vp = firstvar;
	for (; vp; vp = vp->nextvar)
		if (STREQ(vname, vp->varname))
			return(vp);
	return( NULL );
}


/*VARARGS1*/
void
fatal1(s, t1, t2)
CHARSTAR s;
{
	char	buf[MAXNAMLEN*2];
	void	fatal();

	(void)sprintf(buf, s, t1, t2);
	fatal(buf);
}


void
fatal(s)
CHARSTAR s;
{
	fflush(stdout);
	fprintf(stderr, "\n");
	fprintf(stderr, "make");
	fprintf(stderr, ": fatal error");
	if (s)
		fprintf(stderr, ": %s", s);
	fprintf(stderr, ".\n");
	mkexit(1);
}


yyerror(s)		/* called when yyparse encounters an error */
CHARSTAR s;
{
	CHARSTAR	buf;
	void	fatal();
	extern int	yylineno;

	buf = ck_malloc(strlen(s) + 20 );
	(void)sprintf(buf, "line %d: %s", yylineno, s);
	fatal(buf);
	/* NOTREACHED */
}


void
appendq(head, tail)
register CHAIN head;
CHARSTAR tail;
{
	CHAIN p = ALLOC(chain);
	p->datap.charstar = tail;
	while (head->nextchain)
		head = head->nextchain;
	head->nextchain = p;
}


CHARSTAR
mkqlist(p)
register CHAIN p;
{
	static char	qbuf[OUTMAX];
	register CHARSTAR s, qbufp = qbuf, qp = &qbuf[OUTMAX-3];

	for ( ; p; p = p->nextchain) {
		s = p->datap.charstar;
		if (qbufp != qbuf)
			*qbufp++ = BLANK;
		if (qbufp + strlen(s) > qp) {
			fprintf(stderr, "$? (bu35)\n");
			break;
		}
		while (*s)
			*qbufp++ = *s++;
	}
	*qbufp = CNULL;
	return(qbuf);
}


int
sindex(s1, s2)
CHARSTAR s1, s2;
{
	register CHARSTAR p1 = &s1[0],
			  p2 = &s2[0];
	register int	ii = 0, flag = -1;

	for (; ; ii++) {
		while (*p1 == *p2) {
			if (flag < 0)
				flag = ii;
			if (*p1++ == CNULL)
				return(flag);
			p2++;
		}
		if (*p2 == CNULL)
			return(flag);
		if (flag >= 0) {
			flag = -1;
			p2 = &s2[0];
		}
		if (*s1++ == CNULL)
			return(flag);
		p1 = s1;
	}
}


CHARSTAR
trysccs(str)		/* change xx to s.xx or /x/y/z to /x/y/s.z */
register CHARSTAR str;
{
	CHARSTAR 	sstr = str;
	register int	i = 2;
	for (; *str; str++)
		;
	str[2] = CNULL;
	str--;
	for (; str >= sstr; str--) {
		if ((*str == SLASH) && (i == 2)) {
			i = 0;
			*(str + 2) = DOT;
			*(str + 1) = 's';
		}
		*(str + i) = *str;
	}
	if (i == 2) {
		*(str + 2) = DOT;
		*(str + 1) = 's';
	}
	return(sstr);
}


int
is_sccs(filename)
register CHARSTAR filename;
{
	register CHARSTAR p = filename;
	for (; *p; p++)
		if ((*p == 's') &&
		    ((p == filename && p[1] == DOT) ||
		     (p[-1] == SLASH && p[1] == DOT)))
			return(YES);
	return(NO);
}


CHARSTAR
sdot(p)
register CHARSTAR p;
{
	register CHARSTAR ps = p;
	for (; *p; p++)
		if ((*p == 's') &&
		    ((p == ps && p[1] == DOT) ||
		     (p[-1] == SLASH && p[1] == DOT)))
			return(p);
	return(NULL);
}


CHARSTAR
addstars(pfx)		/* change pfx to /xxx/yy/*zz.* or *zz.* */
register CHARSTAR pfx;
{
	register CHARSTAR p1 = pfx, p2;
	
	for (; *p1; p1++)
		;
	p2 = p1 + 3;		/* 3 characters, '*', '.', and '*'. */
	p1--;
	*p2-- = CNULL;
	*p2-- = STAR;
	*p2-- = DOT;
	while (p1 >= pfx) {
		if (*p1 == SLASH) {
			*p2 = STAR;
			return(pfx);
		}
		*p2-- = *p1--;
	}
	*p2 = STAR;
	return(p2);
}



#define NENV	300


void
setenv()	/* set up the environment */
{
	register CHARSTAR *ea, p;
	register int	nenv;
	register VARBLOCK vp;
	CHARSTAR *es;

	if ( !firstvar )
		return;

	if ( !(es = ea = (CHARSTAR *)calloc(NENV, sizeof * ea)) )
m_error:	fatal("cannot alloc mem");

	nenv = 0;

	for (vp = firstvar; vp; vp = vp->nextvar)
		if (vp->envflg) {
			char str[OUTMAX];
			if (++nenv >= NENV)
				fatal("too many env parameters.");
			/* expand any variables in values */
			(void)subst(vp->varval.charstar, str);
			if ( !(*ea = (CHARSTAR) calloc( (unsigned) (strlen(vp->varname) +
					     strlen(str) + 2),
					     sizeof *ea)) )
				goto m_error;
			p = copstr(*ea++, vp->varname);
			p = copstr(p, "=");
#if 0
			p = copstr(p, vp->varval.charstar);
#endif
			p = copstr(p, str);
		}
	*ea = 0;
	if (nenv > 0)
		environ = es;
#ifdef MKDEBUG
	if (IS_ON(DBUG))
		printf("nenv = %d\n", nenv);
#endif
}


/*
 *	Shift a string `pstr' count places. negative is left, pos is right
 *	Negative shifts cause char's at front to be lost.
 *	Positive shifts assume enough space!
 */
CHARSTAR
strshift(pstr, count)
register CHARSTAR pstr;
register int	count;
{
	register CHARSTAR sstr = pstr;

	if (count < 0) {
		for (count = -count; *pstr = pstr[count]; pstr++)
			;
		*pstr = 0;
		return(sstr);
	}
	for (; *pstr; pstr++)
		;
	do
		pstr[count] = *pstr;
	while (pstr--, count--);

	return(sstr);
}



/*
 * get() does an SCCS get on the file ssfile.
 *	For the get command, get() uses the value of the variable "GET".
 *	If the rlse string variable is set, get() uses it in the
 *	get command sequence.
 *	Thus a possible sequence is:
 *		set -x;
 *		get -r2.3.4.5 s.stdio.h
 *
 */

GOTHEAD gotfiles;

get(ssfile, rlse)
register CHARSTAR ssfile;
CHARSTAR rlse;
{
	register CHARSTAR pr, pr1;
	CHARSTAR trysccs(), sdot(), rmsdot();
	char	gbuf[BUF_SIZE], sfile[BUF_SIZE], dfile[BUF_SIZE];
	int	retval, access();
	GOTF	gf;
	
	(void)copstr(sfile, ssfile);
	if (!sdot(sfile)) {
		(void)trysccs(sfile);
	}
	if (access(sfile, 4) && IS_OFF(GET))
		return(NO);

	pr = gbuf;
	if (IS_OFF(SIL))
		pr = copstr(pr, "set -x;\n");

	pr = copstr(pr, varptr("GET")->varval.charstar);
	pr = copstr(pr, " ");
	pr = copstr(pr, varptr("GFLAGS")->varval.charstar);
	pr = copstr(pr, " ");

	if ((pr1 = rlse) && pr1[0] != CNULL) {
		if (pr1[0] != MINUS)	/* RELEASE doesn't have '-r' */
			pr = copstr(pr, "-r");
		pr = copstr(pr, pr1);
		pr = copstr(pr, " ");
	}

	pr = copstr(pr, sfile);

	/*
	 *	exit codes are opposite of error codes so do the following:
	 */

	if ( retval = !system(gbuf) ? YES : NO ) {
		if ( !gotfiles ) {
			gotfiles = ALLOC(gothead);
			gf = (GOTF)gotfiles;
			gotfiles->gnextp = NULL;
			gotfiles->endp = (GOTF)gotfiles;
		} else {
			gf = gotfiles->endp;
			gf->gnextp = ALLOC(gotf);
			gf = gf->gnextp;
			gf->gnextp = NULL;
		}

		/* set the name of the file to dest directory, plus the *
		 * file name without the source path or the "s." stuff  */

		(void) cat(dfile, rmsdot(sfile), 0);
		gf->gnamep = copys(dfile);
		gotfiles->endp = gf;
	}
	return(retval);
}



/* rmsdot(sccsname) returns a pointer to the portion of an sccs file
 * 	name that is beyond the directory specifications and the
 *	"s." prefix of the sccs file name.
 */

CHARSTAR
rmsdot(sccsname)
CHARSTAR sccsname;		/* pointer to the name string to fix */
{
	register CHARSTAR p = sccsname;
	for (; *p != '\0'; p++)
		;	/* skip to end of full pathname */

	for (; *p != '/' && p >= sccsname; p--)
		;	/* back up to beginning of name part */

	return(p + 3);			/* skip the "/s." characters */
}



static void
rm_gots()	/* remove gotten files. */
{
	int unlink();
	if (IS_ON(GF_KEEP))
		return;
	else {
		register GOTF gf = (GOTF)gotfiles;

		for ( ; gf; gf = gf->gnextp)
			if (gf->gnamep) {
#ifdef MKDEBUG
				if (IS_ON(DBUG))
					printf("rm_got: %s\n", gf->gnamep);
#endif
				(void)unlink(gf->gnamep);
			}
	}
}


int
isprecious(p)
CHARSTAR p;
{
	register NAMEBLOCK np;

	if (np = SRCHNAME(".PRECIOUS")) {
		register LINEBLOCK lp = np->linep;
		register DEPBLOCK  dp;

		for ( ; lp; lp = lp->nextline)
			for (dp = lp->depp; dp; dp = dp->nextdep)
				if (STREQ(p, dp->depname->namep))
					return(YES);
	}
	return(NO);
}


int
isdir(filnam)
char	*filnam;
{
	int 	stat();
	struct stat statbuf;

	if (stat(filnam, &statbuf) == -1)
		return(2);			/* can't stat, don't remove */
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
		return(1);
	return(0);
}


/* for notations a(b) and a((b)), look inside archives 
 *	a(b)	is file member   b   in archive a
 *	a((b))	is entry point   b  in object archive a
 */
static time_t
lookarch(filename)
register CHARSTAR filename;
{
	register int	i;
	register CHARSTAR p;
	CHARSTAR q, Q, sname();
	CHARSTAR sfname;
	time_t	ret_la, la();
	char	s[15];
	int	objarch = NO;
	
#ifdef MKDEBUG
	if (IS_ON(DBUG)) 
		printf("lookarch(%s)\n", filename);
#endif
	if (filename[0] == CNULL)
f_error:	fatal1("null archive name `%s'", filename);
		
	for (p = filename; *p != LPAREN ; ++p)
		;
	Q = p++;
	sfname = sname(filename);
	if ((i = Q - sfname) == 0)
		fatal1("null archive name `%s'", sfname);
	if (*p == LPAREN) {
		objarch = YES;
		++p;
		if ( !(q = strchr(p, RPAREN)) )
			q = p + strlen(p);
		(void)strncpy(s, p, (size_t) (q - p) );
		s[q-p] = CNULL;
	} else {
		if ( !(q = strchr(p, RPAREN)) )
			q = p + strlen(p);
		i = q - p;
		(void)strncpy(archmem, p, (size_t) i);
		archmem[i] = CNULL;
		if (archmem[0] == CNULL)
			goto f_error;
		if (q = strrchr(archmem, SLASH))
			++q;
		else
			q = archmem;
		(void)strncpy(s, q, (size_t) 14);
		s[14] = CNULL;
	}
	*Q = CNULL;
	ret_la = la(filename, s, objarch);
	insert_ar(sname(filename));
	*Q = LPAREN;
	return(	ret_la );
}

static int
sstat(pname, buffer)
NAMEBLOCK pname;
struct stat *buffer;
{
	register char	*s;
	register int	 nonfile = NO;
	int	stat();
	char	*temp;

	CHARSTAR filename = pname->namep;

	temp = ck_malloc(MAXPATHLEN);

	archmem[0] = CNULL;
	if (ANY(filename, LPAREN)) {
		nonfile = YES;

		strcpy(temp,filename);
		(void)compath(temp);
		for (s = temp; !(*s == CNULL || *s == LPAREN); s++)
			;
		*s = CNULL;
		pname->alias = copys(temp);


	}

	strcpy(temp,filename);
	(void)compath(temp);
	if ((nonfile && (buffer->st_mtime = lookarch(temp))) ||
	    (!(stat(temp, buffer) || nonfile))){
		free(temp);
		return(1);
	}else{ 
		free(temp);
		return(-1);
	}

}


time_t
exists(pname)
NAMEBLOCK pname;
{
	struct stat buf;

	if (pname == NULL)
		return(-1L);

	if ( sstat(pname, &buf) < 0 ) {
		return(-1L);
	} else {
#ifdef MKDEBUG
		if (IS_ON(DBUG))
			fprintf(stdout, "time = %ld\n", buf.st_mtime);
#endif
		return(buf.st_mtime);
	}
}



void
mkexit(arg)		/* remove files make automatically got */
{
	extern NAMEBLOCK curpname;
	extern int lib_cpd;
	void kill_run();
	void	exit();
	time_t	exists();
	int	member_ar();
	NAMEBLOCK	lookup_name();
	void pwait();

	(void)signal(SIGINT, SIG_IGN);	/* ignore signals */
	(void)signal(SIGQUIT, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);
	if (IS_ON(TOUCH))		/* remove `touch' temporary file */
		(void)unlink(touch_t);

	/* if library was target & copied in, remove it */
	if (lib_cpd)
		(void)unlink(curpname->namep);

	rm_gots();

	if(IS_ON(PAR) && nproc)
		kill_run();

	exit(arg < 0 ? arg + 256 : arg);
}


/*	dname(), sname(), and cat() were stolen from libPW	*/

/*
	dname() returns directory name containing a file (by
	modifying its argument). 
	Returns "." if current directory; handles root correctly. 
	Returns its argument.
	Bugs: doesn't handle null strings correctly.
*/

char *dname(p)
char *p;
{
	register char *c;
	register int s = strlen(p) + 1;

	for(c = p+s-2; c > p; c--)
		if(*c == '/') {
			*c = '\0';
			return(p);
		}
	if (p[0] != '/')
		p[0] = '.';
	p[1] = 0;
	return(p);
}


/*
	sname() returns pointer to "simple" name of path name; that
	is, pointer to first character after last "/".  If no
	slashes, returns pointer to first char of arg.
	If the string ends in a slash, returns a pointer to the first
	character after the preceeding slash, or the first character.
*/

char	*sname(s)
char *s;
{
	register char *p;
	register int j;
	register int n = strlen(s);

	--n;
	if (s[n] == '/')
		for (j=n; j >= 0; --j)
			if (s[j] != '/') {
				s[++j] = '\0';
				break;
			}

	for(p=s; *p; p++)
		if(*p == '/')
			s = p + 1;
	return(s);
}


/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
*/

/*VARARGS*/
void
cat(va_alist)
va_dcl
{
	register char *d, *s;
	va_list ap;

	va_start(ap);
	d = va_arg(ap, CHARSTAR);

	while (s = va_arg(ap, CHARSTAR)) {
		while (*d++ = *s++) ;
		d--;
	}
}



/*
 *	compath(pathname)
 *
 *	This compresses pathnames.  All strings of multiple slashes are
 *	changed to a single slash.  All occurrences of "./" are removed.
 *	Whenever possible, strings of "/.." are removed together with
 *	the directory names that they follow.
 *
 *	WARNING: since pathname is altered by this function, it should
 *		 be located in a temporary buffer. This avoids the problem
 *		 of accidently changing strings obtained from makefiles
 *		 and stored in global structures.
 */

char	*
compath(pathname)
register char	*pathname;
{
	register char	*nextchar, *lastchar, *sofar;
	char	*pnend;
	int	pnlen;

	/*	return if the path has no "/"	*/

	if ( !strchr(pathname, '/') )
		return(pathname);

	/*	find all strings consisting of more than one '/'	*/

	for (lastchar = pathname + 1; *lastchar != '\0'; lastchar++)
		if ((*lastchar == '/') && (*(lastchar - 1) == '/')) {

			/* find the character after the last slash */

			nextchar = lastchar;
			while (*++lastchar == '/')
				;

			/* eliminate the extra slashes by copying
			 *  everything after the slashes over the slashes
			 */
			sofar = nextchar;
			while ((*nextchar++ = *lastchar++) != '\0')
				;
			lastchar = sofar;
		}

	/*	find all strings of "./"	 */

	for (lastchar = pathname + 1; *lastchar != '\0'; lastchar++)
		if ((*lastchar == '/') &&
		    (*(lastchar - 1) == '.') && 
		    ((lastchar - 1 == pathname) ||
		     (*(lastchar - 2) == '/'))) {

			/* copy everything after the "./" over the "./" */

			nextchar = lastchar - 1;
			sofar = nextchar;
			while ((*nextchar++ = *++lastchar) != '\0')
				;
			lastchar = sofar;
		}

	/*	find each occurrence of "/.."	*/

	for (lastchar = pathname + 1; *lastchar != '\0'; lastchar++)
		if ((lastchar != pathname) && (*lastchar == '/') && 
		    (*(lastchar+1) == '.') &&
		    (*(lastchar+2) == '.') && 
		    ((*(lastchar+3) == '/') || (*(lastchar+3) == '\0'))) {

			/* find the directory name preceding the "/.." */

			nextchar = lastchar - 1;
			while ((nextchar != pathname) && 
			       (*(nextchar - 1) != '/'))
				--nextchar;

			/*  make sure the preceding directory's name
			 *	is not "." or ".."
			 */
			if (!((*nextchar == '.') && 
			      (*(nextchar + 1) == '/') || 
			      ((*(nextchar + 1) == '.') &&
			       (*(nextchar + 2) == '/')))) {

				/*  prepare to eliminate either		*
				 *    "dir_name/../" or "dir_name/.." 	*/

				if (*(lastchar + 3) == '/')
					lastchar += 4;
				else
					lastchar += 3;

				/*  copy everything after the "/.." to   *
				 *   before the preceding directory name */

				sofar = nextchar - 1;
				while ((*nextchar++ = *lastchar++) != '\0')
					;
				lastchar = sofar;

				/*  if (the character before what was
				 *	taken out is '/'),
				 *	set up to check
				 *	if the slash is part of "/.."
				 */
				if ((sofar + 1 != pathname) && (*sofar == '/'))
					--lastchar;
			}
		}

	/*  if the string is more than a character long and ends in '/'
	 *	eliminate the '/'.
	 */
	pnend = strchr(pathname, '\0') - 1;
	if (((pnlen = strlen(pathname)) > 1) && (*pnend == '/')) {
		*pnend-- = '\0';
		pnlen--;
	}

	/*  if the string has more than two characters and ends in "/.",
	 *	remove the "/.".
	 */
	if ((pnlen > 2) && (*(pnend - 1) == '/') && (*pnend == '.'))
		*--pnend = '\0';

	/*
	 *	if all characters were deleted, return ".";
	 *	otherwise return pathname
	 */
	if (*pathname == '\0')
		(void)strcpy(pathname, ".");

	return(pathname);
}



typedef struct ar_node {
	char *name;
	struct ar_node *next;
} ar_node;

ar_node *ar_set = (ar_node *)NULL;	/* linked list of archive names */

static void
insert_ar(name)		/* save an archive's name */
char *name;
{
	if (! member_ar(name) ) {
		register ar_node *temp = (ar_node *)ck_malloc(sizeof(ar_node));

		temp -> name = ck_malloc((unsigned) (strlen(name) + 1));
		strcpy(temp -> name, name);
		strcat(temp -> name, "\0");
		if (ar_set == (ar_node *)NULL)
			temp -> next = (ar_node *)NULL;
		else
			temp -> next = ar_set;
		ar_set = temp;
	}		
}


int
member_ar(name)		/* archive name previously saved? */
register char *name;
{
	register ar_node *p = ar_set;

	for (; p; p = p -> next)
		if (!strcmp(p -> name, name))
			return(1);

	return(0);
}
char *
ck_malloc(len)
int len;
{
	char *p;
	if((p= malloc(len)) == NULL)
		fatal1("couldn't malloc len=%d", len);
	return(p);
}


