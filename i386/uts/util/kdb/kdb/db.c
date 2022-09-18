/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/kdb/kdb/db.c	1.7"
#ident	"$Header: $"

/*	AT&T 80x86 Kernel Debugger	*/


#include <util/types.h>
#include <util/param.h>
#include <proc/user.h>
#include <proc/reg.h>
#include <util/cmn_err.h>
#include <io/sysmsg/sysmsg.h>
#include <util/kdb/kdebugger.h>
#include <util/kdb/xdebug.h>
#include <util/debug.h>
#include <util/kdb/kdb/debugger.h>
#include <util/kdb/db_as.h>

#ifndef E425M
/* Remote Console Support */
extern struct smsg_flags smsg_flags;
static int smsg_rcef;
static int rmcsan;
#endif /* not E425M */

label_t dbsave;
int db_msg_pending;
int db_show_exit;

int dbactive;
ulong *ex_frame;	/* Pointer to exception stack frame */

int * volatile db_r0ptr;
extern int in_demon;
extern int panic_level;
extern int cpu_family;
int save_panic_level;

static int	db_reason;
static unsigned db_d6;
static unsigned	enter;
static char	*do_cmds = NULL;

struct brkinfo db_brk[MAX_BRKNUM+1];
int db_hbrk[MAX_HBRKNUM+1];
int db_brknum;
unsigned long db_brkaddr;

extern int dbsingle;
extern int db_pass_calls;

static int dbsingle_noenter;

static as_addr_t bpt3_addr;
static u_char	opc_int3 = OPC_INT3;

char *findsymname();
long findsymaddr();
void dbprintf();

static void load_pbrks(), load_hbrks(), unload_brks(), set_tmp_brk();

int db_master();

void _debugger();
struct kdebugger db_kdb = {
        _debugger
};

void
db_init()
{
	unsigned	hbrknum;

	for (hbrknum = 0; hbrknum <= MAX_HBRKNUM;)
		db_hbrk[hbrknum++] = -1;

        kdb_register(&db_kdb);
	if (cpu_family >= 5) {
		asm("movl %cr4, %eax");
		asm("orl $0x8, %eax"); 		/* enable debugging extensions */
		asm("movl %eax, %cr4");
	}
}

void
_debugger(reason, r0ptr)
	int	reason;
	int	*r0ptr;
{
	unsigned	d6;
	int		oldpri;

	if (!dbactive) {
#if !DEBUG
		if (kdb_security && (reason == DR_USER || reason == DR_OTHER))
#else
		if (kdb_security && reason == DR_USER)
#endif
			return;
		oldpri = splhi();
	} else
		splhi();

	d6 = _dr6();	/* read debug status register */
	_wdr6(0);		/* clear it for next time */
	_wdr7(0);		/* disable breakpoints to prevent recursion */

	flushtlb();

	if(debug_on_console())
		Switch2VT0Text();

	db_r0ptr = r0ptr;
	{
		if (dbactive) {
			dbprintf("\nRestarting DEBUGGER\n");
			longjmp(&dbsave);
		}

		db_d6 = d6;

		if ((db_reason = reason) == DR_SECURE_USER)
			db_reason = DR_USER;

		db_st_r0ptr = db_r0ptr;
		db_stacktrace(NULL, NULL, 1);
		ex_frame = (ulong *)(regset[0]);


		if (setjmp(&dbsave)) {
			/* make sure we don't disassemble
			   if we're restarted */
			db_reason = DR_USER;
		}

		if (db_master()) {
			/* Bail out if we had to revert to slave mode */
			splx(oldpri);
			return;
		}
	}

	load_hbrks();

	dbactive = 0;
	in_demon = 0;

	splx(oldpri);
}


int
db_master()
{
	if (!dbactive) {
		save_panic_level = panic_level;
		panic_level = 0;
		db_brknum = -1;
		unload_brks();
	}

	dbactive = 1;
	in_demon = 1;

#ifndef E425M
	/* Remote Console Support
	 *
	 * Don't have the kernel debugger write to both
	 * the alternate and the remote console.
	 * Turn off the sanity timer; we're going to be here awhile.
	 */
	if ((smsg_rcef = smsg_flags.rcef) != 0)
		smsg_flags.rcef = 0;
	rmcsan = rmcsantim(0);
#endif /* not E425M */

	if (db_reason == DR_BPT1) {  /* called due to trap 1 brkpt */
		int	hbrknum;

		db_reason = DR_OTHER;
		for (hbrknum = 0; hbrknum <= MAX_HBRKNUM; hbrknum++) {
			if (!(db_d6 & (1 << hbrknum)))
				continue;
			switch (hbrknum) {
			case 0:	db_brkaddr = _dr0();  break;
			case 1:	db_brkaddr = _dr1();  break;
			case 2:	db_brkaddr = _dr2();  break;
			case 3:	db_brkaddr = _dr3();  break;
			}
			if ((db_brknum = db_hbrk[hbrknum]) == -1)
				continue;	/* shouldn't happen */
			break;
		}
	} else if (db_reason == DR_STEP) {	/* called due to single-step */
		if (ex_frame) {
			as_addr_t	addr;
			u_char		opc;
			flags_t		efl;

			
			SET_KVIRT_ADDR(addr, ex_frame[EIP] - 1);
			db_read(addr, &opc, 1);
			if (opc == OPC_PUSHFL) {
				/* The pushfl will have pushed the single-
				   step flag bit onto the stack, so we have
				   to turn it off. */
				addr.a_addr = ex_frame[ESP] + ESP_OFFSET;
				db_read(addr, (char *)&efl, sizeof(efl));
				efl.fl_tf = 0;
				db_write(addr, (char *)&efl, sizeof(efl));
			}
		}
	}

	/* Clear temporary breakpoint */
	db_brk[TMP_BRKNUM].state = BRK_CLEAR;

	if (db_brknum == TMP_BRKNUM) {	/* From bkpt set by single-stepping */
		db_reason = DR_STEP;
		db_brknum = -1;
	}

	if (db_reason == DR_STEP) {
		if (dbsingle)
			--dbsingle;
	} else
		dbsingle = 0;

	enter = !dbsingle && !dbsingle_noenter;
	do_cmds = NULL;
	db_show_exit = 0;

	if (db_reason == DR_INIT) {
		if (*(do_cmds = debugger_init) == '\0')
			enter = 0;
	} else if (db_brknum != -1) {
		db_reason = DR_BPT1;
		db_msg_pending = 1;
		if (db_brk[db_brknum].state != BRK_ENABLED) {
			db_brk_msg(0);
			dbprintf(" - wasn't set!\n");
		} else {
			if (db_brk[db_brknum].tcount) {
				db_brk_msg(0);
				dbprintf(" (%d left)\n",
					 --db_brk[db_brknum].tcount);
				enter = 0;
			} else
				do_cmds = db_brk[db_brknum].cmds;
		}
	}

	if (db_reason == DR_BPT1 || db_reason == DR_INIT) {
		if (enter && !do_cmds)
			db_brk_msg(1);
	} else {
		if (db_reason != DR_STEP) {
			dbprintf("\nDEBUGGER:");
			if (db_reason == DR_BPT3)
				dbprintf(" Unexpected INT 3 Breakpoint!");
			dbprintf("\n");
		}
		if (db_reason != DR_USER && ex_frame != NULL &&
		    !dbsingle_noenter)
			db_show_frame();
	}
	if (enter) {
		db_show_exit = !do_cmds;
		dbinterp(do_cmds);
		if (dbsingle == 0 && db_show_exit)
			dbprintf("DEBUGGER exiting\n");
	}

	dbsingle_noenter = 0;

	if (ex_frame != NULL) {
		((flags_t *)&ex_frame[EFL])->fl_rf = 1;
		((flags_t *)&ex_frame[EFL])->fl_tf = 0;
		if (dbsingle) {
			if (db_pass_calls) {
				uint		n;

				SET_KVIRT_ADDR(bpt3_addr, ex_frame[EIP]);
				if ((n = db_is_call(bpt3_addr)) > 0)
					bpt3_addr.a_addr += n;
				else
					bpt3_addr.a_addr = 0;
			} else
				bpt3_addr.a_addr = 0;
			if (bpt3_addr.a_addr)
				set_tmp_brk(bpt3_addr.a_addr);
			else
				((flags_t *)&ex_frame[EFL])->fl_tf = 1;
		}
	}
	load_pbrks();

#ifndef E425M
	/* Remote Console Support */
	smsg_flags.rcef = smsg_rcef;	/* restore output flag */
	rmcsantim(rmcsan);
#endif /* not E425M */

	panic_level = save_panic_level;

	return 0;
}


db_show_frame()
{
	as_addr_t	addr;

	db_st_offset = 0;
	db_frameregs((ulong_t)ex_frame, 0, dbprintf);
	SET_KVIRT_ADDR(addr, ex_frame[EIP]);
	db_dis(addr, 1);
}

db_brk_msg(eol)
{
	db_msg_pending = 0;
	dbprintf("\nDEBUGGER: breakpoint %d at 0x%lx", db_brknum, db_brkaddr);
	if (eol) {
		dbprintf("\n");
		if (ex_frame)
			db_show_frame();
	}
	db_show_exit = 1;
}

dbextname(name)          /* look for external names in kernel debugger */
    char *name;
{
    if ((dbstack[dbtos].value.number = findsymaddr(name)) != NULL) {
	if (dbstackcheck(0, 1))
	    return 1;
	dbstack[dbtos].type = NUMBER;
	dbtos++;
	return 1;
    }
    return 0;   /* not found */
}

void dbtellsymname(name, addr, sym_addr)
	char	*name;
	ulong	addr, sym_addr;
{
	dbprintf("%s", name);
	if (addr != sym_addr)
		dbprintf("+0x%lx", addr - sym_addr);
	dbprintf("\n");
}


/* kernel stack dump - addresses increase right to left and bottom to top */

stackdump(dummy)
{
    ushort *tos;            /* top-of-stack pointer */
    ushort *sp;             /* stack pointer */

    tos = (ushort *)&dummy - 4;	    /* bp */
    sp = tos;                       /* for stack selector */
    *(ushort *)&sp = KSTKSZ * 2;    /* offset of high end of stack */

    do {
	sp -= 8;
	dbprintf("%4x %4x  %4x %4x  %4x %4x  %4x %4x    %4x\n",
		*(sp+7),*(sp+6),*(sp+5),*(sp+4),
		*(sp+3),*(sp+2),*(sp+1),*(sp),
		(ushort)sp);
    } while (sp > tos);
    dbprintf("bp at %x\n", (ushort)tos);
}


static u_long	brk_dr7_mask[] = {
	0xF0202, 0xF00208, 0xF000220, 0xF0000280
};

static void
load_pbrks()
{
	unsigned	brknum, hbrknum;
	struct brkinfo	*brkp;

	/* First pass: assign hardware bkpts to bkpts which require hardware */
	hbrknum = 0;
	brkp = &db_brk[brknum = MAX_BRKNUM];
	do {
		if (brkp->state != BRK_ENABLED)
			continue;
		if (brkp->type == BRK_INST)
			continue;
		if (hbrknum == MAX_HBRKNUM) {
			dbprintf("Too many data breakpoints enabled;");
			dbprintf(" #%d ignored\n", brknum);
			continue;
		}
		db_hbrk[hbrknum++] = brknum;
	} while (--brkp, brknum-- != 0);

	/* Second pass: assign remaining hardware bkpts */
	/* Load breakpoint opcodes (INT 3) after hardware bkpts used up */
	brkp = db_brk;
	for (brknum = 0; brknum <= MAX_BRKNUM; ++brkp, ++brknum) {
		if (brkp->state != BRK_ENABLED)
			continue;
		if (brkp->type != BRK_INST)
			continue;
		if (hbrknum <= MAX_HBRKNUM)
			db_hbrk[hbrknum++] = brknum;
		else if (ex_frame && ex_frame[EIP] == brkp->addr) {
			if (!((flags_t *)&ex_frame[EFL])->fl_tf) {
				((flags_t *)&ex_frame[EFL])->fl_tf = 1;
				dbsingle_noenter = 1;
			}
		} else {
			bpt3_addr.a_addr = brkp->addr;
			bpt3_addr.a_as = AS_KVIRT;
			db_read(bpt3_addr, &brkp->saved_opc, 1);
			db_write(bpt3_addr, &opc_int3, 1);
			brkp->state = BRK_LOADED;
		}
	}

	/* Clear any leftover hardware bkpts */
	while (hbrknum <= MAX_HBRKNUM)
		db_hbrk[hbrknum++] = -1;
}

static void
load_hbrks()
{
	unsigned	dr7, hbrknum;
	struct brkinfo	*brkp;
	static unsigned long	hbrkaddr[MAX_HBRKNUM + 1];

	/* Load hardware breakpoint registers */
	dr7 = 0;
	for (hbrknum = MAX_HBRKNUM + 1; hbrknum-- > 0;) {
		if (db_hbrk[hbrknum] == -1)
			continue;
		brkp = &db_brk[db_hbrk[hbrknum]];
		dr7 |= brkp->type << (16 + 4 * hbrknum);
		dr7 |= 2 << (2 * hbrknum);
		if ((brkp->type == BRK_IO) && (cpu_family < 5)) {
			dbprintf("The Intel%s CPU does not support i/o breakpoints\n",
				(cpu_family == 3) ? "386" : "486");
			continue;
		}
		if (brkp->type != BRK_INST)
			dr7 |= 0x200;
		hbrkaddr[hbrknum] = brkp->addr;
	}

	_wdr0(hbrkaddr[0]);
	_wdr1(hbrkaddr[1]);
	_wdr2(hbrkaddr[2]);
	_wdr3(hbrkaddr[3]);
	_wdr7(dr7);
}

static void
unload_brks()
{
	struct brkinfo	*brkp;

	brkp = &db_brk[MAX_BRKNUM + 1];
	while (brkp-- != db_brk) {
		if (brkp->state != BRK_LOADED)
			continue;
		bpt3_addr.a_addr = brkp->addr;
		bpt3_addr.a_as = AS_KVIRT;
		db_write(bpt3_addr, &brkp->saved_opc, 1);
		brkp->state = BRK_ENABLED;
		if (db_reason == DR_BPT3 && ex_frame &&
		    ex_frame[EIP] == brkp->addr + 1) {
			db_brknum = brkp - db_brk;
			ex_frame[EIP] = brkp->addr;
		}
	}
}

static void
set_tmp_brk(addr)
{
	db_brk[TMP_BRKNUM].addr = addr;
	db_brk[TMP_BRKNUM].type = BRK_INST;
	db_brk[TMP_BRKNUM].state = BRK_ENABLED;

	/* tcount and cmds are never set for TMP_BRKNUM */
}


int
db_is_call(addr)
	as_addr_t	addr;
{
	u_char	opc;

	if (db_read(addr, &opc, sizeof(opc)) == -1)
		return 0;
	switch (opc) {
	case OPC_CALL_REL:
		return 5;
	case OPC_CALL_DIR:
		return 7;
	}
	return 0;
}
