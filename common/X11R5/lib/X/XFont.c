/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFont.c	1.2"

#define NEED_REPLIES
#include "Xlibint.h"

static XFontStruct *_XQueryFont();

#if NeedFunctionPrototypes
XFontStruct *XLoadQueryFont(
   register Display *dpy,
   _Xconst char *name)
#else
XFontStruct *XLoadQueryFont(dpy, name)
   register Display *dpy;
   char *name;
#endif
{
    XFontStruct *font_result;
    register long nbytes;
    Font fid;
    xOpenFontReq *req;
    unsigned long seq;

    LockDisplay(dpy);
    GetReq(OpenFont, req);
    seq = dpy->request;
    nbytes = req->nbytes  = name ? strlen(name) : 0;
    req->fid = fid = XAllocID(dpy);
    req->length += (nbytes+3)>>2;
    Data (dpy, name, nbytes);
    font_result = _XQueryFont(dpy, fid, seq);
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}

XFreeFont(dpy, fs)
    register Display *dpy;
    XFontStruct *fs;
{ 
    register xResourceReq *req;
    register _XExtension *ext = dpy->ext_procs;

    LockDisplay(dpy);
    while (ext) {		/* call out to any extensions interested */
	if (ext->free_Font) (*ext->free_Font)(dpy, fs, &ext->codes);
	ext = ext->next;
	}    
    GetResReq (CloseFont, fs->fid, req);
    UnlockDisplay(dpy);
    SyncHandle();
    _XFreeExtData(fs->ext_data);
    if (fs->per_char)
       Xfree ((char *) fs->per_char);
    if (fs->properties)
       Xfree ((char *) fs->properties);
    Xfree ((char *) fs);
}

static XFontStruct *
_XQueryFont (dpy, fid, seq)
    register Display *dpy;
    Font fid;
    unsigned long seq;
{
    register XFontStruct *fs;
    register long nbytes;
    xQueryFontReply reply;
    register xResourceReq *req;
    register _XExtension *ext;
    _XAsyncHandler async;
    _XAsyncErrorState async_state;

    if (seq) {
	async_state.min_sequence_number = seq;
	async_state.max_sequence_number = seq;
	async_state.error_code = BadName;
	async_state.major_opcode = X_OpenFont;
	async_state.minor_opcode = 0;
	async_state.error_count = 0;
	async.next = dpy->async_handlers;
	async.handler = _XAsyncErrorHandler;
	async.data = (XPointer)&async_state;
	dpy->async_handlers = &async;
    }
    GetResReq(QueryFont, fid, req);
    if (!_XReply (dpy, (xReply *) &reply,
       ((SIZEOF(xQueryFontReply) - SIZEOF(xReply)) >> 2), xFalse)) {
	if (seq)
	    DeqAsyncHandler(dpy, &async);
	return (XFontStruct *)NULL;
    }
    if (seq)
	DeqAsyncHandler(dpy, &async);
    if (! (fs = (XFontStruct *) Xmalloc (sizeof (XFontStruct)))) {
	_XEatData(dpy, (unsigned long)(reply.nFontProps * SIZEOF(xFontProp) +
				       reply.nCharInfos * SIZEOF(xCharInfo)));
	return (XFontStruct *)NULL;
    }
    fs->ext_data 		= NULL;
    fs->fid 			= fid;
    fs->direction 		= reply.drawDirection;
    fs->min_char_or_byte2	= reply.minCharOrByte2;
    fs->max_char_or_byte2 	= reply.maxCharOrByte2;
    fs->min_byte1 		= reply.minByte1;
    fs->max_byte1 		= reply.maxByte1;
    fs->default_char 		= reply.defaultChar;
    fs->all_chars_exist 	= reply.allCharsExist;
    fs->ascent 			= cvtINT16toInt (reply.fontAscent);
    fs->descent 		= cvtINT16toInt (reply.fontDescent);
    
#ifdef MUSTCOPY
    {
	xCharInfo *xcip;

	xcip = (xCharInfo *) &reply.minBounds;
	fs->min_bounds.lbearing = cvtINT16toShort(xcip->leftSideBearing);
	fs->min_bounds.rbearing = cvtINT16toShort(xcip->rightSideBearing);
	fs->min_bounds.width = cvtINT16toShort(xcip->characterWidth);
	fs->min_bounds.ascent = cvtINT16toShort(xcip->ascent);
	fs->min_bounds.descent = cvtINT16toShort(xcip->descent);
	fs->min_bounds.attributes = xcip->attributes;

	xcip = (xCharInfo *) &reply.maxBounds;
	fs->max_bounds.lbearing = cvtINT16toShort(xcip->leftSideBearing);
	fs->max_bounds.rbearing =  cvtINT16toShort(xcip->rightSideBearing);
	fs->max_bounds.width =  cvtINT16toShort(xcip->characterWidth);
	fs->max_bounds.ascent =  cvtINT16toShort(xcip->ascent);
	fs->max_bounds.descent =  cvtINT16toShort(xcip->descent);
	fs->max_bounds.attributes = xcip->attributes;
    }
#else
    /* XXX the next two statements won't work if short isn't 16 bits */
    fs->min_bounds = * (XCharStruct *) &reply.minBounds;
    fs->max_bounds = * (XCharStruct *) &reply.maxBounds;
#endif /* MUSTCOPY */

    fs->n_properties = reply.nFontProps;
    /* 
     * if no properties defined for the font, then it is bad
     * font, but shouldn't try to read nothing.
     */
    fs->properties = NULL;
    if (fs->n_properties > 0) {
	    nbytes = reply.nFontProps * sizeof(XFontProp);
	    fs->properties = (XFontProp *) Xmalloc ((unsigned) nbytes);
	    nbytes = reply.nFontProps * SIZEOF(xFontProp);
	    if (! fs->properties) {
		Xfree((char *) fs);
		_XEatData(dpy, (unsigned long)
			  (nbytes + reply.nCharInfos * SIZEOF(xCharInfo)));
		return (XFontStruct *)NULL;
	    }
	    _XRead32 (dpy, (char *)fs->properties, nbytes);
    }
    /*
     * If no characters in font, then it is a bad font, but
     * shouldn't try to read nothing.
     */
    /* have to unpack charinfos on some machines (CRAY) */
    fs->per_char = NULL;
    if (reply.nCharInfos > 0){
	nbytes = reply.nCharInfos * sizeof(XCharStruct);
	if (! (fs->per_char = (XCharStruct *) Xmalloc ((unsigned) nbytes))) {
	    if (fs->n_properties) Xfree((char *) fs->n_properties);
	    Xfree((char *) fs);
	    _XEatData(dpy, (unsigned long)
			    (reply.nCharInfos * SIZEOF(xCharInfo)));
	    return (XFontStruct *)NULL;
	}
	    
#ifdef MUSTCOPY
	{
	    register XCharStruct *cs = fs->per_char;
	    register int i;

	    for (i = 0; i < reply.nCharInfos; i++, cs++) {
		xCharInfo xcip;

		_XRead(dpy, &xcip, SIZEOF(xCharInfo));
		cs->lbearing = cvtINT16toShort(xcip.leftSideBearing);
		cs->rbearing = cvtINT16toShort(xcip.rightSideBearing);
		cs->width =  cvtINT16toShort(xcip.characterWidth);
		cs->ascent =  cvtINT16toShort(xcip.ascent);
		cs->descent =  cvtINT16toShort(xcip.descent);
		cs->attributes = xcip.attributes;
	    }
	}
#else
	nbytes = reply.nCharInfos * SIZEOF(xCharInfo);
	_XRead16 (dpy, (char *)fs->per_char, nbytes);
#endif
    }

    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
	if (ext->create_Font)
		(*ext->create_Font)(dpy, fs, &ext->codes);
	ext = ext->next;
	}    
    return fs;
}


XFontStruct *XQueryFont (dpy, fid)
    register Display *dpy;
    Font fid;
{
    XFontStruct *font_result;

    LockDisplay(dpy);
    font_result = _XQueryFont(dpy, fid, 0L);
    UnlockDisplay(dpy);
    SyncHandle();
    return font_result;
}

   
   
