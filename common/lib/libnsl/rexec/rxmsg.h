/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libnsl:common/lib/libnsl/rexec/rxmsg.h	1.1.4.2"
#ident  "$Header: rxmsg.h 1.2 91/06/26 $"


/* types of messages which flow between rexec client and server */

#define	RXM_OPEN_REQ	1	/* open request message */
#define	RXM_OPEN_ARGS	2	/* message containing service arguments */
#define	RXM_OPEN_ENVF	3	/* message containing environment file */
#define	RXM_OPEN_ENV	4	/* message containing user environment */
#define	RXM_OPEN_DONE	5	/* message indicating end of open messages */
#define	RXM_OPEN_REPLY	6	/* open reply message */
#define	RXM_CLOSE_REQ	7	/* close request message */
#define	RXM_CLOSE_REPLY	8	/* close reply message */
#define	RXM_CLOSE_TA	9	/* close typeahead message */
#define	RXM_DATA	10	/* message containing data */
#define	RXM_WRITEACK	11	/* write acknowledgement message */
#define	RXM_SIGNAL	12	/* message containing a signal */
#define	RXM_SIGNALACK	13	/* signal acknowledgement message */
#define	RXM_IOCTL	14	/* message containing an ioctl */


/* rexec message header */

struct rx_msg_head {
	long	msg_type;	/* type of rx message */
	long	msg_len;	/* length of rx message */
};


/* message structures */


/* service opening protocol messages */

/* RXM_OPEN_REQ */

struct open_req {
	long	version;		/* rexec client version */
	char	service[RX_MAXSVCSZ];	/* service to start */
	long	flags;			/* open options */
};

#define	RX_VERSION	1


/* RXM_OPEN_ARGS */

struct open_args {
	char	argstr[RX_MAXARGSZ];	/* argument string */
};

#define	RX_OPEN_ARGS_SZ(argv_sz) (argv_sz)


/* RXM_ENVF */

struct open_envf {
	char	envfile[RX_MAXENVFNAME];/* environment file name */
};


/* RXM_OPEN_ENV */

struct open_env {
	char	envstr[RX_MAXENVSZ];	/* environment string */
};

#define	RX_OPEN_ENV_SZ(envp_sz)	(envp_sz)


/* RXM_OPEN_REPLY */

struct open_reply {
	long	version;	/* rxserver version */
	long	ret_code;	/* return code for open operation */
	long	credit;		/* initial write credit */
};

/* server will only buffer 1 data message worth of data */

#define	RX_INITCREDIT	1


/* service closing protocol messages */

/* RXM_CLOSE_REQ */

struct close_req {
	long	ret_code;	/* dying process' return code */
	long	tasize;		/* amount of unused typeahead at server */
};


/* RXM_CLOSE_REPLY */

struct close_reply {
	long	tasize;		/* amount of typeahead to return */
};


/* RXM_CLOSE_TA */

struct close_ta {
	long	tasize;			/* returned typeahead size */
	char	tabuf[RX_MAXTASZ];	/* returned typeahead buffer */
};

#define	RX_CLOSE_TA_SZ(tabuf_sz) (sizeof(struct close_ta) - RX_MAXTASZ + tabuf_sz)


/* data and data acknowledgement messages */

/* RXM_DATA */

struct data_msg {
	long	fd;			/* orig / dest fd */
	long	len;			/* len of data */
	char	buf[RX_MAXDATASZ];	/* data */
};

#define	RX_DATA_MSG_SZ(buf_sz)	(sizeof(struct data_msg) - RX_MAXDATASZ + buf_sz)


/* RXM_WRITE_ACK */

struct writeack_msg {
	long	credit;	/* acknowledgement flag */
};


/* signal and signal acknowledgement messages */

/* RXM_SIGNAL */

struct signal_msg {
	long	sig;		/* signal number */
};


/* RXM_SIGNALACK */

struct signalack_msg {
	long	sig;		/* signal number */
};


/* ioctl message */

/* RXM_IOCTL */

struct ioctl_msg {
	long	fd;			/* destination fd */
	long	ioc;			/* ioctl number */
	long	arglen;			/* ioctl argument length */
	char	arg[RX_MAXIOCARGSZ];	/* ioctl argument buffer */
};

#define	RX_IOCTL_MSG_SZ(arg_sz)	(sizeof(struct ioctl_msg) - RX_MAXIOCARGSZ + arg_sz)


/* client/server states */

#define	RXS_OPENING	1
#define	RXS_OPEN	2
#define	RXS_CLOSING	3
#define	RXS_CLOSED	4
