/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:mail.h	1.78"
#endif

#include <stdio.h>
#include <string.h>
#include <memutil.h>
#include <buffutil.h>
#include <textbuff.h>
#include <sys/types.h>
#include <Xol/Flat.h>
#include <Gizmo/Gizmos.h>
#include <Gizmo/PopupGizmo.h>
#include "ListGizmo.h"
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/ModalGizmo.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/FileGizmo.h>
#include <DnD/OlDnDVCX.h>
#include <DtI.h>
#include <Xol/buffutil.h>
#include "text.h"

typedef Boolean			(*PFB)();
typedef void			(*PFV)();

typedef struct _Alias
   {
      char * name;
      char * addr;
      char   inUse;
   } Alias;

typedef Bufferof(Alias)		AliasTable;
typedef Bufferof (char *)	CharBuf;
typedef Bufferof (ListItem)	ListBuf;
typedef Bufferof (char *)	Token;

#ifdef DEBUG
#define FPRINTF(x)	fprintf x
#else
#define FPRINTF
#endif

#define FREEGIZMO(c, g) if (g!=0) { FreeGizmo (c, g); }

/* Gizmo names */

#define SUMMARY_LIST		"summaryList"
#define DELETE_LIST		"deleteList"
#define READ_MAIL		"readMail"
#define SEND			"sendBase"
#define APPEND			"appendFile"
#define HEADER_LIST		"headerList"
#define ALIAS_LIST      	"aliasList"
#define BRIEFHEADER		"briefHeader"

/* Non internationalizable text strings */

#define NTS_COMMENT0	"#dtmail:\n"
#define NTS_COMMENT1	"#dtmail: If you want to change the variables in\n"
#define NTS_COMMENT2	"#dtmail: this mailrc file and have those changes\n"
#define NTS_COMMENT3	"#dtmail: reflected in dtmail you should remove all\n"
#define NTS_COMMENT4	"#dtmail: lines starting with 'set zzz'.\n"

#define NTS_UX_MAILX			"UX:mailx: ERROR: "
#define NTS_UX_MAIL			"UX:mail: "
#define NTS_NO_MAIL		"No mail"
#define NTS_SET_ZZZ		"set zzz"
#define NTS_POUND_DTMAIL	"#dtmail"
#define NTS_SET_ZZZSIGN		"set zzzsign=\""
#define NTS_SET_ZZZMBOX		"set zzzMBOX=\"%s\"\n"
#define NTS_SET_ZZZRECORD_YES	"set zzzrecord=\"Yes\"\n"
#define NTS_SET_ZZZRECORD_NO	"set zzzrecord=\"No\"\n"
#define NTS_SET_ZZZNOBRIEF	"set zzznobrief\n"
#define NTS_SET_ZZZUNIGNORE	"set zzzunignore=\""
#define NTS_SET_ZZZIGNORE	"set zzzignore=\""
#define NTS_FORWARD		"Your mail is being forwarded to"
#define NTS_NO_APPL_MESS	"No applicable messages\n"
#define NTS_MAILX		"/bin/mailx"
#define NTS_SET_TEXT		"set prompt=%s"
#define NTS_READ_ONLY		"WARNING: You are already reading mail."
#define NTS_READ_ONLY_41	"UX:mailx: WARNING: You are already reading mail."
#define NTS_DEFAULTS		"iprompt= keepsave outfolder hold sendwait noasksub noaskcc noaskbcc noautoprint noignoreeof nodot nodebug cmd=\"%s\" PAGER= "
#define NTS_4DOT0		"4.0"
#define UX_MAILX_ERROR		"UX:mailx: ERROR: "
#define NTS_UX_INFO		"UX:mailx: INFO: "
#define NTS_PERMISSION_DENIED	"Permission denied\n"
#define NTS_EMPTY_FILE		"empty file\n"
#define NTS_NO_SUCH_FILE	"No such file or directory\n"
#define NTS_VAR_MAIL		"/var/mail"
#define NTS_EXIT_MNEMONIC	'E'
#define NTS_NEW_MNEMONIC	'N'
#define NTS_NOT_PRINTABLE	"*** Message content is not printable"
#define NTS_IGNORE_TEXT		"UX:mailx: INFO: No fields currently being ignored.\n"
#define NTS_MAILBOX		"mailbox"
#define NTS_DEFAULT_SAVE_FILE	"SavedMail.ml"
#define NTS_SENT_MAIL		"SentMail.ml"

#define IGNORE			"ignore"
#define DUMMY_FILE		"/"
#define FREENULL(p)		if (p != NULL) FREE(p)

/* Mailx commands */

#define NTS_FILE_CMD		"file"
#define NTS_VERSION_CMD		"version"
#define QUIT_SAVE		"q\n"
#define PIPE_CMD		"Pipe"
#define SET_DEAD		"set DEAD="
#define SET_CMD			"set"
#define SAVE_CMD		"save"
#define EQUALS			"="
#define FROM_CMD		"from"
#define FROM_1_DOLLAR		"from 1-$"
#define FROM_DOLLAR		"from $"
#define PRINT_CMD		"P"
#define BPRINT_CMD		"Bp"
#define DELETE_CMD		"d"
#define UNDELETE_CMD		"u"

#define PROMPT_TEXT	"\004\001\026\005"
#define PROMPT_LEN	4

/* The following define the position of fields */

#define F_PIXMAP	0
#define F_TEXT		1
#define F_STATUS	5
#define FORMAT		"%b %s"
#define NUM_FIELDS	2
#define BUF_SIZE	1024
#define BUF_SMALL	50

#define NUL		(XtPointer)0
#define GROWTH		20	/* leave room for some growth in aliases */
#define LISTHEIGHT	5	/* alias manager: 5 rows shown */
#define LISTWIDTH	80	/* alias manager: approximately 80 chars */
#define SHORT_PAUSE   3		/* milliseconds */

typedef enum {
	UndoOp, DeleteOp
} LastManageOperation;

typedef enum {
	UndoIt, DeleteIt, SelectIt, UnselectIt
} LastReadOperation;

typedef enum {Brief, Full} HeaderSettings;	/* Type of read header */
typedef enum {DontKnow, DontDoIt, DoIt} RecordType;

typedef enum {			
		HelpMail,	
		HelpManager,	
		HelpReader,			
		HelpSender,
		HelpAliasManager, 

		HelpTOC,	
		HelpDesk,

		HelpManagerOpen, 
		HelpManagerSaveAs,	
		HelpManagerUndelete,	

		HelpReaderOpen,		
		HelpReaderSaveAs,	

		HelpSenderOpen,	
		HelpSenderSaveAs,	

		HelpReaderProperties,
		HelpSenderProperties,

		HelpAliases,

		HelpAliasManagerUndelete,
		HelpAliasManagerOverwrite,
		HelpAliasManagerSure,

} HelpItemIndex;

typedef struct _SendRec {
	BaseWindowGizmo *	baseWGizmo;	/* Base window Gizmo */
	struct _SendRec *	next;
	FileGizmo *		saveAsPopup;
	FileGizmo *		openPopup;
	Boolean			used;		/* True = in use */
	Boolean			exitPending;	/* Exit after op */
	char *			saveFilename;	/* File to do saves into */
	char *			origText;	/* This text is used */
	char *			origTo;		/* to determine if */
	char *			origSubject;	/* the text has */
	char *			origCc;		/* changed. */
	char *			origBcc;
} SendRec;

typedef struct _ReadRec {
	BaseWindowGizmo *	baseWGizmo;	/* Base window Gizmo */
	struct _ReadRec *	next;
	LastReadOperation	lastOp;		/* For undo */
	struct _MailRec *	mp;		/* Pointer to mailx */
	int			message;	/* For next/prev */
	int			lastMessage;
	FileGizmo *		readOpenUserPopup;
	FileGizmo *		readSaveAsPopup;
} ReadRec;

typedef struct _ManageRec {
	BaseWindowGizmo *	baseWGizmo;
	struct _ManageRec *	next;
	struct _MailRec *	mp;
	Boolean			mapped;		/* This list is mapped */
	LastManageOperation	lastOp;		/* For undo */
	char *			previousDeletes;
	FileGizmo *		openUserPopup;
	FileGizmo *		saveAsPopup;
	PopupGizmo *		deleteListPopup;
} ManageRec;

typedef struct _AliasRec {
	BaseWindowGizmo *       baseWGizmo;
	ListHead *              aliasInfo;      /* first alias node */
	Boolean                 mapped;         /* window is mapped */
	Boolean                 unsaved;	/* any unsaved alias changes? */
	PopupGizmo *            aliasUndeletePopup;
	Widget			aliasWidget;	/* alias manager */
	Widget			aliasScrollWidget;	/* scroll list */
	Widget			aliasShowWidget;	/* alias list */
	AliasTable *		userAlias;	/* user aliases */
	AliasTable *		systemAlias;	/* system aliases */
	int			userCount;	/* number of user aliase */
	int			systemCount;	/* number of system aliase */
	int			aliasTotal;	/* user + system aliases */
	char *			userMailrc;	/* user mailrc file name */
	char *			systemMailrc;	/* system mailrc file name */
	CharBuf *		name;		/* alias names */
	CharBuf *		addr;		/* alias addresses */
	ListBuf *		list;		/* alias addresses */
					} AliasRec;

typedef struct _MailRec {
	ListHead *	alias;		/* Alias list for modify */
	ListHead *	summary;	/* Points to mail headings */
	ListHead *	deleted;	/* Points to deleted mail headings */
	mode_t		st_mode;	/* Permission flags */
	FILE *		fp[2];		/* Read & write to mail file */
	char *		filename;	/* Current mail file (full path) */
	int	numBaseWindows;	/* For exiting dtmail: numPopups == 0 */
	int	defaultItem;	/* Item initially selected by mailx */
	o_ino_t		inode;	/* Inode of currently open file */
	off_t		size;	/* Size of the currently open file in bytes */
	Boolean		noMail;	/* no mail associated with filename */
	ReadRec *	rp;	/* Read base window */
	ManageRec *	mng;	/* Manage base window */
	AliasRec *  aliasRec;   /* Alias base window */
	struct _MailRec *	next;
	PopupGizmo *	aliasPopup;
} MailRec;

extern char *		MYMALLOC (int size);
extern char *		MYREALLOC (char *p, int size);
#ifndef DELETE_C
extern void		ResetUndo (MailRec *mp);
extern char *		MyStrtok (char *string, char *sepset, char **savept);
extern Widget		GetDeletedListWidget (ManageRec *mng);
extern void		UpdateLists(ManageRec *mng);
extern void		UndeleteProc(ManageRec *mng);
extern void		ManageUndelete(ManageRec *mng);
extern void		ReadDelete(ReadRec *rp);
extern void		ManageDelete(MailRec *mp);
extern void		ManageUndo(ManageRec *mng);
extern void		ReadUndo(ReadRec *rp);
#endif /* DELETE_C */

#ifndef FILE_C
extern void		DisplayErrorPrompt(Widget shell, char *buf);
extern char *		AddDotMl(char *text);
extern Boolean		DisplaySaveStatus(BaseWindowGizmo *bw, char *buf);
extern void		ManageSaveSaveAsCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ManageSaveAs(ManageRec *mng);
extern void		ManageSave(ManageRec *mng);
extern void		QuitMailx(MailRec *mp);
extern void		UnmapShell (Widget sh);
extern void		ExitManager(ManageRec *mng);
extern void		ManageExitCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		FreeSummaryOrDeleteList(Widget wid, ListHead *lp);
extern void		RaiseManageWindow(MailRec *mp);
extern MailRec *	OpenNewMailFile(char *filename, BaseWindowGizmo *bw);
extern void		OpenCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		DisplayAlreadyOpen (
				BaseWindowGizmo *bw, char *filename
			);
extern void		OpenUser(ManageRec *mng);
#endif /* FILE_C */

#ifndef IO_C
extern void		AddNewMail (MailRec *mp, char *buf, int num);
extern void		WriteToMailx(MailRec *mp, char *string, int len);
extern char *		ProcessCommand(MailRec *mp, char *cmd, char *name);
extern char *		CompactListOfSelectedItems(
				MailRec *mp, ListHead *lp, char *name,
				int start, int end
			);
extern char *		ListOfSelectedItems(
				MailRec *mp, ListHead *lp, char *name,
				int start, int end
			);
extern char *		ListOfSelectedReadMessages(MailRec *mp);
extern char *		ReadFile(Widget shell, char *filename);
extern int		OpenFile(Widget shell, char *filename);
extern void		UpdateMailrc(MailRec *mp);
#endif /* IO_C */

#ifndef MAIL_C
extern void		DeleteMailRec(MailRec *mp);
extern void		GetSettings (MailRec *mp, char *buf);
extern MailRec *	OpenMailx();
extern char *		GetErrorText(
				int errnum, char *errmess,
				char *text, char *filename
			);
extern int		GetDefaultItem(MailRec *mp);
extern void		SetStatusField(char *status, char **fields);
extern void		CreateHeader(MailRec *mp, int i, char *string);
extern Boolean		SwitchMailx(
				MailRec *mp, char *filename,
				BaseWindowGizmo *bw
			);
extern MailRec *	FindMailRec(Widget wid);
#endif /* MAIL_C */

#ifndef MAIN_C
#ifdef DEBUG
extern char *		_strdup(char *s, char *f, int l);
#endif
extern void		ExitMainCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		HelpCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		DisplayNewMailMessage (MailRec *mp, char *filename);
extern char *		GetUserMailFile();

extern HelpInfo     	MailWinHelp;
extern HelpInfo     	MgrWinHelp;
extern HelpInfo     	MgrOpenWinHelp;
extern HelpInfo     	MgrSaveWinHelp;
extern HelpInfo     	ReaderWinHelp;
extern HelpInfo     	SenderWinHelp;
extern HelpInfo     	SenderSaveWinHelp;
extern HelpInfo     	ReadPropWinHelp;
extern HelpInfo     	SendPropWinHelp;
extern HelpInfo     	AliasMgrWinHelp;
extern HelpInfo     	TOCHelp;
extern HelpInfo     	HelpDeskHelp;

#endif /* MAIN_C */

#ifndef MANAGE_C
extern void		ExecuteCB (
				Widget wid,
				XtPointer client_data,
				OlFlatCallData *call_data
			);
extern void		PrintMessageList(
				MailRec *mp, 
				BaseWindowGizmo *bw,
				char *messages,
				char *printed
			);
extern Boolean		ManageDropNotify (
				Widget w,
				Window win,
				Position x,
				Position y,
				Atom selection,
				Time timestamp,
				OlDnDDropSiteID drop_site_id,
				OlDnDTriggerOperation op,
				Boolean send_done,
				Boolean forwarded,	/* not used */
				XtPointer closure
			);
extern void		WindowManagerEventHandler(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern ManageRec *	FindManageRec(Widget wid);
extern ListGizmo *	GetSummaryListGizmo(ManageRec *mng);
extern Widget		GetSummaryListWidget(ManageRec *mng);
extern void		DisplayStatus(MailRec *mp, char *buf);
extern void		UpdateFooter(MailRec *mp);
extern void		UpdateStatusOfMessage(MailRec *mp, int start, int end);
extern void		UnselectAll (ManageRec *mng, ListHead *lp, int except);
extern void		LookForAdjustCB(
				Widget wid,
				ListHead *lp,
				XtPointer call_data
			);
extern void		UnselectCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SelectCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		UndeleteCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		DisplayInLeftFooter(
				BaseWindowGizmo *bw,
				char *buf,
				Boolean raise
			);
extern void		CreateManageRec(MailRec *mp);
extern void		DeleteManageRec(ManageRec *mng);
#endif /* MANAGE_C */

#ifndef MENUS_C
extern void		CancelCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SetSensitivity(
				BaseWindowGizmo *bw, char *name,
				int start, int end, Boolean flag
			);
extern void		SetSaveFunction (PFV func);
#endif /* MENUS_C */

#ifndef POPUPS_C
extern ModalGizmo *	DefaultModalGizmo (
				Widget parent, MenuGizmo *menu, char *message
			);
extern char *		GetCurrentDirectory();
extern void		CreateSendOpenPopup(SendRec *sp);
extern void		CreateSaveReplyAsPopup(SendRec *sp);
extern void		CreateManageSaveAsPopup(ManageRec *mng);
extern void		CreateReadSaveAsPopup(ReadRec *rp);
extern void		CreateFileExistsModal(Widget parent);
extern void		CreateOpenUserPopup(ManageRec *mng);
extern void		CreateReadOpenUserPopup(ReadRec *rp);
extern void		CreateDeleteListPopup(ManageRec *mng);
extern void		CreateTextChangedModal(Widget parent);
extern void		CreateExitModal(Widget parent);
extern void		CreateNewSendModal(Widget parent);
extern void		CreateTransErrorModal(Widget parent, char *text);
extern void		CreateInvalidAddressModal(Widget parent, char *buf);
extern void		CreateInvalidTargetModal(Widget parent);
#endif /* POPUPS_C */

#ifndef READ_C
extern int		MenssageNumber (MailRec *mp, int item);
extern int		ItemNumber (MailRec *mp, int message);
extern void		NextReadMessage(ReadRec *rp);
extern ReadRec *	FindMessageInList(MailRec *mp, int item);
extern void		ExitReader(ReadRec *rp);
extern void		ClientDestroyCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern ReadRec *	CreateReadRec(MailRec *mp);
extern void		GetCurrentItem (MailRec *mp, ReadRec *rp);
extern Boolean		InsertIntoList (
				MailRec *mp, char *string,
				ListHead *hp, Boolean init
			);
extern ReadRec *	FindReadRec(Widget wid);
extern void		DeleteReadRec(ReadRec *rp);
extern void		ReadItem(MailRec *mp, ReadRec *rp, int i);
extern void		SelectItem(MailRec *mp, ListHead *lp, int item);
extern void		UnselectItem(MailRec *mp, ListHead *lp, int item);
extern Boolean		Prev(ReadRec *rp);
extern Boolean		Next(ReadRec *rp);
extern void		ReadSaveSaveAsCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern MailRec *	GetMailx (o_ino_t inode);
extern int		GetLastSelectedItem(MailRec *mp);
extern MailRec *	OpenReadWindowOnly(
				char *filename, BaseWindowGizmo *bw
			);
extern void		ReadProc(MailRec *mp);
extern void		DeleteRead (ReadRec *rp);
extern void		ReadManageCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ReadOpenCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
#endif /* READ_C */

#ifndef READPROP_C
extern void		InitBriefList(MailRec *mp);
extern void		ApplyToEachMailx(PFV cmd);
extern void		Unignore (MailRec *mp);
extern void		ReadPropPopupCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
#endif /* READPROP_C */

#ifndef SEND_C
extern void		DeleteSendRec(SendRec *sp);
extern SendRec *	CreateSendRec();
extern void		RemoveFileCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ManageSend();
extern Widget		GetToplevelShell(Widget wid);
extern Boolean		TextChanged(SendRec *sp);
extern void		LeaveSend(SendRec *sp);
extern SendRec *	FindSendRec(Widget wid);
extern void		ReallyExitCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		NewCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SendExitCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SendDropReply (SendRec *sp, Widget wid);
extern void		ReadReplyAllAttCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ReadReplySenderAttCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ReadReplyAllCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ReadReplySenderCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ReplyAll(Widget wid);
extern void		ReplyAllAtt(Widget wid);
extern void		ReplySenderProc(Widget wid);
extern void		ReplySenderAttProc(Widget wid);
extern void		Forward(MailRec *mp);
extern void		ReadForwardCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ReadSaveCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AppendFileCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		UndoCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ClearTextCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ClearAllCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		ClearHeaderCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SignCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SaveSaveReplyAsCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SendOpenCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SendFileCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SendEditCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		SendMailCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern Boolean		SendDropNotify (
				Widget w,
				Window win,
				Position x,
				Position y,
				Atom selection,
				Time timestamp,
				OlDnDDropSiteID drop_site_id,
				OlDnDTriggerOperation op,
				Boolean send_done,
				Boolean forwarded, /* not used */
				XtPointer closure
			);
extern void		SendAgainCB (
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
#endif /* SEND_C */

#ifndef SENDLIST_C
extern void		InitOriginalText(SendRec *sp);
extern SendRec *	AddToSendList();
extern void		MarkUnused(SendRec *sp);
#endif /* SENDLIST_C */

#ifndef SENDPROP_C
extern void		SendPropPopupCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
#endif /* SENDPROP_C */

#ifndef STAT_C
extern time_t		StatFile(char *filename, o_ino_t *inode, off_t *size);
extern mode_t		GetUmask();
#endif STAT_C

#ifndef ALIAS_C
extern void		NoticeMessage(
				char * mess
			);
extern void		InitAliasScrollingList(
			);
extern void		AliasWinInit (
				char * username
			);
#endif ALIAS_C

#ifndef ALIASEDIT_C
extern void		AddOverwriteCB (
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AddCancelCB (
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		HelpAddCB (
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasUndoCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasNewCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasAddCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasDeleteCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasUndeleteCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasUndeleteApplyCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern Boolean  OnlyOneQ(
                char *
            );
extern void     AliasShowPrep(
            );
#endif ALIASEDIT_C

#ifndef ALIASFILE_C
extern void		AliasSaveCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasPrintCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasExitCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasExitYesCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
#endif ALIASFILE_C

#ifndef ALIASSHOW_C
extern void		AliasShowPopupCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasShowSelectCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasShowUnselectCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern void		AliasDropProcCB(
				Widget wid,
				XtPointer client_data,
				XtPointer call_data
			);
extern char *		ExpandAlias(
				char * name
			);
#endif ALIASSHOW_C

#ifndef ALIASTBL_C

extern AliasTable *		ReadAlias(
				char * file
			);
static Token *	Tokenize(
				char * string
			);
static void		SortAliasTable(
				AliasTable * aliasTable
			);
static int		qstrcmp(
				const void * s1,
				const void * s2
			);
int         	DeleteAlias(
				char * name,
				AliasTable * aliasTable
			);
extern int		ReplaceAlias(
				char * alias,
				char * addresses,
				AliasTable * aliasTable
			);
extern char *	GenerateAliasTable(
				char * alias,
				AliasTable * aliasTable,
				char recurse
			);
#endif ALIASTBL_C

#ifndef ERROR_C

extern char *	GetTextGivenText (char *perror);
extern char *	GetTextGivenErrno (int errno);
#endif /* ERROR_C */
