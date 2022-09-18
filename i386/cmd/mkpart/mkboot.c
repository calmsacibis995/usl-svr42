/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mkpart:i386/cmd/mkpart/mkboot.c	1.1.1.3"
#ident "$Header: mkboot.c 1.1 91/05/29 $"

#include <stdio.h>
#include <sys/fcntl.h>
#include <a.out.h>
#include <sys/vtoc.h>
#include <sys/fdisk.h>
#include <libelf.h>
#include "mkpart.h"

extern	char	*bootfile;	/* bootstrap file	*/
extern	devstanza *mydev;	/* device stanza that we worked on 	*/
extern	struct	absio	absbuf; /* for RDABS and WRABS ioctl calls	*/
extern	struct	disk_parms dp;	/* device parameters	*/
extern	struct	ipart	*unix_part; /* fdisk partition table	*/
extern	struct	vtoc	vtoc;	/* table of contents	*/

static	int	bootfd;		/* boot file descriptor	*/
int	devfd;			/* device stanza file descriptor	*/
FILHDR	filehdr;		/* see filehdr.h	*/
AOUTHDR aouthdr;		/* see aouthdr.h	*/
SCNHDR	scnhdr;			/* see scnhdr.h		*/

/* structures to be used for the loading of an ELF bootable */
static	Elf	*elfd;		/* ELF file descriptor */
static  Elf32_Ehdr *ehdr;	/* ELF executable header structure */
static	Elf32_Phdr *ephdr, *phdr; /* ELF program header structure */
static	int	elfboot = 0;    /* flag to designate either ELF or COFF boot */

static	char	*read_elf_boot();
static	int	cpyboot();
static	void	buildlabel();
static	int	writelabel();
static	void	strnblk();

/*
 *
 * getboot ()
 *
 *	check for bootfile and reading it
 *
 * Parameters: none
 *
 * Return Values:
 *
 * 	true -> found a valid bootstrap a.out and read its filehdr and aouthdr.
 *	false-> initialized filehdr and aouthdr to 0 sections
 *		and 0 sized text, data, bss.
 *
 * Exit State:
 *
 *	30: opening/reading bootfile failed
 *	31: reading ELF-header failed
 *	
 */
int
getboot()
{
	int	len;

	if (bootfile) { /* if -B, overwrite boot filename gotten from stanza */
		mydev->ds_boot = bootfile;
	}

	if (!mydev->ds_boot || !mydev->ds_boot[0]) { /* check for null name */
		goto noboot;
	}
	if ((bootfd = open(mydev->ds_boot, O_RDONLY)) == -1 || 
	    (len = read(bootfd, (char *) & filehdr, FILHSZ)) != 0 && 
	    len != FILHSZ) {
		fprintf(stderr, "Opening/reading boot file ");
		perror(mydev->ds_boot);
		exit(30);
	} else if (!len) {	/* boot file was empty! */
noboot:
		filehdr.f_nscns = 0;
		aouthdr.tsize = aouthdr.dsize = aouthdr.bsize = 0;
		return 0;	/* no bootstrap found */
	}

	if (!ISCOFF(filehdr.f_magic)) {
		/* Not COFF boot, check if ELF Format */
		lseek(bootfd, 0, 0);
		if (elf_version (EV_CURRENT) == EV_NONE) {
			fprintf (stderr, "ELF access library out of date\n");
			exit (31);
		}
		if ((elfd = elf_begin (bootfd, ELF_C_READ, NULL)) == NULL) {
			fprintf (stderr, "can't elf_begin: %s\n", elf_errmsg (0));
			exit (31);
		}
		if ((ehdr = elf32_getehdr (elfd)) == NULL) {
			elf_end (elfd);
			fprintf(stderr, "Invalid Boot file, not ELF or COFF executable \n");
			exit(31);
		} else {
			elfboot = 1;
			return 1; /* ELF bootstrap ok */
		}
	}

	if (filehdr.f_opthdr > 0) {
		if (read(bootfd, (char *) & aouthdr, filehdr.f_opthdr) != 
		    filehdr.f_opthdr) {
			fprintf(stderr, "Reading optional header from boot file ");
			perror(mydev->ds_name->name);
			exit(30);
		}
	}

	return 1;	/* bootstrap looks ok */
}


/*
 *
 * read_elf_boot ()
 *
 * 	reads in the elf bootable into a buffer which is returned. Routine 
 *	primarily uses libelf calls to do elf specific actions.
 *
 * Parameters: none
 *
 * Return Values:
 *
 *	pointer to successful allocated buffer
 *
 * Exit State:
 *
 *	31: reading ELF-header/segment failed
 *	
 */

static	char	*
read_elf_boot()
{
	char	*buf;
	int	i;
	Elf_Scn * scn;
	Elf32_Shdr * eshdr;

	if ((ephdr = elf32_getphdr (elfd)) == NULL) {
		fprintf (stderr, "can't get ELF program header: %s\n", elf_errmsg (0));
		exit(31);
	}

	for (i = 0; i < (int)ehdr->e_phnum; i++)
		if (ephdr[i].p_type == PT_LOAD && ephdr[i].p_filesz > 0)
			break;
	if (i >= (int)ehdr->e_phnum) {
		fprintf (stderr, "can't find loadable ELF segment\n");
		exit(31);
	}
	if ((buf = (char *)malloc(((ephdr->p_filesz + dp.dp_secsiz - 1) / dp.dp_secsiz)
	    *dp.dp_secsiz)) == NULL) {
		fprintf (stderr, "can't all allocate boot buffer\n");
		exit(31);
	}
	phdr = &ephdr[i];
	for (scn = NULL; scn = elf_nextscn (elfd, scn); ) {
		if ((eshdr = elf32_getshdr (scn)) == NULL) {
			free (buf);
			printf ("Invalid boot, empty segment\n");
			exit(31);
		}
		if (eshdr->sh_addr >= phdr->p_vaddr && 
		    eshdr->sh_addr < phdr->p_vaddr + phdr->p_filesz && 
		    eshdr->sh_type == SHT_PROGBITS && 
		    eshdr->sh_flags & SHF_ALLOC) {
			int	nbytes;
			Elf_Data * data;

			if ((data = elf_getdata (scn, NULL)) == NULL || 
			    data->d_buf == NULL) {
				free (buf);
				fprintf (stderr, "Invalid boot, empty segment\n");
				exit(31);
			}
			nbytes = eshdr->sh_size;
			if (eshdr->sh_addr + eshdr->sh_size > phdr->p_vaddr + 
			    phdr->p_filesz)
				nbytes -= eshdr->sh_addr + eshdr->sh_size - 
				    phdr->p_vaddr - phdr->p_filesz;
			memcpy (&buf[eshdr->sh_addr - phdr->p_vaddr], 
			    (char *) data->d_buf, nbytes);
		}
	}
	return (buf);
}


/*
 *
 * writeboot ()
 *
 *	writes the bootstrap code and the current volume label out to the disk
 *
 * Parameters: none
 *
 * Return Values: none
 *
 * Exit State:
 *
 *	40: seeking/reading section header failed
 *	41: seeking section failed
 *	43: writing boot to disk failed
 *	45: allocation of boot buffer failed
 *
 * Description:
 *
 * 	Writes the bootstrap code and the current volume label out to the disk
 * 	(the modified volume label is updated later in writevtoc(); this 
 *	separation supports -b).  The volume label appears in the middle of 
 *	the bootstrap code; it appears at sector VLAB_SECT, offset by 
 *	VLAB_START.  We merely open a hole in the bootstrap for the label, but 
 *	the bootstrap must take this hole into account and do something like 
 *	jump around it (this must be a pretty clever trick for the bootstrap 
 *	coder, but Intel likes it this way )-: ).  We guarantee that bss is 
 *	initialized to 0, but Intel's old bootstrap doesn't assume that.
 *
 */

void
writeboot()
{
	char	*buf;
	char	*p;
	daddr_t isecp;
	long	len;
	ushort          secno = 0;
	long	blockno;
	int	i;
	struct	partition *pp;
	int	rootfound = 0;

	if (elfboot)
		buf = read_elf_boot();
	else { /* COFF bootable will be read into buf */
		/* get a buffer for the whole bootstrap and label */
		/* rif -- the bootstrap can be no bigger than HDPDLOC sectors */

		if ((buf = malloc(HDPDLOC * dp.dp_secsiz)) == NULL)
		{
			fprintf(stderr, "Cannot malloc boot buffer\n");
			exit(45);
		}
		p = buf;	/* p will walk thru buf, where data is read */
		/* isecp will point at scnhdr structs in a.out */
		isecp = FILHSZ + filehdr.f_opthdr;

		/*
	 	* Loop for each section in the a.out.  Lseek and read the boot
	 	* section header.  Subloop to read all of section into buf.
	 	*/
		for (; secno < filehdr.f_nscns; (isecp += SCNHSZ), secno++) {
			/* seek and read section header */
			if ((lseek(bootfd, isecp, 0) == -1) || 
			    (read(bootfd, &scnhdr, SCNHSZ) != SCNHSZ)) {
				fprintf(stderr, "Seeking/reading section header %d ", secno);
				perror(mydev->ds_boot);
				exit(40);
			}
			/* seek start of section */
			if (lseek(bootfd, scnhdr.s_scnptr, 0) == -1) {
				fprintf(stderr, "Seeking section %d ", secno);
				perror(mydev->ds_boot);
				exit(41);
			}
			/* read section */
			for (blockno = 0; len = readbootblock(blockno, p); blockno++) {
				p += len;	/* advance buffer pointer */
				/*
			 	* The reading loop terminates if we tried to read a
			 	* block and it had zero length, or if the current
			 	* block was short.
			 	*/
				if (len != dp.dp_secsiz) 
					break;
			}
		}
	}

	/*
	 * Write out the boot.
	 */

	/*
	 * Our boot track goes where the fdisk partition table says it
	 * should go.
	 */

	/* If we don't have a root partition, don't write the boot. */

	for (pp = vtoc.v_part; pp < &vtoc.v_part[vtoc.v_nparts]; pp++) {
		if (pp->p_tag == V_ROOT && (pp->p_flag & V_VALID)) {
			rootfound = 1;
			break;
		}
	}

	if (!rootfound)
		return;

	/* round length of boot to a sector boundary */
	if (elfboot)
		len = (((phdr->p_filesz) + (dp.dp_secsiz - 1)) / dp.dp_secsiz) * dp.dp_secsiz;
	else
		len = ((long) ((p - buf) + (dp.dp_secsiz - 1)) / dp.dp_secsiz) * dp.dp_secsiz;

	for (i = 0; i < len / 512; i++) {
		absbuf.abs_sec = unix_part->relsect + i;
		absbuf.abs_buf = (buf + (i * 512));
		if (ioctl(devfd, V_WRABS, &absbuf) != 0) {
			perror("WRITE of boot");
			exit(43);
		}

	}
	return;

noboot:
	fprintf(stderr, "Run fdisk to create a UNIX System partition in the ");
	fprintf(stderr, "proper cylinders and\nRe-run mkpart with the -b flag.\n");
	return;
}


/*
 *
 * readbootblock ( blockno, buf)
 *
 *	reading the boot section
 *
 * Parameters:
 *
 *	blockno: block number in the current section
 *	buf:	 buffer block
 *
 * Return Values: 
 *	
 *	length read for the block
 *
 * Exit State:
 *
 *	42: reading of the block failed
 *
 * Readbootblock ( block # in current section, buffer pointer )
 * returns length read for this block.
 */

int
readbootblock(blockno, buf)
long	blockno;
char	*buf;
{
	int	len;

	/* calculate length to read */
	if (blockno < (long )scnhdr.s_size / dp.dp_secsiz) {
		len = dp.dp_secsiz;
	} else {
		len = (int) scnhdr.s_size % dp.dp_secsiz;
	}

	/*
	 * If the section type is text or data, read real data from the file.
	 * If the section is bss, just return len.  If the section
	 * is some other kind, don't read anything and report 0 length;  this
	 * will advance us to the next section.
	 */
	if (scnhdr.s_flags & (STYP_TEXT | STYP_DATA)) {
		if (read(bootfd, buf, len) != len) {
			fprintf(stderr, "Reading section, block number %ld ", blockno);
			perror(mydev->ds_boot);
			exit(42);
		}
	} else 
		len = 0;
	return len;
}
