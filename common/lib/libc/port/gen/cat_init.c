/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cat_init.c	1.4.1.5"

#include "synonyms.h"
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <nl_types.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

extern int munmap();
extern char *_cat_itoa();
static int cat_malloc_init();
static int cat_mmp_init();

/*
 * Read a catalog and init the internal structure
 */
_cat_init(name, res)
  char *name;
  nl_catd res;
{
  int fd;
  long magic;

  /*
   * Read file header
   */
  if((fd=open(name,0)) < 0) {
    /*
     * Need read permission
     */
    return 0;
  }

  if (read(fd, (char *)&magic, sizeof(long)) == sizeof(long)){
    if (magic == CAT_MAGIC)
      return cat_malloc_init(fd,res);
    else
      return cat_mmp_init(fd,name,res);
  }
  return 0;
}

/*
 * Read a malloc catalog and init the internal structure
 */
static int
cat_malloc_init(fd, res)
  int fd;
  nl_catd res;
{
  struct cat_hdr hdr;
  char *mem;

  lseek(fd,0L,0);
  if (read(fd, (char *)&hdr, sizeof(struct cat_hdr)) != sizeof(struct cat_hdr))
    return 0;
  if ((mem = malloc(hdr.hdr_mem)) != (char*)0){

    if (read(fd, mem, hdr.hdr_mem) == hdr.hdr_mem){
      res->info.m.sets = (struct cat_set_hdr*)mem;
      res->info.m.msgs = (struct cat_msg_hdr*)(mem + hdr.hdr_off_msg_hdr);
      res->info.m.data = mem + hdr.hdr_off_msg;
      res->set_nr = hdr.hdr_set_nr;
      res->type = MALLOC;
      close(fd);
      return 1;
    } else
      free(mem);
  }

  close(fd);
  return 0;
}


extern int _mmp_opened;

/*
 * Do the gettxt stuff
 */
static int
cat_mmp_init (sfd,catname,res)
  char  *catname;
  nl_catd res;
{
  char symb_name[MAXNAMLEN];
  char symb_path[MAXNAMLEN];
  char message_file[MAXNAMLEN];
  char buf[MAXNAMLEN];
  struct stat sb;
  char *ptr;
  static int count = 1;
  int mfd;

  if (_mmp_opened == NL_MAX_OPENED) {
    close(sfd);
    return 0;
  }

  /*
   * get the number of sets
   * of a set file
   */
  if (fstat(sfd, &sb) == -1) {
    close(sfd);
    return 0;
  }

  res->info.g.sets = (struct set_info *)mmap(0, sb.st_size,
       PROT_READ, MAP_SHARED, sfd, 0);

  if (res->info.g.sets == (struct set_info *)-1) {
    close(sfd);
    return 0;
  }

  res->type = MKMSGS;
  res->info.g.set_size = sb.st_size;
  res->set_nr = *((int*)(res->info.g.sets));
  close(sfd);
  
  if (res->set_nr > NL_SETMAX) {
    munmap((caddr_t)res->info.g.sets, sb.st_size);
    return 0;
  }

  /*  Now open the file with the messages in and memory map this
   */
  /* sprintf(message_file, "%s.m", catname); */
  strcpy(message_file, catname);
  strcat(message_file, ".m");

  if ((mfd = open(message_file, O_RDONLY)) < 0) {
    munmap((caddr_t)res->info.g.sets, sb.st_size);
    return 0;
  }

  if (fstat(mfd, &sb) == -1) {
    munmap((caddr_t)res->info.g.sets, sb.st_size);
    close(mfd);
    return 0;
  }

  if ((res->info.g.msgs = mmap(0, sb.st_size, PROT_READ,
       MAP_SHARED, mfd, 0)) == (caddr_t)-1) {
    munmap((caddr_t)res->info.g.sets, sb.st_size);
    close(mfd);
    return 0;
  }

  res->info.g.msg_size = sb.st_size;
  close(mfd);
  _mmp_opened++;
  return 1;
}
