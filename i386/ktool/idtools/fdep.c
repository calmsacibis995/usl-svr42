/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)idtools:i386/ktool/idtools/fdep.c	1.4"
#ident	"$Header:"

#include "inst.h"
#include "devconf.h"
#include "defines.h"
#include <stdio.h>

/*
 * In a cross-environment, make sure this header is for the target system
 */
#include <sys/elf.h>

extern char *sfile;
extern unsigned short file_mode;
extern FILE *open1();

fdep_prsec(moddir, drv)
char *moddir;
driver_t *drv;
{
	FILE *fp;
	char file[512];
	char *pfx;
	struct depend_list *dep;

	sprintf(file, "%s/%s", moddir, sfile);
	fp = open1(file, "w", FULL);
	chmod(file, file_mode);
	pfx = drv->mdev.prefix;

	fprintf(fp, "\t.section\t.mod_dep,\"aw\",%d\n", SHT_MOD);
	fprintf(fp, "\t.globl\t%s_wrapper\n", pfx);
	fprintf(fp, "\t.long\t%s_wrapper\n", pfx);
	for (dep = drv->mdev.depends; dep != NULL; dep = dep->next)
		fprintf(fp, "\t.string\t\"%s\"\n", dep->name);
	fclose(fp);
}
