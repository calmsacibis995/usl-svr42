/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Msgtable_h
#define _Msgtable_h
#ident	"@(#)debugger:inc/common/Msgtable.h	1.1"

#include "Signature.h"

enum Msg_class {
	MSGCL_invalid,
	MSGCL_error,
	MSGCL_info,
	MSGCL_help,
};

/* msgtab holds the information needed to translate a call to printm or printe
 * into a message.  This includes the parameter signature and the
 * printf-style format string.
 * catindex holds the message number for gettxt()
 * Not all of the entries must be retrieved from the catalog
 * since some of the format strings don't have to be translated,
 * tabular output, for example, needs only the headings translated
 * NOTE- - this file is included by a C file and should not have
 * C++isms
 */

struct	Msgtab
{
	enum Msg_class	mclass;		/* type of message - error, info, etc. */
	enum Signature	signature;	/* specifies types of parameters*/
	int		catindex;	/* index of the translated message */
	const char	*format;	/* default, untranslated message */
	const char	*iformat;	/* cached translated message */
};

#endif /* _Msgtable_h */
