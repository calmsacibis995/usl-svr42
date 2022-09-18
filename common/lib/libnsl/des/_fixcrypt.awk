
#ident	"@(#)libnsl:common/lib/libnsl/des/_fixcrypt.awk	1.1.5.1"
#ident  "$Header: _fixcrypt.awk 1.2 91/06/26 $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
#! /bin/sh
#
# @(#)_fixcrypt.awk 1.1 87/07/07 (C) 1987 SMI
#
# Convert the first ".data" line to a ".text" line.

awk '$0 ~ /^[ 	]*\.data$/ { if ( FIRST == 0 ) { FIRST = 1 ; print "\t.text" } else { print $0 } }
$0 !~ /^[ 	]*\.data$/ { print $0 }'
