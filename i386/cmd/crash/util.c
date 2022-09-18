/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/util.c	1.12.1.3"
#ident	"$Header: util.c 1.1 91/07/23 $"

/*
 * This file contains code for utilities used by more than one crash function.
 */

#include "sys/param.h"
#include "a.out.h"
#include "signal.h"
#include "stdio.h"
#include "setjmp.h"
#include "ctype.h"
#include "sys/types.h"
#include "sys/var.h"
#include "sys/proc.h"
#include <sys/bootinfo.h>
#include "crash.h"
#include "priv.h"

extern jmp_buf jmp;
extern struct syment *Curproc, *V;
extern long vtop();
extern int opipe;
extern FILE *rp;
void exit();
void free();

/* close pipe and reset file pointers */
int
resetfp()
{
	extern void pipesig();

	if(opipe == 1) {
		pclose(fp);
		signal(SIGPIPE,pipesig);
		opipe = 0;
		fp = stdout;
	}
	else {
		if((fp != stdout) && (fp != rp) && (fp != NULL)) {
			fclose(fp);
			fp = stdout;
		}
	}
}

/* signal handling */
void 
sigint()
{
	extern int *stk_bptr;

#if defined(__STDC__)
	signal(SIGINT, (void (*)(int))sigint);
#else
	signal(SIGINT, sigint);
#endif
	if(stk_bptr) {
		free((char *)stk_bptr);
		stk_bptr = NULL;
	}
	fflush(fp);
	resetfp();
	fprintf(fp,"\n");
	longjmp(jmp, 0);
}

/* used in init.c to exit program */
/*VARARGS1*/
int
fatal(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{
	fprintf(stderr,"crash: ");
	fprintf(stderr,string,arg1,arg2,arg3);
	exit(1);
}

/* string to hexadecimal long conversion */
long
hextol(s)
char *s;
{
	int	i,j;
		
	for(j = 0; s[j] != '\0'; j++)
		if((s[j] < '0' || s[j] > '9') && (s[j] < 'a' || s[j] > 'f')
			&& (s[j] < 'A' || s[j] > 'F'))
			break ;
	if(s[j] != '\0' || sscanf(s, "%x", &i) != 1) {
		prerrmes("%c is not a digit or letter a - f\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to decimal long conversion */
long
stol(s)
char *s;
{
	int	i,j;
		
	for(j = 0; s[j] != '\0'; j++)
		if((s[j] < '0' || s[j] > '9'))
			break ;
	if(s[j] != '\0' || sscanf(s, "%d", &i) != 1) {
		prerrmes("%c is not a digit 0 - 9\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to octal long conversion */
long
octol(s)
char *s;
{
	int	i,j;
		
	for(j = 0; s[j] != '\0'; j++)
		if((s[j] < '0' || s[j] > '7')) 
			break ;
	if(s[j] != '\0' || sscanf(s, "%o", &i) != 1) {
		prerrmes("%c is not a digit 0 - 7\n",s[j]);
		return(-1);
	}
	return(i);
}


/* string to binary long conversion */
long
btol(s)
char *s;
{
	int	i,j;
		
	i = 0;
	for(j = 0; s[j] != '\0'; j++)
		switch(s[j]) {
			case '0' :	i = i << 1;
					break;
			case '1' :	i = (i << 1) + 1;
					break;
			default  :	prerrmes("%c is not a 0 or 1\n",s[j]);
					return(-1);
		}
	return(i);
}

/* string to number conversion */
long
strcon(string,format)
char *string;
char format;
{
	char *s;

	s = string;
	if(*s == '0') {
		if(strlen(s) == 1)
			return(0); 
		switch(*++s) {
			case 'X' :
			case 'x' :	format = 'h';
					s++;
					break;
			case 'B' :
			case 'b' :	format = 'b';
					s++;
					break;
			case 'D' :
			case 'd' :	format = 'd';
					s++;
					break;
			default  :	format = 'o';
		}
	}
	if(!format)
		format = 'd';
	switch(format) {
		case 'h' :	return(hextol(s));
		case 'd' :	return(stol(s));
		case 'o' :	return(octol(s));
		case 'b' :	return(btol(s));
		default  :	return(-1);
	}
}


/* lseek */
int
seekmem(addr,mode,proc)
long addr;
int mode,proc;
{
	long paddr;
	extern long lseek();
	extern paddr_t adjmemhole();

	if(!mode || !Virtmode)
		paddr = addr;
	else
		paddr = vtop(addr,proc);
	if(paddr == -1)
		error("%x is an invalid address\n",addr);
	paddr = adjmemhole(paddr);
	if(lseek(mem,paddr,0) == -1)
		error("seek error on address %x\n",addr);
}

/* lseek and read */
int
readmem(addr,mode,proc,buffer,size,name)
long addr;
int mode,proc;
char *buffer;
unsigned size;
char *name;
{
	seekmem(addr,mode,proc);
	if(read(mem,buffer,size) != size)
		error("read error on %s\n",name);
}



/* error handling */
/*VARARGS1*/
int
error(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{

	if(rp) 
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	fflush(fp);
	resetfp();
	longjmp(jmp,0);
}


/* print error message */
/*VARARGS1*/
int
prerrmes(string,arg1,arg2,arg3)
char *string;
int arg1,arg2,arg3;
{

	if((rp && (rp != stdout)) || (fp != stdout)) 
		fprintf(stdout,string,arg1,arg2,arg3);
	fprintf(fp,string,arg1,arg2,arg3);
	fflush(fp);
}


/* simple arithmetic expression evaluation ( +  - & | * /) */
long
eval(string)
char *string;
{
	int j,i;
	char rand1[ARGLEN];
	char rand2[ARGLEN];
	char *op;
	long addr1,addr2;
	struct syment *sp;
	extern char *strpbrk();

	if(string[strlen(string)-1] != ')') {
		prerrmes("(%s is not a well-formed expression\n",string);
		return(-1);
	}
	if(!(op = strpbrk(string,"+-&|*/"))) {
		prerrmes("(%s is not an expression\n",string);
		return(-1);
	}
	for(j=0,i=0; string[j] != *op; j++,i++) {
		if(string[j] == ' ')
			--i;
		else rand1[i] = string[j];
	}
	rand1[i] = '\0';
	j++;
	for(i = 0; string[j] != ')'; j++,i++) {
		if(string[j] == ' ')
			--i;
		else rand2[i] = string[j];
	}
	rand2[i] = '\0';
	if(!strlen(rand2) || strpbrk(rand2,"+-&|*/")) {
		prerrmes("(%s is not a well-formed expression\n",string);
		return(-1);
	}
	if(sp = symsrch(rand1))
		addr1 = sp->n_value;
	else if((addr1 = strcon(rand1,NULL)) == -1)
		return(-1);
	if(sp = symsrch(rand2))
		addr2 = sp->n_value;
	else if((addr2 = strcon(rand2,NULL)) == -1)
		return(-1);
	switch(*op) {
		case '+' : return(addr1 + addr2);
		case '-' : return(addr1 - addr2);
		case '&' : return(addr1 & addr2);
		case '|' : return(addr1 | addr2);
		case '*' : return(addr1 * addr2);
		case '/' : if(addr2 == 0) {
				prerrmes("cannot divide by 0\n");
				return(-1);
			   }
			   return(addr1 / addr2);
	}
	return(-1);
}

static struct procslot {
	proc_t *p;
	pid_t   pid;
} *slottab;
static maketab = 1;

void
makeslottab()
{
	proc_t *prp, pr;
	struct pid pid;
	struct syment *practive;
	register i;

	maketab = 0;
	if(!(practive = symsrch("practive")))
		fatal("practive not found in symbol table\n");

	/*
	 * Make it reuseable
	 */
	if(slottab != (struct procslot *)NULL)
		free((char *)slottab);

	slottab = (struct procslot*)malloc(vbuf.v_proc*sizeof(struct procslot));
	if(slottab == (struct procslot *)NULL)
		fatal("can't allocate slot table\n");

	for (i = 0; i < vbuf.v_proc; i++)
		slottab[i].p = 0;
	readmem((long)practive->n_value, 1, -1,(char *)&prp, sizeof (proc_t *),
		"practive");
	for (; prp != NULL; prp = pr.p_next) {
		readmem(prp, 1, -1,(char *)&pr, sizeof (struct proc),
			"proc table");
		readmem(pr.p_pidp, 1, -1,(char *)&pid, sizeof (struct pid),
			"pid table");
		i = pid.pid_prslot;
		if (i < 0 || i >= vbuf.v_proc)
			fatal("process slot out of bounds\n");
		slottab[i].p = prp;
		slottab[i].pid = pid.pid_id;
	}
}

pid_t
slot_to_pid(slot)
int slot;
{
	if (slot < 0 || slot >= vbuf.v_proc)
		return NULL;
	if (maketab)
		makeslottab();
	return slottab[slot].pid;
}

proc_t *
slot_to_proc(slot)
int slot;
{
	if (slot < 0 || slot >= vbuf.v_proc)
		return NULL;
	if (maketab)
		makeslottab();
	return slottab[slot].p;
}

/* convert proc address to proc slot */
int
proc_to_slot(addr)
long addr;
{
	int i;

	if (addr == NULL)
		return -1;

	if (maketab)
		makeslottab();

	for (i = 0; i < vbuf.v_proc; i++)
		if (slottab[i].p == (proc_t *)addr)
			return i;

	return -1;

}
		
/* get current process slot number */
int
getcurproc()
{
	long curproc;

	readmem((long)Curproc->n_value,1,-1,(char *)&curproc,
		sizeof curproc,"current process");
	return(proc_to_slot(curproc));
}


/* determine valid process table entry */
int
procntry(slot,prbuf)
int slot;
struct  proc *prbuf;
{
	long addr;

	if(slot == -1) 
		slot = getcurproc();

	if ((addr = (long)slot_to_proc(slot)) == NULL)
		error(" %d is not a valid process\n",slot);
	readmem((long)addr,1, -1,(char *)prbuf,sizeof (proc_t),"proc table");
}

/* argument processing from **args */
long
getargs(max,arg1,arg2)
int max;
long *arg1,*arg2;
{	
	struct syment *sp;
	long slot;

	/* range */
	if(strpbrk(args[optind],"-")) {
		range(max,arg1,arg2);
		return;
	}
	/* expression */
	if(*args[optind] == '(') {
		*arg1 = (eval(++args[optind]));
		return;
	}
	/* symbol */
	if((sp = symsrch(args[optind])) != NULL) {
		*arg1 = (sp->n_value);
		return;
	}

	/* slot number or address */
	if((slot = strcon(args[optind],'d')) == -1) {
		*arg1 = -1;
		return;
	}
	*arg1 = slot;
} 

/* get slot number in table from address */
int
getslot(addr,base,size,phys,max)
long addr,base,max;
int size,phys;
{
	long pbase;
	int slot;
	
	if(phys || !Virtmode) {
		pbase = vtop(base,-1);
		if(pbase == -1)
			error("%x is an invalid address\n",base);
		slot = ((addr&~MAINSTORE) - pbase) / size;
	}
	else slot = (addr - base) / size;
	if((slot >= 0) && (slot < max))
		return(slot);
	else return(-1);
}


/* file redirection */
int
redirect()
{
	int i;
	FILE *ofp;

	ofp = fp;
	if(opipe == 1) {
		fprintf(stdout,"file redirection (-w) option ignored\n");
		return;
	}
	if(fp = fopen(optarg,"a")) {
		fprintf(fp,"\n> ");
		for(i=0;i<argcnt;i++)
			fprintf(fp,"%s ",args[i]);
		fprintf(fp,"\n");
	}
	else {
		fp = ofp;
		error("unable to open %s\n",optarg);
	}
}


/*
 * putch() recognizes escape sequences as well as characters and prints the
 * character or equivalent action of the sequence.
 */
int
putch(c)
unsigned int  c;
{
	if(isgraph(c))
		fprintf(fp," %c ",c);
	else
		fprintf(fp," \\%#o ",c);
/*
	c &= 0377;
	if(c < 040 || c > 0176) {
		fprintf(fp,"\\");
		switch(c) {
		case '\0': c = '0'; break;
		case '\t': c = 't'; break;
		case '\n': c = 'n'; break;
		case '\r': c = 'r'; break;
		case '\b': c = 'b'; break;
		default:   c = '?'; break;
		}
	}
	else fprintf(fp," ");
	fprintf(fp,"%c ",c);
*/
}

/* sets process to input argument */
int
setproc()
{
	int slot;

	if((slot = strcon(optarg,'d')) == -1)
		error("\n");
	if((slot > vbuf.v_proc) || (slot < 0))
		error("%d out of range\n",slot);
	return(slot);
}

/* check to see if string is a symbol or a hexadecimal number */
int
isasymbol(string)
char *string;
{
	int i;

	for(i = (int)strlen(string); i > 0; i--)
		if(!isxdigit(*string++))
			return(1);
	return(0);
}


/* convert a string into a range of slot numbers */
int
range(max,begin,end)
int max;
long *begin,*end;
{
	int i,j,len,pos;
	char string[ARGLEN];
	char temp1[ARGLEN];
	char temp2[ARGLEN];

	strcpy(string,args[optind]);
	len = (int)strlen(string);
	if((*string == '-') || (string[len-1] == '-')){
		fprintf(fp,"%s is an invalid range\n",string);
		*begin = -1;
		return;
	}
	pos = strcspn(string,"-");
	for(i = 0; i < pos; i++)
		temp1[i] = string[i];
	temp1[i] = '\0';
	for(j = 0, i = pos+1; i < len; j++,i++)
		temp2[j] = string[i];
	temp2[j] = '\0';
	if((*begin = (int)stol(temp1)) == -1)
		return;
	if((*end = (int)stol(temp2)) == -1) {
		*begin = -1;
		return;
	}
	if(*begin > *end) {
		fprintf(fp,"%d-%d is an invalid range\n",*begin,*end);
		*begin = -1;
		return;
	}
	if(*end >= max) 
		*end = max - 1;
}


/*
 * print the privilege vector symbolically.
*/
void
prt_symbprvs(strp, vect)
char	*strp;
pvec_t	vect;
{
	register int	had_privs = 0;
	register int	prnt = 0;
	register int	k = 0;

	fprintf(fp,"%s", strp);
	if (all_privs_on(vect)) {
		fprintf(fp, "allprivs");
		++had_privs;
	}
	else {
		for (k = 0; k < NPRIVS; k++) {
			if ((pm_privbit(k) & vect)) {
				if ((prnt = prt_symprvs(k, prnt)))
					++had_privs;
			}
		}
	}
	if (!had_privs)
		fprintf(fp, "<none>");
	fprintf(fp,"\n");
}


/*
 * This routine actually does the symbolic printing.
*/
int
prt_symprvs(priv, printed)
int	priv;
int	printed;
{
	char	pbuf[BUFSIZ];
	char	*pbp = &pbuf[0];

	if (printed) {
		printed = 0;
		fprintf(fp,",");
	}
	if (privname(pbp, priv) != -1) {
		fprintf(fp,"%s", pbp);
		printed = 1;
	}
	return(printed);
}

/*
 * This routine returns 1 if all privileges are on in the privilege
 * vector passed.  Otherwise, it returns 0.
*/
int
all_privs_on(prv_vector)
pvec_t	prv_vector;
{
	register int	k;

	for (k = 0; k < NPRIVS; k++) {
		if (!(pm_privbit(k) & prv_vector))
			return(0);
	}
	return(k);
}

static memholecnt = 0;
static struct bootmem *memholes;

void
findmemholes()
{
	struct bootinfo binfo;
	struct syment *Binfo = NULL;
	int i;
	paddr_t lastmem;

	/* If cannot find bootinfo, no choice but to take chances */
	if((Binfo = symsrch("bootinfo")) == NULL) {
		fprintf(fp,"Unable to find bootinfo\nPhysical address to lseek translation may be inaccurate\n");
		return;
	}
	readmem(Binfo->n_value,1,-1,
		(char *)&binfo,sizeof(struct bootinfo),"bootinfo");
	if(binfo.memavailcnt <= 0) {
		fprintf(fp,"bootinfo corrupted\nPhysical address to lseek translation may be inaccurate\n");
		return;
	}

	/* get memory to match whole memavail array since that is
	  maximum number of wholes (including any whole at the beginning
	  of memory); unlikely will need that many, but memavailcnt is
	  by default 3 so it should not be a problem */
	memholes = (struct bootmem *) 
			malloc(binfo.memavailcnt * sizeof(struct bootmem));
	if(memholes == NULL) {
		fprintf(fp,"Unable to get needed memory\nPhysical address to lseek translation may be inaccurate\n");
		return;
	}

	/* find the holes; note that memavail is sorted by the kernel
		so we do not have to do the sort here */
	lastmem = 0;
	for(i = 0; i < binfo.memavailcnt; i++) {
		if(lastmem != binfo.memavail[i].base) {
			memholes[memholecnt].base = lastmem;
			memholes[memholecnt].extent = 
				binfo.memavail[i].base - lastmem;
			memholecnt++;
		}
		lastmem = binfo.memavail[i].base +
			   binfo.memavail[i].extent;
	}
}
			
paddr_t
adjmemhole(ip)
paddr_t ip;
{
	int i = 0;
	long op = ip;
	while(i < memholecnt) {
		if(ip > memholes[i].base) {
			op -= memholes[i].extent;
			i++;
		}
		else return(op);
	}
	return(op);
}

/* get address for symbol which may reside in loadable kernel module */
void
getdsym(loc, name, fatal)
struct syment **loc;
char *name;
boolean_t fatal;
{
	extern struct syment dummyent;
	if(active || !*loc)
		if((*loc = symsrch(name)) == NULL) {
			if(fatal)
				error("%s not found in symbol table\n",name);
			else
				*loc = &dummyent;
		}
}
