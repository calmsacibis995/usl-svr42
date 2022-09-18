/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef STR_H
#define STR_H
#ident	"@(#)debugger:inc/common/str.h	1.1"

// The s* functions manage a table of unique character strings,
// looking up the incoming strings and not making a copy if
// the string already exists.  The saved strings may be compared for
// equality by comparing the pointers.

// str() looks up the string and makes a copy if not found
// strlook() looks up the string, but does not save a copy if it
//      isn't there
// strn() is the same as str(), but it only uses the first n bytes
//      of the string
// sf() is equivalent to str( sprintf( ... ))

extern char *str( const char * );
extern char *strlook( const char * );
extern char *strn( const char *, int );
extern char *sf( const char *, ... );

// The make* functions just save a copy without doing the lookup.
// makestr() saves a copy of the string
// makesf is equivalent to makestr( sprintf( ... ))

extern char *makestr( const char * );
extern char *makesf( const char *, ... );

#endif /* STR_H */
