/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef PARSER_H
#define PARSER_H
#ident	"@(#)debugger:inc/common/Parser.h	1.6"

//
// NAME
//	Node, Op, OpType, Opnd
//
// ABSTRACT
//	Support for the command language parser
//
// DESCRIPTION
//	The parser calls the scanner to tokenize each input line, then
//	constructs a tree of Nodes to represent each logical command.
//	The execute() function takes Node trees and evaluates them.
//	Associated actions are stored as Node trees and will persist
//	after execution of the original command.  Other Nodes are
//	freed by execute().
//
// DATA
//	Op is an enumeration of the values of Node::op
//	OpType is an enumeration of the types of operands (union Opnd)
//	in Node::first, Node::second, Node::third, and Node::fourth.
//	Node::optype[] is an array of four chars which contain values
//	from enum OpType.  optype[0] is the type of Node::first, etc.
//
// OPERATIONS
//	Node::Node() constructs a node with op = NoOp, and no operands.
//	The other constructors allow setting the op field and 0, 1, 2,
//	3, or all 4 operands at once.
//
//	Node::~Node() recursively destroys a tree, unless op == SAVE.
//
//	Node::clear() is a quick zero of the Node, used by the constructors.
//
//	Node::setopnd(int n, Optype, void *) is a utility which sets the
//	value and type of the nth operand.
//
//	char *Node::print() will return a string representation of the
//	node (not recursive).
//
//	void Node::dump() recursively prints (via printe()) the node tree.

#include <string.h>


// Warning : changing this list requires changing the help[] table
// in libcmd/common/Help.C

enum Op {
	NoOp = 0,
	REDIR, CMDLIST, SHELL, SAVE,
	ALIAS, BREAK, CANCEL, CD, CHANGE, CONTINUE, CREATE,
	DELETE, DIS, DISABLE, DUMP,
	ENABLE, EVENTS, EXPORT, FC, FUNCTIONS, GRAB, HALT, HELP, 
	IF, INPUT, JUMP, KILL, LIST, LOGOFF, LOGON, MAP,
	ONSTOP, PENDING, PFILES, PPATH, PRINT, PS, PWD, QUIT, 
	REGS, RELEASE, RENAME, RUN,
	SCRIPT, SET, SIGNAL, STACK, STEP, STOP, SYMBOLS, SYSCALL,
	VERSION, WHILE,
};

extern void check_collisions( const char *, int classes );

// classes of names for check_collisions()
#define ALIASES		1
#define COMMANDS	2
#define MACROS		4

enum OpType {
	NONE = 0,
	NODE,
	STRING_OPND,	// when printed, should include quotes
	CHARSTAR,	// no quotes - filenames, etc.
	INT, TOKEN,
	PROCLIST, LOC, REGLIST,
	SYSCALLS, EXPLIST, FILELIST, EVENTLIST, EVENT_EXPR,
};

struct Node;
typedef int Opts;
struct Proclist;
class IntList;
struct Location;
struct Exp;
struct Filelist;
struct Eventlist;
struct StopEvent;
class Token;

union Opnd {
	Node	  *np;		// another Node
	char	  *cp;		// a string (generic arg)
	int	   i;		// an integer (generic arg)
	Token	  *tl;		// list of tokens (for alias)
	Proclist  *procs;	// list of processes
	Location  *loc;		// location
	IntList	  *sys;		// list of syscall numbers
	Exp	  *exp;		// list of expressions
	Filelist  *files;	// list of filenames
	Eventlist *events;	// list of event numbers
	StopEvent *stop;	// stop expression
};


struct Node {
	Op	op;
	Opts	opts;
	Opnd	first;
	Opnd	second;
	Opnd	third;
	Opnd	fourth;
	Opnd	fifth;
	char	optype[5];
	// setopnd and getopnd depend on all members of Opnd being
	// same size
	void	setopnd(int n, OpType t, void *v)
			{ int *p = (int *) &first; optype[--n] = t;
			  p[n] = (int)v; }
	void	*getopnd(int n)
			{ int *p = (int *) &first; return (void *)p[--n]; }
	void	clear() 
		{ memset((char *)this, 0, sizeof(*this)); }	// quick zero
	Node()	{ clear(); }
	Node(Op o)
		{ clear(); op = o; }
	Node(Op o, OpType ot1, void *op1)
		{ clear(); op = o;
		  setopnd(1, ot1, op1); }
	Node(Op o, OpType ot1, void *op1, OpType ot2, void *op2)
		{ clear(); op = o;
		  setopnd(1, ot1, op1);
		  setopnd(2, ot2, op2); }
	Node(Op o, OpType ot1, void *op1, OpType ot2, void *op2,
			OpType ot3, void *op3)
		{ clear(); op = o;
		  setopnd(1, ot1, op1);
		  setopnd(2, ot2, op2);
		  setopnd(3, ot3, op3); }
	Node(Op o, OpType ot1, void *op1, OpType ot2, void *op2,
			OpType ot3, void *op3, OpType ot4, void *op4);
	/*	{ clear(); op = o;
		  setopnd(1, ot1, op1);
		  setopnd(2, ot2, op2);
		  setopnd(3, ot3, op3);
		  setopnd(4, ot4, op4); }  (can't inline) */
	Node(Op o, OpType ot1, void *op1, OpType ot2, void *op2,
			OpType ot3, void *op3, 
			OpType ot4, void *op4, OpType ot5, void *op5);
	/*	{ clear(); op = o;
		  setopnd(1, ot1, op1);
		  setopnd(2, ot2, op2);
		  setopnd(3, ot3, op3);
		  setopnd(4, ot4, op4);
		  setopnd(5, ot5, op5); }  (can't inline) */


	~Node();

#ifdef DEBUG
	char *print();
	void dump();
	void dumpopnd(int);
#endif
};

enum Redir {
	RedirNone = 0,
	RedirFile,
	RedirAppend,
	RedirPipe
};

struct Ptrlist {			// dynamically growing list of void *
	int	  count;
	void	**ptr;			// actually [count+1] (null terminated)
		  Ptrlist(int);
		 ~Ptrlist();
	void	 *operator[](int);
	void	  concat(const Ptrlist *);	// grow this, add others
virtual	char	 *print(char sep = ' ');// for display
};

#ifdef __cplusplus	// C++ 2.0 on
#define PTRLIST Ptrlist
#else			// C++ 1.2
#define PTRLIST
#endif

struct Proclist : public Ptrlist{	// dynamic list of char * (proc names)
	Proclist(int n) : PTRLIST(n)	{}
	char	 *operator[](int i)	{ return (char *)(*(Ptrlist*)this)[i]; }
	char 	 *print(char sep = ',');
};

struct Exp : public Ptrlist{		// dynamic list of char * ("expressions")
	Exp(int n) : PTRLIST(n)	{}
	Exp(char *p) : PTRLIST(1)	{ ptr[0] = p; }
	void	 concat(char *p)	// add single expression
			{ Exp *n = new Exp(p);
			  this->Ptrlist::concat((Ptrlist*)n);
			  delete n; }
	char	 *operator[](int i)	{ return (char *)(*(Ptrlist*)this)[i]; }
};

struct Filelist : public Ptrlist{	// dynamic list of char * (file names)
	Filelist(int n) : PTRLIST(n)	{}
	Filelist(char *p) : PTRLIST(1)	{ ptr[0] = p; }
	char	 *operator[](int i)	{ return (char *)(*(Ptrlist*)this)[i]; }
};

struct Eventlist : public Ptrlist{	// dynamic list of char * (event numbers)
	Eventlist(int n) : PTRLIST(n)	{}
	Eventlist(char *p) : PTRLIST(1)	{ ptr[0] = p; }
	char	 *operator[](int i)	{ return (char *)(*(Ptrlist*)this)[i]; }
};

// singly linked list of integers 
// always add - never delete
// to loop through list:
// IntList	*p;
// int		i;
//  if (p->first())
//  	do {
// 		i = p->val();
//		while(p->next());
//  	}

class IntItem {
	int	idata;
	IntItem	*inext;
	friend class IntList;
};

class IntList {
	IntItem	*head;
	IntItem	*tail;
	IntItem	*current;
public:
	IntList() { head = 0; tail = 0; current = 0; };
	~IntList();
	int		val() 	{ return current->idata; };
	void		add(int);
	int		first()	{ if ((current=head) == 0) return 0; else return 1; };
	int		next()	{ if (current==tail) return 0; else { current=current->inext; return 1; }};
	void		concat(IntList *);
	char		*print();
};

// debugger variable classification

enum dbg_vtype {
	NoType = 0, 
	Db_lang, File, Follow, Func, Glob_path, 
	Lang, Lastevent, Line, Lline, Lfile, Lwp,
	Mode, Num_bytes, Num_lines,
	Path, PFrame, PLoc, Proc, Pprogram, Prompt,
	PResult, PRedir, Thisevent,
	Verbose, Wait, 
};

struct dvars { 
	const char	*varname;
	dbg_vtype	vartype;
};

extern dvars vartab[];

class LWP;

// verbosity levels
#define V_QUIET		0
#define V_SOURCE	1
#define V_EVENTS	2
#define V_REASON	3
#define	V_ALL		4
#define V_HIGH		5
// synchronous or asynchronous
#define NO_WAIT		0
#define WAIT		1

// objects to follow
#define FOLLOW_NONE	0
// #define FOLLOW_THREADS	1
#define FOLLOW_PROCS	1
#define FOLLOW_ALL	2

extern void parse_and_execute(const char *);
extern char *signames( unsigned int );
extern const char *signame( int );
extern const char *sysname( int );
extern IntList *getsys( const char * );
extern IntList *getsys_mach(int);
extern void dump_syscalls();
extern int execute( Node*);
extern const char *list_cmd( Node*);
extern int do_help(char *);
extern dbg_vtype dbg_var(const char *);
extern void docommands();
extern void doscript();
extern int do_shell(const char *, int update_last);

// the following macro maps
// 'a'-'z' to bits 0x00000001 thru 0x2000000

#define	DASH(X)	((1<<((X)-'a')))

#endif /* PARSER_H */
