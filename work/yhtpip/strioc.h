

/*****************************************************
 *
 * SVR 4.2 STREAMS YHTP - Release 1.0 
 *
 * Copyright 1994 NET612 Computer Department of NUDT 
 * All Rights Reserved. 
 *
 ****************************************************/




#ifndef _NET_YHTPIP_STRIOC_H	/* wrapper symbol for kernel use */
#define _NET_YHTPIP_STRIOC_H	/* subject to change without notice */



struct iocqp {
	unsigned short iqp_type;
	unsigned short iqp_value;
};

/* Queue types */
#define IQP_RQ		0		/* standard read queue */
#define IQP_WQ		1		/* standard write queue */
#define IQP_HDRQ	2		/* stream head read queue */
#define IQP_MUXRQ	3		/* multiplexor read queue */
#define IQP_MUXWQ	4		/* multiplexor write queue */
#define IQP_NQTYPES	5

#define MODL_INFO_SZ          2
#define DRVR_INFO_SZ          3
#define MUXDRVR_INFO_SZ       5

/* Queue parameter (value) types */
#define IQP_LOWAT	0x10		/* Low water mark */
#define IQP_HIWAT	0x20		/* High water mark */
#define IQP_NVTYPES     2

/* Masks */
#define IQP_QTYPMASK	0x0f
#define IQP_VTYPMASK	0xf0

/* Ioctl */
#define INITQPARMS      ('Q'<<8|0x01)

#ifdef _KERNEL
extern int yhinitqparms();
#endif
#endif	/* _NET_YHTPIP_STRIOC_H */
