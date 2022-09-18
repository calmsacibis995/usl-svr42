/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _UTIL_MOD_MODULE_H	/* wrapper symbol for kernel use */
#define _UTIL_MOD_MODULE_H	/* subject to change without notice */

#ident	"@(#)uts-comm:util/mod/module.h	1.2"
#ident	"$Header: $"

/* used by unixsyms at user level */
/* basic record of internal information associated with a loaded module */

/* structure stubs so no coplaints if other headers not included */
struct modctl_list;
struct mod_wrapper;

struct module  {

	char * md_symspace;	/* space for symbol table info incl.
					symbol table, string table,
					hash chains and buckets */
	unsigned int md_symsize;		/* size of symspace */
	unsigned int md_symentsize;	/* size of a symbol table entry from 
					symbol table section header */
	unsigned int md_page;		/* is symspace pageable? */
	char * md_strings;		/* pointer into symspace for symbol table
					string table */
	unsigned long * md_buckets;	/* buckets for hash into symtbl */
	unsigned long * md_chains;	/* chains for hash into symtbl */
	char * md_space;		/* space where text, data, 
					and pre-allocated bss is loaded */
	unsigned int md_space_size;
	char * md_bss;		/* space where common symbols defined in module
					are loaded */
	unsigned int md_bss_size;
	char *md_path;		/* full path of loaded module */
	struct mod_wrapper *md_mw; /* pointer to mod_wrapper */
	struct modctl_list *md_mcl; /* list of modules on which this module is
					dependent */
};


#define MOD_OBJHASH	101		/* size of hash table */

#endif	/* _UTIL_MOD_MODULE_H */
