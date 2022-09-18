#ident	"@(#)debugger:libutil/common/dump_raw.C	1.2"

#include "Interface.h"
#include "utility.h"
#include "LWP.h"
#include "Parser.h"
#include "Proglist.h"
#include "Symbol.h"
#include "global.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <signal.h>

#ifdef MULTIBYTE
#include <libw.h>
#endif

// dump file contents in hexadecimal and ASCII

#define LSZB	16		// bytes per line 
#define LSZW	(LSZB/sizeof(int)) // words per line
#define NCPW	(sizeof(int)*2)	   // chars per word printed as hex 
#define NCPB	(sizeof(char)*2)   // chars per byte printed as hex 

// determine target byte order
#define UNK 0
#define LSB 1
#define MSB 2

enum
{
	W_L0, W_L1, W_L2, W_L3
};

enum
{
	W_M3, W_M2, W_M1, W_M0,
	W_sizeof
};

static int
get_order()
{
	union {
		unsigned long	w;
		unsigned char	c[W_sizeof];
	} u;
	u.w = 0x10203;
	if  ((((((((u.c)[W_L3]<<8)
		+(u.c)[W_L2])<<8)
		+(u.c)[W_L1])<<8)
		+(u.c)[W_L0]) == 0x10203)
		return LSB;
	else if ((((((((u.c)[W_M3]<<8)
		+(u.c)[W_M2])<<8)
		+(u.c)[W_M1])<<8)
		+(u.c)[W_M0]) == 0x10203)
		return MSB;
	else
		return UNK;
}

int
dump_raw(Proclist *procl, Location *location, int cnt)
{
	static		int byte_order = UNK;
	unsigned char	*save;
	unsigned char	*cur_line, *next_line;
	unsigned char	*cur_byte;
	unsigned int	*word_ptr;
	int		bcount, wcount;
	int		n;
	int		first, count, diff;
	Iaddr		addr, oaddr;
	int 		single = 1;
	int		ret = 1;
	LWP		*lwp;
	plist		*list;
	char		*cur;
#ifdef MULTIBYTE
	int		is_print = 0;
	int		clen = 0;
	unsigned char	*cur_char;
	wchar_t		wchar;
	char		buf[(5*NCPW)+(MB_LEN_MAX*LSZB)+MB_LEN_MAX+10];
#else
	char		buf[(5*NCPW)+(NCPB*LSZB)+10];
#endif


	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl);
		lwp = list++->p_lwp;
	}
	else
	{
		lwp = proglist.current_lwp();
	}
	if (!lwp)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	if (byte_order == UNK)
	{
		if ((byte_order = get_order()) == UNK)
		{
			printe(ERR_byte_order, E_ERROR);
			return 0;
		}
	}
	// possibly bigger than needed, but okay for 
	// max unaligned address
	save = new(unsigned char[cnt + 16]);

	sigrelse(SIGINT);
	do
	{
		Symbol	func;

		if (interrupt & sigbit(SIGINT))
			break;
		if (!lwp->state_check(E_RUNNING|E_DEAD))
		{
			ret = 0;
			continue;
		}
		printm(MSG_raw_dump_header, lwp->lwp_name(),
			lwp->prog_name());
		first = 1;
		count = cnt;
		if ( get_addr( lwp, location, oaddr, st_notags, func ) == 0 )
		{
			ret = 0;
			continue;
		}
		// truncate addr to 16 byte boundary
		addr = oaddr & 0xFFFFFFF0;
		diff = (int)(oaddr - addr);
		cur_line = save;
#ifdef MULTIBYTE
		cur_char = cur_byte = save;
#endif
		for (int i = 0; i < diff; i++)
			*cur_line++ = '\0';
		if ( lwp->read(oaddr, count, (char *)cur_line) == 0)
		{
			ret = 0;
			continue;
		}
		count += diff;
		next_line = save;

		while (count > 0) 
		{
			if (interrupt & sigbit(SIGINT))
				break;
			cur_line = next_line;
			cur = buf;
			// # of bytes for this line
			if (count < LSZB)
				bcount = count;
			else
				bcount = LSZB;

			word_ptr = (unsigned int *)cur_line;
			next_line += bcount;
			count -= bcount;
			cur += sprintf(cur, "%8lx:", 
				first ? oaddr : addr);
			addr += bcount;
			// # of words for this line
			wcount = (bcount + sizeof(int)-1)/sizeof(int);

			if (first)
			{
				// first line of output
				// if we do not start on a 16-byte
				// boundary, print ".", until the
				// starting point
				first = 0;
				for (n = 0; n < diff; n++ )
				{
					if (!(n % sizeof(int)))
					{
						// word boundary
						*cur++ = ' ';
					}
					*cur++ = '.';
					*cur++ = '.';
				}
				int j = n % sizeof(int);
				// may be starting in middle of a word
				// if so, must pass printf the
				// correct address for current byte
				// order - low order byte if (little
				// endian), else high order byte
				if (j)
				{
					int	h = sizeof(int) - j;
					int	k;
					if (byte_order == LSB)
						// least sig byte
						k = n + h - 1;
					else 
						k = n;
					for (; j < sizeof(int); j++)
					{
						cur += sprintf(cur,"%*.*x",NCPB,NCPB, *(cur_line + k));
						if (byte_order == LSB)
						{
							k--;
						}
						else
						{
							k++;
						}
					}
					n += h;
				}
				n = n / sizeof(int);
			}
			else
			{
				n = 0;
			}
			for (; n < LSZW; ++n)
			{
				if (n < wcount)
					cur += sprintf(cur," %*.*x", NCPW, NCPW, *(word_ptr + n));
				else
					cur += sprintf(cur," %*.*s",NCPW, NCPW, "");
			}
			*cur++ = ' ';
			*cur++ = ' ';
#ifdef MULTIBYTE
			// handle multibyte characters:
			// characters may cross over a line;
			// if they do, print '>' for each byte
			// left at end of line and print multibyte
			// char on next line
			//
			// first handle left-over from previous line
			if (cur_char < cur_line)
			{
				if (is_print)
				{
					// we have a printable char from 
					// last_line
					memcpy(cur, cur_char, clen);
					is_print = 0;
					cur += clen;
				 	cur_byte += clen;
				}
				else
				{
					while(clen--)
					{
						*cur++ = *cur_byte++;
					}
				}
			}
			// print out current line
			while(cur_byte < next_line)
			{
				int	left_in_line = next_line - cur_byte;
				clen = mbtowc(&wchar, cur_char, MB_CUR_MAX);
				if (clen <= 0)
				{
					// NULL or not multibyte char
					*cur++ = '.';
					cur_byte++;
					cur_char++;
					continue;
				}
				if ((is_print = wisprint(wchar)) == 0)
				{
					// non printing multibyte
					while(clen && left_in_line)
					{
						clen--; left_in_line--;
						*cur++ = '.';
						cur_byte++;
					}
					if (!clen)
						cur_char = cur_byte;
					continue;
				}
				// printable multibyte
				if (clen > left_in_line)
				{
					*cur++ = '\t';
					while(left_in_line--)
						*cur++ = '>';
					break;
				}
				memcpy(cur, cur_char, clen);
				cur_byte += clen;
				cur_char = cur_byte;
				cur += clen;
			}
#else
			// print out ASCII
			cur_byte = cur_line;
			for (n = 0; n < bcount; ++n, ++cur_byte)
				if (isprint(*cur_byte))
					*cur++ = *cur_byte;
				else
					*cur++ = '.';
#endif
			*cur = 0;
			printm(MSG_raw_dump, buf);
		}
		if (!single)
			printm(MSG_newline);
	}
	while(!single && ((lwp = list++->p_lwp) != 0));

	sighold(SIGINT);
	delete save;
	return ret;
}
