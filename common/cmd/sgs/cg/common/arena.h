/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cg:common/arena.h	1.3"
#include "manifest.h"

#ifdef __STDC__
#define PROTO(x,y) x y
#else
#define PROTO(x,y) x()
#endif
typedef struct Arena_struct *Arena;

extern myVOID * PROTO(	arena_alloc,(Arena,int size));
extern void PROTO(	arena_term,(Arena));
extern Arena PROTO(	arena_init,(void));

#define Arena_alloc(arena,cnt,type) \
	(type *) arena_alloc(arena,(cnt)*sizeof(type))
