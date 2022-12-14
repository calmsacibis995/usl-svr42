/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcgen:rpc_util.h	1.2.9.3"
#ident  "$Header: rpc_util.h 1.3 91/07/01 $"

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
*       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
*       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
*       (c) 1990,1991  UNIX System Laboratories, Inc
*          All rights reserved.
*/ 
/* rpc_util.h 1.15 89/02/22 (C) 1987 SMI */

/*
 * rpc_util.h, Useful definitions for the RPC protocol compiler 
 */

#include  <stdio.h>
  /*
   * This trick is used to distinguish between SYSV and V7 systems.
   * We assume that L_ctermid is only defined in stdio.h in SYSV
   * systems, but not in V7 or Berkeley UNIX.
   */

extern char *malloc();

#define alloc(size)		malloc((unsigned)(size))
#define ALLOC(object)   (object *) malloc(sizeof(object))

#define s_print	(void) sprintf
#define f_print (void) fprintf

struct list {
	char *val;
	struct list *next;
};
typedef struct list list;

/*
 * Global variables 
 */
#define MAXLINESIZE 1024
extern char curline[MAXLINESIZE];
extern char *where;
extern int linenum;

extern char *infilename;
extern FILE *fout;
extern FILE *fin;

extern list *defined;

/*
 * All the option flags
 */
extern int inetdflag;
extern int pmflag;   
extern int tblflag;
extern int logflag;

/*
 * Other flags related with inetd jumpstart.
 */
extern int indefinitewait;
extern int exitnow;
extern int timerflag;

extern int nonfatalerrors;

/*
 * rpc_util routines 
 */
void storeval();

#define STOREVAL(list,item)	\
	storeval(list,(char *)item)

char *findval();

#define FINDVAL(list,item,finder) \
	findval(list, (char *) item, finder)

char *fixtype();
char *stringfix();
char *locase();
void pvname();
void ptype();
int isvectordef();
int streq();
void error();
void expected1();
void expected2();
void expected3();
void tabify();
void record_open();

/*
 * rpc_cout routines 
 */
void cprint();
void emit();

/*
 * rpc_hout routines 
 */
void print_datadef();

/*
 * rpc_svcout routines 
 */
void write_most();
void write_register();
void write_rest();
void write_programs();
void write_svc_aux();
void write_inetd_register();
void write_netid_register();
void write_nettype_register();
/*
 * rpc_clntout routines
 */
void write_stubs();

/*
 * rpc_tblout routines
 */
void write_tables();
