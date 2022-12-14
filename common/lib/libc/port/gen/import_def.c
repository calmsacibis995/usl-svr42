/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/import_def.c	1.1"

#include "synonyms.h"
#include <stdlib.h>

VOID * (* _libc_malloc)() = &malloc;
VOID * (* _libc_realloc)() = &realloc;
VOID * (* _libc_calloc)() = &calloc;
void (* _libc_free)() = &free;
