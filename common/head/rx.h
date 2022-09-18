/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _RX_H
#define _RX_H

#ident	"@(#)head.usr:rx.h	1.1.5.5"
#ident  "$Header: rx.h 1.3 91/06/21 $"

/*
 * rexec client program interface
 *
 */

#if defined(__STDC__)

extern int rexecve(char *, char *, char **, char **, long);
extern int rx_proc_msg(int, long *, long *);
extern int rx_write(int, char *, long);
extern int rx_signal(int, int);
extern int rx_ack_exit(int, char *, long);
extern int rx_set_ioctl_hand(int, int (*) (int, int, ...));
extern int rx_set_write_hand(int, ssize_t (*) (int, const void *, size_t));
extern int rx_fd(int);
extern int rx_free_conn(int);

#else

extern int rexecve();
extern int rx_proc_msg();
extern int rx_write();
extern int rx_signal();
extern int rx_ack_exit();
extern int rx_set_ioctl_hand();
extern int rx_set_write_hand();
extern int rx_fd();
extern int rx_free_conn();

#endif


/*
 * rexec flags
 *
 */

#define	RXF_SEPERR	0001	/* Separate stderr from stdout */
#define	RXF_STDINPIPE	0002	/* Standard input is redirected from a pipe/file */
#define	RXF_STDOUTTERM	0004	/* Standard output is going to a terminal */
#define	RXF_DEBUG	0100	/* enable debug mode on server side */

/*
 * all the flags together
 *
 */

#define	RXF_ALL		(RXF_SEPERR | RXF_STDINPIPE | RXF_STDOUTTERM | RXF_DEBUG)


/*
 * message type codes returned in msg_type parameter to rx_proc_msg()
 *
 */

#define	RX_INCOMPLETE	1	/* incomplete message */
#define	RX_PROTOCOL	2	/* protocol mesage (open, close, etc) */
#define	RX_SERVICE_DEAD	3	/* service termination message */
#define	RX_TYPEAHEAD	4	/* typeahead message */
#define	RX_DATA		5	/* data message */
#define	RX_IOCTL	6	/* ioctl message */
#define	RX_EOF		7	/* 0-length message */


/*
 * various rexec constants
 *
 */

#define	RX_SVCNAME	"listen:rexec"
#define	RX_LOGFILE	"/var/adm/log/rexec.log"
#define	RX_MODULEID	"rexec"

#define	RX_MAXRXCONN	5	/* maximum number of open rexec client connections */
#define	RX_MAXSVCLINE	1024	/* maximum service entry line size */
#define	RX_MAXSVCSZ	14	/* maximum service name size */
#define	RX_MAXSVCDESCR	256	/* maximum service description */
#define	RX_MAXSVCDEF	256	/* maximum service definition */
#define	RX_MAXUTMP	1	/* maximum utmp flag size */
#define	RX_MAXMSGSZ	5120	/* maximum rx message size */
#define	RX_MAXARGSZ	4096	/* maximum argument string size */
#define	RX_MAXENVSZ	4096	/* maximum environment string size */
#define	RX_MAXTASZ	1024	/* maximum typeahead buffer size */
#define	RX_MAXDATASZ	1024	/* maximum data buffer size */
#define	RX_MAXIOCARGSZ	1024	/* maximum ioctl argument buffer size */
#define	RX_MAXENVFNAME	256	/* maximum environment file name */
#define	RX_MAXARGS	64	/* maximum number of arguments to service */
#define	RX_MAXENVS	128	/* maximum number of environment variables */
#define	RX_WRITEWAIT	1	/* seconds to wait in case of RXE_AGAIN */


/*
 * rexec error numbers
 *
 */

#define	RXE_OK		0	/* no error */
#define	RXE_2MANYRX	1	/* too many open client rexec connections */
#define	RXE_BADFLAGS	2	/* bad options/flags specified */
#define	RXE_BADARGS	3	/* too many arguments */
#define	RXE_BADENV	4	/* bad environment specified */
#define	RXE_BADMACH	5	/* unknown host */
#define	RXE_CONNPROB	6	/* connection problem */
#define	RXE_NORXSERVER	7	/* host is not running rxserver */
#define	RXE_BADVERSION	8	/* unsupported version */
#define	RXE_NOSVCFILE	9	/* could not open services file */
#define	RXE_NOSVC	10	/* no such service */
#define	RXE_NOTAUTH	11	/* not authorized to execute service */
#define	RXE_NOPTS	12	/* no pseudo terminals available */
#define	RXE_PIPE	13	/* cannot make pipe for stderr */
#define	RXE_BADSTART	14	/* error in starting server side */
#define	RXE_NOSPACE	15	/* server side memory allocation problems */
#define	RXE_BADCNUM	16	/* bad rexec connection number */
#define	RXE_AGAIN	17	/* write would cause server to block, try later */
#define	RXE_BADSIG	18	/* bad signal number */
#define	RXE_BADSTATE	19	/* conn. is in wrong state to perform operation */
#define	RXE_TIRDWR	20	/* could not push module "tirdwr" at client */
#define	RXE_WRITE	21	/* write handler failure at client */
#define	RXE_IOCTL	22	/* ioctl handler failure at client */
#define	RXE_PROTOCOL	23	/* protocol failure - unexpected message */
#define	RXE_UNKNOWN	99	/* unknown error code */


/*
 * dflag should be defined in the same file as main()
 *
 */

#define	Printf0(format)		if (Dflag) { (void) printf(format); }
#define	Printf1(format,x)	if (Dflag) { (void) printf(format,x); }
#define	Printf2(format,x,y)	if (Dflag) { (void) printf(format,x,y); }
#define	Printf3(format,x,y,z)	if (Dflag) { (void) printf(format,x,y,z); }

#endif /* _RX_H */
