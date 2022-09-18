/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/multibufst.h	1.1"
/*
 * $XConsortium: multibufst.h,v 1.7 91/01/05 16:32:49 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 *
 */

#ifndef _MULTIBUFST_H_
#define _MULTIBUFST_H_

/*
 * Protocol requests constants and alignment values
 * These would really be in Multi-Buffering's X.h and Xproto.h equivalents
 */

#include "multibuf.h"

#if defined(__STDC__) && !defined(UNIXCPP)
#define MbufGetReq(name,req,info) GetReq (name, req); \
	req->reqType = info->codes->major_opcode; \
	req->mbufReqType = X_##name;
#else
#define MbufGetReq(name,req,info) GetReq (name, req); \
	req->reqType = info->codes->major_opcode; \
	req->mbufReqType = X_/**/name;
#endif

#define Multibuffer CARD32

typedef struct {
	CARD32	visualID B32;		/* associated visual */
	CARD16	maxBuffers B16;		/* maximum supported buffers */
	CARD8	depth;			/* depth of visual (redundant) */
	CARD8	unused;
} xMbufBufferInfo;
#define sz_xMbufBufferInfo 8

typedef struct {
    BYTE    type;
    BYTE    unused;
    CARD16  sequenceNumber B16;
    CARD32  buffer B32;			/* affected buffer */
    BYTE    state;			/* current status */
    CARD8   unused1;
    CARD16  unused2 B16;
    CARD32  unused3 B32;
    CARD32  unused4 B32;
    CARD32  unused5 B32;
    CARD32  unused6 B32;
    CARD32  unused7 B32;
} xMbufClobberNotifyEvent;

typedef struct {
    BYTE    type;
    BYTE    unused;
    CARD16  sequenceNumber B16;
    CARD32  buffer B32;			/* affected buffer */
    CARD32  timeStamp B32;		/* update time */
    CARD32  unused1 B32;
    CARD32  unused2 B32;
    CARD32  unused3 B32;
    CARD32  unused4 B32;
    CARD32  unused5 B32;
    CARD32  unused6 B32;
} xMbufUpdateNotifyEvent;

typedef struct {
    CARD8	reqType;		/* always codes->major_opcode */
    CARD8	mbufReqType;		/* always X_MbufGetBufferVersion */
    CARD16	length B16;
} xMbufGetBufferVersionReq;
#define sz_xMbufGetBufferVersionReq	4

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD8	majorVersion;	/* major version of Multi-Buffering protocol */
    CARD8	minorVersion;	/* minor version of Multi-Buffering protocol */
    CARD16	pad1 B16;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
    CARD32	pad6 B32;
} xMbufGetBufferVersionReply;
#define sz_xMbufGetBufferVersionReply	32

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufCreateImageBuffers */
    CARD16	length B16;
    CARD32	window B32;	/* associated window */
    CARD8	updateAction;	/* action at update */
    CARD8	updateHint;	/* hint as to frequency of updates */
    CARD16	unused;
} xMbufCreateImageBuffersReq;	/* followed by buffer ids */
#define sz_xMbufCreateImageBuffersReq	12

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	numberBuffer B16;	/* number successfully allocated */
    CARD16	unused1 B16;
    CARD32	unused2 B32;
    CARD32	unused3 B32;
    CARD32	unused4 B32;
    CARD32	unused5 B32;
    CARD32	unused6 B32;
} xMbufCreateImageBuffersReply;
#define sz_xMbufCreateImageBuffersReply 32

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufDestroyImageBuffers */
    CARD16	length B16;
    CARD32	window B32;	/* associated window */
} xMbufDestroyImageBuffersReq;
#define sz_xMbufDestroyImageBuffersReq	8

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufDisplayImageBuffers */
    CARD16	length B16;
    CARD16	minDelay B16;	/* minimum time between last update and now */
    CARD16	maxDelay B16;	/* maximum time between last update and now */
} xMbufDisplayImageBuffersReq;	/* followed by list of buffers */
#define sz_xMbufDisplayImageBuffersReq	8

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufSetMBufferAttributes */
    CARD16	length B16;
    CARD32	window B32;	/* associated window */
    CARD32	valueMask B32;	/* modified entries */
} xMbufSetMBufferAttributesReq;	/* followed by values */
#define sz_xMbufSetMBufferAttributesReq 12

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufGetMBufferAttributes */
    CARD16	length B16;
    CARD32	window B32;	/* associated window */
} xMbufGetMBufferAttributesReq;
#define sz_xMbufGetMBufferAttributesReq 8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	displayedBuffer B16;	/* currently visible buffer */
    CARD8	updateAction;
    CARD8	updateHint;
    CARD8	windowMode;
    CARD8	unused0;
    CARD16	unused1 B16;
    CARD32	unused2 B32;
    CARD32	unused3 B32;
    CARD32	unused4 B32;
    CARD32	unused5 B32;
} xMbufGetMBufferAttributesReply;
#define sz_xMbufGetMBufferAttributesReply 32

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufSetBufferAttributes */
    CARD16	length B16;
    CARD32	buffer B32;
    CARD32	valueMask B32;
} xMbufSetBufferAttributesReq;	/* followed by values */
#define sz_xMbufSetBufferAttributesReq 12

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufGetBufferAttributes */
    CARD16	length B16;
    CARD32	buffer B32;
} xMbufGetBufferAttributesReq;
#define sz_xMbufGetBufferAttributesReq 8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	window B32;
    CARD32	eventMask B32;
    CARD16	bufferIndex B16;
    CARD8	side;
    CARD8	unused0;
    CARD32	unused1 B32;
    CARD32	unused2 B32;
    CARD32	unused3 B32;
} xMbufGetBufferAttributesReply;
#define sz_xMbufGetBufferAttributesReply 32

typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufGetBufferInfo */
    CARD16	length B16;
    Drawable	drawable B32;
} xMbufGetBufferInfoReq;
#define sz_xMbufGetBufferInfoReq 8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	unused;			/* not used */
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	normalInfo B16;
    CARD16	stereoInfo B16;
    CARD32	unused1 B32;
    CARD32	unused2 B32;
    CARD32	unused3 B32;
    CARD32	unused4 B32;
    CARD32	unused5 B32;
} xMbufGetBufferInfoReply;			/* followed by buffer infos */
#define sz_xMbufGetBufferInfoReply 32


typedef struct {
    CARD8	reqType;	/* always codes->major_opcode */
    CARD8	mbufReqType;	/* always X_MbufCreateStereoWindow */
    CARD16	length B16;
    CARD8	unused0;
    CARD8	unused1;
    CARD8	unused2;
    CARD8	depth;
    Window	wid B32;
    Window	parent B32;
    Multibuffer	left B32;	/* associated buffers */
    Multibuffer	right B32;
    INT16	x B16;
    INT16	y B16;
    CARD16	width B16;
    CARD16	height B16;
    CARD16	borderWidth B16;  
    CARD16	class B16;
    VisualID	visual B32;
    CARD32	mask B32;
} xMbufCreateStereoWindowReq;		/* followed by value list */
#define sz_xMbufCreateStereoWindowReq 44


#undef Multibuffer

#endif /* _MULTIBUFST_H_ */
