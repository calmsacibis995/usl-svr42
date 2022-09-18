/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash:i386/cmd/crash/stat.c	1.2"
#ident "$Header: stat.c 1.1 91/07/23 $"

/*
 * This file contains code for the crash function stat.
 */

#include <sys/param.h>
#include <a.out.h>
#include <stdio.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include <sys/utsname.h>
#include <sys/immu.h>
#include <time.h>
#include "crash.h"

static struct syment *Sys, *Time, *Lbolt;	/* namelist symbol */
extern struct syment *Panic;				/* pointers */

#define	DATE_FMT	"%a %b %e %H:%M:%S %Y\n"
/*	
 *	%a	abbreviated weekday name
 *	%b	abbreviated month name
 *	%e	day of month
 *	%H	hour
 *	%M	minute
 *	%S	second
 *	%Y	year
 */

static	char	time_buf[50];		/* holds date and time string */	

/* get arguments for stat function */
int
getstat()
{
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if(args[optind])
		longjmp(syn,0);
	else prstat(); 
}


/* print system statistics */
int
prstat()
{
	int toc, lbolt;
	int panicstr;
	char panicbuf[200];
	struct utsname utsbuf;

	/* 
	 * Locate, read, and print the system name, node name, release number,
	 * version number, and machine name.
	 */

	if(!Sys)
		if(!(Sys = symsrch("utsname")))
			error("utsname not found in symbol table\n");
	readmem((long)Sys->n_value,1,-1,(char *)&utsbuf,
		sizeof utsbuf,"utsname structure");

	fprintf(fp,"system name:\t%s\nrelease:\t%s\n",
		utsbuf.sysname,
		utsbuf.release);
	fprintf(fp,"node name:\t%s\nversion:\t%s\n",
		utsbuf.nodename,
		utsbuf.version);
	fprintf(fp,"machine name:\t%s\n", utsbuf.machine) ;
	/*
	 * Locate, read, and print the time of the crash.
	 */

	if(!Time)
		if(!(Time = symsrch("time")))
			error("time not found in symbol table\n");

	readmem((long)Time->n_value,1,-1,(char *)&toc,
		sizeof toc,"time of crash");
	cftime(time_buf, DATE_FMT, (long *)&toc);
	fprintf(fp,"time of crash:\t%s", time_buf);

	/*
	 * Locate, read, and print the age of the system since the last boot.
	 */

	if(!Lbolt)
		if(!(Lbolt = symsrch("lbolt")))
			error("lbolt not found in symbol table\n");

	readmem((long)Lbolt->n_value,1,-1,(char *)&lbolt,
		sizeof lbolt,"lbolt");

	fprintf(fp,"age of system:\t");
	lbolt = lbolt/(60*HZ);
	if(lbolt / (long)(60 * 24))
		fprintf(fp,"%d day, ", lbolt / (long)(60 * 24));
	lbolt %= (long)(60 * 24);
	if(lbolt / (long)60)
		fprintf(fp,"%d hr., ", lbolt / (long)60);
	lbolt %= (long) 60;
	if(lbolt)
		fprintf(fp,"%d min.", lbolt);
	fprintf(fp,"\n");

	/*
	 * Determine if a panic occured by examining the size of the panic string. If
	 * no panic occurred return to main(). If a panic did occur locate, read, and
	 *  print the panic registers. Note: in examining an on-line system, the panic
	 *  registers will always appear to be zero.
	 */

	fprintf(fp,"panicstr:\t");
	readmem((long)Panic->n_value,1,-1,(char *)&panicstr,sizeof panicstr,"panicstr");
	if (panicstr != 0) {
		readmem(panicstr,1,-1,panicbuf,sizeof panicbuf,"panicstr");
		panicbuf[sizeof panicbuf - 1] = '\0';
		fprintf(fp, "%s\n", panicbuf);
	} else fprintf(fp,"\n");
}
