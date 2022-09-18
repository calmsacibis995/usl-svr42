/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nas:common/sect.h	1.4"
/*
* common/sect.h - common assembler section header
*
* Depends on:
*	<libelf.h> - if FORMAT == ELF
*	"common/as.h"
*/

	/* section attribute bits */
#if FORMAT == ELF		/* values from <libelf.h> */
#  define Attr_Alloc	SHF_ALLOC
#  define Attr_Exec	SHF_EXECINSTR
#  define Attr_Write	SHF_WRITE
#else
#  define Attr_Alloc	0x1
#  define Attr_Exec	0x2
#  define Attr_Write	0x4
#endif

#if FORMAT == ELF		/* values from <libelf.h> */
   enum	/* builtin section types */
   {
	SecTy_Null	= SHT_NULL,
	SecTy_Progbits	= SHT_PROGBITS,
	SecTy_Symtab	= SHT_SYMTAB,
	SecTy_Strtab	= SHT_STRTAB,
	SecTy_RelocA	= SHT_RELA,
	SecTy_Hash	= SHT_HASH,
	SecTy_Dynamic	= SHT_DYNAMIC,
	SecTy_Note	= SHT_NOTE,
	SecTy_Nobits	= SHT_NOBITS,
	SecTy_Reloc	= SHT_REL,
	SecTy_Dynsym	= SHT_DYNSYM
   };
#else
   enum	/* builtin section types */
   {
	SecTy_Null,
	SecTy_Progbits,
	SecTy_Symtab,
	SecTy_Strtab,
	SecTy_RelocA,
	SecTy_Hash,
	SecTy_Dynamic,
	SecTy_Note,
	SecTy_Nobits,
	SecTy_Reloc,
	SecTy_Dynsym
   };
#endif

enum	/* section content kinds */
{
	CodeKind_Unset,		/* as yet unused */
	CodeKind_RelSym,	/* relocation off symbol */
	CodeKind_RelSec,	/* relocation off section */
	CodeKind_FixInst,	/* fixed-sized instruction */
	CodeKind_VarInst,	/* variable-sized instruction */
	CodeKind_Expr,		/* expression value */
	CodeKind_Align,		/* alignment request */
	CodeKind_Backalign,	/* alignment at label */
	CodeKind_Skipalign,	/* the padding location for Backalign */
	CodeKind_Pad,		/* skip bytes */
	CodeKind_Zero,		/* fill zero bytes */
	CodeKind_String		/* string value */
};

enum	/* common CodeKind_Expr target forms */
{
	CodeForm_Float,		/* single precision floating */
	CodeForm_Double,	/* double precision floating */
	CodeForm_Extended,	/* double extended floating */
	CodeForm_Integer	/* integer binary value */
};

typedef struct t_more_	More;	/* defined in sect.c */

struct t_code_	/* each unit of a section's contents */
{
	Code			*code_next;	/* next code for sect */
	union
	{
		More		*code_more;	/* for Pad/String/align */
		const Inst	*code_inst;	/* for CodeKind_...Inst */
		Symbol		*code_sym;	/* for CodeKind_RelSym */
		Section		*code_sec;	/* for CodeKind_RelSec */
		Uint		code_form;	/* for CodeKind_Expr */
	} info;
	union
	{
		Oplist		*code_olst;	/* only for instructions */
		Expr		*code_expr;	/* expression value */
		const Ulong	*code_align;	/* for Align/Backalign */
		Ulong		code_skip;	/* for String/Zero/Pad */
		Ulong		code_setadd;	/* incoming addend and... */
		long		code_addend;	/* outgoing (SecTy_RelocA) */
	} data;
	Ulong			code_addr;	/* section-relative */
	Ushort			code_size;	/* not if code_skip used */
	Uchar			code_kind;	/* CodeKind_* value */
	Uchar			code_impdep;	/* for implementations */
};

struct t_sect_	/* section information */
{
	Uchar		*sec_data;	/* final contents */
	Section		*sec_next;	/* next section in list */
	Section		*sec_relo;	/* matching relocation info */
	Symbol		*sec_sym;	/* symbol for section name */
	Code		*sec_code;	/* list of contents */
	Code		*sec_last;	/* last of list */
	Code		*sec_prev;	/* next-to-last of list */
	Code		*sec_lastpad;	/* most recent padding code */
	union
	{
		Expr	*sec_expr;	/* attribute as expression */
		Ulong	sec_value;	/* attribute as value */
	} attr;
	union
	{
		Expr	*sec_expr;	/* type as expression */
		Ulong	sec_value;	/* type as value */
	} type;
	Ulong		sec_align;	/* current max alignment */
	Ulong		sec_size;	/* current size of section */
	size_t		sec_index;	/* 1-based index for section */
	Ushort		sec_impdep;	/* for implementations */
	Uchar		sec_flags;	/* internal flags */
};

#ifdef __STDC__
void	initsect(void);
void	setsect(Symbol *, Expr *, Expr *);	/* define/set section */
Section	*relosect(Section *, int);		/* create matching reloc */
Section	*cursect(void);				/* return current sect */
void	prevsect(void);				/* toggle cur/prev sects */
int	stacksect(Section *);			/* push/pop sect stack */
void	walksect(void);				/* walk thru all sects */
void	gensect(void);				/* generate all contents */
int	sectoptalign(Section *, const Ulong *);	/* test alignment and fix */
void	sectalign(Section *, const Ulong *, Uint); /* add alignment request */
void	sectbackalign(Section *, const Symbol *, const Ulong *, Uint);
void	sectzero(Section *, Ulong);		/* add zero-valued bytes */
void	sectpad(Section *, Eval *);		/* add padding bytes */
void	sectexpr(Section *, Expr *, Ulong, int);/* add expression value */
void	sectstr(Section *, Expr *);		/* add string value */
void	sectvinst(Section *, const Inst *, Oplist *);	/* add var-inst */
void	sectfinst(Section *, const Inst *, Oplist *);	/* add fix-inst */
Code	*sectrelsec(Section *, int, Ulong, Section *);	/* section based */
Code	*sectrelsym(Section *, int, Ulong, Symbol *);	/* symbol based */

		/* implementation provides */
void	obssectattr(Section *, const Expr *);	/* handle obsolete attrs */
int	validalign(Ulong);			/* verify alignment value */
#else
void	initsect(), setsect(), prevsect();
int	stacksect();
Section	*relosect(), *cursect();
void	walksect(), gensect();
int	sectoptalign();
void	sectalign(), sectbackalign(), sectzero(), sectpad(), sectexpr();
void	sectstr(), sectvinst(), sectfinst();
Code	*sectrelsec(), *sectrelsym();

void	obssectattr();
int	validalign();
#endif
