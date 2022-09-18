/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)dtadmin:internet/expand.c	1.17"
#endif

#include <Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>			/* need this for XtNtitle */
#include <Xol/OpenLook.h>
#include <sys/stat.h>
#include "inet.h"
#include "error.h"

extern char *	ApplicationName;

extern void	SaveOld();
extern void	DiscardOld();
extern void	DeleteLineBuffer();
extern void	AppendOrReplaceList();
extern void	HelpCB();

static void	AppendCB();
static void	ReplaceCB();
static void	CancelCB();
static void	CopyRemoteHostsTable();

static MenuItems appendItems[] = {
    {True,	label_save,	mnemonic_save,	NULL,	SaveOld},
    {True,	label_discard,	mnemonic_discard,NULL,	DiscardOld},
    {True,	label_cancel,	mnemonic_cancel,NULL,	CancelCB},
    {NULL,	NULL,		(XtArgVal)NULL,	NULL,	NULL}
};

MenuGizmo Append = {
	NULL,			/* help		*/
	"append",		/* name		*/
	"Append",		/* title	*/
	appendItems,		/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	2			/* default Item	*/
};

static ModalGizmo AppendNotice = {
	NULL,
	"append",
	title_appendWarn,
	&Append,
	string_itemNotSave,
};

static HelpText AppHelp = {
    title_expand, HELP_FILE, help_expand,
};
static MenuItems expandItems[] = {
    {True,	label_append,	mnemonic_append,	NULL,	AppendCB},
    {True,	label_replace,	mnemonic_replace,	NULL,	ReplaceCB},
    {True,	label_cancel,	mnemonic_cancel,	NULL,	CancelCB},
    {True,	label_help,	mnemonic_help,	NULL,	HelpCB, (char *)&AppHelp },
    {NULL,	NULL,		NULL,		NULL,	NULL}
};

MenuGizmo Expand = {
	NULL,			/* help		*/
	"expand",		/* name		*/
	"Expand",		/* title	*/
	expandItems,		/* items	*/
	0,			/* function	*/
	NULL,			/* client_data	*/
	CMD,			/* buttonType	*/
	OL_FIXEDROWS,		/* layoutType	*/
	1,			/* measure	*/
	2			/* default Item	*/
};

static ModalGizmo ExpandNotice = {
	NULL,
	"expand",
	title_expand,
	&Expand,
	string_expandWarn,
};

void
CreateExpandPopup (rhost)
String rhost;
{
	SET_HELP(AppHelp);
	hf->expandPrompt = CopyGizmo (
		ModalGizmoClass,
		&ExpandNotice
	);
	hf->expandPopup = CreateGizmo (
		hf->toplevel,
		ModalGizmoClass,
		hf->expandPrompt,
		0, 0
	);
} /* CreateExpandPopup */

void
ExpandHostsCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	HostData	*dp;

	/* Nothing to operate on, just return */
	if (hf->numFlatItems == 0) { /* nothing to operate */
		return;
	}

	dp = hf->flatItems[hf->currentItem].pField;
	/* check to see if the local machine has been selected */
	if (strcmp(dp->f_name, hf->nodeName) == 0) {
		CLRMSG();
		PUTMSG(GGT(string_noExpand));
		return;
	}

	if (hf->expandPrompt == (ModalGizmo *)0) {
		CreateExpandPopup (dp->f_name);
	}
	MapGizmo (ModalGizmoClass, hf->expandPrompt);
} /* ExpandHostsCB */

static void
HandleAppendWarning()
{
	if (hf->appendPrompt == (ModalGizmo *)0) {
		hf->appendPrompt = CopyGizmo (
			ModalGizmoClass,
			&AppendNotice
		);
		hf->appendPopup = CreateGizmo (
			hf->toplevel,
			ModalGizmoClass,
			hf->appendPrompt,
			0, 0
		);
	}
	MapGizmo (ModalGizmoClass, hf->appendPrompt);
} /* HandleAppendWarning */

void
AppendOrReplaceList(append_or_replace)
Boolean append_or_replace;
{
	CopyRemoteHostsTable(append_or_replace);
	DeleteFlatItems();
	DeleteLineBuffer();
	GetFlatItems(hf->filename);
	GetPermission();
	PutFlatItems(hf->filename);
	XtVaSetValues (
		hf->scrollingList,
		XtNitems,		hf->flatItems,
		XtNnumItems,		hf->numFlatItems,
		XtNviewHeight,          VIEWHEIGHT,
		XtNitemsTouched,	True,
		(String)0
	);
	if (hf->numFlatItems) {
#ifdef debug
		PrintFlatItems();
#endif
		UnselectSelect ();
	}
	hf->changesMade = False;
} /* AppendOrReplaceList */

static void
AppendCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (hf->expandPopup);
	if (hf->changesMade) {
		HandleAppendWarning();
		return;
	}
	AppendOrReplaceList(APPEND);
} /* AppendCB */

static void
ReplaceCB(wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
	BringDownPopup (hf->expandPopup);
	AppendOrReplaceList(REPLACE);
} /* ReplaceCB */

static void
CancelCB (wid, client_data, call_data)
Widget wid;
XtPointer client_data;
XtPointer call_data;
{
        XtPopdown ((Widget)_OlGetShellOfWidget (wid));
} /* CancelCB */

static void
CopyRemoteHostsTable(replace_or_append)
int	replace_or_append;
{

	HostData	*dp = hf->flatItems[hf->currentItem].pField;
	char 	cmdline[BUFSIZ], *bp;
	char	exitp[128];
	char	stderr_file[PATH_MAX] = "/var/tmp/copyhosts";
	int  	x, i;
	int	sfd;
	FILE	*stderr_p;
	char	*tmpfile;
	struct stat	statbuf;

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

	/* create tmp file to hold the database */
	tmpfile = tempnam("/usr/tmp", NULL);
	if ( tmpfile  == NULL ) {
		fprintf(stderr, "cannot create temporary file name");
		tmpfile = strdup("/var/tmp/hosts");
	}

	(void)fprintf(stderr, "**** START TO COPY /ETC/HOSTS TABLE ****\n");

	/* form the rsh command line */

	sprintf( cmdline,
		"/usr/bin/rsh -l %s %s \"cat %s\" > %s",
		hf->userName, dp->f_name, system_path, tmpfile);
	i = system(cmdline);
	if (i != 0) {

		/* the commmand failed on the local system */

#ifdef debug
		(void)sprintf(exitp,
			"command (%s) failed on local system: exit code (%d)",
			cmdline, ((i>>8) && 0xff));
		fprintf(stdout, "%s\n", exitp);
#endif
		PUTMSG(GGT(string_cp1Fail));

		/* put back the stderr as before */
		(void)close(2);
		(void)dup(sfd);
		(void)close(sfd);
		remove(tmpfile);
		return;
	} else { /* rsh succeed; be careful, it doesn't mean everything is cool */
		if (stat(tmpfile, &statbuf) == -1) {
			fprintf(stdout, "Can't stat %s\n", tmpfile);
			PUTMSG(GGT(string_cp2Fail));
			/* put back the stderr as before */
			(void)close(2);
			(void)dup(sfd);
			(void)close(sfd);
			remove(tmpfile);
			return;
		}
		if (statbuf.st_size <= 0) {
			PUTMSG(GGT(string_cp2Fail));
			/* put back the stderr as before */
			(void)close(2);
			(void)dup(sfd);
			(void)close(sfd);
			remove(tmpfile);
			return;
		}
			
		x = sprintf( cmdline,
			"/usr/bin/cat %s", tmpfile);
		bp = &cmdline[x];

		if(replace_or_append == REPLACE)
			(void)sprintf(bp, " > %s", hf->filename);
		else
			(void)sprintf(bp, " >> %s", hf->filename);
		i = system(cmdline);
		if (i != 0) {
#ifdef debug
			(void)sprintf(exitp,
				"command (%s) failed on local system: exit code (%d)",
				cmdline, ((i>>8) && 0xff));
			fprintf(stdout, "%s\n", exitp);
#endif
			PUTMSG(GGT(string_cp1Fail));

		} else { /* succeed;  */
#ifdef debug
			sprintf(exitp,
				"Hosts Table from (%s) has been copied to (%s)",
				dp->f_name, hf->filename);
#endif
			PUTMSG(GGT(string_cpOK));

		}
		/* put back the stderr as before */
		(void)close(2);
		(void)dup(sfd);
		(void)close(sfd);
		remove(tmpfile);
	}
} /* CopyRemoteHostsTable */

int
GetExitValue(fin,token)
FILE	*fin;
char	*token;
{
	char	buf[BUFSIZ];
	char	*namep;
	int	exit_val;
	int	not_found;
	int	save_pos;
	int	len;

	exit_val  = 0;
	not_found = 0;
	len       = strlen(token);

	save_pos = fseek(fin, 0L, SEEK_CUR);
	rewind(fin);
	while(fgets(buf, sizeof buf, fin) != NULL) {

		if ( (namep = strstr(buf, token))
				!= (char *)NULL &&
				isdigit((int)(*(namep + len))) ) {
			exit_val = atoi(namep + len);
			not_found = 1;
		}

		if (not_found == 0)
			continue;
		else
			break;
	}
	fseek(fin, save_pos, SEEK_SET);
	return(exit_val);
} /* GetExitValue */
