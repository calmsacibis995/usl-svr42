#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wksh:xksrc/listsyms.sh	1.1"

#	Copyright (c) 1990, 1991 AT&T and UNIX System Laboratories, Inc. 
#	All Rights Reserved     

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T    
#	and UNIX System Laboratories, Inc.			
#	The copyright notice above does not evidence any       
#	actual or intended publication of such source code.    
#
#
${NM:-nm} -ph $1 | grep " [A-TV-Z] " | cut -f3 -d" " 
