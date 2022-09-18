/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:common/map.c	1.31"

#include "rtinc.h"

#define COMPLAIN	_rt_lasterr
#define	MMAP 		_mmap
#define MPROTECT 	_mprotect
#define CLRPAGE		_clrpage
#define CLOSE		_close
#define RTMKSPACE	_rtmkspace

/*
** Error Messages
*/
#define M_CannotSeek    "%s: %s: cannot seek to program header for file: %s"
#define M_Close		"%s: %s: cannot close %s"
#define M_EClass	"%s: %s: %s has wrong class or data encoding"
#define M_EMapHdr	"%s: %s: Cannot map elf header for file %s"
#define M_EMapFile	"%s: %s: Cannot map from file %s"
#define M_EMapSeg	"%s: %s: Cannot map segment %d for file %s"
#define M_ENotElf 	"%s: %s: %s is not an ELF file"
#define M_EDevZero	"%s: %s: Cannot open /dev/zero for file %s"
#define M_SegOutOfOrder "%s: %s: invalid program header - segments out of order: %s"
#define M_NoLoadSegs    "%s: %s: no loadable segments in %s"
#define M_ProtError	"%s: %s: cannot set protections file %s"
#define M_EZeroPg	"%s: %s: cannot map zero filled pages for file %s"
#define M_ENotExec	"%s: %s: %s not an executable file"
#define M_ENotSO	"%s: %s: %s not a shared object"
#define M_EBadMc	"%s: %s: bad machine type for file: %s"
#define M_EVersion	"%s: %s: bad file version for file: %s"

/*
** Useful Macros
*/

#define isTextSeg(l) (((l)->p_flags & (PF_R + PF_W + PF_X)) == (PF_R + PF_X))

#define setProt(phdrPtr, prot) \
	prot = 0; \
	if (phdrPtr->p_flags & PF_R)\
		prot |= PROT_READ;\
	if (phdrPtr->p_flags & PF_W)\
		prot |= PROT_WRITE;\
	if (phdrPtr->p_flags & PF_X)\
		prot |= PROT_EXEC


char *_proc_name;
int  _devzero_fd;
int  _rt_nodelete ;

struct rt_private_map *_map_so(fd, fname)
	int fd;
	CONST char *fname;
{
	int  hsize;
	int  rdCnt;
	int  isMain;
	int  vmemSize;	/* amount of virtual space needed to load all loadable
			 * segments */
	int firstProt;	/* protection mode of the first loadable segment */
	int protMode; 	/* protection mode needed for mmap*/
	int mapCriteria;/* map modes required for _mmap */

	int vaddrOffsetDiff; /* difference between fileoffset and vadd for first
			     ** loadable segment */
	unsigned long addrOffset;

	int tmpSz, index;
	ulong fileOffset;
	ulong entryPoint; /* entry point of the shared object */
	CONST char *soname;
	caddr_t baseAddress;	/* start address where object is mapped*/
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *textPhdr,		/* Prog. header for text segment */
		   *firstLoadable,	/* Prog. header for 1st load segment */
		   *lastLoadable,	/* Prog. header for last load segment*/
		   *phdr,		/* Prog. header ptr for the object*/
		   *pptr;		/* temp pointer */
	Elf32_Dyn  *mld;
	struct rt_private_map *lm;	/* link map for the so */

	DPRINTF(LIST,(2,"rtld: _map_so(%d, %s)\n",fd, 
			fname? fname : (CONST char *)"0"));

	if(fname == (char *)0){ /* is the object a.out */
		isMain = 1; 
		soname = _proc_name;
	} else {
		isMain = 0;
		soname = fname;
	}

	hsize = sizeof(Elf32_Ehdr) + (8 * sizeof(Elf32_Phdr));
	if(hsize < PAGESIZE) hsize = PAGESIZE;
	ehdr = (Elf32_Ehdr *)MMAP(0, hsize, PROT_READ, MAP_SHARED, fd, 0);
	if((int)ehdr == -1){
		COMPLAIN(M_EMapHdr, _rt_name, _proc_name, soname);
		return 0;
	}


	/* Calculate actual size of ehdr and phdr and map in the
	** exact amount
	*/
	rdCnt = ehdr->e_ehsize + ehdr->e_phnum * ehdr->e_phentsize;
	if(rdCnt > hsize){
		ehdr=(Elf32_Ehdr *)MMAP(0, rdCnt, PROT_READ, MAP_SHARED, fd, 0);
		if((int)ehdr == -1){
			COMPLAIN(M_EMapHdr, _rt_name, _proc_name, soname);
			return 0;
		}
	}
	phdr = (Elf32_Phdr *)((char *)ehdr + ehdr->e_ehsize);

	/* 
	** Verify information in file header 
	*/

        /* check ELF identifier */
        if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
		ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
		ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
		ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		COMPLAIN(M_ENotElf, _rt_name, _proc_name, soname);
		return 0;
	}

	/* check class and encoding */
	if (ehdr->e_ident[EI_CLASS] != M_CLASS ||
		ehdr->e_ident[EI_DATA] != M_DATA) {
		COMPLAIN(M_EClass, _rt_name, _proc_name, soname);
		return 0;
	}
        /* check magic number */
        if (isMain) {
                if (ehdr->e_type != ET_EXEC) {
			COMPLAIN(M_ENotExec, _rt_name, _proc_name, soname);
			return 0;
		}
	} else { /* shared object */
                if (ehdr->e_type != ET_DYN) {
			COMPLAIN(M_ENotSO, _rt_name, _proc_name, soname);
                        return 0;
                }
	}

	/* check machine type */
	if (ehdr->e_machine != M_MACH) {
		COMPLAIN(M_EBadMc, _rt_name, _proc_name, soname);
		return 0;
	}
	if (ehdr->e_flags != M_FLAGS) {
		if (_flag_error(ehdr->e_flags, soname))
			return 0;
	}

	/* verify ELF version */
	/* ??? is this too restrictive ??? */
	if (ehdr->e_version > EV_CURRENT) {
		COMPLAIN(M_EVersion, _rt_name, _proc_name, soname);
		return 0;
	}

	DPRINTF(LIST, (2, "phnum = %d phoff = %d phentsize = %d\n",
		ehdr->e_phnum, ehdr->e_phoff, ehdr->e_phentsize));

	/* traverse the program header information and  determine
	** if any loadable segments exist and find  the first and 
	** last loadable segment.
	*/

	for (index = (int)ehdr->e_phnum, pptr = phdr, 
		firstLoadable=0, textPhdr=0,
                tmpSz = ehdr->e_phentsize;
                index-- > 0;
                pptr=(Elf32_Phdr *)((unsigned long)pptr + tmpSz)){
                if (pptr->p_type == PT_LOAD) {
                        if ((textPhdr==0) && isTextSeg(pptr)){
				textPhdr = pptr;
                        }
                        if (firstLoadable== 0) {
                                firstLoadable = pptr;
                        }
                        else if (pptr->p_vaddr <= lastLoadable->p_vaddr) {
                                COMPLAIN(M_SegOutOfOrder,
                                        (char*) _rt_name,_proc_name,soname);
                                return 0;
                        }
                        lastLoadable = pptr;
                }
                else if (pptr->p_type == PT_DYNAMIC)
                        mld = (Elf32_Dyn *)(pptr->p_vaddr);
        } /* end of for */

	if(!firstLoadable){
		COMPLAIN(M_NoLoadSegs, _rt_name, _proc_name, soname);
		return 0;
	}

	/* Compute the amount of virtual address space that we need to map:
	** the difference between end of the last loadable segment
	** and the start of the first one ( everything is rounded or
	** truncted as appropriate at page boundaries). We map in that 
	** much amount of space from 'fd' to make sure we have the required
	** amount of contiguous virtual space. We map it in with the
	** protections and fileoffset and vaddr offset constraints of the
	** first loadable segment, thus ensuring that the first segment is
	** loaded correctly. For all subsequent segments that do not have the 
	** same file offset and vaddr difference as the first segment
	** we remap appropriate portions from fd. For all subsequent segments
	** that have the same file offset and vaddr difference we just 
	** set the protections. (Note : in some cases
	** the amount of virtual space needed may be larger than the filesize,
	** because the library does not have the bss section. In that case 
	** if the bss section extends beyond a page boundary we will 
	** map in /dev/zero to get zero filled pages mapped to bss.
	*/

	vmemSize = lastLoadable->p_vaddr + lastLoadable->p_memsz 
		   - STRUNC(firstLoadable->p_vaddr);
	fileOffset = STRUNC(firstLoadable->p_offset);
	vaddrOffsetDiff = firstLoadable->p_vaddr - firstLoadable->p_offset;

	if(ehdr->e_type == ET_DYN){
		baseAddress = 0;
		mapCriteria = MAP_PRIVATE;
	} else { /* a.out */
		/* The a.out is mapped at the exact address specified
		** by the p_vaddr field
		*/
		baseAddress = (caddr_t) STRUNC(firstLoadable->p_vaddr);
		mapCriteria = MAP_PRIVATE|MAP_FIXED;
	}
	setProt(firstLoadable, firstProt);
	baseAddress = MMAP(baseAddress, vmemSize, firstProt,
			   mapCriteria, fd, fileOffset);
	if(baseAddress == (caddr_t) -1){
		COMPLAIN(M_EMapFile, _rt_name, _proc_name, soname);
		return 0;
	}
	DPRINTF(LIST,(2,"Mapped %x bytes at addr = %x from offset = %x in file %s\n", 
		vmemSize, baseAddress, fileOffset, soname));	

	addrOffset = (ehdr->e_type == ET_DYN) ? (int) baseAddress : 0;
	mapCriteria = MAP_PRIVATE|MAP_FIXED; /*all mapping fixed from now on*/
	for(index=ehdr->e_phnum, pptr = phdr,
                tmpSz = ehdr->e_phentsize;
                index-- > 0;
                pptr=(Elf32_Phdr *)((unsigned long)pptr + tmpSz)){

		if(pptr->p_type != PT_LOAD) continue;

		/* check if we need to map anything from the file*/
		if(pptr != firstLoadable){

			/* map from file only if difference between
			** vaddr and file offset are different from
			** the first loadable segment else just set
			** the protections
			*/
			ulong tmpProt;
			int retVal;
			ulong extraRead=0;
			ulong baddr = STRUNC(pptr->p_vaddr + addrOffset);

			setProt(pptr, tmpProt);
			extraRead = pptr->p_vaddr + addrOffset - baddr;

			if((pptr->p_vaddr - pptr->p_offset) != vaddrOffsetDiff){
				retVal = (int)MMAP((caddr_t)baddr,
					pptr->p_filesz + extraRead,
					tmpProt, mapCriteria, fd,
					pptr->p_offset - extraRead);
				if(retVal == -1){
				   COMPLAIN(M_EMapSeg, _rt_name, _proc_name,
					    index, soname);
				   return 0;
				}
				DPRINTF(LIST,(2,
				  "Remapped seg %d, size = %x offset = %x\n", 
				  ehdr->e_phnum - index,
				  pptr->p_filesz + extraRead,
				  pptr->p_offset - extraRead));

			} else {
			    if(tmpProt != firstProt)
				retVal = MPROTECT((caddr_t)baddr,
					   pptr->p_filesz+extraRead,tmpProt);
				if(retVal == -1){
				   COMPLAIN(M_ProtError,
					 _rt_name,_proc_name,soname);
				   return 0;
				}
				DPRINTF(LIST, (2, 
				  "Changing protection of seg# %d, size = %x\n",
				  ehdr->e_phnum - index,
				  pptr->p_filesz));
			}
			/* If memsize is greater than filesize then we
			** will have to make sure we have zero filled 
			** memory for the extra virtual space. For the
			** extra virtual space within the last page
			** boundary we just zero fill it by _clrpage.
			** For the virtual space beyond the last page
			** we map in '/dev/zero' because we may not
			** have valid mappings from 'fd' for these pages.
			*/
			if(pptr->p_memsz > pptr->p_filesz){
			    ulong fileAddr;/*extent to which valid mapping exist*/
			    ulong startAddr = baddr+ extraRead;
			    int zCnt;
			    int devZeroMapped = 0;

			    fileAddr = PROUND(startAddr+pptr->p_filesz);
			    /* See if we need any extra pages */
			    if(fileAddr < (startAddr+pptr->p_memsz)){
				if(_devzero_fd < 0){
				   if(_rt_opendevzero() < 0){
					COMPLAIN(M_EDevZero, _rt_name, 
						_proc_name, soname);
					return 0;
			
				   }
				}
				DPRINTF(LIST, (2,
"mapping extra zero-filled pages for bss; from = 0x%x, size = 0x%x\n",
		fileAddr, (startAddr+pptr->p_memsz - fileAddr)));

				retVal = (int)MMAP((caddr_t)fileAddr, 
				(startAddr+pptr->p_memsz - fileAddr), /* size */
				tmpProt, mapCriteria, _devzero_fd, 0);
				if(retVal == -1){
				    COMPLAIN(M_EZeroPg,
					 _rt_name,_proc_name,soname);
			 	    return 0;
				}
				devZeroMapped = 1;
			    }

			    /* zero out last page which was mapped from
			    ** fd
			    */
			    zCnt = fileAddr - (startAddr+pptr->p_filesz);
			    if(zCnt > 0)
			       CLRPAGE((caddr_t)startAddr+pptr->p_filesz, zCnt);

			    DPRINTF(LIST, (2,
				"rtld: zero filled 0x%x bytes at 0x%x\n", 
				zCnt,startAddr+pptr->p_filesz));

			    /* return any unused virtual space to the
			    ** rtld storage allocator. Note this space
			    ** is usually from the bss and has write permission
			    ** so can be used by the allocator. Strictly we
			    ** we should check for write permission and only
			    ** then let the allocator use the space.
			    **
	 		    ** Note: we return the space only if it was mapped
			    ** from /dev/zero: it can be used by
			    ** the allocator without having to call _clrpage.
			    ** If it was mapped from the file the overhead of
			    ** _clrpage may not offset having to map a full 
			    ** page from /dev/zero. So we dont give the
			    ** space to the allocator.
			    */
			    if(!isMain && _rt_nodelete && (devZeroMapped==1)){
				ulong mAddr = startAddr+pptr->p_memsz;
				RTMKSPACE(mAddr, PROUND(mAddr) - mAddr);
				DPRINTF(LIST, (2, 
				"rtld: returning 0x%x bytes from 0x%x to the allocator\n",
				PROUND(mAddr) - mAddr, mAddr));
			    }

			}
		}
	} /* for */
	if(CLOSE(fd) < 0){
		COMPLAIN(M_Close, _rt_name, _proc_name, soname);
		return 0;
	}
	/* adjust entry points */
	entryPoint = (unsigned long)ehdr->e_entry + addrOffset;
	mld  = (Elf32_Dyn *)((unsigned long)mld + addrOffset);

	/* create and return new rt_private_map structure */

	lm = _new_lm(fname, mld, (unsigned long)baseAddress, 
			vmemSize, entryPoint, phdr, 
			ehdr->e_phnum, ehdr->e_phentsize);
	if (!lm)
		return 0;

	if (textPhdr) {
		TEXTSTART(lm) = textPhdr->p_vaddr + NAME(lm) ? ADDR(lm) : 0;
		TEXTSIZE(lm) = textPhdr->p_memsz;
	}
	if (TEXTREL(lm))
		if (_set_protect(lm, PROT_WRITE) == 0) {
			_rt_cleanup(lm);
			return 0;
		}
	return(lm);
}

/* create a new rt_private_map structure and initialize all values. */

struct rt_private_map *_new_lm(pname, ld, addr, msize, entry, phdr, phnum, phsize)
CONST char *pname;
Elf32_Dyn *ld;
unsigned long addr, msize, entry, phnum, phsize;
Elf32_Phdr *phdr;
{
	register struct rt_private_map *lm;
	register unsigned long offset;
	int rpathflag = 0;

	DPRINTF(LIST,(2, "rtld: _new_lm(%s, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, %d, %d)\n",
		(pname ?  pname : (CONST char *)"0"),(unsigned long)ld,addr,msize,entry,
		(unsigned long)phdr,phnum,phsize));

	/* allocate space */
	if ((lm = (struct rt_private_map *)_rtmalloc(sizeof(struct rt_private_map))) == 0) 
		return 0;

	/* all fields not filled in were set to 0 by _rtmalloc */
	NAME(lm) = (char *)pname;
	DYN(lm) = ld;
	ADDR(lm) = addr;
	MSIZE(lm) = msize;
	ENTRY(lm) = entry;
	PHDR(lm) = (VOID *)phdr;
	PHNUM(lm) = (unsigned short)phnum;
	PHSZ(lm) = (unsigned short)phsize;


	/* fill in rest of rt_private entries with info the from
	 * the file's dynamic structure
	 * if shared object, add base address to each address;
	 * if a.out, use address as is
	 */
	if (pname)
		offset = addr;
	else 
		offset = 0;

	/* read dynamic structure into an arry of ptrs to Elf32_Dyn
	 * unions - array[i] is pointer to Elf32_Dyn with tag == i
	 */
	for ( ; ld->d_tag != DT_NULL; ++ld ) {
		switch (ld->d_tag) {
		case DT_SYMTAB:
			SYMTAB(lm) = (char *)ld->d_un.d_ptr + offset;
			break;

		case DT_STRTAB:
			STRTAB(lm) = (char *)ld->d_un.d_ptr + offset;
			break;

		case DT_SYMENT:
			SYMENT(lm) = ld->d_un.d_val;
			break;

		case DT_TEXTREL:
			TEXTREL(lm) = 1;
			break;

	/* at this time we can only handle 1 type of relocation per object */
		case DT_REL:
		case DT_RELA:
			REL(lm) = (char *)ld->d_un.d_ptr + offset;
			break;

		case DT_RELSZ:
		case DT_RELASZ:
			RELSZ(lm) = ld->d_un.d_val;
			break;

		case DT_RELENT:
		case DT_RELAENT:
			RELENT(lm) = ld->d_un.d_val;
			break;

		case DT_HASH:
			HASH(lm) = (unsigned long *)(ld->d_un.d_ptr + offset);
			break;

		case DT_PLTGOT:
			PLTGOT(lm) = (unsigned long *)(ld->d_un.d_ptr + offset);
			break;

		case DT_PLTRELSZ:
			PLTRELSZ(lm) = ld->d_un.d_val;
			break;

		case DT_JMPREL:
			JMPREL(lm) = (char *)(ld->d_un.d_ptr) + offset;
			break;

		case DT_INIT:
			INIT(lm) = (void (*)())((unsigned long)ld->d_un.d_ptr + offset);
			break;

		case DT_FINI:
			FINI(lm) = (void (*)())((unsigned long)ld->d_un.d_ptr + offset);
			break;

		case DT_SYMBOLIC:
			SYMBOLIC(lm) = 1;
			break;

		case DT_RPATH:
			rpathflag = 1;
			RPATH(lm) = (char *) ld->d_un.d_val;
			break;

		case DT_DEBUG:
		/* set pointer to debugging information in a.out's
		 * dynamic structure
		 */
			ld->d_un.d_ptr = (Elf32_Addr)&_r_debug;
			break;
		}
	}
	if (rpathflag)
		RPATH(lm) = (char *)((unsigned long )RPATH(lm) + (char *)STRTAB(lm));

	return(lm);
}

/* function to correct protection settings 
 * segments are all mapped initially with  permissions as given in
 * the segment header, but we need to turn on write permissions
 * on a text segment if there are any relocations against that segment,
 * and them turn write permission back off again before returning control
 * to the program.  This function turns the permission on or off depending
 * on the value of the argument
 */

int _set_protect(lm, permission)
struct rt_private_map *lm;
int permission;
{
	register int i, prot;
	register Elf32_Phdr *phdr;
	unsigned long msize, addr;

	DPRINTF(LIST,(2, "rtld: _set_protect(%s, %d)\n",(NAME(lm) ? NAME(lm)
		:"a.out"), permission));

	phdr = (Elf32_Phdr *)PHDR(lm);
	/* process all loadable segments */
	for (i = 0; i < (int)PHNUM(lm); i++) {
		if ((phdr->p_type == PT_LOAD) && ((phdr->p_flags & PF_W) == 0)) {
			prot = PROT_READ | permission;
			if (phdr->p_flags & PF_X)
				prot |=  PROT_EXEC;
			addr = (unsigned long)phdr->p_vaddr + NAME(lm) ?
				ADDR(lm) : 0;
			msize = phdr->p_memsz + (addr - PTRUNC(addr));
			if (_mprotect((caddr_t)PTRUNC(addr), msize, prot) == -1){
				_rt_lasterr("%s: %s: can't set protections on segment of length 0x%x at 0x%x",(char*) _rt_name, _proc_name,msize, PTRUNC(addr));
				return(0);
			}
		}
		phdr = (Elf32_Phdr *)((unsigned long)phdr + PHSZ(lm));
	} /* end for phdr loop */
	return(1);
}
