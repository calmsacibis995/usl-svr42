
/*=================================================================*/
/*
**	The ident string is enclosed in a comment because
**	'rpcgen' will choke on it if it isn't.
**
#ident	"@(#)lp:lib/lpNet/xdrMsgs.x	1.3.1.2"
#ident	"$Header: $"
*/
%#define	MSGS_VERSION_MAJOR	1
%#define	MSGS_VERSION_MINOR	2

/*---------------------------------------------------------*/
/*
**  This is the logical message TAG of the
**  network message.
*/
enum	networkMsgType
{
	JobControlMsg	= 0,
	SystemIdMsg	= 1,
	DataPacketMsg	= 2,
	FileFragmentMsg	= 3,
	PacketBundleMsg	= 4
};
enum	jobControlCode
{
	NormalJobMsg	 = 0,
	RequestToSendJob = 1,
	ClearToSendJob	 = 2,
	AbortJob	 = 3,
	JobAborted	 = 4,
	RequestDenied	 = 5
};
struct	routingControl
{
	unsigned int	sysId;		/*  Id of the sending system.  */
	unsigned int	msgId;		/*  Id of the message (DG)     */
};
struct	jobControl_1_1
{
	unsigned char	controlCode;
	unsigned char	priority;
	unsigned char	endOfJob;
	unsigned int	jobId;
	         long	timeStamp;
};
struct	jobControlMsg
{
	unsigned char	controlCode;
	unsigned char	priority;
	unsigned char	endOfJob;
	unsigned int	jobId;
	         long	timeStamp;
	unsigned int	uid;
	unsigned int	gid;
	unsigned int	lid;
	unsigned int	mode;
	string		ownerp <>;
};
struct	networkMsgTag_1_1
{
	unsigned char	versionMajor;	/*  Major and minor version    */
	unsigned char	versionMinor;	/*  of number our message set. */
	struct
	routingControl	routeControl;
	networkMsgType	msgType;
	struct
	jobControl_1_1	*jobControlp;
};

struct	networkMsgTag_1_2
{
	unsigned char	versionMajor;	/*  Major and minor version    */
	unsigned char	versionMinor;	/*  of number our message set. */
	networkMsgType	msgType;
	struct
	routingControl	routeControl;
};
/*---------------------------------------------------------*/
/*
**  The message set.
*/
struct	systemIdMsg
{
	string	systemNamep <>;
	opaque	data <>;
};

struct	dataPacketMsg
{
	int	endOfPacket;
	opaque	data <>;
};

struct	packetBundleMsg
{
	struct
	dataPacketMsg	packets <>;
};

struct	fileFragmentMsg_1_1
{
	int		endOfFile;
	long		sizeOfFile;
	string		destPathp <>;
	opaque		fragment <>;
};

struct	fileFragmentMsg_1_2
{
	int		endOfFile;
	unsigned long	uid;
	unsigned long	gid;
	unsigned long	lid;
	unsigned long	mode;
	unsigned long	sizeOfFile;
	string		ownerp <>;
	string		destPathp <>;
	opaque		fragment <>;
};
/*=================================================================*/
