#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)R5Xlib:util/mkks.sh	1.1"
#!/bin/sh

cat $* | awk 'BEGIN { \
    printf "/*\n * This file is generated from %s.  Do not edit.\n */\n", \
	   "$(INCLUDESRC)/keysymdef.h";\
} \
/^#define/ { \
	len = length($2)-3; \
	printf("{ \"%s\", %s },\n", substr($2,4,len), $3); \
}' 

