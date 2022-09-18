/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:HyperTextP.h	1.6"
#endif
/***************************************************************
**
**      Copyright (c) 1990
**      AT&T Bell Laboratories (Department 51241)
**      All Rights Reserved
**
**      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
**      AT&T BELL LABORATORIES
**
**      The copyright notice above does not evidence any
**      actual or intended publication of source code.
**
**      Author: Hai-Chen Tu
**      File:   HyperTextP.h
**      Date:   08/06/90
**
****************************************************************/

#ifndef HYPERTEXTP_H
#define HYPERTEXTP_H
#ident "@(#)libDtI:HyperTextP.h	1.1 8/20/91"

#include <Xol/PrimitiveP.h>
#include "HyperText.h"

/************************************
 *
 *  Class structure
 *
 ***********************************/

/* New fields for the HyperText widget class record */
typedef struct _HyperTextClass {
    int empty;                  /* not used */
} HyperTextClassPart;


/* Full class record declaration */
typedef struct _HyperTextClassRec {
    CoreClassPart       core_class;
    PrimitiveClassPart  primitive_class;
    HyperTextClassPart  hyper_text_class;
} HyperTextClassRec;

extern HyperTextClassRec hyperTextClassRec;

/***************************************
 *
 *  Local structures
 *
 **************************************/
typedef struct hyper_line HyperLine;

struct hyper_line {
    HyperSegment * first_segment;
    HyperSegment * last_segment;
    HyperLine *    next;
    HyperLine *    prev;
    };

/*********** functions defined in HyperText0.h *********************/

extern HyperLine *  HyperLineFromFile(/* widget, file_name */);
extern HyperLine *  HyperLineFromString(/* widget, str */);
extern HyperLine *  HyperLineAppendStr(/* widget, hl, str */);
extern void         HyperLineFree(/* widget, hl */);

/***************************************
 *
 *  Instance (widget) structure
 *
 **************************************/

/* New fields for the HyperText widget record */
typedef struct {
    char           *string;
    char           *file;
    int            source_type;
    Pixel          key_color;
    GC             gc;
    XFontStruct    *font;
    XtCallbackList callbacks;
    Dimension      left_margin;
    Dimension      top_margin;
    Dimension      right_margin;
    Dimension      bot_margin;
    Dimension      max_lines;
    Boolean        resizable;

    /* private */
    char           *src_copy;
    unsigned char  dyn_flags;
    HyperLine      *first_line;
    HyperLine      *last_line;
    Dimension      w0, h0;      /* max width and height required  */
    Dimension      line_height;
    Dimension      tab_width;
    HyperSegment   *highlight;
    Boolean        window_gravity_set;
    Boolean        is_busy;
} HyperTextPart;


/* Full widget declaration */
typedef struct _HyperTextRec {
    CorePart      core;
    PrimitivePart primitive;
    HyperTextPart hyper_text;
} HyperTextRec;

/* dynamic resource bit masks */
#define OL_B_HYPERTEXT_BG		(1<<0)
#define OL_B_HYPERTEXT_FONTCOLOR	(1<<1)
#define OL_B_HYPERTEXT_FONT		(1<<2)
#endif /* HYPERTEXTP_H  */
