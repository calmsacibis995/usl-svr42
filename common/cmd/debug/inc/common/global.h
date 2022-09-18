/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	global_h
#define	global_h
#ident	"@(#)debugger:inc/common/global.h	1.5.1.1"

// Declare global variables which almost every module needs.

#include "List.h"
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <sys/procset.h>


#ifdef DEBUG
extern int	debugflag;	// global debugging flag
#endif

extern int	interrupt;	
extern int	pathage;	
extern List	waitlist;

extern long	pagesize;

// signal sets: U=SIGUSR1, P=SIGPOLL, I=SIGINT
extern  sigset_t	sset_UPI, sset_P, sset_PI, sset_UP;

extern const char	*follow_path;
extern char		*prog_name;
extern FILE		*log_file;
extern char		*original_dir;	// original working directory

// bits in interrupt long word, use with <signal.h>
inline	int	sigbit(int sig) { return 1L<<(sig-1); }

extern int	quitflag;	// leave debugger

// variables used internally for support of debugger
// "%" variables
extern char	*global_path;
extern int	cmd_result;	// result status of previous command
extern int	last_error;	// last error message issued
extern int	vmode; 		// verbosity
extern int	wait_4_proc;	// background or foreground exec
extern int	follow_mode;	// follow children, threads, all, none
extern int	redir_io;	// should process I/O be redirected?
extern int	synch_mode;	// global sync/asynch flag
extern int	num_line;	// number of lines for list cmd
extern int	num_bytes;	// number of bytes for dump cmd

#endif	/* global_h */
