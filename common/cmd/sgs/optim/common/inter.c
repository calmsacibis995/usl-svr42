/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)optim:common/inter.c	1.18"

#include "optim.h"
#include <varargs.h>

extern int in_safe_asm;
NODE *lastnode;	/* reference to node being built by Saveop */

	NODE *
Saveop(opn, str, len, op) /* save part of inst */
     register int opn; 
     register char *str;
     unsigned len;
     unsigned short op; {

	register NODE *p = lastnode;
	if (opn == 0) { /* make a new node and link it in */
		p = lastnode = GETSTR(NODE);
		if ((p->op = op) != GHOST) {
			INSNODE(p, &ntail);
			ninst++;
			if (in_safe_asm) p->op += SAFE_ASM;
		}
		p->extra = NO_REMOVE;
		for (op = 1; (int)op <= (int)MAXOPS + 1; ++op )
			p->ops[op] = NULL;
#ifdef LIVEDEAD
		p->nlive = p->ndead = 0;
 		p->nrlive = p->nrdead = 0;
#endif
		p->ebp_offset = 0;
		p->esp_offset = 0;
		p->zero_op1 = 0;
		p->sasm = 0;
#ifdef IDVAL
		p->uniqid = IDVAL;
#endif
#if defined(USERDATA) && defined(USERINITVAL) 
		p->userdata = USERINITVAL;
#endif
	}
	if (opn < 0 || opn > MAXOPS)
		fatal("invalid opn field for %s\n", str);
	if (len) { /* clean space in the end of the strings */
		len = strlen(str) - 1; /* real ( not len) string size - 2 */
		while((( (int) len) >= 0) && isspace(str[len]))
			len--;  /* remove space from the end */
		p->ops[opn] = COPY(str, len+2);
		p->ops[opn][len+1] = '\0'; /* Needed for the case space in the end */
	} else 
		p->ops[opn] = str;
	if (p->ops[opn] && ( !strncmp(p->ops[opn],"%cr",3)
		|| !strncmp(p->ops[opn],"tr",3) || !strncmp(p->ops[opn],"%dr",3)))
		*p->ops[opn] = '&'; /*cr should not look like a register*/
	if (op == TSRET)  /* TMPRET */
		p->extra = TMPSRET;
	return (p);
}
static SWITCH_TBL *s;
	void
start_switch_tbl() {
	s  = GETSTR(SWITCH_TBL);
	s->first_ref = lastref;
}
	void 
end_switch_tbl(str) char *str; {
	s->first_ref = s->first_ref->nextref->nextref;
	s->last_ref = lastref;
	s->switch_table_name = str;
	s->next_switch = sw0.next_switch;
	sw0.next_switch = s;
}
	void
addref(str, len,str2) char *str,*str2; unsigned len; {
/* add text ref to reference list */

	register REF *r = lastref = lastref->nextref = GETSTR(REF);

	r->lab = COPY(str, len);
	r->nextref = NULL;
	r->switch_table_name = str2;
}

	void
prtext() { /* print text list */

	extern void prinst();
	register NODE *p;

	for (ALLN(p)) {
		prinst(p);
	}
}

	boolean
same(p, q) NODE *p, *q; { /* return true iff nodes are the same */


	register char **pp, **qq;
	register int i;

	if (p->op != q->op)
		return (false);

	/* first check for equal numbers of active operands */

	for (pp = p->ops, qq = q->ops, i = MAXOPS + 1;
	    --i >= 0 && (*pp != NULL || *qq != NULL); pp++, qq++)
		if (*pp == NULL || *qq == NULL)
			return (false);

	/* then check for equality of the active operands */

	while (pp > p->ops)
		if (**--pp != **--qq || strcmp(*pp, *qq))
			return (false);
	return (true);
}

	boolean
sameaddr(p, q) NODE *p, *q; { /* return true iff ops[1...] are the same */

	register char **pp, **qq;
	register int i;

	/* first check for equal numbers of active operands */

	for (pp = p->ops, qq = q->ops, i = MAXOPS + 1;
	    --i >= 0 && (*pp != NULL || *qq != NULL); pp++, qq++)
		if (*pp == NULL || *qq == NULL)
			return (false);

	/* then check for equality of the active operands */

	while (pp > p->ops + 1)
		if (**--pp != **--qq || strcmp(*pp, *qq))
			return (false);
	return (true);
}

	char *
xalloc(n) register unsigned n; { /* allocate space */

	extern char *malloc();
	register char *p;

	if ((p = malloc(n)) == NULL)
		fatal("out of space\n");
	return (p);
}

	void
xfree(p) char *p; { /* free up space allocated by xalloc */

	extern void free();

	free(p);			/* return space */
}

/* VARARGS */
	void
fatal(va_alist)
va_dcl
{
	va_list args;
	char *fmt;
	extern void exit();

	va_start(args);
	FPRINTF(stderr, "Optimizer: ");
	fmt=va_arg(args,char *);
	(void)vfprintf(stderr, fmt, args);
	va_end(args);
	exit(2);
}
