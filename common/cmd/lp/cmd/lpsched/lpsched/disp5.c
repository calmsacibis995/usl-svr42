/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)lp:cmd/lpsched/lpsched/disp5.c	1.22.3.8"
#ident  "$Header: disp5.c 1.2 91/06/27 $"

#include "dispatch.h"
 
extern int		Net_fd;

extern MESG *		Net_md;

#define	WHO_AM_I	I_AM_LPSCHED
#include "oam.h"

/**
 ** s_child_done()
 **/

int
#ifdef	__STDC__
s_child_done (char *m, MESG *md)
#else
s_child_done (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_child_done)

	long			key;
	short			slot;
	short			status;
	short			err;


	(void) getmessage (m, S_CHILD_DONE, &key, &slot, &status, &err);

	if (slot < 0
		|| slot >= ET_Size
		|| Exec_Table[slot].key != key
		|| Exec_Table[slot].md != md)
	{
#ifdef	DEBUG
		if (debug & (DB_EXEC|DB_DONE))
		{
			execlog (
				"FAKE! slot %d pid ??? status %d err %d\n",
				slot, status, err);
		}
#endif
		return	0;
	}


#ifdef	DEBUG
	if (debug & (DB_EXEC|DB_DONE))
	{
		EXEC *ep = &Exec_Table[slot];

		execlog (
			"OKAY: slot %d pid %d status %d err %d\n",
			slot, ep->pid, status, err);
		execlog ("%e", ep);
	}
#endif
	/*
	 * Remove the message descriptor from the listen
	 * table, then forget about it; we don't want to
	 * accidently match this exec-slot to a future,
	 * unrelated child.
	 */
	DROP_MD (Exec_Table[slot].md) /* EMPTY */ ;
	Exec_Table[slot].md = 0;

	Exec_Table[slot].pid = -99;
	Exec_Table[slot].status = status;
	Exec_Table[slot].errno = err;
	DoneChildren++;

	if (Exec_Table[slot].type == EX_INTERF)
		CutEndJobAuditRec ((int) status, 1,
			Exec_Table[slot].ex.printer->request->secure->user,
			Exec_Table[slot].ex.printer->request->secure->req_id);
	else
	if (Exec_Table[slot].type == EX_SLOWF)
		CutEndJobAuditRec ((int) status, 2,
			Exec_Table[slot].ex.request->secure->user,
			Exec_Table[slot].ex.request->secure->req_id);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     r_new_child
 *
 * Restrictions:
 *               ioctl(2): None
 *               mconnect: None
 *               flvlfile(2): None
 *               mputm: None
 *               mdisconnect: None
*/

/* ARGSUSED0 */
#ifdef	__STDC__
int
r_new_child (char *m, MESG *md)
#else
int
r_new_child (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (r_new_child)

	int			were_waiting	= 0;
    	int			cred_size;
	char *			name;
	char *			originator_name;
	short			status;

	MESG *			new_md;
	MESG *			drop_md;
	MESG *			hold_md;
	SSTATUS *		pss;
	PSTATUS *		pps;

	struct strrecvfd	recvfd;
    	struct s_strrecvfd *	recvbufp;


	(void) getmessage (m, R_NEW_CHILD, &name, &originator_name, &status);
	schedlog (
		"Received R_NEW_CHILD for system %s requested by %s\n",
		name,
		originator_name
	);

	if (!(pss = search_stable(name)))
	{
		schedlog ("%s is an unknown system\n", name);
		recvfd.fd = -1;	/* So as not to clobber someone else */
		(void) ioctl (Net_fd, I_RECVFD, &recvfd);
		(void) close (recvfd.fd);
		return 0;
	}


	switch (status) {

	case MOK:
		break;

	case MUNKNOWN:
		/*
		 * The network manager doesn't know about this system.
		 * While strictly speaking this ought not occur, it can
		 * because we can't prevent someone from mucking with
		 * the system table. So if this happens we disable the
		 * printer(s) that go to this system.
		 */
		for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
			if (pps->system == pss)
				(void)disable (pps, CUZ_NOREMOTE, DISABLE_STOP);
		return 0;

	case MNOSTART:
		/*
		 * We get this only in response to our request for a
		 * connection. However, between the time we had asked
		 * and the receipt of this response we may have received
		 * another R_NEW_CHILD originated remotely. So, we try
		 * again later only if we still need to.
		 */
		if (!pss->exec->md) {
			schedlog (
				"Failed contact with %s, retry in %d min.\n",
				name,
				WHEN_NOSTART / MINUTE
			);
			resend_remote (pss, WHEN_NOSTART);
		}
		return 0;

	default:
		schedlog (
			"Strange status (%d) in R_NEW_CHILD for %s.\n",
			status,
			name
		);
		return 0;
	}
	/*
	**	Retrieve the file descriptor
	*/
	if ((cred_size = secadvise (0, SA_SUBSIZE, 0)) < 0)
	{
		/*
		**  Errno is already set.
		*/
		return	0;
	}
	recvbufp = (struct s_strrecvfd *)
        		Calloc(1, sizeof (struct s_strrecvfd) +
			cred_size - sizeof (struct sub_attr));

	if (!recvbufp)
	{
		errno = ENOMEM;
		return	0;
	}
	if (ioctl (Net_fd, I_S_RECVFD, recvbufp) < 0)
	{
		switch (errno) {
		case EBADMSG:
			schedlog ("No file descriptor passed.\n");
			break;

		case ENXIO:
			schedlog ("System server terminated early.\n");
			break;

		case EMFILE:
			schedlog ("Too many open files!\n");
			break;
		}
		Free (recvbufp);
		return	0;
	}

	new_md = mconnect (NULL, recvbufp->fd, recvbufp->fd);
	if (! new_md)
		mallocfail ();

	new_md->credp = (struct sub_attr *) Calloc (1, cred_size);

	if (!new_md->credp)
	{
	    	Free (recvbufp);
	    	Free (new_md);
		errno = ENOMEM;
		return	0;
	}
	new_md->gid = recvbufp->gid;
	new_md->uid = recvbufp->uid;
	if (flvlfile (new_md->readfd, MAC_GET, &(new_md->lid)) < 0)
	{
		if (errno != ENOPKG) {
			int	save = errno;
			Free (recvbufp);
			Free (new_md->credp);
			Free (new_md);
			errno = save;
			return	0;
		}
	}
	(void)	memcpy (new_md->credp, &(recvbufp->s_attrs), cred_size);

	Free (recvbufp);
	/*
	 * Save this flag, because in the hustle and bustle below
	 * we may lose the original information.
	 */
	were_waiting = (pss->exec->flags & EXF_WAITCHILD);


	/*
	 * Check for a collision with another system trying to contact us:
	 *
	 *	* We had asked for a connection to this system (i.e. had
	 *	  sent a S_NEW_CHILD message), but this isn't the response
	 *	  to that message.
	 *
	 *	* We already have a connection.
	 *
	 * These cases are handled separately below, but the same
	 * arbitration is used: The system with the name that comes
	 * ``first'' in collating order gets to keep the connection
	 * it originated.
	 */
	if (were_waiting)
	{
		if (STREQU(Local_System, originator_name))
		{
			/*
			 * This is the usual case.
			 */
			schedlog ("Making new connection to %s\n", name);
			hold_md = new_md;
			drop_md = 0;

		}
		else
		{
			/*
			 * We have a pending collision, since we
			 * are still waiting for a response to our
			 * connection request (this isn't it). Resolve
			 * the collision now, by either accepting
			 * this response (we'll have to refuse our
			 * real response later) or by refusing this
			 * response.
			 */
			schedlog (
				"Potential collision between %s and %s\n",
				Local_System,
				name);
			if (strcmp(Local_System, name) < 0)
			{
				schedlog ("Take no connection.\n");
				hold_md = 0;
				drop_md = new_md;
			}
			else
			{
				schedlog ("Drop this connection.\n");
				hold_md = new_md;
				drop_md = 0;
			}
		}

	}
	else 
	if (pss->exec->md) {
		MESG *			my_md;
		MESG *			his_md;

		schedlog (
			"Collision between %s and %s!\n",
			Local_System,
			name);

		/*
		 * The message descriptor we got last time
		 * MAY NOT be for the connection we originated.
		 * We have to check the "originator_name" to be sure.
		 */
		if (STREQU(Local_System, originator_name))
		{
			my_md = new_md;
			his_md = pss->exec->md;
		}
		else
		{
			my_md = pss->exec->md;
			his_md = new_md;
		}

		/*
		 * (First means < 0, right?)
		 */
		if (strcmp(Local_System, name) < 0)
		{
			schedlog ("I win!\n");
			drop_md = his_md;
			hold_md = my_md;
		}
		else
		{
			schedlog ("He wins.\n");
			drop_md = my_md;
			hold_md = his_md;
		}

	}
	else
	{
		schedlog ("Accepting unsolicited connection.\n");
		hold_md = new_md;
		drop_md = 0;
	}
	if (drop_md)
	{
		if (drop_md == pss->exec->md)
		{
			schedlog ("Dropping fd %d from listen table\n",
				drop_md->readfd);
			DROP_MD (drop_md) /* EMPTY */ ;

			/*
			 * We are probably waiting on a response
			 * to an S_SEND_JOB from the network child
			 * on the other end of the connection we
			 * just dropped. If so, we have to resend the
			 * job through the new channel...yes, we know
			 * we have a new channel, as the only way to
			 * get here is if we're dropping the exising
			 * channel, and we do that only if we have to
			 * pick between it and the new channel.
			 */
			if (pss->exec->flags & EXF_WAITJOB)
			{
				resend_remote (pss, -1);
				were_waiting = 1;
			}
		}
		else
		{
			schedlog (
				"Sending S_CHILD_SYNC M2LATE on %x\n",
				drop_md);
			drop_md->type = MD_CHILD;
			(void) mputm (drop_md, S_CHILD_SYNC, M2LATE);
			(void) mdisconnect (drop_md);
		}
	}
	if (hold_md)
	{
		if (hold_md != pss->exec->md)
		{
			schedlog (
				"Sending S_CHILD_SYNC MOK on %x\n",
				hold_md);
			hold_md->type = MD_CHILD;
			(void) mputm (hold_md, S_CHILD_SYNC, MOK);
			pss->exec->md = hold_md;
			if (mlistenadd(pss->exec->md, POLLIN) == -1)
				mallocfail ();
		}
		pss->exec->flags &= ~EXF_WAITCHILD;
	}

	/*
	 * If we still have a connection to the remote system,
	 * and we had been waiting for the connection, (re)send
	 * the job.
	 */
	if (pss->exec->md && were_waiting)
		schedule (EV_SYSTEM, pss);

	return	1;
}

/*
 * Procedure:     r_send_job
 *
 * Restrictions:
 *               putrequest: None
*/
  
/* ARGSUSED */
#ifdef	__STDC__
int
r_send_job (char *m, MESG *md)
#else
int
r_send_job (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (r_send_job)

	char			buf[MSGMAX];

	char *			name;
	char *			sent_msg;
	char *			req_id;
	char *			remote_name;
	char *			s1;
	char *			s2;
	char *			s3;
	char *			s4;
	char *			s5;

	short			status;
	short			sent_size;
	short			rank;
	short			h1;

	long			lstatus;
	long			l1;
	long			l2;
	level_t			lid;

	SSTATUS *		pss;

	RSTATUS *		prs;

	PSTATUS *		pps;

#ifdef	SEND_CANCEL_RESPONSE
	MESG *			user_md;
#endif


	(void)	getmessage (m, R_SEND_JOB, &name, &status,
		&sent_size, &sent_msg);
	schedlog ("Received R_SEND_JOB from system %s.\n", name);

	if (!(pss = search_stable(name)))
	{
		schedlog ("%s is an unknown system\n", name);
		return	0;
	}
	prs = pss->exec->ex.request;

	if (!(prs->request->outcome & RS_SENDING))
	{
		schedlog ("Unexpected R_SEND_JOB--no request sent!\n");
		return	0;
	}
	if (!(pss->exec->flags & EXF_WAITJOB))
	{
		schedlog ("Unexpected R_SEND_JOB--not waiting!\n");
		return	0;
	}
	switch (status) {
	case MOK:
	case MOKMORE:
		break;

	case MTRANSMITERR:
		schedlog ("Received MTRANSMITERR from %s, retrying.\n", name);
		resend_remote (pss, 0);
		return	0;

	default:
		schedlog ("Odd status in R_SEND_JOB, %d!\n", status);
		return	0;

	}
	switch (mtype(sent_msg)) {

	case R_PRINT_REQUEST:
		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;

		(void) getmessage (
			sent_msg,
			R_PRINT_REQUEST,
			&status,
			&req_id,
			&chkprinter_result
		);

		prs->request->outcome &= ~RS_SENDING;
		prs->printer->status &= ~PS_BUSY;

		if (status == MOK)
		{
			char	*reqno;
			char	*path;

			schedlog("S_SEND_JOB had succeeded\n");
			prs->request->outcome |= RS_SENT;
                        /*
                         * Adding print request to printer list, allow user
                         * to issue 'cancel <printer>'
                         */
			/*prs->printer->request = prs;*/
				/* Backing out change to allow user to issue
				 * 'cancel <printer>'
				 * This is not the correct location to add
				 * the request to the remote printer list.
				 * There may be an appropriate location and
				 * corresponding duration for the request to
				 * be on the printer, but determining that
				 * will wait for now. cjh on MR ul92-08611
				 */
			/*
			 * Record the fact that we've sent this job,
			 * to avoid sending it again if we restart.
			 */
			(void) putrequest (prs->req_file, prs->request);
			/*
			 * Remove the notify/error file to allow
			 * returning file from remote system
			 */
			if (prs->secure && prs->secure->req_id) {
				reqno = getreqno (prs->secure->req_id);
				path = makepath (Lp_Temp, reqno, (char *)0);
				(void) Unlink (path);
				Free(path);
			}
			if (pss->system->protocol == BSD_PROTO)
				schedule (EV_LATER, WHEN_POLLBSD,
					EV_POLLBSDSYSTEMS, pss);
		}
		else
		{
			schedlog ("S_SEND_JOB had failed, status was %d!\n",
				status);
			/*
			 * This is very much like what happens if the
			 * print service configuration changes and causes
			 * a local job to be no longer printable.
			 */
			prs->reason = status;
			(void) cancel (prs, 1);
		}
		break;

	case R_GET_STATUS:
		/*
		 * Were we expecting this?
		 */
		if (!(prs->status & RSS_RECVSTATUS))
		{
			schedlog ("Unexpected GET_STATUS from system: %s\n",
				pss->system->name);
			break;
		}
		if (status != MOKMORE)
			rmreq (prs);

		/*
		 * Is the protocol correct?
		 */
		if (pss->system->protocol == S5_PROTO) {
			schedlog(
			"Protocol mismatch: system %s, got BSD, expected S5\n",
				pss->system->name);
			break;
		}

		(void) getmessage (sent_msg, R_GET_STATUS, &status, &remote_name);

		for (pps = walk_ptable(1); pps; pps = walk_ptable(0)) {
			if (pps->system != pss)
				continue;
			if (STREQU(pps->remote_name, remote_name))
				break;
		}
		if (!pps) {
			schedlog (
			"Received GET_STATUS on unknown printer %s\n",
			remote_name);
			break;
		}

		load_bsd_stat (pss, pps);

		if (status != MOKMORE) {
			/*
		 	* Processed the last printer for pss,
		 	* so clear the waiting-for-R_SEND_JOB flag.
		 	*/
			pss->exec->flags &= ~EXF_WAITJOB;

			if (pss->tmp_pmlist) {
				if (pss->pmlist)
					freelist (pss->pmlist);
				pss->pmlist = pss->tmp_pmlist;
				pss->tmp_pmlist = NULL;
			}
			md_wakeup (pss);
	
		}

		break;

	case R_INQUIRE_PRINTER_STATUS:
		/*
		 * Expecting this?
		 */
		if (!(prs->status & RSS_RECVSTATUS))
		{
			schedlog (
			"Unexpected INQUIRE_PRINTER_STATUS from system %s\n",
				pss->system->name);
			break;
		}
		if (status != MOKMORE)
			rmreq (prs);

		/*
		 * Protocol ok?
		 */
		if (pss->system->protocol == BSD_PROTO)
		{
			schedlog (
			"Protocol mismatch:  system %s is BSD got S5\n",
			pss->system->name);
			break;
		}
		(void) getmessage (
			sent_msg,
			R_INQUIRE_PRINTER_STATUS,
			&status,
			&remote_name,
			&s1, &s2, &s3, &s4, &h1, &s5, &l1, &l2
		);
		for (pps = walk_ptable(1); pps; pps = walk_ptable(0))
		{
			if (pps->system != pss)
				continue;
			if (STREQU(pps->remote_name, remote_name))
				break;
		}
		if (!pps)
		{
		schedlog (
		"Received INQUIRE_PRINTER_STATUS from unknown printer %s\n",
			remote_name);
			break;
		}
		(void) putmessage (
			buf,
			R_INQUIRE_REMOTE_PRINTER,
			(status == MOK? MOKMORE : status),
			pps->printer->name,
			s1, s2, s3, s4, h1, s5, l1, l2
		);
		mesgadd (pss, buf);

		break;


	case R_INQUIRE_REQUEST_RANK:
		/*
		 * Expecting this?
		 */
		if (!(prs->status & RSS_RECVSTATUS)) {
			schedlog ("Unexpected INQUIRE_REQUEST_RANK from system %s\n",
				pss->system->name);
			break;
		}
		if (status != MOKMORE)
			rmreq (prs);

		/*
		 * Protocol ok?
		 */
		if (pss->system->protocol == BSD_PROTO) {
			schedlog ("Protocol mismatch: system %s is BSD got S5\n",
				pss->system->name);
			break;
		}

		/*
		 * If a list of new messages has been created
		 * free the old list (if any) and point to
		 * the new one.
		 */
		if (pss->tmp_pmlist) {
			if (pss->pmlist)
				freelist (pss->pmlist);
			pss->pmlist = pss->tmp_pmlist;
			pss->tmp_pmlist = 0;
		}

		(void) getmessage (
			sent_msg,
			R_INQUIRE_REQUEST_RANK,
			&status,
			&req_id,
			&s1, &l1, &l2, &h1, &s2, &s3, &s4,
			&rank,
			&lid
		);

		if (status == MOKMORE || status == MOK)
			update_req (req_id, rank);

		if (status != MOKMORE) {
			/*
			 * Last of status received from the system, so
			 * clear the waiting-for-R_SEND_JOB flag.
			 */
			pss->exec->flags &= ~EXF_WAITJOB;
			md_wakeup (pss);
		}

		break;

	case R_CANCEL:
		schedlog ("Received R_CANCEL\n");
		if (! (prs->request->outcome & RS_CANCELLED))
		{
			schedlog ("Unexpected R_CANCEL (not canceled)!\n");
			break;
		}
		(void) getmessage (sent_msg, R_CANCEL, &status,
			&lstatus, &req_id);

		if (!STREQU(prs->secure->req_id, req_id))
		{
			schedlog (
				"Out of sync on R_CANCEL: wanted %s, got %s\n",
				prs->secure->req_id, req_id);
			break;
		}
		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;
		prs->request->outcome &= ~RS_SENDING;
		/*
		**  For SVR4+ systems:
		**    The S_CANCEL that we sent will cause notification of
		**    the job completion to be sent back to us.
		**    s_job_completed() will called, and it will do the
		**    following:
		**	dowait_remote (EX_NOTIFY, prs, 0, (char *)0);
		**  HOWEVER:
		**    for BSD systems there is no notification and so
		**    we must do the call here.
		*/
		if (pss->system->protocol == BSD_PROTO)
		{
			notify (prs, (char *)0, 0, 0, 0);
			prs->printer->request = 0;
/*
			dowait_remote (EX_NOTIFY, prs, 0, (char *)0);
*/
		}
		break;

	case R_JOB_COMPLETED:
		if (!(prs->request->outcome & RS_DONE)) {
			schedlog ("Unexpected R_JOB_COMPLETED (not done)!\n");
			return 0;
		}

		schedlog ("Received R_JOB_COMPLETED, request %s\n", prs->secure->req_id);

		/*
		 * Clear the waiting-for-R_SEND_JOB flag now that
		 * we know this message isn't bogus.
		 */
		pss->exec->flags &= ~EXF_WAITJOB;

		(void) getmessage (sent_msg, R_JOB_COMPLETED, &status);
		if (status == MUNKNOWN)
  		     lpnote (INFO, E_SCH_REFREQ,
		        	name,
				prs->secure->req_id
			);

		prs->request->outcome &= ~RS_SENDING;
		dowait_remote (EX_NOTIFY, prs, 0, (char *)0);

		break;

	}

	/*
	 * There may be another request waiting to go out over
	 * the network.
	 */
	schedule (EV_SYSTEM, pss);

	return	status == MOK ? 1 : 0;
}

/*
 * Procedure:     s_job_completed
 *
 * Restrictions:
 *               mputm: None
*/

#ifdef	__STDC__
int
s_job_completed (char *m, MESG *md)
#else
int
s_job_completed (m, md)

char *	m;
MESG *	md;
#endif
{
	DEFINE_FNNAME (s_job_completed)

	char *		req_id;
	char *		errfile;
	short		outcome;
	RSTATUS *	prs;


	(void) getmessage (m, S_JOB_COMPLETED, &outcome, &req_id, &errfile);

	if (!(prs = request_by_id(req_id))) {
		schedlog ("Got S_JOB_COMPLETED for unknown request %s\n",
			req_id);
		(void) mputm (md, R_JOB_COMPLETED, MUNKNOWN);
		return	0;
	}

	(void) mputm (md, R_JOB_COMPLETED, MOK);
	dowait_remote (EX_INTERF, prs, outcome, errfile);

	return	1;
}
