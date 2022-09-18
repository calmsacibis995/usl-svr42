/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/mod/modksym.cf/Stubs.c	1.4"
#ident	"$Header: $"

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/immu.h>
#include <sys/mod_obj.h>
#include <sys/mod_k.h>

struct module *mod_obj_kern = 0;
size_t mod_obj_size = 0;
int mod_obj_pagesymtab = MOD_NOPAGE;

void modinit(const char *n) {}
boolean_t mod_obj_validaddr(unsigned long value) {

	if(value >= (unsigned long) stext && value < (unsigned long) sdata)
		return(B_TRUE);
	return(B_FALSE);
}
unsigned long mod_obj_getsymvalue(const char *n, boolean_t k, boolean_t p) { return(0); }
char *mod_obj_getsymname(unsigned long v, unsigned long *o, boolean_t p, char *r) { return(0); }
int getksym() { return(ENOSYS); }
int mod_obj_ioksym(const char *k, char *b, size_t l, boolean_t i, int rw) {
	return(ENOMATCH);
}
