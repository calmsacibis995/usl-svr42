/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _extern_h
#define _extern_h

#pragma ident	"@(#)dtm:extern.h	1.178"

/************************************************************************

	FUNCTION PROTOTYPES
*/
/* "exit" points for Desktop */
extern void	DmPromptExit(Boolean);
extern void	DmShutdownPrompt();
extern void	DmExit();
extern void     DmPrepareForRestart();

/*	Folder Routines							*/

/* Callbacks for Folder "File" menu items */
extern void     DmFileCopyCB(Widget, XtPointer, XtPointer);
extern void     DmFileDeleteCB(Widget, XtPointer, XtPointer);
extern void     DmFileLinkCB(Widget, XtPointer, XtPointer);
extern void     DmFileMoveCB(Widget, XtPointer, XtPointer);
extern void     DmFileNewCB(Widget, XtPointer, XtPointer);
extern void     DmFileOpenCB(Widget, XtPointer, XtPointer);
extern void     DmFilePrintCB(Widget, XtPointer, XtPointer);
extern void     DmFilePropCB(Widget, XtPointer, XtPointer);
extern void     DmFileRenameCB(Widget, XtPointer, XtPointer);

/* Callback for New Button*/
extern void    DmPromptCreateFile(DmFolderWinPtr fwp);

/* Callback for Find Button*/
extern void	DmFindCB(Widget, XtPointer, XtPointer);

/* Callbacks for Folder "View" menu items */
extern void	DmViewAlignCB(Widget, XtPointer, XtPointer);
extern void	DmViewCustomizedCB(Widget, XtPointer, XtPointer);
extern void	DmViewFormatCB(Widget, XtPointer, XtPointer);
extern void	DmViewSortCB(Widget, XtPointer, XtPointer);
extern void	DmFormatView(DmFolderWinPtr, DmViewFormatType);
extern void	DmSortItems(DmFolderWindow, DmViewSortType,
			    DtAttrs calc_geom_otions,
			    DtAttrs layout_options, Dimension wrap_width);

/* Callbacks for Folder "Edit" menu items */
extern void	DmEditSelectAllCB(Widget, XtPointer, XtPointer);
extern void	DmEditUnselectAllCB(Widget, XtPointer, XtPointer);
extern void	DmEditUndoCB(Widget, XtPointer, XtPointer);

/* Callbacks for Folder "Folder" menu items */
extern void	DmFolderOpenDirCB(Widget, XtPointer, XtPointer);
extern void	DmFolderOpenOtherCB(Widget, XtPointer, XtPointer);
extern void	DmFolderOpenParentCB(Widget, XtPointer, XtPointer);
extern void	DmFolderOpenTreeCB(Widget, XtPointer, XtPointer);

/* Callbacks for Folder/Wastebasket "Help" menu items */
extern void	DmHelpSpecificCB(Widget, XtPointer, XtPointer);
extern void	DmHelpTOCCB(Widget, XtPointer, XtPointer);
extern void	DmHelpDeskCB(Widget, XtPointer, XtPointer);

/* External routines for Tree View */
extern void	Dm__UpdateTreeView(char * dir1, char * dir2);
extern void	TreeIconMenuCB(Widget, XtPointer, XtPointer);

/* Routines for drawing non-standard file glyphs (icons) */
extern void	DmDrawLongIcon(Widget, XtPointer, XtPointer);
extern void	DmDrawNameIcon(Widget, XtPointer, XtPointer);
extern void	DmDrawTreeIcon(Widget, XtPointer, XtPointer);
extern void	DmDrawLinkIcon(Widget, XtPointer, XtPointer);

/*	Folder: to popup file property sheet(s)			*/
extern void	Dm__PopupFilePropSheet(DmFolderWinPtr, DmItemPtr);
extern void	Dm__PopupFilePropSheets(DmFolderWinPtr);

/*	Sync Timer						*/
extern void	Dm__SyncTimer(XtPointer client_data, XtIntervalId * timer_id);
extern void	Dm__SyncFolder(DmFolderWindow folder);
extern void	Dm__RmFromStaleList(DmFolderWindow folder);

/*	Visited Folders					*/
extern void	Dm__UpdateVisitedFolders(char * old_path, char * new_path);

extern void	Dm__Overwrite(DmTaskInfoListPtr tip, Boolean overwrite);
extern void	DmPromptOverwrite(DmFolderWindow, char *);
extern void	DmFolderFMProc(DmProcReason, XtPointer client_data,
			       XtPointer call_data, char * str1, char * str2);
extern void	Dm__FolderDropProc(Widget, XtPointer, XtPointer);
extern void	Dm__FolderSelect2Proc(Widget, XtPointer, XtPointer);
extern void	DmIconMenuProc(Widget, XtPointer, XtPointer);

/*	Interface to the Help subsystem				*/

extern DmHelpAppPtr	DmNewHelpAppID(Screen *scrn,
				       Window win_id,
				       char *app_name,
				       char *app_title,
				       char *node,
				       char *help_dir,
				       char *icon_file);
extern DmHelpAppPtr	DmGetHelpApp(int app_id);

extern int          DmDisplayHelpSection(DmHelpWinPtr hwp,
                              int  app_id,
                              char *title,
                              char *file_name,
                              char *sect_name,
                              int x, int y);

extern int		DmDisplayHelpTOC(Widget w, DmHelpWinPtr hwp,
					 char *title,
					 char *file_name,
					 int app_id);

extern void Dm__RootPropertyEventHandler(Widget w,
					 XtPointer client_data,
					 XEvent *xevent,
					 Boolean cont_to_dispatch);

/* Object utility functions */

extern Boolean	DmObjMatchFilter(DmFolderWindow folder, DmObjectPtr objptr);
extern void	DmDropObject(DmWinPtr dst_win, Cardinal dst_indx,
			     DmWinPtr src_wp, DmObjectPtr src_op);
extern DmObjectPtr DmFileToObject(char *file);
extern void	DmOpenObject(DmWinPtr wp, DmObjectPtr op);
extern void	DmPrintObject(DmWinPtr wp, DmObjectPtr op);

extern int	DmExecCommand(DmWinPtr wp, DmObjectPtr op,
			      char * name, char * str);
extern int	DmExecuteShellCmd(DmWinPtr wp, DmObjectPtr op, char * cmdstr,
				  Boolean force_chdir);
extern int	DmSameOrDescendant(char * path1, char * path2, int path1_len);
extern void	DmTouchIconBox(DmWinPtr window, ArgList, Cardinal);
extern void	Dm__StampContainer(DmContainerRec *);
extern char *DmGetDTProperty(char *name, DtAttrs *attrs);
extern int DmOpenDesktop();
extern DmFnameKeyPtr DmReadFileClassDB(char *filename);
extern int DmWriteFileClassDBList(DmFnameKeyPtr fnkp);
extern void DmSetFileClass(DmObjectPtr op);
extern void DmInitFileClass(DmFnameKeyPtr fnkp);
extern void DmFreeFileClass(DmFnameKeyPtr fnkp);
extern void DmInitSmallIcons(Widget w);
extern DmFnameKeyPtr DmGetClassInfo(DmFnameKeyPtr fnkp_list, char *name);
extern DmObjectPtr	Dm__CreateObj(DmContainerPtr, char *, DtAttrs);
extern DmObjectPtr	Dm__NewObject(DmContainerPtr cp, char * name);
extern DmObjectPtr	DmDupObject(DmObjectPtr op);
extern void		Dm__FreeObject(DmObjectPtr op);
extern DmContainerPtr	Dm__NewContainer(char *name);
extern int DmReadDtInfo(DmContainerPtr cp, char *filepath, DtAttrs options);
extern void DmWriteDtInfo(DmContainerPtr cp, char *filepath, DtAttrs options);
extern int DmRestartSession(char *path);
extern void DmSaveSession(char *path);
extern void DmSaveDesktopProperties(DmDesktopPtr desktop);
extern int Dm__ReadDir(DmContainerPtr cp, DtAttrs options);
extern DmContainerPtr DmOpenDir(char *path, DtAttrs options);
extern int DmCloseDir(char *path, DtAttrs options);
extern int DmFlushDir(char *path);
extern int DmFlushContainer(DmContainerPtr cp);
extern DmFmodeKeyPtr DmFtypeToFmodeKey(DmFileType ftype);
extern DmFmodeKeyPtr DmStrToFmodeKey(char *str);
extern DmFolderWinPtr DmOpenFolderWindow(char *path, DtAttrs attrs,
                   			 char *geom_str, Boolean iconic);
extern void	DmPrintCB(Widget, XtPointer, XtPointer);
extern void DmFolderPathChanged(char * old_path, char * new_path);
extern char * DmMakeFolderTitle(DmFolderWindow window);
extern char * DmGetFolderName(DmFolderWindow window);
extern DmFolderWinPtr DmQueryFolderWindow(char *path);
extern DmFolderWinPtr DmFindFolderWindow(Widget widget);
extern DmFolderWinPtr DmIsFolderWin(Window win);
extern void DmCloseFolderWindows(void);
extern void DmCloseFolderWindow(DmFolderWindow);
extern void DmCloseWindow(DmWinPtr window);
extern void DmSaveXandY(DmItemPtr ip, int count);
extern void	DmDelObjectFromContainer(DmContainerPtr cp, DmObjectPtr op);
extern DmObjectPtr DmGetObjectInContainer(DmContainerPtr cp, char *name);

extern char * DmClassName(DmFclassPtr fcp);
extern char * DmObjClassName(DmObjectPtr op);
extern DmItemPtr DmObjNameToItem(DmWinPtr win, register char * name);
extern int DmObjectToIndex(DmWinPtr wp, DmObjectPtr op);
extern DmItemPtr DmObjectToItem(DmWinPtr wp, DmObjectPtr op);
extern void **DmGetItemList(DmWinPtr window, int item_index);
extern void **DmOneItemList(DmItemPtr ip);
extern char **DmItemListToSrcList(void **ilist, int *count);
extern char *DmHasSystemFiles(void **ipp);

extern int	ItemLabelsMaxLen(DmItemPtr bip, int count);
extern void	DmComputeItemSize(DmItemPtr item, DmViewFormatType view_type,
				    Dimension * width, Dimension * height);

extern DmTaskInfoListPtr DmDoFileOp(DmFileOpInfoPtr, DmClientProc, XtPointer);
extern int		DmUndoFileOp(DmTaskInfoListPtr tip);

extern void DmComputeLayout(Widget, DmItemPtr, int count, int type,
			    Dimension width,
			    DtAttrs options,    /* size, icon pos. options */
			    DtAttrs lattrs);    /* layout attributes */


extern void DmStopCB(Widget, XtPointer, XtPointer);
extern void DmStopFileOp(DmTaskInfoListPtr tip);

extern void DmFreeTaskInfo(DmTaskInfoListPtr tip);
extern void DmUpdateWindow(DmFileOpInfoPtr, DtAttrs update_options);
extern void DmUpdateFolderTitle(DmFolderWindow);

extern Cardinal DmAddObjToFolder(DmFolderWindow, DmObjectPtr,
				 Position, Position, Dimension);
extern void DmRmItemFromFolder(DmFolderWindow folder, DmItemPtr item);

extern unsigned int DmInitWasteBasket(char *geom_str, Boolean iconic,
				      Boolean map_window);
extern char *DmGetLongName(DmItemPtr item, int len);

/* Help Desk external routines */
extern unsigned int DmInitHelpDesk(char *geom_str, Boolean iconic,
                          Boolean map_window);
extern void DmHDExit();
extern void DmHDOpenCB();


/* Wastebasket external routines */
extern int	DmWBCleanUpMethod(DmDesktopPtr);
extern void	DmWBExit(void);
extern void	DmWBFilePropCB(Widget, XtPointer, XtPointer);
extern void	DmMoveFileToWB(DmFileOpInfoPtr, Boolean client_req);
extern void	DmMoveFileFromWB(DmItemPtr, DmFileOpInfoPtr, Boolean);
extern int	DmMoveToWBProc2(char *pathname, Screen*, XEvent*, DtRequest*);
extern DmFolderWinPtr DmIsWB(Window);

extern void	DmHMExit();

/* functions to expand property */
extern char *	DmObjProp(char *str, XtPointer client_data);
extern char *	DmDropObjProp(char *str, XtPointer client_data);
extern void	DmSetSrcObject(DmObjectPtr op);
extern char *	DmDTProp(char *str, XtPointer client_data);

extern void	Dm__VaPrintMsg(char * msg, ... );
extern void	DmVaDisplayState(DmWinPtr window, char * msg, ...);
extern void	DmVaDisplayStatus(DmWinPtr window, int type, char * msg, ...);
extern char *	StrError(int err);
extern void     DmAllBusyWindow(DmFolderWindow, Boolean);
extern void DmBusyWindow(Widget w, Boolean state, XtCallbackProc strop_proc, XtPointer stop_data);
extern void DmAddWindow(DmWinPtr *list, DmWinPtr newp);
extern void DmDisplayBinder();
extern char *Dm__gettxt(char *msg);
extern void DmMenuSelectCB(Widget, XtPointer, XtPointer);
extern XtPointer DmGetWinPtr(Widget);
extern XtPointer DmGetLastMenuShell(Widget *);
extern void DmDisplayStatus(DmWinPtr window);
extern void DmMapWindow(DmWinPtr window);
extern void DmUnmapWindow(DmWinPtr window);
extern void DmButtonSelectProc(Widget, XtPointer, XtPointer);
extern void DmRetypeObj(DmObjectPtr op);
extern void DmSyncWindows(DmFnameKeyPtr new_fnkp, DmFnameKeyPtr del_fnkp);
extern DmFclassFilePtr DmCheckFileClassDB(char *filename);


/* drag&drop functions */
extern Boolean DmConvertSelectionProc(  Widget		w,
					Atom *		selection,
					Atom *		target,
					Atom *		type_rtn,
					XtPointer *	val_rtn,
					unsigned long *	length_rtn,
					int *		format_rtn);
extern void DmProtocolActionCB( Widget			w,
		   		Atom			selection,
		   		OlDnDProtocolAction	action,
		   		Boolean			convert_not_fail,
		   		XtPointer		closure);
extern void DmGetSelectionCB(   Widget		w,
	      			XtPointer	client_data,
	      			Atom *		selection,
	      			Atom *		type,
	      			XtPointer	value,
	      			unsigned long *	length,
	      			int *		format);
extern void DmTransactionStateProc(Widget			w,
		     		   Atom				selection,
		     		   OlDnDTransactionState	state,
		     		   Time				timestamp,
		     		   XtPointer			closure);
extern Boolean DmTriggerMsg(Widget                   w,
	     		    Window                   win,
	     		    Position                 x,
	     		    Position                 y,
	     		    Atom                     selection,
	     		    Time                     timestamp,
	     		    OlDnDDropSiteID          id,
	     		    OlDnDTriggerOperation    op,
	     		    Boolean                  send_done,
	     		    XtPointer                closure);
extern DmDnDInfoPtr
DmDnDNewTransaction(DmWinPtr				wp,
		    DmItemPtr				*list,
		    DtAttrs				attrs,
		    OlDnDDragDropInfoPtr		root_info,
		    Window				dst_win,
		    OlVirtualName			virtual_name,
		    XtConvertSelectionProc		convert_proc,
		    OlDnDTransactionStateCallback	trans_state_proc);
extern void DmDnDFreeTransaction(DmDnDInfoPtr dip);

extern Boolean
DmFolderTriggerNotify(Widget			w,
		      Window			win,
		      Position			x,
		      Position			y,
		      Atom			selection,
		      Time			timestamp,
		      OlDnDDropSiteID		drop_site_id,
		      OlDnDTriggerOperation	op,
		      Boolean			send_done,
		      Boolean			forwarded, /* not used */
		      XtPointer			closure);

extern Boolean
DmTBTriggerNotify(Widget			w,
		  Window			win,
		  Position			x,
		  Position			y,
		  Atom				selection,
		  Time				timestamp,
		  OlDnDDropSiteID		drop_site_id,
		  OlDnDTriggerOperation		op,
		  Boolean			send_done,
		  Boolean			forwarded, /* not used */
		  XtPointer			closure);

#endif /* _extern_h */
