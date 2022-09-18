/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libcmd:common/lib/libcmd/magic.c	1.2.3.2"
/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */


#include <stdio.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>

/*
**	Types
*/

#define	BYTE	0
#define	SHORT	2
#define	LONG	4
#define	STR	8

/*
**	Opcodes
*/

#define	EQ	0
#define	GT	1
#define	LT	2
#define	STRC	3	/* string compare */
#define	ANY	4
#define AND	5
#define NSET	6	/* True if bit is not set */
#define	SUB	64	/* or'ed in, SUBstitution string, for example %ld, %s, %lo */
			/* mask: with bit 6 on, used to locate print formats */
/*
**	Misc
*/

#define	BSZ	128
#define	NENT	200 
#define	reg	register

/*
**	Structure of magic file entry
*/

struct	entry	{
	char	e_level;	/* 0 or 1 */
	long	e_off;		/* in bytes */
	char	e_type;		/* BYTE, SHORT, LONG, STR */
	char	e_opcode;	/* EQ, GT, LT, ANY, AND, NSET */
	union	{
		long	num;
		char	*str;
	}	e_value;
	char	*e_msgid;
	char	*e_str;
};

typedef	struct entry	Entry;
static Entry	*mtab;

char	*calloc();
char	*malloc();
char	*realloc();
long	atolo();

static const char
	fmterror[] = "uxlibc:82:Fmt error, no tab after %son line %d\n",
	nomem[] = "uxlibc:83:Not enough memory for magic table: %s\n";

static prf(x)
char *x;
{
	if (pfmt(stdout, MM_NOSTD, "uxlibc:84:%s:\t", x) < 9)
		printf("\t");
}

static char *
get_str(ep)
	Entry *ep;
{
	if (ep->e_msgid)
		return gettxt(ep->e_msgid, ep->e_str);
	else
		return ep->e_str;
}
		
int
mkmtab(magfile, cflg)
char	*magfile;
reg	int	cflg;
{
	reg	Entry	*ep;
	reg	FILE	*fp;
	reg	int	lcnt = 0;
	auto	char	buf[BSZ];
	auto	Entry	*mend;

	ep = (Entry *) calloc(sizeof(Entry), NENT);
	if(ep == NULL) {
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		return(-1);
	}
	mtab = ep;
	mend = &mtab[NENT];
	fp = fopen(magfile, "r");
	if(fp == NULL) {
		pfmt(stderr, MM_ERROR, "uxlibc:85:Cannot open magic file <%s>: %s\n",
			magfile, strerror(errno));
		return(-1);
	}
	while(fgets(buf, BSZ, fp) != NULL) {
		reg	char	*p = buf;
		reg	char	*p2, *p3;
		reg	char	opc;

		if(*p == '\n' || *p == '#')
			continue;
		lcnt++;
			

			/* LEVEL */
		if(*p == '>') {
			ep->e_level = 1;
			p++;
		}
			/* OFFSET */
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				pfmt(stderr, MM_ERROR, fmterror, p, lcnt);
			continue;
		}
		*p2++ = NULL;
		ep->e_off = atolo(p);
		while(*p2 == '\t')
			p2++;
			/* TYPE */
		p = p2;
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				pfmt(stderr, MM_ERROR, fmterror, p, lcnt);
			continue;
		}
		*p2++ = NULL;
		if(*p == 's') {
			if(*(p+1) == 'h')
				ep->e_type = SHORT;
			else
				ep->e_type = STR;
		} else if (*p == 'l')
			ep->e_type = LONG;
		while(*p2 == '\t')
			p2++;
			/* OP-VALUE */
		p = p2;
		p2 = strchr(p, '\t');
		if(p2 == NULL) {
			if(cflg)
				pfmt(stderr, MM_ERROR, fmterror, p, lcnt);
			continue;
		}
		*p2++ = NULL;
		if(ep->e_type != STR) {
			opc = *p++;
			switch(opc) {
			case '=':
				ep->e_opcode = EQ;
				break;

			case '>':
				ep->e_opcode = GT;
				break;

			case '<':
				ep->e_opcode = LT;
				break;

			case 'x':
				ep->e_opcode = ANY;
				break;

			case '&':
				ep->e_opcode = AND;
				break;

			case '^':
				ep->e_opcode = NSET;
				break;
			default:		/* EQ (i.e. 0) is default	*/
				p--;		/* since global ep->e_opcode=0	*/
			}
		}
		if(ep->e_opcode != ANY) {
			if(ep->e_type != STR)
				ep->e_value.num = atolo(p);
			else if ((ep->e_value.str = strdup(p)) == NULL) {
				pfmt(stderr, MM_ERROR, nomem, strerror(errno));
			 	return(-1);
			}
		}
		p2 += strspn(p2, "\t");
			/* STRING or MSGID */
		if ((p3 = strchr(p2, '\t')) != NULL){
			/* MSGID */
			*p3++ = '\0';
			p3 += strspn(p3, "\t");
			if ((ep->e_msgid = strdup(p2)) == NULL){
				pfmt(stderr, MM_ERROR, nomem, strerror(errno));
				return -1;
			}
			p2 = p3;
		}
			/* STRING */
		if ((ep->e_str = strdup(p2)) == NULL) {
			pfmt(stderr, MM_ERROR, nomem, strerror(errno));
			return(-1);
		}
		else {
			if ((p = strchr(ep->e_str, '\n')) != NULL)
				*p = '\0';
			if (strchr(ep->e_str, '%') != NULL)
				ep->e_opcode |= SUB;
		}
		ep++;
		if(ep >= mend) {
			unsigned int tbsize, oldsize;
			
			oldsize = mend - mtab;  /* off by one? */
			tbsize = (NENT + oldsize) * sizeof(Entry);
			if ((mtab = (Entry *) realloc((char *) mtab, tbsize))
				== NULL) {
				pfmt(stderr, MM_ERROR, nomem, strerror(errno));
				return(-1);
			}
			else {
				memset(mtab + oldsize, 0, sizeof(Entry) * NENT);
				mend = &mtab[tbsize / sizeof(Entry)];
				ep = &mtab[oldsize];  
			}
		}
	}
	ep->e_off = -1L;
	if (fclose(fp) == EOF) {
		pfmt(stderr, MM_ERROR, "uxlibc:86:Cannot close %s: %s\n",
			magfile, strerror(errno));
		return(-1);
	}
	mtab = (Entry *)realloc((char *)mtab, (1 + ep - mtab) * sizeof(Entry));
	if (mtab == NULL) {
		pfmt(stderr, MM_ERROR, nomem, strerror(errno));
		return(-1);
	}
	return(0);
}

static long
atolo(s)		/* determine read format and get e_value.num */
reg	char	*s;
{
	reg	char	*fmt = "%ld";
	auto	long	j = 0L;

	if(*s == '0') {
		s++;
		if(*s == 'x') {
			s++;
			fmt = "%lx";
		} else
			fmt = "%lo";
	}
	sscanf(s, fmt, &j);
	return(j);
}

int
ckmtab(buf, bufsize, silent)	/* Check for Magic Table entries in the file */
char *buf;
int bufsize, silent;
{

	unsigned short svar;
	reg	Entry	*ep;
	reg	char	*p;
	reg	int	lev1 = 0;
	auto	union	{
		long	l;
		char	ch[4];
		}	val, revval;

	for(ep = mtab; ep->e_off != -1L; ep++) {  /* -1 offset marks end of */
		if(lev1) {			/* valid magic file entries */
			if(ep->e_level != 1)
				break;
		} else if(ep->e_level == 1)
			continue;
		if (ep->e_off > (long) bufsize)
			continue;
		p = &buf[ep->e_off];
		switch(ep->e_type) {
		case STR:
		{
			if(strncmp(p,ep->e_value.str,strlen(ep->e_value.str)))
				continue;
			if (!silent) {
				if(ep->e_opcode & SUB)
					printf(get_str(ep), ep->e_value.str);
				else
					printf(get_str(ep));
			}
			lev1 = 1;
		}

		case BYTE:
			val.l = (long)(*(unsigned char *) p);
			break;

		case SHORT:
			/* 
			 * This code is modified to avoid word alignment problem 
			 * which caused command "more" to core dump on a 3b2 machine 
			 * when the  word pointed to by p is not aleast halfword 
			 * aligned.
			 */
			memcpy(&svar, p, sizeof(unsigned short));
			val.l = svar;
			break;

		case LONG:
			/* This code is modified to avoid word alignment problem */
			memcpy(&val.l, p, sizeof(long));
			break;
		}
		switch(ep->e_opcode & ~SUB) {
		case EQ:
			if(val.l != ep->e_value.num)
				continue;
			break;
		case GT:
			if(val.l <= ep->e_value.num)
				continue;
			break;

		case LT:
			if(val.l >= ep->e_value.num)
				continue;
			break;

		case AND:
			if((val.l & ep->e_value.num) == ep->e_value.num)
				break;
			continue;
		case NSET:
			if((val.l & ep->e_value.num) != ep->e_value.num)
				break;
			continue;
		}
		if(lev1 && !silent)
			putchar(' ');
		if (!silent) {
			if(ep->e_opcode & SUB)
				printf(get_str(ep), val.l);
			else
				printf(get_str(ep));
		}
		lev1 = 1;
	}
	if(lev1) {
		if (!silent)
			putchar('\n');
		return(1 + ep - mtab);
	}
	return(0);
}

void
prtmtab()
{
	reg	Entry	*ep;

	pfmt(stdout, MM_NOSTD, "uxlibc:87:level	off	type	opcode	value	string\n");
	for(ep = mtab; ep->e_off != -1L; ep++) {
		printf("%d\t%d\t%d\t%d\t", ep->e_level, ep->e_off,
			ep->e_type, ep->e_opcode);
		if(ep->e_type == STR)
			printf("%s\t", ep->e_value.str);
		else
			printf("%lo\t", ep->e_value.num);
		printf("%s", get_str(ep));
		if(ep->e_opcode & SUB)
			printf("\tsubst");
		printf("\n");
	}
}
