/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/extern.h	1.10.2.4"
#ident  "$Header: extern.h 1.2 91/06/26 $"

extern	int	check_device_privs(char *);
extern	int	tli_connect(void);
extern	void	update_cache_list(char *);
extern	int	blank(char *);
extern	int	comment(char *);
extern	int	checkscheme(char *, char *, char *);
extern	void	opendebug();
extern	void	debug();
extern  void	*setnetpath();
extern 	struct	netconfig *getnetpath();
extern	int	getscheme();
extern	int	get_alias();
extern	CALL 	*read_dialrequest();	
extern	int 	write_dialrequest();	
extern	int 	dev_stat();	
extern	int 	devalloc();	
extern	char	Scratch[];
extern	char	msg[];
extern 	int 	Debug;
extern 	int 	Verbose;
extern	CALL	Call;
extern	CALL	*Callp;
extern	int 	Debugging;
extern	int 	netfd;		/* fd into the network	*/
extern	int 	returnfd;	/* authenticated fd to return */
extern 	struct	nd_hostserv	Nd_hostserv;
extern 	struct	nd_hostserv	*Nd_hostservp;
extern	int	Pid;
