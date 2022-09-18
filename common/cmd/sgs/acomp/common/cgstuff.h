/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acomp:common/cgstuff.h	55.1.4.15"
/* cgstuff.h */

/* Declarations for routines that interface to CG. */

extern void cg_defnam();
extern void cg_nameinfo();
extern void cg_incode();
extern void cg_treeok();
extern void cg_ecode();
extern void cg_begf();
extern void cg_endf();
extern void cg_copyprm();
extern void cg_bmove();
extern void cg_deflab(), cg_goto();
extern void cg_swbeg(), cg_swcase(), cg_swend();
extern void cg_filename();
extern void cg_ident();
extern void cg_asmold();
extern void cg_begfile();
extern void cg_eof();
extern void cg_profile();
extern void cg_ldcheck();
extern void cg_zecode();
extern void cg_setlocctr();
extern void cg_instart();
extern void cg_inend();

extern ND1 * cg_defstat();
extern ND1 * cg_strinit();

extern char * cg_extname();

/* cg_tconv() should only be used in cgstuff and p1allo */
extern TWORD cg_tconv();

/* Declarations for functions to convert OFFSET's. */
#define	cg_off_bigger(space, o1, o2)	off_bigger((space),(o1),(o2))
#define cg_off_incr(space, o, n)	off_incr((space),(o),(n))
#define	cg_off_is_err(space, o)		off_is_err((space),(o))
extern OFFSET cg_off_conv();

/* For cg_eof(): */
#define	C_TOKEN		1	/* file had tokens in it */
#define	C_NOTOKEN	0	/* file had no tokens */

/* For cg_indata(), cg_strinit(): */
#define	C_READONLY	0	/* read-only data section */
#define	C_READWRITE	1	/* read/write data section */

/* Support for optimizer interface. */

#ifdef	OPTIM_SUPPORT

extern void os_uand();
extern void os_symbol();
extern void os_loop();

#ifdef	FAT_ACOMP

#define	OS_UAND(sid)	cg_q_sid(os_uand, sid)
#define	OS_SYMBOL(sid)	cg_q_sid(os_symbol, sid)
#define	OS_LOOP(code)	cg_q_int(os_loop, code)

#else	/* ! FAT_ACOMP */

#define OS_UAND(sid)	os_uand(sid)
#define OS_SYMBOL(sid)	os_symbol(sid)
#define OS_LOOP(code)	os_loop(code)

#endif	/* def FAT_ACOMP */

#endif /* def OPTIM_SUPPORT */

/* Support for function-at-a-time compiling */


typedef union {
    int cgq_int;
    ND1 * cgq_nd1;
    ND2 * cgq_nd2;
    SX cgq_sid;
    char * cgq_str;
    T1WORD cgq_type;
} cgq_arg_t;

typedef int Cgq_index; /* Byte offset of a queue element in an array */

	/* Is scope between "start" and "end" */
#define CGQ_SCOPE_BETWEEN(marker,start,end) ( \
	(marker <= end) && (marker >= start) )

typedef struct {
    short cgq_op;	/* queued operation (one of above) */
    void (*cgq_func)();	/* function to call */
    cgq_arg_t cgq_arg;	/* argument to function */
    int cgq_ln;		/* parsing line number */
    int cgq_dbln;	/* debug info line number */
    int cgq_scope_marker; /* count of executable trees to here in the cgq (inclusive) */
    int cgq_delete_flag; /* Set if reg_alloc should ignore */
    Cgq_index cgq_next, cgq_prev; /* for doubly linked list */
} cgq_t;


#define CGQ_FIRST_INDEX 0

/* the CGQ_NEXT_INDEX(last element) yields the following value */
#define CGQ_NULL_INDEX (-(int)sizeof(cgq_t))

/* given an index retrieves the associated cgq_t *   */
#define CGQ_ELEM_OF_INDEX(I) ((cgq_t *)(myVOID *)((char *)td_cgq.td_start+(I)))
#define CGQ_ELEM_OF_INDEX_Q(I,q) ((cgq_t *)(myVOID *)((char *)(*q).td_start+(I)))

#define CGQ_NEXT_INDEX(elem) elem->cgq_next
#define CGQ_PREV_INDEX(elem) elem->cgq_prev
#define CGQ_PREV_ELEM(elem) (CGQ_ELEM_OF_INDEX(CGQ_PREV_INDEX(elem)))
#define CGQ_NEXT_ELEM(elem) (CGQ_ELEM_OF_INDEX(CGQ_NEXT_INDEX(elem)))
#define CGQ_CUR_INDEX()	(TD_USED(td_cgq)*sizeof(cgq_t))
#define CGQ_LAST_ITEM(cgq) (TD_USED((*cgq))*sizeof(cgq_t))

/* first and second must have been added by cg_q, not cg_insert */
#define CGQ_LTE(first,second) (first <= second)

/* used only for debugging */
#define CGQ_INDEX_NUM(index) ((index)/(sizeof(cgq_t)))

#define HAS_ND1(flow) (flow->cgq_op == CGQ_EXPR_ND1 ? \
		flow->cgq_arg.cgq_nd1 : \
	(flow->cgq_op == CGQ_FIX_SWBEG ) ?\
		(ND1 *)flow->cgq_arg.cgq_nd2->left : 0)

	/*
	** CGQ_ND1 returns the address of member of the flow node
	** whose contents are returned by HAS_ND1,
	** so it must only be called when HAS_ND1 is true.
	*/
#define CGQ_ND1(flow) *(flow->cgq_op == CGQ_EXPR_ND1 ? \
		&flow->cgq_arg.cgq_nd1 : \
		(ND1 **)(&flow->cgq_arg.cgq_nd2->left))

/* following iterates over all members of queue,
   declares and gives access to queue element in variable elem  and index
   in variable index
*/
#ifdef lint
#define _LINT_INIT = 0
#else
#define _LINT_INIT
#endif

#define CGQ_FOR_ALL(elem,index) { \
Cgq_index index = CGQ_FIRST_INDEX;\
cgq_t * elem  _LINT_INIT ; \
for(; index != CGQ_NULL_INDEX; index = CGQ_ELEM_OF_INDEX(index)->cgq_next) {\
	elem = CGQ_ELEM_OF_INDEX(index); {
#define CGQ_INDEX_OF_ELEM(elem) ((char *) (elem) - (char *) td_cgq.td_start)

#define CGQ_FOR_ALL_QUEUE(elem,index,queue) { \
Cgq_index index = CGQ_FIRST_INDEX;\
cgq_t * elem  _LINT_INIT ; \
for(; index != CGQ_NULL_INDEX; index = CGQ_ELEM_OF_INDEX(index)->cgq_next) {\
	elem=(cgq_t *)((char *)(queue)->td_start+index); {

#define CGQ_END_FOR_ALL } } }

/* following iterates over all members of queue between indices,
   declares and gives access to queue element in variable elem and index
   in variable index
 */

#define CGQ_FOR_ALL_BETWEEN(elem,index,from_index,to_index) { \
Cgq_index index = from_index; \
cgq_t * elem _LINT_INIT;\
assert(to_index != CGQ_NULL_INDEX);\
assert(CGQ_ELEM_OF_INDEX(to_index));\
for (index = from_index ; index != CGQ_ELEM_OF_INDEX(to_index)->cgq_next;\
	index = CGQ_ELEM_OF_INDEX(index)->cgq_next) { \
	elem = CGQ_ELEM_OF_INDEX(index); {

#define CGQ_END_FOR_ALL_BETWEEN } } }
#define CGQ_FOR_ALL_BETWEEN_BREAK break


	/* Following checks if CGQ_FOR_ALL_BETWEEN will be zero trip */
#define CGQ_NONE_BETWEEN(from_index,to_index) ( \
	from_index == CGQ_ELEM_OF_INDEX(to_index)->cgq_next \
)

#ifdef	FAT_ACOMP

extern void
#ifdef __STDC__
cg_printf(const char *fmt, ...);
#else
cg_printf();
#endif

extern void cg_q_sid();
extern void cg_q_int();
extern void cg_q_nd1();
extern void cg_q_str();
extern void cg_q_type();
extern void cg_q_call();

#ifdef DBLINE
extern void cg_mk_lineno();
extern void cg_lineno();	/* called by CG */
#endif

#ifndef NO_AMIGO
extern Cgq_index cg_inline_endf();	/* returns value of current index */
extern void cg_restore();	/* restores a cgq_index */
extern void cg_q_delete();
extern void cg_q_remove();
extern cgq_t * cg_q_insert();
#endif

/* Operations in CG queue. */

#define	CGQ_EXPR_ND1	1	/* ND1 expression to optimize and compile */
#define	CGQ_EXPR_ND2	2	/* ND2 expression to compile */
#define	CGQ_PUTS	3	/* string to output */
#define	CGQ_CALL	4	/* f(void) */
#define	CGQ_CALL_INT	5	/* f(int) */
#define	CGQ_CALL_SID	6	/* f(SX) */
#define	CGQ_CALL_STR	7	/* f(char *) */
#define	CGQ_CALL_TYPE	8	/* f(T1WORD) */
#define	CGQ_CALL_ND2	9	/* f(ND2 *) */
#define CGQ_START_SCOPE 10
#define CGQ_END_SCOPE	11
#define CGQ_FIX_SWBEG	12
#define CGQ_DELETED	13	/* Used to MARK nodes for deletion rather
				** than actually removing them
				*/
#define CGQ_START_SPILL 14
#define CGQ_END_SPILL 15

#ifndef	INI_CGQ
#ifndef CGQ_DEBUG
#define	INI_CGQ 1
#else
#define INI_CGQ	200
#endif
#endif

#define	CGQ_NOFUNC ((void (*)()) 0)

extern struct td td_cgq;
#ifndef NODBG
void cg_putq();
void old_cg_putq();
char *cg_funcname(), *cg_db_loop();
#endif
#if 0
extern void cg_q_puts();
#endif

#define	CG_PRINTF(args)	cg_printf args
#define	CG_PUTCHAR(x)	cg_printf("%c", x)
#define	CG_COPYPRM(sid) cg_q_sid(cg_copyprm, sid)
#define	CG_ECODE(p)	cg_q_nd1(p)

#else	/* ! FAT_ACOMP */

#define	CG_PRINTF(args)	printf args
#define	CG_PUTCHAR(c)	putchar(c)
#define	CG_COPYPRM(sid)	cg_copyprm(sid)
#define	CG_ECODE(p)	cg_ecode(p)


#endif	/* def FAT_ACOMP */
