/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XcmsIdOfPr.c	1.2"

#include "Xlibint.h"
#include "Xcmsint.h"

/*
 *      EXTERNS
 */
extern XcmsColorSpace **_XcmsDIColorSpaces;
extern XcmsColorSpace **_XcmsDDColorSpaces;
void _XcmsCopyISOLatin1Lowered();


/*
 *	NAME
 *		XcmsFormatOfPrefix
 *
 *	SYNOPSIS
 */
XcmsColorFormat
XcmsFormatOfPrefix(prefix)
    char *prefix;
/*
 *	DESCRIPTION
 *		Returns the Color Space ID for the specified prefix
 *		if the color space is found in the Color Conversion
 *		Context.
 *
 *	RETURNS
 *		Color Space ID if found; zero otherwise.
 */
{
    XcmsColorSpace	**papColorSpaces;
    char		string_buf[64];
    char		*string_lowered;
    int			len;

    /*
     * While copying prefix to string_lowered, convert to lowercase
     */
    if ((len = strlen(prefix)) >= sizeof(string_buf)) {
	string_lowered = (char *) Xmalloc(len+1);
    } else {
	string_lowered = string_buf;
    }
    _XcmsCopyISOLatin1Lowered(string_lowered, prefix);

    /*
     * First try Device-Independent color spaces
     */
    papColorSpaces = _XcmsDIColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if (strcmp((*papColorSpaces)->prefix, string_lowered) == 0) {
		if (len >= sizeof(string_buf)) Xfree(string_lowered);
		return((*papColorSpaces)->id);
	    }
	    papColorSpaces++;
	}
    }

    /*
     * Next try Device-Dependent color spaces
     */
    papColorSpaces = _XcmsDDColorSpaces;
    if (papColorSpaces != NULL) {
	while (*papColorSpaces != NULL) {
	    if (strcmp((*papColorSpaces)->prefix, string_lowered) == 0) {
		if (len >= sizeof(string_buf)) Xfree(string_lowered);
		return((*papColorSpaces)->id);
	    }
	    papColorSpaces++;
	}
    }

    if (len >= sizeof(string_buf)) Xfree(string_lowered);
    return(XcmsUndefinedFormat);
}
