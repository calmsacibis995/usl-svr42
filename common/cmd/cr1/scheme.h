/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cr1:scheme.h	1.1.2.2"
#ident  "$Header: scheme.h 1.2 91/06/25 $"

/* cr1 scheme specific declarations */

#define FDIN 0		/* scheme's input file descriptor */
#define FDOUT 1		/* scheme's output file descriptor */

/* A nonce */

typedef struct {
	long time;	/* a timestamp */
	long pid;	/* the process ID */
} Nonce;

/* The protocol message type */

enum mtype {
	TYPE1=1,
	TYPE2=2,
	TYPE3=3
};

/* Logical protocol message */

typedef struct {
	enum mtype type;	/* The message type */
	Principal principal;	/* The identifier of the responder */
	Nonce nonce1;		/* The responder's nonce */
	Nonce nonce2;		/* The imposer's nonce */
	size_t size;		/* Number of data bytes */
	char data[256];		/* Arbitrary data */
} Pmessage;

/* Physical protocol message */

typedef struct {
	Key icv;
	unsigned int nbytes;
	char data[BUFSIZ];
	int sum_type;
	unsigned int sum;
} Emessage;

/* Declarations of XDR routines for cr1 protocol implementation */

extern bool_t
xdr_pmessage(XDR *xdrs, Pmessage *msg);
