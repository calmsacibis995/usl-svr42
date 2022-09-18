/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */

#ident	"@(#)face:src/filecab/fileb/pathfind.c	1.1"

/***************************************************************************
* (c) 1991 Siemens Nixdorf Informationssysteme AG
* All Rights reserved.
* 
*
* pathfind [-p] <filename>: search for <filename> in all PATH-directories
*      if '-p' is specified: print full path
*      exit-code 0: found, 1: not found or syntax error
*      <filename> must be an executable file (full paths are allowed too)!
*
* written by thomas fuerbringer
*
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __STDC__
void main(int,char **);
int executable(char *);
#endif

/* Is path "path" a executable file ? 1=yes , 0=no */
static int executable(path)
char *path;
{
  struct stat sbuf;

  if (stat(path,&sbuf) < 0) return(0);
  if (S_ISREG(sbuf.st_mode) && (S_IXUSR & sbuf.st_mode))
    return(1);
  else
    return(0);
}

void main(argc,argv)
int argc;
char **argv;
{
  char *file,*path,*p;
  int printflag=0;
  int mall_len;

  if (argc < 2) exit(1);
  if (strcmp(argv[1],"-p")==0)
  {
    printflag=1;
    argc--;
    argv++;
  }
  if (argc != 2) exit(1);
  argv++;

  if ((p=getenv("PATH")) == NULL) exit(1);
  mall_len=strlen(p)+20;
  if (mall_len < 200) mall_len=200;
  path=(char *)malloc(mall_len);
  file=(char *)malloc(mall_len);
  if (!file || !path) exit(1);
  strcpy(path,p);

  /** first check for pathname as argument **/

  if (strrchr(*argv,'/'))
  {
    if (executable(*argv))
    {
      if (printflag)
      {
        if ((*argv)[0]!='/')
        {
           printf("%s",getcwd(file,mall_len));
	   if ((*argv)[0]=='.' && (*argv)[1]=='/')
	     printf("/%s\n",(*argv)+2);
	   else
	     printf("/%s\n",*argv);
        }
	else
          printf("%s\n",*argv);
      }
      exit(0);
    }
    else
    exit(1);
  }

  p=strtok(path,":"); /* separate $PATH into parts */
  while (p)
  {
    int sl;

    strcpy(file,p);    /* file = partpath */
    sl=strlen(file);
    if (file[sl-1]!='/')  /* append '/' if necessary:
             file = partpath/ */
    {
      file[sl]='/';
      file[sl+1]=0;
    }
    strcat(file,*argv);  /* file = partpath/filename */
    if (executable(file)) /* is file executable ? */
    {
      if (printflag) printf("%s\n",file); /* got it !! */
      exit(0);
    }
    p=strtok(NULL,":");
  }
  exit(1);
}

