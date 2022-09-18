/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olmisc:buffutil.h	1.8"
#endif

/*
 * buffutil.h
 *
 */

#ifndef _buffutil_h
#define _buffutil_h

#include <stdio.h>
#ifdef I18N
#ifndef MEMUTIL
#include <stdlib.h>             /* for wchar_t data type */
#else	
#ifndef _WCHAR_T
#define _WCHAR_T
typedef long wchar_t;
#endif /* _WCHAR_T */
#endif /* MEMUTIL */
#include <widec.h>
#endif /* I18N */

	/* shouldn't check for SEEK_SET because it defined in unistd.h  */
	/* in SVR3.2 and X11/Xos.h will include this header file if USG */
#ifndef __STDC__
#ifndef __cplusplus
#ifndef c_plusplus
#define memmove(dest, src, n)  bcopy((char*)src, (char*)dest, (int)n)
#endif
#endif
#endif

#ifdef I18N
typedef wchar_t BufferElement;
#else
typedef char BufferElement;
#endif


#define Bufferof(type) \
   struct \
      { \
      int    size; \
      int    used; \
      int    esize; \
      type * p; \
      }

typedef struct _Buffer
   {
   int    size;
   int    used;
   int    esize;
   BufferElement * p;
   } Buffer;

#define LNMIN       8
#define LNINCRE    24

#define BufferFilled(b)  (b-> size == b-> used)
#define BufferLeft(b)    (b-> size - b-> used)
#define BufferEmpty(b)   (b-> used == 0)

extern Buffer * AllocateBuffer(int element_size, int initial_size);
extern void     GrowBuffer(Buffer *, int increment);
extern Buffer * CopyBuffer(Buffer *);
extern void     FreeBuffer(Buffer *);
extern int      InsertIntoBuffer(Buffer * target, Buffer * source, int offset);
extern int      ReadFileIntoBuffer(FILE * fp, Buffer * buffer);
extern int	ReadStringIntoBuffer(Buffer * sp, Buffer * buffer);
extern Buffer * stropen(char *);
extern Buffer * wcstropen(BufferElement * string);
extern int      strgetc(Buffer *);
extern void     strclose(Buffer *);

#ifdef I18N
extern int _mbstrlen(char * mbstring);
#endif
#endif
