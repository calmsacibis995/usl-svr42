/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/network.c	1.4"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include <Xol/OpenLook.h>
#include <Xol/OlCursors.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "inet.h"
#include "error.h"
#include <sys/secsys.h>


extern void		NotifyUser();

extern PopupGizmo LocalPopup;

/* this routine returns TRUE when the network is available */
Boolean
IsTCP_OK()
{
	extern	errno;
	int	fd, old_errno;

	old_errno = errno;
	if ((fd = open("/dev/tcp", O_RDONLY, 0)) >= 0);
		(void) close(fd);
	errno = old_errno;
	return (fd >= 0);
} /* IsTCP_OK */

static void 
LockCursor()
{
    Widget w = hf->localPopup;

    if (!XtIsRealized(w))
	return;
    XDefineCursor(XtDisplay(w), XtWindow(w), GetOlBusyCursor(XtScreen(w)));
    SetValue(w, XtNbusy, True);
    XSync(XtDisplay(w), False);
    return;
}	


static void 
UnlockCursor()
{
    Widget w = hf->localPopup;

    if (!XtIsRealized(w))
	return;
    XDefineCursor(XtDisplay(w), XtWindow(w), GetOlStandardCursor(XtScreen(w)));
    SetValue(w, XtNbusy, False);
    return;
}

InitNetwork(wid)
Widget wid;
{

	char 	cmdline[BUFSIZ], *bp;
	char	exitp[128];
	char	stderr_file[PATH_MAX] = "/var/tmp/network";
	int  	x, i;
	int	sfd;
	FILE	*stderr_p;
	struct stat	statbuf;
	Boolean	did_setuid;
	uid_t	orig_uid, _loc_id_priv;

	/* Since some network initialization still requires
	 * using the root adminstrator uid, we are going to
	 * setuid() for this portion of code.
	 * If we luck out to be root from the start,
	 * make note not to do the restore by leaving
	 * did_setuid = FALSE.
	 * In an SUM (Super-User-Mode) system, we will
	 * be able to to setuid(non-root) without losing
	 * privilege since we assume we are aquiring
	 * privilege for this process via tfadmin, and
	 * therefore the privs are aquired via fixed
	 * privilege.
	 *
	 * Since privilege is required by the init scripts,
	 * we'll have to handle the error case of not being
	 * able to do the setuid().
	 */

	/* get root administrator user & current user id,
	 * censure -1's
	 */
	orig_uid = getuid();
	_loc_id_priv = secsys(ES_PRVID, 0);

	if ((-1 == orig_uid) || (-1 == _loc_id_priv)) {
		/* what besides?! ********/
		return;
	}
	if ((_loc_id_priv >= 0) &&
	    (orig_uid == _loc_id_priv))
		did_setuid = FALSE;
	else {
		if ((setuid(_loc_id_priv)) < 0) {
			/* what besides?! ********/
			return;
		}
		did_setuid = TRUE;
	}

	(void)umask(0022);

	/* create path name to the stderr file */

#ifdef later
	stderr_p = tmpfile();
#else
	stderr_p = fopen(stderr_file, "a+");
#endif

	/* replace stderr with stderr_file */
	if ( stderr_p  == (FILE *)NULL ) {
		fprintf(stderr, "cannot open stderr_p file");
	} else {
		sfd = dup(2);
		(void)close(2);
		(void)dup(fileno(stderr_p));
		(void)fclose(stderr_p);
	}

	/* form the listener command line */

	sprintf( cmdline,
		"sh /etc/inet/listen.setup 1>&2"
		);
	/* put out a message to the user; the scripts may take a long time */
	SetPopupMessage(&LocalPopup, GGT(string_setupNet));
	OlUpdateDisplay(LocalPopup.message);
	LockCursor();
	i = system(cmdline);
	switch((i >> 8) && 0xff) {

	case 0:
		if (IsTCP_OK() == False) {
			UnlockCursor();
			rexit(10, GGT(string_noTCP), "");
		}
		x = sprintf( cmdline,
			"sh /etc/inet/rc.restart 1>&2"
			);
		/* put out a message to the user; the scripts may take a long time */
		SetPopupMessage(&LocalPopup, GGT(string_startNet));
		/* need to simulate XtMainloop here */
		OlUpdateDisplay(LocalPopup.message);
		i = system(cmdline);
		switch((i >> 8) && 0xff) {
		case 0:
			PUTMSG(GGT(string_networkInit));
			break;
		case 1:
		case 2:
			UnlockCursor();
			rexit (19, GGT(string_listenerFail), "");
			break;
		case 3:
		case 4:
			UnlockCursor();
			NotifyUser(wid, GGT(string_uucpFail));
			break;
		default:
			UnlockCursor();
			(void)fprintf(stderr,"default in InitNetwork taken!!!\n");
			break;
		}
		
		break;
	case 1:
	case 2:
		UnlockCursor();
		rexit (19, GGT(string_listenerFail), "");
		break;
	case 3:
	case 4:
		UnlockCursor();
		NotifyUser(wid, GGT(string_uucpFail));
		break;
	default:
		UnlockCursor();
		(void)fprintf(stderr,"default in InitNetwork taken!!!\n");
		break;
	}
	/* put back the stderr as before */
	(void)close(2);
	(void)dup(sfd);
	(void)close(sfd);
	XtPopup(hf->toplevel, XtGrabNone);

	if (did_setuid)
		if ((setuid(orig_uid)) < 0) {
			/* what besides?! ********/
			return;
		}
} /* InitNetwork */
