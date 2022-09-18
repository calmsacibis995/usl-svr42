#ident	"@(#)r5fontinc:include/Xwincache.h	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef XWINCACHE
#define XWINCACHE 

#define MIN_CACHED_PTRS  10

#include "fontmisc.h"


typedef struct _XwinFontCacheRec {
           long      Xwin_maxcache;
           long      Xwin_lowwater;
	   long	     Xwin_dontfree;
 	   long	     Xwin_overrun;
           long      Xwin_availcache;
           long      Xwin_cachealloc;
           int       Xwin_cache_callbacks;
           short     Xwin_cachedfonts_used;  /* number of pfont ptrs used */ 
           short     Xwin_cachedfonts_size; /* number of pfont ptrs allowed */
           FontPtr   *Xwin_cachedfonts;
} XwinFontCacheRec, *XwinFontCachePtr;


typedef struct _PerFontCacheRec {
           int      perfont_cachealloc; /* total space allocated this font */
           int      perfont_cacheused;	/* used */
           int      perfont_availcache; /* avail */
           short    perfont_glyphsalloc; /* # of glyphs alloc so far */
           short    maxcachePrivate; /* # of CacheList have been used */
           short    maxcachealloc; /* # of CacheList have been alloc */
           pointer     * CacheList;
} PerFontCacheRec, *PerFontCachePtr;          

#endif /* XWINCACHE */
