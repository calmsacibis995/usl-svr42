/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:dt_prop.c	1.7" */

#include <libgen.h>
#include <X11/Intrinsic.h>
#include "Dtm.h"

char *
DmGetDTProperty(name, attrs)
char *name;
DtAttrs *attrs;
{
	return(DtGetProperty(&DESKTOP_PROPS(Desktop), name, attrs));
}

/* this stores the op of the object being dropped */
static DmObjectPtr src_op = NULL;
static DmWinPtr    src_wp = NULL;

static int drop_op = 0;
static int expand_source = 0;

void
DmSetSrcObject(op)
DmObjectPtr op;
{
	src_op = op;
}

void
DmSetSrcWindow(wp)
DmWinPtr wp;
{
	src_wp = wp;
}

static char *
Dm__ObjProp(name, client_data, expand_proc)
char *name;
XtPointer client_data;
char *(*expand_proc)();
{
	DmObjectPtr op = (DmObjectPtr)client_data;
	char *p;

	if (name[0] && (name[1] == '\0')) {
		/* one char variable */
		switch(*name) {
		case 'F':
			/* no need to expand this further */
			if (drop_op && expand_source)
				return(strdup(DmObjPath(src_op)));
			else
				return(strdup(DmObjPath(op)));
			break;
		case 'f':
			/* no need to expand this further */
			if (drop_op && expand_source)
				return(strdup(src_op->name));
			else
				return(strdup(op->name));
			break;
		default:
			return(NULL);
		}
	}
	else {
		if (!(p = DmGetObjProperty(op, name, NULL)))
			p = DmGetDTProperty(name, NULL);
	}

	if (p) {
		int save_expand_source = 0;

		if (expand_source)
			save_expand_source = expand_source;
		expand_source = 0;
		p = Dm__expand_sh(p, expand_proc, client_data);
		if (save_expand_source)
			expand_source = save_expand_source;
		return(p);
	}
	else
		return(NULL);
}

char *
DmObjProp(name, client_data)
char *name;
XtPointer client_data;
{
	return(Dm__ObjProp(name, client_data, DmObjProp));
}

char *
DmDropObjProp(name, client_data)
char *name;
XtPointer client_data;
{
	int save_drop_op;
	int save_expand_source;
	char *p;

	if ((*name == 's') || (*name == 'S')) {
		/* takes care of all source object related variables */

		if (name[1] == '\0') {
			/* just the source name */
			if (*name == 's') {
				if (src_wp)
					return(strdup(src_op->name));
				else
					return(strdup(basename((char*)src_op)));
			}
			else {
				if (src_wp)
					return(strdup(DmObjPath(src_op)));
				else
					return(strdup((char *)src_op));
			}
		}

		if (src_wp && (name[1] == '.')) {
			char *p;

			/* skip "s." */
			name++;
			name++;

			/* reference a property of the source object */
			if (p = DmGetObjProperty(src_op, name, NULL)) {
				save_drop_op = drop_op;
				save_expand_source = expand_source;
				drop_op = 1;
				expand_source = 1;
				p = Dm__expand_sh(p, DmDropObjProp,
					(XtPointer)client_data);
				drop_op = save_drop_op;
				expand_source = save_expand_source;
				return(p);
			}
			else
				return(NULL);
		}
	}

	save_drop_op = drop_op;
	drop_op = 1;
	p = Dm__ObjProp(name, client_data, DmDropObjProp);
	drop_op = save_drop_op;
	return(p);
}

char *
DmDTProp(name, client_data)
char *name;
XtPointer client_data;
{
	return(Dm__expand_sh(DmGetDTProperty(name,NULL),DmDTProp,client_data));
}

