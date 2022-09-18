/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)uts-x86at:boot/at386/tool/tdxtract.c	1.3"
#ident	"$Header: $"

#include <stdio.h>
#include <libelf.h>
#include <fcntl.h>

extern void elf_eoj();

main (argc, argv)
int argc;
char *argv[];
{
	Elf *elf;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	unsigned char *buf;
	char *cmd;
	char *infile, *ofile;
	int fd, fdout;
	unsigned int size;
	int kind;
	unsigned short pnum;

	cmd = argv[0];
	infile = argv[1];
	ofile = argv[2];

	if (argc != 3) {
		fprintf(stderr, "usage: %s infile outfile\n", cmd);
		exit (1);
	}

	if ((elf_version(EV_CURRENT)) == EV_NONE) {
		fprintf(stderr, "%s: ELF Access library out of date\n", cmd);
		exit (1);
	}

	if ((fd = open(infile, O_RDONLY)) == -1) {
		perror(infile);
		exit (1);
	}

	if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		fprintf(stderr, "%s: Can't Elf Begin %s (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(NULL, fd, 1);
	}

	if ((ehdr = elf32_getehdr(elf)) == NULL) {
		fprintf(stderr, "%s: (%s) Can't get Elf Header (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(elf, fd, 1);
	}

	if (((kind = elf_kind(elf)) != ELF_K_ELF) &&
			(kind != ELF_K_COFF)) {
		fprintf(stderr, "%s: %s not a valid binary file\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((phdr = elf32_getphdr(elf)) == NULL) {
		fprintf(stderr, "%s: Can get Program Header for %s (%s)\n",
			cmd, infile, elf_errmsg(-1));
		elf_eoj(elf, fd, 1);
	}

	for (pnum = 0; pnum < ehdr->e_phnum; ++pnum) {
		if ((phdr->p_type == PT_LOAD) &&
			(phdr->p_filesz != 0))
				break;
		++phdr;
	}

	if (pnum >= ehdr->e_phnum) {
		fprintf(stderr, "%s: unable to find program header for %s\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	size = phdr->p_filesz;

	size = (((size - 1) / 512) + 1) * 512;

	if ((buf = (unsigned char *)malloc(size)) == NULL) {
		fprintf(stderr, "%s: unable to malloc space for buffer\n",
			cmd);
		elf_eoj(elf, fd, 1);
	}

	(void)memset(buf, 0, size);

	if ((lseek(fd, phdr->p_offset, 0L)) == -1L) {
		fprintf(stderr, "%s: seek error on %s\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((read(fd, buf, phdr->p_filesz)) != phdr->p_filesz) {
		fprintf(stderr, "%s: read error on %s\n",
			cmd, infile);
		elf_eoj(elf, fd, 1);
	}

	if ((fdout = open(ofile, O_CREAT | O_RDWR, 0777)) == -1) {
		fprintf(stderr, "%s: Cannot open %s\n",
			cmd, ofile);
		elf_eoj(elf, fd, 1);
	}

	if ((write(fdout, buf, size)) != size) {
		fprintf(stderr, "%s: Cannot write %s\n",
			cmd, ofile);
		elf_eoj(elf, fd, 1);
	}

	(void)close(fdout);

	elf_eoj(elf, fd, 0);
}

void
elf_eoj(e, fd, x)
Elf *e;
int fd;
int x;
{
	if (e)
		(void)elf_end(e);
	if (fd != -1)
		(void)close(fd);
	exit (x);
}
