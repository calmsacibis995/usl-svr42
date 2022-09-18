/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olam:file_stuff.c	1.8"
#endif

/*
** file_stuff.c - This file contains the routines for manipulating the files
** that are being administered.
*/


#include <ctype.h>
#include <errno.h>
#ifndef MEMUTIL
#include <malloc.h>
#else
#include <X11/memutil.h>
#endif
#include <stdio.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include "errors.h"

#include <sys/stat.h>

#include "config.h"
#include "file_stuff.h"
#include "util.h"


static void	CheckMode();
static void	DoStat();
static void	GetDirname();


/*
** Checks to see if the file represented by `statbuf' is readable and/or
** writable by the user represented by `uid' and `gid'.  If `uid' == 0 (the
** user is root), the file is considered readable and writable; otherwise,
** the user, group, and other permissions are looked at in order.  If the
** file has the same `uid' or `gid' the permission for user or group
** respecively are used and no further checking is done; this is consistent
** with UNIX.
** The bits corresponding to FILE_READABLE and FILE_WRITEABLE are set (or
** unset) in `stat_flags'.  None of the other bits in `stat_flags' are
** disturbed.
*/
static void
CheckMode(statbuf, uid, gid, stat_flags)
  struct stat		*statbuf;
  unsigned short	uid;
  unsigned short	gid;
  unsigned		*stat_flags;
{

  /*
  ** Wipe out any settings that are there
  */
  UNSET_BIT(*stat_flags, FILE_READABLE);
  UNSET_BIT(*stat_flags, FILE_WRITEABLE);

  if (uid == 0)				/* User is root */
    {
      /*
      ** File is readable and writable regardless of what stat(2) says.
      */
      SET_BIT(*stat_flags, FILE_READABLE);
      SET_BIT(*stat_flags, FILE_WRITEABLE);
    }
  else if (uid == statbuf->st_uid)	/* User owns this file */
    {
      if (BIT_IS_SET(statbuf->st_mode, S_IRUSR))
					/* User is allowed to read */
	SET_BIT(*stat_flags, FILE_READABLE);
      if (BIT_IS_SET(statbuf->st_mode, S_IWUSR))
					/* User is allowed to write */
	SET_BIT(*stat_flags, FILE_WRITEABLE);
    }
  else if (gid == statbuf->st_gid)	/* User has same group as the file */
    {
      if (BIT_IS_SET(statbuf->st_mode, S_IRGRP))
					/* Group is allowed to read */
	SET_BIT(*stat_flags, FILE_READABLE);
      if (BIT_IS_SET(statbuf->st_mode, S_IWGRP))
					/* Group is allowed to write */
	SET_BIT(*stat_flags, FILE_WRITEABLE);
    }
  else					/* Not the owner or the same group; */
					/* try permissions for other */
    {
      if (BIT_IS_SET(statbuf->st_mode, S_IROTH))
					/* World is allowed to read */
	SET_BIT(*stat_flags, FILE_READABLE);
      if (BIT_IS_SET(statbuf->st_mode, S_IWOTH))
					/* World is allowed to write */
	SET_BIT(*stat_flags, FILE_WRITEABLE);
    }

}	/* CheckMode() */



/*
** Uses CheckMode() to set bits in `stat_flags' corresponding to
** FILE_READABLE and FILE_WRITEABLE.  FILE_EXISTS is set by this function.
** If the file doesn't exist and the file's directory is stat(2)'able,
** CheckMode() is called on the directory.
*/
static void
DoStat(name, stat_flags)
  char		*name;
  unsigned	*stat_flags;
{
  char			dirname[MAXPATHLEN];
					/* Directory part of `name' */
  unsigned short	egid;		/* Process' effective gid */
  unsigned short	euid;		/* Process' effective uid */
  /*
  extern unsigned short getegid();
  extern unsigned short geteuid();
  */
  struct stat		statbuf;	/* Returned by stat(2) */


  /*
  ** Get effective uid and gid
  */
  euid = (unsigned short)geteuid();
  egid = (unsigned short)getegid();

  /*
  ** Clear `errno' since its value is used below
  */
  errno = 0;

  if (stat(name, &statbuf) == -1)	/* Stat failed, find out why */
    switch (errno)
      {
      case ENOENT:			/* File named by `name' doesn't */
					/* exist; check the directory */
					/* instead  */
	UNSET_BIT(*stat_flags, FILE_EXISTS);
	GetDirname(name, dirname);
	if (stat(dirname, &statbuf) == -1)
					/* Directory isn't stat'able; */
					/* something's wrong here */
          error_exit( OlGetMessage((Display *)NULL, NULL,
                      0,
                      OleNfilefile_stuff,
                      OleTmsg1,
                      OleCOlClientOlamMsgs,
                      OleMfilefile_stuff_msg1,
                      (XrmDatabase)NULL),
                      dirname, 2);

	else
	  CheckMode(&statbuf, euid, egid, stat_flags);
	break;

      /*
      ** Explainable errors; just clear `stat_flags' and return
      */
      case EACCES:
      case EMULTIHOP:
      case ENOLINK:
      case ENOTDIR:
	*stat_flags = 0;
	break;

      default:
          error_exit( OlGetMessage((Display *)NULL, NULL,
                      0,
                      OleNfilefile_stuff,
                      OleTmsg1,
                      OleCOlClientOlamMsgs,
                      OleMfilefile_stuff_msg1,
                      (XrmDatabase)NULL),
                      name, 2);
	break;				/* Purposefully, redundant */
      }
  else					/* stat(2) successful; check mode */
    {
      SET_BIT(*stat_flags, FILE_EXISTS);
      CheckMode(&statbuf, euid, egid, stat_flags);
    }

}	/* DoStat() */


/*
** Extracts and formats `disp_string' from `disp_line'.  `disp_string' is
** expected to be a line from a file (ie. "/usr/X/lib/Xconnections") with
** three whitespace-separated fields corresponding to "display", "host", and
** "netspec"; comments and blank lines are allowed.  The three fields are
** combined with FormatDispEntry() and the length of the resulting string is
** returned.
** If the three fields couldn't be garnered from `disp_line' (could be a
** comment line or a blank one), `disp_string' is set to "" and 0 (its
** length) is returned.
** This function is expected to be used as the second argument in a call to
** GetLine() (below).
*/
int
ExtractDisp(disp_line, disp_string)
  char	*disp_line;
  char	*disp_string;
{
  char	*cptr;				/* Points to first '#' or '\n' */
  char	*disp;				/* Points to display field */
  char	*host;				/* Points to host field */
  char	*netspec;			/* Points to netspec field */


  /*
  ** Only consider characters up to the first '#' or '\n' as part of the
  ** string
  */
  if ((cptr = strpbrk(disp_line, "#\n")) != NULL)
    *cptr = '\0';

  if (ParseDispEntry(disp_line, &disp, &host, &netspec))
					/* ParseDispEntry() found all three; */
					/* now format them  */
    FormatDispEntry(disp, host, netspec, disp_string);
  else
    *disp_string = '\0';		/* Return an empty string */

  return strlen(disp_string);

}	/* ExtractDisp() */


/*
** Extracts `host' from `host_line'.  `host_line' is a line from a file (ie.
** "/etc/X0.hosts") that may contain a host name; if so, it is returned in
** `host' and its length is returned.
** If there is no host name in `host_line' (could be a comment line of a
** blank one), `host' is set to "" and 0 (its length) is returned.
** This function is expected to be used as the second argument in a call to
** GetLine() (below).
*/
int
ExtractHost(host_line, host)
  char	*host_line;
  char	*host;
{
  char	*cptr1;				/* Traverses along `host_line' */
  char	*cptr2;				/* Traverses along `host' */


  /*
  ** Only consider characters up to the first '#' or '\n' as part of the
  ** string
  */
  if ((cptr1 = strpbrk(host_line, "#\n")) != NULL)
    *cptr1 = '\0';

  cptr1 = host_line;
  cptr2 = host;

  /*
  ** Skip leading whitespace
  */
  while (*cptr1 != '\0' && isspace(*cptr1))
    ++cptr1;

  /*
  ** Copy all non-whitespace characters to `host'.
  */
  while (*cptr1 != '\0' && !isspace(*cptr1))
    *(cptr2++) = *(cptr1++);

  /*
  ** Mark end of `host'
  */
  *cptr2 = '\0';

  return strlen(host);

}	/* ExtractHost() */


/*
** Returns (in `entry') the result of joining `disp', `host', and `netspec'
** into one space-separated string of left-justified, padded fields.
*/
void
FormatDispEntry(disp, host, netspec, entry)
  char	*disp;
  char	*host;
  char	*netspec;
  char	*entry;
{

  sprintf(entry, "%-*s %-*s %-s", DISPFIELDLEN, disp, DISPFIELDLEN, host,
	  netspec);

}	/* FormatDispEntry() */


/*
** Returns the result of stripping the last component in the path `name' in
** `dirname'.
*/
static void
GetDirname(name, dirname)
  char	*name;
  char	*dirname;
{
  char	*cptr;				/* Pointer to last '/' */
  

  /*
  ** Find the last '/' in `name'.
  */
  cptr = strrchr(name, '/');

  if (cptr != NULL)			/* A '/' was found */
    {
      /*
      ** Copy `name' to `dirname' leaving off the last component
      */
      ++cptr;
      while (name != cptr)
	*dirname++ = *name++;
    }
  else					/* No '/' found */
    *dirname++ = '.';			/* Dirname must be '.'*/

  *dirname = '\0';			/* Mark end of `dirname' */

}	/* GetDirname() */


/*
** Reads lines from `fd' until EOF or `extract_line' is successful in
** locating an interesting string.  If such a string is located, memory is
** allocated for it and it is returned.  NULL is returned on EOF.
** Only MAXLINE characters from each `fd' line are considered.
*/
char *
GetLine(fd, extract_line)
  register FILE	*fd;
  register int	(*extract_line)();
{
  char	buf[MAXLINE];		        /* Buffer for fgets(3) */
  register int	len;			/* Length of extracted string */
  char	line[MAXLINE];		        /* Extracted string */
  char		*return_line = NULL;	/* Returned */


  while (fgets(buf, MAXLINE, fd))	/* Read until EOF */
    if (len = (*extract_line)(buf, line))
					/* `extract_line' found what it was */
					/* looking for */
      {
	/*
	** Make a copy of it and return it
	*/
	if ((return_line = malloc((unsigned)(len + 1))) == NULL)
          error_exit( OlGetMessage((Display *)NULL, NULL,
                      0,
                      OleNfilefile_stuff,
                      OleTmsg2,
                      OleCOlClientOlamMsgs,
                      OleMfilefile_stuff_msg2,
                      (XrmDatabase)NULL),
                      len + 1, 2);
	(void)strcpy(return_line, line);
	break;
      }

  return return_line;

}	/* GetLine() */


/*
** Returns a readable stream for `name'.  The result of a DoStat() (above)
** is returned in `stat_flags'.
*/
FILE *
OpenFile(name, stat_flags)
  char		*name;
  unsigned	*stat_flags;
{
  FILE	*stream;
  

  *stat_flags = 0;
  DoStat(name, stat_flags);

  if (BIT_IS_SET(*stat_flags, FILE_EXISTS) &&
      BIT_IS_SET(*stat_flags, FILE_READABLE))
    {					/* File exists and is readable */
      if ((stream = fopen(name, "r")) == (FILE *)NULL)
        error_exit( OlGetMessage((Display *)NULL, NULL,
                    0,
                    OleNfilefile_stuff,
                    OleTmsg3,
                    OleCOlClientOlamMsgs,
                    OleMfilefile_stuff_msg3,
                    (XrmDatabase)NULL),
                    name, 2);

    }
  else					/* Doesn't exist or isn't readable */
    stream = (FILE *)NULL;

  return stream;

}	/* OpenFile() */


/*
** Returns pointers (to pointers) to three whitespace-separated fields from
** `disp_line' in `disp',  `host', and `netspec'.  If all three fields were
** found, 1 is returned, otherwise, 0 is returned.
** Note that the pointers are to positions within `disp_line' itself.  This
** function depends on strtok(3) putting a '\0' after each token.
*/
int
ParseDispEntry(disp_line, disp, host, netspec)
  char	*disp_line;
  char	**disp;
  char	**host;
  char	**netspec;
{
  char	*cptr;


  if ((*disp = strtok(disp_line, " \t")) == NULL)
    return 0;

  if ((*host = strtok(NULL, " \t")) == NULL)
    return 0;

  if ((*netspec = strtok(NULL, " \t")) == NULL)	/* @ */
    return 0;

  if ((cptr = strpbrk(*netspec, " \t")) != NULL)	/* @ */
    *cptr = '\0';

  return 1;

}	/* ParseDispEntry() */
