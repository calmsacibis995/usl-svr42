/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)libDtI:HyperText.h	1.4"
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
**      File:   HyperText.h
**      Date:   08/06/90
**
****************************************************************/

#ifndef HYPERTEXT_H
#define HYPERTEXT_H
#ident "@(#)libDtI:HyperText.h	1.1 8/20/91"

/***********************************************************************
 Name                Class              RepType     Default Value
 ----                -----              -------     -------------
 file                File               String      NULL
 string              String             String      NULL
 sourceType          sourceType         int         OL_STRING_SOURCRE
					       (other: OL_DISK_SOURCE)

 font                Font               FontStruct  fixed
 keyColor            Foreground         Pixel       red

 leftMargin          Margin             Dimension   2
 rightMargin         Margin             Dimension   2
 topMargin           Margin             Dimension   2
 bottomMargin        Margin             Dimension   2

 recomputeSize       RecomputeSize      Boolean     TRUE
 maxLines            MaxSize            Dimension   0

 select              Callback           Pointer     NULL
		     (HyperSegment * call_data;)

**************************************************************************/

#define XtNkeyColor           "keyColor"
#define XtNmaxLines           "maxLines"
#define XtCMaxSize            "MaxSize"

#ifndef XtRDimension
/* for 6386 */
#define XtRDimension          "Int"
#endif

#define HtNewString(str)	((str) ? strdup(str) : NULL)

extern WidgetClass hyperTextWidgetClass;
typedef struct _HyperTextClassRec *HyperTextWidgetClass;
typedef struct _HyperTextRec *HyperTextWidget;

typedef struct hyper_segment HyperSegment;
struct hyper_segment {
	char *key;		/* non-NULL: hyper text key, NULL: not */
	char *script;       	/* reference string */
	char *text;
	unsigned short len;	/* length of text */
	Position x, y;		/* position of segment */
	Position y_text;	/* the y position for text baseline */
	Dimension w, h;
	unsigned short tabs;	/* number of tabs to be skipped first */
	Pixel color;
	Boolean use_color;	/* FALSE: default color, TRUE: color field */
	Boolean reverse_video;
	HyperSegment *next;
	HyperSegment *prev;
};

/******* global functions **************************************/

extern void   HyperTextAppendLine(/* hw, str */);
extern char * HyperSegmentKey(/* hs */);
extern char * HyperSegmentScript(/* hs */);
extern char * HyperSegmentText(/* hs */);
extern Boolean HyperSegmentRV(/* hs */);  /* reverse vedio or not */

/******* unsupported functions **********************************/

extern HyperSegment * HyperTextFindSegmentByXY(/* hw, x1, y1 */);
extern HyperSegment * HyperTextFindSegmentByKey(/* hw, key */);
extern void           HyperTextChangeSegment(/* hw, hs, new_key, new_text */);
extern void           HyperTextSetSegmentColor(/* hw, hs, Pixel color */);
extern void           HyperTextUnsetSegmentColor(/* hw, hs */);
extern void           HyperTextHighlightSegment(/* hw, hs */);
extern void           HyperTextUnhighlightSegment(/* hw */);
extern HyperSegment * HyperTextGetHighlightedSegment(/* hw */);
extern void           HyperTextSetSegmentRV(/* hw, hs, flag */);
extern void           HyperTextScanSegments(/* hw, func, client */);
			    /* func(hw, hs, client) */

extern void           HyperSegmentDump(/* hs */); /* for debugging */


#endif /* HYPERTEXT_H */
