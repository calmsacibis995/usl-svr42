/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libia:common/lib/libia/events.c	1.1.5.5"
#ident  "$Header: events.c 1.2 91/06/21 $"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <audit.h>
#include <pfmt.h>
#include <locale.h>
#include <stdlib.h>

/*
 * Be sure that the message strings always specify the catalog
 * name explicitly, since no setcat() is done in the library.
 * And you don't want to necessarily use the catalog that the
 * calling command uses.
 */
#define FDR4  "uxcore:715:event type \"%s\" is not currently audited\n"
#define FDR3  "uxcore:716:invalid event type or class \"%s\" specified\n"
#define FDR9  "uxcore:711:unable to obtain event class information\n"
#define MSG_MALLOC       "uxcore:717:malloc() failed\n"
#define SETOPEN  	 "uxcore:718:\"open_rd\" and \"open_wr\" events should be set for events that may use a File Descriptor\n"
#define MSG_FIXED  "uxcore:719:event type \"%s\" is a fixed event and may not be manipulated\n"
#define MSG_OBJ  "uxcore:720:event type \"%s\" is not valid for object-level auditing\n"
#define MSG_KEYWORD  "uxcore:721:keyword \"%s\" may not be used in conjunction with event types or classes\n"
#define ALL	"all"
#define NONE	"none"
#define ALLBITS	0xFFFFFFFF

extern int	cr_atbl();
extern void	cr_evtbl();
static void	clremask(), setallsys(), setallobj(), needopen();
void		zclremask(), zsetallsys();

extern	char	**evtlist;	/* pointer to list of classes/types */
struct stat stbuf;

/* auditable events table */
struct adtevt adtevt[] =
{
/*	  EVENT			BIT-POSITION   	      OBJ_T   FIXED	
	---------		------------ 	      -----   ----- */
       { "NULL",		ADT_NULL,		0,	0 },
       { "access",		ADT_ACCESS,     	1,	0 },
       { "acct_off",		ADT_ACCT_OFF,   	0,	0 },
       { "acct_on",		ADT_ACCT_ON,		0,	0 },
       { "acct_sw",		ADT_ACCT_SW,		0,	0 },
       { "add_grp",		ADT_ADD_GRP,		0,	1 },
       { "add_usr",		ADT_ADD_USR,		0,	1 },
       { "add_usr_grp",		ADT_ADD_USR_GRP,	0,	1 },
       { "assign_lid",		ADT_ASSIGN_LID, 	0,	1 },
       { "assign_nm",		ADT_ASSIGN_NM,  	0,	1 },
       { "audit_buf",		ADT_AUDIT_BUF,		0,	1 },
       { "audit_ctl",		ADT_AUDIT_CTL,		0,	1 },
       { "audit_dmp",		ADT_AUDIT_DMP,		0,	1 },
       { "audit_evt",		ADT_AUDIT_EVT,		0,	1 },
       { "audit_log",		ADT_AUDIT_LOG,		0,	1 },
       { "audit_map",		ADT_AUDIT_MAP,		0,	1 },
       { "bad_auth",		ADT_BAD_AUTH,		0,	0 },
       { "bad_lvl",		ADT_BAD_LVL,		0,	0 },
       { "cancel_job",		ADT_CANCEL_JOB,		1,	0 },
       { "chg_dir",		ADT_CHG_DIR,		1,	0 },
       { "chg_nm",		ADT_CHG_NM,		1,	0 },
       { "chg_root",		ADT_CHG_ROOT,		1,	0 },
       { "chg_times",		ADT_CHG_TIMES,		1,	0 },
       { "cov_chan_1",		ADT_COV_CHAN_1,		0,	0 },
       { "cov_chan_2",		ADT_COV_CHAN_2,		0,	0 },
       { "cov_chan_3",		ADT_COV_CHAN_3,		0,	0 },
       { "cov_chan_4",		ADT_COV_CHAN_4,		0,	0 },
       { "cov_chan_5",		ADT_COV_CHAN_5,		0,	0 },
       { "cov_chan_6",		ADT_COV_CHAN_6,		0,	0 },
       { "cov_chan_7",		ADT_COV_CHAN_7,		0,	0 },
       { "cov_chan_8",		ADT_COV_CHAN_8,		0,	0 },
       { "create",		ADT_CREATE,		1,	0 },
       { "cron",		ADT_CRON,		0,	0 },
       { "dac_mode",		ADT_DAC_MODE,		1,	0 },
       { "dac_own_grp",		ADT_DAC_OWN_GRP,	1,	0 },
       { "date",		ADT_DATE,		0,	1 },
       { "deactivate_lid",	ADT_DEACTIVATE_LID,	0,	1 },
       { "def_lvl",		ADT_DEF_LVL,		0,	0 },
       { "del_nm",		ADT_DEL_NM,		0,	1 },
       { "disp_attr",		ADT_DISP_ATTR,		0,	0 },
       { "exec",		ADT_EXEC,		0,	0 },
       { "exit",		ADT_EXIT,		0,	0 },
       { "fcntl",		ADT_FCNTL,		1,	0 },
       { "file_acl",		ADT_FILE_ACL,		1,	0 },
       { "file_lvl",		ADT_FILE_LVL,		1,	0 },
       { "file_priv",		ADT_FILE_PRIV,		1,	0 },
       { "fork",		ADT_FORK,		0,	0 },
       { "init",		ADT_INIT,		0,	1 },
       { "iocntl",		ADT_IOCNTL,		1,	0 },
       { "ipc_acl",		ADT_IPC_ACL,		1,	0 },
       { "kill",		ADT_KILL,		0,	0 },
       { "link",		ADT_LINK,		1,	0 },
       { "login",		ADT_LOGIN,		0,	0 },
       { "lp_admin",		ADT_LP_ADMIN,		0,	0 },
       { "lp_misc",		ADT_LP_MISC,		0,	0 },
       { "misc",		ADT_MISC,		0,	0 },
       { "mk_dir",		ADT_MK_DIR,		1,	0 },
       { "mk_mld",		ADT_MK_MLD,		1,	0 },
       { "mk_node",		ADT_MK_NODE,		1,	0 },
       { "mod_grp",		ADT_MOD_GRP,		0,	1 },
       { "mod_usr",		ADT_MOD_USR,		0,	1 },
       { "mount",		ADT_MOUNT,		1,	0 },
       { "msg_ctl",		ADT_MSG_CTL,		1,	0 },
       { "msg_get",		ADT_MSG_GET,		1,	0 },
       { "msg_op",		ADT_MSG_OP,		1,	0 },
       { "open_rd",		ADT_OPEN_RD,		1,	0 },
       { "open_wr",		ADT_OPEN_WR,		1,	0 },
       { "page_lvl",		ADT_PAGE_LVL,		0,	0 },
       { "passwd",		ADT_PASSWD,		0,	0 },
       { "pipe",		ADT_PIPE,		1,	0 },
       { "pm_denied",		ADT_PM_DENIED,		0,	0 },
       { "proc_lvl",		ADT_PROC_LVL,		0,	0 },
       { "prt_job",		ADT_PRT_JOB,		1,	0 },
       { "prt_lvl",		ADT_PRT_LVL,		0,	0 },
       { "recvfd",		ADT_RECVFD,		1,	0 },
       { "rm_dir",		ADT_RM_DIR,		1,	0 },
       { "sched_lk",		ADT_SCHED_LK,		0,	0 },
       { "sched_rt",		ADT_SCHED_RT,		0,	0 },
       { "sched_ts",		ADT_SCHED_TS,		0,	0 },
       { "sem_ctl",		ADT_SEM_CTL,		1,	0 },
       { "sem_get",		ADT_SEM_GET,		1,	0 },
       { "sem_op",		ADT_SEM_OP,		1,	0 },
       { "set_attr",		ADT_SET_ATTR,		0,	0 },
       { "set_gid",		ADT_SET_GID,		0,	0 },
       { "set_grps",		ADT_SET_GRPS,		0,	0 },
       { "set_lvl_rng",		ADT_SET_LVL_RNG,	0,	0 },
       { "set_pgrps",		ADT_SET_PGRPS,		0,	0 },
       { "set_sid",		ADT_SET_SID,		0,	0 },
       { "set_uid",		ADT_SET_UID,		0,	0 },
       { "setrlimit",		ADT_SETRLIMIT,		0,	0 },
       { "shm_ctl",		ADT_SHM_CTL,		1,	0 },
       { "shm_get",		ADT_SHM_GET,		1,	0 },
       { "shm_op",		ADT_SHM_OP,		1,	0 },
       { "status",		ADT_STATUS,		1,	0 },
       { "sym_create",		ADT_SYM_CREATE,		1,	0 },
       { "sym_status",		ADT_SYM_STATUS,		1,	0 },
       { "tfadmin",		ADT_TFADMIN,		0,	0 },
       { "trunc_lvl",		ADT_TRUNC_LVL,		0,	0 },
       { "ulimit",		ADT_ULIMIT,		0,	0 },
       { "umount",		ADT_UMOUNT,		1,	0 },
       { "unlink",		ADT_UNLINK,		1,	0 },
       { "modpath",		ADT_MODPATH,		0,	0 },
       { "modadm",		ADT_MODADM,		0,	0 },
       { "modload",		ADT_MODLOAD,		1,	0 },
       { "moduload",		ADT_MODULOAD,		0,	0 },
       { "105",			105,			0,	0 },
       { "106",			106,			0,	0 },
       { "107",			107,			0,	0 },
       { "108",			108,			0,	0 },
       { "109",			109,			0,	0 },
       { "110",			110,			0,	0 },
       { "111",			111,			0,	0 },
       { "112",			112,			0,	0 },
       { "113",			113,			0,	0 },
       { "114",			114,			0,	0 },
       { "115",			115,			0,	0 },
       { "116",			116,			0,	0 },
       { "117",			117,			0,	0 },
       { "118",			118,			0,	0 },
       { "119",			119,			0,	0 },
       { "120",			120,			0,	0 },
       { "121",			121,			0,	0 },
       { "122",			122,			0,	0 },
       { "123",			123,			0,	0 },
       { "124",			124,			0,	0 },
       { "125",			125,			0,	0 },
       { "126",			126,			0,	0 },
       { "127",			127,			0,	0 },
       { "128",			128,			0,	0 },
       { "129",			129,			0,	0 },
       { "130",			130,			0,	0 },
       { "131",			131,			0,	0 },
       { "132",			132,			0,	0 },
       { "133",			133,			0,	0 },
       { "134",			134,			0,	0 },
       { "135",			135,			0,	0 },
       { "136",			136,			0,	0 },
       { "137",			137,			0,	0 },
       { "138",			138,			0,	0 },
       { "139",			139,			0,	0 },
       { "140",			140,			0,	0 },
       { "141",			141,			0,	0 },
       { "142",			142,			0,	0 },
       { "143",			143,			0,	0 },
       { "144",			144,			0,	0 },
       { "145",			145,			0,	0 },
       { "146",			146,			0,	0 },
       { "147",			147,			0,	0 },
       { "148",			148,			0,	0 },
       { "149",			149,			0,	0 },
       { "150",			150,			0,	0 },
       { "151",			151,			0,	0 },
       { "152",			152,			0,	0 },
       { "153",			153,			0,	0 },
       { "154",			154,			0,	0 },
       { "155",			155,			0,	0 },
       { "156",			156,			0,	0 },
       { "157",			157,			0,	0 },
       { "158",			158,			0,	0 },
       { "159",			159,			0,	0 },
       { "160",			160,			0,	0 },
       { "161",			161,			0,	0 },
       { "162",			162,			0,	0 },
       { "163",			163,			0,	0 },
       { "164",			164,			0,	0 },
       { "165",			165,			0,	0 },
       { "166",			166,			0,	0 },
       { "167",			167,			0,	0 },
       { "168",			168,			0,	0 },
       { "169",			169,			0,	0 },
       { "170",			170,			0,	0 },
       { "171",			171,			0,	0 },
       { "172",			172,			0,	0 },
       { "173",			173,			0,	0 },
       { "174",			174,			0,	0 },
       { "175",			175,			0,	0 },
       { "176",			176,			0,	0 },
       { "177",			177,			0,	0 },
       { "178",			178,			0,	0 },
       { "179",			179,			0,	0 },
       { "180",			180,			0,	0 },
       { "181",			181,			0,	0 },
       { "182",			182,			0,	0 },
       { "183",			183,			0,	0 },
       { "184",			184,			0,	0 },
       { "185",			185,			0,	0 },
       { "186",			186,			0,	0 },
       { "187",			187,			0,	0 },
       { "188",			188,			0,	0 },
       { "189",			189,			0,	0 },
       { "190",			190,			0,	0 },
       { "191",			191,			0,	0 },
       { "192",			192,			0,	0 },
       { "193",			193,			0,	0 },
       { "194",			194,			0,	0 },
       { "195",			195,			0,	0 },
       { "196",			196,			0,	0 },
       { "197",			197,			0,	0 },
       { "198",			198,			0,	0 },
       { "199",			199,			0,	0 },
       { "200",			200,			0,	0 },
       { "201",			201,			0,	0 },
       { "202",			202,			0,	0 },
       { "203",			203,			0,	0 },
       { "204",			204,			0,	0 },
       { "205",			205,			0,	0 },
       { "206",			206,			0,	0 },
       { "207",			207,			0,	0 },
       { "208",			208,			0,	0 },
       { "209",			209,			0,	0 },
       { "210",			210,			0,	0 },
       { "211",			211,			0,	0 },
       { "212",			212,			0,	0 },
       { "213",			213,			0,	0 },
       { "214",			214,			0,	0 },
       { "215",			215,			0,	0 },
       { "216",			216,			0,	0 },
       { "217",			217,			0,	0 },
       { "218",			218,			0,	0 },
       { "219",			219,			0,	0 },
       { "220",			220,			0,	0 },
       { "221",			221,			0,	0 },
       { "222",			222,			0,	0 },
       { "223",			223,			0,	0 },
       { "224",			224,			0,	0 },
       { "225",			225,			0,	0 },
       { "226",			226,			0,	0 },
       { "227",			227,			0,	0 },
       { "228",			228,			0,	0 },
       { "229",			229,			0,	0 },
       { "230",			230,			0,	0 },
       { "231",			231,			0,	0 },
       { "232",			232,			0,	0 },
       { "233",			233,			0,	0 },
       { "234",			234,			0,	0 },
       { "235",			235,			0,	0 },
       { "236",			236,			0,	0 },
       { "237",			237,			0,	0 },
       { "238",			238,			0,	0 },
       { "239",			239,			0,	0 },
       { "240",			240,			0,	0 },
       { "241",			241,			0,	0 },
       { "242",			242,			0,	0 },
       { "243",			243,			0,	0 },
       { "244",			244,			0,	0 },
       { "245",			245,			0,	0 },
       { "246",			246,			0,	0 },
       { "247",			247,			0,	0 },
       { "248",			248,			0,	0 },
       { "249",			249,			0,	0 },
       { "250",			250,			0,	0 },
       { "251",			251,			0,	0 },
       { "252",			252,			0,	0 },
       { "253",			253,			0,	0 },
       { "254",			254,			0,	0 },
       { "255",			255,			0,	0 },
};



/* 
 * 
 * create an audit event mask based upon 
 * given character string representation 
 * 	exec,fork,kill,...
 */
int
cremask(optmask,sp,emask)
int		optmask;
char		*sp;
adtemask_t	emask;
{
	char **evtbl;
	char *evtptr[ADT_NUMOFEVTS+1];	/* pointers to events to be set */
	register char *p;
	int i;
	int minus 	= 0;
	int plus 	= 0;
	int numevts	= 0;
	int found	= 0;
	int match_event = 0;

	if (*sp == '-') {
		minus++;
		sp++;
	} else if (*sp == '+') {
		plus++;
		sp++;
	} else if (*sp == '!'){
		if (optmask & ADT_OMASK)
			setallobj(emask);
		else
			setallsys(emask);
		minus++;
		sp++;
	} else clremask(emask);
	
	if ((stat(ADT_CLASSFILE,&stbuf) == -1) || stbuf.st_size == 0) {
                (void)pfmt(stderr,MM_ERROR,FDR9);
		return(-1);
	}else 
		if (cr_atbl(stbuf))
			return(-1);

	/*"all" and "none" are valid keywords.*/
	if (strcmp(sp, ALL) == 0) {
		if (optmask & ADT_OMASK)
			setallobj(emask);
		else
			setallsys(emask);
		return(0);
	}
	if (strcmp(sp, NONE) == 0) {
		clremask(emask);
		return(0);
	}

	/*Keywords are ignored if used in conjunction with event types or classes*/
	for (p=strchr(sp,','); sp; sp=p, p=strchr(sp,',')) {
		if (p)
			*p++ ='\0';
		if ((strcmp(sp, ALL) == 0) || (strcmp(sp, NONE) == 0)) {
                	(void)pfmt(stderr,MM_WARNING,MSG_KEYWORD,sp);
		}
		else
			evtptr[numevts++] = sp;
	}

	evtptr[numevts] = '\0';
	evtbl = &evtptr[0];
	cr_evtbl(evtbl);

	/*Each user requested event type is validated against the set*/
	/*of valid event types and the type of mask being set.       */ 
	/*A user requested event type is invalid if:                 */
	/*	-the event is not in the set of system defined       */ 
        /*       event types                                         */
	/*	-the event is a fixed event type                     */
	/*	-the object mask is being set and the event is       */
	/*	 not an object event type                            */
	/*	-the event is to be deleted, but it is not set       */
        /*In all cases the user is informed and processing continues.*/
	/*A return of 0 will indicate that at least one of the       */
	/*user entered event types was valid.                        */
	found=0;
	while (*evtlist != NULL) {
		match_event=0;
		for (i=1; i<=ADT_NUMOFEVTS; i++) {
			if (strcmp(*evtlist, adtevt[i].a_evtnamp) == 0)
			{
				match_event=1;

				/*A fixed event type can not be manipulated*/
				/*Inform the user and process the next event in the list*/
				if (adtevt[i].a_fixed != 0)
				{
                			(void)pfmt(stderr,MM_WARNING,MSG_FIXED,*evtlist);
					break;
				}

				/*Only event types which are defined to be object event*/
				/*types may be selected for the object mask.*/
				/*Inform the user and process the next event in the list*/
				if(optmask & ADT_OMASK)
				{
					if (adtevt[i].a_objt == 0)
					{
                				(void)pfmt(stderr,MM_WARNING,MSG_OBJ,*evtlist);
						break;
					}
				}

				if (minus) {
					if(!EVENTCHK(adtevt[i].a_evtnum,emask)){
                				(void)pfmt(stderr,MM_WARNING,FDR4,*evtlist);
						break;
					}
					EVENTDEL(adtevt[i].a_evtnum,emask); 
				}else
					EVENTADD(adtevt[i].a_evtnum,emask);
				found=1;
				break;
			}
		}

		/*Inform the user if the specified event type is not a system*/
		/*defined event type and process the next event in the list*/
		if (!match_event)
                	(void)pfmt(stderr,MM_WARNING,FDR3,*evtlist);

		evtlist++;
	}

	if (!found)
		return(1);
	else {
		(void)needopen(emask);
		return(0);
	}
}

/* 
 * clear all audit event mask positions
 */
void
clremask(emask)
adtemask_t emask;
{
	int i;

	for (i=0; i<ADT_EMASKSIZE/2; i++) 
		emask[i]=0;
}


/*
 * set all valid object level audit event mask positions
 */
void
setallobj(emask)
adtemask_t emask;
{
	int i;

	for (i=1; i<=ADT_NUMOFEVTS; i++) {
		if (adtevt[i].a_objt)
			EVENTADD(adtevt[i].a_evtnum,emask);
	}
}

/*
 * set all system wide audit event mask positions
 */
void
setallsys(emask)
adtemask_t emask;
{
	int i;
	for (i=0; i<ADT_EMASKSIZE/2; i++) 
		emask[i]=ALLBITS;
}

extern struct adtevt *getadtent();
/*
 * Given and audit event string,
 * return a pointer to the corresponding 
 * adtevt structure element.
 */
struct adtevt *
getevt(event)
const char	*event;
{
	register struct adtevt *p;
	
	setadtent();
	while ((p = getadtent()) != NULL && strcmp(event, p->a_evtnamp))
		;
	endadtent();
	return(p);
}


/* 
 * Given a hexadecimal audit event mask representation,
 * convert and return a pointer to the ascii 
 * event string representation.
 */
char *
cnvemask(emask)
adtemask_t emask;
{
	int i;
	int evtfound = 0;
	int allcnt = 0;
	int NUMOFEVTS = 255;
	static char buf[BUFSIZ];

        buf[0]=NULL;
	for (i=1;i<=NUMOFEVTS;i++)
		if (EVENTCHK(i,emask)) {
			allcnt++;
			if (i != (allcnt))	
				i = (NUMOFEVTS + 1);
		}
	if (allcnt == 0) {
		strcpy(buf,NONE);
		evtfound++;
	}else if (allcnt == NUMOFEVTS) {
		strcpy(buf,ALL);
		evtfound++;
	}else {
        	buf[0]=NULL;
		for (i=1; i<=NUMOFEVTS; ++i)	/* Skip zero position */
			if (EVENTCHK(i,emask)) {
				strcat(buf,adtevt[i].a_evtnamp);
				strcat(buf,",");
				evtfound++;
			}
	}
	if (evtfound>0)  
		return (&buf[0]);
	else 
		return ((char *)NULL);
}

/*
 * given an event number,
 * return a pointer to the event name string.
 */
char * 
prevtnam(num)
int num;
{
	return(adtevt[num].a_evtnamp);
}

/*
 * If an fd based event is set and the events
 * open_rd and open_wr are not set inform the
 * user
 */
void
needopen(emask)
adtemask_t emask;
{
 	if (  (EVENTCHK(ADT_CHG_DIR,emask))
           || (EVENTCHK(ADT_DAC_MODE,emask))
           || (EVENTCHK(ADT_DAC_OWN_GRP,emask))
           || (EVENTCHK(ADT_DISP_ATTR,emask))
           || (EVENTCHK(ADT_FILE_LVL,emask))
           || (EVENTCHK(ADT_RECVFD,emask))
           || (EVENTCHK(ADT_SET_ATTR,emask))
           || (EVENTCHK(ADT_STATUS,emask))   ) {

		if (   !EVENTCHK(ADT_OPEN_RD,emask)
		    || !EVENTCHK(ADT_OPEN_WR,emask) ) {
			/*********************************
			 * DO NOT SET, JUST INFORM!        *
			 * EVENTADD(ADT_OPEN_RD, emask); *
			 * EVENTADD(ADT_OPEN_WR, emask); *
			**********************************/
                	(void)pfmt(stdout,MM_INFO,SETOPEN);
		}
	}
}

/* 
 * 
 * create an audit event mask based upon 
 * given character string representation 
 * 128,129,130,... for trusted applications
 */
int
zcremask(sp,emask)
char		*sp;
adtemask_t	emask;
{
	char **evtbl;
	char *evtptr[257];	/* pointers to events to be set */
	register char *p;
	int i;
	int minus 	= 0;
	int plus 	= 0;
	int numevts	= 0;
	int found	= 0;
	int match_event = 0;

	if (*sp == '-') {
		minus++;
		sp++;
	} else if (*sp == '+') {
		plus++;
		sp++;
	} else if (*sp == '!'){
		zsetallsys(emask);
		minus++;
		sp++;
	} else zclremask(emask);
	
	if ((stat(ADT_CLASSFILE,&stbuf) == -1) || stbuf.st_size == 0) {
                (void)pfmt(stderr,MM_ERROR,FDR9);
		return(-1);
	}else 
		if (cr_atbl(stbuf))
			return(-1);

	/*"all" and "none" are valid keywords.*/
	if (strcmp(sp, ALL) == 0) {
		zsetallsys(emask);
		return(0);
	}
	if (strcmp(sp, NONE) == 0) {
		zclremask(emask);
		return(0);
	}

	/*Keywords are ignored if used in conjunction with event types or classes*/
	for (p=strchr(sp,','); sp; sp=p, p=strchr(sp,',')) {
		if (p)
			*p++ ='\0';
		if ((strcmp(sp, ALL) == 0) || (strcmp(sp, NONE) == 0)) {
                	(void)pfmt(stderr,MM_WARNING,MSG_KEYWORD,sp);
		}
		else
			evtptr[numevts++] = sp;
	}

	evtptr[numevts] = '\0';
	evtbl = &evtptr[0];
	cr_evtbl(evtbl);

	found=0;
	while (*evtlist != NULL) {
		match_event=0;
		for (i=128; i<=255; i++) {
			if (strcmp(*evtlist, adtevt[i].a_evtnamp) == 0)
			{
				match_event=1;

				if (minus) {
					if(!EVENTCHK(adtevt[i].a_evtnum,emask)){
                				(void)pfmt(stderr,MM_WARNING,FDR4,*evtlist);
						break;
					}
					EVENTDEL(adtevt[i].a_evtnum,emask); 
				}else
					EVENTADD(adtevt[i].a_evtnum,emask);
				found=1;
				break;
			}
		}

		/*Inform the user if the specified event type is not a system*/
		/*defined event type and process the next event in the list*/
		if (!match_event)
                	(void)pfmt(stderr,MM_WARNING,FDR3,*evtlist);

		evtlist++;
	}

	if (!found)
		return(1);
	else 
		return(0);
}

/*
 * set all Trusted Application audit event mask positions
 */
void
zsetallsys(emask)
adtemask_t emask;
{
	int i;
	for (i=4; i<ADT_EMASKSIZE; i++) 
		emask[i]=ALLBITS;
}

/* 
 * clear all Trusted Application audit event mask positions
 */
void
zclremask(emask)
adtemask_t emask;
{
	int i;

	for (i=4; i<ADT_EMASKSIZE; i++) 
		emask[i]=0;
}
