/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sdb:cfront/src/typedef.h	1.2"
typedef unsigned char TOK;
typedef char bit;
typedef int (*PFI)();
typedef char (*PFV)();
typedef struct node * Pnode;
typedef struct key * Pkey;
typedef struct name * Pname;
typedef struct basetype * Pbase;
typedef struct type * Ptype;
typedef struct fct * Pfct;
typedef struct field * Pfield;
typedef struct expr * Pexpr;
typedef struct classdef * Pclass;
typedef struct enumdef * Penum;
typedef struct stmt * Pstmt;
typedef struct tstmt * Ptstmt;
typedef struct vec * Pvec;
typedef struct ptr * Pptr;
typedef struct table * Ptable;
typedef struct loc Loc;
typedef struct gen * Pgen;
typedef struct name_list * Plist;
typedef struct iline * Pin;
typedef struct nlist * Pnlist;
typedef struct slist * Pslist;
typedef struct elist * Pelist;
