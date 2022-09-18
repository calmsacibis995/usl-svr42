/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5extensions:include/xteststr.h	1.1"

#define X_XTestGetVersion	0
#define X_XTestCompareCursor	1
#define X_XTestFakeInput	2

#define XTestNumberEvents	0

#define XTestNumberErrors	0

#define XTEST_MAJOR	1
#define XTEST_MINOR	0

#define XTestCurrentCursor ((Cursor)1)

#define XTESTNAME "XTEST"

typedef struct {
    CARD8	reqType;	/* always XTestReqCode */
    CARD8	xtReqType;	/* always X_XTestGetVersion */
    CARD16	length B16;
    CARD8	majorVersion;
    CARD8	pad;
    CARD16	minorVersion B16;
} xXTestGetVersionReq;
#define sz_xXTestGetVersionReq 8

typedef struct {
    BYTE	type;			/* X_Reply */
    CARD8	majorVersion;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD16	minorVersion B16;
    CARD16	pad0 B16;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xXTestGetVersionReply;
#define sz_xXTestGetVersionReply 32

typedef struct {
    CARD8	reqType;	/* always XTestReqCode */
    CARD8	xtReqType;	/* always X_XTestCompareCursor */
    CARD16	length B16;
    Window	window B32;
    Cursor	cursor B32;
} xXTestCompareCursorReq;
#define sz_xXTestCompareCursorReq 12

typedef struct {
    BYTE	type;			/* X_Reply */
    BOOL	same;
    CARD16	sequenceNumber B16;
    CARD32	length B32;
    CARD32	pad0 B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
    CARD32	pad5 B32;
} xXTestCompareCursorReply;
#define sz_xXTestCompareCursorReply 32

/* used only on the client side */
typedef struct {
    CARD8	reqType;	/* always XTestReqCode */
    CARD8	xtReqType;	/* always X_XTestFakeInput */
    CARD16	length B16;
    BYTE	type;
    BYTE	detail;
    CARD16	pad0 B16;
    Time	time B32;
    Window	root B32;
    CARD32	pad1 B32;
    CARD32	pad2 B32;
    INT16	rootX B16, rootY B16;
    CARD32	pad3 B32;
    CARD32	pad4 B32;
} xXTestFakeInputReq;
#define sz_xXTestFakeInputReq 36
