/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libia:common/lib/libia/lvlia.c	1.7.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libia/lvlia.c,v 1.1 91/02/28 20:45:58 ccs Exp $"

#include <sys/types.h>
#include <sys/time.h>
#include <mac.h>
#include <ia.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>


/*
 *	lvlia -  read/write /etc/security/ia/level/login-name files
 */

int
lvlia(cmd, levelp, namep, cnt)
int	cmd;
level_t	**levelp;
char	*namep;
long	*cnt;
{
	int	fd_lvl;
	struct  stat    stbuf;
	char	*lvlfl;
	char	*tmplvlfl;
	char	*tfname = ":tmp";
	int	i;
	long	xcnt;
	level_t *lvlp;

	lvlfl = (char *) malloc(strlen(LVLDIR) + strlen(namep) + (size_t)1);
	tmplvlfl = (char *) malloc(strlen(LVLDIR) + strlen(namep) +
				strlen(tfname) + (size_t)1);

	(void)strcpy(lvlfl, LVLDIR);
	(void)strcat(lvlfl, namep);

	switch(cmd) {
		case IA_READ:
			if ((stat(lvlfl, &stbuf)) != 0)
				return(-1);

			xcnt = (stbuf.st_size/sizeof(level_t));
			lvlp = (level_t *) calloc(xcnt, sizeof(level_t));
			if (lvlp == NULL)
				return -1;

			if ((fd_lvl = open(lvlfl,O_RDONLY)) < 0)
				return -1;

			if ((read(fd_lvl, lvlp, stbuf.st_size)) != stbuf.st_size) {
				(void)close(fd_lvl);
				return -1;
			}
			*cnt = xcnt;
			*levelp = lvlp;
			(void)close(fd_lvl);
			return 0;

		case IA_WRITE:
			if ((stat(lvlfl, &stbuf)) != 0) {
				if ((fd_lvl = creat(lvlfl, 0460)) < 0)
					return -1;

				if ((write(fd_lvl, levelp, (sizeof(level_t) * *cnt))) 
					!= (sizeof(level_t) * *cnt)) {
					(void)close(fd_lvl);
					(void)unlink(lvlfl);
					return -1;
				}
				(void)close(fd_lvl);
				if ((stat(LVLDIR, &stbuf)) == 0) { 
					(void)lvlfile(lvlfl, MAC_SET, &stbuf.st_level);
					(void)chown(lvlfl, stbuf.st_uid, stbuf.st_gid);
				}
				else
					(void)chown(lvlfl, (uid_t)0, (gid_t)3);
			} else {
				(void)strcpy(tmplvlfl, LVLDIR);
				(void)strcat(tmplvlfl, namep);
				(void)strcat(tmplvlfl, tfname);

				if ((fd_lvl = creat(tmplvlfl, 0460)) < 0)
					return -1;

				if ((write(fd_lvl, levelp, (sizeof(level_t) * *cnt))) 
					!= (sizeof(level_t) * *cnt)) {
					(void)close(fd_lvl);
					(void)unlink(lvlfl);
					return -1;
				}
				close(fd_lvl);
				(void)lvlfile(tmplvlfl, MAC_SET, &stbuf.st_level);
				(void)chmod(tmplvlfl,(S_IRUSR | S_IRGRP));
				(void)chown(tmplvlfl, stbuf.st_uid, stbuf.st_gid);

				if (rename(tmplvlfl, lvlfl) < 0 ) {
					(void)unlink(tmplvlfl);
					return -1;
				}
			}
			return 0;

		case IA_FREE:
				free(levelp);
				return 0;

		default:
			return -1;
	}
		
}
