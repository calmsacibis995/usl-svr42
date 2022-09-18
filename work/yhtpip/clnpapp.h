

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/




#define		CLNPVERSION	1	
#define    	ARP_VER         1
#define		ARP_PID    	0x80
#define 	CLNP_PID	0x81
#define  	CLNP_VER 	1
#define		CLNP_MAXTTL	32
#define		TP_PDU		0x1c
#define		TT_PDU		0x1d
#define 	ST_PDU		0x1e
#define 	UD_PDU		0x18
#define 	RT_PDU		0x11
#define 	ER_PDU		0x10
#define		OSI_ADDR_AFI    0x39
#define		OSI_ADDR_IDI    0x460F
#define 	OSI_ADDR_LEN    13
#define  	OSI_ADDR_MASK   0x3f
#define  	OSI_ADDR_RDI    0x7a
struct osi_addr {
	u_char 	addrlen;
	u_char 	afi	;
	u_char	idi[2]	;
	u_char	rdi[4]	;
	u_char sni[2]	;
	u_char	ntn[4]	;
};

struct clnp {
	u_char	clnp_p;			/* protocol */
        u_char	clnp_hl;		/* header length */
	u_char	clnp_v;			/* version */
	u_char	clnp_ttl;			/* time to live */
        u_char  clnp_typ;

#define	CLNP_SP 0x80			/*  fragment perm  flag */
#define	CLNP_MS 0x40			/* more fragments flag */
#define	CLNP_ER 0x20			/* ack fragments flag */

	u_char clnp_sl[2];                /* fragment   length */
	u_char	clnp_sum[2];			/* checksum */
	struct	osi_addr clnp_src;
	struct  osi_addr clnp_dst;	/* source and dest address */
};

struct fragment {
	u_short	clnp_id;			/* identification */
	u_short	clnp_off;			/* fragment offset field */
	u_short	clnp_len;			/* total length */
};
  

struct hdrarp {
	u_char yharp_pid;
	u_char yharp_v;
	u_char yharp_aop;
	u_char yharp_ahln;
	u_char yharp_apln;
	ether_addr_t  yharp_sha;
	struct osi_addr yharp_spa;
	ether_addr_t  yharp_tha;
	struct osi_addr yharp_tpa;
};
/* ---------------wbs---------------
 * a yhroute consists a destination net id and
 * a router's ether_address.
 */
struct yhroute {
	struct in_addr netid;
	ether_addr_t   routeraddr;
};
