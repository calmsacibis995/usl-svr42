/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libia:common/lib/libia/getia.c	1.1.4.3"
#ident  "$Header: getia.c 1.3 91/06/21 $"

#include <sys/types.h>
#include <sys/param.h>
#include <audit.h>
#include <ia.h>
#include <search.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/mac.h>

extern	int	strcmp();
extern	char	*strcpy();

static	void	_set_attr();

/*
 *	getiasz - passed a pointer to an index structure with the
 *	          name member set. The routine searchs the index file
 *		  for the name`s entry and initializes the pointer
 *		  with the length and offset of name's entry in the 
 *		  master file. A 0 is returned on success, a 1 if the
 *		  name was not found, and -1 if an error occurred.
 */
int
getiasz(indxp)
struct	index	*indxp;
{
	int	fd_indx;
	int	cnt;
	struct  stat    stbuf;
	struct	index	*index;
	struct	index	*midxp;

	if ((stat(INDEX, &stbuf)) != 0)
		return(-1);

	cnt = (stbuf.st_size/sizeof(struct index));

	if ((fd_indx = open(INDEX,O_RDONLY)) < 0)
		return(-1);

	if ((midxp = (struct index *)mmap(0, stbuf.st_size, PROT_READ, 
		 MAP_SHARED, fd_indx, 0)) < (struct index *) 0) {

			(void)close(fd_indx);
			return(-1);
	}

	index = bsearch(indxp->name, midxp, cnt, sizeof(struct index), strcmp);

	if(index == NULL) {
		(void)munmap((caddr_t)midxp, stbuf.st_size);
		(void)close(fd_indx);
		return(1);
	}
	
	memcpy(indxp, index, sizeof(struct index));

	if(indxp == NULL) {
		(void)munmap((caddr_t)midxp, stbuf.st_size);
		(void)close(fd_indx);
		return(1);
	}

	(void)munmap((caddr_t)midxp, stbuf.st_size);
	(void)close(fd_indx);
	return(0);
}

/*
 *	getianam - passed a pointer to an index structure containing
 *		   name, length, and offset for name's master entry,
 *		   and a pointer to set to the name's master entry.
 *	 	   Using offset and length the master entry is
 *		   read from the master file into the space pointed 
 *		   to by mastp. 0 is returned on success and -1 otherwise.
 */

int
getianam(indxp, mastp)
struct	index	*indxp;
struct	master	*mastp;
{

	int	fd_mast;

	if ((fd_mast = open(MASTER,O_RDONLY)) == -1)
		return(-1);

	if (lseek(fd_mast, indxp->offset, 0) == -1) {
		(void)close(fd_mast);
		return(-1);
	}

	if ((read( fd_mast, mastp, indxp->length)) != indxp->length) {
		(void)close(fd_mast);
		return(-1);
	}

	mastp->ia_lvlp = (level_t *) (((char *) mastp) + sizeof(struct master));
	mastp->ia_sgidp = (gid_t *) (((char *) mastp) + sizeof(struct master) 
				+ (mastp->ia_lvlcnt * sizeof(level_t)) );
	mastp->ia_dirp = ( ((char *) mastp) + sizeof(struct master) 
				+ (mastp->ia_lvlcnt * sizeof(level_t)) 
				+ (mastp->ia_sgidcnt * sizeof(gid_t)) );
	mastp->ia_shellp = (mastp->ia_dirp + mastp->ia_dirsz +1);

	(void)close(fd_mast);
	return(0);
}

/*
 *	putiaent - passed a name and a pointer to a master structure the name's
 *		   master entry is updated with the new master structure 
 *		   if the name already existed or a new entry is put in the 
 *		   master file. The index file is updated to reflect the changes.
 *		   0 is returned on success and -1 otherwise.
 */		   

int
putiaent(namp, mastp)
char	*namp;
struct	master	*mastp;
{

	int	fd_indx, fd_mast;
	int	fd_indxtmp, fd_mastmp;
	long	offset, msize, cnt;
	char	*mmasp, *mindxp;
	struct	index	*indp, *midxp, *mqidxp;
	struct	index	index;
	struct	index	*indxp = &index;
	struct  stat    m_stbuf, i_stbuf;
	level_t	m_lid, i_lid;

	
	/* calculate size of master entry		*/

	msize = (sizeof(struct master) + mastp->ia_dirsz + mastp->ia_shsz + 2 
		+ (mastp->ia_lvlcnt * sizeof(level_t))
		+ (mastp->ia_sgidcnt * sizeof(gid_t)) );

	/* Check to see if we are changing logname	*/

	if (strcmp(namp, mastp->ia_name) == 0)
		(void)strcpy(indxp->name, mastp->ia_name);
	else
		(void)strcpy(indxp->name, namp);

	/*
	 * get the MAC level (if the level exists) of the MASTER and
	 * INDEX files for use later.
	*/
	if (lvlfile(MASTER, MAC_GET, &m_lid) < 0)
		m_lid = 0;

	if (lvlfile(INDEX, MAC_GET, &i_lid) < 0)
		i_lid = 0;

	/*  call getiasz to determine if we are adding  */
	/*  a new entry or updating an existing one     */

	switch(getiasz(indxp)) {

		case -1:
			return(-1);

		default:
			return(-1);

	/* Update an existing entry	*/

		case 0:

		/* If the new entry fits in the old entry then		*/
		/* update the master with new entry and we are done	*/

		if (stat(MASTER, &m_stbuf) < 0) 
			return(-1);

		if ((fd_mastmp = open( MASTMP, O_WRONLY|O_CREAT, m_stbuf.st_mode)) < 0)
			return(-1);
		
		if ((fd_mast = open( MASTER, O_RDONLY)) < 0) {
			(void)close(fd_mastmp);
			(void)unlink(MASTMP);
			return(-1);
		}

		if ((mmasp = mmap(0, m_stbuf.st_size, PROT_READ, 
			 MAP_SHARED, fd_mast, 0)) < (caddr_t) 0) {
				(void)close(fd_mastmp);
				(void)close(fd_mast);
				(void)unlink(MASTMP);
				return(-1);
		}

		if (write(fd_mastmp, mmasp, m_stbuf.st_size) != m_stbuf.st_size) {
			(void)close(fd_mastmp);
			(void)close(fd_mast);
			(void)unlink(MASTMP);
			(void)munmap(mmasp, m_stbuf.st_size);
			return(-1);
		}

		if ((munmap(mmasp, m_stbuf.st_size) < 0) || (close(fd_mast) < 0)) {
			(void)close(fd_mastmp);
			(void)unlink(MASTMP);
			return(-1);
		}

		/* if the entry fits then lseek to the old entry */
		/* else write the entry at the end of the file   */

		if (msize <= indxp->length) {

			if (lseek(fd_mastmp, indxp->offset, 0) < 0) {
				(void)close(fd_mastmp);
				(void)unlink(MASTMP);
				return(-1);
			}
		}

		if (write(fd_mastmp, mastp, msize) != msize) {
			(void)close(fd_mastmp);
			(void)unlink(MASTMP);
			return(-1);
		}
	
		if (close(fd_mastmp) < 0) 
			return(-1);

		if (access(OMASTER, 0) == 0) {
			if (unlink (OMASTER) < 0) {
				(void)unlink(MASTMP);
				return(-1);
			}
		}

		if (rename(MASTER, OMASTER) == -1) {
			(void)unlink(MASTMP);
			return(-1);
		}

		_set_attr(MASTMP, &m_stbuf, m_lid);

		if (rename(MASTMP, MASTER) == -1) {
			(void)rename(OMASTER, MASTER);
			(void)unlink(MASTMP);
			return(-1);
		}

		/*  If the entry fit and we are not changing logname 	*/
		/*  then we are done 					*/

		if ((msize <= indxp->length) && (strcmp(namp, mastp->ia_name) == 0))
			return(0);

		/* Update the index entry with new offset and length */

		offset = m_stbuf.st_size;

		if (stat(INDEX, &i_stbuf) < 0) {
			(void)rename(OMASTER, MASTER);
			return(-1);
		}

		if ((fd_indxtmp = open(TMPINDEX, O_RDWR|O_CREAT, i_stbuf.st_mode)) < 0) {
			(void)rename(OMASTER, MASTER);
			return(-1);
		}
		
		if ((fd_indx = open( INDEX, O_RDONLY)) < 0) {
			(void)rename(OMASTER, MASTER);
			(void)close(fd_indxtmp);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if ((mindxp = mmap(0, i_stbuf.st_size, PROT_READ, 
			 MAP_SHARED, fd_indx, 0)) < (caddr_t) 0) {
				(void)rename(OMASTER, MASTER);
				(void)close(fd_indx);
				(void)close(fd_indxtmp);
				(void)unlink(TMPINDEX);
				return(-1);
		}

		if (write(fd_indxtmp, mindxp, i_stbuf.st_size) != i_stbuf.st_size) {
			(void)rename(OMASTER, MASTER);
			(void)close(fd_indx);
			(void)close(fd_indxtmp);
			(void)unlink(TMPINDEX);
			(void)munmap(mindxp, i_stbuf.st_size);
			return(-1);
		}

		if ((munmap(mindxp, i_stbuf.st_size) < 0) || (close(fd_indx) < 0)) {
			(void)rename(OMASTER, MASTER);
			(void)close(fd_indxtmp);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if ((midxp = (struct index *)mmap(0, i_stbuf.st_size, PROT_WRITE 
			 |PROT_READ, MAP_SHARED, fd_indxtmp, 0)) < (struct index *) 0) {
				(void)rename(OMASTER, MASTER);
				(void)close(fd_indxtmp);
				(void)unlink(TMPINDEX);
				return(-1);
		}


		cnt = (i_stbuf.st_size/sizeof(struct index));

		/* search the index file for the entry	*/

		indxp = bsearch(namp, midxp, cnt, sizeof(struct index), strcmp);

		if (indxp == NULL) {
			(void)rename(OMASTER, MASTER);
			(void)close(fd_indxtmp);
			(void)unlink(TMPINDEX);
			(void)munmap((caddr_t)midxp, i_stbuf.st_size);
			return(-1);
		}

		/* If names are different then update 	*/
		/* index with new logname 		*/

		if (strcmp(namp, mastp->ia_name) != 0)
			(void)strcpy(indxp->name, mastp->ia_name);

		/* If new entry is larger than the old			*/
		/* entry update index with new length and offset	*/

		if (msize > indxp->length) {
			indxp->length = msize;
			indxp->offset = offset;
		}

		qsort(midxp, cnt, sizeof(struct index), strcmp);

		if (munmap((caddr_t)midxp, i_stbuf.st_size) < 0) {
			(void)rename(OMASTER, MASTER);
			(void)close(fd_indxtmp);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if (close(fd_indxtmp) < 0) {
			(void)rename(OMASTER, MASTER);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if (access(OINDEX, 0) == 0) {
			if (unlink(OINDEX) < 0) {
				(void)rename(OMASTER, MASTER);
				(void)unlink(TMPINDEX);
				return(-1);
			}
		}

		if (rename(INDEX, OINDEX) == -1) {
			(void)rename(OMASTER, MASTER);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		_set_attr(TMPINDEX, &i_stbuf, i_lid);

		if (rename(TMPINDEX, INDEX) == -1) {
			(void)rename(OINDEX, INDEX);
			(void)rename(OMASTER, MASTER);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		/* master and index files are updated	*/
		return(0);

		/* Add a new entry to the master and index files */

		case 1:
		if (stat(MASTER, &m_stbuf) < 0)
			return(-1);

		/* set length and offset of the entry	*/

		indxp->length = msize;
		indxp->offset = m_stbuf.st_size;

		if ((fd_mastmp = open(MASTMP, O_RDWR|O_APPEND|O_CREAT, m_stbuf.st_mode)) < 0)
			return(-1);
		
		if ((fd_mast = open( MASTER, O_RDONLY)) < 0) {
			(void)close(fd_mastmp);
			(void)unlink(MASTMP);
			return(-1);
		}

		if (m_stbuf.st_size){
			if ((mmasp = mmap(0, m_stbuf.st_size, PROT_READ, 
				 MAP_SHARED, fd_mast, 0)) < (caddr_t) 0) {
					(void)close(fd_mastmp);
					(void)close(fd_mast);
					(void)unlink(MASTMP);
					return(-1);
			}

			if (write(fd_mastmp, mmasp, m_stbuf.st_size) != m_stbuf.st_size) {
				(void)munmap(mmasp, m_stbuf.st_size);
				(void)close(fd_mastmp);
				(void)close(fd_mast);
				(void)unlink(MASTMP);
				return(-1);
			}

			if ((munmap(mmasp, m_stbuf.st_size) < 0) || (close(fd_mast) < 0)) {
				(void)close(fd_mastmp);
				(void)unlink(MASTMP);
				return(-1);
			}
		}

		if (stat(INDEX, &i_stbuf) < 0) {
			(void)close(fd_mastmp);
			(void)unlink(MASTMP);
			return(-1);
		}

		if ((fd_indxtmp = open(TMPINDEX, O_RDWR|O_APPEND|O_CREAT, i_stbuf.st_mode)) < 0) {
			(void)close(fd_mastmp);
			(void)unlink(MASTMP);
			return(-1);
		}
		
		if ((fd_indx = open( INDEX, O_RDONLY)) < 0) {
			(void)close(fd_mastmp);
			(void)close(fd_indxtmp);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if (i_stbuf.st_size){
			if ((mindxp = mmap(0, i_stbuf.st_size, PROT_READ, 
				 MAP_SHARED, fd_indx, 0)) < (caddr_t) 0) {
					(void)close(fd_mastmp);
					(void)close(fd_indxtmp);
					(void)close(fd_indx);
					(void)unlink(MASTMP);
					(void)unlink(TMPINDEX);
					return(-1);
			}

			if (write(fd_indxtmp, mindxp, i_stbuf.st_size)  != i_stbuf.st_size) {
				(void)close(fd_mastmp);
				(void)close(fd_indxtmp);
				(void)close(fd_indx);
				(void)unlink(MASTMP);
				(void)unlink(TMPINDEX);
				(void)munmap(mindxp, i_stbuf.st_size);
				return(-1);
			}

			if ((munmap(mindxp, i_stbuf.st_size) < 0) || (close(fd_indx) < 0)) {
				(void)close(fd_mastmp);
				(void)close(fd_indxtmp);
				(void)unlink(MASTMP);
				(void)unlink(TMPINDEX);
				return(-1);
			}
		}

		if (write(fd_mastmp, mastp, msize) != msize) {
			(void)close(fd_mastmp);
			(void)close(fd_indxtmp);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if (close(fd_mastmp) < 0)
			return(-1);

		if (write(fd_indxtmp, indxp, sizeof(struct index)) != sizeof(struct index)) {
			(void)close(fd_indxtmp);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}
		
		if ((mqidxp = (struct index *)mmap(0, (i_stbuf.st_size + sizeof(struct index)), PROT_READ
		    |PROT_WRITE, MAP_SHARED, fd_indxtmp, 0)) < (struct index *) 0) {
				(void)close(fd_indxtmp);
				(void)unlink(MASTMP);
				(void)unlink(TMPINDEX);
				return(-1);
		}

		cnt = ((i_stbuf.st_size/sizeof(struct index)) + 1);

		qsort(mqidxp, cnt, sizeof(struct index), strcmp);

		if (munmap((caddr_t)mqidxp, i_stbuf.st_size + sizeof(struct index)) < 0) {
			(void)close(fd_indxtmp);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if (close(fd_indxtmp) < 0) {
			(void)unlink(MASTMP);
			return(-1);
		}
		
		if (access(OMASTER, 0) == 0) {
			if (unlink (OMASTER) < 0) {
				(void)unlink(MASTMP);
				(void)unlink(TMPINDEX);
				return(-1);
			}
		}

		if (rename(MASTER, OMASTER) == -1) {
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		_set_attr(MASTMP, &m_stbuf, m_lid);

		if (rename(MASTMP, MASTER) == -1) {
			(void)rename(OMASTER, MASTER);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		if (access(OINDEX, 0) == 0) {
			if (unlink(OINDEX) < 0) {
				(void)unlink(MASTMP);
				(void)unlink(TMPINDEX);
				return(-1);
			}
		}

		if (rename(INDEX, OINDEX) == -1) {
			(void)rename(OMASTER, MASTER);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		_set_attr(TMPINDEX, &i_stbuf, i_lid);

		if (rename(TMPINDEX, INDEX) == -1) {
			(void)rename(OINDEX, INDEX);
			(void)unlink(MASTMP);
			(void)unlink(TMPINDEX);
			return(-1);
		}

		return(0);
	}
}


static	void
_set_attr(filep, buf, lid)
	char	*filep;
	struct	stat	*buf;
	level_t	lid;
{
	if (lid) {
		(void) lvlfile(filep, MAC_SET, &lid);
	}
	(void) chmod(filep, (S_IRUSR | S_IRGRP | S_IWGRP));
	(void) chown(filep, buf->st_uid, buf->st_gid);
}
