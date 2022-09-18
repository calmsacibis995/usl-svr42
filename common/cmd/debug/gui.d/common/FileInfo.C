#ident	"@(#)debugger:gui.d/common/FileInfo.C	1.2"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "Machine.h"
#include "FileInfo.h"

static void read_note_sect(int, Elf_Ehdr *, char *&, int &);
static int read_data(int, char *, long, int);
static int is_strntext(const char *, int);

FileInfo::FileInfo(char *name)
{
	Elf_Ehdr	elfhdr;

	fname = name;
	obj_name = NULL;
	note_data = NULL;
	note_sz = 0;
	ftype = FT_UNKNOWN;
	fd = open(name, O_RDONLY);
	if(fd < 0 ||
	   read(fd, &elfhdr, sizeof(elfhdr)) != sizeof(elfhdr)
	  )
		return;
	if(strncmp((char *)elfhdr.e_ident, ELFMAG, SELFMAG) != 0)
	{
		if(is_strntext((char *)elfhdr.e_ident, SELFMAG))
		{
			ftype = FT_TEXT;
			return;
		}
		// COFF file ?
		return;
	}
	switch(elfhdr.e_type)
	{
	case ET_EXEC:
		ftype = FT_EXEC;
		break;
	case ET_CORE:
		ftype = FT_CORE;
		// read note section
		read_note_sect(fd, &elfhdr, note_data, note_sz);
		break;
	}
}

static void
read_note_sect(int fd, Elf_Ehdr *ehdrP, char *&note_data, int &note_sz)
{
	int phdrnum = ehdrP->e_phnum;
	if(!phdrnum)
		return;
        // read program header
	int phdrsz = phdrnum*ehdrP->e_phentsize;
	char *phdr = new char[phdrsz];
        if (read_data(fd, phdr, (long)ehdrP->e_phoff, phdrsz) != phdrsz)
	{
		delete phdr;
                return;
	}
        // read note section 
        Elf_Phdr        *pptr = (Elf_Phdr *)phdr;
        int             i;
        for (i = 0; i < phdrnum; i++, pptr++)
        {
                if (pptr->p_type == PT_NOTE)
                        break;
        }
	if(i >= phdrnum)
	{
		// no note section
		delete phdr;
		return;
	}
        note_sz = (int)pptr->p_filesz;
	note_data = new char[note_sz];
	if(read_data(fd, note_data, (long)pptr->p_offset, note_sz) != note_sz)
	{
		delete note_data;
		note_data = NULL;
		note_sz = 0;
		delete phdr;
		return;
	}
	delete phdr;
}

static int 
read_data(int fd, char *dest, long off, int sz)
{
	if(lseek(fd, off, SEEK_SET) == -1)
		return 0;
	return read(fd, dest, sz);
}

static int
is_strntext(const char *sp, int cnt) 
{
	int i;
	for(i = 0; i < cnt; ++i,++sp)
	{
		int c = *sp++;
		if(!isgraph(c) && !isspace(c))
			return 0;
	}
	return 1;
}

FileInfo::~FileInfo()
{
	close(fd);
	if(note_data != NULL)
		delete note_data;
}

char *
FileInfo::get_obj_name()
{
	switch(ftype)
	{
	case FT_EXEC:
		return fname;
	case FT_CORE:
		break;
	default:
		return "???";
	}
	if(obj_name)
		return obj_name;
	if(!note_data)
		return "";
	// find psargs in note section
	int namesz, descsz, type;
	char *p = note_data;
	int size = note_sz;
	while(size > 0)
	{
		namesz =  *(int *)p; p += sizeof(int);
                descsz = *(int *)p; p += sizeof(int);
                type   = *(int *)p; p += sizeof(int);
                size -= 3 * sizeof(int) + namesz + descsz;
                p += namesz;
                if( type == 3 )
                {
                        obj_name = ((prpsinfo_t *)p)->pr_psargs;
                        break;
                }
                p += descsz;
                int mod = (int)p % sizeof(int);
                if (mod)
                        p += sizeof(int) - mod;
        }
	if(!obj_name)
	{
		obj_name = "";
		return obj_name;
	}
	// return argv[0]
	for(p = obj_name; *p; ++p)
	{
		if(*p == ' ' || *p == '\t')
		{
			*p = '\0';
			break;
		}
	}
	return obj_name;
}

