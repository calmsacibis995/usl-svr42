#ident	"@(#)sdb:libexecon/i386/RegAccess.C	1.15"
#define lint	1

#include	"RegAccess.h"
#include	"Reg1.h"
#include	"prioctl.h"
#include	"i_87fp.h"
#include	"Interface.h"
#include	<memory.h>
#include	<unistd.h>
#include	<sys/reg.h>
#include	<sys/signal.h> 
#include	<sys/fs/s5dir.h>
#include	<sys/user.h>
#include	<errno.h>
#include	"Core.h"

#define ar0offset	((long)&(((struct user *)0)->u_ar0))

static void real2double(fp_tempreal* ,int*);
static void double2real(int* ,fp_tempreal*);
static int  CvtStk2Reg(RegRef, long *);
static void CvtReg2Stk(RegRef, long *, fpregset_t *);
int emetbit;
static fp_tempreal adj_stk[8];

RegAccess::RegAccess()
{
	key.fd = -1;
	key.pid = -1;
	core = 0;
	corefd = -1;
	fpcurrent = 0;
}

int
RegAccess::setup_core( int cfd, Core *coreptr )
{
	corefd = cfd;
	core = coreptr;
	key.fd = cfd;
	key.pid = -1;
	return 1;
}

extern int errno ;

static char *tagname[] = {"VALID","ZERO ","INVAL","EMPTY" };



#if PTRACE

int
RegAccess::setup_live( Key k )
{
	key.pid = k.pid;
	key.fd = -1;
	core = 0;
	::errno = 0;
	gpbase = ::ptrace( 3, key.pid, ar0offset, 0 );
	return ( ::errno == 0 );
}

int
RegAccess::update( prstatus & prstat )
{
	if ( key.pid != -1 )
	{
		::errno = 0;
		gpbase = ::ptrace( 3, key.pid, ar0offset, 0 );
		return ( ::errno == 0 );
	}
	return 0;
}

int
RegAccess::readlive( RegRef regref, long * word )
{
	int	i, *state_off;
	long	offset, sz, fpvalid_off, *fp;

	if ( key.pid == -1 )
	{
		return 0;
	}
	if (regref < FP_INDEX) {
		errno = 0;
		gpbase = ::ptrace( 3, key.pid, ar0offset, 0 );
		offset = gpbase + regs[regref].offset * sizeof(int);
		sz = regs[regref].size;
		i = 0;
		do {
			::errno = 0;
			word[i] = ::ptrace(3,key.pid,offset,0);
			sz -= sizeof(long);
			offset += sizeof(long);
			++i;
		} while ( sz > 0 );
		return (::errno == 0 );
	}
		
	//
	// Offset of flag byte in the u-block 
	//
	fpvalid_off = (long) &((struct user *)0)->u_fpvalid;
	//
	// force the child's floating point state into the u_block 
	//
	asm("fnop");
	asm("smsw	emetbit");
	//
	// 8 bits per byte, byte-reversed, get u_fpvalid, one byte 
	//
	::errno = 0;
	if ( !((ptrace(3,key.pid,fpvalid_off,0) >> ((fpvalid_off&0x3)*8)) & 0xff) )
	{
		// floating point was NOT used. return to calling one
		return 1;
	}
	if (::errno) {
		printe("cannot read u_fpvalid\n");
		return 0 ;
	}

	//
	// floating point was used. get fp stack.
	//
	// get state offset
	//
	state_off = (int *) ((struct user *)0)->u_fps.u_fpstate.state; 
	//	
	// fill up sdbfpstate
	//
	fp = (long *) &sdbfpstate;
	for (i = 0; i<sizeof(sdbfpstate)/sizeof(long); i++) {
		::errno = 0;
		*fp++ = ptrace(3,key.pid,(long)state_off++,0);
		if (::errno) {
			printe("cannot read u_fpstate\n");
			return 0 ;
		}
	}

	return CvtStk2Reg( regref, word );
}

int
RegAccess::writelive( RegRef regref, long * word )
{
	int	i;
	long	offset;
	long	sz;

	if ( key.pid == -1 )
	{
		return 0;
	}

	if ( regref < FP_INDEX )
	{
		offset = gpbase + regs[regref].offset * sizeof(int);
		sz = regs[regref].size;
		i = 0;
		do {
			::errno = 0;
			::ptrace(6,key.pid,offset,word[i]);
			sz -= sizeof(long);
			offset += sizeof(long);
			++i;
		} while ( sz > 0 );
		return (::errno == 0 );
	}

	// Floating Point Stack and Registers

	// fill up sdbfpstate
	long *fp_ptr;
	int *state_off = ((struct user *)0)->u_fps.u_fpstate.state;

	if (readlive( FP_STACK, 0 ) == 0 ) return 0;
	CvtReg2Stk( regref, word, 0 );
	fp_ptr = (long *) &sdbfpstate;
	sz = sizeof(sdbfpstate)/sizeof(long);
	do {
        	::errno = 0;
        	::ptrace(6, key.pid, (long)state_off++, *fp_ptr++);
		sz--;
        } while ( sz > 0 );
	return (::errno == 0 );
}

//
//  /proc
//
#else

int
RegAccess::setup_live( Key k )
{
	key.pid = k.pid;
	key.fd = k.fd;
	core = 0;
	return 1;
}

int
RegAccess::update( prstatus & prstat )
{
	if ( key.pid != -1 )
	{
		::memcpy( (char*)gpreg, (char*)prstat.pr_reg, sizeof(gpreg) );
		return 1;
	}
	return 0;
}

int
RegAccess::readlive( RegRef regref, long * word )
{
	if (regref < FP_INDEX) {
		word[0] = gpreg[regs[regref].offset];
		return 1;
	}
		
	//
	// force the child's floating point state into the u_block 
	//

	asm("fnop");
	asm("smsw	emetbit");

	fpregset_t fpreg;

	if ( ::getfpset(key, fpreg) == 0 ) {
		// floating point was NOT used. retrun to calling one
		return 1;
	}

	//
	// floating point was used. get fp stack.
	//
	// fill up sdbfpstate
	//
	::memcpy( (char*) &sdbfpstate, (char *)&fpreg, sizeof(sdbfpstate));

	return CvtStk2Reg( regref, word );
}

int
RegAccess::writelive( RegRef regref, long * word )
{
	if ( key.fd == -1 )
		return 0;

	if ( regref < FP_INDEX )
	{
		gpreg[regs[regref].offset] = word[0];
		do {
			::errno = 0;
			::ioctl( key.fd, PIOCSREG, &gpreg );
		} while ( ::errno == EINTR );
		return (::errno == 0);
	}

	// Floating Point Stack and Registers
	fpregset_t fpreg;

	if ( ::getfpset(key, fpreg) == 0 ) return 0;
	CvtReg2Stk( regref, word, &fpreg );
	do {
		::errno = 0;
		::ioctl( key.fd, PIOCSFPREG, &fpreg );
	} while ( ::errno == EINTR );
	return ( ::errno == 0 );
}

#endif

Iaddr
RegAccess::top_a_r()
{
	return getreg( REG_AP );
}

Iaddr
RegAccess::getreg( RegRef regref )
{
	Iaddr	addr;
	long	word[3];

	if ( readcore( regref, word ) || readlive( regref, word ) )
	{
		addr = word[0];
	}
	else
	{
		addr = 0;
	}
	return addr;
}

int
RegAccess::readreg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	if ( readcore( regref, word ) || readlive( regref, word ) )
	{
		switch (stype)
		{
		case SINVALID:	return 0;
		case Schar:	itype.ichar = word[0];		break;
		case Suchar:	itype.iuchar = word[0];		break;
		case Sint1:	itype.iint1 = word[0];		break;
		case Suint1:	itype.iuint1 = word[0];		break;
		case Sint2:	itype.iint2 = word[0];		break;
		case Suint2:	itype.iuint2 = word[0];		break;
		case Sint4:	itype.iint4 = word[0];		break;
		case Suint4:	itype.iuint4 = word[0];		break;
		case Saddr:	itype.iaddr = word[0];		break;
		case Sbase:	itype.ibase = word[0];		break;
		case Soffset:	itype.ioffset = word[0];	break;
		case Sxfloat:	itype.rawwords[2] = word[2];
		case Sdfloat:	itype.rawwords[1] = word[1];
		case Ssfloat:	itype.rawwords[0] = word[0];	break;
		default:	return 0;
		}
		return 1;
	}
	return 0;
}

int
RegAccess::readcore( RegRef regref, long * word )
{
	
	register greg_t     *greg;
	register fpregset_t *frptr;

	if ( core == 0 )
		return 0;

	if  (regref < FP_INDEX) {
		greg = core->getstatus()->pr_reg;
		word[0] = greg[regs[regref].offset];
		return 1;
	}

	if ( (frptr = core->getfpregs()) == 0)
		return 0;
	asm("smsw	emetbit");

	//
	// floating point was used. get fp stack.
	// get state offset
	// fill up sdbfpstate
	//
	::memcpy( (char*) &sdbfpstate, (char*) frptr, sizeof(sdbfpstate) );

	return CvtStk2Reg( regref, word );
}

int
RegAccess::writereg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	switch (stype)
	{
		case SINVALID:	return 0;
		case Schar:	word[0] = itype.ichar;		break;
		case Suchar:	word[0] = itype.iuchar;		break;
		case Sint1:	word[0] = itype.iint1;		break;
		case Suint1:	word[0] = itype.iuint1;		break;
		case Sint2:	word[0] = itype.iint2;		break;
		case Suint2:	word[0] = itype.iuint2;		break;
		case Sint4:	word[0] = itype.iint4;		break;
		case Suint4:	word[0] = itype.iuint4;		break;
		case Saddr:	word[0] = itype.iaddr;		break;
		case Sbase:	word[0] = itype.ibase;		break;
		case Soffset:	word[0] = itype.ioffset;	break;
		case Sxfloat:	word[2] = itype.rawwords[2];
		case Sdfloat:	word[1] = itype.rawwords[1];
		case Ssfloat:	word[0] = itype.rawwords[0];	break;
		default:	return 0;
	}
	return ( writecore( regref, word ) || writelive( regref, word ) );
}


static int
CvtStk2Reg( RegRef regref, long * word )
{
	int i, j;

	//
	// convert stack of reals to doubles
	//
	for (i = 0; i < 8; i++ ) {
		j = EM_ONLY ? i : (i+STACK_TOP) % 8;
		::memcpy( (char*) &adj_stk[j], (char*) &sdbfpstate.fp_stack[i],
			sizeof(fp_tempreal) );
	}
	for (i = 0; i < 8; i++ )
		real2double( &adj_stk[i], &fpregvals[i*2] );

	if ( regref == FP_STACK )
		// dumps all floating point registers and stack
		return 1;

	//
	// Only pickup specified floating point stack or register
	//
	if (regref >= FP_CTL) {	// Floating Point Special Registers
		switch (regref) {
		case FP_CW:	word[0] = sdbfpstate.fp_control;	break;
		case FP_SW:	word[0] = sdbfpstate.fp_status;		break;
		case FP_TW:	word[0] = sdbfpstate.fp_tag;		break;
		case FP_IP:	word[0] = sdbfpstate.fp_ip;		break;
		case FP_DP:	word[0] = sdbfpstate.fp_data_addr;	break;
		default:	return 0;
		}
		return 1;
	} 

	// Floating Point General Registers
	i = regref - FP_INDEX;
	word[0] = (long) fpregvals[i*2];
	word[1] = (long) fpregvals[i*2+1];
	return 1;
}

static void
real2double(fp_tempreal *tempreal, int *doubled)
{
	asm("fwait");
	asm("movl	8(%ebp),%eax");
	asm("fwait");
	asm("fldt	(%eax)");
	asm("movl	12(%ebp),%eax");
	asm("fwait");
	asm("fstpl	(%eax)");
}
int
RegAccess::writecore( RegRef regref, long * word )
{
	long	offset;
	long	sz;
	long	greg;
	char *	buf;
	fpregset_t *frptr;

	if ( core == 0 )
	{
		return 0;
	}
	greg = (long) &((prstatus_t *)core->statusbase())->pr_reg;

	if ( regref < FP_REG ) {
		offset = greg + regref * sizeof(int);	
		sz = regs[regref].size;
		buf = (char*) word;
	}
	else {
		offset = core->fpregbase();
		sz = sizeof(fpregset_t);
		frptr = core->getfpregs();
		CvtReg2Stk( regref, word, frptr );
		buf = (char*) frptr;
	}

	if ( ::put_bytes(key,offset,buf,sz) == sz )
	{
		core->update_reg( regref, 0, 0 );
		return 1;
	}
	else
	{
		return 0;
	}
}

static void
CvtReg2Stk( RegRef regref, long * word, fpregset_t * buf )
{
	if ( buf )
		::memcpy( (char*) &sdbfpstate, (char*)buf, sizeof(sdbfpstate) );

	if ( regref >= FP_CTL ) {
		switch ( regref ) {
		case FP_CW:
			sdbfpstate.fp_control = word[0];
			break;
		case FP_SW:
			sdbfpstate.fp_status = word[0];
			break;
		case FP_TW:
			sdbfpstate.fp_tag = word[0];
			break;
		case FP_IP:
			sdbfpstate.fp_ip = word[0];
			break;
		case FP_DP:
			sdbfpstate.fp_data_addr = word[0];
			break;
		}
	}
	else {
		int i = regref - FP_INDEX;
		int j = EM_ONLY ? i : (i+8-STACK_TOP) % 8;

		double2real( (int*)&word[0], &adj_stk[i] );
		::memcpy((char*)&sdbfpstate.fp_stack[j], (char*)&adj_stk[i],
			sizeof(fp_tempreal) );
	}

	if ( buf )
		::memcpy( (char*) buf, (char*) &sdbfpstate, sizeof(sdbfpstate) );
	return;
}

static void
double2real(int *doubled, fp_tempreal *tempreal)
{
	asm("fwait");
	asm("movl	8(%ebp),%eax");
	asm("fwait");
	asm("fldl	(%eax)");
	asm("movl	12(%ebp),%eax");
	asm("fwait");
	asm("fstpt	(%eax)");
}

int
RegAccess::display_regs( int num_per_line )
{
	RegAttrs *p;
	Itype	  x;
	int	  i, j, k, tag;

	i = 1;
	for( p = regs;  p->ref < FP_INDEX;  p++ ) {
		readreg( p->ref, Suint4, x );
		if ( i >= num_per_line )
		{
			printx( "%s	%#10x\n", p->name, x.iuint4 );
			i = 1;
		}
		else
		{
			i++;
			printx( "%s	%#10x\t", p->name, x.iuint4 );
		}
	}
	if ( i != 1 )
		printx("\n");

	if ( readreg( FP_STACK, Suint4, x ) == 0)  return 1;

	//
	// print out fp stacks and registers
	//
	printx("%%fpsw	%#10x\t", sdbfpstate.fp_status);
	printx("%%fpcw	%#10x\t", sdbfpstate.fp_control);
	printx("%%fptw	%#10x\n", sdbfpstate.fp_tag);
	printx("%%fpip	0x%.8x\t", sdbfpstate.fp_ip);
	printx("%%fpdp	0x%.8x\n", sdbfpstate.fp_data_addr);
	for (i = 0; i < 8 ; i++ )	// ST(0) <==> ST(7)
	{
		k = (STACK_TOP + i) % 8;	// Reg No. on Stack Top
		tag = TAG_OF_(k);
		printx("FP Reg Stack %.6s [ %s ] 0x",
			regs[FP_INDEX+k].name, tagname[tag]);
		for (j=4 ; j>=0 ; j--)
			printx("%.4x ", adj_stk[k][j]);
		printx("== %.14g ",fpregvals[k*2],fpregvals[k*2+1]);
		printx("\n");
	}
	return 1;
}
