/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/patcache.c	1.2"
/*

 * $XConsortium: patcache.c,v 1.2 91/06/14 10:58:11 rws Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#include <stdio.h>
#include    <fontmisc.h>
#include    <fontstruct.h>

/*
 * Static sized hash table for looking up font name patterns
 *
 * LRU entries, reusing old entries
 */

#define NBUCKETS	16
#define NENTRIES	64

#define UNSET		(NENTRIES+1)

typedef unsigned char	EntryPtr;

typedef struct _FontPatternCacheEntry {
    struct _FontPatternCacheEntry   *next, **prev;
    short			    patlen;
    char			    *pattern;
    int				    hash;
    FontPtr			    pFont;	/* associated font */
} FontPatternCacheEntryRec, *FontPatternCacheEntryPtr;

typedef struct _FontPatternCache {
    FontPatternCacheEntryPtr	buckets[NBUCKETS];
    FontPatternCacheEntryRec	entries[NENTRIES];
    FontPatternCacheEntryPtr	free;
} FontPatternCacheRec;

/* Create and initialize cache */
FontPatternCachePtr
MakeFontPatternCache ()
{
    FontPatternCachePtr	cache;
    int			i;
    cache = (FontPatternCachePtr) xalloc (sizeof *cache);
    if (!cache)
	return 0;
    for (i = 0; i < NENTRIES; i++)
    {
	cache->entries[i].patlen = 0;
	cache->entries[i].pattern = 0;
	cache->entries[i].pFont = 0;
    }
    EmptyFontPatternCache (cache);
    return cache;
}

/* toss cache */
void
FreeFontPatternCache (cache)
    FontPatternCachePtr	cache;
{
    int	    i;

    for (i = 0; i < NENTRIES; i++)
	xfree (cache->entries[i].pattern);
    xfree (cache);
}

/* compute id for string */
static
Hash (string, len)
    char    *string;
    int	    len;
{
    int	hash;

    hash = 0;
    while (len--)
	hash = (hash << 1) ^ *string++;
    if (hash < 0)
	hash = -hash;
    return hash;
}

/* Empty cache (for rehash) */
EmptyFontPatternCache (cache)
    FontPatternCachePtr	cache;
{
    int	    i;
    
    for (i = 0; i < NBUCKETS; i++)
	cache->buckets[i] = 0;
    for (i = 0; i < NENTRIES - 1; i++)
    {
	cache->entries[i].next = &cache->entries[i+1];
	cache->entries[i].prev = 0;
	cache->entries[i].pFont = 0;
    }
    cache->free = &cache->entries[0];
    cache->entries[i].next = 0;
}

/* add entry */
void
CacheFontPattern (cache, pattern, patlen, pFont)
    FontPatternCachePtr	cache;
    char		*pattern;
    int			patlen;
    FontPtr		pFont;
{
    FontPatternCacheEntryPtr	e;
    char			*newpat;
    int				i;

    newpat = (char *) xalloc (patlen);
    if (!newpat)
	return;
    if (e = cache->free)
    {
	cache->free = e->next;
    }
    else
    {
    	i = rand ();
    	if (i < 0)
	    i = -i;
    	i %= NENTRIES;
	e = &cache->entries[i];
	if (e->next)
	    e->next->prev = e->prev;
	*e->prev = e->next;
	xfree (e->pattern);
    }
    /* set pattern */
    memcpy (newpat, pattern, patlen);
    e->pattern = newpat;
    e->patlen = patlen;
    /* link to new hash chain */
    e->hash = Hash (pattern, patlen);
    i = e->hash % NBUCKETS;
    if (e->next = cache->buckets[i])
	e->next->prev = &(e->next);
    cache->buckets[i] = e;
    e->prev = &(cache->buckets[i]);
    e->pFont = pFont;
}

/* find matching entry */
FontPtr
FindCachedFontPattern (cache, pattern, patlen)
    FontPatternCachePtr	cache;
    char		*pattern;
    int			patlen;
{
    int				hash;
    int				i;
    FontPatternCacheEntryPtr	e;

    hash = Hash (pattern, patlen);
    i = hash % NBUCKETS;
    for (e = cache->buckets[i]; e; e = e->next)
    {
	if (e->hash == hash && e->patlen == patlen &&
	    !bcmp (e->pattern, pattern, patlen))
	{
	    return e->pFont;
	}
    }
    return 0;
}

void
RemoveCachedFontPattern (cache, pFont)
    FontPatternCachePtr	cache;
    FontPtr		pFont;
{
    FontPatternCacheEntryPtr	e;
    int				i;

    for (i = 0; i < NENTRIES; i++)
    {
	if ((e = &cache->entries[i])->pFont == pFont)
	{
	    e->pFont = 0;
	    if (e->next)
		e->next->prev = e->prev;
	    *e->prev = e->next;
	    e->next = cache->free;
	    cache->free = e;
	    xfree (e->pattern);
	    e->pattern = 0;
	}
    }
}
