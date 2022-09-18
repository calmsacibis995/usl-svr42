/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmstruct.h	1.4.7.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ttymon/tmstruct.h,v 1.1 91/02/28 20:16:33 ccs Exp $"

/*
 * /etc/ttydefs structure
 */
struct Gdef {
	char		*g_id;		/* id for modes & speeds 	*/
	char		*g_iflags;	/* initial terminal flags 	*/
	char		*g_fflags;	/* final terminal flags 	*/
	short		g_autobaud;	/* autobaud indicator 		*/
	char		*g_nextid;	/* next id if this speed is wrong */
};

/*
 *	pmtab structure + internal data for ttymon
 */
struct pmtab {
	/* the following fields are from pmtab			*/
	char	*p_tag;		/* port/service tag		*/
	long	p_flags;	/* flags			*/
	char	*p_identity;	/* id for service to run as	*/
	char	*p_res1;	/* reserved field		*/
	char	*p_res2;	/* reserved field		*/
	char	*p_iascheme;	/* identification & authentication secheme */
	char	*p_device;	/* full path name of device	*/
	long	p_ttyflags;	/* ttyflags			*/
	int	p_count;	/* wait_read count		*/
	char	*p_server;	/* full service cmd line	*/
	int	p_timeout;	/* timeout for input 		*/
	char	*p_ttylabel;	/* ttylabel in /etc/ttydefs	*/
	char	*p_modules;	/* modules to push		*/
	char	*p_prompt;	/* prompt message		*/
	char	*p_dmsg;	/* disable message		*/
	struct	sak p_sak;	/* sak definition		*/

	/* the following fields are for ttymon internal use	*/
	int	p_status;	/* status of entry 		*/
	int	p_fd;		/* fd for the open device	*/
	pid_t	p_pid;		/* pid of child on the device 	*/
	int 	p_inservice;	/* service invoked		*/
	int	p_respawn;	/* respawn count in this series */
	long	p_time;		/* start time of a series	*/
	uid_t	p_uid;		/* uid of p_identity		*/
	gid_t	p_gid;		/* gid of p_identity		*/
	char	*p_dir;		/* home dir of p_identity	*/
	int	p_tpstatus;	/* TP related status  		*/
	int	p_muxid;	/* TP multiplexor id  		*/
	int	p_tpdataconnid;	/* connection id for data chan	*/
	int	p_tpctrlfd;	/* TP ctrl channel fd  		*/
	char	*p_realdevice;	/* path name of physical device	*/
                                /*  multilpexed under TP	*/
	ulong	p_reason;	/* if p_status == CHANGED,  	*/
                                /*  p_reason indicates reason	*/
                                /*  for change			*/
	struct	pmtab	*p_next;
};

/*
 *	valid flags for p_flags field of pmtab
 */
#define	X_FLAG	0x1	/* port/service disabled 		*/
#define U_FLAG  0x2	/* create utmp entry for the service 	*/

/*
 *	valid flags for p_ttyflags field of pmtab
 */
#define C_FLAG	0x1	/* invoke service on carrier		*/
#define H_FLAG	0x2	/* hangup the line			*/
#define B_FLAG	0x4	/* bi-directional line			*/
#define R_FLAG	0x8	/* do wait_read				*/

/*
 *	autobaud enabled flag
 */
#define A_FLAG	0x10	/* autobaud flag			*/

/*
 *	values for p_status field of pmtab
 */
#define		NOTVALID	0	/* entry is not valid		*/
#define		VALID		1	/* entry is valid		*/
#define		CHANGED		2	/* entry is valid but changed 	*/
#define		GETTY		3	/* entry is for ttymon express	*/

/*
 *	value for p_tpstatus field of pmtab TP!!!
 */
#define tpNONTRUSTEDSTATE	0	/* indicates TP mechanism is not in a
                                        ** trusted state (waiting for SAK) and
                                        ** therefore not available for I&A
                                        */
#define tpTRUSTEDSTATE		1	/* indicates TP mechanism is in a
                                        ** trusted state for I&A
                                        */


/*	values for p_reason field of pmtab
*/
#define REASidentity	0x00000001
#define REASres1	0x00000002
#define REASres2	0x00000004
#define REASiascheme	0x00000008
#define REASrealdevice	0x00000010
#define REASserver	0x00000020
#define REASttylabel	0x00000040
#define REASmodules	0x00000080
#define REASprompt	0x00000100
#define REASdmsg	0x00000200
#define REASsakdef	0x00000400
#define REASsakundef	0x00000800
#define REASsakchg	0x00001000
#define REASflags	0x00002000
#define REASxflag	0x00004000
#define	REASadduflag	0x00008000
#define	REASdeluflag	0x00010000
#define REASttyflags	0x00020000
#define REAScount	0x00040000
#define REAStimeout	0x00080000
#define REASuid		0x00100000
#define REASgid		0x00200000
#define REASdir		0x00400000
#define REASdisabled	0x00800000
#define	REASspawnlimit	0x01000000



#define	ALLOC_PMTAB \
	((struct pmtab *)calloc((unsigned)1, \
		(unsigned)sizeof(struct pmtab)))

#define	PNULL	((struct pmtab *)NULL)




/* -pids are put on this table when end of login session is detected via the
**  ctrl channel (i.e. Hangup or SAK)
** 
** -sigchild() looks at this table if a pmtab entry can not be found for a pid
*/
struct pidtab{
	pid_t	p_pid;		/* p_pid from pmtab */
	long	p_flags;	/* p_flags from pmtab */
	struct	pidtab *p_next;
};



#define	ALLOC_PIDTAB \
	((struct pidtab *)calloc((unsigned)1, \
		(unsigned)sizeof(struct pidtab)))
