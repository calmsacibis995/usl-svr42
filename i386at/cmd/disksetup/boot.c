/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ident	"@(#)disksetup:i386at/cmd/disksetup/boot.c	1.3.1.2"
#ident "@(#) (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990"

/* This file contains the AT spceific routines to load the boot routine for */
/* a UNIX Sys V Rel. 4.0 partition. The hard disk boot routine is loaded in */
/* the first 28 sectors of the boot slice (which is the first 28 sectors of */
/* the UNIX partition. The loader can load COFF/ ELF format boot routines.  */
/* Or the binary boot program used by the high performance boot program.    */

#include <a.out.h>
#include <fcntl.h>
#include <libelf.h>
#include <malloc.h>
#include <sys/vtoc.h>
#include <sys/bootinfo.h>
#include <sys/bootcntl.h>
#include <stdio.h>

/* structures to be used for the loading boot */
extern  struct	absio  absio;
extern  struct	disk_parms dp;
extern  daddr_t unix_base;
extern  int 	bootfd;
extern  int 	diskfd;
Elf		*elfd;		/* ELF file descriptor */
Elf32_Ehdr	*ehdr;		/* ELF executable header structure */
Elf32_Phdr	*ephdr, *phdr;	/* ELF program header structure */
int		elfboot = 0;    /* flag to designate either ELF or COFF boot */
FILHDR		filehdr;	/* see filehdr.h */
AOUTHDR		aouthdr;	/* see aouthdr.h */
SCNHDR		scnhdr;		/* see scnhdr.h */

char		*rd_hpboot();

/* READ_ELF_BOOT reads in the elf bootable into a buffer which is returned. */
/* Routine primarily uses libelf calls to do elf specific actions.          */
char *
read_elf_boot()
{
	char *buf;
	int  i;
	Elf_Scn *scn;
	Elf32_Shdr *eshdr;

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
	if ((buf = malloc(((ephdr->p_filesz+dp.dp_secsiz-1)/dp.dp_secsiz)
		*dp.dp_secsiz)) == NULL) {
		fprintf (stderr, "can't all allocate boot buffer\n");
		exit(31);
	}
	phdr = &ephdr[i];
	for (scn = NULL; scn = elf_nextscn(elfd, scn); ) {
		if ((eshdr = elf32_getshdr (scn)) == NULL) {
			free (buf);
			printf ("Invalid boot, empty segment\n");
			exit(31);
		}
		if (eshdr->sh_addr >= phdr->p_vaddr && 
		    eshdr->sh_addr < phdr->p_vaddr + phdr->p_filesz &&
		    eshdr->sh_type == SHT_PROGBITS &&
		    eshdr->sh_flags & SHF_ALLOC) {
			int nbytes;
			Elf_Data *data;

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
 * LOADBOOT()
 * First step is to determine if boot is ELF or COFF, if valid boot read
 * into buf using read_elf_boot or readbootblock. Next step is to
 * write the bottstrap code and the current volume label out to the disk.
 * The volume label appears in the middle of the bootstrap code;
 * it appears at sector VLAB_SECT, offset by VLAB_START.  We guarantee that
 * bss is initialized to 0, but Intel's old bootstrap doesn't assume that.
 */
loadboot()
{
	char		*p, *buf;
	daddr_t         isecp;
	int             i, secno = 0;
	long            len, blockno;
	int		hpboot = 0;
	struct		bootcntl hpbootcntl;

	if ((len = read(bootfd, (char *)&filehdr, FILHSZ)) != FILHSZ) {
		fprintf(stderr,"Can't read boot file header.\n");
		exit(30);
	}
	i = sizeof(struct bootcntl);
	lseek(bootfd, 512, 0);
/*	get the boot control block whcih starts at the second 
 *	sector of the boot program
 */
	if ((len = read(bootfd, (char *)&hpbootcntl, i)) != i) {
		fprintf(stderr,"Can't read boot file header.\n");
		exit(30);
	}

	if (ISCOFF(filehdr.f_magic)) {
		if (filehdr.f_opthdr > 0) {
			if (read(bootfd,(char *)&aouthdr,filehdr.f_opthdr) !=
			   filehdr.f_opthdr) {
				fprintf(stderr,"Error reading COFF boot file header.\n");
				exit(30);
			}
		}
	}
/*	check for high performance boot program				*/
	else if (hpbootcntl.bc_magic == BPRG_MAGIC) 
		hpboot++;
	else {
		/* Not COFF boot, check if ELF Format */
		lseek(bootfd, 0, 0);
		if (elf_version (EV_CURRENT) == EV_NONE) {
			fprintf(stderr,"ELF access library out of date\n");
			exit (31);
		}
		if ((elfd = elf_begin(bootfd, ELF_C_READ, NULL)) == NULL) {
			fprintf(stderr,"ELF_begin failure: %s\n",elf_errmsg(0));
			exit (31);
		}
		if ((ehdr = elf32_getehdr (elfd)) == NULL) {
			elf_end (elfd);
			fprintf(stderr,"Invalid Boot file, not ELF or COFF executable \n");
			exit(31);
		}
		else 
			elfboot = 1;
	}
	/* Now read verified boot into buf */
	if (hpboot)
		buf = (char *)rd_hpboot(&len);
	else if (elfboot)
		buf = (char *)read_elf_boot();
	else {  /* COFF bootable will be read into buf */
		/* get a buffer for the whole bootstrap and label */
		/* the bootstrap can be no bigger than VTOC_SEC sectors */

		if ((buf = malloc(VTOC_SEC*dp.dp_secsiz)) == NULL) {
			fprintf(stderr,"Cannot malloc boot buffer\n");
			exit(45);
		}
		p = buf;	/* p will walk thru buf, where data is read */
		/* isecp will point at scnhdr structs in a.out */
		isecp = FILHSZ + filehdr.f_opthdr;

		/*
	 	* Loop for each section in the a.out.  Lseek and read the boot
	 	* section header.  Subloop to read all of section into buf.
	 	*/
		for (; secno < (int)filehdr.f_nscns; (isecp += SCNHSZ), secno++) {
			/* seek and read section header */
			if ((lseek(bootfd,isecp,0) == -1) ||
			   (read(bootfd,&scnhdr,SCNHSZ) != SCNHSZ)) {
				fprintf(stderr,"Seeking/reading section header %d ",secno);
				exit(40);
			}
			/* seek start of section */
			if (lseek(bootfd,scnhdr.s_scnptr,0) == -1) {
				fprintf(stderr,"Seeking section %d ",secno);
				exit(41);
			}
			/* The reading loop terminates if we tried to read a */
			/* block and it had zero length, or if the current */
			/* block was short. */
			for (blockno=0;len=readbootblock(blockno,p);blockno++) {
				p += len;	/* advance buffer pointer */
				if (len != dp.dp_secsiz) 
					break;
			}
		}
	}
	/* round length of boot to a sector boundary 			*/
	if (hpboot)
		;	/* always end at sector boundary		*/
	else if (elfboot)
		len = (((phdr->p_filesz)+(dp.dp_secsiz-1))/dp.dp_secsiz)*dp.dp_secsiz;
	else
		len = ((daddr_t)((p-buf)+(dp.dp_secsiz-1))/(daddr_t)dp.dp_secsiz)*(daddr_t)dp.dp_secsiz;

	/* Write out the boot. at beginning of unix partition */
	set_sig_off();
	for(i=0; i < len/512; i++){
		absio.abs_sec = unix_base + i;
		absio.abs_buf = (buf + (i * 512));
		if(ioctl(diskfd, V_WRABS, &absio) != 0){
			fprintf(stderr,"Error writing boot to disk! Successful completion is\n");
			fprintf(stderr,"required to allow boot from hard disk!\n");
			exit(43);
	    	}
	}
	set_sig_on();
}

/* * Readbootblock (block # in current section, buffer pointer) returns       */
/* length read for this block.						      */
int
readbootblock(blockno,buf)
long    blockno;
char    *buf;
{
	int	len;

	/* calculate length to read */
	if (blockno < scnhdr.s_size/(long)dp.dp_secsiz) 
		len = dp.dp_secsiz;
	else 
		len = scnhdr.s_size%(long)dp.dp_secsiz;

	/* If the section type is text or data, read data from the file. If */
	/* the section is bss, return len. If the section is some other type */
	/* don't read anything and report 0 length to advance ptr to next sect*/
	if (scnhdr.s_flags & (STYP_TEXT|STYP_DATA)) 
		if (read(bootfd,buf,len) != len) {
			fprintf(stderr,"Error reading COFF boot block number %ld \n",blockno);
			exit(42);
		}
	else 
		len = 0;
	return len;
}

/*
 * 	Loading the high performance boot program into memory
 *	When the boot program was built, It has been roundup to the
 *	next sector boundary.
 */
char *
rd_hpboot(filszp)
int	*filszp;
{
	char	*buf;
	int	sz = VTOC_SEC*dp.dp_secsiz;
	
/*	allocate a buffer for the maximum boot program size		*/
	if ((buf = malloc(sz)) == NULL) {
		fprintf(stderr,"Cannot malloc boot buffer\n");
		exit(45);
	}
	lseek(bootfd,0,0);
	*filszp = read(bootfd,buf,sz);
/*	check for zero file size or missing bytes if not align to sector
 *	boundary
 */
	if (!(*filszp) || (*filszp % (int)dp.dp_secsiz)) {
		fprintf(stderr, "Error reading boot program\n");
		exit(42);
	}
	return(buf);
}
