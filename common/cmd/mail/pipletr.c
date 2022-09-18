/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pipletr.c	1.14.2.3"
#ident "@(#)pipletr.c	2.26 'attmail mail(1) command'"
#include "mail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
# include <sys/secsys.h>
#endif
/*
    NAME
	pipeletter - send letter to a pipe

    SYNOPSIS
	void pipeletter(Msg *pmsg, int whereexec, int nowait4process)

    DESCRIPTION
	Execute the command being held for the given surrogate.
	Return the exit code of the surrogate process in pmsg->surg_rc.
*/

void pipeletter(pmsg, whereexec, nowait4process)
Msg		*pmsg;
int		whereexec;
int		nowait4process;
{
    static const char pn[] = "pipeletter";
    char	**argvec;
    pid_t	pid;
    int		i;

    Dout(pn, 0, "whereexec=%d, nowait4process=%d\n", whereexec, nowait4process);
    s_delete(pmsg->SURRcmd);
    /*
     * fork/exec the surrogate. Note that it is BY DESIGN that the
     * surrogates are invoked with the gid of mail. This is so that
     * they can check, if they care, that it is rmail(1) that called
     * them and not some arbitrary user.
     */
    if ((argvec = msetup_exec (pmsg, whereexec)) == (char **)NULL)
	{
	pmsg->surg_rc = SURG_RC_ERR;
	return;
	}

    pmsg->SURRcmd = s_copy("     :");
    for (i= 0; argvec[i] != (char *)NULL; i++)
	pmsg->SURRcmd = s_xappend(pmsg->SURRcmd, argvec[i], " ", (char*)0);
    Dout(pn, 1,"arg vec to exec =\n");
    if (debug > 0)
	for (i= 0; argvec[i] != (char *)NULL; i++)
	    fprintf(dbgfp,"\targvec[%d] = '%s'\n", i, argvec[i]);

    if (pmsg->SURRinput.lettmp == (char*)NULL)
	{
	mktmp(&pmsg->SURRinput);
	if (!mcopylet(pmsg, pmsg->SURRinput.tmpf, REMOTE))
	    {
	    Dout(pn, 0, "can't write tmp file. errno = %d\n", errno);
	    errmsg(E_TMP,"");
	    pmsg->surg_rc = SURG_RC_ERR;
	    goto ret;
	    }
	fclose(pmsg->SURRinput.tmpf);
	}
    pmsg->SURRinput.tmpf = doopen(pmsg->SURRinput.lettmp, "r", E_TMP);

    if (pmsg->SURRerrfile != (FILE*)NULL)
	fclose(pmsg->SURRerrfile);
    if (pmsg->SURRoutfile != (FILE*)NULL)
	fclose(pmsg->SURRoutfile);
    pmsg->SURRerrfile = pmsg->SURRoutfile = 0;

    if ((pmsg->SURRerrfile = tmpfile()) == (FILE *)NULL)
	{
	Dout(pn, 0, "no tmp file. errno = %d\n", errno);
	pmsg->surg_rc = SURG_RC_ERR;
	goto ret;
	}

    if ((pmsg->SURRoutfile = tmpfile()) == (FILE *)NULL)
	{
	Dout(pn, 0, "no tmp file. errno = %d\n", errno);
	pmsg->surg_rc = SURG_RC_ERR;
	goto ret;
	}

    if ((pid = loopfork()) < 0)
	{
	Dout(pn, 0, "cannot fork, errno = %d\n", errno);
	errmsg(E_FORK, "");
	pmsg->surg_rc = SURG_RC_FORK;
	goto ret;
	}

    if (pid == CHILD)
	{
	extern char **environ;

	/* If we're not supposed to wait, fork() again and run as a grandchild. */
	/* The child will immediately exit. */
	if ((nowait4process > 0) && (loopfork() > 0))
	    _exit(0);

	for (i = SIGHUP; i < SIGTERM; i++)
	    setsig(i, exit);

	/* redirect stdin from temp file containing message */
	(void) close(0); (void) dup(fileno(pmsg->SURRinput.tmpf));

	/*
	 * Redirect stdout & stderr output of surrogate command to file
	 * in case needed later...
	 */
	(void) close(1); (void) dup(fileno(pmsg->SURRoutfile));
	(void) close(2); (void) dup(fileno(pmsg->SURRerrfile));

	/* close all unnecessary file descriptors */
	closeallfiles(3);

	environ = altenviron;
#ifdef SVR4_1
	/* Don't permit any of our privileges (if any) */
	/* to be passed on to child processes. */
	(void) procprivl(CLRPRV, pm_max(P_ALLPRIVS), (priv_t)0);
#endif /* SVR4_1 */
	execvp(*argvec, argvec);
	Dout(pn, 0, "execvp failed. errno = %d\n", errno);
	_exit(255);
	}

    /* parent */
    pmsg->surg_rc = dowait(pid);

ret:
    fclose(pmsg->SURRinput.tmpf);
    pmsg->SURRinput.tmpf = 0;
    return;
}
