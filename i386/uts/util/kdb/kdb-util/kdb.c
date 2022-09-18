/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/kdb/kdb-util/kdb.c	1.11"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <proc/user.h>
#include <proc/reg.h>
#include <util/cmn_err.h>
#include <util/kdb/kdebugger.h>
#include <util/kdb/xdebug.h>
#include <util/kdb/db_as.h>
#include <util/mod/mod_obj.h>


#ifdef MB2
#include "sys/mps.h"
#include "sys/immu.h"
#include "sys/ics.h"
extern ics_slot_t ics_slotmap[];
long FPIServChan;
unsigned char	FPIClients [ICS_MAX_SLOT] = {0};
#endif	/* MB2 */

boolean_t kdb_installed = B_TRUE;

char db_xdigit[] = "0123456789abcdef";

extern void (*kdb_inittbl[])();

/* Initial empty init commands; replaced by unixsyms */
static char empty_commands[2];
char *kdbcommands = empty_commands;
int kdbcommsz = sizeof empty_commands;

int debugger_early = 1;
char *debugger_init = "";
unsigned dbg_putc_count;

gregset_t *regset[NREGSET];

static int db_is_after_call();
static boolean_t hextoi(char**, ulong_t *);
/*
 * For machines with an interrupt button.
 */
kdbintr()
{
#ifdef MB2
	int i;
	unsigned char	val;

	if ((ics_myslotid()) == 0)  {
		for (i = 0; i < ICS_MAX_SLOT; i++) {
			if (FPIClients[i]) {
				val = ics_read(i,ICS_GeneralControl);
				ics_write(i,ICS_GeneralControl,(val | ICS_SWNMI_MASK));
			}
		}
	}
#endif
	(*cdebugger) (DR_USER, NO_FRAME);
}


/*
 * First-time initialization.  Called from kernel startup.c.
 */
kdb_init(early)
{
	/* Get pointer to command strings at end of symbol table */
	debugger_init = kdbcommands;

	/* Get pointer to command strings at end of symbol table */
	if ((debugger_early = early) != 0) {
		void	(**initf)();
	/* Get pointer to command strings at end of symbol table */

		/* Make sure debug registers are initially cleared */
		_wdr7(0);
		_wdr6(0);

		/* Initialize each installed interface. */
		for (initf = kdb_inittbl; *initf != (void (*)())0;)
			(*(*initf++)) ();

		/* Skip over normal init string to get to early-access string */
		while (*debugger_init++ != '\0')
			;

		/* Call debugger to execute early-access commands, if any */
		if (*debugger_init)
			(*cdebugger) (DR_INIT, NO_FRAME);
				/* (Can't use traps yet, so call directly.) */
	} else {
		/* Call debugger to execute normal init commands, if any */
		if (*debugger_init && cdebugger != nullsys) {
			extern int	demon_call_type;

			/*
			 * Set a flag and generate a trap into the debugger.
			 * This is done, rather than calling the debugger
			 * directly, to get a trap frame saved.
			 */
			demon_call_type = DR_INIT;
			asm(" int $3"); /* Force a debug trap */
		}
	}
}


#ifdef MB2
/*
 * For machines with an interrupt button that sends a message.
 */
kdbmesg(mbp)
mps_msgbuf_t *mbp;
{
	mps_free_msgbuf(mbp);
	(*cdebugger) (DR_USER, NO_FRAME);
}

#define	FPI_PORT_ID			(unsigned short)0x521
#define FPILocateServerC	0x01
#define FPIArmServerC 		0x02
#define NMISource			0x01

unsigned short	FPIServerFound; 
unsigned short	FPIServerArmed; 
int				do_fpi_locate ;
mb2socid_t 		FPIServer;

int
FPIServerMsg(tmsg)
struct msgbuf *tmsg;
{
	struct FPILocateServerResp {
		unchar Type;			/* FPILocateServerC */
		unchar Status;			/* FPISuccess, FPIFailure */
		unchar fill[18];
	}	*lmsgr;
	unsigned char	our_tid;
	mb2socid_t 	FPIClient;
	mps_msgbuf_t 	*mbp;

	lmsgr = (struct FPILocateServerResp *)mps_msg_getudp(tmsg);

	/*
	 * if we are in slot 0, this is a FPI Server request
	 */
	if ((ics_myslotid()) == 0)  {
		FPIClient = mps_mk_mb2socid(mps_msg_getsrcmid(tmsg), mps_msg_getsrcpid(tmsg));  
		if ((mbp = mps_get_msgbuf(KM_NOSLEEP)) == ((mps_msgbuf_t *)0))
			cmn_err (CE_PANIC,"kdb: FPIServerMsg cannot get message buffers\n");
		if (lmsgr->Type == FPILocateServerC) 
			lmsgr->Status = 0;
		else {
			if (lmsgr->Type == FPIArmServerC) {
				if (lmsgr->Status == 0)  /* actually command */
					FPIClients[mps_msg_getsrcmid(tmsg)] = 0;
				else {
					if (lmsgr->Status == NMISource)  {
						FPIClients[mps_msg_getsrcmid(tmsg)] = 1;
						lmsgr->Status = 0;
					}
					else
						lmsgr->Status = 0x80;
				}
			}
			else 
				lmsgr->Status = 0x80;
		}

		/* now send a reply */

		if ((our_tid = mps_get_tid(FPIServChan)) == 0)
			cmn_err(CE_NOTE, "kdb: FPIServerMsg cannot obtain tid\n");
		else {
			mps_mk_unsol(mbp, FPIClient, our_tid, 
			(unsigned char *)&lmsgr, sizeof(struct FPILocateServerResp));
			if (mps_AMPsend(FPIServChan, mbp) == -1L) {
				mps_msg_showmsg(mbp);
				cmn_err(CE_PANIC, 
					"kdb: FPIServerMsg send failure: chan=0x%x\n",FPIServChan);
			}
			mps_free_tid(FPIServChan, our_tid);
		}
	}
	else {
		if (lmsgr->Status == 0) {
			if (lmsgr->Type == FPILocateServerC) {
				FPIServerFound = 1;
				FPIServer = mps_mk_mb2socid(mps_msg_getsrcmid(tmsg), FPI_PORT_ID);  
			}
			if (lmsgr->Type == FPIArmServerC) {
				FPIServerArmed = 1;
			}
		}
		else
			cmn_err(CE_NOTE, "FPIlocate: status = 0x%x from FPI server\n", 
				lmsgr->Status);

		mps_free_msgbuf(tmsg);
	}
}

/*
 *	This is code to implement a broadcast locate protocol.
 *	This should ideally be modified to use the location service when
 *	that gets implemented.
*/

void
FPIlocate()
{	

	static struct FPILocArmServerReq	{
		unchar Type;			/* FPILocateServerC */
		unchar Command;			/* Command */
		unchar fill[18];
	} lmsg = { 0 };

	unsigned char		our_tid;
	unsigned short		start_chan; 
	unsigned short		loop_cnt;
	long				FPIChannel ;
	mps_msgbuf_t 			*mbp;

	FPIChannel = 0;
	FPIServerFound = 0;
	FPIServerArmed = 0;

	/* until TKI learns to allocate a free channel keep trying */

	for (start_chan = 0xA000; start_chan < 0xffff; start_chan++) { 
		FPIChannel = mps_open_chan(start_chan, FPIServerMsg, MPS_SRLPRIO);
		if (FPIChannel != -1L) 
			break;
	}
	if (FPIChannel == -1L) {
		cmn_err(CE_PANIC, "FPIlocate: cannot open broadcast channel\n");
	}

	if ((mbp = mps_get_msgbuf(KM_NOSLEEP)) == ((mps_msgbuf_t *)0))
		cmn_err (CE_PANIC, "FPIlocate: cannot get message buffers\n");
	lmsg.Type = FPILocateServerC;
	mps_mk_brdcst(mbp, FPI_PORT_ID, (unsigned char *)&lmsg, sizeof(lmsg));
	
	if (mps_AMPsend(FPIChannel, mbp) == -1)
		cmn_err(CE_PANIC, "FPIlocate: Broadcast failure. mbp=%x\n", mbp);

	for (loop_cnt = 0; loop_cnt < 2000; loop_cnt++) {
		if (FPIServerFound)
			break;
		spinwait(1);
	}

	if (!FPIServerFound) {
		dri_printf("FPI server not found\n");
		return;
	}

	/* now arm the FPI server */

	if ((mbp = mps_get_msgbuf(KM_NOSLEEP)) == ((mps_msgbuf_t *)0))
		cmn_err (CE_PANIC, "FPILocate: Cannot get message buffers\n");
	lmsg.Type = FPIArmServerC;
	lmsg.Command = NMISource;
	our_tid = mps_get_tid((long)FPIChannel); 
	mps_mk_unsol(mbp, FPIServer, our_tid, (unsigned char *)&lmsg, sizeof(lmsg));
	if (mps_AMPsend_rsvp(FPIChannel, mbp, 0, 0) == -1L) {
		mps_msg_showmsg(mbp);
		cmn_err(CE_PANIC, "FPILocate: send failure: chan=0x%x\n", FPIChannel);
	}
	for (loop_cnt = 0; loop_cnt < 1000; loop_cnt++) {
		if (FPIServerArmed)
			break;
		spinwait(1);			
	}
	mps_free_tid(FPIChannel, our_tid);
	mps_close_chan(FPIChannel);
	if (!FPIServerArmed) 
		dri_printf("FPI server not armed\n");
	dri_printf("FPI server found and initialized\n");
	return ;
}

/*
 * First-time Multibus initialization.
 */
kdbinit()
{
unsigned short ics_nmi_reg;
#define CSM_SLOT 0
#define CSM_MDR 0x3a

	/* 
	 * First check if we are in slot 0, if so front panel interrupts 
	 * arrive directly on the master PIC
	 */

	do_fpi_locate = 0;

	if ((ics_myslotid()) == 0) 
		return;

	/* if a CSM/001 isn't in slot 0 */

	if (strcmp("CSM/001", ics_slotmap[CSM_SLOT].s_pcode) == 0){
		do_fpi_locate = 0;
		if(mps_open_chan(MPS_FP_PORT, kdbmesg, MPS_BLKPRIO) == -1) {
			cmn_err(CE_WARN,"db: Cannot initialize front panel interrupt.");
		} else {
			ics_write(CSM_SLOT,CSM_MDR,(unsigned char)mps_lhid());
		}
	} else { 
		do_fpi_locate = 1;
		ics_nmi_reg = ics_read(ics_myslotid(),ICS_NMIEnable);
		ics_write(ics_myslotid(),ICS_NMIEnable,(ics_nmi_reg | ICS_SWNMI_MASK));
	}
}

kdbstart()
{
	/*
	 * Initialization that needs the message space.
	 */

	/*
	 * if we are in slot 0, open the FPI Server channel to listen to 
	 */
	if ((ics_myslotid()) == 0)  {
		FPIServChan = mps_open_chan(FPI_PORT_ID, FPIServerMsg, MPS_SRLPRIO);
		if (FPIServChan == -1L) 
			cmn_err(CE_PANIC,"kdb: kdbstart cannot open FPI Server channel\n");
		else
			cmn_err(CE_CONT, "kdb: Front Panel Interrupt server initialized\n");
	}
	else {
		if (do_fpi_locate)
			FPIlocate();
	}
}

#endif /* MB2 */


static struct kdebugger *debuggers = NULL;
static struct kdebugger *cur_debugger = NULL;

void
kdb_register(debugger)
	struct kdebugger *debugger;
{
	if ((debugger->kdb_next = debuggers) == NULL) {
		debuggers = debugger->kdb_next = debugger;
		cdebugger = (cur_debugger = debugger)->kdb_entry;
	} else
		(debugger->kdb_prev = debuggers->kdb_prev)->kdb_next = debugger;

	debuggers->kdb_prev = debugger;
}

void
kdb_next_debugger()
{
	cur_debugger = cur_debugger->kdb_next;
	cdebugger = cur_debugger->kdb_entry;
}

void
kdb_next_io()
{
	if (++cdbg_io >= &dbg_io[ndbg_io])
		cdbg_io = dbg_io;
}

int
debug_on_console()
{
	extern struct conssw conssw;
	extern struct conssw **cdbg_io;

	return (*cdbg_io == &conssw);
}


/*
 * findsyminfo does a symbol table lookup to find the routine name which begins
 * closest to (but not after) value.
 */

char *
findsyminfo(value, loc_p, valid_p)
	ulong	value;
	ulong *	loc_p;
	uint *	valid_p;
{
	char *name;
	ulong offset;
	extern char stext[];

	name = mod_obj_getsymname(value, &offset, B_FALSE, NULL);
	*loc_p = value - offset;
	if (name == NULL || *loc_p < (ulong)stext) {
		*valid_p = 0;
		*loc_p = 0;
		return "ZERO";
	}
	*valid_p = 1;
	return name;
}

char *
findsymname(value, tell)
	ulong	value;
	void	(*tell)();   /* function to print name and location (or NULL) */
{
	char *	p;
	ulong	loc;
	uint	valid;

	p = findsyminfo(value, &loc, &valid);
	if (tell)
		(*tell) (p, value, loc);
	return valid? p : NULL;
}

void
db_sym_and_off(addr, prf)
	ulong	addr;
	void	(*prf)();
{
	char *	p;
	ulong	sym_addr;
	uint	valid;

	p = findsyminfo(addr, &sym_addr, &valid);
	if (!valid) {
		(*prf) ("?0x%x?", addr);
		return;
	}
	(*prf) ("%s", p);
	if (addr != sym_addr)
		(*prf) ("+0x%lx", addr - sym_addr);
}

ulong
findsymval(value)
	ulong	value;
{
	ulong	loc;
	uint	valid;

	(void) findsyminfo(value, &loc, &valid);
	return valid? loc : (ulong)0;
}

/*
 * findsymaddr does a symbol table lookup to find the address of name.
 */

ulong
findsymaddr(name)
	char *name;
{
	return mod_obj_getsymvalue(name, B_FALSE, B_FALSE);
}


volatile ulong_t db_st_startsp;
volatile ulong_t db_st_startpc;
volatile ulong_t db_st_startfp;
ulong_t db_st_offset;
int *db_st_r0ptr;
uint_t db_max_args = 3;

extern char stext[], sdata[];
extern void k_trap(), u_trap(), systrap(), sigclean();

#define IS_TRAP_ROUTINE(pc) \
		((pc) == (ulong_t)k_trap || \
		 (pc) == (ulong_t)u_trap || \
		 (pc) == (ulong_t)systrap || \
		 (pc) == (ulong_t)sigclean)

static boolean_t find_return(ulong_t, ulong_t *, ulong_t *, void (*)(),
			     long *);
static void nframe();
static void tframe(ulong_t, ulong_t, void (*)(), int);
static void iframe(ulong_t, ulong_t, void (*)());

#define _G(x,i) (&((unsigned long *)((long)(x) + db_st_offset))[i])
#define USTACKLO ((unsigned long)&u)
#define USTACKLIM ((unsigned long)&u + KSTKSZ)
#define INSTACK(lower,value) ((lower) <= (value) && (value) < stacklim)
#define ISKTRAP(r0ptr) \
		(!(G(r0ptr, EFL) & PS_VM) && !(G(r0ptr, CS) & SEL_LDT))

#define UNKNOWN	((ulong_t)-1)	/* unknown value in %ebp or %esp registers */

static unsigned rsnext;		/* Next register set number */
static enum { RT_RET, RT_IRET } ret_type;	/* Type of return inst */

static ulong
G(ulong bp, int offset)
{
	as_addr_t	addr;
	ulong		val;

	SET_KVIRT_ADDR(addr, (ulong)_G(bp, offset));
	if (db_read(addr, &val, sizeof val) == -1)
		val = 0;
	return val;
}

void
db_stacktrace(void (*prf)(), ulong_t dbg_entry, int local)
{
	static void do_stacktrace(void (*)(), ulong_t);

	if (local) {
		db_get_stack();
		db_st_offset = 0;
	}
	do_stacktrace(prf, dbg_entry);
	db_st_r0ptr = NULL;
}


static void
do_stacktrace(void (*prf)(), ulong_t dbg_entry)
{
	ulong	pc;		/* program counter (eip) in current function */
	ulong	prevpc;		/* program counter (eip) in previous function */
	ulong	sp;		/* stack ptr (esp) for current function */
	ulong	sp_start;	/* stack ptr (esp) at start of search */
	ulong	ebp;		/* "frame ptr" (ebp) for current function */
	ulong	ap;		/* argument ptr for current function */
	ulong	fn_entry;	/* entry point for current function */
	ulong	fn_start;	/* start of current function (from symbols) */
	as_addr_t pctry;	/* trial program counter for previous func */
	long	sp_delta;	/* first adjustment of esp after func call */
	int	skip_frames = 0;/* flag: skip frames inside of debugger */
	int	ktrap;		/* interrupt/trap was from kernel mode */
	int	narg;		/* # arguments */
	char	tag;		/* call-type tag: '*' indirect or '~' close */
	ulong	stacklim;	/* current upper limit on stack */
	ulong_t trap_r0;	/* r0ptr for trap frame */

	for (rsnext = NREGSET; rsnext != 0;)
		regset[--rsnext] = NULL;

	pc = db_st_startpc;
	ebp = db_st_startfp;
	if ((sp = db_st_startsp) >= USTACKLO && sp < USTACKLIM)
		stacklim = USTACKLIM;
	else
		stacklim = (sp & PAGEMASK) + PAGESIZE;

	if (dbg_entry && prf) {
		skip_frames = 1;
		dbg_entry = findsymval(dbg_entry);
	}

	if ((trap_r0 = (ulong_t)db_st_r0ptr) != 0) {
		if (!skip_frames) {
			if (ISKTRAP(trap_r0))
				pc = (ulong_t)k_trap;
			else
				pc = (ulong_t)u_trap;
			goto trap_frame;
		}
	}

	while (mod_obj_validaddr(pc)) {
		/* look through the stack for a valid ret addr */
		fn_start = findsymval(pc);
		sp_start = sp;
		if (!find_return(pc, &sp, &ebp, prf, (long *)NULL) ||
		    !INSTACK(sp_start, sp)) {
			nframe(pc, '>', 0, 0, 0, prf);
			break;
		}
		SET_KVIRT_ADDR(pctry, G(sp, 0));
		prevpc = pctry.a_addr;
		if (ret_type == RT_IRET) {
			skip_frames = 0;
			iframe(pc, sp - EIP * sizeof(int), prf);
			if (!ISKTRAP(sp - EIP * sizeof(int)))
				break;
			pc = prevpc;
			sp += 3 * sizeof(int); /* adjust for 'iret' inst */
			continue;
		}
		if (!mod_obj_validaddr(prevpc) ||
		    !db_is_after_call(pctry, &fn_entry)) {
			if (prf) {
				(*prf) ("<<invalid return address: %08X"
					" (esp = %08X)>>\n",
					prevpc, sp);
			}
			break;
		}
		ap = sp + sizeof(ulong);
		if (skip_frames) {
		    if (fn_start == dbg_entry) {
			(*prf) ("DEBUGGER ENTERED FROM ");
			if (ap) {
			    switch (G(ap, 0)) {
			    case DR_USER:
			    case DR_SECURE_USER:
				(*prf) ("USER REQUEST");
				break;
			    case DR_BPT1:
			    case DR_BPT3:
				(*prf) ("BREAKPOINT");
				break;
			    case DR_STEP:
				(*prf) ("SINGLE-STEP");
				break;
			    case DR_PANIC:
				(*prf) ("PANIC");
				break;
			    default:
				db_sym_and_off(prevpc, prf);
				break;
			    }
			}
			(*prf) ("\n");
done_skip:
			skip_frames = 0;
			if (db_st_r0ptr) {
				if (ISKTRAP(trap_r0))
					pc = (ulong_t)k_trap;
				else
					pc = (ulong_t)u_trap;
				goto trap_frame;
			}
		    }
		} else {
			if (fn_entry == 0) {
				tag = '*';
				fn_entry = pc;
			} else {
				if (fn_entry < fn_start || fn_entry > pc)
					nframe(pc, '>', 0, 0, 0, prf);
				tag = ' ';
			}
			narg = (stacklim - ap) / sizeof(int);
			if (IS_TRAP_ROUTINE(fn_entry))
				narg = 0;
			else if (find_return(pctry.a_addr, &ap, &ebp,
					     (void (*)())NULL, &sp_delta) &&
				 sp_delta > 0) {
				if (narg > sp_delta / sizeof(int)) {
					narg = sp_delta / sizeof(int);
					if (narg > 7 && narg > db_max_args) {
						if ((narg = db_max_args) < 7)
							narg = 7;
					}
				}
			} else if (narg > db_max_args)
				narg = db_max_args;
			nframe(fn_entry, tag, prevpc, ap, narg, prf);
			if (IS_TRAP_ROUTINE(fn_entry)) {
				trap_r0 = G(ap, 0);
trap_frame:
				skip_frames = 0;
				ktrap = ISKTRAP(trap_r0);
				tframe(pc, trap_r0, prf, ktrap);
				if (!ktrap ||
				    (G(trap_r0, CS) & 0xFFFF) != KCSSEL)
					break;
				pc = G(trap_r0, EIP);
				sp = trap_r0 + ((ktrap ? EFL : SS) + 1) *
					        sizeof(int);
				ebp = G(trap_r0, EBP);
				continue;
			}
		}
		pc = prevpc;
		sp = ap;	  /* adjust for 'ret' inst */
		if (skip_frames && db_st_r0ptr && sp >= trap_r0)
			goto done_skip;
	}
}

#define MAXINST		3000
#define MAXBRANCH	64

#ifdef DEBUG
int find_return_verbose = 0;
#endif

static boolean_t
find_return(ulong_t pc, ulong_t *spp, ulong_t *ebpp, void (*prf)(),
	    long *sp_delta_p)
{
	ulong_t sp, ebp;
	ulong_t prev_sp;
	as_addr_t addr, nextaddr;
	char *p, *endp;
	ulong_t unk_addr;
	ulong_t pushed_ebp, ebp_addr;
	ulong_t branch_addr[MAXBRANCH];
	short branch_tried[MAXBRANCH];
	uint_t ninst, nbranch;
	uint_t br, last_try, last_new_branch, start_time;
	boolean_t looped;
	extern char mneu[];
	extern void resume();

	ninst = nbranch = 0;
	unk_addr = 0;
	looped = B_FALSE;
restart:
	SET_KVIRT_ADDR(addr, pc);
	sp = *spp;
	ebp = *ebpp;
	ebp_addr = UNKNOWN;
	start_time = last_try = last_new_branch = ninst;

	/* Disassemble instructions until we find a return instruction. */
	for (;; addr.a_addr = nextaddr.a_addr) {
		if (++ninst > MAXINST) {
			if (prf)
				(*prf) ("<<giving up at %x after disassembling "
					"%d instructions>>\n", addr.a_addr,
					MAXINST);
			return B_FALSE;
		}
		nextaddr = dis_dot(addr);
#ifdef DEBUG
		if (find_return_verbose && prf)
			(*prf)("%x:  %s\n", addr.a_addr, mneu);
#endif
		/* Set p to point to the first operand, if any. */
		for (p = mneu; *p && *p != ' '; p++)
			;
		while (*p == ' ')
			++p;
		/* Strip trailing blanks; leave endp pointing to end. */
		endp = p + strlen(p);

		while (endp != mneu && endp[-1] == ' ')
			*--endp = '\0';
		/* Check for instructions which affect %esp or %ebp. */
		if (strncmp(mneu, "ret", 3) == 0) {
			ret_type = RT_RET;
			break;
		}
		if (strncmp(mneu, "iret", 4) == 0) {
			ret_type = RT_IRET;
			break;
		}
		prev_sp = sp;
		if (strncmp(p, "Error", 5) == 0) {
			sp = ebp = UNKNOWN;
			break;
		} else if (strncmp(mneu, "leave", 5) == 0) {
			if ((sp = ebp) != UNKNOWN) {
				ebp = G(sp, 0);
				sp += 4;
			}
		} else if (strncmp(mneu, "pushl", 5) == 0) {
			if (sp != UNKNOWN) {
				sp -= 4;
				if (strcmp(p, "%ebp") == 0) {
					pushed_ebp = ebp;
					ebp_addr = sp;
				}
			}
		} else if (strncmp(mneu, "pushw", 5) == 0) {
			if (sp != UNKNOWN)
				sp -= 2;
		} else if (strncmp(mneu, "pushal", 6) == 0) {
			if (sp != UNKNOWN)
				sp -= 32;
		} else if (strncmp(mneu, "pushaw", 6) == 0) {
			if (sp != UNKNOWN)
				sp -= 16;
		} else if (strncmp(mneu, "pushfl", 6) == 0) {
			if (sp != UNKNOWN)
				sp -= 4;
		} else if (strncmp(mneu, "pushfw", 6) == 0) {
			if (sp != UNKNOWN)
				sp -= 2;
		} else if (strncmp(mneu, "popl", 4) == 0) {
			if (strcmp(p, "%ebp") == 0) {
				if (sp != UNKNOWN) {
					if (ebp_addr == sp) {
						ebp = pushed_ebp;
						ebp_addr = UNKNOWN;
					} else
						ebp = G(sp, 0);
				} else
					ebp = UNKNOWN;
			}
			if (sp != UNKNOWN)
				sp += 4;
		} else if (strncmp(mneu, "popw", 4) == 0) {
			if (strcmp(p, "%bp") == 0)
				ebp = UNKNOWN;
			if (sp != UNKNOWN)
				sp += 2;
		} else if (strncmp(mneu, "popal", 5) == 0) {
			if (sp != UNKNOWN) {
				ebp = G(sp, 2);
				sp += 32;
			} else
				ebp = UNKNOWN;
		} else if (strncmp(mneu, "popaw", 5) == 0) {
			ebp = UNKNOWN;
			if (sp != UNKNOWN)
				sp += 16;
		} else if (strncmp(mneu, "popfl", 5) == 0) {
			if (sp != UNKNOWN)
				sp += 4;
		} else if (strncmp(mneu, "popfw", 5) == 0) {
			if (sp != UNKNOWN)
				sp += 2;
		} else if (endp > mneu + 4 &&
			 (strcmp(endp - 4, "%esp") == 0 ||
			  strcmp(endp - 4, "%ebp") == 0 ||
			  strcmp(endp - 3, "%sp") == 0 ||
			  strcmp(endp - 3, "%bp") == 0)) {
			boolean_t reg16 = (endp[-3] != 'e');
			boolean_t immediate, setting_sp;
			ulong_t imm_val, new_val;

			if ((setting_sp = (strcmp(endp - 2, "sp") == 0)))
				new_val = sp;
			else
				new_val = ebp;

			if ((immediate = (*p == '$'))) {
				++p;
				if (!hextoi(&p, &imm_val)) {
					--p;
					immediate = B_FALSE;
				}
			}

			if (strncmp(mneu, "inc", 3) == 0) {
				if (reg16)
					new_val = UNKNOWN;
				else
					new_val++;
			} else if (strncmp(mneu, "dec", 3) == 0) {
				if (reg16)
					new_val = UNKNOWN;
				else
					new_val--;
			} else if (strncmp(mneu, "mov", 3) == 0) {
				if (reg16)
					new_val = UNKNOWN;
				else if (immediate)
					new_val = imm_val;
				else if (strncmp(p, "%esp", 4) == 0)
					new_val = sp;
				else if (strncmp(p, "%ebp", 4) == 0)
					new_val = ebp;
				else if (findsymval(addr.a_addr) == (ulong)resume) {
					/*
					 * This is a real hack to deal with the fact that the
					 * SVR4.1 context switch code restores %esp with an
					 * indirect move via %ebx from the saved TSS value.
					 */
					new_val = db_st_startsp;
				} else
					new_val = UNKNOWN;
			} else if (strncmp(mneu, "add", 3) == 0) {
				if (reg16 || !immediate)
					new_val = UNKNOWN;
				else
					new_val += imm_val;
			} else if (strncmp(mneu, "sub", 3) == 0) {
				if (reg16 || !immediate)
					new_val = UNKNOWN;
				else
					new_val -= imm_val;
			} else if (strncmp(mneu, "lea", 3) == 0) {
				if (reg16 || !hextoi(&p, &imm_val))
					new_val = UNKNOWN;
				else {
					new_val += imm_val;
					if (setting_sp) {
					    if (strncmp(p, "(%esp)", 6) != 0 &&
					        strncmp(p, "(%esp,1)", 8) != 0)
						new_val = UNKNOWN;
					} else {
					    if (strncmp(p, "(%ebp)", 6) != 0 &&
					        strncmp(p, "(%ebp,1)", 8) != 0)
						new_val = UNKNOWN;
					}
				}
			} else if (strncmp(mneu, "enter", 5) == 0) {
				new_val = UNKNOWN; /* XXX - for now */
			} else if (strncmp(mneu, "xchg", 4) == 0 ||
				   strncmp(mneu, "lds", 3) == 0 ||
				   strncmp(mneu, "les", 3) == 0 ||
				   strncmp(mneu, "lfs", 3) == 0 ||
				   strncmp(mneu, "lgs", 3) == 0 ||
				   strncmp(mneu, "lss", 3) == 0 ||
				   strncmp(mneu, "adc", 3) == 0 ||
				   strncmp(mneu, "sbb", 3) == 0 ||
				   strncmp(mneu, "neg", 3) == 0 ||
				   strncmp(mneu, "not", 3) == 0 ||
				   strncmp(mneu, "and", 3) == 0 ||
				   strncmp(mneu, "or", 2) == 0 ||
				   strncmp(mneu, "xor", 3) == 0 ||
				   strncmp(mneu, "rol", 3) == 0 ||
				   strncmp(mneu, "ror", 3) == 0 ||
				   strncmp(mneu, "sal", 3) == 0 ||
				   strncmp(mneu, "sar", 3) == 0 ||
				   strncmp(mneu, "sal", 3) == 0 ||
				   strncmp(mneu, "shr", 3) == 0 ||
				   strncmp(mneu, "shl", 3) == 0 ||
				   strncmp(mneu, "rcl", 3) == 0 ||
				   strncmp(mneu, "rcr", 3) == 0 ||
				   strncmp(mneu, "imul", 4) == 0) {
				new_val = UNKNOWN;
			}
			if (setting_sp)
				sp = new_val;
			else
				ebp = new_val;
		}
		if (sp == UNKNOWN) {
			if (unk_addr == 0)
				unk_addr = addr.a_addr;
		} else if (sp != prev_sp) {
			if (sp_delta_p) {
				*sp_delta_p = sp - prev_sp;
				return B_TRUE;
			}
		}
		/* If we're looking for 1st delta, bail out on a call inst */
		if (sp_delta_p && strncmp(mneu, "call", 4) == 0)
			return B_FALSE;
		/* Check for conditional or unconditional branches */
		if (mneu[0] != 'j' && strncmp(mneu, "loop", 4) != 0)
			continue;
		/* Handle branch instructions */
		for (br = 0; br < nbranch; br++) {
			if (branch_addr[br] == addr.a_addr)
				break;
		}
		if (br < nbranch) {
			if (branch_tried[br]) {
				if (branch_tried[br] >= last_try) {
					/* We looped; give up. */
					if (last_try != start_time) {
						looped = B_TRUE;
						goto restart;
					}
					return B_FALSE;
				}
				if (strncmp(mneu, "jmp", 3) != 0 &&
				    branch_tried[br] >= last_new_branch) {
					branch_tried[br] = ninst;
					continue;
				}
				branch_tried[br] = ninst;
			} else
				last_try = branch_tried[br] = ninst;
		} else if (nbranch < MAXBRANCH) {
			last_try = last_new_branch = ninst;
			branch_addr[nbranch++] = addr.a_addr;
			if (strncmp(mneu, "jmp", 3) == 0)
				branch_tried[br] = ninst;
			else {
				branch_tried[br] = 0;
				continue;
			}
		}
		/* Follow jump address */
		while (*p && *p != '<')
			p++;
		if (*p++ != '<' ||
		    !hextoi(&p, &nextaddr.a_addr)) {
			if (last_try != start_time)
				goto restart;
			if (!looped && prf)
				(*prf) ("<<unknown branch: %s>>\n", mneu);
			return B_FALSE;
		}
	}
	if (sp == UNKNOWN) {
		if (last_try != start_time)
			goto restart;
		if (!looped && prf) {
			(*prf) ("<<unknown esp adjustment at %08X>>\n",
				unk_addr);
		}
		return B_FALSE;
	}
	*spp = sp;
	*ebpp = ebp;
	if (sp_delta_p)
		*sp_delta_p = 0;
	return B_TRUE;
}

#define LINE_WIDTH	80
#define FUNC_WIDTH	(LINE_WIDTH - 1 - 28)

static void
nframe(pc, tag, prevpc, ap, narg, prf)
	ulong		pc, prevpc, ap;
	unsigned	narg;
	char		tag;
	void		(*prf)();
{
	ulong	ebp_est = ap - 2 * sizeof(ulong);

	if (prf == NULL)
		return;

	dbg_putc_count = 0;

	(*prf) ("%c", tag);
	db_sym_and_off(pc, prf);
	if (ap == 0) {
		(*prf) ("()\n");
		return;
	}
	(*prf) ("(");
	while (narg-- != 0) {
		(*prf) ("%x", G(ap, 0));
		ap += sizeof(ulong);
		if (narg != 0)
			(*prf) (" ");
	}
	(*prf) (")");

	if (dbg_putc_count > FUNC_WIDTH) {
		while (dbg_putc_count < LINE_WIDTH - 1)
			(*prf) (".");
		(*prf) ("\n");
		dbg_putc_count = 0;
		(*prf) ("      ");
	}
	while (dbg_putc_count <= FUNC_WIDTH)
		(*prf) (".");

	(*prf) ("(ebp:%08x) ", ebp_est);
	if (prevpc)
		(*prf) ("ret:%08x\n", prevpc);
	else
		(*prf) ("\n");
}

static void
tframe(ulong_t pc, ulong_t trap_r0, void (*prf)(), int ktrap)
{
	ulong_t		fn_start;

	regset[rsnext++] = (gregset_t *)_G(trap_r0, 0);
	if (rsnext >= NREGSET)
		rsnext = NREGSET - 1;
	if (prf == NULL)
		return;

	fn_start = findsymval(pc);
	if (fn_start == (ulong_t)k_trap || fn_start == (ulong_t)u_trap)
		(*prf) ("TRAP 0x%x", G(trap_r0, TRAPNO));
	else if (fn_start == (ulong_t)systrap)
		(*prf) ("SYSTEM CALL");
	else if (fn_start == (ulong_t)sigclean)
		(*prf) ("SIGNAL RETURN");
	else {
		(*prf) ("?TRAP TO ");
		db_sym_and_off(pc, prf);
		(*prf) (" (trap 0x%x)", G(trap_r0, TRAPNO));
	}
	(*prf) (" from %x:%x (r0ptr:%x",
			G(trap_r0, CS) & 0xFFFF, G(trap_r0, EIP), trap_r0);
	if (ktrap)
		(*prf) (")\n");
	else {
		(*prf) (", ss:esp: %x:%x)\n",
				G(trap_r0, SS) & 0xFFFF, G(trap_r0, UESP));
	}
	db_frameregs(trap_r0, rsnext - 1, prf);
}

static void
iframe(ulong_t pc, ulong_t trap_r0, void (*prf)())
{
	ulong_t		fn_start;

	regset[rsnext++] = (gregset_t *)_G(trap_r0, 0);
	if (rsnext >= NREGSET)
		rsnext = NREGSET - 1;
	if (prf == NULL)
		return;

	fn_start = findsymval(pc);
	(*prf) ("INTERRUPT TO ");
	db_sym_and_off(pc, prf);
	(*prf) (" from %x:%x (r0ptr:%x",
			G(trap_r0, CS) & 0xFFFF, G(trap_r0, EIP), trap_r0);
	if (ISKTRAP(trap_r0))
		(*prf) (")\n");
	else {
		(*prf) (", ss:esp: %x:%x)\n",
				G(trap_r0, SS) & 0xFFFF, G(trap_r0, UESP));
	}
	db_frameregs(trap_r0, rsnext - 1, prf);
}

void
db_frameregs(ulong_t r0p, uint_t rs, void (*prf)())
{
	(*prf) ("   eax:%8x ebx:%8x ecx:%8x edx:%8x efl:%8x   ds:%4x\n",
		G(r0p, EAX), G(r0p, EBX), G(r0p, ECX),
		G(r0p, EDX), G(r0p, EFL), G(r0p, DS) & 0xFFFF);
	(*prf) ("   esi:%8x edi:%8x esp:%8x ebp:%8x regset:%2d      es:%4x\n",
		G(r0p, ESI), G(r0p, EDI),
		r0p + ((ISKTRAP(r0p) ? EFL : SS) + 1) * sizeof(int),
		G(r0p, EBP), rs, G(r0p, ES) & 0xFFFF);

}


static int
db_is_after_call(addr, dst_addr_p)
	as_addr_t	addr;
	caddr_t		*dst_addr_p;
{
	u_char	opc[7], *opp;

	addr.a_addr -= 7;
	if (db_read(addr, opc, sizeof(opc)) == -1)
		return 0;
	addr.a_addr += 7;
	if (opc[2] == OPC_CALL_REL) {
		*dst_addr_p = addr.a_addr + *(caddr_t *)(opc + 3);
		if (mod_obj_validaddr((ulong)*dst_addr_p))
			return 1;
	}
	if (opc[0] == OPC_CALL_DIR) {
		*dst_addr_p = *(caddr_t *)(opc + 1);
		if (mod_obj_validaddr((ulong)*dst_addr_p))
			return 1;
	}
	for (opp = opc + 5; opp >= opc; opp--) {
		if (*opp != OPC_CALL_IND)
			continue;
		if ((opp[1] & 0x38) == 0x10) {
			*dst_addr_p = (caddr_t)0;
			return 1;
		}
	}
	return 0;
}


#define LCASEBIT	0x20

static boolean_t
xdig(char c, uint_t *valp)
{
	uint_t d;

	for (d = 16; d-- != 0;) {
		if ((c | LCASEBIT) == db_xdigit[d]) {
			*valp = d;
			return B_TRUE;
		}
	}
	return B_FALSE;
}


static boolean_t
hextoi(char **cpp, ulong_t *valp)
{
	char *cp = *cpp;
	ulong_t val;
	uint_t d;

	if (cp[0] == '0' && (cp[1] | LCASEBIT) == 'x')
		cp += 2;
	if (!xdig(*cp++, &d))
		return B_FALSE;
	val = d;
	while (xdig(*cp, &d)) {
		val = (val << 4) + d;
		++cp;
	}
	*cpp = cp;
	*valp = val;
	return B_TRUE;
}
