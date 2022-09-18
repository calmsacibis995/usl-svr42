/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:i386/local.c	1.2.10.2"

#include "optim.h" 	/* includes "defs" */
#include "optutil.h"
#include "sgs.h"
#include "paths.h"
#include "regal.h"
#include "storclass.h"
#include "debug.h"
#include <malloc.h>

#define ASDATE ""

static char * linptr, * line;	/* pointers to current statement. linptr
				   moves, line doesn't */
static int opn;			/* current operand, starting at 1 */ 
boolean found_safe_asm = false;
int in_safe_asm = false;
static int set_nopsets = false;
static int lineno = IDVAL;
static int first_src_line = 0;	/* Value is set each time we see a
				   ..FS.. label */
int auto_elim=0;		/* true if function has no autos
					   or all autos have been placed
					   into registers by raoptim.*/
int fp_removed =0;		/* true if remove_enter_leave was done */
boolean no_safe_asms = false;    /* for treating safe asms as none safe */
int double_aligned;

#ifdef STATS			/* statistics gathering */
/* Except for ndisc, these variables are not presently used */
/* but should be updated at the appropriate places in the future in */
/* order to do statistics gathering. */
int nusel = 0;		/* semantically useless instructions */
int nspinc = 0;		/* useless sp increments */
int nmc = 0;		/* move followed by compare */
int nmal = 0;		/* move followed by arithmetic or logical */
int nredcmp = 0;	/* redundant compares */
int nadpsh = 0;		/* addw or subw followed by push */
int nadmsh = 0;		/* addw or subw followed by mov */
int nadmv = 0;		/* replace addw or subw by mov */
extern int ndisc;		/* instructions discarded */
				/* only ndisc is properly updated at this time. */
#endif /* STATS */
#ifdef P5
#ifdef  STATISTICS
int first_run =1;
#endif
#endif

static int numauto;		/* number of bytes of automatic vars. */
static int numnreg;		/* number of registers */
static int autos_pres_el_done = false; /*enter_leave remove in presence of
										 automatic variables.           */

int optmode = ODEFAULT;		/* optimization mode */
enum CC_Mode ccmode = Transition;/* compilation mode */      

#ifdef LIVEDEAD
extern void	ldanal();
#endif

#define	BIGREGS	16	/* Any number that signifies many registers */
#define	BIGAUTO	(1<<30)	/* Any number that signifies many auto vars */


static char *optbl[] = {
	"aaa", "aad", "aam", "aas", "adc",
	"adcb", "adcl", "adcw", "addb", "addl",
	"addw", "andb", "andl", "andw", "arpl",
	"bound", "boundl", "boundw", "bsf", "bsfl",
	"bsfw", "bsr", "bsrl", "bsrw", "bswap",
	"bt", "btc", "btcl", "btcw", "btl",
	"btr", "btrl", "btrw", "bts", "btsl",
	"btsw", "btw", "call", "cbtw", "clc",
	"cld", "cli", "clr", "clrb", "clrl",
	"clrw", "cltd", "clts", "cmc", "cmp",
	"cmpb", "cmpl", "cmps", "cmpsb", "cmpsl",
	"cmpsw", "cmpw", "cmpxchg", "cmpxchgb", "cmpxchgl",
	"cmpxchgw", "cts", "cwtd", "cwtl", "daa",
	"das", "dec", "decb", "decl", "decw",
	"div", "divb", "divl", "divw", "enter",
	"esc", "f2xm1", "fabs", "fadd", "faddl",
	"faddp", "fadds", "fbld", "fbstp", "fchs",
	"fclex", "fcom", "fcoml", "fcomp", "fcompl",
	"fcompp", "fcomps", "fcoms", "fcos", "fdecstp",
	"fdiv", "fdivl", "fdivp", "fdivr", "fdivrl",
	"fdivrp", "fdivrs", "fdivs", "ffree", "fiadd",
	"fiaddl", "ficom", "ficoml", "ficomp", "ficompl",
	"fidiv", "fidivl", "fidivr", "fidivrl", "fild",
	"fildl", "fildll", "fimul", "fimull", "fincstp",
	"finit", "fist", "fistl", "fistp", "fistpl",
	"fistpll", "fisub", "fisubl", "fisubr", "fisubrl",
	"fld", "fld1", "fldcw", "fldenv", "fldl",
	"fldl2e", "fldl2t", "fldlg2", "fldln2", "fldpi",
	"flds", "fldt", "fldz", "fmul", "fmull",
	"fmulp", "fmuls", "fnclex", "fninit", "fnop",
	"fnsave", "fnstcw", "fnstenv", "fnstsw", "fpatan",
	"fprem", "fprem1", "fptan", "frndint", "frstor",
	"fsave", "fscale", "fsetpm", "fsin", "fsincos",
	"fsqrt", "fst", "fstcw", "fstenv", "fstl",
	"fstp", "fstpl", "fstps", "fstpt", "fsts",
	"fstsw", "fsub", "fsubl", "fsubp", "fsubr",
	"fsubrl", "fsubrp", "fsubrs", "fsubs", "ftst",
	"fucom", "fucomp", "fucompp", "fwait", "fxam",
	"fxch", "fxtract", "fyl2x", "fyl2xp1", "hlt",
	"idiv", "idivb", "idivl", "idivw", "imul",
	"imulb", "imull", "imulw", "in", "inb",
	"inc", "incb", "incl", "incw", "inl",
	"ins", "insb", "insl", "insw", "int",
	"into", "invd", "invlpg", "inw", "iret",
	"ja", "jae", "jb", "jbe", "jc",
	"jcxz", "je", "jg", "jge", "jl",
	"jle", "jmp", "jna", "jnae", "jnb",
	"jnbe", "jnc", "jne", "jng", "jnge",
	"jnl", "jnle", "jno", "jnp", "jns",
	"jnz", "jo", "jp", "jpe", "jpo",
	"js", "jz", "lahf", "lar", "larl",
	"larw", "lcall", "lds", "ldsl", "ldsw",
	"lea", "leal", "leave", "leaw", "les",
	"lesl", "lesw", "lfs", "lfsl", "lfsw",
	"lgdt", "lgs", "lgsl", "lgsw", "lidt",
	"ljmp", "lldt", "lmsw", "lock", "lods",
	"lodsb", "lodsl", "lodsw", "loop", "loope",
	"loopne", "loopnz", "loopz", "lret", "lsl",
	"lsll", "lslw", "lss", "lssl", "lssw",
	"ltr", "mov", "movb", "movl", "movs",
	"movsb", "movsbl", "movsbw", "movsl", "movsw",
	"movswl", "movw", "movzbl", "movzbw", "movzwl",
	"mul", "mulb", "mull", "mulw", "neg",
	"negb", "negl", "negw", "nop", "not",
	"notb", "notl", "notw", "or", "orb",
	"orl", "orw", "out", "outb", "outl",
	"outs", "outsb", "outsl", "outsw", "outw",
	"pop", "popa", "popal", "popaw", "popf",
	"popfl", "popfw", "popl", "popw", "push",
	"pusha", "pushal", "pushaw", "pushf", "pushfl",
	"pushfw", "pushl", "pushw", "rcl", "rclb",
	"rcll", "rclw", "rcr", "rcrb", "rcrl",
	"rcrw", "rep", "repe", "repne", "repnz",
	"repz", "ret", "rol", "rolb", "roll",
	"rolw", "ror", "rorb", "rorl", "rorw",
	"sahf", "sal", "salb", "sall", "salw",
	"sar", "sarb", "sarl", "sarw", "sbb",
	"sbbb", "sbbl", "sbbw", "scab", "scal",
	"scas", "scasb", "scasl", "scasw", "scaw",
	"scmp", "scmpb", "scmpl", "scmpw", "seta",
	"setae", "setb", "setbe", "setc", "sete",
	"setg", "setge", "setl", "setle", "setna",
	"setnae", "setnb", "setnbe", "setnc", "setne",
	"setng", "setnl", "setnle", "setno", "setnp",
	"setns", "setnz", "seto", "setp", "setpe",
	"setpo", "sets", "setz", "sgdt", "shl",
	"shlb", "shld", "shldl", "shldw", "shll",
	"shlw", "shr", "shrb", "shrd", "shrdl",
	"shrdw", "shrl", "shrw", "sidt", "sldt",
	"slod", "slodb", "slodl", "slodw", "smov",
	"smovb", "smovl", "smovw", "smsw", "ssca",
	"sscab", "sscal", "sscaw", "ssto", "sstob",
	"sstol", "sstow", "stc", "std", "sti",
	"stos", "stosb", "stosl", "stosw", "str",
	"sub", "subb", "subl", "subw", "test",
	"testb", "testl", "testw", "verr", "verw",
	"wait", "wbinvd", "xadd", "xaddb", "xaddl",
	"xaddw", "xchg", "xchgb", "xchgl", "xchgw",
	"xlat", "xor", "xorb", "xorl", "xorw"
};

# define numops (sizeof(optbl) / sizeof(char *)) /* number of mnemonics */

extern char *getenv();
#ifdef DEBUG
static char *get_label();
#endif
extern char *tempnam();
static FILE *tmpopen();
static FILE *stmpfile;	/* Temporary storage for switch tables that are in the text
			 * section. The tables are collected and printed at
			 * the end of a function. */
static char tmpname[50];	/* name of file for storing switch */
static FILE *atmpfile; /* Temporary storage for input while scanning for presence of
			 * #ASM in function if aflag on */
static char atmpname[50];

int Aflag = false;		/* suppress alignment of 16 */
int asmflag = false;	/* indicates whether an 'asm' has been encountered */
static long asmotell;	/* location in the output file where the last function ended */
int aflag = false;	/* indicates whether -a flag was entered, if true
			   no atempt will be made to optimize functions
			   containing ASMs */

			/* Next three flags apply globally -- set on
			   command line by  -_r, -_e, -_s */

static int never_register_allocation = false;
static int never_enter_leave = false; 
static int never_stack_cleanup = false;

			/*
			** Next three flags apply on a per function basis:
			** suppress ( only under -Xt ) register allocation
			** if fn contains a setjmp.  Similarly, suppress
			** enter/leave removal if a function contains
			** a longjmp (for -Xa and -Xt as well)
			** Lastly,  stop the cleanup of the stack 
			** for a given function if it calls alloca().
			** These flags reinitialized for each function
			** to the value of the global flags declared above.
			*/

static int suppress_register_allocation = false;
/* static */ int suppress_enter_leave = false;
static int suppress_stack_cleanup = false;

int tflag = false; 	/* suppress removal of redundant loads (rmrdld())? */
int Tflag = false; 	/* trace removal of redundand loads routine? */
static boolean identflag = false;/* output .ident string? */

static boolean new_csline,get_first_src_line = false, get_lineno = false;
static boolean inswitch = false;/* currently in switch table */

enum Section section=CStext, prev_section;      /* Control sections. */

#ifdef IMPIL
boolean swflag = false;		/* switch table appears in function */
extern void pcdecode();
#endif /* IMPIL */
int rvregs = 1;			/* # registers used to hold return value */
#ifdef pre50
#ifdef IMPREGAL
boolean comp_provided_weights = false;
				/* indicates comp provided #REGAL weights */
#endif /* IMPREGAL */
#endif
/* function declarations */
extern int unlink();

#ifdef IMPIL
extern void ilinit();		/* initialization for in-line substitution */
extern void ilmark();
extern void ilstat();
extern void ilfile();
#endif /* IMPIL */
#ifdef IMPREGAL
extern struct regal *lookup_regal();
extern int raoptim();
#endif /* IMPREGAL */
extern void rmrdld();
extern void wrapup();	/* print unprocessed text and update statistics file */
extern void dstats();

extern char *getstmnt(); /* also called from debug.c */
/* Input function (getline()), lifted from HALO */
static char *ExtendCopyBuf(); /* Used only by getstmnt */
static void eliminate_ebp();
void set_refs_to_blocks(); /* connect switch tables names to blocks */
static void hide_safe_asm();
static void recover_safe_asm();
static void reset_crs();
#ifdef DEBUG
static int verify_safe_asm();
#endif

int plookup();			/* look up pseudo op code */

static void parse_com();
#ifdef IMPREGAL
static void parse_regal();		/* Parses a #REGAL line. */
static void parse_alias();		/* Parses a #ALIAS line. */
#endif /* IMPREGAL */
static void parse_op();
static void parse_pseudo();
static int remove_enter_leave();
static enum Section parse_section();	/* Parse the argument to .section. */
static int lookup();			/* look up op code ordinal */
static void asmopen();	/* opens the temp file for storing input while looking for 'asm' */
int putasm();	/* writes to temp file for 'asm' processing */
static void asmchk();	/* checks for 'asm' in files and undoes code movement */
static void setautoreg();	/* set indicator for number of autos/regs */
static void reordtop();
static void reordbot();
static void copyafter();
static void putstr();
static void prstr();
static void filter();
void sets_and_uses();
static void stack_cleanup();
void peep();
int imull();
void leave_opt();
static void forget_bi();
static void clean_label_sets_uses();
extern int check_double(), replace_consts();
extern SWITCH_TBL * get_base_label();
extern void bldgr();
extern void schedule(), backalign(), sw_pipeline(), rm_tmpvars(), rmrdpp(),
			postpeep(), ebboptim();
extern char *mystrstr();
extern int i486_opts;
#ifdef P5
extern int i586_opts;
#endif
static char *switch_label;

	void
yylex()
{
	extern int cflag;			/* enable/disable common tail */
	register char * s;			/* temporary string ptr */
	register char * start;		/* another temporary string ptr */
	int first_label = 1;

#ifdef IMPCOMTAIL
	if( optmode == OSIZE )
	  cflag = 0;
	else
#endif /* IMPCOMTAIL */
	  cflag = -1;	/* turn off common tail merging:  improve
			** speed, not space. */

#ifdef IMPIL
	if (! i486_opts)
		ilinit();
#endif /* IMPIL */

	if( aflag ) asmopen();		/* open temp file for asm treatment */
	line = linptr = getstmnt();
	while ( linptr 
		&& ( !aflag || putasm(linptr) ))
	{
	if (section == CSdebug) {	/* hook to handle an entire debug
					   section.  We do this, because
					   parse_debug wants to do its own
					   I/O and parsing */
		parse_debug(linptr);
	}		
	else {
		switch (*linptr)		/* dispatch on first char in line */
		{
		case '\n':			/* ignore blank lines */
		case '\0':
			break;
			
		case CC:			/* check for special comments */
			parse_com(linptr);
			break;
	
		default:			/* label */
			s = strchr(linptr,':');	/* find : */
			if (s == NULL) 		/* error if not found */
			  fatal("Bad input format\n");
			*s++ = '\0';		/* change : to null, skip past */
			if (section == CStext) {
			if(strncmp(linptr,"..LN",4) == 0)  /* linenumber label */
				get_lineno = true;
			else if(strncmp(linptr,"..FS",4) == 0)  
			/* 1st source line for function */
				get_first_src_line = true;
			else {
			if(first_label) {
				save_text_begin_label(linptr);
				first_label = 0;
			}
					applbl(linptr,s-linptr);  /* append label node */
					lastnode->uniqid = IDVAL;
			}
			}
			else if(section != CSline) {
				printf("%s:\n",linptr);
				addref(linptr,(unsigned int)(s-linptr),NULL);
				if (inswitch) { /*the jump table label */
					switch_label = getspace(LABELSIZE);
					(void) strcpy(switch_label,linptr);
				}
			}
		linptr = s;
			continue;			/* next iteration */
	/* handle more usual case of line beginning with space/tab */
		case ' ':
		case '\t':
			s = linptr;
				/* linptr now points to beginning of line after label */
			SkipWhite(s);
			
			switch(*s)			/* dispatch on next character */
			{
			case '\0':			/* line of white space */
			case CC:			/* comment */
			case '\n':			/* new line */
			break;			/* ignore */
			
			case '.':			/* start of pseudo-op */
			parse_pseudo(s);	/* do pseudo-ops */
			break;
	
			default:			/* normal instruction not */
			if (section != CStext) { /* in .text section this is weird case */
			if (section != CSline) printf("\t%s\n",s); /* just write line */
			}
			else			/* normal instruction in text */
			{
				char lastchar;
				int opc, m;
				for (start = s; isalnum(*s); s++)
				;		/* skip over instruction mnemonic */
						/* start points to mnemonic */
	
				lastchar = *s;
				*s = '\0';	/* demarcate with null */
	
				if ((opc = lookup(start,&m)) == OTHER)
				saveop(0,strdup(start),0,opc);
						/* save funny instruction */
				else
				saveop(0,optbl[m],0,opc);
						/* save normal inst */
				*s = lastchar;
	
				SkipWhite(s);
			/* Check if this is a call to a local label,
			   if so we need to prevent it from being
			   removed.  This only arises when the compiler
			   is generating PIC.
			   
			   Also check for call to setjmp if compiling
			   in transition mode (-Xt).  Such calls must
			   disable register allocation. */

			if (opc == JMP && (s[0] == '*')
			 &&  (s[1] == '%')) {
				suppress_enter_leave = true;
			}
			if( opc == CALL || opc == LCALL ) {
				if(*s == '.' ) {
				   char *p = s+1; /* point at 'L' */
				   while ( isalnum(*p) ) p++;
				   *p = '\0';
				   addref(s,(unsigned int)(p-s+1),NULL);
				}
				else if(*s == 's' && (strcmp(s,"setjmp")==0)) {
					suppress_enter_leave = true;
					if (ccmode == Transition )
					suppress_register_allocation = true;
				}
				else if(*s == 'l' && (strcmp(s,"longjmp") == 0)) {
					suppress_enter_leave = true;
				}
				else if(*s == 'a' && (strcmp(s,"alloca") == 0)) {
					suppress_stack_cleanup = true,
					suppress_enter_leave = true;
				}
				/* hack to avoid confusing this function,
				   which makes assumptions about the frame
				   pointer of its caller */
			}
				opn = 1;		/* now save operands */
				parse_op(s);
			}
			break;
			}   /* end space/tab case */
		break;
		}	/* end first character switch */
	}	/* end if (section ...) ... else */
	line = linptr = getstmnt();
	}   /* end while */
	return;				/* just exit */
}

	static void
parse_com(s)
register char *s;
{
 extern void fatal();

	switch(*++s){
	case 'A':	/* #ALIAS, #ASM, or #ASMEND ? */
#ifdef IMPREGAL
		if(strncmp(s,"ALIAS",5) == 0){
			parse_alias(s+5);
			break;
		}
		else
#endif /* IMPREGAL */
		if(strncmp(s,"ASM",3) != 0) 
			break;
			/* since #ASM processing chews up everything to #ASMEND ... */
		if(strncmp(s+3,"END",3) == 0)
			if (!in_safe_asm)
				fatal("parse_com: unbalanced asm.\n");
			else {
				in_safe_asm = false;
				saveop(0,strdup(s-1),0,SAFE_ASM);
				break;
			}
		asmflag = true;		/* #ASM appears in function */
		/* here if #ASM<stuff> */
		s = linptr;
		do {
			while(*linptr != '\0') linptr++;
			/* *linptr++ = '\0'; take this out (psp) */
			saveop(0,strdup(s),0,ASMS);
			if(strncmp(s+1,"ASMEND",6) == 0) {
				break;
			}
		} while ( (line=linptr=getstmnt()) != NULL
			&& ( !aflag || putasm(s) ));
		if(linptr == NULL)
			fatal("parse_com: unbalanced asm.\n");
		break;
#ifdef IMPREGAL
	 case 'L':	/* #LOOP ? */
		if(strncmp(s,"LOOP",4) != 0) break;
		s += 4;		/* enter it into the list. */
		SkipWhite(s);	/* Skip white space to retrieve arg.	*/
		saveop(0,s,3,LCMT);	/* append loop-cmt node with its arg */
		break;
	 case 'P':	/* POS_OFFSET ? */
		if (strncmp(s,"POS_OFFSET",10) != 0) break;
		if (lastnode == NULL) break;
		s += 10;
		SkipWhite(s);
		lastnode->ebp_offset = atoi(s);
		break;
	 case 'R':	/* #REGAL ? */
		if(strncmp(s,"REGAL",5) != 0) break;
		parse_regal(s+5);	/*Read NAQ info in #REGAL stmt*/
		break;
#endif /* IMPREGAL */
	 case 'S':	/* #SWBEG or #SWEND ? */
		if(strncmp(s,"SWBEG",5) == 0)
		  {
			start_switch_tbl();
			inswitch = true; 	/* Now in a switch table. */
#ifdef	IMPIL
			swflag = true;
#endif /* IMPIL */
			  }
		else if(strncmp(s,"SWEND",5) == 0) {
			end_switch_tbl(switch_label);
			inswitch = false;	/*Out of switch table.*/
		} else if (!strncmp(s,"SAFE_ASM",8)) {
			if (no_safe_asms) { /*treat safe asms as asms */
				asmflag = true;
				s = linptr;
				do {
					while(*linptr != '\0') linptr++;
						saveop(0,strdup(s),0,ASMS);
						if(strncmp(s+1,"ASMEND",6) == 0) {
							break;
					}
				} while ( (line=linptr=getstmnt()) != NULL
					&& ( !aflag || putasm(s) ));
				if(linptr == NULL)
					fatal("parse_com: unbalanced asm.\n");
				break;
			} else {
				saveop(0,strdup(s-1),0,SAFE_ASM);
				in_safe_asm = true;
				found_safe_asm = true;
			}
		}
		break;
	 case 'T':
/* #ifdef IMPIL */
		if (lastnode == NULL)	/* #TMPSRET ? */
			break;
		if (strncmp(s,"TMPSRET",7) == 0)
			if (lastnode->op == PUSHL)	/* identify push of addr */
				saveop(2,"TMPSRET\0",8,TSRET);	/* for struct funct ret. */
		break;
/* #endif /* IMPIL */
	 case 'V':	/* #VOL_OPND ? */
		if(strncmp(s,"VOL_OPND", 8) != 0) break;
		s += 8;
		SkipWhite(s);
		if (lastnode == NULL)
			break;
		while( *s != '\0' && *s != '\n' ){
			/* set volatile bit in operand */
			mark_vol_opnd(lastnode,*s - '0');
			s++;
			SkipWhite(s);
			if( *s == ',' ){ 
				s++;
				SkipWhite(s);
			}
		}
		break;
	} /* end of switch(*++s) */
}

#ifdef IMPREGAL
	static void
parse_regal( p ) 		/* read #REGAL comments */
register char *p;

{
#ifdef pre50
 extern boolean comp_provided_weights;
 int estim;
#endif
 register struct regal *r;
 extern void fatal();
 char *q;
 char *name;
 int len,rt;

	/* the formats recognized are:
	 * 1) #REGAL <wt> NODBL
	 * 2) #REGAL <wt> {AUTO,PARAM,EXTERN,EXTDEF,STATEXT,STATLOC} <name> <size> [FP]
	 *
	 * However only the following subset is used by the 386 optimizer:
	 * #REGAL <wt> {AUTO,PARAM} <name> <size>
	 * and the other formats are ignored.
	 */

			/* scan to estimator and read it */
	SkipWhite(p);
#ifdef pre50
	estim = strtol( p, &q, 0 );
#else
	strtol( p, &q, 0 );
#endif
	if(p == q)
		fatal("parse_regal: missing weight field\n");
	p = q;


			/* scan to storage class and read it */
	SkipWhite(p);
	rt = SCLNULL;
	switch(*p){
	case 'A':
		if(strncmp(p,"AUTO",4) == 0)
			rt = AUTO;
		break;
	case 'E':
		if(strncmp(p,"EXTDEF",6) == 0)
			return;			/* ignore */
		else if(strncmp(p,"EXTERN",6) == 0)
			return;			/* ignore */
		break;
	case 'N':
		if(strncmp(p,"NODBL",5) == 0)
			return;			/* ignore */
		break;
	case 'P':
		if(strncmp(p,"PARAM",5) == 0)
			rt = PARAM;
		break;
	case 'S':
		if(strncmp(p,"STATEXT",7) == 0)
			return;			/* ignore */
		else if(strncmp(p,"STATLOC",7) == 0)
			return;			/* ignore */
		break;
	}
	if( rt == SCLNULL )
		fatal( "parse_regal:  illegal #REGAL type:\n\t%s\n",p);

			/* scan to name and delimit it */
	FindWhite(p);
	SkipWhite(p);
	name = p;
	FindWhite(p);
	*p++ = '\0';
	if ( ((q = (strchr(name,'('))) == NULL) ||
		 (strncmp(q,"(%ebp)",6) != 0) )
		fatal("parse_regal:  illegal name in #REGAL\n");
			/* only "nnn(%ebp) currently allowed */

			/* scan to length in bytes */
	SkipWhite(p);
	len = strtol( p, &q, 0 );
	if(p == q)
		fatal("parse_regal: missing length\n");
	p = q;

			/* read floating point indicator */
	SkipWhite(p);
	if( (p[0] != '\0' && p[0] == 'F') &&
	   (p[1] != '\0' && p[1] == 'P') )
		return;			/* ignore */

			/* install regal node */
	if ((r = lookup_regal(name,true)) == NULL)
		fatal("parse_regal:  trouble installing a regal\n");
	r->rglscl = rt;
	r->rgllen = len;
#ifdef pre50
	if (estim) {
		/* If the compiler provides a non-zero weight in any */
		/* #REGAL statement, then we assume that it has computed */
		/* all the weight information.  Otherwise we must compute */
		/* the information in Estim(). */
		comp_provided_weights = true;
		r->rglestim = estim;
	}
#endif
 return;
	  }

	static void
parse_alias(s)
register char *s;

{
 extern struct regal *new_alias();
 char *name,*t;
 int len;
 register struct regal *a;

			/* scan to name and delimit it */
 SkipWhite(s);
 name = s;
 FindWhite(s);
 *s++ = '\0';
 if (strchr(name,'(') == NULL)
   return;		/* catch only AUTOs and PARAMs - "n(%ebp)" */


			/* scan to length in bytes */
 SkipWhite(s);
 len = strtol( s, &t, 0 );
 if(s == t)
   fatal("parse_alias: missing length\n");
 s = t;

			/* read floating point indicator */
 SkipWhite(s);
 if( s[0] == 'F' && s[1] == 'P' )
   return;

			/* append node to list of aliases */
 a = new_alias();
 a->rglname = strdup(name);
 a->rgllen = len;

 return;
}
#endif	/* IMPREGAL */

	static void
parse_op(s)
	register char *s;
{
	register char *t;
	register more, prflg;

	more=true;
	prflg=false;

	while (more) { /* scan to end of operand and save */	
	if (! *s )
		return;
	SkipWhite(s);
	t = s;		/* remember effective operand start */

	while(*s && (*s != ',') ) { /* read to end of this operand */
		switch ( *s++ ) {
		case CC: /* process comment */
		if( isret( lastnode ) ) rvregs = *s - '0';
		*s = *(s-1) = '\0'; /* that's what the old code did ??? */
		break;
		case '(':
		prflg = true;
		break;
		case ')':
		prflg = false;
		break;
		}
		if(prflg && (*s==',')) s++;
	}

	if(*s ) *s = '\0';	/* change ',' to '\0' */
	else more = false;	/* found the end of instruction */
	/* now s points to null at end of operand */
	saveop(opn++, t, (++s)-t, 0);
	if (is_label_text(t) && inswitch)
		addref(t,(unsigned int)(s-t),NULL);
	} /* while(more) */
	lastnode->uniqid = lineno;
	lineno = IDVAL;
}



	static void
parse_pseudo (s)
	register char *s;		/* points at pseudo-op */
{
#define NEWSECTION(x) prev_section=section; section=(x);
	void add_enter_leave();		/* in optutil.c */
	register int pop;		/* pseudo-op code */
	char savechar;			/* saved char that follows pseudo */
	char * word = s;		/* pointer to beginning of pseudo */
	int m;
	enum Section save;		/* for swaps */


	FindWhite(s);			/* scan to white space char */
	savechar = *s;			/* mark end of pseudo-op */
	*s = '\0';
	pop = plookup(word);		/* identify pseudo-op */
	*s = savechar;			/* restore saved character */

	if(section == CSline) {
		if ( new_csline && pop == (int) FOURBYTE) {
			new_csline = false;
			if (get_first_src_line)	{
				get_first_src_line = false;
				first_src_line = atoi(s);
			} else if (get_lineno) {	
				lineno = atoi(s);
				get_lineno = false; 
			} 
		}
		if (pop != (int) PREVIOUS)  return;
		else if (! init_line_flag) init_line_section();
		/* assume .line entries are simple mindedly bracketted
		   by .section .line ... .previous 
		   The .previous will get printed by the code that
		   comes next. */
	}
	if (section != CStext) {
		switch(pop) { /* check for section changes and .long,
				 otherwise just print it. */
		case TEXT:
			NEWSECTION(CStext);
			printf("%s\n", line);
		/* don't print: is this right??? (psp) */
		break;
		case PREVIOUS:
			save=section;
			section=prev_section;
			prev_section=save;
			printf("%s\n", linptr); /* flush to output */
		break;
		case SECTION:
			printf("%s\n", line);
			prev_section=section;
			section=parse_section(s);
		break;
		case LONG: /* FALLTHRU */
		break;
		default:
		printf("%s\n",line);
		break;
		} /* switch */
		if(pop != (int) LONG) return;
	} /* non text sections done ( except for .long ) */

	switch (pop) {			/* dispatch on pseudo-op in text */
					/* and on .long in any section */
		case TEXT:
			NEWSECTION(CStext);
			break;
		case BYTE:
			printf("%s\n",line);
			break;
		case LOCAL:
		case GLOBL:
			appfl(line, strlength(line)); /* filter node - why */
			break;
		case FIL:
		case BSS: /* assume .bss has an argument,
				 this will get .bss <no arg> wrong */
			printf("%s\n", line);
			break;
		case ALIGN:
			m=strlength(line);
			if (inswitch) {
				saveop(0, line+1, m-1, OTHER);
				opn = 1;
				lastnode->uniqid = lineno;
				lineno = IDVAL;
			}
			else
				appfl(line, m);
			break;
		case SET:
			m=strlength(line);
			appfl(line, m);
			break;
		case DATA:
			printf("%s\n", line);
			NEWSECTION(CSdata);
			break;
		case SECTION:
			printf("%s\n", line);
			prev_section=section;
			section=parse_section(s);
			break;
		case PREVIOUS:
			save=section;
			section=prev_section;
			prev_section=save;
			printf("%s\n", linptr); /* flush to output */
			break;
		case LONG:
			SkipWhite(s);

/* we have to deal with whether .long is within a switch (SWBEG/SWEND) or
** not, and whether or not it appears in a data section.
*/

			if (inswitch) {		/* always add reference */
				char *t;
				if(t = strchr(s,'-')) {
				/* Assume we are looking at .long .Lxx-.Lyy,
				   where .Lyy is the target of a call ( PIC
				   code ).  So we make the first label
				   hard.  The second label is handled
				   when the call is parsed. */
				*t = '\0';
				addref(s,(unsigned int)(t-s+1),NULL);
				*t = '-';
				}
				else
				addref(s,(unsigned int)(strlen(s)+1),switch_label);
					/* we assume only one arg per .long */
			}

			if (section != CStext) 	/* not in .text, flush to output */
				printf("%s\n",linptr); /* print line */
			else if (inswitch)  	/* text and switch */
				putstr(linptr);	/* flush to special file */
			else {			/* text, not in switch */
				saveop(0,".long",6,OTHER);
				opn = 1;	/* doing first operand */
				parse_op(s);
			}
			break;

		/*
		* ELF new pseudo_op
		*
		* do optimizations, then spit out the 
		* input .size line which is assumed to be
		* of the form .size foo,.-foo ( no intervening white space )
		*/
		case SIZE: /* For clarity, this code should be placed in a separate function. */
		{
			char * ptr;
			SkipWhite(s);
			ptr=strchr(s,',');
			if(ptr == NULL) { /* not the right format */
				printf("%s\n",line);
				break;
			}
			if(*(ptr+1) != '.' || *(ptr+2) != '-') {
				printf("%s\n",line);
				break;
			}
			*ptr = '\0'; /* s points to function name, now */
			if(strcmp(s,ptr+3) != 0) { 
			/* Some kind of .size in text unknown to us. */
				*ptr = ',';
				printf("%s\n",line);
				fatal("parse_pseudo(): unrecognized .size directive\n");
				break;
			}
			if (first_src_line != 0) {
				print_FS_line_info(first_src_line,s);
				first_src_line = 0;
			}
			*ptr = ',';
		}
			printf("	.text\n");
#ifdef DEBUG
			{
				void ratable();
				static int d;
				int min, max;
				char * str;
				++d;
				str=getenv("max");
				if (str)
					max=atoi(str);
				else
					max=9999;
				str=getenv("min");
				if (str)
					min=atoi(str);
				else
					min=0;
				if (d> max || d < min) {
					fprintf(stderr,"SKIP %d %s ", d,get_label());
					ratable();
					goto noopt;
				}
				else if (min)
					fprintf(stderr,"%d %s ", d,get_label());
			}
#endif
			if( !asmflag || !aflag  ) {
				sets_and_uses();
				reordtop();
				if(!suppress_enter_leave && !asmflag) {
					if (double_aligned = check_double())
						suppress_enter_leave = true;
				} else 
					double_aligned = false; 
				hide_safe_asm();
				setautoreg();	/* set numnreg and numauto */
				recover_safe_asm();
				auto_elim= !numauto;
#ifdef IMPREGAL
				if(!suppress_register_allocation) {
					hide_safe_asm();
					numnreg = raoptim(numnreg, numauto, &auto_elim);
					recover_safe_asm();
				} else {
					void ratable();
					ratable();		
					suppress_register_allocation = never_register_allocation;
				}
				filter();
#endif /* IMPREGAL */
#ifdef DEBUG
				(void ) verify_safe_asm();
#endif /* DEBUG */
#ifdef LIVEDEAD
				if (suppress_stack_cleanup)
					suppress_stack_cleanup = never_stack_cleanup;
				else {
					hide_safe_asm();
					stack_cleanup();
					recover_safe_asm();
				}
#endif /* LIVEDEAD */
#ifdef IMPIL
				if (! i486_opts)
					ilmark();
#endif /* IMPIL */
				loop_index();
#ifdef P5
#ifdef  STATISTICS
				first_run = 1;
#endif
#endif
				optim(); /* do standard improvements */
				clean_label_sets_uses();
				const_index();
				ebboptim(ZERO_PROP); /*extended basic block zero value*/
				peep();	 /* do peephole improvements */
				if (imull()) {
					sets_and_uses();
				}
				hide_safe_asm();
				rm_tmpvars();/*  temp /REGAL removing */
				recover_safe_asm();
				ebboptim(ZERO_PROP);/* extended basic block zero tracing */
				replace_consts(true);
				rmrdld();/* remove redundant loads   */
			/* make one more pass	     */
				const_index();
				optim(); /* do standard improvements */
				clean_label_sets_uses();
				ebboptim(COPY_PROP); /*extended basic block copy elimination*/
				peep();	 /* do peephole improvements */
				if (replace_consts(false)) {
					ldanal();
					ebboptim(COPY_PROP);
				}
				rmrdpp(); /*remove redundant push /pop*/
#ifdef IMPIL
				if (! i486_opts)
					ilstat(numnreg, numauto);
#endif /* IMPIL */
				hide_safe_asm();
				if (suppress_enter_leave || asmflag) {
					if (double_aligned) {
						fp_removed = remove_enter_leave(numauto);
						if (!fp_removed) {
							optim();
							clean_label_sets_uses();
							fp_removed = remove_enter_leave(numauto);
							if (!fp_removed)
								fatal("cant eliminate frame pointer.\n");
						}
					}
					auto_elim = 0;
				} else {
					fp_removed = remove_enter_leave(numauto);
					if (!fp_removed) {
						optim();
						clean_label_sets_uses();
						fp_removed = remove_enter_leave(numauto);
						if (!fp_removed) {
							fatal("cant eliminate frame pointer.\n");
						}
					}
				}
				recover_safe_asm();
				forget_bi();
				
				/* argument indictes whether to remove the
				   "leave" statements from the function */
				reordbot(fp_removed);
				if (i486_opts) {
					schedule();  /*instruction scheduler*/
					postpeep();  /*post schedule peepholes*/
					sw_pipeline();  /*post schedule sw pileline for FSTP */
#ifdef P5
					if ( ! i586_opts)
#endif
						backalign();   /* .backalign support */
				}
			} /* if( !asmflag || !aflag ) ) */

#ifdef DEBUG
			noopt: 
#endif
			{
				long start, end;
				if (fp_removed)
					start= ftell(stdout);
				else if (i486_opts)
					leave_opt();
/* enable inserting "nops" that change condition codes for .align
** disable if ASMS exist in the function.
*/
				if (! set_nopsets) {
					if (!( asmflag || aflag) ) {
						printf("\n	.nopsets	\"cc\"");
						set_nopsets = true;
					}
				} 
				else if (asmflag || aflag ) {
						printf("\n	.nopsets");
						set_nopsets = false;
				}
				if (!Aflag)
					printf("\n	.align	16\n");
				else 
					printf("\n");
				reset_crs();
				prtext();
				if (fp_removed) {/* if enter leave done */
					end = ftell(stdout);
				/* records the first and last bytes of
				   the function containing the enter leave
				   optimization. This info used to suppress
				   inlining in a function that has no
				   enter-leave
				*/
					add_enter_leave(start,end);
				}
			}

#ifndef IMPIL
				numauto = 0;  /* reset for next rtn */
#else
				if (i486_opts)
					numauto = 0;  /* reset for next rtn */
#endif /* IMPIL */
			prstr();
			printf("%s\n", line);
			if( aflag ) asmchk();
			asmflag = false;
			rvregs = 1; /* re-init ret value reg cnt */
			autos_pres_el_done = false; /*reset*/
			fp_removed = false;
			suppress_enter_leave = never_enter_leave; /*reset*/
			found_safe_asm = false; /*reset */
			init();
			break;
		default:			/* all unrecognized text
						** pseudo-ops
						*/
			if (! inswitch)
				printf("%s\n", linptr); /* flush to output */
			else
				putstr(linptr);	/* in switch:  to
						** special file
						*/
			break;
	}
}

	static void
filter() /* print FILTER nodes and remove from list, also
		clean up loop comment nodes, normally done in
		raoptim. */
{
	register NODE *p;
	for(ALLN(p)) {
	if (p->op == FILTER) {
		(void) puts(p->ops[0]);
		DELNODE(p);
	}
	else if(suppress_register_allocation && p->op == LCMT)
		DELNODE(p);
	}
}
	static enum Section
parse_section(s)
register char * s; /* string argument of .section */
{
	char savechar, *sname;

	SkipWhite(s);
	if(*s == NULL) fatal("parse_section(): no section name\n");
	for(sname = s; *s != '\0'; ++s)
			if(*s == ',' || isspace(*s)) break;
	savechar = *s;
	*s = '\0';
	/* If this is too slow, look at first and last char
	   to identify string. */
	if(strcmp(sname, ".debug") == 0)
		section=CSdebug;
	else if(strcmp(sname, ".line") == 0) {
		section=CSline;
		new_csline = true;
	}
	else if(strcmp(sname, ".text") == 0)
			section = CStext;
	else if(strcmp(sname, ".data") == 0)
			section = CSdata;
	else if(strcmp(sname, ".rodata") == 0)
			section = CSrodata;
	else if(strcmp(sname, ".data1") == 0)
			section = CSdata1;
	else if(strcmp(sname, ".bss") == 0)
		section=CSbss;
	else	/* unknown section */
			section = CSother;
	*s = savechar;
	return section;
}
	int
plookup(s)	/* look up pseudo op code */
	char *s;

{
	static char *pops[POTHER] = {
			"2byte",
			"4byte",
			"align",
			"bcd",
			"bss",
			"byte",
			"comm",
			"data",
			"double",
			"even",
			"ext",
			"file",
			"float",
			"globl",
			"ident",
			"lcomm",
			"local",
			"long",
			"previous",
			"section",
			"set",
			"size",
			"string",
			"text",
			"type",
			"value",
			"version",
			"weak",
			"word", 
			"zero",
	};

	register int l,r,m,x; /* temps for binary search */

	l = 0; r = (int) POTHER-1;
	while (l <= r) {
		m = (l+r)/2;
		x = strcmp(s+1, pops[m]); /* s points at . */
		if (x<0) r = m-1;
		else if (x>0) l = m+1;
		else return(m);
	}
	fatal("plookup(): illegal pseudo_op: %s\n",s);	
/* NOTREACHED */
}


	void
yyinit(flags) char * flags; {

	section = CStext;	/* Assembler assumes the current section 
				   is .text at the top of the file. */

	for (; *flags != '\0'; flags++) {
		switch( *flags ) {
		case 'V':			/* Want version ID.	*/
			fprintf(stderr,"optim: %s%s\n",SGU_PKG,SGU_REL);
			break;
		case 'a':
			aflag = true;
			break;
		case 'A':
			Aflag = true;
			break;
		case 't':	/* suppress rmrdld() */
			tflag = true;
			break;
		case 'T':	/* trace rmrdld */
			Tflag = true;
			break;
		default:
			fprintf(stderr,"Optimizer: invalid flag '%c'\n",*flags);
		}
	}
}

int pic_flag;	/* set by this function only */
int ieee_flag = true;

#ifndef I386_OPTS
int i486_opts = true;	/* set by this function only */
#else
int i486_opts = false;	/* set by this function only */
#endif
#ifdef P5
int i586_opts = false;	/* set by this function only */
#endif

	char **
yysflgs( p ) char **p; /* parse flags with sub fields */
{
	extern void fatal();
	register char *s;	/* points to sub field */

	s = *p + 1;
	if (*s == NULL) {
		switch(**p) {
		case 'K': case 'Q': case 'X': case 'S': case 'y':
			fatal("-%c suboption missing\n",**p);
			break;
		}
	}
	switch( **p ) {
	case 'K':
		if( *s == 's' )
			switch( *++s ) {
			case 'd': optmode = OSPEED; return( p );
			case 'z': optmode = OSIZE; return( p );
			}
		else if ((*s == 'P' && strcmp(s,"PIC") == 0) ||
			 (*s == 'p' && strcmp(s,"pic") == 0)) {
			pic_flag=true;
			return p;
		}
		else if (*s == 'i' && strcmp(s,"ieee") == 0) {
			ieee_flag = true;
			return p;
		}
		else if (*s == 'n' && strcmp(s,"noieee") == 0) {
			ieee_flag = false;
			return p;
		}
		fatal("-K suboption invalid\n");
		break;
	case 'Q':
		switch( *s ) {
		case 'y': identflag = true; break;
		case 'n': identflag = false; break;
			default: 
			fatal("-Q suboption invalid\n");
			break;
		}
		return( p );
	case 'X':	/* set ansi flags */
		switch( *s ){
		case 't': ccmode = Transition; break;
		case 'a': ccmode = Ansi; break;
		case 'c': ccmode = Conform; break;
		default: 
			fatal("-X requires 't', 'a', or 'c'\n");
			break;
		}
		return( p );
	case '_':	/* suppress optimizations (for debugging) */
		SkipWhite(s);
		for(;*s;s++)
			switch(*s) {
			case 'r':
				never_register_allocation = true;
				suppress_register_allocation = true;
				break;
			case 's':
				never_stack_cleanup = true;
				suppress_stack_cleanup = true;
				break;
			case 'e':
				never_enter_leave = true;
				suppress_enter_leave = true;
				break;
			default:
				fatal("-_ requires 'r' or 'e' or 's'\n");
				break;
			}
		return(p);
#ifdef IMPIL
	case 'y':
		pcdecode( s );
		return( p );
#endif /* IMPIL */
	case '3':
		if (!strcmp(s,"86")) {
			i486_opts = false;
#ifdef P5
			i586_opts = false;
#endif
			return p;
		}
		fatal("-386 suboption invalid\n");
		break;
	case '4':
		if (!strcmp(s,"86")) {
			i486_opts = true;
#ifdef P5
			i586_opts = false;
#endif
			return p;
		}
		fatal("-486 suboption invalid\n");
		break;
#ifdef P5
	case '5':
		if (!strcmp(s,"86")) {
			i486_opts = true;
			i586_opts = true;
			return p;
		}
		fatal("-586 suboption invalid\n");
#else
	case '5':
	fatal("not compiled for 586, invalid option 586\n");
#endif
	default:
		return( p );
	}
/* NOTREACHED */
}

	static int
lookup(op,indx)		/* look up op code and return opcode ordinal */
	 char *op;		/* mnemonic name */
	 int *indx;		/* returned index into optab[] */
{
#define ALIAS 0xF0000
	register int f,l,om,m,x;

	static int ocode[numops] = {
	AAA, AAD, AAM, AAS, ALIAS + 2,
	ADCB, ADCL, ADCW, ADDB, ADDL,
	ADDW, ANDB, ANDL, ANDW, ARPL,
	BOUND, BOUNDL, BOUNDW, ALIAS + 1, BSFL,
	BSFW, ALIAS + 1, BSRL, BSRW, BSWAP,
	ALIAS + 4, ALIAS + 1, BTCL, BTCW, BTL,
	ALIAS + 1, BTRL, BTRW, ALIAS + 1, BTSL,
	BTSW, BTW, CALL, CBTW, CLC,
	CLD, CLI, ALIAS + 2, CLRB, CLRL,
	CLRW, CLTD, CLTS, CMC, ALIAS + 2,
	CMPB, CMPL, ALIAS + 2, CMPSB, CMPSL,
	CMPSW, CMPW, ALIAS + 2, CMPXCHGB, CMPXCHGL,
	CMPXCHGW, CTS, CWTD, CWTL, DAA,
	DAS, ALIAS + 2, DECB, DECL, DECW,
	ALIAS + 2, DIVB, DIVL, DIVW, ENTER,
	ESC, F2XM1, FABS, FADD, FADDL,
	FADDP, FADDS, FBLD, FBSTP, FCHS,
	FCLEX, FCOM, FCOML, FCOMP, FCOMPL,
	FCOMPP, FCOMPS, FCOMS, FCOS, FDECSTP,
	FDIV, FDIVL, FDIVP, FDIVR, FDIVRL,
	FDIVRP, FDIVRS, FDIVS, FFREE, FIADD,
	FIADDL, FICOM, FICOML, FICOMP, FICOMPL,
	FIDIV, FIDIVL, FIDIVR, FIDIVRL, FILD,
	FILDL, FILDLL, FIMUL, FIMULL, FINCSTP,
	FINIT, FIST, FISTL, FISTP, FISTPL,
	FISTPLL, FISUB, FISUBL, FISUBR, FISUBRL,
	FLD, FLD1, FLDCW, FLDENV, FLDL,
	FLDL2E, FLDL2T, FLDLG2, FLDLN2, FLDPI,
	FLDS, FLDT, FLDZ, FMUL, FMULL,
	FMULP, FMULS, FNCLEX, FNINIT, FNOP,
	FNSAVE, FNSTCW, FNSTENV, FNSTSW, FPATAN,
	FPREM, FPREM1, FPTAN, FRNDINT, FRSTOR,
	FSAVE, FSCALE, FSETPM, FSIN, FSINCOS,
	FSQRT, FST, FSTCW, FSTENV, FSTL,
	FSTP, FSTPL, FSTPS, FSTPT, FSTS,
	FSTSW, FSUB, FSUBL, FSUBP, FSUBR,
	FSUBRL, FSUBRP, FSUBRS, FSUBS, FTST,
	FUCOM, FUCOMP, FUCOMPP, FWAIT, FXAM,
	FXCH, FXTRACT, FYL2X, FYL2XP1, HLT,
	ALIAS + 2, IDIVB, IDIVL, IDIVW, ALIAS + 2,
	IMULB, IMULL, IMULW, ALIAS + 6, INB,
	ALIAS + 2, INCB, INCL, INCW, INL,
	INS, INSB, INSL, INSW, INT,
	INTO, INVD, INVLPG, INW, IRET,
	JA, JAE, JB, JBE, JC,
	JCXZ, JE, JG, JGE, JL,
	JLE, JMP, JNA, JNAE, JNB,
	JNBE, JNC, JNE, JNG, JNGE,
	JNL, JNLE, JNO, JNP, JNS,
	JNZ, JO, JP, JPE, JPO,
	JS, JZ, LAHF, LAR, LARL,
	LARW, LCALL, LDS, LDSL, LDSW,
	ALIAS + 1, LEAL, LEAVE, LEAW, LES,
	LESL, LESW, ALIAS + 1, LFSL, LFSW,
	LGDT, ALIAS + 1, LGSL, LGSW, LIDT,
	LJMP, LLDT, LMSW, LOCK, ALIAS + 2,
	LODSB, LODSL, LODSW, LOOP, LOOPE,
	LOOPNE, LOOPNZ, LOOPZ, LRET, LSL,
	LSLL, LSLW, ALIAS + 1, LSSL, LSSW,
	LTR, ALIAS + 2, MOVB, MOVL, ALIAS + 4,
	ALIAS + 1, MOVSBL, MOVSBW, MOVSL, ALIAS + 1,
	MOVSWL, MOVW, MOVZBL, MOVZBW, MOVZWL,
	ALIAS + 2, MULB, MULL, MULW, ALIAS + 2,
	NEGB, NEGL, NEGW, NOP, ALIAS + 2,
	NOTB, NOTL, NOTW, ALIAS + 2, ORB,
	ORL, ORW, ALIAS + 2, OUTB, OUTL,
	OUTS, OUTSB, OUTSL, OUTSW, OUTW,
	ALIAS + 2, POPA, POPAL, POPAW, POPF,
	POPFL, POPFW, POPL, POPW, ALIAS + 7,
	PUSHA, PUSHAL, PUSHAW, PUSHF, PUSHFL,
	PUSHFW, PUSHL, PUSHW, ALIAS + 2, RCLB,
	RCLL, RCLW, ALIAS + 2, RCRB, RCRL,
	RCRW, REP, REPE, REPNE, REPNZ,
	REPZ, RET, ALIAS + 2, ROLB, ROLL,
	ROLW, ALIAS + 2, RORB, RORL, RORW,
	SAHF, ALIAS + 2, SALB, SALL, SALW,
	ALIAS + 2, SARB, SARL, SARW, ALIAS + 2,
	SBBB, SBBL, SBBW, SCAB, SCAL,
	ALIAS + 2, SCASB, SCASL, SCASW, SCAW,
	ALIAS + 2, SCMPB, SCMPL, SCMPW, SETA,
	SETAE, SETB, SETBE, SETC, SETE,
	SETG, SETGE, SETL, SETLE, SETNA,
	SETNAE, SETNB, SETNBE, SETNC, SETNE,
	SETNG, SETNL, SETNLE, SETNO, SETNP,
	SETNS, SETNZ, SETO, SETP, SETPE,
	SETPO, SETS, SETZ, SGDT, ALIAS + 5,
	SHLB, ALIAS + 1, SHLDL, SHLDW, SHLL,
	SHLW, ALIAS + 5, SHRB, ALIAS + 1, SHRDL,
	SHRDW, SHRL, SHRW, SIDT, SLDT,
	ALIAS + 2, SLODB, SLODL, SLODW, ALIAS + 2,
	SMOVB, SMOVL, SMOVW, SMSW, ALIAS + 2,
	SSCAB, SSCAL, SSCAW, ALIAS + 2, SSTOB,
	SSTOL, SSTOW, STC, STD, STI,
	ALIAS + 2, STOSB, STOSL, STOSW, STR,
	ALIAS + 2, SUBB, SUBL, SUBW, ALIAS + 2,
	TESTB, TESTL, TESTW, VERR, VERW,
	WAIT, WBINVD, ALIAS + 2, XADDB, XADDL,
	XADDW, ALIAS + 2, XCHGB, XCHGL, XCHGW,
	XLAT, ALIAS + 2, XORB, XORL, XORW
};

	f = 0;
	l = numops;
	om = 0;
	m = (f+l)/2;
	while (m != om) {
		x = strcmp(op,optbl[m]);
		if (x == 0) {
			if (ocode[m] & ALIAS) /* aliased to other opcode. */
				m = (ocode[m] & ~ALIAS) + m;
			*indx = m;
			return(ocode[m]);
			  }
		else if (x < 0)
			l = m-1;
			else
			f = m+1;
		om = m;
		m = (f+l)/2;
		}
	*indx = m;
	return(OTHER);
	}

	static FILE *
tmpopen() {
	strcpy( tmpname, tempnam( TMPDIR, "25cc" ) );
	return( fopen( tmpname, "w" ) );
	}

	static void
putstr(string)   char *string; {
	/* Place string from the text section into a temporary file
	 * to be output at the end of the function */

	if( stmpfile == NULL )
		stmpfile = tmpopen();
	fprintf(stmpfile,"%s",string);
	}

	static void
prstr() {
/* print the strings stored in stmpfile at the end of the function */

	if( stmpfile != NULL ) {
		register int c;

		stmpfile = freopen( tmpname, "r", stmpfile );
		if( stmpfile != NULL )
			while( (c=getc(stmpfile)) != EOF )
				putchar( c );
		else
			{
			fprintf( stderr, "optimizer error: ");
			fprintf( stderr, "lost temp file\n");
			}
		(void) fclose( stmpfile );	/* close and delete file */
		unlink( tmpname );
		stmpfile = NULL;
		}
}

/* opens the temp file for storing input while looking for 'asm' */
	static void
asmopen() {
	strcpy( atmpname, tempnam( TMPDIR, "asm" ) );
	atmpfile = fopen( atmpname, "w" );
	asmotell = ftell( stdout );
}

/* writes to temp file for 'asm' processing */
	int
putasm( lptr )
char *lptr;
{
	if(section == CSdebug) return true;
	if (*lptr == ' ') *lptr = '\t';
	if(fputs( lptr, atmpfile ) == EOF) return 0;
	else return (fputc('\n',atmpfile) != EOF);
}

/* checks for 'asm' in files and undoes code movement */
	static void
asmchk() 
{
	register c;
	long endotell;
#ifdef IMPREGAL
	extern int vars;
#endif /* IMPREGAL */

	if( asmflag ) {
		if( freopen( atmpname, "r", atmpfile ) != NULL ) {
			endotell = ftell( stdout );
			fseek( stdout, asmotell, 0 ); /* This is okay as long 
				as IMPIL is defined because it 
				is not really stdout, it is the file used by
				in-line expansion.  That file is still used, 
				even when in-line expansion is suppressed. 
				If IMPIL is not defined, optim will not work
				correctly to a terminal, but it will work
				correctly to a file.  
				This should be fixed.  */
			while( ( c = getc( atmpfile ) ) != EOF ) putchar( c );
			while( ftell( stdout ) < endotell ) printf( "!\n" );/* ? */
		}
		else fprintf( stderr, "optimizer error: asm temp file lost\n" );
	}
	freopen( atmpname, "w", atmpfile );
	asmotell = ftell( stdout );
#ifdef IMPREGAL
	vars=0; 	/* reinitialize for global reg allocation */
#endif /* IMPREGAL */
}

	void
putp(p,c) register NODE *p; char *c; {  /* insert pointer into jump node */

	p->op1 = c;
}

#ifdef STATISTICS
/* statistics of the scheduler.                  */
extern int sflag;
extern int orig_agi, final_agi, agi_work;
extern int orig_conc, final_conc, conc_work;
extern int total_inst, on_X86, on_jalu, pairables;
extern int org_total,org_on_U,org_on_V,org_pairable;
extern int fin_total,fin_on_U,fin_on_V,fin_pairable;
extern int try_peep , done_peep;
int peep_ratio;
/* statistics of the extended basic block improvements. */
extern int ndels , nrrs , ch2xor;
#endif

	void
dstats() { /* print stats on machine dependent optimizations */

#if STATS
	fprintf(stderr,"%d semantically useless instructions(s)\n", nusel);
	fprintf(stderr,"%d useless move(s) before compare(s)\n", nmc);
	fprintf(stderr,"%d merged move-arithmetic/logical(s)\n", nmal);
	fprintf(stderr,"%d useless sp increment(s)\n", nspinc);
	fprintf(stderr,"%d redundant compare(s)\n", nredcmp);
#ifdef STATISTICS
   fprintf(stderr,"ridiculous indices removed: %d\n",nrrs);
   fprintf(stderr,"ridiculous zero assignments changed: %d\n",ch2xor);
   fprintf(stderr,"redundant zero assinments deleted: %d\n",ndels);
#ifdef P5
   if (i586_opts) {
	 if (org_total) {
	   if (try_peep == 0)
		 fprintf(stderr,"no chance to try peep\n");
	   else {
		 peep_ratio = (100 * done_peep) / try_peep;
		 fprintf(stderr,"tried: %d  done: %d  that's %d percent\n",
					  try_peep,done_peep,peep_ratio);
	   }
	   fprintf(stderr,"\noriginal code pairing stats:\n");
	   fprintf(stderr,"total instructions: %d\n",org_total);
	   fprintf(stderr,"pairable instructions: %d\n",org_pairable);
	   peep_ratio = (org_on_U * 100) / org_total;
	   fprintf(stderr,"on X86: %d percent: %d\n",org_on_U,peep_ratio);
	   peep_ratio = (org_on_V * 100) / org_total;
	   fprintf(stderr,"on JALU: %d precent: %d\n",org_on_V,peep_ratio);
	   if (org_pairable == 0)
		 fprintf(stderr,"a program without pairables\n");
	   else {
		 peep_ratio = (org_on_V * 100) / org_pairable;
		 fprintf(stderr,"pairing ratio: %d\n", peep_ratio);
	   }
	   fprintf(stderr,"\nafter schedule stats:\n");
	   fprintf(stderr,"total instructions: %d\n",total_inst);
	   fprintf(stderr,"pairable instructions: %d\n",pairables);
	   peep_ratio = (on_X86 * 100) / total_inst;
	   fprintf(stderr,"on X86: %d percent: %d\n",on_X86,peep_ratio);
	   peep_ratio = (on_jalu * 100) / total_inst;
	   fprintf(stderr,"on JALU: %d precent: %d\n",on_jalu,peep_ratio);
	   if (pairables == 0)
		 fprintf(stderr,"a program without pairables\n");
	   else {
		 peep_ratio = (on_jalu * 100) / pairables;
		 fprintf(stderr,"pairing ratio: %d\n", peep_ratio);
		}
		fprintf(stderr,"\nafter schedule stats again:\n");
		fprintf(stderr,"total instructions: %d\n",fin_total);
		fprintf(stderr,"pairable instructions: %d\n",fin_pairable);
		peep_ratio = (fin_on_U * 100) / fin_total;
		fprintf(stderr,"on X86: %d percent: %d\n",fin_on_U,peep_ratio);
		peep_ratio = (fin_on_V * 100) / fin_total;
		fprintf(stderr,"on JALU: %d precent: %d\n",fin_on_V,peep_ratio);
		if (fin_pairable == 0)
		  fprintf(stderr,"a program without pairables\n");
		else {
		peep_ratio = (fin_on_V * 100) / fin_pairable;
		fprintf(stderr,"pairing ratio: %d\n", peep_ratio);
		}
	  }
	}
	else { /* not i586_opts, we're doing i486 work. */
#endif
		if (orig_agi == 0)
		  fprintf(stderr,"a program without agi's!\n");
		else {
		  agi_work = ((orig_agi - final_agi) *100)/orig_agi;
		  fprintf(stderr,
		  "original agis: %d  final agis: %d  prevention ratio %d \
difference %d \n "
			,orig_agi,final_agi,agi_work,orig_agi-final_agi);
		}
		if (orig_conc == 0)
		  fprintf(stderr,"a program without concurrent's!\n");
		else {
		  conc_work = ((orig_conc - final_conc) *100)/orig_conc;
		  fprintf(stderr,
		  "original concurrents: %d  final concurrents: %d  prevention\
ratio %d difference %d \n "
		  ,orig_conc,final_conc,conc_work,orig_conc-final_conc);
		}
#ifdef P5
   }
#endif
#endif
#endif /* STATS */
}

	void
wrapup() { /* print unprocessed text and update statistics file */

#ifdef STATS
	FILE *sp;
	int mc,mal,usel,spinc,redcmp,disc,inst;
#endif


	if (n0.forw != NULL) {
		printf("	.text\n");
		filter();
		prtext();
		prstr();
		if(identflag)			/* Output ident info.	*/
			printf("\t.ident\t\"optim: %s\"\n",SGU_REL);
		}
#ifdef IMPIL
	if (!i486_opts)
		ilfile();
#endif /* IMPIL */


	if( aflag ) {
		(void) fclose( atmpfile );	/* close and delete file */
		unlink( atmpname );
	}

	exit_line_section(); /* print the last .line entry */
	print_debug(); /* Dump out the debugging info for the whole file. */

#ifdef STATS
	/* STATSFILE should be defined to be a string representing a pathname */
	sp = fopen(STATSFILE,"r");
	if (sp != NULL) {
		fscanf(sp, "%d %d %d %d %d %d %d",
		   &mc,&mal,&usel,&spinc,&redcmp,&disc,&inst);
		fclose(sp);
		mc += nmc;
		mal += nmal;
		usel += nusel;
		spinc += nspinc;
		redcmp += nredcmp;
		disc += ndisc;
		inst += ninst;
		sp = fopen(STATSFILE,"w");
		if (sp != NULL) {
			fprintf(sp, "%d %d %d %d %d %d %d \n",
			   mc,mal,usel,spinc,redcmp,disc,inst);
			fclose(sp);
			}
		}
#endif /* STATS */

	}


	static void
setautoreg() { /* set indicator for number of autos/regs */

	register NODE *p;

	numauto = 0;
	for(p = n0.forw; p != &ntail && (is_debug_label(p) || !islabel(p)) ; p = p->forw)
		;	/* Skip over initial .text and stuff */
	if ( 		/* this line assumes left->right execution */
		 p == NULL
		 ||  p == &ntail
		 ||  !islabel(p)
		 ||  !ishl(p)
	   ) {					/* Can't determine sizes */
		numauto = BIGAUTO;
		numnreg = BIGREGS;
		fatal("cant find numauto\n");
		return;
	}
	if (p->forw->op == POPL && usesvar("%eax",p->forw->op1) &&
		p->forw->forw->op == XCHGL && usesvar("%eax",p->forw->forw->op1) &&
		strcmp("0(%esp)",p->forw->forw->op2) == 0)
		p = p->forw->forw;
	if ( 		/* this line assumes left->right execution */
			 (p = p->forw)->op != PUSHL
		 ||  strcmp(p->op1,"%ebp") != 0
		 ||  (p = p->forw)->op != MOVL
		 ||  strcmp(p->op1,"%esp") != 0
		 ||  strcmp(p->op2,"%ebp") != 0
	   ) {					/* Can't determine sizes */
		numauto = BIGAUTO;
		numnreg = BIGREGS;
		fatal("cant find numauto\n");
		return;
	}
	p = p->forw;
	if (p->op == SUBL && p->op1[0] == '$' && samereg(p->op2,"%esp")) {
		numauto = atoi(&p->op1[1]);	/* remember # of bytes */
		if (double_aligned) {
			numauto += 4;
			p->op1 = getspace(ADDLSIZE);
			sprintf(p->op1,"$%d",numauto);
		}
		p = p->forw;
	} else if (p->op == PUSHL && strcmp(p->op1,"%eax") == 0) {
		numauto = INTSIZE;
		if (double_aligned) {
			numauto += 4;
			chgop(p,SUBL,"subl");
			p->op1 = getspace(ADDLSIZE);
			sprintf(p->op1,"%d",numauto);
			p->op2 = "%esp";
			new_sets_uses(p);
		}
		p = p->forw;
	}
	numnreg = 0;
	while ( p != &ntail
		&&  p->op == PUSHL
		&&  ( strcmp(p->op1,"%ebx") == 0
		   || strcmp(p->op1,"%esi") == 0
		   || strcmp(p->op1,"%ebi") == 0
		   || strcmp(p->op1,"%edi") == 0 ) ) {
		numnreg++;
		p = p->forw;
	}
	return;
	}

/*
 * The following code reorders the Top of a C subroutine.
 * So that the 2 extraneous jumps are removed.
 */
	static void
reordtop()
{
	register NODE *pt, *st, *end;

	for(pt = n0.forw; pt != &ntail; pt = pt->forw)
		if ( islabel(pt) && !is_debug_label(pt))
			break;
	if ( islabel(pt) && ishl(pt) ) {
		pt = pt->forw;
		if ( !isuncbr(pt) && pt->op != JMP )
			return;
		if ( pt->op1[0] != '.' )
			return;
		if ( !islabel(pt->forw) || ishl(pt->forw) )
			return;
		for (st = pt->forw; st != &ntail; st = st->forw) {
			if ( !islabel(st) )
				continue;
			if ( strcmp(pt->op1, st->opcode) != 0 )
				continue;
			/* Found the beginning of code to move */
			for( end = st->forw;
				end != &ntail && !isuncbr(end);
				end = end->forw )
				;
			if ( end == &ntail )
				return;
			if ( end->op != JMP ||
				strcmp(end->op1,pt->forw->opcode) != 0 )
				return;

			/* Relink various sections */
			st->back->forw = end->forw;
			end->forw->back = st->back;

			pt->back->forw = st->forw;
			st->forw->back = pt->back;

			end->forw = pt->forw;
			pt->forw->back = end;
			return;			/* Real Exit */
		}
	}
}

/* The code generated for returns is as follows:
 * 	jmp Ln
 *	...
 *
 * Ln:
 * 	popl %ebx	/optional
 * 	popl %esi	/optional
 * 	popl %edi	/optional
 * 	leave
 * 	ret
 *
 * This optimization replaces unconditional jumps to the return
 * sequence with the actual sequence.
 */
	static void
reordbot(no_leave) int no_leave;
{
	NODE	*retlp,		/* ptr to first label node in return seq. */
		*firstp;	/* ptr to first non-label node in return seq. */
	register NODE *endp,	/* ptr to last node in return seq. */
		*lp,		/* ptr to label nodes in return seq. */
	*new, *p, *q;		/* ptrs to nodes in function */
	endp = &ntail;
	do {			/* scan backwards looking for RET */
		endp = endp->back;
		if (endp == &n0)
			return;
	} while (endp->op != RET);	
				/* Now endp points to RET node */
	firstp = endp->back;
	if (firstp->op != LEAVE)
		return;
	/*if remove enter leave was done, then two cases:
	**if there were autos, then restore esp by adding
	**the number of autos to it.
	**if there were no autos then delete the leave instruction.
	**if remove enter leave was not done, the leave instruction
	**stays.
	*/
	if (no_leave) {
		if (autos_pres_el_done) {
			chgop(firstp,ADDL,"addl");
			firstp->op1 = getspace(ADDLSIZE);
			/* UNDOCUMENTED CHANGE */
			for (p = firstp->back; ; p = p->back)
				if (isbr(p) || islabel(p) || p->op == ASMS 
				  || (p->sets | p->uses) & ESP)
				   break;
			if (p->op == ADDL && p->op1[0] == '$' && 
										   p->sets == (ESP | CONCODES)) {
				sprintf(firstp->op1,"$%d",numauto + atoi(&p->op1[1]));
				DELNODE(p);
			} else
				sprintf(firstp->op1,"$%d",numauto);
			firstp->op2 = "%esp";
			firstp->uses = ESP;
			firstp->sets = ESP | CONCODES;
			if (double_aligned) {
				new = insert(firstp);
				chgop(new,POPL,"popl");
				new->op1 = "%ebp";
				new->uses = ESP;
				new->sets = ESP | EBP;
			}
		} else {
			DELNODE(firstp);
			firstp=endp;
		}
	}
#ifdef pre50
	if (is_debug_label(firstp->back->op))/* Skip .def-generated hard label */
		firstp = firstp->back;	/* left for debugging */
#endif
	while (firstp->back->op == POPL) /* Restores of saved registers */
		firstp = firstp->back;
				/* Now firstp points to first non-label */
	retlp = firstp->back;
	if (!islabel(retlp) || ishl(retlp))
		return;
	do
		retlp = retlp->back;
	while (islabel(retlp) && !ishl(retlp));
				/* Now retlp points before first label */

	for (p = ntail.back; p != &n0; p = p->back) {
				/* Scan backwds for JMP's */
		if (p->op != JMP)
			continue;
		for (lp = retlp->forw; lp != firstp; lp = lp->forw) {
				/* Scan thru labels in ret seq. */
			if (strcmp(p->op1,lp->opcode) == 0) {
				/* found a JMP to the ret. seq., so copy
				it over the JMP node */
				copyafter(firstp,endp,p);
				q = p;
				p = p->back;
				DELNODE(q);
				break;
			}
		}
	}
}
	static void
copyafter(first,last,current) 
NODE *first; 
register NODE *last;
NODE *current;
{
	register NODE *p, *q;
	q = current;
	for(p = first; p != last->forw; p = p->forw) {
		if (is_debug_label(p)) continue;
		q = insert(q);
		chgop(q,(int)p->op, p->opcode);
		q->op1 = p->op1;
		q->op2 = p->op2;
		q->uses = p->uses;
		q->sets = p->sets;
		q->uniqid = IDVAL;
	}
}


#define ENTER_LEAVE_THRESH 30

#ifdef DEBUG
static char *get_label() {
	NODE *pn;
	for( ALLN( pn ) ) {
		/* first label has the function name */
		if( islabel( pn ) && !is_debug_label(pn) )
			return pn->opcode;
	}
}
#endif


static void do_remove_enter_leave();
static process_body();
static process_block();
static process_header();

/* driver which conditionally performs enter-leave removal */
static
remove_enter_leave(autos_pres) int autos_pres;
{
	NODE *pn;
	int depth;	/* change in stack due to register saves */
#ifdef DEBUG
	static int dcount;
	static int lcount= -2;
	static int hcount=999999;


	if (lcount == -2) {
		char *h=getenv("hcount");
		char *l=getenv("lcount");
		if (h) hcount=atoi(h);
		if (l) {
			lcount=atoi(l);
			fprintf(stderr,"count=[%d,%d]\n",lcount,hcount);
		}
		else   lcount = -1;
	}
	++dcount;
	if ( dcount < lcount || dcount > hcount)
		return;
	if (lcount != -1)
		fprintf(stderr,"%d(%s) ", dcount, get_label());
#endif


	/* check if function prolog is as expected */
	switch (process_header(&pn,&depth,autos_pres,2)) {
		case 0:
			return 0;
		case 2:
			return 1;
	}

	/* check if rest of function as as expected, mark statements that contain
	   frame pointer references; these will be rewritten as stack pointer
	   references
	*/
	if (process_body(pn)) {
	/* rewrite the prolog and body of a function, epilog is rewritten
		   in reordbot
		*/
		do_remove_enter_leave(depth);
		if (!auto_elim || double_aligned)
			autos_pres_el_done = true;
		return 1;
	}
	return 0;
}

static int
const_at(p) NODE *p;
{
unsigned int reg;
	reg = scanreg(p->op1,false);
	for(; p!= &n0; p = p->back) {
		if (p->sets & reg) {
			if  ( *p->op1 == '$')
				return atoi(p->op1+1);
			else reg = p->uses;
		}
	}
	return 0;
}/*end const_at*/

/* given a node that effects the stack pointer, returns the change caused by this
   function
*/

static
process_push_pop(node)
NODE *node;
{
	switch(node->op) {
	case PUSHL: case PUSHFL:  case PUSHF:
		return 4;
	case PUSHW: case PUSHFW:
		return 2;
	case POPL:  case POPFL: case POPF:
		return -4;
	case POPW:  case POPFW:
		return -2;
	case LEAL:
		return -atoi(node->op1);
	case SUBL:
		if (isreg(node->op1) && samereg(node->op2,"%esp"))
			return (const_at(node));
		if (*node->op1 == '$' && node->op1[1])
			return (atoi(node->op1+1));
	/* FALLTHRU */
	case ADDL:
		if (isreg(node->op1) && samereg(node->op2,"%esp"))
			return (-const_at(node));
		if (*node->op1 == '$' && node->op1[1])
			return (-atoi(node->op1+1));
	/* FALLTHRU */
	case LEAVE:
		return 0;
	/* FALLTHRU */
	default:
	fatal("process_push_pop(): improper push pop\n");
	}/*end switch*/
	/* NOTREACHED */
}
/* rewrite operands of the format:
	*n(%ebp)
	n(%ebp)
	n(%ebp,r)
to use the stack pointer
*/
static char *
rewrite(pn,rand,depth)
NODE *pn;
char *rand;	/* operand to rewrite */
int depth;	/* stack growth since entry into the called address, this
		   includes saving registers, and any growth causes by pushing
		   arguments for function calls
		*/
{
	char *star="";
	char *lparen;
	int offset;
	int is_auto;
	if (*rand == '*') {
		++rand;
		star = "*";
	}
	if (*rand == '-' || (pn->ebp_offset != 0))
		is_auto = true;
	else
		is_auto = false;
	if (double_aligned && is_auto) {
		if (*star == '*')
			rand--;
		return rand;
	}
	offset=strtol(rand,&lparen,10);

	/* check if operand is of the desired format */
	if (!lparen || lparen[0] != '(' || lparen[1]!='%' || lparen[2]!='e' 
		|| lparen[3]!='b' || lparen[4] != 'p') {
		if (*star == '*')
			rand--;
		return rand;
	}

	pn->ebp_offset = offset;
	pn->uses = pn->uses &~EBP | ESP;
	rand = getspace(NEWSIZE);
	lparen[3] = 's'; /*make it ebp for the moment*/

	/* the new offset. The original offset assumes the function return address
	   and old frame pointer are on stack. With this optimization only the
	   function return address are on stack. This is the reason for the "-4".
	   depth
	*/
	if (double_aligned) depth += 4;
	offset += depth;
	if (!is_auto) offset -= 4;
	if (!auto_elim) offset += numauto;

	sprintf(rand,"%s%d%s",star,offset,lparen);
	lparen[3] = 'b'; /*retrieve */
	return rand;
}
static void
do_remove_enter_leave(depth) int depth; /* depth of stack after prolog */
{
	NODE *pn, *pf = NULL;
	for (pn=n0.forw; pn != 0; pn = pf) {
		pf = pn->forw;
		if (pn->extra == REMOVE) {
			DELNODE(pn);
		}
		else if (pn->extra > REMOVE) {
			if (pn->op1)
				pn->op1 = rewrite(pn,pn->op1,pn->extra+depth);
			if (pn->op2)
				pn->op2 = rewrite(pn,pn->op2,pn->extra+depth);
			if (pn->op3)
				pn->op3 = rewrite(pn,pn->op3,pn->extra+depth);
		
		}
	}
}
/* For each instruction after the prolog, this routine calculates ther
   runtime stack depth before this instruction. We can make this
   calculation, because we  require that the execution of each basic
   block of the function has a net change of 0 on the stack. This
   routine returns 1 if this requirement is met, and the function does
   not have too many insturctions (> limit).  We except the basic
   block containing the function's epilog from this requirement.
*/
/* The function was changed, Jan, 1991. It is more liberal now.
   It scans the flow graph rather than the flat function body, and
   therefore it can calculate the run time depth at an instruction
   without the requirement that the stack level is unchanged by the
   basic blocks. The limit on the number of instruction was also
   removed.
*/
#define CLEAR		0
#define TOUCHED		1
#define PROCESSED	2
static 
process_body (pn)
register NODE *pn; /*first instruction after the prolog */
{
	BLOCK *b;

#ifdef BBOPTIM
	bldgr(false,false);
#else
	bldgr(false);
#endif
	set_refs_to_blocks(); /* connect switch tables nanes to blocks */
	for (b = b0.next; b ; b = b->next) { /*init*/
		b->marked = CLEAR;
		b->entry_depth = b->exit_depth =0;
	}
	for (b = b0.next; b ; b = b->next)
		if (b->marked != PROCESSED) {
			if (process_block(b,pn) == 0)
				return 0;
		}

	return 1;
}


static int
process_block(b,pn) BLOCK *b; NODE *pn;
{
BLOCK *b1;
NODE *p;
REF *r;
SWITCH_TBL *sw;

int i;
boolean tmpsret = false;

		b->marked = PROCESSED;
		b->exit_depth = b->entry_depth;
		p = b == b0.next ? pn : b->firstn;
		for ( ; p != b->lastn->forw; p = p->forw) {
			if (p->uses&EBP && p->op != PUSHA)
				p->extra = (short) b->exit_depth;
			if (tmpsret && p->op == CALL) {
					p->op3 = "/TMPSRET";
					tmpsret = false;
					b->exit_depth -= 4;
			}				
			if (p->sets&ESP  ) {
				b->exit_depth += process_push_pop(p);
				if (p->extra == TMPSRET)
					tmpsret = true;
			}
			if (p->op == CALL && p->forw->op == LABEL  
			  && (strcmp(p->op1,p->forw->opcode) == 0))
				b->exit_depth += 4;
		}
		if (b->nextl) {
			if (b->nextl->marked > CLEAR) {
				if (b->nextl->entry_depth != b->exit_depth) {
					return 0;
				} else if (b->nextl->marked != PROCESSED) {
					if (process_block(b->nextl,pn) == 0)
						return 0;
				}
			} else {
				b->nextl->entry_depth = b->exit_depth;
				b->nextl->marked = TOUCHED;
				if (process_block(b->nextl,pn) == 0)
					return 0;
			}
		}
		if (b->nextr) {
			if (b->nextr->marked > CLEAR) {
				if (b->nextr->entry_depth != b->exit_depth) {
					return 0;
				} else if (b->nextr->marked != PROCESSED) {
					if (process_block(b->nextr,pn) == 0)
						return 0;
				}
			} else /* if (b->nextr->marked == CLEAR) */ {
				b->nextr->marked = TOUCHED;
				b->nextr->entry_depth = b->exit_depth;
				if (process_block(b->nextr,pn) == 0)
					return 0;
			}
		}
		if (is_jmp_ind(b->lastn)) {
			sw = get_base_label(b->lastn->op1);
			for (r = sw->first_ref; r; r = r->nextref) {
				if (sw->switch_table_name == r->switch_table_name) {
					b1 = r->switch_entry;
					if (b1->marked > CLEAR) {
						if (b->exit_depth != b1->entry_depth) {
							return 0;
						}
						else if (b1->marked != PROCESSED)
							if (process_block(b1,pn) == 0)
								return 0;
					} else {
						b1->marked = TOUCHED;
						b1->entry_depth = b->exit_depth;
						if (process_block(b1,pn) == 0)
							return 0;
					}
				}
				if ( r == sw->last_ref )
					break;
			}
		}
	return 1;
}

/* Calculates the stack depth after the function prolog; flags
   instructions in the "enter" sequence for removal.
   Does not handle functions that return structures, or functions
   with auto's.
*/
/*
	As from Jan. 91, it does. That enables fp elimination from
	any function.
*/
static 
process_header(first_node, stack_level,autos_pres,x)
NODE **first_node; 
int *stack_level; /* stack level after the prolog */
int autos_pres; /* function initially had auto's, but regal assigned these
		   to registers */
int x; /*value to return if there is a "push ebp" and no "mov esp ebp"*/
{
	register NODE *pn;
	int count=0;
	long pushes=0;

	/* get to beginning of prolog */
	for (pn = n0.forw; pn != 0; pn = pn->forw) {
		if (pn->op == MOVL || pn->op == PUSHL || pn->op == POPL) break;
	}

	if (!pn) {
		return 0;
	}

	/*skip over code for getting the address of returned struct, if any*/
	if (isgetaddr(pn))
		pn = pn->forw->forw;

	/* skip over profiling code */
	if (isprof( pn->forw ))
		pn = pn->forw->forw;

	/* expect saving of frame pointer */
	if (pn->op != PUSHL || !usesvar("%ebp", pn->op1)) {
		return 0;
	}
	pn->extra = double_aligned ? NO_REMOVE: REMOVE;
	pn = pn->forw;

	/* expect adjust the frame pointer */
	if (pn->op != MOVL ||
		!usesvar("%esp",pn->op1) || !usesvar("%ebp",pn->op2)) {
		eliminate_ebp();
		return x;
	}
	pn->extra = double_aligned ? NO_REMOVE: REMOVE;
	/* if optimizing alignment of doubles, need to adjust ebp */
	/* if called second time, then the andl instruction is already in */
	if (double_aligned && x==2) {
		if (pn->forw->op == ANDL
		 && pn->forw->op1[0] == '$'
		 && atoi(&pn->forw->op1[1]) == -8
		 && samereg(pn->forw->op2,"%ebp")
		 )
			pn = pn->forw;
		else { /*hasnt been inserted yet, it's first time */
			pn = insert(pn);
			chgop(pn,ANDL,"andl");
			pn->op1 = "$-8";
			pn->op2 = "%ebp";
			pn->nlive = pn->back->nlive;
			pn->uses = EBP;
			pn->sets = EBP | CONCODES;
		}
	}

	pn = pn->forw;

	if (!autos_pres)
	/* EMPTY */
		;
	/* expect set up the locals */
	else if (pn->op == SUBL && samereg("%esp",pn->op2)) {
		pn->extra = (double_aligned || ! auto_elim) ? NO_REMOVE: REMOVE;
		pn = pn->forw;
	} else if (pn->op == PUSHL &&
		   samereg("%eax",pn->op1)) {
		pn->extra = (double_aligned || ! auto_elim) ? NO_REMOVE: REMOVE;
		pn = pn->forw;
	}
	else {
		fprintf(stderr,"CASE DOES HAPPEN\n");
		return 0;
	}

	/* process the register saves. Register saves start by pushing non-scratch
	   registers and end when:
		a. A scratch register is pushed.
		b. A non-scratch register is pushed for the second time
	*/
	for (; pn != 0; pn = pn->forw) {
		int reg;
		if (pn->op != PUSHL || !isreg(pn->op1) ||
			((reg=(pn->uses&~ESP))&pushes) || reg&(EAX|EDX|ECX|FP0|FP1))
			break;
		count += 4;
		pushes |= reg;

	}
	*first_node = pn;
	*stack_level = count;
	return 1;
}
 char *
getstmnt() 
{
 register char *s;
 register int c;
 static char *front, *back;		/* Line buffer */ 

#define eoi(c) ((c=='\n')||(c==';')||(c==NULL)||(c==CC)) /* end of instruction */
#define eol(c) ((c=='\n')||(c==NULL)) /* end of line */
	/* Each line of input can contain multiple instructions separated
	 * by semicolons.
	 * getstmnt() returns the next instruction as a null terminated
	 * string.
	 */

	if( front == NULL )	/* initialize buffer */
		{front = (char *)malloc(LINELEN+1);
		 if(front == NULL)
			fatal("getstmnt: out of buffer space\n");
		 back = front + LINELEN;
		}
	/* read until end of instruction */
	s = front;
	while( (c = getchar()) != EOF )
		{if(s >= back)
			s = ExtendCopyBuf(&front,&back,(unsigned)2*(back-front+1));
		 if(eoi(c))
			{switch(c)
				{case ';':
				 case NULL:
				 case '\n':
					*s = NULL;
					return(front);
				 case CC:
					*s++ = (char)c;
					break;
				}
			 /* here if CC, read to end of line */
			 while((c = getchar()) != EOF)
				{if(s >= back)
					s = ExtendCopyBuf(&front,&back,(unsigned)2*(back-front+1));
				 if(eol(c))
					{*s = NULL;
					 return(front);
					}
				 else
					*s++ = (char)c;
				}
			 /* premature EOF */
			 if(s > front)
				{*s = NULL;
				 return(front);
				}
			 return(NULL);
			}
		 else
			*s++ = (char)c;
		}
	/* EOF */
	if(s > front)	/* premature */
		{*s = NULL;
		 return(front);
		}
	return(NULL);
}
static char *
ExtendCopyBuf(p0,pn,nsize)
char **p0;
char **pn;
unsigned int nsize;
{
 char *b0 = *p0;
 char *bn = *pn;
 unsigned int osize;
 extern void fatal();

 /* input buffer looks like:
  *
  *	-----------------------
  *	|   |   | ... |   |   |
  *	-----------------------
  *       ^                 ^
  *       |                 |
  *	 *p0               *pn == s
  *
  * where the current user pointer, s, is at the end of buffer.
  * after buffer extension, the new buffer looks like:
  *
  *	---------------------------------------------
  *	|   |   | ... |   |   |   |   | ... |   |   |
  *	---------------------------------------------
  *       ^                 ^                     ^
  *       |                 |                     |
  *      *p0                s                    *pn
  *
  * where s is at the same distance from the beginning of the buffer,
  * and the contents of the buffer from *p0 to s is unchanged,
  * and s is returned.
  */

 osize = bn - b0 + 1;
 if(nsize <= osize)
	fatal("ExtendCopyBuf: new size <= old size\n");
 b0 = realloc(b0,nsize);
 if(b0 == NULL)
	fatal("ExtendCopyBuf: out of space\n");
 bn = b0 + nsize - 1;
 *p0 = b0;
 *pn = bn;
 return(b0 + osize - 1);
}

#define LEALSIZE (10+1+4+1+1)		/* for "2147483647(%esp)\0" */

static void
stack_cleanup()
{
	NODE *p;
	NODE *last_add = NULL;
	int stack_level, stack_count, block_stack_count, fcn_count;


	if (!process_header(&p, &stack_level, numauto,0)) {
		return;
	}

	ldanal();
	stack_count = block_stack_count = 0;

	for (; p != &ntail ; p = p->forw) {
		if (((((p->op >= CMPB && p->op <= TESTL) || p->op == SAHF)
			  && isbr(p->forw)) ||
			 p->op == ASMS || isbr(p) || islabel(p) || p->op == LEAVE ||
			 (p->op == POPL && strcmp(p->op1, "%ecx")))
		 && !stack_count) {
			if (block_stack_count > 0) {
				if (process_push_pop(last_add) != block_stack_count) {
					if (block_stack_count == 4 && ! (p->nlive & ECX) ) {
						chgop(last_add, POPL, "popl");
						last_add->op1 = "%ecx";
						last_add->uses = ESP;
						last_add->sets = ESP | ECX;
						last_add->op2 = NULL;
					} else if (last_add->op == ADDL && *last_add->op1 == '$') {
						last_add->op1 = getspace(ADDLSIZE);
						sprintf(last_add->op1, "$%d", block_stack_count);
					} else if (p->op == LEAL) {
						last_add->op1 = getspace(LEALSIZE);
						sprintf(last_add->op1, "%d(%%esp)", block_stack_count);
					} else if (last_add->op == POPL) {
						chgop(last_add, LEAL, "leal");
						last_add->uses = ESP;
						last_add->sets = ESP;
						last_add->op1 = getspace(LEALSIZE);
						sprintf(last_add->op1, "%d(%%esp)", block_stack_count);
						last_add->op2 = "%esp";
					}
					if (!((p->uses | p->nlive) & CONCODES) &&
						!(last_add->op == POPL &&
						(!islabel(p) && p->nlive & ECX ||
						  islabel(p) && p->back->nlive & ECX))) {
						DELNODE(last_add);
						INSNODE(last_add, p);
						lexchin(p, last_add);
					}
				} else if (block_stack_count == 4 && ! (p->nlive & ECX) ) {
						chgop(last_add, POPL, "popl");
						last_add->op1 = "%ecx";
						last_add->op2 = NULL;
						last_add->uses = ESP;
						last_add->sets = ESP | ECX;
				}
				last_add = NULL; 
				block_stack_count = 0;
				if (p->op >= CMPB && p->op <= TESTL)
					p = p->forw;
			}
		}
		else if (p->sets & ESP && p->extra != TMPSRET) {
			stack_count += process_push_pop(p);
		}
		else if (p->op == CALL || p->op == LCALL) {
			p = p->forw;
			if (p->sets & ESP && p->op != LEAVE &&
				(p->op != POPL || !strcmp(p->op1, "%ecx")))
			   if ((fcn_count = -process_push_pop(p)) ==
				   stack_count) {	/* all current calls are done */
				block_stack_count += stack_count;
				stack_count = 0;
				if (last_add)
					DELNODE(last_add);
				last_add = p;
			   }
			   else if (fcn_count >= 0) {
				stack_count -= fcn_count;
			   }
			   else {		/* another push instruction? */
				p = p->back;
			   }
			else		/* end of block? */
			   p = p->back;
		}
	}
}

void
sets_and_uses()
{
NODE *p;
	for(ALLN(p)) {
		p->sets = sets(p);
		p->uses = uses(p);
	}
}/*end sets_and_uses*/

static void
clean_label_sets_uses()
{
NODE *p;
	for(ALLN(p))
		if(islabel(p)) {
			p->sets = p->uses =0;
		}
}/*clean_label_sets_uses*/

static char *
bi2bp(op) char *op;
{
char *t, *mystrstr();
	if (!isreg(op)) {
		if (t = mystrstr(op,"%ebi")) {
			t+=3;
			*t = 'p';
		}
		if (t = mystrstr(op,"%bi")) {
			t += 2;
			*t = 'p';
		}
	} else {
		if (scanreg(op,false) == EBI)
			return "%ebp";
		else if (scanreg(op,false) == BI)
			return "%bp";
	}
	return op;
}/*end bi2bp*/

static void
forget_bi()
{
	NODE *p;
	int i;
	for (ALLN(p))
		for (i = 1; i <= 3; i++)
			if (p->ops[i] && (scanreg(p->ops[i],0) & EBI)) {
				p->ops[i] = bi2bp(p->ops[i]);
				p->ops[i] = bi2bp(p->ops[i]);
				if (p->uses & Ebi) {
					p->uses &= ~EBI;
					p->uses |= EBP;
				} else if (p->uses & BI) {
					p->uses &= ~BI;
					p->uses |= BP;
				}
				if (p->sets & Ebi) {
					p->sets &= ~EBI;
					p->sets |= EBP;
				} else if (p->sets & BI) {
					p->sets &= ~BI;
					p->sets |= BP;
				}
			}
}/*end forget_bi*/

static void
eliminate_ebp()
{
NODE *p;
	for (ALLN(p)) {
		if ((p->uses | p->sets) & EBP)
			DELNODE(p);
	}
}/*end eliminate_ebp*/


static void
hide_safe_asm()
{
NODE *p;
	if (!found_safe_asm) 
		return; /* save time */
	for (ALLN(p))
		if (p->op > SAFE_ASM) {
			p->op -= SAFE_ASM;
			p-> sasm = SAFE_ASM;
		}
}/*end hide_safe_asm*/

static void
recover_safe_asm()
{
NODE *p;
	if (!found_safe_asm) 
		return; /* save time */
	for (ALLN(p)) {
		p->op += p->sasm;	
	}
}/*end recover_safe_asm*/

static void
reset_crs()
{
NODE *p;
int i;
	for (ALLN(p)) {
		for (i = 1; i < 3; i++) {
			if (p->ops[i] && !strncmp(p->ops[i],"&cr",3))
				*p->ops[i] = '%';
		}
	}
}/*end reset_crs*/

#ifdef DEBUG
static boolean
holds_const(node,limit) NODE *node, *limit;
{
NODE *q;
int reg = scanreg(node->op1,false);
	for (q = node->back; q != limit; q = q->back) {
		if ((q->sets & ~CONCODES) == reg) {
			if (isconst(q->op1) && isdigit(q->op1[1]))
				return true;
			else
				reg = q->uses;
		}
	}
	return false;
}/*end holds_const*/

static boolean
change_esp_by_const(node,limit)
NODE *node, *limit;
{
	switch(node->op - SAFE_ASM) {
	case PUSHL: case PUSHW: case POPL: case POPW:
	case PUSHFL: case PUSHFW: case POPFL: case POPFW:
		return true;
	case LEAL:
		if (node->uses == ESP && node->sets == ESP)
			return true;
		else
			return false;
		break;
	case SUBL: case ADDL:
		if (isreg(node->op1) && samereg(node->op2,"%esp")) {
			if (holds_const(node,limit))
				return true;
			else
				return false;
		} else if (*node->op1 == '$' && isdigit(node->op1[1])) {
			return true;
		} else
			return false;
	/* FALLTHRU */
	case LEAVE:
		return true;
	default:
		return false;
	}
	/* NOTREACHED */
}

static boolean
verify_safe_asm()
{
NODE *p,*q,*r,*last;
int found;
int x;
boolean retval = true;
extern int isregal();
	for (ALLN(p)) {
		if (is_safe_asm(p)) {
			for (last = p; !is_safe_asm(last) ; last = last->forw) ;
				for (q = p; q != last; q = q->forw) {
					if (sa_islabel(q)) {
						for (r = n0.forw; r != p; r = r->forw) {
							if ((isbr(r) || sa_isbr(r))
								&& !strcmp(q->opcode,r->op1)) {
									fprintf(stderr,
									"UNSAFE: label refed from before ");
									fprinst(q); fprinst(r); 
									retval = false;
								}
						}
						for (r = last; r != &ntail; r = r->forw) {
							if ((isbr(r) || sa_isbr(r))
								&& !strcmp(q->opcode,r->op1)) {
									fprintf(stderr,
									"UNSAFE: label refed from after ");
									fprinst(q); fprinst(r); 
									retval = false;
								}
						}
					}
					if (sa_isbr(q) && q->op1) {
						found = false;
						for (r = p; r != last; r = r->forw) {
							if (sa_islabel(r) && !strcmp(q->op1,r->opcode))
								found = true;
						}
						if (!found) {
							fprintf(stderr,"UNSAFE: br target out of asm");
							fprinst(q); 
							retval = false;
						}
					}
					if (!fp_removed && q->sets & EBP) {
						fprintf(stderr,"UNSAFE: sets EBP ");
						fprinst(q); 
						retval = false;
					}
					if ((q->sets & ESP) && !change_esp_by_const(q,p)) {
						fprintf(stderr,"UNSAFE: sets ESP ");
						fprinst(q); 
						retval = false;
					}
				}
		p = last;
		}
	}
	return retval;
}/*end verify_safe_asm*/
#endif
