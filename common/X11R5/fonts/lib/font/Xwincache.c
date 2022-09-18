#ident	"@(#)r5fontlib:font/Xwincache.c	1.18"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <fontstruct.h>
#include <stdio.h>
#include <fontfilest.h>
#include <Xwincache.h>
#include <sys/stat.h>
#include <unistd.h>

static int logcachestats=0;
static short xwincache_initialized=NULL;
static XwinFontCacheRec Xwincache;
static XwinFontCachePtr xwincache;
static const int csize=MIN_CACHED_PTRS;
static int logfile_opened = 0;
static FILE *log_fd;
static long pagesize=0;

int InitPerFontCache();
char *CacheGlyphs();
void FreePerFontCache();
int InitXwinFontCache();
void FreeXwinCache();
static void LogXwinCacheStats();
static int cache_free_only=0;
#ifdef NOTMACRO
static int CheckFontCache();
#else
#define CheckFontCache(s) ((xwincache->Xwin_availcache) >= (s)? 1: -1)
#endif
static pointer AllocFontMemory();
static int DoFontAlgorithm();
static char * GetGlyphFromCache();
static int FindFontToFree();
#define LOGFILE		"/tmp/cache.log"


InitPerFontCache(pFont, size, count)	
FontPtr pFont;
int size;
int count;
{
   
   pointer ptr;
   int result, err, newsize;

   if (logcachestats>0)
	LogXwinCacheStats("InitPerFontCache:: ");
#ifdef DEBUG
   fprintf(stderr,"InitPerFontCache:: pFont=%0x,size=%d, count=%d\n",pFont, size, count);
#endif

   if (!xwincache_initialized) 	{
	if (logcachestats>0)
	LogXwinCacheStats("InitPerFontCache:: xwincache not initialized\n");
	fprintf(stderr,"InitPerFontCache:: xwincache not initialized\n");
	return(-1);
  }

#ifdef XXX
   if (size > xwincache->Xwin_maxcache)  {
	fprintf(stderr,"InitPerFontCache:: size requested  greater than maxcache",size,xwincache->Xwin_maxcache);
	if (logcachestats > 0) LogXwinCachestats("InitPerFontCache: ");
	return (-1);
   }
#endif
		
       while ( (result = CheckFontCache(size)) == -1) {
	   err = FindFontToFree(pFont);
           if (err)	{
#ifdef DEBUG
		fprintf(stderr,"InitPerFontCache:: can't find enough room withFindFontToFree size=%d avail=%d\n",size,xwincache->Xwin_availcache);
#endif
		if (logcachestats>0)
		LogXwinCacheStats("InitPerFontCache:: can't find enough room withFindFontToFree");
		 /*return(err);*/
	   }
       }

   pFont->cachestats = (PerFontCachePtr)xalloc(sizeof(PerFontCacheRec));
   if (!pFont->cachestats) return  (-1);

   pFont->cachestats->perfont_cachealloc = 0;
   pFont->cachestats->perfont_cacheused = 0;
   pFont->cachestats->perfont_availcache = 0;
   pFont->cachestats->perfont_glyphsalloc = 0;
   pFont->cachestats->maxcachePrivate = 0; 
   pFont->cachestats->maxcachealloc = csize; 
   pFont->cachestats->CacheList = (pointer *) xalloc(sizeof(char *)*(csize+1));
   if(!pFont->cachestats->CacheList)  {
	xfree(pFont->cachestats);
	pFont->cachestats = 0;
        return (-1);
	}
   memset(pFont->cachestats->CacheList, 0, sizeof(char *)*(csize+1));


   if (xwincache->Xwin_cachedfonts_used == xwincache->Xwin_cachedfonts_size) {
        newsize = xwincache->Xwin_cachedfonts_used + csize;
        xwincache->Xwin_cachedfonts = (FontPtr *)xrealloc 
		(xwincache->Xwin_cachedfonts, sizeof(FontPtr)*(newsize+1));
        if (!xwincache->Xwin_cachedfonts)	{ 
		if (logcachestats>0)
		LogXwinCacheStats("InitPerFontCache xrealloc err");
		perror("InitPerFontCache:: xrealloc err:");
  		xfree(pFont->cachestats->CacheList);
		xfree(pFont->cachestats);
  		pFont->cachestats = 0;
		return (-1);
	}
        xwincache->Xwin_cachedfonts_size = newsize;
   	memset (xwincache->Xwin_cachedfonts+xwincache->Xwin_cachedfonts_used,
		0, sizeof(FontPtr)*(csize+1));
   }

   xwincache->Xwin_cachedfonts[xwincache->Xwin_cachedfonts_used] = pFont;
   xwincache->Xwin_cachedfonts_used++;
   newsize = size;

   if(size > 0)	{
       if(pFont->info.glyphsizeSet != 1)	{
	  if (pagesize != 0)
          	newsize = (int)(size/pagesize + 1) * pagesize; 
       }

      if (CheckFontCache(newsize) == -1) {
	   	newsize = size;
       }

       if (!(ptr = (pointer)AllocFontMemory(pFont,newsize))) 
       {
	    fprintf(stderr,"InitPerFontCache::AllocFontMemory error ptr=%0xsize=%d\n", ptr, newsize);
		FreePerFontCache(pFont);
	    	return -1;
       }

    }
   
   pFont->info.usingGlyphCache = 1 ;
   return 1;
}

char * CacheGlyphs(pFont, size, count,flag)
FontPtr pFont;
int size;
int count;
int flag;
{
        pointer *ptr, p;
	int result, err, index; 

#ifdef DEBUGL
	fprintf(stderr,"CacheGlyphs:: size=%d, count=%d\n", size, count);
#endif


#ifdef XXX
	if(size > xwincache->Xwin_maxcache) 	{
		fprintf(stderr,"CacheGlyphs:: size=%d greater than total cache=%d\n",
			size,  xwincache->Xwin_maxcache);
		return((char *) 0);
	}

#endif
	/* need to save count in the cache totals */
	if (size < 0) return (char *)0;
		/* check for invalid sizes */

	if (pFont->cachestats == (PerFontCachePtr) NULL) {
		/* check if cacheallocated for this font */
		fprintf(stderr,"CacheGlyphs:: trying to cache & not initialized");
		return (char *) 0;
	}

	pFont->cachestats->perfont_glyphsalloc += count;

	if(size <= pFont->cachestats->perfont_availcache)	{
		ptr = pFont->cachestats->CacheList;
		while(*ptr)	{	
		    if(size <= (int ) *(int *)*ptr)
		        return(GetGlyphFromCache(pFont, *ptr, size));
		    else	ptr++;
	        }
	    } 	/* size small but can't find fit */

	    result = DoFontAlgorithm(pFont, size, count);
	    if  (result <0) {
		result = size;
		/* if flag is true the allocation cannot fail, even
		if the total cache is exceeded temporarily */
		xwincache->Xwin_overrun++;
                pFont->info.freeWhen = FONT_FREE_NEXT;
		}
			/* free this font first since it overran
				the cache on allocation  */

	    
	    if (!(p=AllocFontMemory(pFont,result)))	{
#ifdef DEBUG
			fprintf(stderr,"CacheGlyphs:: AllocFontMemory error=%0x pFont=%0x result=%d\n",p,pFont,result);
#endif
       			return ((char *) 0);
	    }

	    return(GetGlyphFromCache(pFont,p, size));


}


void FreePerFontCache(pFont)
FontPtr pFont;
{
 /* adjust the cache statistics to reflect the memory freed */

	register int size = 0;
	register pointer *ptr;
	
	if (logcachestats>0)
	LogXwinCacheStats("FreePerFontCache before free:: ");
#ifdef DEBUG
	fprintf(stderr,"FreePerFontCache::font=%0x\n", pFont);
#endif

	if ((pFont->cachestats == (PerFontCachePtr) NULL)) {
#ifdef DEBUG
		fprintf(stderr,"trying to free %0x %0x twice\n ",pFont, pFont->cachestats);
#endif
		return;
	}
	ptr = pFont->cachestats->CacheList;
	while (*ptr)	{
		xfree(*ptr);
		ptr ++;
	}

        xwincache->Xwin_cachealloc -= pFont->cachestats->perfont_cachealloc;
        xwincache->Xwin_availcache += pFont->cachestats->perfont_cachealloc; 

	if (!cache_free_only)  {
        	xwincache->Xwin_cachedfonts_used --;
		xfree (pFont->cachestats->CacheList);
		xfree (pFont->cachestats);
		pFont->cachestats = 0;
		delfont(pFont);
	}else {
		xfree (pFont->cachestats->CacheList);
		memset(pFont->cachestats, 0 , sizeof(PerFontCacheRec));
			/* keep the cachestats but zero out for reuse 
				when font glyphs were freed by cache */
	
		pFont->info.freed++;
		pFont->info.reallocate = 1;
		pFont->info.freeWhen = 0;
		pFont->info.rerender = 1;

	}
	if (logcachestats>1)
	LogXwinCacheStats("FreePerFontCache after free::");
}

pointer
AllocFontMemory(pFont, size)
FontPtr pFont;
int size;
{
	pointer  *ptr, p;
	short newsize;
	register short i = pFont->cachestats->maxcachePrivate  ;
	
#ifdef DEBUGL
	fprintf(stderr,"AllocFontMemory:: size=%d,maxcacheP=%0x\n", size, i);
#endif
	if(pFont->cachestats->maxcachealloc == i)	{
  	    newsize = i + csize;
	    pFont->cachestats->CacheList = (pointer *) xrealloc
		(pFont->cachestats->CacheList, sizeof(char *)*(newsize+1));
	    if (!pFont->cachestats->CacheList)	{
		if (logcachestats>0)
		LogXwinCacheStats("AllocFontMemory: xrealloc err: " );
		perror("AllocFontMemory:: xrealloc err:");
		return((pointer) 0);
	    }
	    pFont->cachestats->maxcachealloc = newsize;
	    memset(pFont->cachestats->CacheList+i, 0, sizeof(char *)*(csize+1));
	}

	p=(pointer) xalloc(size+ 2*sizeof(int));
	if (!p) {
		perror("Xwincache: allocation err CacheList ");
		return(pointer) 0;
	}
	pFont->cachestats->CacheList[i] = p;
	*(int *)p = size;
	p += sizeof(int);
	*(int *)p = (int)( p + sizeof(int));

	pFont->cachestats->perfont_cachealloc += size;
	pFont->cachestats->perfont_availcache += size;
	pFont->cachestats->maxcachePrivate ++ ;
	xwincache->Xwin_cachealloc += size;
	xwincache->Xwin_availcache -= size;
	return (pFont->cachestats->CacheList[i]);
}

int
DoFontAlgorithm(pFont, size, count)
FontPtr pFont;
int size;
int count;
{
	int  err, newsize;
	int amount=0;

#ifdef DEBUGL
	fprintf(stderr,"DoFontAlgorithm:: size=%d, count=%d\n", size, count);
#endif

	while ( CheckFontCache(size) == -1) {
           err = FindFontToFree(pFont);
           if (err) 	{
		if (logcachestats>0)
		LogXwinCacheStats("DoFontAlgorithm: findfonttofree ");
#ifdef DEBUG
		fprintf(stderr,"DoFontAlgorithm: findfonttofree err size=%d xwincache->Xwin_availcache=%d\n",size,xwincache->Xwin_availcache);
#endif
		return(err);
	   }
	   amount = 1;
	}
	if(amount) 	 return(size);
	if(((xwincache->Xwin_cachealloc+size) >= xwincache->Xwin_lowwater))
		return(size);


	amount=pFont->info.tot_glyphsize-pFont->cachestats->perfont_cacheused;
	if ((amount <=0) && (pFont->info.glyphsizeSet == 0)) amount = size;
		/* don't know total size so at least allocate size asked for */
	if(amount <=0)	{
#ifdef DEBUG
		fprintf(stderr,"DoFontAlogrithm:: amount=%d totglyphsize=%d cachealloc=%d\n",amount,pFont->info.tot_glyphsize, pFont->cachestats->perfont_cachealloc);
		fprintf(stderr,"DoFontAlogrithm:: perfont_cacheused=%d perfontglyphsalloc=%d perfont_availacache=%d\n",pFont->cachestats->perfont_cacheused,pFont->cachestats->perfont_glyphsalloc, pFont->cachestats->perfont_availcache);
#endif
		if (logcachestats>0)
		LogXwinCacheStats("DoFontAlgorithm: tot_glyphsize less then alloc:");
	}
	if(pFont->info.alloc_units<=0)
		newsize = pagesize;
	else
		newsize = pFont->info.alloc_units*pagesize;
	newsize = amount >= newsize ? newsize : amount;
	while (size > newsize) {
	
	newsize =  newsize+pagesize;
	/* need to check that the pagesize boundary is big enough */
	}
        if ( CheckFontCache(newsize) == -1) {
		return size;
		/* return original size if new size would cause
		use to have to free a font to satisfy a bigger
		"pagesize boundary" allocation */
        }
	if((xwincache->Xwin_cachealloc+newsize)>=xwincache->Xwin_lowwater)
		return (size);
	return(newsize);
}

char *
GetGlyphFromCache(pFont, index, size)
FontPtr pFont;
char * index;	
int size;
{
	char *ptr = index;
	char *cacheptr;

#ifdef DEBUGL
	fprintf(stderr,"GetGlyphFromCache:: pFont=%0x,index=%0x,size=%d\n", 
		pFont, ptr,size);
#endif

	*(int *)ptr -= size;
	ptr += sizeof(int);
	cacheptr =(char *) *(int *)ptr;
	*(int *)ptr += size;

	pFont->cachestats->perfont_cacheused += size;
	pFont->cachestats->perfont_availcache -= size;
	return(cacheptr);
}

#ifdef NOTMACRO
int
CheckFontCache(size)	
int size;
{
#ifdef DEBUG
	fprintf(stderr,"CheckFontCache:: size=%d\n", size);

#endif
  if (xwincache->Xwin_availcache >= size) return(TRUE);

  return(-1);
}

#endif

int
InitXwinFontCache(mincache, maxcache, log)	
long mincache;
long maxcache;
int log;
{


#ifdef DEBUG
	fprintf(stderr,"InitXwinFontCache:: min=%ld max=%dlog =%d\n", mincache, maxcache,log);
#endif

       if(!pagesize)
	       pagesize = sysconf(_SC_PAGESIZE);

	if (!pagesize) pagesize = 4096;	/* set default pagesize */
	xwincache = &Xwincache;
	xwincache->Xwin_maxcache = maxcache;
	xwincache->Xwin_lowwater = mincache;
	xwincache->Xwin_availcache = maxcache;
	xwincache->Xwin_cachealloc = 0;
	xwincache->Xwin_cache_callbacks = 0;
	xwincache->Xwin_overrun = 0;
	xwincache->Xwin_cachedfonts_used = 0;
	xwincache->Xwin_cachedfonts_size = csize; 


	xwincache->Xwin_cachedfonts = 
			(FontPtr *)xalloc (sizeof(FontPtr)*(csize+1));

	if (!xwincache->Xwin_cachedfonts) 	{
		fprintf(stderr,"xalloc to Xwin_cachedfonts failed\n");
		if (logcachestats>0)
		LogXwinCacheStats("xalloc to Xwin_cachedfonts failed");
		return (TRUE);
	}
	memset(xwincache->Xwin_cachedfonts, 0, sizeof(FontPtr)*(csize+1));
	xwincache_initialized  =  1;


	if (log > 0 && logfile_opened == 0) {
	if((log_fd = fopen("/tmp/cache.log", "w")) == NULL)	{
		perror("InitXwinFontCache Logfile open::");
#ifdef DEBUG
		fprintf(stderr,"Open logfile failed :%s\n", LOGFILE, 0);
#endif
	        logcachestats = 0;	
		return(TRUE);
	}
	logfile_opened = 1;
	logcachestats = log;
	LogXwinCacheStats("XwinFontCache Initialized ");

	}
	return(TRUE);
}

int
FreeXwinFontCache()
{
    
	FontPtr *ptr;
	pointer tmp;
	pointer *listptr;

#ifdef DEBUG
	fprintf(stderr,"FreeXwinFontCache::\n");
#endif
	if (logcachestats>0)
	LogXwinCacheStats("FreeXwinFontCache:: ");

        if (!xwincache_initialized)   {
	        fprintf(stderr,"FreeXwinFontCache:: xwincache not initialized\n");
	        return(-1);
        }

	xwincache_initialized = 0;
	ptr = xwincache->Xwin_cachedfonts;
	
	while (*ptr)	{
		FreePerFontCache(*ptr);
		ptr ++;
	}
	xfree(xwincache->Xwin_cachedfonts);
	xwincache->Xwin_cachedfonts = NULL;	    
	xwincache->Xwin_maxcache = 0;
	xwincache->Xwin_lowwater =  0;
	xwincache->Xwin_availcache = 0;
	xwincache->Xwin_cachealloc = 0;
	xwincache->Xwin_cache_callbacks = 0;
	xwincache->Xwin_cachedfonts_size = 0;
	xwincache->Xwin_cachedfonts_used = 0;

	if (logcachestats>0) {
	LogXwinCacheStats("Everything freed FreeXwinFontCache:: ");
	}
	return(TRUE);
}

void
LogXwinCacheStats(msg)
char *msg;
{
	FontPtr ptr,*pptr;
	pointer *ptmp, tmp;
	register i;


	fprintf(log_fd,"%s \n",msg);
	fprintf(log_fd,"%s::\n*************  xwincache Record *******\n",msg);
	fprintf(log_fd,"	maxcache:%ld\t	lowwater:%ld\t\
	availcache:%d\n	cachealloc:%d\t	callback:%d\t\
	cache used:%ld\n	cache_size:%ld	overrun:%d\n\n", xwincache->Xwin_maxcache, 
	xwincache->Xwin_lowwater, xwincache->Xwin_availcache,
	xwincache->Xwin_cachealloc,xwincache->Xwin_cache_callbacks,
	xwincache->Xwin_cachedfonts_used,xwincache->Xwin_cachedfonts_size,
	xwincache->Xwin_overrun);
	
	pptr = xwincache->Xwin_cachedfonts;
	i = xwincache->Xwin_cachedfonts_used;
	fprintf(log_fd,"cachedfonts=%d\n",i);

	if (logcachestats > 1) {
	while(i--)	{
	ptr = *pptr;
	fprintf(log_fd,"     +++++++++++ PerFontRec:: %0x +++++\n",ptr);
        fprintf(log_fd,"	cachealloc=%d\t	cacheused=%d\t	availcache=%d\n\
	glyphsalloc=%d\t	maxcachePrivates=%d\n", 
		ptr->cachestats->perfont_cachealloc,
	 	ptr->cachestats->perfont_cacheused,
		ptr->cachestats->perfont_availcache,
		ptr->cachestats->perfont_glyphsalloc,
		ptr->cachestats->maxcachePrivate);

	fprintf(log_fd," 	freeWhen=%d\t freed=%d\n",ptr->info.freeWhen, ptr->info.freed);
	ptmp = ptr->cachestats->CacheList;
	if (logcachestats > 2) {
	while (*ptmp)	{	
	    tmp = *ptmp;
	    fprintf(log_fd,"	-----ptr=%0x, avail=%d::freeptr=%0x\n", 
			tmp,*(int *)tmp, *(int *)(tmp+sizeof(int)));
	    ptmp++;
	}
	}
	pptr ++;
	} /* end while */
	} /* only do while when cachestats level 2 or greater */
	
	fprintf(log_fd,"\n");
	fflush(log_fd);
}




int
FindFontToFree(pFont)	
FontPtr pFont;
{
	register FontPtr target,next, *ptr;
	FontPtr font_to_free;
	int i,max,result;

#ifdef DEBUG
	fprintf(stderr,"FindFontToFree:: pFont=%0x\n", pFont, 0);
#endif
	max = xwincache->Xwin_cachedfonts_used;
	font_to_free = NULL;
        target  = *xwincache->Xwin_cachedfonts;

	for (i =0; i < max ; i++) {
		next = xwincache->Xwin_cachedfonts[i+1];
		if (next == NULL) break;
			/* end of fonts */
		if (target == pFont) {
			target = next;
			continue;
		}
		if ((target->cachestats) && 
		(target->cachestats->perfont_cachealloc == 0)) {
			target = next;
			continue;
		}
		if (target->info.freeWhen == FONT_FREE_NEXT) {
			font_to_free = target;
			break ;
		}
		if (target->info.freeWhen == FONT_DONT_FREE) {
			target = next;
			continue;
		}
		if (next == pFont) continue;

		if (next->info.freeWhen == FONT_FREE_NEXT) {
			font_to_free = next;
			break;
		}
		if ((next->cachestats) && 
		(next->cachestats->perfont_cachealloc == 0)) {
			continue;
		}
		if (next->info.freeWhen == FONT_DONT_FREE) {
			continue;
		}

		result = compscore(target, next);
		/* returns one to keep */
	
		if (result ==  1)  {
			target = next;
			continue ;
		}
	} /* end for */

/* needs to call the font's free_glyphs routine  */

	if (font_to_free == NULL) {
		if ((target->info.freeWhen != FONT_DONT_FREE) &&
		(target->cachestats) &&
		(target->cachestats->perfont_cachealloc > 0) &&
		(target != pFont))
		font_to_free = target;
	else 
		return -1;
		/* no font to free */
	}

	if (font_to_free != NULL ) {
		if (*font_to_free->free_glyphs == NULL) 
			return -1;
		/* no free routine for this font */
	if (logcachestats>0) 
	LogXwinCacheStats("freeing a font from cache to make room",0,0,0);

		(*font_to_free->free_glyphs) (font_to_free);
		xwincache->Xwin_cache_callbacks++;
		cache_free_only =  1;
		FreePerFontCache(font_to_free);
		cache_free_only = 0;
		return(0);
	}

}

#ifdef XXX
int
compscore(target,next)
FontPtr target,next;
{
	register int targetscore, nextscore, i;

#ifdef DEBUG
	fprintf(stderr,"compscore::target=%0x, next=%0x\n", target, next);
#endif

	targetscore = nextscore = 0;

	if (target->info.last_blittime > next->info.last_blittime)
		targetscore += 10;
	else
	if (target->info.last_blittime < next->info.last_blittime)
		nextscore += 10;

	if (target->info.render_time > next->info.render_time )
                targetscore += 10;
	else
	if (target->info.render_time < next->info.render_time )
                nextscore += 10;

	targetscore += target->refcnt;
	nextscore += next->refcnt;

	if (targetscore == nextscore) {
		if(target->cachestats->perfont_cachealloc >= 
		   next->cachestats->perfont_cachealloc)  
			return(2);
		else	return (1);
	}
	return(targetscore > nextscore ? 1 : 2);
		/* return number of one to keep checking */
}

#endif 



int
compscore(target,next)
FontPtr target,next;
{
	register int targetscore, nextscore, i;

#ifdef DEBUG
	fprintf(stderr,"compscore::target=%0x, next=%0x\n", target, next);
	fprintf(stderr,"target: refcnt=%d render_time=%d blittime=%d used=%d\n",target->refcnt,target->info.render_time, target->info.last_blittime,target->cachestats->perfont_cachealloc);
	fprintf(stderr,"next: refcnt=%d render_time=%d blittime=%d used=%d\n",next->refcnt,next->info.render_time, next->info.last_blittime,next->cachestats->perfont_cachealloc);
#endif

	targetscore = target->info.last_blittime ;
			/* +target->refcnt + target->info.render_time;*/
	nextscore = next->info.last_blittime;
		/* + next->refcnt + next->info.render_time;*/

#ifdef DEBUG
fprintf(stderr,"targetscore=%d nextscore=%d\n",targetscore,nextscore);
#endif

	if (targetscore == nextscore) {
		if(target->cachestats->perfont_cachealloc >= 
		   next->cachestats->perfont_cachealloc)  
			return(2);
		else	return (1);
	}
	return(targetscore > nextscore ? 1 : 2);
		/* keep one with highest score */
		/* return number of one to keep checking */
}

delfont(pFont)
FontPtr pFont;
{
	int i;

#ifdef DEBUG
	fprintf(stderr,"delfont::\n",0,0);
#endif

	for(i=0; pFont != xwincache->Xwin_cachedfonts[i]; i++)
	;

	for(; i<xwincache->Xwin_cachedfonts_used; i++)	
	   xwincache->Xwin_cachedfonts[i] = xwincache->Xwin_cachedfonts[i+1];
	xwincache->Xwin_cachedfonts[i] = NULL;
}
		
	
