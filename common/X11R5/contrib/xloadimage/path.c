/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4xloadimage:path.c	1.1"
/* path.c:
 *
 * functions that deal with the image path
 *
 * jim frost 10.03.89
 *
 * Copyright 1989, 1990 Jim Frost.  See included file "copyright.h" for
 * complete copyright information.
 */

#include "copyright.h"
#include "xloadimage.h"
#include <X11/Xos.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

extern int errno;

static unsigned int  NumPaths= 0;
static unsigned int  NumExts= 0;
static char         *Paths[BUFSIZ];
static char         *Exts[BUFSIZ];
static char         *PathToken= "path=";
static char         *ExtToken= "extension=";

#define VOIDSECTION 0
#define PATHSECTION 1
#define EXTSECTION  2

static void readPathsAndExts(name)
     char *name;
{ FILE         *f;
  char          tokenbuf[BUFSIZ];
  char          buf[BUFSIZ];
  unsigned int  secnum;
  unsigned int  linenum;
  unsigned int  a, b, l;
  int           c;

  if (! (f= fopen(name, "r")))
    return;

  secnum= VOIDSECTION;
  linenum= 0;
  while (fscanf(f, "%s", tokenbuf) > 0) {
    linenum++;
    l= strlen(tokenbuf);
    for (a= 0, b= 0; a < l; a++, b++) {
      if (tokenbuf[a] == '\\')
	tokenbuf[b]= tokenbuf[++a];
      else if (b != a)
	tokenbuf[b]= tokenbuf[a];
      if (tokenbuf[a] == '#') {
	tokenbuf[b]= '\0';
	while (((c= fgetc(f)) != '\n') && (c != EOF))
	  ;
	break;
      }
    }

    if (!strncmp(tokenbuf, PathToken, strlen(PathToken))) {
      secnum= PATHSECTION;
      if (sscanf(tokenbuf + strlen(PathToken), "%s", buf) != 1)
	continue;
    }
    else if (!strncmp(tokenbuf, ExtToken, strlen(ExtToken))) {
      secnum= EXTSECTION;
      if (sscanf(tokenbuf + strlen(ExtToken), "%s", buf) != 1)
	continue;
    }
    else
      strcpy(buf, tokenbuf);
    if (buf[0] == '\0')
      continue;

    switch (secnum) {
    case VOIDSECTION:
      printf("%s: %d: Syntax error\n", name, linenum); /* ala BASIC */
      fclose(f);
      return;
    case PATHSECTION:
      if (NumPaths < BUFSIZ - 1)
	Paths[NumPaths++]= dupString(buf);
      else {
	printf("%s: %d: Path table overflow\n", name, linenum);
	fclose(f);
	return;
      }
      break;
    case EXTSECTION:
      if (NumExts < BUFSIZ - 1)
	Exts[NumExts++]= dupString(buf);
      else {
	printf("%s: %d: Extension table overflow\n", name, linenum);
	fclose(f);
      }
      break;
    }
  }
}

void loadPathsAndExts()
{ static int     havepaths= 0;
  struct passwd *pw;
  char           buf[BUFSIZ];

  if (havepaths)
    return;
  havepaths= 1;

  if (pw= getpwuid(getuid())) {
    sprintf(buf, "%s/.xloadimagerc", pw->pw_dir);
    if (! access(buf, R_OK)) {
      readPathsAndExts(buf);
      return; /* don't read system file if user has one */
    }
  }
  else
    printf("Can't find your password file entry?!?\n");
#ifdef SYSPATHFILE
  readPathsAndExts(SYSPATHFILE);
#endif
}

/* find an image with paths and extensions from defaults files.  returns
 * -1 if access denied or not found, 0 if ok.
 */

int findImage(name, fullname)
     char *name, *fullname;
{ unsigned int   p, e;
  struct stat    sbuf;

  strcpy(fullname, name);
  if (! stat(fullname, &sbuf))
    return(access(fullname, R_OK));
  strcat(fullname, ".Z");
  if (! stat(fullname, &sbuf))
    return(access(fullname, R_OK));
  for (p= 0; p < NumPaths; p++) {
    sprintf(fullname, "%s/%s", Paths[p], name);
    if (! stat(fullname, &sbuf))
      return(access(fullname, R_OK));
    strcat(fullname, ".Z");
    if (! stat(fullname, &sbuf))
      return(access(fullname, R_OK));
    for (e= 0; e < NumExts; e++) {
      sprintf(fullname, "%s/%s%s", Paths[p], name, Exts[e]);
      if (! stat(fullname, &sbuf))
	return(access(fullname, R_OK));
      strcat(fullname, ".Z");
      if (! stat(fullname, &sbuf))
	return(access(fullname, R_OK));
    }
  }
  errno= ENOENT; /* file not found */
  return(-1);
}

/* list images along our path
 */

void listImages()
{ unsigned int a;
  char         buf[BUFSIZ];

  if (!NumPaths) {
    printf("No image path\n");
    return;
  }
  for (a= 0; a < NumPaths; a++) {
    printf("%s:\n", Paths[a]);
    fflush(stdout);
    sprintf(buf, "ls %s", Paths[a]);
    if (system(buf) < 0) {
      perror("ls");
      return;
    }
  }
  return;
}

void showPath()
{ int a;

  if (!NumPaths && !NumExts) {
    printf("No image paths or extensions\n");
    return;
  }
  if (NumPaths) {
    printf("Image path:");
    for (a= 0; a < NumPaths; a++)
      printf(" %s", Paths[a]);
    printf("\n");
  }
  if (NumExts) {
    printf("Image extensions:");
    for (a= 0; a < NumExts; a++)
      printf(" %s", Exts[a]);
    printf("\n");
  }
}
