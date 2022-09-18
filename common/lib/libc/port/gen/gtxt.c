/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/gtxt.c	1.9"

/* __gtxt(): Common part to gettxt() and pfmt()	*/

#ifdef __STDC__	
	#pragma weak setcat = _setcat
#endif
#include "synonyms.h"
#include "shlib.h"
#include <string.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <pfmt.h>
#include "_locale.h"
#include <unistd.h>
#include <errno.h>

#define P_locale "/usr/lib/locale/"
#define L_locale	(sizeof P_locale)
#define DEF_LOCALE (const char *)"C"
#define MESSAGES "/LC_MESSAGES/"
static const char *not_found = "Message not found!!\n";
static struct db_info *db_info;
static int db_count, maxdb;

#define FALSE			0
#define TRUE			1

static struct db_info {
	char	db_name[DB_NAME_LEN];	/* Name of the message file */
	int	addr;			/* Virtual memory address   */
	size_t	length;
	char	saved_locale[15];
	char	flag;
};

#define DB_EXIST	1		/* The catalogue exists	   */
#define DB_OPEN		2		/* Already tried to open   */
#define DB_DFLT		4		/* In default locale	   */

/* Minimum number of open catalogues */
#define MINDB			3

static char cur_cat[DB_NAME_LEN];

/* setcat(cat): Specify the default catalogue.
 * Return a pointer to the local copy of the default catalogue
 */
const char *
setcat(cat)
const char *cat;
{
	if (cat){
		(void) strncpy(cur_cat, cat, sizeof cur_cat - 1);
		cur_cat[sizeof cur_cat - 1] = '\0';
	}
	return cur_cat[0] ? cur_cat : NULL;
}

/* __gtxt(catname, id, dflt): Return a pointer to a message.
 *	catname is the name of the catalog. If null, the default catalog is
 *		used.
 *	id is the numeric id of the message in the catalogue
 *	dflt is the default message.
 *	
 *	Information about non-existent catalogues is kept in db_info, in
 *	such a way that subsequent calls with the same catalogue do not
 *	try to open the catalogue again.
 */
const char *
__gtxt(catname, id, dflt)
const char *catname;
int id;
const char *dflt;
{
	char pathname[128];
	register int i;
	int fd;
	int save_errno = errno;
	struct	stat sb;
	caddr_t addr;
	char cur_flag;
	struct db_info *db;

	/* Check for invalid message id */
	if (id < 0)
		return not_found;

	/* First time called, allocate space */
	if (!db_info){
		if ((db_info = (struct db_info *)
			malloc(MINDB * sizeof(struct db_info))) == 0)
			return not_found;
		maxdb = MINDB;
	}

	/* If catalogue is unspecified, use default catalogue.
	/* No catalogue at all is an error */
	if (!catname || !*catname){
		if (!cur_cat || !*cur_cat)
			return not_found;
		catname = cur_cat;
	}

	/* Retrieve catalogue in the table */
	for(i = 0, cur_flag = 0 ; ; ){
		for (; i < db_count ; i++){
			if (strcmp(catname, db_info[i].db_name) == 0)
				break;
		}
		/* New catalogue */
		if (i == db_count){
			if (db_count == maxdb){
				if ((db_info = (struct db_info *)
					realloc(db_info, ++maxdb * sizeof(struct db_info))) == 0)
					return not_found;
			}
			(void) strcpy(db_info[i].db_name, catname);
			db_info[i].flag = cur_flag;
			db_info[i].saved_locale[0] = '\0';
			db_count++;
		}
		db = &db_info[i];

		/* Check for a change in locale. If necessary unmap and close
		 * the opened catalogue. The entry in the table is
		 * NOT freed. The catalogue offset remains valid. */
		if (strcmp(_cur_locale[LC_MESSAGES], db->saved_locale) != 0){
			if (db->flag & (DB_OPEN|DB_EXIST)==(DB_OPEN|DB_EXIST)){
				munmap((caddr_t)db->addr, db->length);
			}
			db->flag &= ~(DB_OPEN|DB_EXIST);
		}

		/* Retrieve the message from the catalogue */
		for (;;){
			/* Open and map catalogue if not done yet. In case of
			 * failure, mark the catalogue as non-existent */
			if (!(db->flag & DB_OPEN)){
				db->flag |= DB_OPEN;
				(void) strcpy(db->saved_locale, 
					_cur_locale[LC_MESSAGES]);
				(void)strcpy(pathname, P_locale);
				(void)strcpy(pathname + L_locale - 1, 
					(db->flag & DB_DFLT) ? 
					DEF_LOCALE : db->saved_locale);
				(void)strcat(pathname, MESSAGES);
				(void)strcat(pathname, db->db_name);
				if ((fd = open(pathname, O_RDONLY)) == -1 
					|| fstat(fd, &sb) ==-1
					|| (addr = mmap(0, sb.st_size, 
					PROT_READ, MAP_SHARED,
					fd, 0)) == (caddr_t)-1){
					if (fd != -1) {
						(void) close(fd);
					}
					errno = save_errno;
				}
				else {
					(void)close(fd);
					db->flag |= DB_EXIST;
					db->addr = (int)addr;
					db->length = sb.st_size;
				}
			}
			/* Return the message from the catalogue */
			if (id != 0 && db->flag & DB_EXIST && id <= *(int *)(db->addr)){
				return (char *)(db->addr + *(int *)(db->addr + 
					id * sizeof(int)));
			}
			/* Return the default message (or 'Message not found' 
			 * if no default message was passed */

			if (db->flag & DB_DFLT ||
				    strcmp(db->saved_locale, DEF_LOCALE) == 0)
				return (dflt && *dflt) ? dflt : not_found;
			if (!(db->flag & DB_EXIST)){
				db->flag |= DB_DFLT;
				db->flag &= ~DB_OPEN;
				continue;
			}
			break;
		}
		cur_flag = DB_DFLT;
		i++;
	}
}

