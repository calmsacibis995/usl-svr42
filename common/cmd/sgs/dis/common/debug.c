/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dis:common/debug.c	1.8"

#include        <stdio.h>
#include        <malloc.h>
#include	<string.h>
#include	<memory.h>

#include	"dis.h"
#include	"structs.h"
#include	"libelf.h"
#include        "sgs.h"
#include	"dwarf.h"
#include	"ccstypes.h"
#include	"data32.h"

extern	int	trace; /* for debugging */
extern	int	file_byte;


static		Elf_Data	*debug_data;
static		Elf32_Shdr      *shdr;
static		unsigned char	*p_debug_data, *ptr;
static 		long		length = 0;
static		long		current;

static		long	get_long();
static		short	get_short();
static		char	get_byte();
static unsigned char	*get_string();
static		void	print_record(),
			not_interp();

void	get_debug_info(),
	get_line_info(),	 
	print_line();

void
get_debug_info()
{

	extern  Elf             *elf;
        extern  int             debug;
	extern	char		*fname;
        Elf_Scn         *scn;

	if ( (scn = elf_getscn(elf, debug)) == NULL)
	{
		(void) fprintf(stderr, 
		"%dis: %s: failed to get section .debug; limited functionality\n"
		, SGS, fname);
		debug = 0;
		return;
	}
	else
		if ((shdr = elf32_getshdr(scn)) != 0)
		{
			debug_data = 0;
			if ((debug_data=elf_getdata(scn, debug_data))==0 || debug_data->d_size == 0)
                        {
				(void) fprintf(stderr,
                                "%sdis: no data in section .debug\n", SGS);
				debug = 0;
                                return;		
			}
			else
			{
				p_debug_data = (unsigned char *)debug_data->d_buf;
				ptr = p_debug_data;
				print_record( shdr->sh_offset , shdr->sh_size);

			}
		}
}

/*ARGSUSED*/
static void
print_record( offset, size)
Elf32_Off	offset;
size_t		size;
{
	extern	void	build_labels();
	long	word;
	short	sword;
	short	attrname;
	short   len2;
	unsigned	char	*tag_source_name;
	short		tag;
	long 		tag_address;

  current = 0;
  while ( current < size)
  { 
	tag_source_name = NULL;

	word = get_long();

	if ( word <= 8)
	{
		if (trace) (void)printf("\n0x%-10lx\n", word);
		if(word < 4)
		{
			current += 4;
		}
		else
		{
			current += word;
			ptr += word - 4;
		}
		continue;
	}
	else
		current += word;

	if (trace) (void)printf("\n0x%-10lx", word);

	length = word - 4;

	tag = get_short();

	if (tag == TAG_label)
	{
		while(length > 0)
		{
			attrname = get_short();
			if (attrname == AT_name)
			{
				tag_source_name = get_string();
				if (trace)
                                	(void)printf("%s\n",tag_source_name);
			}
			else if (attrname == AT_low_pc)
			{
				tag_address = get_long();
				if (trace)
					(void)printf("0x%lx\n", tag_address);
			}
			else 
				not_interp(attrname);
		}
		build_labels(tag_source_name, tag_address);	
	}
	else
		ptr += length;
    }
  return;
}

static char 
get_byte()
{
	unsigned char 	*p;

	p = ptr; 
	++ptr;
	length -= 1;
	return *p;
}

static short
get_short()
{
	short x;

	if (file_byte == ELFDATA2MSB)
		x = MGET_SHORT(ptr);
	else
		x = LGET_SHORT(ptr);

	ptr += 2;
        length -= 2;
        return x;

}

static long
get_long()
{
	long 	x;

	if (file_byte == ELFDATA2MSB)
		x = MGET_LONG(ptr);
	else
		x = LGET_LONG(ptr);

	ptr += 4;
        length -= 4;
        return x;
}

static unsigned char *
get_string()
{
	unsigned char	*s;
	register 	int	len;
	
	len = strlen((char *)ptr) +1;
	s = (unsigned char *)malloc(len);
	(void)memcpy(s,ptr,len);
	ptr += len;
	length -= len;
	return s;

}


static void
not_interp( attrname ) 
short	attrname;
{
        short   len2;
        long    word;

        switch( attrname & FORM_MASK )
        {
                case FORM_NONE: break;
                case FORM_ADDR:
                case FORM_REF:  word = get_long();
				if (trace) (void)printf("<0x%lx>\n", word);
				break;
                case FORM_BLOCK2:       len2 = get_short();
                                        length -= len2;
                                        ptr += len2;
					if (trace) (void)printf("0x%x\n", len2);
					break;
                case FORM_BLOCK4:       word = get_long();
                                        length -= word;
                                        ptr += word;
					if (trace) (void)printf("0x%lx\n", word);
					break;
                case FORM_DATA2:        len2 = get_short();
					if (trace) (void)printf("0x%x\n", len2);
					break;
                case FORM_DATA8:        word = get_long();
					if (trace) (void)printf("0x%lx ", word);
					break;
                case FORM_DATA4:        word = get_long();
					if (trace) (void)printf("0x%lx\n", word);
					break;
                case FORM_STRING:       word = strlen((char *)ptr) + 1;
                                        length -= word;
					if (trace) (void)printf("%s\n", ptr);
                                        ptr += word;
					break;
                default:
			if (trace)
                        (void)printf("<unknown form: 0x%x>\n", (attrname & FORM_MASK) );
			length = 0;
        }
}

void
get_line_info()
{
	extern 	Elf	*elf;
	extern	int	line;
	extern	unsigned	char *ptr_line_data;
	extern	size_t		size_line;

	Elf_Scn	*scn;
	Elf_Data	*line_data;

	if ( (scn = elf_getscn(elf, line)) == NULL)
	{
		(void) fprintf(stderr,
		"%sdis: failed to get section .line; limited functioality\n"
		,SGS);
		line = 0;
		return;
	}
	else
	if ((shdr = elf32_getshdr(scn)) != 0)
	{
		line_data = 0;	
		if ((line_data=elf_getdata(scn, line_data))==0 || line_data->d_size ==0)
		{
			(void) fprintf(stderr,
			"%sdis: no data in section .line\n", SGS);
			line = 0;
			return;
		}
		else
		{
			ptr_line_data = (unsigned char *)line_data->d_buf;
			size_line	= line_data->d_size;
		}
	}
	return;
	
}

void
print_line(current, ptr_line, size_line)
long current;
unsigned	char *ptr_line;
size_t	size_line;
{
	extern	char	*fname;
	long  line;
	long  pcval;
	long  base_address;
	long  size;
	short delta;


	ptr = ptr_line;
	size = size_line;
	
	while (size > 0)
	{
		length = get_long();
		length -= 4;
		size -= 4;
		base_address = get_long();
		size -= 4;
	
		if(size < length-4)
		{
			(void)fprintf(stderr, "%sdis: %s: bad line info section -  size=%ld length-4=%ld\n", SGS, fname, size, length-4);
			return;
		}
		while(length > 0)
		{
			line = get_long();
			size -= 4;
			(void)get_short();
			size -= 2;
			delta = get_long();
			size -= 4;
			pcval = base_address + delta;

			if (current == pcval)
			 (void)printf("[%ld]", line);
			else
			if (current < pcval)
				return; /* can return because line
					   number info in ascending
					   order */
		}
	}
}
