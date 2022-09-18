/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_LOG_LOG_H	/* wrapper symbol for kernel use */
#define _IO_LOG_LOG_H	/* subject to change without notice */

#ident	"@(#)uts-x86:io/log/log.h	1.4"
#ident	"$Header: $"
/*
 * Header file for the Streams Log Driver
 */

#ifdef _KERNEL_HEADERS

#ifndef _IO_STREAM_H
#include <io/stream.h>	/* REQUIRED */
#endif

#elif defined(_KERNEL)

#include <sys/stream.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * Minor devices for the log driver.
 * 0 through 5 are reserved for well-known entry points.
 * Minors above CLONEMIN are given out on clone opens.
 */
#define CONSWMIN	0	/* minor device to write to console log */
#define CLONEMIN	5	/* minor device of clone interface */

struct log {
	unsigned log_state;
	queue_t *log_rdq;
	mblk_t	*log_tracemp;
};

/*
 * Driver state values.
 */
#define LOGOPEN 	0x01
#define LOGERR		0x02
#define LOGTRC		0x04
#define LOGCONS		0x08

/* 
 * Module information structure fields
 */
#define LOG_MID		44
#define LOG_NAME	"LOG"
#define LOG_MINPS	0
#define LOG_MAXPS	256
#define LOG_HIWAT	4096
#define LOG_LOWAT	256

/*
 * STRLOG(mid,sid,level,flags,fmt,args) should be used for those trace
 * calls that are only to be made during debugging.
 */
#if defined(DEBUG) || defined(lint)
#define STRLOG	strlog
#else
#define STRLOG	0 && strlog
#endif


/*
 * Utility macros for strlog.
 */

/*
 * logadjust - move a character pointer up to the next int boundary after
 * its current value.  Assumes sizeof(int) is 2**n bytes for some integer n. 
 */
#define logadjust(wp) (char *)(((unsigned)wp + sizeof(int)) & ~(sizeof(int)-1))

/*
 * logstrcpy(dp, sp) copies string sp to dp.
 */

/*
 * This is a catchall definition for those processors that have not had
 * this coded in assembler above.
 */
#define logstrcpy(dp, sp)  for (; (*dp = *sp) != 0; dp++, sp++)

 /* console logging attributes (see conslog_set()) */

#define CONSLOG_DIS 0	      /* disable console logging */
#define CONSLOG_ENA 1	      /* enable console logging */
#define CONSLOG_STAT 2	      /* console logging status */

#endif	/* _IO_LOG_LOG_H */
