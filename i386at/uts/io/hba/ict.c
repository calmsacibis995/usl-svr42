/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/hba/ict.c	1.16"
#ident	"$Header: $"

#include "util/sysmacros.h"
#include "fs/buf.h"
#include "util/cmn_err.h"
#include "io/vtoc.h"
#include "io/target/sdi_edt.h"
#include "io/target/sdi.h"
#include "io/hba/dcd.h"
#include "io/target/scsi.h"
#include "io/hba/gendev.h"
#include "io/target/st01.h"
#include "io/i8237A.h"
#include "io/hba/ict.h"
#include "svc/sysenvmt.h"
#include <util/mod/moddefs.h>

MOD_MISC_WRAPPER(ict_, NULL, NULL, "ict - loadable DCD tape driver");

char ict_cntrl_mask = '\0';
struct ICT_CONTROL ict_cntrl;
unsigned char	ict_status[ ICT_STATBUF_SZ ];
unsigned char	ict_status_new[ ICT_STATBUF_SZ ];
unchar ict_cntr_status;
int ict_init_time = 0;
int ict_datardy = 0;
int ict_statrdy = 0;
int ict_dma_stat = 0;
int ict_newstatus = 0;
int ict_at_eom = 0;
int ict_at_filemark = 0;
int ict_at_bot = 1;
int ict_nocartridge = 0;
int ict_reles = 0;
int ict_sleepflag = 0;
paddr_t ict_mem_addr;

#ifdef ICTDEBUG
int ict_print = 0;
#endif

#ifdef ICTSTAT
struct ICT_STAT ict_stat;
#endif

#define NEXCPTS	14		/* Number of exceptions */
struct ict_ex {
	int	type;
	int	byte0;
	int	byte1;
	int	mask0;
	int	mask1;
	int	drverror;
	char	*errtext;
} ict_ex[] = {
	{ICT_NCT, 0xc0, 0x00, 0xef, 0x00, 10, "No Cartridge"},
	{ICT_EOM, 0x88, 0x00, 0xff, 0x00, 26, "End of Media"},
	{ICT_EOF, 0x81, 0x00, 0xe7, 0x00, 26, "Read a Filemark"},
	{ICT_WRP, 0x90, 0x00, 0xff, 0x77,  9, "Write Protected"},
	{ICT_DFF, 0x20, 0x00, 0xff, 0xff, 13, "Device Fault Flag"},
	{ICT_RWA, 0x84, 0x88, 0xef, 0xff, 13, "Read or Write Abort"},
	{ICT_BBX, 0x84, 0x00, 0xef, 0xff, 13, "Read Error, Bad Block Xfer"},
	{ICT_FBX, 0x86, 0x00, 0xef, 0xff, 13, "Read Error, Filler Block Xfer"},
	{ICT_NDT, 0x86, 0xa0, 0xef, 0xff, 13, "Read Error, No Data"},
	{ICT_NDE, 0x8e, 0xa0, 0xef, 0xff, 13, "Read Error, No Data & EOM"},
	{ICT_ILC, 0x00, 0xc0, 0x0f, 0xf7, 16, "Illegal Command"},
	{ICT_PRR, 0x00, 0x81, 0x0f, 0xf7,  0, "Power On/Reset"},
	{ICT_MBD, 0x81, 0x10, 0xef, 0xff, 13, "Marginal Block Detected"},
	{ICT_UND, 0x00, 0x00, 0x00, 0x00, 25, "Undetermined Error"}
};

ict_bdinit(cfgp, dcbp)
register struct gdev_cfg_entry *cfgp;
gdev_dcbp dcbp;
{
	int	contr_type;

	ict_init_time = 1;

#ifdef ICTSTAT
	ict_stat.ict_savecmd = 0;
	ict_stat.ict_drvcmd = 0;
	ict_stat.ict_savecmd = 0;
	ict_stat.ict_errabort = 0;
	ict_stat.ict_writecnt = 0;
	ict_stat.ict_readcnt = 0;
	ict_stat.ict_nblocks = 0;
	ict_stat.ict_prev_nblocks = 0;
#endif

	/***                                              ****
	** Initialize the non-controller specific ict_cntrl **
	** structure fields.                                **
	****                                              ***/

	/***                    ****
	** Control Port Bit Masks **
	****                    ***/
	ict_cntrl.ict_online	= 0x01;
	ict_cntrl.ict_dma1_2	= 0x08;

	/***                         ****
	** Status Port Bit Definitions **
	****                         ***/
	ict_cntrl.ict_direction	= 0x04;

	/***                                            ****
	** The following entries are specific to the      **
	** supported controllers, and will be given       **
	** appropriate values once the type of controller **
	** has been determined.                           **
	****                                            ***/
	ict_cntrl.ict_type	= ICT_WANGTEK;

	for( contr_type = 0; contr_type < 2; ++contr_type ) {

		if( contr_type == ICT_WANGTEK ) {
#ifdef ICTDEBUG
			printf("ICT: Searching for a Wangtek or MCA Archive controller ... ");
#endif
			/***            ****
			** Port Addresses **
			****            ***/
			ict_cntrl.ict_status = cfgp->cfg_ioaddr1;
			ict_cntrl.ict_control = cfgp->cfg_ioaddr1;
			ict_cntrl.ict_command = cfgp->cfg_ioaddr1 + 1;
			ict_cntrl.ict_data = cfgp->cfg_ioaddr1 + 1;

			ict_cntrl.ict_reset	= 0x02;
			ict_cntrl.ict_request	= 0x04;
			ict_cntrl.ict_intr_enable = 0x8;

			ict_cntrl.ict_ready	= 0x01;
			ict_cntrl.ict_exception	= 0x02;

			ict_cntrl.ict_dma_go	= 0;
			ict_cntrl.ict_reset_dma	= 0;

			/***                  ****
			** Power On Reset Delay **
			****                  ***/
			ict_cntrl.ict_por_delay	= 1000000;

			if( sysenvmtp->machflags & MC_BUS ) {
				ict_cntrl.ict_intr_enable = 0x48;
				ict_cntrl.ict_dma_enable = 0x9;
				contr_type = ICT_MCA_ARCHIVE;
			}
		}
		else if( contr_type == ICT_ARCHIVE ) {
#ifdef ICTDEBUG
			printf("ICT: Searching for an Archive controller ... ");
#endif
			/***            ****
			** Port Addresses **
			****            ***/
			ict_cntrl.ict_status = cfgp->cfg_ioaddr1 + 1;
			ict_cntrl.ict_control = cfgp->cfg_ioaddr1 + 1;
			ict_cntrl.ict_command = cfgp->cfg_ioaddr1;
			ict_cntrl.ict_data = cfgp->cfg_ioaddr1;

			ict_cntrl.ict_reset	= 0x80;
			ict_cntrl.ict_request = 0x40;
			ict_cntrl.ict_intr_enable = 0x20;

			ict_cntrl.ict_ready	= 0x40;
			ict_cntrl.ict_exception	= 0x20;

			ict_cntrl.ict_dma_go	= cfgp->cfg_ioaddr1 + 2;
			ict_cntrl.ict_reset_dma	= cfgp->cfg_ioaddr1 + 3;

			/***                  ****
			** Power On Reset Delay **
			****                  ***/
			ict_cntrl.ict_por_delay	= 5000000;
		}

		ICT_DEASSERT( ict_cntrl.ict_intr_enable );

		if( ict_cntrl.ict_type == ICT_WANGTEK ) {
			ICT_DEASSERT( ict_cntrl.ict_online );
		}

		ICT_ASSERT( ict_cntrl.ict_reset );

		drv_usecwait( 250 );

		ICT_DEASSERT( ict_cntrl.ict_reset );

		drv_usecwait( ict_cntrl.ict_por_delay );

		if( ict_rdstatus() == ICT_SUCCESS ) {
			if( ict_status[ 1 ] & ( ICT_POR | ICT_SBYTE1 ) ) {
#ifdef ICTDEBUG
				printf("Found\n");
#endif
				ict_cntrl.ict_type = contr_type;

				if( contr_type == ICT_ARCHIVE ) {
					outb( ict_cntrl.ict_reset_dma, 0x01 );
				}
				return( 1 );
				break;
			}
		}
		else {
#ifdef ICTDEBUG
			printf("Not Found\n");
#endif
		}
	}

#ifdef ICTDEBUG
	cmn_err(CE_NOTE,"ICT: No tape devices found.\n");
#endif

	return( 0 );
}

ict_drvinit(dcbp, dpbp)
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	dpbp->dpb_drvflags = 0;
	dpbp->dpb_flags = DFLG_FIXREC;
	dpbp->dpb_devtype = DTYP_TAPE;

	dpbp->dpb_cyls = 24;
	dpbp->dpb_rescyls = 0;
	dpbp->dpb_heads = 1;
	dpbp->dpb_sectors = 65536;
	dpbp->dpb_ressecs = 0;
	dpbp->dpb_secsiz = 512;
	dpbp->dpb_secovhd = 0;
	dpbp->dpb_pcyls = 15;
	dpbp->dpb_pheads = 1;
	dpbp->dpb_psectors = 65536;
	dpbp->dpb_pbytes = 512;
	dpbp->dpb_interleave = 1;
	dpbp->dpb_skew = 0;
	dpbp->dpb_secbufsiz = 16;

	dpbp->dpb_inqdata.id_type = ID_TAPE;
	dpbp->dpb_inqdata.id_pqual = ID_QOK;
	dpbp->dpb_inqdata.id_qualif = 0;
	dpbp->dpb_inqdata.id_rmb = 0;
	dpbp->dpb_inqdata.id_ver = 0x1;
	dpbp->dpb_inqdata.id_len = 31;

	strncpy(dpbp->dpb_inqdata.id_vendor, "(ict)   ", VID_LEN);
	strncpy(dpbp->dpb_inqdata.id_prod, "Non-SCSI QICTAPE", PID_LEN);
	strncpy(dpbp->dpb_inqdata.id_revnum, "1.00", REV_LEN);

	ict_init_time = 0;

	return;
}

int
ict_cmd(cmd, dcbp, dpbp)
int	cmd;
register gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int	ospl;

	int i;
	int resid;
	dcdblk_t *bp;
	struct drq_entry *drqp;
	struct mode *mp;
	struct sense *sp;
	gdev_dpbp ict_chk_excpt();


	if( ict_nocartridge ) {
		if( ict_cntrl.ict_type == ICT_WANGTEK ) {
			ICT_DEASSERT( ict_cntrl.ict_online );
		}

		ICT_ASSERT( ict_cntrl.ict_reset );

		drv_usecwait( 250 );

		ICT_DEASSERT( ict_cntrl.ict_reset );

		drv_usecwait( ict_cntrl.ict_por_delay );

		ict_at_bot = 1;
		ict_reles = 1;
		ict_nocartridge = 0;
		dpbp->dpb_drverror = 0;
#ifdef ICTSTAT
		ict_stat.ict_savecmd = 0;
#endif
		dpbp->dpb_savecmd = 0;
	}

	if( cmd != DCMD_LOAD )
		ict_status_upd( dpbp );

	if( ict_nocartridge &&
	  (cmd != DCMD_READ && cmd != DCMD_WRITE && cmd != DCMD_RELES))
		return ( 1 );

	switch (cmd) {

	case DCMD_READ:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_READ\n");
		}
#endif
		dpbp->dpb_drvcmd = ICT_READ;
#ifdef ICTSTAT
		ict_stat.ict_drvcmd = ICT_READ;
#endif
		dpbp->dpb_state = DSTA_NORMIO;
		if( ict_at_filemark || ict_nocartridge ) {
			ict_gen_intr(dpbp);
			return( 1 );
		}
		ict_mem_addr = dpbp->dpb_curaddr;
		break;

	case DCMD_WRITE:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_WRITE\n");
		}
#endif
		if( ict_at_eom || ict_nocartridge ) {
			ict_gen_intr(dpbp);
			return( 1 );
		}
		dpbp->dpb_drvcmd = ICT_WRITE;
#ifdef ICTSTAT
		ict_stat.ict_drvcmd = ICT_WRITE;
#endif
		dpbp->dpb_state = DSTA_NORMIO;
		ict_mem_addr = dpbp->dpb_curaddr;
		break;

	case DCMD_ERASE:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_ERASE\n");
		}
#endif
		dpbp->dpb_drvcmd = ICT_ERASE;
#ifdef ICTSTAT
		ict_stat.ict_drvcmd = ICT_ERASE;
#endif
		dpbp->dpb_state = DSTA_RECAL;
		ict_mem_addr = (paddr_t)0;
		break;

	case DCMD_RETENSION:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_RETENSION\n");
		}
#endif
		dpbp->dpb_drvcmd = ICT_RETENSION;
#ifdef ICTSTAT
		ict_stat.ict_drvcmd = ICT_RETENSION;
#endif
		dpbp->dpb_state = DSTA_RECAL;
		ict_mem_addr = (paddr_t)0;
		break;

	case DCMD_WFM:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCDM_WFM\n");
		}
#endif

		dpbp->dpb_drvcmd = ICT_WRFILEM;
#ifdef ICTSTAT
		ict_stat.ict_drvcmd = ICT_WRFILEM;
#endif
		dpbp->dpb_state = DSTA_RECAL;
		ict_mem_addr = (paddr_t)0;
		break;

	case DCMD_REWIND:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_REWIND\n");
		}
#endif
		ict_mem_addr = (paddr_t)0;
		if( dpbp->dpb_savecmd == ICT_READ &&
		    ict_cntrl.ict_type == ICT_WANGTEK ) {

			/***                                          ***
			** WorkAround for a WANGETK Conrtoller Problem **
			***                                           ***
			** Some Wangtek controllers will not execute   **
			** a REWIND command if the last command was    **
			** a READ. This logic forces the REWIND by     **
			** dropping the ONLINE bit.                    **

			***                                           **/
			ict_datardy = 0;

			/***                                           ***
			** Someone may have popped the tape, in which   **
			** we will never get an interrupt from the      **
			** implied REWIND due to dropping ONLINE, since **
			** there would be no media in this case. Thus,  **
			** we issue a READSTATUS which will generate    **
			** an exception if there is no media present.   **
			***                                            **/
			if( ict_rdstatus() == ICT_FAILURE ) {
				ict_nocartridge = 1;
				return( 1 );
			}

			ICT_DEASSERT( ict_cntrl.ict_online );
			ICT_DEASSERT( ict_cntrl.ict_intr_enable );
			dpbp->dpb_savecmd = dpbp->dpb_drvcmd = ICT_REWIND;
#ifdef ICTSTAT
			ict_stat.ict_savecmd = ICT_REWIND;
#endif
			dpbp->dpb_state = DSTA_RECAL;
			drv_usecwait( 10 );
			ICT_ASSERT( ict_cntrl.ict_intr_enable );
			ict_at_bot = 1;
			return( 0 );
		}
		else {
			dpbp->dpb_drvcmd = ICT_REWIND;
#ifdef ICTSTAT
			ict_stat.ict_savecmd = ICT_REWIND;
#endif
			dpbp->dpb_state = DSTA_RECAL;
		}

		break;

	case DCMD_SEOF:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_SEOF\n");
		}
#endif
		dpbp->dpb_drvcmd = ICT_RDFILEM;
#ifdef ICTSTAT
		ict_stat.ict_savecmd = ICT_RDFILEM;
#endif
		dpbp->dpb_state = DSTA_RECAL;
		ict_mem_addr = (paddr_t)0;
		break;

	case DCMD_LOAD:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_LOAD\n");
		}
#endif
		ict_mem_addr = (paddr_t)0;
		if( ict_rdstatus() == ICT_FAILURE ) {
			ict_nocartridge = 1;
			return( 1 );
		}

		if( ict_status[ 0 ] & 0x40 ) {
			/***                     ****
			**  Cartidge Not In Place  **
			***                       ***
			**      Try it again       **
			** for Archive 2150L drive **
			****                     ***/
			if( ict_rdstatus() == ICT_FAILURE ) {
				ict_nocartridge = 1;
				return( 1 );
			}
		}

		if( ict_status[ 0 ] & 0x40 ) {
			/***                   ****
			** Cartidge Not In Place **
			****                   ***/
			ict_nocartridge = 1;
			return( 1 );
		}

		ict_reles = 0;
		return( 0 );

	case DCMD_REQSEN:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_REQSEN\n");
		}
#endif
		/***                                             ****
		** Currently a RDSTATUS is not being issued        **
		** since the only time we __SHOULD__ be getting    **
		** a REQSENSE command is after we report an        **
		** exception back up the chain, and we will have   **
		** done the RDSTATUS at the time of the exception. **
		** If we need to handle REQSENSE commands in cases **
		** other than exceptions, a RDSTATUS will have to  **
		** be issued here.                                 **
		**                                               ***/

		ict_mem_addr = (paddr_t)0;
		drqp = dpbp->dpb_req;

		sp = &((struct tape *)(drqp->drq_addr1))->t_sense;
		sp->sd_errc = 0x70;
		sp->sd_valid = 1;
		sp->sd_ili = 0;
		sp->sd_key = SD_NOSENSE;
		sp->sd_eom = ict_at_eom;
		sp->sd_fm = ict_at_filemark;
#ifdef ICTDEBUG
		if( ict_print == 1) {
			printf("ICT_CMD: ict_at_filemark %d ict_at_eom %d\n",
				ict_at_filemark,ict_at_eom);
			printf("ICT_CMD: resid(dpb_sectcount) = 0x%x\n",
				dpbp->dpb_sectcount); 
		}
#endif
		sp->sd_ba = sdi_swap32(dpbp->dpb_sectcount);
		sp->sd_len = 0x0e;
		sp->sd_sencode = SC_IDERR;

		if( ict_newstatus ) {
			for( i = 0; i < ICT_STATBUF_SZ; ++i ) {
				ict_status[ i ] = ict_status_new[ i ];
			}
			ict_newstatus = 0;
		}
		ict_status_upd( dpbp );
		return( 0 );
		break;

	case DCMD_MSENSE:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_MSENSE\n");
		}
#endif
		ict_mem_addr = (paddr_t)0;
		drqp = dpbp->dpb_req;
		mp = (struct mode *)drqp->drq_addr1;
		mp->md_len = 0x0;
		mp->md_media = 0x0;
		mp->md_speed = 0x0;
		mp->md_bm = 0x0;
		mp->md_bdl = 0x0;
		mp->md_dens = 0x0;
		mp->md_nblks = 0x0;
		mp->md_res = 0x0;
		mp->md_bsize = (unsigned)0x200;

		if( ict_rdstatus() == ICT_FAILURE ) {
			return( 1 );
		}

		if( ict_status[ 0 ] & 0x10 ) {
			mp->md_wp = 1;
		}
		else {
			mp->md_wp = 0;
		}

		return( 0 );
		break;

	case DCMD_RELES:
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CMD: DCMD_RELES\n");
		}
#endif
		dpbp->dpb_savecmd = dpbp->dpb_drvcmd = 0;
#ifdef ICTSTAT
		ict_stat.ict_prev_nblocks = ict_stat.ict_nblocks;
		ict_stat.ict_nblocks = 0;
		ict_stat.ict_savecmd = 0;
#endif
		ict_reles = 1;

		if( !ict_at_eom )
			return( 0 );
		/* else do a reset */

	case DCMD_RESET:
#ifdef ICTDEBUG
		if( ict_print == 1  &&  cmd == DCMD_RESET ) {
			printf("ICT_CMD: DCMD_RESET\n");
		}
#endif

		ict_datardy = 0;
		ict_mem_addr = (paddr_t)0;
		if( ict_cntrl.ict_type == ICT_WANGTEK ) {
			ICT_DEASSERT( ict_cntrl.ict_online );
		}

		ICT_ASSERT( ict_cntrl.ict_reset );

		drv_usecwait( 250 );

		ICT_DEASSERT( ict_cntrl.ict_reset );

		drv_usecwait( ict_cntrl.ict_por_delay );

		if( ict_rdstatus() == ICT_SUCCESS ) {
			if( ict_status[ 1 ] & ( ICT_POR | ICT_SBYTE1 ) ) {
				ict_at_bot = 1;
				return( 0 );
			}
			else {
				return( 1 );
			}
		}
		else {
			return( 1 );
		}

	case DCMD_SETPARMS:
		ict_mem_addr = (paddr_t)0;
		return( 0 );

	default:
		ict_mem_addr = (paddr_t)0;
		cmn_err(CE_NOTE,"ict_cmd: Unknown command [%d]\n",cmd);
		return( 0 );
	}

	if( ict_docmd( dcbp, dpbp ) == 0 ) {
		dpbp->dpb_savecmd = dpbp->dpb_drvcmd;
#ifdef ICTSTAT
		ict_stat.ict_drvcmd = dpbp->dpb_drvcmd;
#endif
		return( 0 );
	}
	else {
		if( cmd == DCMD_READ || cmd == DCMD_WRITE ) {
			ict_gen_intr(dpbp);
		}

#ifdef ICTSTAT
		ict_stat.ict_savecmd = 0;
#endif
		dpbp->dpb_savecmd = 0;
		return( 1 );
	}
}

struct gdev_parm_block *
ict_int(dcbp, intidx)
gdev_dcbp dcbp;
int	intidx;
{
	gdev_dpbp ret;
	gdev_dpbp dpbp;
	struct drq_entry *drqp;
	gdev_dpbp ict_chk_excpt();

#ifdef ICTDEBUG
	if( ict_print == 1 ) {
		printf("ICT_INT\n");
	}
#endif

	dpbp = dcbp->dcb_dpbs[dcb_curdriv(dcbp)+dcb_firstdriv(dcbp)];

	if (( ret = ict_chk_excpt(dpbp)) != (gdev_dpbp)1 ) {
		ict_datardy = 0;
		return (ret);
	} 

	if( ict_statrdy ) {
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_INT: ict_rdstat_rdy DINT_ERRABORT\n");
		}
#endif
		ict_datardy = 0;
		ict_statrdy = 0;
		ict_rdstat_rdy();
		dpbp->dpb_intret = DINT_ERRABORT;
		return (dpbp);
	}

	if( ict_datardy ) {
		/*
		** Special case, first read/write interrupt		**
		** indicating device is ready for data transfer.				*/
		if( ict_datardy == DMA_WRITE ) {
			ict_datardy = 0;
			wakeup( &ict_sleepflag );
			return (NULL);
		}
		ict_setdma( dpbp, ict_datardy );
		ict_datardy = 0;
		return (NULL);
	}

	if( ( dcbp->dcb_flags & CFLG_BUSY) == 0 ) {
		/***                                             ****
		** Stray interrupt since we're not doing anything. **
		****                                             ***/
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_INT: Stray\n");
		}
#endif
		return( NULL );
	}

	/***
	** If a DMA job was in progress, mask out DMA channel 1
	***/
	if( ict_cntrl.ict_type != ICT_MCA_ARCHIVE &&
	  ( dpbp->dpb_drvcmd == ICT_WRITE ||
	    dpbp->dpb_drvcmd == ICT_READ )) {

		outb( DMA_STR, 0x05 );
	}

	/***                                         ****
	** Deassert Interrupt Enable on the controller **
	****                                         ***/
	ICT_DEASSERT( ict_cntrl.ict_intr_enable );

	switch ( (int)dpbp->dpb_state ) {

	case DSTA_RECAL:

		drqp = dpbp->dpb_req;

		ict_at_bot = (dpbp->dpb_drvcmd == ICT_REWIND ||
			      dpbp->dpb_drvcmd == ICT_ERASE) ? 1:0; 

		if( dpbp->dpb_drvcmd == ICT_WRFILEM ||
		    dpbp->dpb_drvcmd == ICT_RDFILEM ) {

			if( dpbp->dpb_drvcmd == ICT_RDFILEM ) {
				ict_clrstatus( ict_status );
				ict_at_filemark = 0;
			}

			--drqp->drq_count;

			if( (int)drqp->drq_count > (int)0 ) {
				if( ict_docmd( dcbp, dpbp ) == 0 ) {
					dpbp->dpb_intret = DINT_COMPLETE;
					return( NULL );
				}
				else {
					dpbp->dpb_intret = DINT_GENERROR;
					cmn_err(CE_WARN,"ICT_INT: Unable to write filemark.");
				}
			}
			else {
				dpbp->dpb_intret = DINT_COMPLETE;
			}
		}
		else {
			dpbp->dpb_intret = DINT_COMPLETE;
		}
		break;

	case DSTA_NORMIO:

		dcbp->dcb_xferok( dpbp, 1 );
#ifdef ICTSTAT
		ict_stat.ict_nblocks++;
#endif

		ict_at_bot = 0;
		if( dpbp->dpb_drverror == 0 ) {
			if( dpbp->dpb_intret == DINT_CONTINUE ) {
				if( dpbp->dpb_drvcmd == ICT_WRITE ) {
#ifdef ICTSTAT
					++ict_stat.ict_writecnt;
#endif
					ict_setdma( dpbp, DMA_WRITE );
				}
				else {
#ifdef ICTSTAT
					++ict_stat.ict_readcnt;
#endif
					ict_setdma( dpbp, DMA_READ );
				}
			}
		}
		else {
#ifdef ICTDEBUG
			if( ict_print == 1 ) {
				printf("ICT_INT: NORMIO DINT_ERRABORT\n");
			}
#endif
#ifdef ICTSTAT
			++ict_stat.ict_errabort;
#endif
			dpbp->dpb_intret = DINT_ERRABORT;
		}

		break;
	default:
#ifdef ICTSTAT
		++ict_stat.ict_errabort;
#endif
		dpbp->dpb_intret = DINT_ERRABORT;
		cmn_err(CE_WARN,"ict_int: Invalid State - 0x%x",dpbp->dpb_state);
		break;
	}

	return( dpbp );
}

ict_docmd(dcbp, dpbp)
gdev_dcbp dcbp;
register gdev_dpbp dpbp;
{
	int time_id;
	int ret = 0;
	int ict_timeout();

	ict_datardy = 0;
	if( dpbp->dpb_drvcmd == ICT_WRITE &&
		dpbp->dpb_savecmd == ICT_WRITE ) {
#ifdef ICTSTAT
		++ict_stat.ict_writecnt;
#endif
		ict_setdma( dpbp, DMA_WRITE );
		return( 0 );
	}

	if( dpbp->dpb_drvcmd == ICT_READ &&
	    dpbp->dpb_savecmd == ICT_READ &&
	    ict_at_filemark == 0 &&
	    ict_at_eom == 0 ) {
#ifdef ICTSTAT
		++ict_stat.ict_readcnt;
#endif
		ict_setdma( dpbp, DMA_READ );
		return( 0 );
	}

	if( ict_rdy_wait( ) == ICT_FAILURE ) {
		dpbp->dpb_drverror = ICT_NOT_READY;
		dpbp->dpb_intret = DINT_ERRABORT;
		return( 1 );
	}

	if( (ict_cntrl.ict_type == ICT_WANGTEK || 
	     ict_cntrl.ict_type == ICT_MCA_ARCHIVE) &&
	    ( dpbp->dpb_drvcmd == ICT_READ ||
	      dpbp->dpb_drvcmd == ICT_WRITE || 
	      dpbp->dpb_drvcmd == ICT_RDFILEM ||
	      dpbp->dpb_drvcmd == ICT_WRFILEM ) ) {

		ICT_ASSERT( ict_cntrl.ict_online );
	}

	outb( ict_cntrl.ict_command, dpbp->dpb_drvcmd );

	ICT_ASSERT( ict_cntrl.ict_request );

	if( ict_rdy_wait( ) == ICT_FAILURE ) {
		dpbp->dpb_drverror = ICT_NOT_READY;
		dpbp->dpb_intret = DINT_ERRABORT;
		return( 1 );
	}

	if( dpbp->dpb_drvcmd == ICT_WRITE ) {

#ifdef ICTSTAT
		++ict_stat.ict_writecnt;
#endif
                ICT_DEASSERT( ict_cntrl.ict_request );

                if( ict_nrdy_wait( ) == ICT_FAILURE ) {
                        dpbp->dpb_drverror = ICT_NOT_READY;
                        dpbp->dpb_intret = DINT_ERRABORT;
                        return( 1 );
                }

                if( ict_rdy_wait( ) == ICT_FAILURE ) {
			ict_datardy = DMA_WRITE;
			if( ict_at_bot ) {
				ict_sleepflag = 1;
				time_id = timeout( ict_timeout, 0, ICT_WR_MAXTOUT*HZ );
				ICT_ASSERT( ict_cntrl.ict_intr_enable );

				while( ict_sleepflag ) {
					if(ret = sleep(&ict_sleepflag, PCATCH|(PZERO+1) ))
						ict_sleepflag = 0;
				}
				untimeout( time_id );
			}
			if( ict_datardy || ret ) {
                        	dpbp->dpb_drverror = ICT_NOT_READY;
                        	dpbp->dpb_intret = DINT_ERRABORT;
				ict_datardy = 0;
				return( 1 );
			}
		}
		ict_setdma( dpbp, DMA_WRITE );
	}
	else if( dpbp->dpb_drvcmd == ICT_READ ) {

#ifdef ICTSTAT
		++ict_stat.ict_readcnt;
#endif
		ICT_DEASSERT( ict_cntrl.ict_request );

		if( ict_nrdy_wait( ) == ICT_FAILURE ) {
			dpbp->dpb_drverror = ICT_NOT_READY;
			dpbp->dpb_intret = DINT_ERRABORT;
			return( 1 );
		}

		ict_datardy = DMA_READ;
		ICT_ASSERT( ict_cntrl.ict_intr_enable );
	}
	else {
		ICT_ASSERT( ict_cntrl.ict_intr_enable );
		ICT_DEASSERT( ict_cntrl.ict_request );

	}

	dpbp->dpb_drverror = 0;

	return( 0 );
}

ict_timeout( arg )
int arg;
{
	ict_sleepflag = 0;
	wakeup( &ict_sleepflag );
}

ict_rdstatus()
{
	register int i;

	ICT_DEASSERT( ict_cntrl.ict_intr_enable );

	ict_clrstatus( ict_status);

	outb( ict_cntrl.ict_command, ICT_RD_STATUS );

	ICT_ASSERT( ict_cntrl.ict_request );

	if( ict_excpt_clr() == ICT_FAILURE ) {
		return( ICT_FAILURE );
	}

	if( ict_rdy_wait( ) == ICT_FAILURE ) {
		return( ICT_FAILURE );
	}

	ICT_DEASSERT( ict_cntrl.ict_request );

	if( ict_nrdy_wait( ) == ICT_FAILURE ) {
		return( ICT_FAILURE );
	}

	for( i = 0; i < 6; ++i ) {

		if( ict_rdy_wait( ) == ICT_FAILURE ) {
			return( ICT_FAILURE );
		}

		ict_status[ i ] = inb( ict_cntrl.ict_data );

		ICT_ASSERT( ict_cntrl.ict_request );

		if( ict_nrdy_wait( ) == ICT_FAILURE ) {
			ICT_DEASSERT( ict_cntrl.ict_request );
			return( ICT_FAILURE );
		}

		ICT_DEASSERT( ict_cntrl.ict_request );
	}

	return( ICT_SUCCESS );
}

int
ict_rdy_wait( )
{
	register int	i;

	for( i = 0; i < ICT_WAIT_LIMIT; ++i ) {
		if( (inb( ict_cntrl.ict_status ) & ict_cntrl.ict_ready) ) {
			drv_usecwait( 10 );
		}
		else {
			return( ICT_SUCCESS );
		}
	}

	return( ICT_FAILURE );
}

int
ict_nrdy_wait( )
{
	register int	i;

	for( i = 0; i < ICT_WAIT_LIMIT; ++i ) {
		if( (inb( ict_cntrl.ict_status ) & ict_cntrl.ict_ready) ) {
			return( ICT_SUCCESS );
		}
		else {
			drv_usecwait( 10 );
		}
	}

	return( ICT_FAILURE );
}

int
ict_excpt_clr()
{
	register int	i;

	for( i = 0; i < ICT_WAIT_LIMIT; ++i ) {
		if( (inb( ict_cntrl.ict_status ) & ict_cntrl.ict_exception) ) {
			break;
		}
		else {
			drv_usecwait( 20 );
		}
	}

	if( i >= ICT_WAIT_LIMIT ) {
		return( ICT_FAILURE );
	}
	else {
		return( ICT_SUCCESS );
	}
}

ict_setdma( dpbp, rd_wr_flag )
register gdev_dpbp dpbp;
char	rd_wr_flag;
{
	char	b1_addr;
	char	b2_addr;
	char	b3_addr;
	char	b1_len;
	char	b2_len;

	b1_addr = (char)(ict_mem_addr & 0xFF);
	b2_addr = (char)((ict_mem_addr >> 8) & 0xFF);
	b3_addr = (char)((ict_mem_addr >> 16) & 0xFF);
	b1_len = (char)((512 - 1) & 0xFF);
	b2_len = (char)(((512 - 1) >> 8) & 0xFF);

	asm("cli");

	outb( DMA_STR, 0x05 );

	outb(DMA1CBPFF, (char)0);
	outb(DMA1WMR, rd_wr_flag);
	outb(DMA1BCA1, b1_addr);
	outb(DMA1BCA1, b2_addr);
	outb(DMACH1PG, b3_addr);
	outb(DMA1BCWC1, b1_len);
	outb(DMA1BCWC1, b2_len);

	asm("sti");

	switch( ict_cntrl.ict_type ) {
	case ICT_WANGTEK:
		ICT_ASSERT( ict_cntrl.ict_intr_enable );
		outb( DMA_STR, 0x01 );
		break;
	case ICT_ARCHIVE:
		ICT_ASSERT( ict_cntrl.ict_intr_enable );
		outb( ict_cntrl.ict_dma_go, 0x01 );
		outb( DMA_STR, 0x01 );
		break;
	case ICT_MCA_ARCHIVE:
		outb( DMA_STR, 0x01 );
		outb( ict_cntrl.ict_control, ict_cntrl.ict_dma_enable);
		break;
	}

	ict_dma_stat = inb( DMA_STAT );

	ict_mem_addr += 512;
	return( 0 );
}

struct gdev_parm_block *
ict_excpt_hndlr(dpbp)
register gdev_dpbp dpbp;
{
	int i;
	int exception;
	gdev_dpbp ret = dpbp;

#ifdef ICTDEBUG
	if( ict_print == 1 ) {
		printf("ICT_EXCPT_HNDLR: RDSTATUS !\n");
	}
#endif

	/***                                        ****
	** Read the controller's status register file **
	****                                        ***/
	ict_rdstatus();

	if( ict_init_time == 1 ) {
#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_EXCPT: Init time exception!\n");
		}
#endif
		return( NULL );
	}

	dpbp->dpb_intret = DINT_ERRABORT;

	for(i=0; i<NEXCPTS; i++) {
		if((ict_status[0] & ict_ex[i].mask0) == ict_ex[i].byte0  &&
		   (ict_status[1] & ict_ex[i].mask1) == ict_ex[i].byte1) {
			exception = ict_ex[i].type;
			dpbp->dpb_drverror = ict_ex[i].drverror;
			break;
		}
	}

#ifdef ICTDEBUG
	if( ict_print == 1 ) {
		printf("ICT_EXCPT: %s\n", ict_ex[i].errtext);
	}
#endif
	switch (exception) {
	case ICT_EOM:
		if( ict_at_eom == 0 ) {
			ict_at_eom = 1;
			return((gdev_dpbp)1);
		}
		break;
	case ICT_EOF:
		if( ict_at_filemark == 0 ) {
			ict_at_filemark = 1;
			return((gdev_dpbp)1);
		}
		break;
	case ICT_NCT:
		ict_nocartridge = 1;
		break;
	}

	if( ict_datardy ) {
		ict_nocartridge = 1;
	}
		
	return( ret );
}

ict_rdstat_setup()
{
	ICT_DEASSERT( ict_cntrl.ict_intr_enable );

	ict_clrstatus( ict_status_new );

	outb( ict_cntrl.ict_command, ICT_RD_STATUS );

	ICT_ASSERT( ict_cntrl.ict_request );

	if( ict_excpt_clr() == ICT_FAILURE ) {
		return( ICT_FAILURE );
	}

	if( ict_rdy_wait( ) == ICT_FAILURE ) {
		return( ICT_FAILURE );
	}

	ICT_DEASSERT( ict_cntrl.ict_request );

	if( ict_nrdy_wait( ) == ICT_FAILURE ) {
		return( ICT_FAILURE );
	}
	ICT_ASSERT( ict_cntrl.ict_intr_enable );
	return;
}

ict_rdstat_rdy()
{
	register int i;

	ICT_DEASSERT( ict_cntrl.ict_intr_enable );

	for( i = 0; i < 6; ++i ) {

		if( ict_rdy_wait( ) == ICT_FAILURE ) {
			return( ICT_FAILURE );
		}

		ict_status_new[ i ] = inb( ict_cntrl.ict_data );

		ICT_ASSERT( ict_cntrl.ict_request );

		if( ict_nrdy_wait( ) == ICT_FAILURE ) {
			ICT_DEASSERT( ict_cntrl.ict_request );
			return( ICT_FAILURE );
		}

		ICT_DEASSERT( ict_cntrl.ict_request );
	}
	
	ict_newstatus = 1;
	return( ICT_SUCCESS );
}

ict_status_upd( dpbp )
gdev_dpbp dpbp;
{
	struct ict_ex *exp;

	if( ict_reles) {
		ict_reles = 0;
		ict_rdstatus( );
	}
	exp = &ict_ex[0];
	if( ((ict_status[ 0 ] & exp->mask0 ) == exp->byte0)  &&
	    ((ict_status[ 1 ] & exp->mask1 ) == exp->byte1) ) {
		/* NO Cartridge */
		ict_nocartridge = 1;
		dpbp->dpb_drverror = 10;
	}

	exp++;
	if( ((ict_status[ 0 ] & exp->mask0 ) == exp->byte0)  &&
	   	((ict_status[ 1 ] & exp->mask1 ) == exp->byte1) ) {
		ict_at_eom = 1;
	}
	else {
		ict_at_eom = 0;
	}

	exp++;
	if( ((ict_status[ 0 ] & exp->mask0 ) == exp->byte0)  &&
	    ((ict_status[ 1 ] & exp->mask1 ) == exp->byte1) ) {
		ict_at_filemark = 1;
	}
	else {
		ict_at_filemark = 0;
	}
}

ict_clrstatus (stat_buf)
unsigned char *stat_buf;
{
	int i;
	/***
	** Clear the status buffer.
	***/
	for( i = 0; i < ICT_STATBUF_SZ; ++i ) {
		*stat_buf++ = '\0';
	}
}

ict_clrflags ()
{
	ict_at_filemark = 0;
	ict_at_eom = 0;
	ict_statrdy = 0;
}

struct gdev_parm_block *
ict_chk_excpt(dpbp)
register gdev_dpbp dpbp;
{
	/***                             ****
	** Read the controller status port **
	****                             ***/
	ict_cntr_status = inb( ict_cntrl.ict_status );


	/***                                  ****
	** Is the controller showing EXCEPTION? **
	****                                  ***/
	if( ((ict_cntrl.ict_type == ICT_WANGTEK ||
	      ict_cntrl.ict_type == ICT_MCA_ARCHIVE ) &&
	    ((ict_cntr_status & W_S_EXCEPTION) == 0) ) ||
	    (ict_cntrl.ict_type == ICT_ARCHIVE &&
	    (ict_cntr_status & A_S_EXCEPTION)) ) {

#ifdef ICTDEBUG
		if( ict_print == 1 ) {
			printf("ICT_CHK_EXCPT: Execption!\n");
		}
#endif
		return( ict_excpt_hndlr(dpbp) );
	} 
	
	return( (gdev_dpbp)1 );

}

/*
 * ict_gen_intr - generate an interrupt from the command side 
 *	to complete (unsuccessfully the request).
 */
ict_gen_intr(dpbp)
gdev_dpbp dpbp;
{
#ifdef ICTDEBUG
	if( ict_print ) {
		printf("ict_gen_intr: cmd 0x%x\n", dpbp->dpb_drvcmd);
	}
#endif
	ict_datardy = 0;
#ifdef ICTSTAT
	ict_stat.ict_savecmd = 0;
#endif
	dpbp->dpb_savecmd = 0;
		/* Issue a reset to clear any pending commands */
	ICT_ASSERT( ict_cntrl.ict_reset );
	drv_usecwait( 250 );
	ICT_DEASSERT( ict_cntrl.ict_reset );
	drv_usecwait( ict_cntrl.ict_por_delay );
		/* Initiate a read status to generate an interrupt */
	ict_rdstat_setup();
	ict_statrdy = 1;
}

/*
 * ict_selfmt - select format for read/writing.  Choices include:
 *	QIC-24 format for reading
 *	QIC-120 format for writing(15 track)
 *	QIC-150 format for writing(18 track)
 * (currently not called, so commented out - future ioctl may need it)

ict_selfmt( cmd)
int cmd;
{
#ifdef ICTDEBUG
	if( ict_print ) {
		printf("ict_selfmt: cmd 0x%x\n", cmd);
	}
#endif
	outb( ict_cntrl.ict_command, cmd );

	ICT_ASSERT( ict_cntrl.ict_request );

	if( ict_rdy_wait( ) == ICT_FAILURE ) {
		return( 1 );
	}

        ICT_DEASSERT( ict_cntrl.ict_request );

        if( ict_nrdy_wait( ) == ICT_FAILURE ) {
                return( 1 );
        }
	if( ict_rdy_wait( ) == ICT_FAILURE ) {
		return( 1 );
	}

	return( 0 );
}
end of commented code */


#ifdef ICTDEBUG
/***            ****
** DEBUG ROUTINES **
****            ***/
ict_disp_cntrl()
{
	printf("ict_disp_cntrl:\n");
	printf("               ict_cntrl_mask: 0x%x\n",(int)ict_cntrl_mask);
	printf("               register: 0x%x\n",(int)inb( ict_cntrl.ict_control ));

	return( 0 );
}
#endif
