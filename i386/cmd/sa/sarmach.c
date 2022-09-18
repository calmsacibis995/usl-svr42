/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sa:i386/cmd/sa/sarmach.c	1.1.1.5"
#ident  "$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/fs/rf_acct.h>
#include <limits.h>
#include "sa.h"

extern	int	lines;
extern	int	tblmap[];
extern	float	tdiff;
extern	float   sec_diff, totsec_diff;
extern	struct	sa	nx,ox,ax,bx;

/* the following are to keep averages for sar -x since both sar -x and
 * sar -c keep averages on open, lookup, create, and readdir operations 
 */
static	ulong   a_rfs_in_open, a_rfs_in_lookup, a_rfs_in_create, a_rfs_in_readdir;
static	ulong   a_rfs_out_open, a_rfs_out_lookup, a_rfs_out_create, a_rfs_out_readdir;

static	double	magic = 4.294967296e9;    /* 2^32 (a long is 32 bits) */

/*
 *
 * prtspechdg (ccc)
 *
 *	prints machine special headings
 *
 * Parameter:
 *
 *	ccc	option from main call
 *
 * Return Values: none
 *
 * Exit States: none
 *
 */
void
prtspechdg(ccc)
char	ccc;
{
	switch(ccc) {
	case 'm':
		tsttab();
		printf(" %7s %7s\n",
			"msg/s",
			"sema/s");
		break;
	case 'r':
		tsttab();
		printf(" %7s %7s\n",
			"freemem",
			"freeswp");
		break;
	case 'k':
		tsttab();
		printf(" %7s %7s %5s %7s %7s %5s %11s %5s\n",
			"sml_mem",
			"alloc",
			"fail",
			"lg_mem",
			"alloc",
			"fail",
			"ovsz_alloc",
			"fail");
		break;


	case 'x':
		tsttab();
		printf(" %6s %8s %8s %9s %9s %9s %7s\n",
			"open/s",
			"create/s",
			"lookup/s",
			"readdir/s",
			"getpage/s",
			"putpage/s",
			"other/s");
		break;
			
	case 'S':
		tsttab();
		printf("%13s %9s %9s %9s %9s\n",
			"serv/lo - hi",
			"request",
			"request",
			"server",
			"server");
		tsttab();
		printf("%8d -%3d %8s %10s %9s %10s\n",
			nx.minserve,
			nx.maxserve,
			"%busy",
			"avg lgth",
			"%avail",
			"avg avail");
		break;
	case 'C':
		tsttab();
		printf(" %11s %11s %11s %11s %11s %11s\n",
			"snd-inv/s",
			"snd-msg/s",
			"rcv-inv/s",
			"rcv-msg/s",
			"dis-bread/s",
			"blk-inv/s");
		break;
	case 'p':
		tsttab();
		printf(" %7s %7s %7s %7s %7s %7s\n",
			"atch/s",
			"pgin/s",
			"ppgin/s",
			"pflt/s",
			"vflt/s",
			"slock/s");
		break;

	case 'g':
		tsttab();
		printf(" %8s %8s %8s %8s\n",
			"pgout/s",
			"ppgout/s",
			"pgfree/s",
			"pgscan/s");
		break;
	}
}

/*
 *
 * prtoptdsk ()
 *
 *	prints activities of block devices, e.g., disk or tape drive.
 *
 * Return Values: none
 *
 * Exit States: none
 *
 */
void
prtoptdsk()
{
	register int	ii = 0;
	register int	j, kk, mm;
	ulong		active, response;
	int	hz = HZ;

	for (j = 0; j < SINFO; j++){
	for (kk = 0; kk < tblmap[j]; kk++) {
		if (((nx.devio[ii][IO_OPS] - ox.devio[ii][IO_OPS]) != 0) 
		&& ((nx.devio[ii][IO_ACT] - ox.devio[ii][IO_ACT]) != 0)){
			tsttab();
			printf("  %5s%-2d", "dsk-", ii);

			active = nx.devio[ii][IO_ACT] - ox.devio[ii][IO_ACT];
			response = nx.devio[ii][IO_RESP] - ox.devio[ii][IO_RESP];

			printf("%7.0f %7.1f %7.0f %7.0f %7.1f %7.1f\n",
			((float)active/(tdiff*hz/HZ) *100.0) <= 100.0
				? (float)active/(tdiff*hz/HZ) *100.0
				: 100.0,
			((float)response/(float)active - 1) >= 0.0
				? (float)response/(float)active - 1
				: 0.0,
			(float)(nx.devio[ii][IO_OPS] - ox.devio[ii][IO_OPS])
				/tdiff * HZ,
			(float)(nx.devio[ii][IO_BCNT] - ox.devio[ii][IO_BCNT])
				/tdiff * HZ,
			((float)response - (float)active)
				/(float)(nx.devio[ii][IO_OPS]
					- ox.devio[ii][IO_OPS])
					/hz * 1000.,
			(float)active /
				(float)(nx.devio[ii][IO_OPS]
					- ox.devio[ii][IO_OPS])
					/hz * 1000.);
			for(mm = 0; mm < 4; mm++)
				ax.devio[ii][mm] +=
				    nx.devio[ii][mm] - ox.devio[ii][mm];
		}
		ii++;
	}
	}
}

/*
 *
 * prtspecopt (ccc)
 *
 *	prints machine special options
 *
 * Parameter:
 *
 *	ccc	option from main call
 *
 * Return Values: none
 *
 * Exit States: none
 *
 */
void
prtspecopt(ccc)
char	ccc;
{
	switch(ccc) {
	case 'S':
		tsttab();
		printf("%10.0f %12.1f %9.0f %9.1f %10.0f\n",
			sec_diff <= 0.0 ? 0.0 :
				(float)(nx.rf_srv.rfsi_nservers -
					ox.rf_srv.rfsi_nservers)
					/sec_diff,

			sec_diff <= 0.0 ? 0.0 :
			(float)((nx.rf_srv.rfsi_rcv_occ - ox.rf_srv.rfsi_rcv_occ)
					/sec_diff * 100.0) <= 100.0 ?
				(nx.rf_srv.rfsi_rcv_occ - ox.rf_srv.rfsi_rcv_occ)
					/sec_diff *100.0 :
				100.0,

			((nx.rf_srv.rfsi_rcv_occ - ox.rf_srv.rfsi_rcv_occ <= 0) ? 0.0 :
				(float)(nx.rf_srv.rfsi_rcv_que - ox.rf_srv.rfsi_rcv_que)/
					(float)(nx.rf_srv.rfsi_rcv_occ - ox.rf_srv.rfsi_rcv_occ)),

			sec_diff <= 0.0 ? 0.0 :
			(float)((nx.rf_srv.rfsi_srv_occ - ox.rf_srv.rfsi_srv_occ)
					/sec_diff * 100.0) <= 100.0 ?
				(nx.rf_srv.rfsi_srv_occ - ox.rf_srv.rfsi_srv_occ)
					/sec_diff * 100.0 :
				100.0,

			(nx.rf_srv.rfsi_srv_occ - ox.rf_srv.rfsi_srv_occ == 0) ? 0.0 :
				(float)(nx.rf_srv.rfsi_srv_que - ox.rf_srv.rfsi_srv_que)/
					(float)(nx.rf_srv.rfsi_srv_occ - ox.rf_srv.rfsi_srv_occ));
 


		ax.rf_srv.rfsi_nservers += nx.rf_srv.rfsi_nservers - ox.rf_srv.rfsi_nservers;
		ax.rf_srv.rfsi_rcv_occ += nx.rf_srv.rfsi_rcv_occ - ox.rf_srv.rfsi_rcv_occ;
		ax.rf_srv.rfsi_rcv_que += nx.rf_srv.rfsi_rcv_que - ox.rf_srv.rfsi_rcv_que;
		ax.rf_srv.rfsi_srv_occ += nx.rf_srv.rfsi_srv_occ - ox.rf_srv.rfsi_srv_occ;
		ax.rf_srv.rfsi_srv_que += nx.rf_srv.rfsi_srv_que - ox.rf_srv.rfsi_srv_que;
		break;
	case 'C':
		tsttab();
		printf("%11.1f %11.1f %11.1f %11.1f %11.1f %11.1f\n",
			(float)(nx.rfc.rfci_snd_dis - ox.rfc.rfci_snd_dis)/tdiff * HZ,
			(float)(nx.rfc.rfci_snd_msg - ox.rfc.rfci_snd_msg)/tdiff * HZ,
			(float)(nx.rfc.rfci_rcv_dis - ox.rfc.rfci_rcv_dis)/tdiff * HZ,
			(float)(nx.rfc.rfci_rcv_msg - ox.rfc.rfci_rcv_msg)/tdiff * HZ,
			(float)(nx.rfc.rfci_dis_data - ox.rfc.rfci_dis_data)/tdiff * HZ,
			(float)(nx.rfc.rfci_pabort - ox.rfc.rfci_pabort)/tdiff * HZ);

		ax.rfc.rfci_snd_dis += nx.rfc.rfci_snd_dis - ox.rfc.rfci_snd_dis;
		ax.rfc.rfci_snd_msg += nx.rfc.rfci_snd_msg - ox.rfc.rfci_snd_msg;
		ax.rfc.rfci_rcv_dis += nx.rfc.rfci_rcv_dis - ox.rfc.rfci_rcv_dis;
		ax.rfc.rfci_rcv_msg += nx.rfc.rfci_rcv_msg - ox.rfc.rfci_rcv_msg;
		ax.rfc.rfci_dis_data += nx.rfc.rfci_dis_data - ox.rfc.rfci_dis_data;
		ax.rfc.rfci_pabort += nx.rfc.rfci_pabort - ox.rfc.rfci_pabort;
		break;

	{
	unsigned long k0, k1, x;
	case 'r':
		tsttab();
		k1 = (nx.mi.freemem[1] - ox.mi.freemem[1]);
		if (nx.mi.freemem[0] >= ox.mi.freemem[0]) {
			k0 = nx.mi.freemem[0] - ox.mi.freemem[0]; 
		}
		else
		{ 	k0 = 1 + (~(ox.mi.freemem[0] - nx.mi.freemem[0]));
			k1--; 
		}
		printf(" %7.0f %7.0f\n",
			((double)k0 + magic * (double)k1)/tdiff,
			(float)nx.mi.freeswap);

			x = ax.mi.freemem[0];
			ax.mi.freemem[0] += k0;
			ax.mi.freemem[1] += k1;
			if ( x > ax.mi.freemem[0])
				ax.mi.freemem[1]++;
			ax.mi.freeswap += nx.mi.freeswap;
		break;
	}
	case 'k':
		tsttab();
		printf(" %7.0f %7.0f %5.0f %7.0f %7.0f %5.0f %11.0f %5.0f\n",
			(float)nx.km.km_mem[KMEM_SMALL],
			(float)nx.km.km_alloc[KMEM_SMALL],
			(float)nx.km.km_fail[KMEM_SMALL],
			(float)nx.km.km_mem[KMEM_LARGE],
			(float)nx.km.km_alloc[KMEM_LARGE],
			(float)nx.km.km_fail[KMEM_LARGE],
			(float)nx.km.km_alloc[KMEM_OSIZE],
			(float)nx.km.km_fail[KMEM_OSIZE]);

		ax.km.km_mem[KMEM_SMALL] += nx.km.km_mem[KMEM_SMALL];
		ax.km.km_alloc[KMEM_SMALL] += nx.km.km_alloc[KMEM_SMALL];
		ax.km.km_fail[KMEM_SMALL] += nx.km.km_fail[KMEM_SMALL];
		ax.km.km_mem[KMEM_LARGE] += nx.km.km_mem[KMEM_LARGE];
		ax.km.km_alloc[KMEM_LARGE] += nx.km.km_alloc[KMEM_LARGE];
		ax.km.km_fail[KMEM_LARGE] += nx.km.km_fail[KMEM_LARGE];
		ax.km.km_alloc[KMEM_OSIZE] += nx.km.km_alloc[KMEM_OSIZE];
		ax.km.km_fail[KMEM_OSIZE] += nx.km.km_fail[KMEM_OSIZE];
		break;

	case 'x':
		tsttab();
		printf("\n%-8s %6.2f %8.2f %8.2f %9.2f %9.2f %9.2f %7.2f\n",
			"  in",
			(float)(nx.rfs_in.fsivop_open - ox.rfs_in.fsivop_open)
					/tdiff * HZ,
			(float)(nx.rfs_in.fsivop_create - ox.rfs_in.fsivop_create)
					/tdiff * HZ,
			(float)(nx.rfs_in.fsivop_lookup - ox.rfs_in.fsivop_lookup)
					/tdiff * HZ,
			(float)(nx.rfs_in.fsivop_readdir - ox.rfs_in.fsivop_readdir)
					/tdiff * HZ,
			(float)(nx.rfs_in.fsivop_getpage - ox.rfs_in.fsivop_getpage)
					/tdiff * HZ,
			(float)(nx.rfs_in.fsivop_putpage - ox.rfs_in.fsivop_putpage)
					/tdiff * HZ,
			(float)(nx.rfs_in.fsivop_other - ox.rfs_in.fsivop_other)
					/tdiff * HZ);
		
		printf("%-8s %6.2f %8.2f %8.2f %9.2f %9.2f %9.2f %7.2f\n",
			"  out",
			(float)(nx.rfs_out.fsivop_open - ox.rfs_out.fsivop_open)
					/tdiff * HZ,
			(float)(nx.rfs_out.fsivop_create - ox.rfs_out.fsivop_create)
					/tdiff * HZ,
			(float)(nx.rfs_out.fsivop_lookup - ox.rfs_out.fsivop_lookup)
					/tdiff * HZ,
			(float)(nx.rfs_out.fsivop_readdir - ox.rfs_out.fsivop_readdir)
					/tdiff * HZ,
			(float)(nx.rfs_out.fsivop_getpage - ox.rfs_out.fsivop_getpage)
					/tdiff * HZ,
			(float)(nx.rfs_out.fsivop_putpage - ox.rfs_out.fsivop_putpage)
					/tdiff * HZ,
			(float)(nx.rfs_out.fsivop_other - ox.rfs_out.fsivop_other)
					/tdiff * HZ);

		ax.rfs_in.fsivop_getpage += nx.rfs_in.fsivop_getpage 
					  - ox.rfs_in.fsivop_getpage;
		ax.rfs_out.fsivop_getpage += nx.rfs_out.fsivop_getpage 
					  - ox.rfs_out.fsivop_getpage;
		ax.rfs_in.fsivop_putpage += nx.rfs_in.fsivop_putpage 
					  - ox.rfs_in.fsivop_putpage;
		ax.rfs_out.fsivop_putpage += nx.rfs_out.fsivop_putpage 
					  - ox.rfs_out.fsivop_putpage;
		ax.rfs_in.fsivop_other += nx.rfs_in.fsivop_other 
					  - ox.rfs_in.fsivop_other;
		ax.rfs_out.fsivop_other += nx.rfs_out.fsivop_other 
					  - ox.rfs_out.fsivop_other;
		a_rfs_in_open += nx.rfs_in.fsivop_open 
				- ox.rfs_in.fsivop_open;
		a_rfs_in_lookup += nx.rfs_in.fsivop_lookup 
				- ox.rfs_in.fsivop_lookup;
		a_rfs_in_create += nx.rfs_in.fsivop_create 
				- ox.rfs_in.fsivop_create;
		a_rfs_in_readdir += nx.rfs_in.fsivop_readdir 
				- ox.rfs_in.fsivop_readdir;
	
		a_rfs_out_open += nx.rfs_out.fsivop_open 
				- ox.rfs_out.fsivop_open;
		a_rfs_out_lookup += nx.rfs_out.fsivop_lookup 
				- ox.rfs_out.fsivop_lookup;
		a_rfs_out_create += nx.rfs_out.fsivop_create 
				- ox.rfs_out.fsivop_create;
		a_rfs_out_readdir += nx.rfs_out.fsivop_readdir 
				- ox.rfs_out.fsivop_readdir;
		
	break;

	case 'p':
		tsttab();
		printf(" %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
			(float)((nx.vmi.v_xsfrec - ox.vmi.v_xsfrec) 
				+ (nx.vmi.v_xifrec - ox.vmi.v_xifrec))
				/ tdiff * HZ,

			(float)((nx.vmi.v_pgin - ox.vmi.v_pgin) / tdiff * HZ),

			(float)((nx.vmi.v_pgpgin - ox.vmi.v_pgpgin) 
				/ tdiff * HZ),
	
			(float)((nx.vmi.v_pfault - ox.vmi.v_pfault) 
				/ tdiff * HZ),

			(float)((nx.vmi.v_vfault - ox.vmi.v_vfault) 
				/ tdiff * HZ),

			(float)((nx.vmi.v_sftlock - ox.vmi.v_sftlock) 
				/ tdiff * HZ));
		
			ax.vmi.v_xsfrec += nx.vmi.v_xsfrec - ox.vmi.v_xsfrec;
			ax.vmi.v_xifrec += nx.vmi.v_xifrec - ox.vmi.v_xifrec;
			ax.vmi.v_pgin += nx.vmi.v_pgin - ox.vmi.v_pgin;
			ax.vmi.v_pgpgin += nx.vmi.v_pgpgin - ox.vmi.v_pgpgin;
			ax.vmi.v_pfault += nx.vmi.v_pfault - ox.vmi.v_pfault;
			ax.vmi.v_vfault += nx.vmi.v_vfault - ox.vmi.v_vfault;
			ax.vmi.v_sftlock += nx.vmi.v_sftlock - ox.vmi.v_sftlock;
		break;

	case 'g':
		{
		tsttab();
		printf(" %8.2f %8.2f %8.2f %8.2f\n",
			(float)((nx.vmi.v_pgout - ox.vmi.v_pgout) / tdiff * HZ),

			(float)((nx.vmi.v_pgpgout - ox.vmi.v_pgpgout) 
				/ tdiff * HZ),
			(float)((nx.vmi.v_dfree - ox.vmi.v_dfree) 
				/ tdiff * HZ),
			(float)((nx.vmi.v_scan - ox.vmi.v_scan) 
				/ tdiff * HZ));
	
			ax.vmi.v_pgout += nx.vmi.v_pgout - ox.vmi.v_pgout;
			ax.vmi.v_pgpgout += nx.vmi.v_pgpgout - ox.vmi.v_pgpgout;
			ax.vmi.v_dfree += nx.vmi.v_dfree - ox.vmi.v_dfree;
			ax.vmi.v_scan += nx.vmi.v_scan - ox.vmi.v_scan;
		break;
		}
	}
}

/*
 *
 * prtavgdsk ()
 *
 *	prints average activities of block devices, e.g., disk or tape drive.
 *
 * Return Values: none
 *
 * Exit States: none
 *
 */
void
prtavgdsk()
{
	register int	ii = 0;
	register int	j, kk;
	int	hz = HZ;

	printf("Average ");

	for (j = 0; j < SINFO; j++){
	for (kk = 0; kk < tblmap[j]; kk++) {
		if ((ax.devio[ii][IO_OPS] != 0) 
		&& (ax.devio[ii][IO_ACT] != 0)){
			tsttab();
			printf("  %5s%-2d", "dsk-", ii);

			printf("%7.0f %7.1f %7.0f %7.0f %7.1f %7.1f\n",
			((float)ax.devio[ii][IO_ACT]/(tdiff*hz/HZ)*100.0) <= 100.0
				? (float)ax.devio[ii][IO_ACT]/(tdiff*hz/HZ)*100.0
				: 100.0,
			((float)ax.devio[ii][IO_RESP]
				/(float)ax.devio[ii][IO_ACT] - 1) >= 0.0
				? (float)ax.devio[ii][IO_RESP]
					/(float)ax.devio[ii][IO_ACT] - 1
				: 0.0,
			(float)ax.devio[ii][IO_OPS]/tdiff *HZ,
			(float)ax.devio[ii][IO_BCNT]/tdiff *HZ,
			(float)(ax.devio[ii][IO_RESP]
				- ax.devio[ii][IO_ACT]) /
				(float)ax.devio[ii][IO_OPS] /hz * 1000.,
			(float)ax.devio[ii][IO_ACT]
				/(float)ax.devio[ii][IO_OPS] /
				hz *1000.);
		}
		ii++;
	}
	}
	printf("\n");
}

/*
 *
 * prtspecavg (ccc)
 *
 *	prints machine special average
 *
 * Parameter:
 *
 *	ccc	option from main call
 *
 * Return Values: none
 *
 * Exit States: none
 *
 */
void
prtspecavg(ccc)
char	ccc;
{
	switch(ccc) {
	case 'S':
		printf("Average %10.0f %12.1f %9.0f %9.1f %10.0f\n",
			totsec_diff <= 0.0 ? 0.0 :
				(float)ax.rf_srv.rfsi_nservers / totsec_diff,

			totsec_diff <= 0.0 ? 0.0 :
			(float)(ax.rf_srv.rfsi_rcv_occ / totsec_diff * 100.0)
				<= 100.0 ?
				ax.rf_srv.rfsi_rcv_occ / totsec_diff * 100.0 :
				100.0,

			(ax.rf_srv.rfsi_rcv_occ == 0) ? 0.0 :
				(float)ax.rf_srv.rfsi_rcv_que /
				(float)ax.rf_srv.rfsi_rcv_occ,

			totsec_diff <= 0.0 ? 0.0 :
			(float)(ax.rf_srv.rfsi_srv_occ / totsec_diff * 100.0)
				<= 100.0 ?
				ax.rf_srv.rfsi_srv_occ / totsec_diff * 100.0 :
				100.0,

			(ax.rf_srv.rfsi_srv_occ == 0 ) ? 0.0 :
				(float)ax.rf_srv.rfsi_srv_que / (float)ax.rf_srv.rfsi_srv_occ );
		break;
	case 'C':
		printf("Average %11.1f %11.1f %11.1f %11.1f %11.1f %11.1f\n",
			(float)(ax.rfc.rfci_snd_dis)/tdiff * HZ,
			(float)(ax.rfc.rfci_snd_msg)/tdiff * HZ,
			(float)(ax.rfc.rfci_rcv_dis)/tdiff * HZ,
			(float)(ax.rfc.rfci_rcv_msg)/tdiff * HZ,
			(float)(ax.rfc.rfci_dis_data)/tdiff * HZ,
			(float)(ax.rfc.rfci_pabort)/tdiff * HZ);
		break;
	case 'r':
		printf("Average  %7.0f",
			((double)ax.mi.freemem[0] + magic * (double)ax.mi.freemem[1])/tdiff);
		printf(" %7.0f\n",(float)(ax.mi.freeswap) / lines);
		break;
	case 'k':
		printf("Average  %7.0f %7.0f %5.0f %7.0f %7.0f %5.0f %11.0f %5.0f\n",
			(float)(ax.km.km_mem[KMEM_SMALL] / lines),
			(float)(ax.km.km_alloc[KMEM_SMALL] / lines),
			(float)(ax.km.km_fail[KMEM_SMALL] / lines),
			(float)(ax.km.km_mem[KMEM_LARGE] / lines),
			(float)(ax.km.km_alloc[KMEM_LARGE] / lines),
			(float)(ax.km.km_fail[KMEM_LARGE] / lines),
			(float)(ax.km.km_alloc[KMEM_OSIZE] / lines),
			(float)(ax.km.km_fail[KMEM_OSIZE] / lines));
		break;

	case 'x':
		printf("Average\n%-8s %6.2f %8.2f %8.2f %9.2f %9.2f %9.2f %7.2f\n",
			"  in",
			(float)(a_rfs_in_open / tdiff * HZ),
			(float)(a_rfs_in_create / tdiff * HZ),
			(float)(a_rfs_in_lookup / tdiff * HZ),
			(float)(a_rfs_in_readdir / tdiff * HZ),
			(float)(ax.rfs_in.fsivop_getpage / tdiff * HZ),
			(float)(ax.rfs_in.fsivop_putpage / tdiff * HZ),
			(float)(ax.rfs_in.fsivop_other / tdiff * HZ));

		printf("%-8s %6.2f %8.2f %8.2f %9.2f %9.2f %9.2f %7.2f\n",
			"  out",
			(float)(a_rfs_out_open / tdiff * HZ),
			(float)(a_rfs_out_create / tdiff * HZ),
			(float)(a_rfs_out_lookup / tdiff * HZ),
			(float)(a_rfs_out_readdir / tdiff * HZ),
			(float)(ax.rfs_out.fsivop_getpage / tdiff * HZ),
			(float)(ax.rfs_out.fsivop_putpage / tdiff * HZ),
			(float)(ax.rfs_out.fsivop_other / tdiff * HZ));
		break;
			
	case 'p':
		printf("Average  %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
			(float)((ax.vmi.v_xsfrec + ax.vmi.v_xifrec)
				/ tdiff * HZ),
			(float)(ax.vmi.v_pgin / tdiff * HZ), 
			(float)(ax.vmi.v_pgpgin / tdiff * HZ), 
			(float)(ax.vmi.v_pfault / tdiff * HZ), 
			(float)(ax.vmi.v_vfault / tdiff * HZ), 
			(float)(ax.vmi.v_sftlock / tdiff * HZ));
		break;

	case 'g':
		printf("Average  %8.2f %8.2f %8.2f %8.2f\n",
			(float)(ax.vmi.v_pgout / tdiff * HZ), 
			(float)(ax.vmi.v_pgpgout / tdiff * HZ), 
			(float)(ax.vmi.v_dfree / tdiff * HZ), 
			(float)(ax.vmi.v_scan / tdiff * HZ));
		break;
	}
}
