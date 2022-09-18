/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/target/sdi.c	1.25"
#ident	"$Header: miked 1/23/92 $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

/*
 * sdi.c
 *  SDI interface module.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/user.h>
#include <util/cmn_err.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <proc/proc.h>
#include <fs/buf.h>
#include <proc/cred.h>
#include <io/conf.h>
#include <io/target/scsi.h>
#include <io/target/sdi_edt.h>
#include <io/target/dynstructs.h>
#include <io/target/sdi.h>
#include <io/ddi.h>

#define SDI_CONTROL(x)  ((x >> 3) & 0x07)       /* C# from SCB  */

#define TRUE 1
#define FALSE 0
#define ONE_SEC 1000

/*
#define SDI_DEBUG	1
*/

#if	(_SYSTEMENV == 4)
int	sdi_devflag	= D_NEW | D_DMA;/* 4.0 style driver flag */
#endif

static int sdi_debug = 	0;

static long sdi_inited = 0;
long init_time = 0;
static struct sdi_edt *freeedt;		/* free pointer into edtpool */
static struct owner *freeowner;		/* free pointer into owner_pool */
int sdi_hacnt = 0;			/* number of HBAs */
struct ident	inq_data;	    	/* Inquiry data 		*/
struct head	sm_poolhead;		/* head of small structs for hba drvs */
struct jpool	smfreelist;
struct head	lg_poolhead;		/* head for job structs for targ drvs */
struct jpool	lgfreelist;

/*
 *	Error Message Defines for SCSI and SDI
 *
 *	NOTE:  These lists are currently sorted in ascending order
 *	       on the sense or completion codes in the arrays.
 *	       Maintain this ordering or sdi_err will fail.
 */

struct CompCodeMsg {
	ulong_t		CompCode; 		/* SDI Completion Code */
	char		*Msg;			/* Message String */
};

static struct CompCodeMsg CompCodeTable[] = {
	{SDI_NOALLOC,	"This block is not allocated"},
	{SDI_ASW,   	"Job completed normally"},
	{SDI_LINKF0,	"Linked command done without flag"},
	{SDI_LINKF1,	"Linked command done with flag"},
	{SDI_QFLUSH,	"Job was flushed"},
	{SDI_ABORT,   	"Command was aborted"},
	{SDI_RESET,   	"Reset was detected on the bus"},
	{SDI_CRESET,	"Reset was caused by this unit"},
	{SDI_V2PERR,	"Vtop failed"},
	{SDI_TIME,   	"Job timed out"},
	{SDI_NOTEQ,   	"Addressed device not present"},
	{SDI_HAERR,   	"Host adapter error"},
	{SDI_MEMERR,	"Memory fault"},
	{SDI_SBUSER,	"SCSI bus error"},
	{SDI_CKSTAT,	"Check the status byte"},
	{SDI_SCBERR,	"SCB error"},
	{SDI_OOS,   	"Device is out of service"},
	{SDI_NOSELE,	"The SCSI bus select failed"},
	{SDI_MISMAT,	"Parameter mismatch"},
	{SDI_PROGRES,	"Job in progress"},
	{SDI_UNUSED,	"Job not in use"},
	{SDI_ONEIC,   	"More than one immediate request"},
	{SDI_SFBERR,	"SFB error"},
	{SDI_TCERR,   	"Target protocol error detected"},
};

#define	COMP_CODE_MSG_COUNT	(sizeof(CompCodeTable)/sizeof(struct CompCodeMsg))


struct SenseKeyMsg {
	uchar_t		Key;			/* Sense Key */
	char		*Msg;			/* Message String */
};

static struct SenseKeyMsg	SenseKeyTable[] = {
	{SD_NOSENSE, "NO SENSE INFORMATION AVAILABLE"},
	{SD_RECOVER, "RECOVERED ERROR"},
	{SD_NREADY, "NOT READY"},
	{SD_MEDIUM, "MEDIUM ERROR"},
	{SD_HARDERR, "HARDWARE ERROR"},
	{SD_ILLREQ, "ILLEGAL REQUEST"},
	{SD_UNATTEN, "UNIT ATTENTION"},
	{SD_PROTECT, "WRITE PROTECTED"},
	{SD_BLANKCK, "BLANK CHECK"},
	{SD_VENUNI, "VENDOR SPECIFIC"},
	{SD_COPYAB, "COPY/COMPARE ABORTED"},
	{SD_ABORT, "ABORTED COMMAND"},
	{SD_EQUAL, "EQUAL"},
	{SD_VOLOVR, "VOLUME OVERFLOW"},
	{SD_MISCOMP, "MISCOMPARE"},
	{SD_RESERV, "RESERVED"},
};

#define	SENSE_KEY_MSG_COUNT	(sizeof(SenseKeyTable)/sizeof(struct SenseKeyMsg))

struct ExtSenseKeyMsg {
	uchar_t		Key;		/* Extended Sense Key */
	uchar_t		Qual;		/* Extended Sense Key Qualifier */
	char		*Msg;		/* Message String */
};

static struct ExtSenseKeyMsg	ExtSenseKeyTable[] = {
	{SC_NOSENSE, SC_NOSENSE, "NO ADDITIONAL SENSE INFORMATION AVAILABLE"},
	{SC_NOSENSE, 0x01, "FILEMARK DETECTED"},
	{SC_NOSENSE, 0x02, "END-OF-PARTITION/MEDIUM DETECTED"},
	{SC_NOSENSE, 0x03, "SETMARK DETECTED"},
	{SC_NOSENSE, 0x04, "BEGINNING-OF-PARTITION/MEDIUM DETECTED"},
	{SC_NOSENSE, 0x05, "END-OF-DATA DETECTED"},
	{SC_NOSENSE, 0x06, "I/O PROCESS TERMINATED"},
	{SC_NOSENSE, 0x11, "AUDIO PLAY OPERATION IN PROGRESS"},
	{SC_NOSENSE, 0x12, "AUDIO PLAY OPERATION PAUSED"},
	{SC_NOSENSE, 0x13, "AUDIO PLAY OPERATION SUCCESSFULLY COMPLETED"},
	{SC_NOSENSE, 0x14, "AUDIO PLAY OPERATION STOPPED DUE TO ERROR"},
	{SC_NOSENSE, 0x15, "NO CURRENT AUDIO STATUS TO RETURN"},
	{SC_NOSGNL, 0x00, "NO INDEX/SECTOR SIGNAL"},
	{SC_NOSEEK, 0x00, "NO SEEK COMPLETE"},
	{SC_WRFLT, 0x00, "PERIPHERAL DEVICE WRITE FAULT"},
	{SC_WRFLT, 0x01, "NO WRITE CURRENT"},
	{SC_WRFLT, 0x02, "EXCESSIVE WRITE ERRORS"},
	{SC_DRVNTRDY, 0x00, "LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE"},
	{SC_DRVNTRDY, 0x01, "LOGICAL UNIT IS IN PROCESS OF BECOMING READY"},
	{SC_DRVNTRDY, 0x02, "LOGICAL UNIT NOT READY, INITIALIZING COMMAND REQUIRED"},
	{SC_DRVNTRDY, 0x03, "LOGICAL UNIT NOT READY, MANUAL INTERVENTION REQUIRED"},
	{SC_DRVNTRDY, 0x04, "LOGICAL UNIT NOT READY, FORMAT IN PROGRESS"},
	{SC_DRVNTSEL, 0x00, "LOGICAL UNIT DOES NOT RESPOND TO SELECTION"},
	{SC_NOTRKZERO, 0x00, "NO REFERENCE POSITION FOUND"},
	{SC_MULTDRV, 0x00, "MULTIPLE PERIPHERAL DEVICES SELECTED"},
	{SC_LUCOMM, 0x00, "LOGICAL UNIT COMMUNICATION FAILURE"},
	{SC_LUCOMM, 0x01, "LOGICAL UNIT COMMUNICATION TIME-OUT"},
	{SC_LUCOMM, 0x02, "LOGICAL UNIT COMMUNICATION PARITY ERROR"},
	{SC_TRACKERR, 0x00, "TRACK FOLLOWING ERROR"},
	{SC_TRACKERR, 0x01, "TRACKING SERVO FAILURE"},
	{SC_TRACKERR, 0x02, "FOCUS SERVO FAILURE"},
	{SC_TRACKERR, 0x03, "SPINDLE SERVO FAILURE"},
	{0x0A, 0x00, "ERROR LOG OVERFLOW"},
	{0x0C, 0x00, "WRITE ERROR"},
	{0x0C, 0x01, "WRITE ERROR RECOVERED WITH AUTO REALLOCATION"},
	{0x0C, 0x02, "WRITE ERROR - AUTO REALLOCATION FAILED"},
	{SC_IDERR, 0x00, "ID CRC OR ECC ERROR"},
	{SC_UNRECOVRRD, 0x00, "UNRECOVERED READ ERROR"},
	{SC_UNRECOVRRD, 0x01, "READ RETRIES EXHAUSTED"},
	{SC_UNRECOVRRD, 0x02, "ERROR TOO LONG TO CORRECT"},
	{SC_UNRECOVRRD, 0x03, "MULTIPLE READ ERRORS"},
	{SC_UNRECOVRRD, 0x04, "UNRECOVERED READ ERROR - AUTO REALLOCATE FAILED"},
	{SC_UNRECOVRRD, 0x05, "L-EC UNCORRECTABLE ERROR"},
	{SC_UNRECOVRRD, 0x06, "CIRC UNRECOVERED ERROR"},
	{SC_UNRECOVRRD, 0x07, "DATA RESYNCHRONIZATION ERROR"},
	{SC_UNRECOVRRD, 0x08, "IMCOMPLETE BLOCK READ"},
	{SC_UNRECOVRRD, 0x09, "NO GAP FOUND"},
	{SC_UNRECOVRRD, 0x0A, "MISCORRECTED ERROR"},
	{SC_UNRECOVRRD, 0x0B, "UNRECOVERED READ ERROR - RECOMMEND REASSIGNMENT"},
	{SC_UNRECOVRRD, 0x0C, "UNRECOVERED READ ERROR - RECOMMEND REWRITE THE DATA"},
	{SC_NOADDRID, 0x00, "ADDRESS MARK NOT FOUND FOR ID FIELD"},
	{SC_NOADDRDATA, 0x00, "ADDRESS MARK NOT FOUND FOR DATA"},
	{SC_NORECORD, 0x00, "RECORDED ENTITY NOT FOUND"},
	{SC_NORECORD, 0x01, "RECORD NOT FOUND"},
	{SC_NORECORD, 0x02, "FILEMARK OR SETMARK NOT FOUND"},
	{SC_NORECORD, 0x03, "END-OF-DATA NOT FOUND"},
	{SC_NORECORD, 0x04, "BLOCK SEQUENCE ERROR"},
	{SC_SEEKERR, 0x00, "RANDOM POSITIONING ERROR"},
	{SC_SEEKERR, 0x01, "MECHANICAL POSITIONING ERROR"},
	{SC_SEEKERR, 0x02, "POSITIONING ERROR DETECTED BY READ OF MEDIUM"},
	{SC_DATASYNCMK, 0x00, "DATA SYNCHRONIZATION MARK ERROR"}, 
	{SC_RECOVRRD, 0x00, "RECOVERED DATA WITH NO ERROR CORRECTION APPLIED"},
	{SC_RECOVRRD, 0x01, "RECOVERED DATA WITH RETRIES"},
	{SC_RECOVRRD, 0x02, "RECOVERED DATA WITH POSITIVE HEAD OFFSET"},
	{SC_RECOVRRD, 0x03, "RECOVERED DATA WITH NEGATIVE HEAD OFFSET"},
	{SC_RECOVRRD, 0x04, "RECOVERED DATA WITH RETRIES AND/OR CIRC APPLIED"},
	{SC_RECOVRRD, 0x05, "RECOVERED DATA USING PREVIOUS SECTOR ID"},
	{SC_RECOVRRD, 0x06, "RECOVERED DATA WITHOUT ECC - DATA AUTO-REALLOCATED"},
	{SC_RECOVRRD, 0x07, "RECOVERED DATA WITHOUT ECC - RECOMMENDED REASSIGNMENT"},
	{SC_RECOVRRDECC, 0x00, "RECOVERED DATA WITH ERROR CORRECTION APPLIED"},
	{SC_RECOVRRDECC, 0x01, "RECOVERED DATA WITH ERROR CORRECTION AND RETRIES APPLIED"},
	{SC_RECOVRRDECC, 0x02, "RECOVERED DATA - DATA AUTO-REALLOCATED"},
	{SC_RECOVRRDECC, 0x03, "RECOVERED DATA WITH CIRC"},
	{SC_RECOVRRDECC, 0x04, "RECOVERED DATA WITH LEC"},
	{SC_RECOVRRDECC, 0x05, "RECOVERED DATA - RECOMMEND REASSIGNMENT"},
	{SC_DFCTLSTERR, 0x00, "DEFECT LIST ERROR"},
	{SC_DFCTLSTERR, 0x01, "DEFECT LIST NOT AVAILABLE"},
	{SC_DFCTLSTERR, 0x02, "DEFECT LIST ERROR IN PRIMARY LIST"},
	{SC_DFCTLSTERR, 0x03, "DEFECT LIST ERROR IN GROWN LIST"},
	{SC_PARAMOVER, 0x00, "PARAMETER LIST LENGTH ERROR"},
	{SC_SYNCTRAN, 0x00, "SYNCHRONOUS DATA TRANSFER ERROR"},
	{SC_NODFCTLST, 0x00, "DEFECT LIST NOT FOUND"},
	{SC_NODFCTLST, 0x01, "PRIMARY DEFECT LIST NOT FOUND"},
	{SC_NODFCTLST, 0x02, "GROWN DEFECT LIST NOT FOUND"},
	{SC_CMPERR, 0x00, "MISCOMPARE DURING VERIFY OPERATION"},
	{SC_RECOVRIDECC, 0x00, "RECOVERED ID WITH ECC CORRECTION"},
	{SC_INVOPCODE, 0x00, "INVALID COMMAND OPERATION CODE"},
	{SC_ILLBLCK, 0x00, "LOGICAL BLOCK ADDRESS OUT OF RANGE"},
	{SC_ILLBLCK, 0x01, "INVALID ELEMENT ADDRESS"},
	{SC_ILLFUNC, 0x00, "ILLEGAL FUNCTION FOR DEVICE TYPE"},
	{SC_ILLCDB, 0x00, "INVALID FIELD IN CDB"},
	{SC_INVLUN, 0x00, "LOGICAL UNIT NOT SUPPORTED"},
	{SC_INVPARAM, 0x00, "INVALID FIELD IN PARAMETER LIST"},
	{SC_INVPARAM, 0x01, "PARAMETER NOT SUPPORTED"},
	{SC_INVPARAM, 0x02, "PARAMETER VALUE INVALID"},
	{SC_INVPARAM, 0x03, "THRESHOLD PARAMETERS NOT SUPPORTED"},
	{SC_WRPROT, 0x00, "WRITE PROTECTED"},
	{SC_MEDCHNG, 0x00, "NOT READY TO READY TRANSITION (MEDIUM MAY HAVE CHANGED)"},
	{SC_MEDCHNG, 0x01, "IMPORT OR EXPORT ELEMENT ACCESSED"},
	{SC_RESET, 0x00, "POWER ON, RESET, OR BUS DEVICE RESET OCCURRED"},
	{SC_MDSELCHNG, 0x00, "PARAMETERS CHANGED"},
	{SC_MDSELCHNG, 0x01, "MODE PARAMETERS CHANGED"},
	{SC_MDSELCHNG, 0x02, "LOG PARAMETERS CHANGED"},
	{0x2B, 0x00, "COPY CANNOT EXECUTE SINCE HOST CANNOT DISCONNECT"},
	{0x2C, 0x00, "COMMAND SEQUENCE ERROR"},
	{0x2C, 0x01, "TOO MANY WINDOWS SPECIFIED"},
	{0x2C, 0x02, "INVALID COMBINATION OF WINDOWS SPECIFIED"},
	{0x2D, 0x00, "OVERWRITE ERROR ON UPDATE IN PLACE"},
	{0x2F, 0x00, "COMMANDS CLEARED BY ANOTHER INITIATOR"},
	{SC_INCOMP, 0x00, "INCOMPATIBLE MEDIUM INSTALLED"},
	{SC_INCOMP, 0x01, "CANNOT READ MEDIUM - UNKNOWN FORMAT"},
	{SC_INCOMP, 0x02, "CANNOT READ MEDIUM - INCOMPATIBLE FORMAT"},
	{SC_INCOMP, 0x03, "CLEANING CARTRIDGE INSTALLED"},
	{SC_FMTFAIL, 0x00, "MEDIUM FORMAT CORRUPTED"},
	{SC_FMTFAIL, 0x01, "FORMAT COMMAND FAILED"},
	{SC_NODFCT, 0x00, "NO DEFECT SPARE LOCATION AVAILABLE"},
	{SC_NODFCT, 0x01, "DEFECT LIST UPDATE FAILURE"},
	{0x33, 0x00, "TAPE LENGTH ERROR"},
	{0x36, 0x00, "RIBBON, INK, OR TONER FAILURE"},
	{0x37, 0x00, "ROUNDED PARAMETER"},
	{0x38, 0x00, "SEQUENTIAL POSITIONING ERROR"},
	{0x38, 0x0D, "MEDIUM DESTINATION ELEMENT FULL"},
	{0x39, 0x00, "SAVING PARAMETERS NOT SUPPORTED"},
	{0x3A, 0x00, "MEDIUM NOT PRESENT"},
	{0x3B, 0x01, "TAPE POSITION ERROR AT BEGINNING-OF-MEDIUM"},
	{0x3B, 0x02, "TAPE POSITION ERROR AT END-OF-MEDIUM"},
	{0x3B, 0x03, "TAPE OR ELECTRONIC VERTICAL FORMS UNIT NOT READY"},
	{0x3B, 0x04, "SLEW FAILURE"},
	{0x3B, 0x05, "PAPER JAM"},
	{0x3B, 0x06, "FAILED TO SENSE TOP-OF-FORM"},
	{0x3B, 0x07, "FAILED TO SENSE BOTTOM-OF-FORM"},
	{0x3B, 0x08, "REPOSITION ERROR"},
	{0x3B, 0x09, "READ PAST END OF MEDIUM"},
	{0x3B, 0x0A, "READ PAST BEGINNING OF MEDIUM"},
	{0x3B, 0x0B, "POSITION PAST END OF MEDIUM"},
	{0x3B, 0x0C, "POSITION PAST BEGINNING OF MEDIUM"},
	{0x3B, 0x0D, "MEDIUM DESTINATION ELEMENT FULL"},
	{0x3B, 0x0E, "MEDIUM SOURCE ELEMENT EMPTY"},
	{0x3D, 0x00, "INVALID BITS IN IDENTIFY MESSAGE"},
	{0x3E, 0x00, "LOGICAL UNIT HAS NOT SELF-CONFIGURED YET"},
	{0x3F, 0x00, "TARGET OPERATING CONDITIONS HAVE CHANGED"},
	{0x3F, 0x01, "MICROCODE HAS BEEN CHANGED"},
	{0x3F, 0x02, "CHANGED OPERATING DEFINITION"},
	{0x3F, 0x03, "INQUIRY DATA HAS CHANGED"},
	{SC_RAMFAIL, 0x00, "VENDOR UNIQUE"},
	{SC_DATADIAG, 0x00, "DATA PATH FAILURE"},
	{SC_POWFAIL, 0x00, "POWER-ON OR SELF-TEST FAILURE"},
	{SC_MSGREJCT, 0x00, "MESSAGE ERROR"},
	{SC_CONTRERR, 0x00, "INTERNAL TARGET FAILURE"},
	{SC_SELFAIL, 0x00, "SELECT OR RESELECT FAILURE"},
	{SC_SOFTRESET, 0x00, "UNSUCCESSFUL SOFT RESET"},
	{SC_PARITY, 0x00, "SCSI PARITY ERROR"},
	{SC_INITERR, 0x00, "INITIATOR DETECTED ERROR MESSAGE RECEIVED"},
	{SC_ILLMSG, 0x00, "INVALID MESSAGE ERROR"},
	{0x4A, 0x00, "COMMAND PHASE ERROR"},
	{0x4B, 0x00, "DATA PHASE ERROR"},
	{0x4C, 0x00, "LOGICAL UNIT FAILED SELF-CONFIGURATION"},
	{0x4E, 0x00, "OVERLAPPED COMMANDS ATTEMPTED"},
	{0x50, 0x00, "WRITE APPEND ERROR"},
	{0x50, 0x01, "WRITE APPEND POSITION ERROR"},
	{0x50, 0x02, "POSITION ERROR RELATED TO TIMING"},
	{0x51, 0x00, "ERASE FAILURE"},
	{0x52, 0x00, "CARTRIDGE FAULT"},
	{0x53, 0x00, "MEDIA LOAD OR EJECT FAILED"},
	{0x53, 0x01, "UNLOAD TAPE FAILURE"},
	{0x53, 0x02, "MEDIUM REMOVAL PREVENTED"},
	{0x54, 0x00, "SCSI TO HOST SYSTEM INTERFACE FAILURE"},
	{0x55, 0x00, "SYSTEM RESOURCE FAILURE"},
	{0x57, 0x00, "UNABLE TO RECOVER TABLE-OF-CONTENTS"},
	{0x58, 0x00, "GENERATION DOES NOT EXIST"},
	{0x59, 0x00, "UPDATED BLOCK READ"},
	{0x5A, 0x00, "OPERATOR REQUEST OR STATE CHANGE INPUT (UNSPECIFIED)"},
	{0x5A, 0x01, "OPERATOR MEDIUM REMOVAL REQUEST"},
	{0x5A, 0x02, "OPERATOR SELECTED WRITE PROTECT"},
	{0x5A, 0x03, "OPERATOR SELECTED WRITE PERMIT"},
	{0x5B, 0x00, "LOG EXEPTION"},
	{0x5B, 0x01, "THRESHOLD CONDITION MET"},
	{0x5B, 0x02, "LOG COUNTER AT MAXIMUM"},
	{0x5B, 0x03, "LOG LIST CODES EXHAUSTED"},
	{0x5C, 0x00, "RPL STATUS CHANGE"},
	{0x5C, 0x01, "SPINDLES SYNCHRONIZED"},
	{0x5C, 0x02, "SPINDLES NOT SYNCHRONIZED"},
	{0x60, 0x00, "LAMP FAILURE"},
	{0x61, 0x00, "VIDEO ACQUISITION ERROR"},
	{0x61, 0x01, "UNABLE TO ACQUIRE VIDEO"},
	{0x61, 0x02, "OUT OF FOCUS"},
	{0x62, 0x00, "SCAN HEAD POSITIONING ERROR"},
	{0x63, 0x00, "END OF USER AREA ENCOUNTERED ON THIS TRACK"},
	{0x64, 0x00, "ILLEGAL MODE FOR THIS TRACK"}
	/*
	 * Vender Unique Sense Key's (0x80->0xFF) and Vender Unique
	 * Qualifiers (0x80->0xFF) need to be checked explicitely.
	 */ 
};

#define EXT_SENSE_KEY_MSG_COUNT	(sizeof(ExtSenseKeyTable)/sizeof(struct ExtSenseKeyMsg))

struct StatusMsg {
	uchar_t		Status; 	/* SCSI Status Byte from Target */
	char		*Msg;		/* Message String */
};

static struct StatusMsg StatusTable[] = {
	{S_GOOD, "GOOD"},
	{S_CKCON, "CHECK CONDITION"},
	{S_METGD, "CONDITION MET"},
	{S_BUSY, "BUSY"},
	{S_INGD, "INTERMEDIATE"},
	{S_INMET, "INTERMEDIATE-CONDITION MET"},
	{S_RESER, "RESERVATION CONFLICT"},
	{0x22, "COMMAND TERMINATED"},
	{0x28, "QUEUE FULL"},
};

#define	STATUS_MSG_COUNT	(sizeof(StatusTable)/sizeof(struct StatusMsg))

extern struct sdi_edt edtpool[];	/* pool of edt entries */
extern struct sdi_edt edt_hash[];	/* edt hash table */
extern struct hba_cfg HBA_tbl[];
extern void (*sdi_rinits[])();
extern int sdi_rtabsz;		/* Number of slots in rinit table */
extern int sdi_hbaswsz;		/* Number of slots in HBA table */
extern int sdi_tape_luns;	/* Number of tape luns to inquiry */

/***
** This sense structure is used by sdi_cklu to when issueing
** a REQUEST SENSE command if the SS_TEST command failed.
***/
struct sense sdi_sense;

struct owner *
alloc_ownerblk()
{
	struct owner *free;

	if( freeowner ) {
		free = freeowner;
		freeowner = freeowner->next;
	}
	else {
		cmn_err(CE_WARN,"SDI: Cannot allocate Owner Block");
		return( (struct owner *)NULL );
	}

	return( free );
}

free_ownerblk( ownerblk )
struct owner *ownerblk;
{
	ownerblk->edtp = (struct sdi_edt *)NULL;
	ownerblk->maj.b_maj = -1;
	ownerblk->maj.c_maj = -1;
	ownerblk->fault = 0;
	ownerblk->flt_parm = 0;
	ownerblk->res1 = 0;
	ownerblk->res2 = 0;
	ownerblk->name = (char *)NULL;

	ownerblk->next = freeowner;
	freeowner = ownerblk;

	return( 1 );
}

static struct sdi_edt *
get_edt()
{
	struct sdi_edt *free;
#ifdef SDI_DEBUG
	if(sdi_debug > 1)
		printf("get_edt()\n");
#endif

	if (free = freeedt) 
		freeedt = freeedt->hash_p;
#ifdef DEBUG
	else
		cmn_err(CE_PANIC, "Cannot alloc edt\n");
#endif
	return free;
}

int	sdi_act_hbacnt = 0;

sdi_init()
{
	register int i;
	int	sdi_act_hba; 
#ifdef SDI_DEBUG
	if(sdi_debug >= 1){
		printf("sdi_init: sdi_inited = %x\n", sdi_inited);
		printf("edtpool = %x edt_hash = %x owner_pool %x\n",
			edtpool, edt_hash, owner_pool);
		ml_pause();
	}
#endif

	if (!sdi_inited) {
		init_time = 1;
			/*
			 * Init the edt pool.  freeedt points at the next
			 * edt to use, and the hash pointer then continues
			 * the chain.
			 */
		for (i=0; i < NSDIDEV-1; i++) {
			edtpool[i].hash_p = &edtpool[i+1];
			edtpool[i].hba_no = -1;
		}
		edtpool[NSDIDEV-1].hba_no = -1;
		edtpool[NSDIDEV-1].hash_p = (struct sdi_edt *)NULL;
		freeedt = edtpool;

			/*
			 * Init the edt hash table.  These are also free
			 * edt structures, which are used before diving
			 * into the free pool, and are the head of each
			 * hash queue.
			 */
		for (i=0; i <  EDT_HASH_LEN; i++) {
			edt_hash[i].hash_p = (struct sdi_edt *)NULL;
			edt_hash[i].hba_no = -1;
		}

			/*
 			 * Init the owner pool
			 */
		for (i=0; i < NOWNER-1; i++) {
			owner_pool[i].next = & owner_pool[i+1];
		}
		owner_pool[i].next = (struct owner *)NULL;
		freeowner = owner_pool;

		/* initialize rinits array to NULLs */
		for (i=0; i < sdi_rtabsz; i++) {
			sdi_rinits[i] = NULL;
		}
		/* initialize small struct pool (HBA scsi blocks and drq's) */
        	sm_poolhead.f_maxpages = 1;
		smfreelist.j_ff = smfreelist.j_fb = &smfreelist;
		sm_poolhead.f_freelist = &smfreelist;
		sm_poolhead.f_isize = 28;
		sdi_poolinit(&sm_poolhead);

		/* Now initialize large struct pool (job structs and xsbs) */
        	lg_poolhead.f_maxpages = 1;
		lgfreelist.j_ff = lgfreelist.j_fb = &lgfreelist;
		lg_poolhead.f_freelist = &lgfreelist;
		lg_poolhead.f_isize = sizeof(struct xsb);
		sdi_poolinit(&lg_poolhead);

		sdi_inited++;	/* only do this once */

		init_time = 0;
	}

}

int
sdi_cklu(hba_no, target, lun, inquire, pd_type)
int hba_no, target, lun;
char *inquire;
unsigned char *pd_type;
{
	struct scs	tur_cdb;	/* Test Unit Ready cdb */
	char *p;
/*
 * If the id_type of the device is ID_NODEV, it is not a valid device.
 */
	if (inq_data.id_type == ID_NODEV)
		return FALSE;

/*
 * If the target we are currently looking at is the
 * host adapter, it is good by definition.
 */
	if (target == IDP(HBA_tbl[hba_no].idata)->ha_id)
		return TRUE;

/*
 * If the device we just inquired is type ID_RANDOM and it does not have
 * removable media, do an SS_TEST on it to see if it really exists.
 *
 * Some devices answer the SS_INQUIRE at a bunch of the lun's for the
 * same target.  This test allows us to eliminate some of these.
 *
 * We check for removable media since if the device is capable of
 * removable media, it won't test ready unless the media is inserted
 * and we have no way to guarentee this.
 */
	if ((inq_data.id_type == ID_RANDOM) && (!(inq_data.id_rmb))) {
		tur_cdb.ss_op = SS_TEST; /* test unit ready cdb */
		tur_cdb.ss_lun = lun;
		tur_cdb.ss_addr = NULL;
		tur_cdb.ss_addr1 = NULL;
		tur_cdb.ss_len = NULL;
		tur_cdb.ss_cont = NULL;
	
		if( sdi_docmd(hba_no, target, lun, &tur_cdb, SCS_SZ, NULL, NULL, B_READ) != SDI_ASW ) {
			/***
			** Since the SS_TEST command failed, issue a REQUEST SENSE
			** command to get the device status and cleanup the SCSI
			** error status.
			***/
			tur_cdb.ss_op = SS_REQSEN;
			tur_cdb.ss_lun = lun;
			tur_cdb.ss_addr = NULL;
			tur_cdb.ss_addr1 = NULL;
			tur_cdb.ss_len = SENSE_SZ;
			tur_cdb.ss_cont = NULL;
			sdi_docmd(hba_no, target, lun, &tur_cdb, SCS_SZ, &sdi_sense, NULL, B_READ);
		}

		tur_cdb.ss_op = SS_TEST; /* test unit ready cdb */
		tur_cdb.ss_lun = lun;
		tur_cdb.ss_addr = NULL;
		tur_cdb.ss_addr1 = NULL;
		tur_cdb.ss_len = NULL;
		tur_cdb.ss_cont = NULL;

		if (sdi_docmd(hba_no, target, lun, &tur_cdb, SCS_SZ, NULL, NULL, B_READ) == SDI_ASW) {
			return TRUE;
		} else {
			return FALSE;
		}
	}

/*
 *	We will now check the peripheral qualifier, id_pqual,
 *	returned by the device.  these bits must be zero if the device
 *	supports this lu.
 */

	if (inq_data.id_pqual != 0) {
		return FALSE;
	}

	if (inq_data.id_type == ID_TAPE) {
		/*
		 * Some ill behaved tapes return a valid ident string
		 * on luns for which no tapes exist, so if the ident
		 * string for luns>0 match the lun 0 exactly, then
 		 * we treat this lun as un-equiped.  If the
 		 * inquires do not match, we assume the device
		 * really exists.
		 */
		if(lun == 0) {
               		(void)strncpy(inquire, inq_data.id_vendor, INQ_LEN);
               		inquire[INQ_LEN-1] = NULL;
               		*pd_type = inq_data.id_type;
		} else {
       			if ((inq_data.id_type == *pd_type) &&
               		(!strncmp(inq_data.id_vendor, inquire, (INQ_LEN-1)))) {
               			return FALSE;
			}
		}
	}
	return TRUE;
}

void
sdi_start()
{
#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		printf("sdi_start: sdi_inited = %x\n", sdi_started);
#endif
}

/*
 * findle_edt searches the EDT for an entry that is the one specified, or
 * failing that, the place to insert a new entry after in the hash queue.
 * There are three possible return values:  a pointer to the existing,
 * entry, a pointer to an empty entry (e.g., hba_no == -1; this happens
 * when a hash queue is empty), and a pointer to an entry that is not the
 * one requested.
 */
struct sdi_edt *
findle_edt(hba, scsi_id, lun)
short hba;
unsigned char  scsi_id, lun;
{
	struct sdi_edt *edtp = &EDT_HASH(hba, scsi_id, lun);
	struct sdi_edt *edtprev = (struct sdi_edt *)NULL;

#ifdef SDI_DEBUG
	if(sdi_debug > 1){
		printf("findle_edt(%x, %x, %x)\n", hba, scsi_id, lun);
		printf("edtp = %x\n", edtp);
	}
#endif

	for ( ; edtp && edtp->hba_no != -1;
		edtprev = edtp, edtp = edtp->hash_p) {

		if (edtp && edtp->hba_no == hba &&
			edtp->scsi_id == scsi_id &&
				edtp->lun == lun) {
			return edtp;
		}
	}


	return edtprev ? edtprev : edtp;
}

struct sdi_edt *
sdi_redt(hba, scsi_id, lun)
int hba, scsi_id, lun;
{
	struct sdi_edt *edtp = findle_edt(hba, scsi_id, lun);

	if (hba == edtp->hba_no && scsi_id == edtp->scsi_id &&
			lun == edtp->lun) {
		return edtp;
	}
	return (struct sdi_edt *)NULL;
}


/***
** SDI_DISCLAIM:
**		IF currently owner of drive
**		THEN
**			Null out curdrv, no longer owner of drive
**		ELSE
**			Do nothing
**		ENDIF
** SDI_REMOVE:
**		IF currently owner of drive
**		THEN
**			Null out curdrv, no longer owner of drive,
**		ENDIF
**
**		Free owner block via free_onerblk()
***/
int
sdi_access(edtp, access, owner)
struct sdi_edt *edtp;
int access;
struct owner *owner;
{
	struct sdi_edt *edtp2;
	struct owner *op, *oprev;

		/* validate edtp and owner struct */
	if (!edtp || !owner ) {
		return SDI_RET_ERR;
	}

	edtp2 = findle_edt(edtp->hba_no, edtp->scsi_id, edtp->lun);

	if (edtp2->hba_no != edtp->hba_no ||
			edtp2->scsi_id != edtp->scsi_id ||
				edtp2->lun != edtp->lun) {
		/*
		 * Device not known to exist:
		 * grant the request if reasonable.
		 */

		switch (access) {
		case SDI_ADD|SDI_CLAIM:	/* add and claim */
		case SDI_ADD:		/* add to list */
			break;
		case 0:			/* remove */
		case SDI_CLAIM:		/* remove claim */
		default:
			return SDI_RET_ERR;
		}

		if (edtp2->hba_no != -1) {
			/* not head of hash queue, get a free edt */
			if (!(edtp2->hash_p = get_edt()) ) {
				/* no free entries left */
				return SDI_RET_ERR;
			}
			edtp2 = edtp2->hash_p;
		}

		*edtp2 = *edtp;
		edtp2->owner_list = owner;
		owner->next = (struct owner *)NULL;
	} else {
		/*
		 * Device is already in EDT.  Validate access, perform
		 * request if possible.
		 */
		switch (access) {
		case SDI_ADD|SDI_CLAIM:	/* add and claim */
			if (edtp2->curdrv) {
				return SDI_RET_ERR;
			}
			/* FALLTHRU */
		case SDI_ADD:		/* add to list */
			break;
		case 0:			/* remove */
			oprev = (struct owner *)NULL;
			for (op = edtp2->owner_list; op; op = op->next) {
				if (op->maj.b_maj == owner->maj.b_maj &&
					op->maj.c_maj == owner->maj.c_maj) {
					break;
				}
				oprev = op;
			}
			if (!op) {
				return SDI_RET_ERR;
			}
			if (oprev) {
				oprev->next = op->next;
			} else {
				edtp2->owner_list = op->next;
			}
			if (edtp2->curdrv == op) {
				edtp2->curdrv = (struct owner *)NULL;
			}
			return SDI_RET_OK;

		case SDI_REMOVE:
		case SDI_REMOVE|SDI_DISCLAIM:

			oprev = (struct owner *)NULL;

			/***
			** Find the owner block on the EDT's list.
			***/
			for (op = edtp2->owner_list; op; op = op->next) {
				if (op->maj.b_maj == owner->maj.b_maj &&
					op->maj.c_maj == owner->maj.c_maj) {
					break;
				}
				oprev = op;
			}

			if (!op) {
				/***
				** Owner block not on EDT's list!
				**/
				return( SDI_RET_ERR );
			}

			if (edtp2->curdrv == op) {
				edtp2->curdrv = (struct owner *)NULL;
			}

			if (oprev) {
				/***
				** There is an owner block in front
				** of the one being freed  on the EDT's list.
				***/
				oprev->next = op->next;
			}
			else {
				/***
				** The owner block being freed is the first
				** one on the EDT's list.
				***/
				edtp2->owner_list = op->next;
			}

			if( free_ownerblk( owner ) != 0 ) {
				return( SDI_RET_ERR );
			}

			return( SDI_RET_OK );

		case SDI_DISCLAIM:

			if (edtp2->curdrv == op) {
				edtp2->curdrv = (struct owner *)NULL;
			}

			return( SDI_RET_OK );

		case SDI_CLAIM:		/* remove claim */
		default:
			return SDI_RET_ERR;
		}
		if (edtp2->curdrv && SDI_CLAIM & access) {
			return SDI_RET_ERR;
		}
		owner->next = edtp2->owner_list;
		edtp2->owner_list = owner;
	}

	owner->edtp = edtp2;
	if (access & SDI_CLAIM) {
		edtp2->curdrv = owner;
	}
	return SDI_RET_OK;
}

sdi_wedt(edtp, devtype, inq)
struct sdi_edt *edtp;
int devtype;
char *inq;
{
	struct sdi_edt *link;

	if (!edtp || !edtp->curdrv) {
		return SDI_RET_ERR;
	}
	link = sdi_redt(edtp->hba_no, edtp->scsi_id, edtp->lun);
	if (link != edtp) {
		return SDI_RET_ERR;
	}
	edtp->pdtype = devtype;
	strncpy(edtp->inquiry, inq, INQ_LEN);
	return SDI_RET_OK;
}

sdi_fltinit(addr, func, param)
struct scsi_ad *addr;
void (*func)();
long param;
{
	struct sdi_edt *edtp;
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_fltinit(%x, %x, %x)\n", addr, func, param);
#endif

	if (!(edtp = sdi_redt(SDI_CONTROL(addr->sa_fill),
				SC_TCN(ad2dev_t(*addr)),
				SC_LUN(ad2dev_t(*addr)))) ) {
		return SDI_RET_ERR;
	} else if (!edtp->curdrv) {
		return SDI_RET_ERR;
	}

	edtp->curdrv->fault = func;
	edtp->curdrv->flt_parm = param;

	return SDI_RET_OK;
}

int
#if	(_SYSTEMENV == 4)
sdi_open(devp, flags, otype, cred_p)
dev_t	*devp;
cred_t	*cred_p;
#else
sdi_open(dev, flags, otype)
dev_t dev;
#endif
int	flags;
int	otype;
{
#if	(_SYSTEMENV == 4)
	dev_t dev = *devp;
#endif

	return (*HIP(HBA_tbl[SC_HAN(dev)].info)->hba_open)
#if	(_SYSTEMENV == 4)
				(devp, flags, otype, cred_p);
#else
				(dev, flags, otype);
#endif
}

int
sdi_close(dev, flags, otype, cred_p)
#if	(_SYSTEMENV == 4)
cred_t	*cred_p;
#else
int cred_p;
#endif
dev_t	dev;
int	flags;
int	otype;
{
	return (*HIP(HBA_tbl[SC_HAN(dev)].info)->hba_close)(dev, flags, otype, cred_p);
}

int
sdi_ioctl(dev, cmd, arg, mode, cred_p, rval_p)
#if	(_SYSTEMENV == 4)
cred_t	*cred_p;
int	*rval_p;
#else
int	cred_p;
int	rval_p;
#endif
dev_t	dev;
int	cmd;
caddr_t	arg;
int	mode;
{
	int 	hba_no=0;
	int	tc=0;
	int	lun=0;
	int 	edtsz=0;
	int	hba_active=0;
	struct 	scsi_edt *sc_edt;
	struct 	scsi_edt *sep;
	struct 	sdi_edt *edtp;
	char	buff[80];		/*** should be removed - used f/debug*/
	char	*ep, *sp;

	switch (cmd) {
	case B_HA_CNT:
		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {
			if (HBA_tbl[hba_no].active)
				hba_active++;
		}
                if (copyout((caddr_t)&hba_active, arg, sizeof(hba_active))) {
                        return(EFAULT);
		}
                break;

	case B_REDT:
		edtsz = sdi_hacnt * MAX_TCS * sizeof(struct scsi_edt);
		sc_edt=(struct scsi_edt *)kmem_zalloc(edtsz,KM_SLEEP);
		sep = sc_edt;

		for (hba_no = 0, hba_active=0; hba_no < sdi_hacnt; hba_no++) {

		  if (!HBA_tbl[hba_no].active)
			continue;
		  else
			hba_active++;

		  for (tc = 0; tc < MAX_TCS; tc++, sep++) {
		    for (lun = 0; lun < MAX_LUS; lun++) {
		      if (edtp = sdi_redt(hba_no,tc,lun)) {

			sep->tc_equip = 1;
			sep->ha_slot = hba_active;
		        sep->n_lus++;
		        sep->lu_id[lun] = 1;
			sep->pdtype = edtp->pdtype;
			strncpy(sep->tc_inquiry, edtp->inquiry, INQ_LEN);
			sep->tc_inquiry[INQ_LEN] = '\0';
		        if (edtp->curdrv) {
                                sep->c_maj = edtp->curdrv->maj.c_maj;
                                sep->b_maj = edtp->curdrv->maj.b_maj;
                                ep = edtp->curdrv->name;
                                sp = sep->drv_name;
                                while (*ep != ' ' && *ep != '\t' && *ep != '\0')      
					*sp++ = *ep++;
				*sp = '\0';
		        } else {
                          strcpy(sep->drv_name, "VOID");
                          sep->drv_name[4] = '\0';
		        }
		      }
		    }
		  }
		}

                if (copyout((caddr_t)sc_edt, arg, edtsz))
                        return(EFAULT);

		kmem_free(sc_edt, edtsz);
		sc_edt = NULL;

                break;
	default:
		return (*HIP(HBA_tbl[SC_HAN(dev)].info)->hba_ioctl)
					(dev, cmd, arg, mode, cred_p, rval_p);
	}
	return(0);
}

/*
** Function name: sdi_config()
** Description:
**	The target drivers pass to this function a pointer to their
**	tc_data structure which contains the inquiry strings of the
**	devices they support. This routine walks through the EDT data
**	searching for the inquiry strings that match. It returns the
**	number of TCs found, and for each TC a tc_edt structure is
**	populated.
**  Note: we don't know how many tcedts we can fill up, so if we overflow it
**        we're screwed
*/


sdi_config (drv_name, c_maj, b_maj, tc_data, tc_size, tc_edtp)
char		*drv_name;	/* driver ASCII name	*/
int		 c_maj;		/* character major num 	*/
int		 b_maj;		/* block major num   	*/
struct tc_data	*tc_data;	/* TC inquiry data	*/
int		 tc_size;	/* TC data size 	*/
struct tc_edt	*tc_edtp;	/* pointer to TC edt	*/
{

	register int	 i;
	struct tc_data	*tc_p;
	register unsigned char lun, scsi_id;
	register short 	 hba_no;
	int		 tc_count = 0;
	char		 found;
	struct sdi_edt *findle_edt();
	struct sdi_edt *hash;
	struct owner *ownerp, *alloc_ownerblk();

#ifdef SDI_DEBUG
	if(sdi_debug > 2)
	{
		printf("sdi_config(%s, %x, %x, %x, %x ,%x)\n",
			drv_name, c_maj, b_maj, tc_data, tc_size, tc_edtp);
		printf("sdi_hacnt = %x\n", sdi_hacnt);
	}
#endif
	
	for (hba_no = 0; hba_no < sdi_hacnt; hba_no++)
	{
	    for (scsi_id = 0; scsi_id < MAX_TCS; scsi_id++)
	    {
		if (scsi_id == IDP(HBA_tbl[hba_no].idata)->ha_id)
			continue;
		for(lun = 0; lun < MAX_LUS; lun++)
		{
			hash = findle_edt(hba_no, scsi_id, lun);
			if(hash->hba_no != hba_no || hash->scsi_id != scsi_id ||
				hash->lun != lun)
			{
#ifdef SDI_DEBUG
	if(sdi_debug > 3)
		printf("sdi_config: no match\n" );
#endif	
				continue;
			}

			found = FALSE;
			for (i = 0, tc_p = tc_data; i < tc_size; i++, tc_p++)
			{
#ifdef SDI_DEBUG
	if(sdi_debug > 3)
		printf("sdi_config: compare %s %s\n", 
			hash->inquiry, tc_p->tc_inquiry);
#endif	
				if (strncmp(hash->inquiry, 
					tc_p->tc_inquiry, INQ_LEN) == 0)
				{
					/* if someone else doesn't own it */
					if(!hash->curdrv){
						found = TRUE;
						break;
					}
				}
			}

			if (found)
			{
				/***
				** Get an owner structure.
				***/
				ownerp = alloc_ownerblk();
				if( ownerp == (struct owner *)NULL ) {
					/***
					** Owner Structure Allocation Failed.
					***/
					continue;
				}

				hash->curdrv = ownerp;
				hash->curdrv->name = drv_name;
				
				ownerp->edtp = hash;
				
				tc_edtp->ha_slot = hash->hba_no;
				tc_edtp->tc_id = hash->scsi_id;
				tc_edtp->n_lus = 1;
			        tc_edtp->lu_id[lun] = 1;

#ifdef SDI_DEBUG
	if(sdi_debug > 1){
		printf("sdi_config: ha_slot %x tc_id %x : %s", 
			tc_edtp->ha_slot, tc_edtp->tc_id, tc_p->tc_inquiry);
		ml_pause();
	}
#endif	
				tc_count++;
				tc_edtp++;
			}
		}
	    }
	}

#ifdef SDI_DEBUG
	if(sdi_debug > 1){
		printf("sdi_config: returning %x\n", tc_count);
		ml_pause();
	}
#endif	
	return (tc_count);
}

struct sb *
sdi_getblk()
{
	register struct xsb *sbp;

	sbp = (struct xsb *)sdi_get(&lg_poolhead, KM_SLEEP);
	sbp->sb.SCB.sc_comp_code = SDI_UNUSED;
	sbp->sb.SCB.sc_link = NULL;
	sbp->hbadata_p = NULL;
	sbp->owner_p = NULL;
	return (struct sb *)sbp;
}

long
sdi_freeblk(sbp)
struct sb *sbp;
{
	int c;

	if (sbp->sb_type == SFB_TYPE) {
		c = SDI_CONTROL(sbp->SFB.sf_dev.sa_fill);
	} else {
		c = SDI_CONTROL(sbp->SCB.sc_dev.sa_fill);
	}
	hba_freeblk(c, sbp);
	sdi_free(&lg_poolhead, (jpool_t *)sbp);
	return (SDI_RET_OK);
}

void
sdi_getdev(addr, dev)
struct scsi_ad *addr;
dev_t *dev;
{
	int i;
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_getdev(%x, %x)\n", addr, dev);
#endif
	if (SDI_ILLEGAL(SDI_HAN(addr), SDI_TCN(addr), SDI_LUN(addr)) ) {
		return;
	}
	*dev = makedevice(sdi_major,
			HAMINOR(SDI_HAN(addr), SDI_TCN(addr), SDI_LUN(addr)) );

	return;
}

void
sdi_name(addr, name)
struct scsi_ad *addr;
char *name;
{
	int i;
	struct hbagetinfo hbagetinfo;
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_name(%x, %s)\n", addr, name);
#endif
	
	i = SDI_CONTROL(addr->sa_fill);
	hbagetinfo.name = name;
#ifdef DEBUG
	if (!HIP(HBA_tbl[i].info)->hba_getinfo)
		cmn_err(CE_WARN, "SDI: HBA driver has no name()\n");
#endif
	if (HIP(HBA_tbl[i].info)->hba_getinfo)
		(*HIP(HBA_tbl[i].info)->hba_getinfo)( addr, &hbagetinfo);
}

void
sdi_iotype(addr, iotype, inq_dp)
struct scsi_ad *addr;
int *iotype;
struct	ident	*inq_dp;
{
	int i;
	struct hbagetinfo hbagetinfo;
	char name[48];
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_iotype(%x, %d)\n", addr, iotype);
#endif
	
	i = SDI_CONTROL(addr->sa_fill);
	hbagetinfo.name = name;
#ifdef DEBUG
	if (!HIP(HBA_tbl[i].info)->hba_getinfo)
		cmn_err(CE_WARN, "SDI: HBA driver has no iotype()\n");
#endif
	if (HIP(HBA_tbl[i].info)->hba_getinfo)
		(*HIP(HBA_tbl[i].info)->hba_getinfo)( addr, &hbagetinfo);

	*iotype = hbagetinfo.iotype;

	if(inq_dp->id_rmb)	{
		*iotype |= F_RMB;
	}
}

void
sdi_translate(sb, bflags, procp)
register struct sb *sb;
int bflags;
proc_t *procp;
{
	register struct xsb *xsb = (struct xsb *)sb;
	int i;

#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_translate(%x, %x, %x)\n", sb, bflags, procp);
#endif
	if (sb->sb_type == SFB_TYPE) {
		i = SDI_CONTROL(sb->SFB.sf_dev.sa_fill);
	} else {
		i = SDI_CONTROL(sb->SCB.sc_dev.sa_fill);
	}
	/* Check to see if we have allocated the hba specific  portion
	 * of the xsb data structure.  If we have not, then allocate
	 * one now.
	 */
	if(! xsb->hbadata_p ) {
		xsb->hbadata_p = HIP(HBA_tbl[i].info)->hba_getblk();
		xsb->hbadata_p->sb = xsb;
	}
	(*HIP(HBA_tbl[i].info)->hba_xlat)( xsb->hbadata_p, bflags, procp);
}

int
sdi_icmd(pt)
struct sb *pt;
{
	int i;
	struct xsb *xsb = (struct xsb *)pt;
	int retval;
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_icmd(%x)\n", pt);
#endif

	if (pt->sb_type == SFB_TYPE) {
		i = SDI_CONTROL(pt->SFB.sf_dev.sa_fill);
	} else {
		i = SDI_CONTROL(pt->SCB.sc_dev.sa_fill);
	}
	/* Check to see if we have allocated the hba specific  portion
	 * of the xsb data structure.  If we have not, then allocate
	 * one now.  We have probably already allocated the hbadata_p
	 * if the target driver called sdi_translate.
	 */
	if( xsb->hbadata_p == NULL)
	{
		xsb->hbadata_p = HIP(HBA_tbl[i].info)->hba_getblk();
		xsb->hbadata_p->sb = xsb;
	}
	retval = (*HIP(HBA_tbl[i].info)->hba_icmd)(xsb->hbadata_p);
	return retval;
}

long
sdi_send(pt)
register struct sb *pt;
{
	register struct xsb *xsb = (struct xsb *)pt;
	int i;
	
#ifdef SDI_DEBUG
	if(sdi_debug >= 1)
		printf("sdi_send(%x)\n", pt);
#endif

	if (pt->sb_type == SFB_TYPE) {
		i = SDI_CONTROL(pt->SFB.sf_dev.sa_fill);
	} else {
		i = SDI_CONTROL(pt->SCB.sc_dev.sa_fill);
	}
	/* Check to see if we have allocated the hba specific  portion
	 * of the xsb data structure.  If we have not, then allocate
	 * one now.  We have probably already allocated the hbadata_p
	 * if the target driver called sdi_translate.
	 */
	if( xsb->hbadata_p == NULL) {
		xsb->hbadata_p = HIP(HBA_tbl[i].info)->hba_getblk();
		xsb->hbadata_p->sb = xsb;
	}
	return ((*HIP(HBA_tbl[i].info)->hba_send)(xsb->hbadata_p));
}

void
sdi_callback(sbp)
register struct sb *sbp;
{
#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		printf("sdi_callback(%x)\n", sbp);
#endif

	if (sbp->sb_type == SCB_TYPE || sbp->sb_type == ISCB_TYPE) {
		if(sbp->SCB.sc_int)
			(sbp->SCB.sc_int)(sbp);
	} else {
		if(sbp->SFB.sf_int)
			(sbp->SFB.sf_int)(sbp);
	}
}

/*
 * sdi_aen()
 * The sdi_aen routine is used to notify target drivers of asynchronous events
 * occuring on ther devices and/or the devices' bus.  It takes four parameters:
 * an event code indicating what happened, the Host Bus Adapter Number,
 * the SCSI id and the logical unit number of the device that produced it.
 * If a target controller caused the event, the LUN value will be -1.  If a
 * BUS RESET occured, both the SCSI id and the LUN will be -1.  The event
 * code names are prefixed by SDI_FLT_ and reside in sys/sdi.h.
 * Sdi_aen scans the EDT to find the device(s) affected, and calls through the
 * fault routine in the owner block indicated by curdrv, passing the
 * flt_parm as an argument.
 */
void
sdi_aen(event, hba, scsi_id, lun)
int event;
int hba;
int scsi_id;
int lun;
{
	struct sdi_edt *edtp;
	void sdi_fltcallbk();

	if(scsi_id == -1)	/* do all */
	{
	    for (scsi_id = 0; scsi_id < MAX_TCS; scsi_id++)
	    {
		if (scsi_id == IDP(HBA_tbl[hba].idata)->ha_id)
			continue;
		if(lun == -1)	/* do all */
		{
			for(lun = 0; lun < MAX_LUS; lun++)
			{
				edtp = sdi_redt(hba, scsi_id, lun);
				sdi_fltcallbk(event, edtp);
			}
		}else{
			edtp = sdi_redt(hba, scsi_id, lun);
			sdi_fltcallbk(event, edtp);
		}
	    }
	} else{
		edtp = sdi_redt(hba, scsi_id, lun);
		sdi_fltcallbk(event, edtp);
	}
	
}

void
sdi_fltcallbk(event, edtp)
int event;
struct sdi_edt *edtp;
{
	if(edtp == NULL)
		return;
	if (!edtp->curdrv) 
		return ;
	
	if(!edtp->curdrv->fault)
		return;
	edtp->curdrv->fault(edtp->curdrv->flt_parm, event);
}
	


static
hba_freeblk(hba_no, sbp)
int hba_no;
struct sb *sbp;
{
#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		printf("hba_freeblk(%x, %x)\n", hba_no, sbp);
#endif
	if (((struct xsb *)sbp)->hbadata_p) {
		(*HIP(HBA_tbl[hba_no].info)->hba_freeblk)(((struct xsb *)sbp)->hbadata_p);
		((struct xsb *)sbp)->hbadata_p = (struct hbadata *)NULL;
	}
}

/*
** Function name: sdi_docmd()
** Description:
**	Create and send an SCB associated SCSI command. 
*/

sdi_docmd (hba, scsi_id, lun, cdb_p, cdbsz, data_p, datasz, rw_flag)
int	hba;		/* HA Controller 	*/
int	scsi_id;	/* target controller	*/
int	lun;		/* logical unit		*/
caddr_t	cdb_p;		/* pointer to cdb 	*/
long	cdbsz;		/* size of cdb		*/
caddr_t	data_p;		/* command data area 	*/
long	datasz;		/* size of buffer	*/
int	rw_flag;	/* read/write flag	*/
{
	register struct sb   *sp;
	register struct buf  *bp;
	struct proc	     *procp;
	int		     retcode;
	void		     sdi_int();
#ifdef SDI_DEBUG
	if(sdi_debug > 2)
		printf("sdi_docmd (%x, %x, %x, %x, %x, %x, %x %x)\n",
		    hba, scsi_id, lun, cdb_p, cdbsz, data_p, datasz, rw_flag);
#endif


	bp = getrbuf(KM_SLEEP);
	bp->b_flags |= B_BUSY;
	bp->b_iodone = NULL;

	sp = sdi_getblk();
	sp->sb_type       = ISCB_TYPE;
	sp->SCB.sc_int    = NULL;
	sp->SCB.sc_cmdpt  = cdb_p;
	sp->SCB.sc_cmdsz  = cdbsz;
	sp->SCB.sc_datapt = data_p;
	sp->SCB.sc_datasz = datasz;
	sp->SCB.sc_wd     = (long) bp;
	sp->SCB.sc_time   = (10 * ONE_SEC);
	sp->SCB.sc_dev.sa_lun  = lun;
	sp->SCB.sc_dev.sa_fill = ((hba << 3) | scsi_id);
	sp->SCB.sc_dev.sa_minor =
			(((hba&0x7) << 5) | ((scsi_id&0x7) <<2) | (lun&0x3));
        if (rw_flag & B_READ)
           sp->SCB.sc_mode = SCB_READ;
        else
           sp->SCB.sc_mode = SCB_WRITE;

	drv_getparm (UPROCP, (ulong*)&procp);
	sdi_translate (sp, rw_flag, procp);

	/* we assume that with interrupts off, icmd will complete the i/o
	 * and call back to sdi_int before returning here
   	 */
#ifdef SDI_DEBUG
	if(sdi_debug > 1)
		printf("sdi_docmd: calling sdi_icmd\n");
	if(sdi_debug > 3)
		ml_pause();
#endif
	sdi_icmd(sp);
	retcode = sp->SCB.sc_comp_code;
#ifdef SDI_DEBUG
	printf("sdi_docmd: retcode = %x\n", retcode);
	if(sdi_debug > 1)
		printf("sdi_docmd: calling sdi_freeblk\n");
	if(sdi_debug > 3)
		ml_pause();
#endif
	sdi_freeblk(sp);
	freerbuf(bp);
	return retcode;

}

/* %BEGIN HEADER
 * %NAME: sdi_blkio - Non-sector-size io for odd counts and offsets.
 * %DESCRIPTION: 
	Does blocking and deblocking for i/o requests that do not
	start on sector boundaries or for non-sector size counts.

	Kernel's idea of "sector" size is 512 (NBPSCTR).  Here we
	implement blocking/deblocking for actual disk sector sizes
	of 1K, 2K, 4K, ..., for both read and write operations.
 * %CALLED BY: target driver strategy()
 * %SIDE EFFECTS: 
 * %RETURN VALUES:
 * %END HEADER */
void
sdi_blkio(obp, sshift, strategy)
register buf_t	*obp;	/* the original buffer header		*/
unsigned int	sshift;	/* shift, to xlate blocks to sectors	*/
void	(*strategy)();
{
	register buf_t	*bp;
	unsigned int	i, lbps, ssize, smask;
	unsigned int	offset, count;
	void		sdi_blkio1();

	/*
	 * Figure out kernel block/sector to disk sector conversion factors
	 * e.g. for 2K , lbps = 4, sshift = 2, ssize = 0x800, smask = 0x7FF
	 */
	lbps = 1 << sshift;
	ssize = NBPSCTR << sshift;
	smask = ssize - 1;

 	if ((bp = (buf_t *) getrbuf(KM_NOSLEEP)) == 0)	{
 		obp->b_flags |= B_ERROR;
 		obp->b_error = EIO;
#ifdef DEBUG
 		cmn_err(CE_WARN, "SDI: sdi_blkio: cannot get buffer header");
#endif
 		biodone(obp);
 		return;
 	}
	*bp = *obp;
	bp->b_flags |= B_BUSY;
	bp->b_flags &= ~(B_PAGEIO|B_REMAPPED|B_DONE|B_ASYNC);
	bp->b_error = 0;
	bp->b_iodone = NULL;
	bp->b_vp = NULL;

	obp->b_resid = obp->b_bcount;

	/*
	 * If the i/o does not start at a disk sector boundary, then do the
	 * i/o on the first sector, contained within that single sector.
	 */
	if (i = (bp->b_blkno & (lbps-1))) {
		offset = i * NBPSCTR;
		count = ssize - offset;
		if (count > bp->b_bcount) count = bp->b_bcount;
		bp->b_blkno &= ~(lbps-1);
		sdi_blkio1(bp, offset, count, ssize, strategy);
		if (bp->b_flags & B_ERROR) {
			obp->b_flags |= B_ERROR;
			obp->b_error = bp->b_error;
			freerbuf(bp);
			biodone(obp);
			return;
		}
		if (bp->b_resid) {
			freerbuf(bp);
			biodone(obp);
			return;
		}
		bp->b_flags = obp->b_flags;
		bp->b_blkno += lbps;
		bp->b_bcount -= count;
		obp->b_resid -= count;
		bp->b_un.b_addr += count;
	}

	/*
	 * We are at a disk sector boundary now.
	 * If the remainder of the i/o request is 1 sector or more,
	 * then do the whole sectors by calling strategy() directly.
	 */
	count = bp->b_bcount & smask; /* any remaining partial sector */

	if (bp->b_bcount >= ssize) {
		bp->b_bcount &= ~smask;
		bp->b_flags &= ~B_DONE;
		strategy(bp);
		biowait(bp);
		if (bp->b_flags & B_ERROR) {
			obp->b_flags |= B_ERROR;
			obp->b_error = bp->b_error;
			freerbuf(bp);
			biodone(obp);
			return;
		}
		if (bp->b_resid) {
			obp->b_resid -= bp->b_bcount - bp->b_resid;
			freerbuf(bp);
			biodone(obp);
			return;
		}
		bp->b_blkno += bp->b_bcount >> SCTRSHFT;
		obp->b_resid -= bp->b_bcount;
		bp->b_un.b_addr += bp->b_bcount;
	}

	/*
	 * We are still at a disk sector boundary.
	 * Any residual i/o is for less than a disk sector.
	 */
	if (count) {
		sdi_blkio1(bp, 0, count, ssize, strategy);
		if (bp->b_flags & B_ERROR) {
			obp->b_flags |= B_ERROR;
			obp->b_error = bp->b_error;
		}
		else
			obp->b_resid -= count - bp->b_resid;
	}
	freerbuf(bp);
	biodone(obp);
	return;
}

/* %BEGIN HEADER
 * %NAME: sdi_blkio1 - Non-sector-size io within one disk sector.
 * %DESCRIPTION: 
	Does blocking and deblocking for i/o requests that do not
	start or end on a sector boundary, within a single sector.

	Kernel's idea of "sector" size is 512 (NBPSCTR).  Here we
	implement blocking/deblocking for actual disk sector sizes
	of 1K, 2K, 4K, ..., for both read and write operations.
 * %CALLED BY: blkio
 * %SIDE EFFECTS: 
 * %RETURN VALUES:
 * %END HEADER */
void
sdi_blkio1(obp, offset, count, ssize, strategy)
register buf_t	*obp;
unsigned int	offset;
unsigned int	count;
unsigned int	ssize;
void	(*strategy)();
{
	register buf_t	*bp;
	caddr_t		sect;

	obp->b_resid = obp->b_bcount;
 	if ((bp = (buf_t *) getrbuf(KM_NOSLEEP)) == 0)	{
 		obp->b_flags |= B_ERROR;
 		obp->b_error = EIO;
#ifdef DEBUG
 		cmn_err(CE_WARN, "SDI: sdi_blkio1 cannot get buffer header");
#endif
 		return;
 	}
	if ((sect = kmem_alloc(ssize, KM_NOSLEEP)) == 0) {
#ifdef DEBUG
 		cmn_err(CE_WARN, "SDI: sdi_blkio1 cannot get buffer");
#endif
 		obp->b_flags |= B_ERROR;
 		obp->b_error = EIO;
 		return;
 	}
	*bp = *obp;

	bp->b_flags |= (B_BUSY | B_READ);
	bp->b_flags &= ~(B_PAGEIO|B_REMAPPED|B_DONE|B_ASYNC);
	bp->b_error = 0;
	bp->b_iodone = NULL;
	bp->b_vp = NULL;
	bp->b_bcount = ssize;
	bp->b_un.b_addr = sect;
	strategy(bp);
	biowait(bp);
	if (bp->b_flags & B_ERROR) {
		obp->b_flags |= B_ERROR;
		obp->b_error = bp->b_error;
		kmem_free(sect, ssize);
		sect = NULL;
		freerbuf(bp);
		bp = NULL;
		return;
	}
	if (bp->b_resid) {
		kmem_free(sect, ssize);
		sect = NULL;
		freerbuf(bp);
		bp = NULL;
		return;
	}

	/*
	 * If reading, copy data into the original buffer.
	 * If writing, copy from the original buffer and rewrite.
	 */
	if (obp->b_flags & B_READ) {
		bcopy(sect + offset, obp->b_un.b_addr, count);
		obp->b_resid = 0;
	}
	else {
		bcopy(obp->b_un.b_addr, sect + offset, count);
		bp->b_flags &= ~(B_READ | B_DONE);
		strategy(bp);
		biowait(bp);
		if (bp->b_flags & B_ERROR) {
			obp->b_flags |= B_ERROR;
			obp->b_error = bp->b_error;
		}
		else if (bp->b_resid == 0)
			obp->b_resid = 0;
	}
	kmem_free(sect, ssize);
	sect = NULL;
	freerbuf(bp);
	bp = NULL;
	return;
}

/*
** Function name: sdi_swap16()
** Description:
**	This function swaps bytes in a 16 bit data type.
*/

short
sdi_swap16(x)
unsigned int x;
{
	unsigned short rval;

	rval =  (x & 0x00ff) << 8;
	rval |= (x & 0xff00) >> 8;
	return (rval);
}


/*
** Function name: sdi_swap24()
** Description:
**	This function swaps bytes in a 24 bit data type.
*/

sdi_swap24(x)
unsigned int x;
{
	unsigned int rval;

	rval =  (x & 0x0000ff) << 16;
	rval |= (x & 0x00ff00);
	rval |= (x & 0xff0000) >> 16;
	return (rval);
}


/*
** Function name: sdi_swap32()
** Description:
**	This function swaps bytes in a 32 bit data type.
*/

long
sdi_swap32(x)
unsigned long x;
{
	unsigned long rval;

	rval =  (x & 0x000000ff) << 24;
	rval |= (x & 0x0000ff00) << 8;
	rval |= (x & 0x00ff0000) >> 8;
	rval |= (x & 0xff000000) >> 24;
	return (rval);
}

int
sdi_gethbano(int n)
{
	int	cntlr;

	if(n < 0)	{
		cntlr = sdi_hacnt;
	}
	else	{
		cntlr = n;
	}

	if(cntlr >= sdi_hbaswsz)	{
		return(-ECONFIG);
	}

	if(HBA_tbl[cntlr].info != NULL)	{
		return(-ECONFIG);
	}

	return(cntlr);
}

int
sdi_register(void *infop, void *idatap)
{
	struct	hba_cfg	*hbap;
	int	i, cntlr, onext_hba;

	if((cntlr = IDP(idatap)->cntlr) < 0)	{
		return(-1);
	}

	if(cntlr >= sdi_hbaswsz)	{
		return(-ECONFIG);
	}

	onext_hba = sdi_hacnt;
	if((cntlr + 1) > sdi_hacnt)	{
		sdi_hacnt = cntlr + 1;
	}

	hbap = &HBA_tbl[cntlr];

	if(hbap->info != NULL)	{
		return(-ECONFIG);
	}

	hbap->info	= HIP(infop);
	hbap->idata	= IDP(idatap);
	hbap->active	= 1;

	if(sdi_add_edt(cntlr) <= 1)	{  /* <=1 because HBA has edt entry */
		hbap->info = NULL;
		hbap->idata = NULL;
		hbap->active = 0;
		sdi_hacnt = onext_hba;

		return(-1);
	}

	/* Now call the rinit functions currently registered. */
	for (i=0; i < sdi_rtabsz; i++) {
		if (sdi_rinits[i] != NULL)
			(*sdi_rinits[i])();
	}

	return(cntlr);
}

int
sdi_add_edt(int hba_no)
{
	struct scs	inq_cdb;
	register struct sdi_edt *edtp;
	register struct sdi_edt *hash;
	int scsiid, lun;
	struct sdi_edt * findle_edt();
	char *p, inquire[INQ_LEN];
	unsigned char pd_type;
	int i, ndev;
	struct scsi_ad scsi_ad;

	inq_cdb.ss_op = SS_INQUIR;	/* inquiry cdb */
	inq_cdb.ss_addr1 = 0;
	inq_cdb.ss_addr = 0;
	inq_cdb.ss_len = IDENT_SZ;
	inq_cdb.ss_cont = 0;

	ndev = 0;

	for (scsiid = 0; scsiid < MAX_TCS; scsiid++)	{
		inquire[0] = '\0';
		pd_type = ID_NODEV;

		for(lun = 0; lun < MAX_LUS; lun++)	{

			if(inq_data.id_type == ID_TAPE  &&
			   lun >= sdi_tape_luns) {
				break;
			}
			inq_cdb.ss_lun = lun;	
			if(sdi_docmd(hba_no, scsiid, lun,
				     &inq_cdb, SCS_SZ, &inq_data,
				     IDENT_SZ, B_READ) != SDI_ASW)	{
				continue;
			}

			if (!sdi_cklu(hba_no, scsiid, lun, &inquire, &pd_type)) {
				continue;
			}

			hash = findle_edt(hba_no, scsiid, lun);
			if(hash->hba_no != -1)	{
				if(!(edtp = get_edt()))	{
					continue;
				}
				hash->hash_p = edtp;
				hash = edtp;
			}
			hash->hash_p = NULL;
			hash->hba_no = hba_no;
			hash->scsi_id = scsiid;
			hash->lun = lun;
			hash->pdtype = inq_data.id_type;
			scsi_ad.sa_fill = (hba_no<<3) | scsiid;
			sdi_iotype(&scsi_ad, &hash->iotype, &inq_data);

			p = &inq_data.id_vendor[0];
			for (i = 0; i < (INQ_LEN -1); i++, p++)
				hash->inquiry[i] = *p;
			hash->inquiry[i] = NULL;

			ndev++;
		}
	}
	return(ndev);
}

char *
sdi_comp_str(comp_code)
ulong_t comp_code;
{
	register int index;

	for (index = 0; index < COMP_CODE_MSG_COUNT; index++) {
		if ( comp_code == CompCodeTable[index].CompCode )
			return(CompCodeTable[index].Msg);
	}

	return("Unknown");
}

char *
sdi_status_str(status_code)
uchar_t status_code;
{
	register int index;

	for (index = 0; index < STATUS_MSG_COUNT; index++) {
		if ( status_code == StatusTable[index].Status )
			return(StatusTable[index].Msg);
	}

	return("Unknown");
}

char *
sdi_sense_str(sense_key)
uchar_t sense_key;
{
	register int index;

	for (index = 0; index < SENSE_KEY_MSG_COUNT; index++) {
		if ( sense_key == SenseKeyTable[index].Key )
			return(SenseKeyTable[index].Msg);
	}

	return("Unknown");
}

char *
sdi_ext_sense_str(sense_key,qualifier)
uchar_t sense_key,qualifier;
{
	register int index;

	for (index = 0; index < EXT_SENSE_KEY_MSG_COUNT; index++) {
		if (( sense_key == ExtSenseKeyTable[index].Key ) &&
		    ( qualifier == ExtSenseKeyTable[index].Qual )) {
			return(ExtSenseKeyTable[index].Msg);
		}
	}

	return("Unknown");
}

void
sdi_errmsg(dev_id,addr,sbp,sense,type,err_code)
char *dev_id;
struct scsi_ad *addr;
struct sb *sbp;
struct sense *sense;
int type,err_code;
{
	int blkno;
	char name[NAMESZ];

	sdi_name(addr, name);

	switch (type) {
	case SDI_SFB_ERR:
		if (err_code == 0) {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR:",
				dev_id, name, sbp->SFB.sf_dev.sa_lun);
		} else {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR 0x%x:",
				dev_id, name, sbp->SFB.sf_dev.sa_lun, err_code);
		}
		cmn_err(CE_CONT, "Completion code indicates \"%s\"\n",
				sdi_comp_str(sbp->SFB.sf_comp_code));
		break;
	case SDI_CKCON_ERR:
/*
 *	Put recovered errors and not ready errors to putbuf
 */
		if ( sense->sd_key == SD_RECOVER || sense->sd_key == SD_NREADY ) {
		if (err_code == 0) {
			cmn_err(CE_WARN, "!%s Driver: %s LU %d - %s:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun,
				sdi_status_str(sbp->SCB.sc_status));
		} else {
			cmn_err(CE_WARN, "!%s Driver: %s LU %d - %s 0x%x:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun,
				sdi_status_str(sbp->SCB.sc_status), err_code);
		}
		if ( sense->sd_key != SD_NOSENSE ) {
			cmn_err(CE_CONT, "!A \"%s\" condition has been detected.\n", 
				sdi_sense_str(sense->sd_key));
			if ((sense->sd_sencode!=SC_NOSENSE) ||
				(sense->sd_qualifier!=SC_NOSENSE)) {
				cmn_err(CE_CONT, "!Additional data = \"%s\".\n",
					sdi_ext_sense_str(sense->sd_sencode,sense->sd_qualifier));
			}
		}

		if (sense->sd_valid)
		{
			blkno = sdi_swap32(sense->sd_ba);
			cmn_err(CE_CONT, "!Logical block address = 0x%x\n",blkno);
		}

		} else {	/* put messages on the console */

		if (err_code == 0) {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - %s:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun,
				sdi_status_str(sbp->SCB.sc_status));
		} else {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - %s 0x%x:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun,
				sdi_status_str(sbp->SCB.sc_status), err_code);
		}
		if ( sense->sd_key != SD_NOSENSE ) {
			cmn_err(CE_CONT, "A \"%s\" condition has been detected.\n", 
				sdi_sense_str(sense->sd_key));
			if ((sense->sd_sencode!=SC_NOSENSE) ||
				(sense->sd_qualifier!=SC_NOSENSE)) {
				cmn_err(CE_CONT, "Additional data = \"%s\".\n",
					sdi_ext_sense_str(sense->sd_sencode,sense->sd_qualifier));
			}
		}

		if (sense->sd_valid)
		{
			blkno = sdi_swap32(sense->sd_ba);
			cmn_err(CE_CONT, "Logical block address = 0x%x\n",blkno);
		}
		}
		break;
	case SDI_CKSTAT_ERR:
		if (err_code == 0) {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun);
		} else {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR 0x%x:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun, err_code);
		}
		cmn_err(CE_CONT, "Target status - \"%s\"\n",
			sdi_status_str(sbp->SCB.sc_status));
		break;
	case SDI_DEFAULT_ERR:
		if (err_code == 0) {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun);
		} else {
			cmn_err(CE_WARN, "%s Driver: %s LU %d - I/O ERROR 0x%x:",
				dev_id, name, sbp->SCB.sc_dev.sa_lun, err_code);
		}
		if (sense->sd_valid)
		{
			blkno = sdi_swap32(sense->sd_ba);
			cmn_err(CE_CONT, "Logical block address = 0x%x\n",blkno);
		}
		cmn_err(CE_CONT, "Completion code indicates \"%s\"\n",
				sdi_comp_str(sbp->SCB.sc_comp_code));
		break;
	}

	return;
}
