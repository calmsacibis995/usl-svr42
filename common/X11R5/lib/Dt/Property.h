/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)Dt:Property.h	1.3"
#endif

#ifndef __Property_h
#define __Property_h

/* attributes */
/* system reserved bits */
#define DT_PROP_ATTR_SYS	(0xff)
#define DT_PROP_ATTR_MENU	(1 << 0)
#define DT_PROP_ATTR_DONTCOPY	(1 << 1)
#define DT_PROP_ATTR_INSTANCE	(1 << 2)

typedef struct {
	DtAttrs		attrs;		/* attributes */
	char 		*name;		/* name */
	char		*value;		/* value */
} DtPropRec, *DtPropPtr;

typedef struct {
	DtPropPtr	ptr;		/* ptr to list */
	unsigned int	count;		/* # of entries in list */
} DtPropList, *DtPropListPtr;

#endif /* __Property_h */
