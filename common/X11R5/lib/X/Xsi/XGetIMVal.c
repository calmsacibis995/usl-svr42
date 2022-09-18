/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XGetIMVal.c	1.2"

#include "Xlibint.h"
#include "Xi18nint.h"
#include "XIMlibint.h"
#if NeedVarargsPrototypes
# include <stdarg.h>
# define Va_start(a,b) va_start(a,b)
#else
# include <varargs.h>
# define Va_start(a,b) va_start(a)
#endif

/*
 * Query Input Method.
 */
static char *
_IMGetValues(im, args, pmask)
    register XipIM im;
    register XIMArg *args;
    unsigned long *pmask;
{
    register XIMArg		*arg;
    register int		i;
    register XrmName		Name;
    register XIMrmResourceList	xrmres;
    unsigned int		num_resources = im->num_resources;
    unsigned long		mask = 0L;
    XrmQuark			query_input =
				     XrmPermStringToQuark(XNQueryInputStyle);

    for (arg = args; arg->name && *(arg->name); arg++) {
	Name = XrmStringToName(arg->name);
	for (xrmres = im->resources, i = 0;
	     i < num_resources; i++, xrmres++) {
	    if (Name == xrmres->xrm_name) {
		if (!pmask) {
		    if (Name == query_input) {
			char *p;
			XIMStyles *styles;
			int size = sizeof(XIMStyle)
				   * im->values.input_styles.count_styles;
			int all_size = sizeof(XIMStyles) + size;
			if ((p = Xmalloc((unsigned)all_size)) == NULL) {
			    continue;
			}
			styles = (XIMStyles *)p;
			styles->count_styles =
					im->values.input_styles.count_styles;
			styles->supported_styles =
					(XIMStyle *)(p + sizeof(XIMStyles));
			bcopy((char *)im->values.input_styles.supported_styles,
			      (char *)styles->supported_styles, size);
			bcopy((char *)&styles, (char *)arg->value,
			      sizeof(XIMStyles *));
		    } else {
			(void) _XCopyToArg((char *)im - xrmres->xrm_offset - 1,
					   &arg->value,
					   (unsigned int)xrmres->xrm_size);
		    }
		}
		mask |= (1L << (xrmres->mask));
		break;
	    }
	}
	if (i >= num_resources) return(arg->name);
    }
    if (pmask)
	*pmask = mask;
    return NULL;
}

static Status
_ReceiveIMValues(im, mask)
    register XipIM im;
    unsigned long mask;
{
    register char	*p = NULL;
    ximGetIMReq		req;
    ximGetIMReply	reply;
    XIMStyle		*supported_styles = NULL;
    char		dummy_buf[512];

    req.reqType = XIM_GetIM;
    req.length = sz_ximGetIMReq;
    req.mask = mask;
    if ((_XipWriteToIM(im, (char *)&req, sz_ximGetIMReq) < 0) ||
	(_XipFlushToIM(im) < 0)) {
	return(-1);
    }

    if (_XipReadFromIM(im, (char *)&reply, sz_ximGetIMReply) < 0) {
	return(-1);
    }
    if (reply.state != 0) {
	return(-1);
    }
    if (reply.num_styles > 0) {
	if ((supported_styles =
	     (XIMStyle *)Xmalloc((unsigned)(sizeof(XIMStyle) *
					    reply.num_styles))) == NULL){
	    if (_XipReadFromIM(im, dummy_buf, sizeof(XIMStyle) * reply.num_styles)
		< 0) {
	    }
	    return(-1);
	}
	if (_XipReadFromIM(im, (char *)supported_styles,
			   sizeof(XIMStyle) * reply.num_styles) < 0) {
	    return(-1);
	}
    }
    if (reply.nbytes > 0) {
	if ((p = Xmalloc((unsigned)(reply.nbytes + 1))) == NULL) {
	    if (_XipReadFromIM(im, dummy_buf, reply.nbytes) < 0) {
	    }
	    return(-1);
	}
	if (_XipReadFromIM(im, p, reply.nbytes) < 0) {
	    return(-1);
	}
	p[reply.nbytes] = '\0';
    }
    if (mask & (1L << IMQueryInputStyle)) {
	if ((im->values.input_styles.count_styles > 0)
	    && (im->values.input_styles.supported_styles)) {
	    Xfree(im->values.input_styles.supported_styles);
	}
	im->values.input_styles.supported_styles = supported_styles;
	im->values.input_styles.count_styles = reply.num_styles;
    }
#ifdef	XML
    if ((mask & (1L << IMQueryLanguage)) && (p != NULL)) {
	if (im->values.supported_language) {
	    Xfree(im->values.supported_language);
	}
	im->values.supported_language = p;
    }
#else	/* XML */
    if (reply.nbytes > 0) Xfree(p);
#endif	/* XML */
    return 0;
}
    

char *
_XipGetIMValues(supim, args)
    XIM supim;
    XIMArg *args;
{
    XipIM im = (XipIM)supim;
    unsigned long mask;
    char *err;

    if (im->fd < 0) return(NULL);

    err = _IMGetValues(im, args, &mask);
    if (err) return err;

    if (!_ReceiveIMValues(im, mask)) {
	return(_IMGetValues(im, args, (unsigned long *)NULL));
    } else if (args) {
	return(args->name);
    } else {
	return((char *)"SomethingErrorOccured");
    }
}
