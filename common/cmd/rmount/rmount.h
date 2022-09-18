/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef	  _RMOUNT_H
#define   _RMOUNT_H

#ident	"@(#)rmount:rmount.h	1.1.1.2"
#ident	"@(#)rmount:rmount.h	1.1.1.2"

extern void	lock();	
extern void	unlock();	
extern int	wr_rmnttab();
extern int	rd_rmnttab();
extern int	ismac();

#define	Strcmp	(void)strcmp
#define	Strcat	(void)strcat
#define	Strcpy	(void)strcpy
#define Chown	(void)chown
#define Chmod	(void)chmod
#define Link	(void)link
#define Unlink	(void)unlink
#define Printf	(void)printf
#define Fprintf	(void)fprintf
#define Close	(void)close
#define Fclose	(void)fclose
#define Fcntl	(void)fcntl
#define Signal	(void)signal
#define Fputc	(void)fputc
#define Rename	(void)rename

#define MNTTAB		"/etc/mnttab"
#define TMPRMNT		"/etc/rfs/tmprmnt"
#define TMP		"/etc/rfs/tmp"
#define RSEM_FILE	"/etc/rfs/.rmnt.lock"

#define	TIMEOUT	180 /* time limit (seconds) for mount to complete */
#define MASK	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)

#endif /* _RMOUNT_H */
