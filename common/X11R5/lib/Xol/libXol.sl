#ident	"@(#)shlib:libXol.sl	1.29"

#target /usr/X/lib/libXol_s

#address .text 0xA1800000
#address .data 0xA1C00000
## 0x801a0000 for 3b2

## list	all functions to be exported
#branch
	AcceptFMReply 				1
	AcceptWSMReply 				2
	AppendToResourceBuffer 			3
	CheckCallback 				4
	ClearFMQueue 				5
	ClearWSMQueue 				6
	DequeueFMRequest 			7
	DequeueWSMRequest 			8
	EnqueueCharProperty 			9
	EnqueueFMRequest 			10
	EnqueueWSMRequest 			11
	FMInitialize 				12
	GetAtomList 				13
	GetCharProperty 			14
	GetHelpKeyMessage 			15
	GetLongProperty 			16
	GetOLWinAttr 				17
	GetOLWinColors 				18
	GetWMDecorationHints 			19
	GetWMPushpinState 			20
	GetWMState 				21
	GetWMWindowBusy 			22
	InitializeOpenLook 			23
	InitializeResourceBuffer 		24
	Look 					25
	MoveNextPage 				26
	MovePreviousPage 			27
	OlAsciiSinkCreate 			28
	OlAsciiSinkDestroy 			29
	OlConvertVirtualTranslation 		30
	OlCreatePackedWidget 			31
	OlCreatePackedWidgetList 		32
	OlDiskSourceCreate 			33
	OlDiskSourceDestroy 			34
	OlDiskSrcRead 				35
	OlError 				36
	OlGetResolution 			37
	OlInitialize 				38
	OlMenuPost 				39
	OlMenuUnpost 				40
	OlPostInitialize 			41
	OlPreInitialize 			42
	OlRegisterHelp 				43
	OlSetErrorHandler 			44
	OlSetWarningHandler 			45
	OlStringSourceCreate 			46
	OlStringSourceDestroy 			47
	OlTextCopySubString 			48
	OlTextInvalidate 			49
	OlTextTopPosition 			50
	OlTextUnsetSelection 			51
	OlWarning 				52
	SendLongNotice 				53
	SendProtocolMessage 			54
	SendResourceBuffer 			55
	SendFMReply 				56
	SendWSMReply 				57
	SetWMDecorationHints 			58
	SetWMIconSize 				59
	SetWMPushpinState 			60
	SetWMState 				61
	SetWMWindowBusy 			62
	StFetchSelection 			63
	WSMInitialize 				64
	_OlAddVirtualMappings 			65
	_OlBeepDisplay 				66
	_OlButtonPreview 			67
	_OlComposeArgList 			68
	_OlAppAddVirtualMappings 		69
	_OlCvtStringToFontStruct 		70
	_OlCvtStringToGravity 			71
	_OlCvtStringToOlDefine 			72
	_OlCvtStringToPixel 			73
	_OlDisplayFontCache 			74
	_OlDrawHighlightButton 			75
	_OlDrawNormalButton 			76
	_OlDumpVirtualMappings 			77
	_OlFlatExpandItem 			78
	_OlFreeFont 				79
	_OlFreeImage 				80
	_OlGetApplicationTitle 			81
	_OlGetFont 				82
	_OlGetImage 				83
	_OlGetMouseDampingFactor 		84
	_OlGetShellOfWidget 			85
	_OlGetTrollWidget 			86
	_OlGetVirtualMappings 			87
	_OlGrabPointer 				88
	_OlInitVirtualMappings 			89
	_OlIsMenuMouseClick 			90
	_OlIsVirtualButton 			91
	_OlIsVirtualEvent 			92
	_OlIsVirtualKey 			93
	_OlListLength 				94
	_OlListTail 				95
	_OlNodeIndex 				96
	_OlOffsetNode 				97
	_OlPopupHelpTree 			98
	_OlPropagateMenuState 			99
	_OlPtBuild 				100
	_OlPtDestroy 				101
	_OlPtInitialize 			102
	_OlPtLineFromPos 			103
	_OlPtPosFromLine 			104
	_OlPtSetLine 				105
	_OlRegisterOlDefineType 		106
	_OlAddTextVirtualMappings 		107
	_OlScrollText 				108
	_OlSetApplicationTitle 			109
	_OlSetPressDragMode 			110
	_OlSetWMProtocol 			111
	_OlTextCopySubString 			112
	_OlTextScrollAbsolute 			113
	_OlUngrabPointer 			114
	_OlUpdateVerticalSB 			115
	OlVaDisplayErrorMsg 			116
	OlVaDisplayWarningMsg 			117
	OlSetVaDisplayErrorMsgHandler 		118
	OlSetVaDisplayWarningMsgHandler 	119
	_OlRegisterFocusWidget 			120
	_OlSharedLibFixup 			121
	_OlAddEventHandler 			122
	_OlRemoveEventHandler 			123
	OlCallAcceptFocus 			124
	OlMoveFocus 				125
	_OlCallHighlightHandler 		126
	_OlDeleteDescendant 			127
	_OlInsertDescendant 			128
	OlHasFocus 				129
	OlCanAcceptFocus 			130
	_OlClearWidget 				131
	_OlGrabServer 				132
	_OlSelectDoesPreview 			133
	_OlUngrabServer 			134
	_OlMark 				135
	_OlDisplayMark 				136
	OlTextGetSelectionPos 			137
	OlTextSetSelection 			138
	_AsciiDisplayMark 			139
	OlFlatGetValues 			140
	OlFlatSetValues 			141
	_OlCopyFromXtArgVal 			142
	_OlCopyToXtArgVal 			143
	_OlDoGravity 				144
	_OlFlatDoLayout 			145
	_OlFlatPreviewItem 			146
	_OlFlatRefreshItem 			147
	_OlFlatInitializeItems 			148
	_OlWidgetToGadget 			149
	_OlConvertToXtArgVal 			150
	_OlFlatScreenManager 			151
	OlLayoutScrolledWindow 			152
	GetOlSWGeometries 			153
	AllocateBuffer 				154
	AllocateTextBuffer 			155
	BackwardScanTextBuffer 			156
	CopyBuffer 				157
	CopyTextBufferBlock 			158
	EndCurrentTextBufferWord 		159
	ForwardScanTextBuffer 			160
	FreeBuffer 				161
	FreeTextBuffer 				162
	GetOlBusyCursor 			163
	GetOlDuplicateCursor 			164
	GetOlMoveCursor 			165
	GetOlPanCursor 				166
	GetOlQuestionCursor 			167
	GetOlStandardCursor 			168
	GetOlTargetCursor 			169
	GetTextBufferBlock 			170
	GetTextBufferBuffer 			171
	GetTextBufferChar 			172
	GetTextBufferLine 			173
	GetTextBufferLocation 			174
	GrowBuffer 				175
	IncrementTextBufferLocation 		176
	InsertIntoBuffer 			177
	LastTextBufferLocation 			178
	LastTextBufferPosition 			179
	LineOfPosition 				180
	LocationOfPosition 			181
	LookupOlColors 				182
	LookupOlInputEvent 			183
	NextLocation 				184
	NextTextBufferWord 			185
	OlCallDynamicCallbacks 			186
	OlDetermineMouseAction 			187
	OlDragAndDrop 				188
	OlGet50PercentGrey 			189
	OlGet75PercentGrey 			190
	OlGetApplicationResources 		191
	OlGrabDragPointer 			192
	OlRegisterDynamicCallback 		193
	OlReplayBtnEvent 			194
	OlTextEditClearBuffer 			195
	OlTextEditCopyBuffer 			196
	OlTextEditCopySelection 		197
	OlTextEditGetCursorPosition 		198
	OlTextEditGetLastPosition 		199
	OlTextEditInsert 			200
	OlTextEditPaste 			201
	OlTextEditReadSubString 		202
	OlTextEditRedraw 			203
	OlTextEditResize 			204
	OlTextEditSetCursorPosition 		205
	OlTextEditTextBuffer 			206
	OlTextEditUpdate 			207
	OlTextFieldCopyString 			208
	OlTextFieldGetString 			209
	OlUngrabDragPointer 			210
	OlUnregisterDynamicCallback 		211
	PositionOfLine 				212
	PositionOfLocation 			213
	PreviousLocation 			214
	PreviousTextBufferWord 			215
	ReadFileIntoBuffer 			216
	ReadFileIntoTextBuffer 			217
	ReadStringIntoBuffer 			218
	ReadStringIntoTextBuffer 		219
	RegisterTextBufferScanFunctions 	220
	RegisterTextBufferUpdate 		221
	RegisterTextBufferWordDefinition 	222
	ReplaceBlockInTextBuffer 		223
	ReplaceCharInTextBuffer 		224
	SaveTextBuffer 				225
	StartCurrentTextBufferWord 		226
	UnregisterTextBufferUpdate 		227
	_BuildWrapTable 			228
	_CalculateCursorRowAndXOffset 		229
	_Calloc 				230
	_ChangeTextCursor 			231
	_CharWidth 				232
	_CreateCursor 				233
	_CreateCursorFromBitmaps 		234
	_CreateCursorFromData 			235
	_CreateCursorFromFiles 			236
	_CreateTextCursors 			237
	_DisplayText 				238
	_DrawTextCursor 			239
	_DrawWrapLine 				240
	_ExtendSelection 			241
	_FirstWrapLine 				242
	_Free 					243
	_GetBandWXColors 			244
	_GetNextWrappedLine 			245
	_IncrementWrapLocation 			246
	_IsGraphicsExpose 			247
	_LastDisplayedWrapLine 			248
	_LastWrapLine 				249
	_LineNumberOfWrapLocation 		250
	_LocationOfWrapLocation 		251
	_Malloc 				252
	_MoveCursorPosition 			253
	_MoveCursorPositionGlyph 		254
	_MoveDisplay 				255
	_MoveDisplayLaterally 			256
	_MoveDisplayPosition 			257
	_MoveSelection 				258
	_MoveToWrapLocation 			259
	_NextTabFrom 				260
	_NumberOfWrapLines 			261
	_OlDisplayTextEditLineNumberMargin 	262
	_OlDynamicHandler 			263
	_OlPopupHelpTree 			264
	_OlRegisterTextLineNumberMargin 	265
	_OlUnregisterTextLineNumberMargin 	266
	_PositionFromXY 			267
	_PositionOfWrapLocation 		268
	_Realloc 				269
	_RectFromPositions 			270
	_SetDisplayLocation 			271
	_SetMemutilDebug 			272
	_SetTextXOffset 			273
	_StringOffsetAndPosition 		274
	_StringWidth 				275
	_TextEditOwnPrimary 			276
	_TurnTextCursorOff 			277
	_UpdateWrapTable 			278
	_WrapLine 				279
	_WrapLineEnd 				280
	_WrapLineLength 			281
	_WrapLineOffset 			282
	_WrapLocationOfLocation 		283
	_WrapLocationOfPosition 		284
	_XtLastTimestampProcessed 		285
	strclose 				286
	streexp 				287
	strexp 					288
	strgetc 				289
	strmch 					290
	strndup 				291
	strnmch 				292
	stropen 				293
	strrexp 				294
	OlgDrawAbbrevMenuB 			295
	OlgSizeAbbrevMenuB 			296
	OlgCreateAttrs 				297
	OlgDestroyAttrs 			298
	OlgSetStyle3D 				299
	OlgDrawCheckBox 			300
	OlgSizeCheckBox 			301
	OlgDrawTextLabel 			302
	OlgSizeTextLabel 			303
	OlgDrawPixmapLabel 			304
	OlgSizePixmapLabel 			305
	OlgDrawChiseledBox 			306
	OlgDrawBox 				307
	OlgDrawOblongButton 			308
	OlgSizeOblongButton 			309
	OlgDrawMenuMark 			310
	OlgDrawPushPin 				311
	OlgSizePushPin 				312
	OlgDrawRBox 				313
	OlgDrawFilledRBox 			314
	OlgDrawObject 				315
	OlgDrawRectButton 			316
	OlgSizeRectButton 			317
	OlgDrawScrollbar 			318
	OlgSizeScrollbarAnchor 			319
	OlgSizeScrollbarElevator 		320
	OlgUpdateScrollbar 			321
	_OlgGetDeviceData 			322
	_OlgGetBitmaps 				323
	OlSetGaugeValue 			324
	_OlGetScaleMap 				325
	OlMenuPopup 				326
	OlMenuPopdown 				327
	_OlFlatDrawLabel 			328
	_OlClass 				329
	_OlGetDontCareModifiers 		330
	_OlGetMultiObjectCount 			331
	_OlStringToVirtualKeyName 		332
	OlGrabVirtualKey 			333
	OlUngrabVirtualKey 			334
	_OlStringToOlKeyDef 			335
	_OlStringToOlBtnDef 			336
	_OlCanonicalKeysym 			337
	_OlLookupInputEvent 			338
	OlGetCurrentFocusWidget 		339
	_OlSetCurrentFocusWidget 		340
	OlSetInputFocus 			341
	OlAction 				342
	OlActivateWidget 			343
	OlAssociateWidget 			344
	OlUnassociateWidget 			345
	_OlRegisterShell 			346
	_OlUnregisterShell 			347
	_OlDestroyKeyboardHooks 		348
	_OlMakeAcceleratorText 			349
	OlQueryMnemonicDisplay 			350
	OlQueryAcceleratorDisplay 		351
	_OlAddMnemonic 				352
	_OlRemoveMnemonic 			353
	_OlFetchMnemonicOwner 			354
	_OlAddAccelerator 			355
	_OlRemoveAccelerator 			356
	_OlFetchAcceleratorOwner 		357
	_OlGetDefault 				358
	_OlGetFocusData 			359
	_OlGetVendorClassExtension 		360
	_OlGetVendorPartExtension 		361
	_OlLoadVendorShell 			362
	_OlSetDefault 				363
	_OlSetPinState 				364
	OlRegisterConverters 			365
	_OlAddOlDefineType 			366
	_OlGetClassExtension 			367
	_OlArrayDelete 				368
	__OlArrayFind 				369
	__OlArrayInsert 			370
	_OlListDelete 				371
	_OlListDestroy 				372
	_OlListFind 				373
	_OlListInsert 				374
	_OlListNew 				375


## list	all object files that make up the shared library
#objects
	libXoli.o
	Arrow.o
	Scrollbar.o
	BulletinBo.o
	ScrolledWi.o
	Nonexclusi.o
	CheckBox.o
	Button.o
	OblongButt.o
	RectButton.o
	Exclusives.o
	Stub.o
	List.o
	ListPane.o
	TextField.o
	AbbrevMenu.o
	AbbrevStac.o
	MenuButton.o
	ButtonStac.o
	Pushpin.o
	Menu.o
	Form.o
	ControlAre.o
	Caption.o
	PopupWindo.o
	FooterPane.o
	StaticText.o
	Notice.o
	Text.o
	TextPane.o
	TextPos.o
	Display.o
	SourceDsk.o
	SourceStr.o
	EventObj.o
	FCheckBox.o
	FExclusive.o
	FNonexclus.o
	Flat.o
	FlatCvt.o
	FlatExpand.o
	FlatState.o
	Copy.o
	OpenLook.o
	MaskArgs.o
	Manager.o
	Primitive.o
	OlCommon.o
	Traversal.o
	Action.o
	Accelerate.o
	OlStrings.o
	OlGetFont.o
	OlGetRes.o
	Vendor.o
	Virtual.o
	Error.o
	Slider.o
	Help.o
	BaseWindow.o
	Mag.o
	FMcomm.o
	WSMcomm.o
	Converters.o
	Extension.o
	array.o
	linkedList.o
	Olinitfix.o
	Packed.o
	TextVMap.o
	Dynamic.o
	OlCursors.o
	memutil.o
	buffutil.o
	strutil.o
	regexp.o
	textbuff.o
	TextEdit.o
	TextDisp.o
	TextEPos.o
	TextUtil.o
	TextWrap.o
	Margin.o
	OlgAbbrev.o
	OlgAttr.o
	OlgCheck.o
	OlgInit.o
	OlgLabel.o
	OlgLines.o
	OlgOblong.o
	OlgPushpin.o
	OlgRBox.o
	OlgRect.o
	OlgScrollb.o

## list	all imported functions and data	here
#init	libXoli.o
	_libXol_XBell				   XBell
	_libXol_XChangeProperty			   XChangeProperty
	_libXol_XChangeWindowAttributes		   XChangeWindowAttributes
	_libXol_XCheckWindowEvent		   XCheckWindowEvent
	_libXol_XClearArea			   XClearArea
	_libXol_XClearWindow			   XClearWindow
	_libXol_XClipBox			   XClipBox
	_libXol_XConvertSelection		   XConvertSelection
	_libXol_XCopyArea			   XCopyArea
	_libXol_XCopyPlane			   XCopyPlane
	_libXol_XCreateBitmapFromData		   XCreateBitmapFromData
	_libXol_XCreateFontCursor		   XCreateFontCursor
	_libXol_XCreateGC			   XCreateGC
	_libXol_XCreatePixmap			   XCreatePixmap
	_libXol_XCreatePixmapFromBitmapData	   XCreatePixmapFromBitmapData
	_libXol_XCreateRegion			   XCreateRegion
	_libXol_XDefaultDepth			   XDefaultDepth
	_libXol_XDefaultScreen			   XDefaultScreen
	_libXol_XDefaultScreenOfDisplay		   XDefaultScreenOfDisplay
	_libXol_XDefineCursor			   XDefineCursor
	_libXol_XDeleteProperty			   XDeleteProperty
	_libXol_XDisplayOfScreen		   XDisplayOfScreen
	_libXol_XDrawImageString		   XDrawImageString
	_libXol_XDrawLine			   XDrawLine
	_libXol_XDrawLines			   XDrawLines
	_libXol_XDrawRectangle			   XDrawRectangle
	_libXol_XDrawSegments			   XDrawSegments
	_libXol_XDrawString			   XDrawString
	_libXol_XFillPolygon			   XFillPolygon
	_libXol_XFillRectangle			   XFillRectangle
	_libXol_XFillRectangles			   XFillRectangles
	_libXol_XFlush				   XFlush
	_libXol_XFree				   XFree
	_libXol_XFreeFont			   XFreeFont
	_libXol_XFreeGC				   XFreeGC
	_libXol_XFreePixmap			   XFreePixmap
	_libXol_XGetDefault			   XGetDefault
	_libXol_XGetFontProperty		   XGetFontProperty
	_libXol_XGetImage			   XGetImage
	_libXol_XGetSelectionOwner		   XGetSelectionOwner
	_libXol_XGetWMHints			   XGetWMHints
	_libXol_XGetWindowAttributes		   XGetWindowAttributes
	_libXol_XGetWindowProperty		   XGetWindowProperty
	_libXol_XGrabPointer			   XGrabPointer
	_libXol_XGrabServer			   XGrabServer
	_libXol_XIfEvent			   XIfEvent
	_libXol_XInternAtom			   XInternAtom
	_libXol_XIntersectRegion		   XIntersectRegion
	_libXol_XKeysymToKeycode		   XKeysymToKeycode
	_libXol_XKeysymToString			   XKeysymToString
	_libXol_XLoadQueryFont			   XLoadQueryFont
	_libXol_XLookupString			   XLookupString
	_libXol_XMapRaised			   XMapRaised
	_libXol_XMapWindow			   XMapWindow
	_libXol_XMoveWindow			   XMoveWindow
	_libXol_XPutBackEvent			   XPutBackEvent
	_libXol_XPutImage			   XPutImage
	_libXol_XQueryPointer			   XQueryPointer
	_libXol_XRaiseWindow			   XRaiseWindow
	_libXol_XRectInRegion			   XRectInRegion
	_libXol_XRootWindow			   XRootWindow
	_libXol_XSelectInput			   XSelectInput
	_libXol_XSendEvent			   XSendEvent
	_libXol_XSetClipRectangles		   XSetClipRectangles
	_libXol_XSetFillStyle			   XSetFillStyle
	_libXol_XSetInputFocus			   XSetInputFocus
	_libXol_XSetNormalHints			   XSetNormalHints
	_libXol_XSetSelectionOwner		   XSetSelectionOwner
	_libXol_XSetStipple			   XSetStipple
	_libXol_XSetWMHints			   XSetWMHints
	_libXol_XSetWindowBackground		   XSetWindowBackground
	_libXol_XSetWindowBackgroundPixmap	   XSetWindowBackgroundPixmap
	_libXol_XSetWindowBorder		   XSetWindowBorder
	_libXol_XSetWindowBorderWidth		   XSetWindowBorderWidth
	_libXol_XStoreBuffer			   XStoreBuffer
	_libXol_XStringToKeysym			   XStringToKeysym
	_libXol_XSync				   XSync
	_libXol_XTextWidth			   XTextWidth
	_libXol_XTranslateCoordinates		   XTranslateCoordinates
	_libXol_XUngrabPointer			   XUngrabPointer
	_libXol_XUngrabServer			   XUngrabServer
	_libXol_XUnionRectWithRegion		   XUnionRectWithRegion
	_libXol_XUnmapWindow			   XUnmapWindow
	_libXol_XWarpPointer			   XWarpPointer
	_libXol_XrmQuarkToString		   XrmQuarkToString
	_libXol_XrmStringToQuark		   XrmStringToQuark
	_libXol_XtAddCallback			   XtAddCallback
	_libXol_XtAddCallbacks			   XtAddCallbacks
	_libXol_XtAddConverter			   XtAddConverter
	_libXol_XtAddEventHandler		   XtAddEventHandler
	_libXol_XtAddExposureToRegion		   XtAddExposureToRegion
	_libXol_XtAddGrab			   XtAddGrab
	_libXol_XtAddRawEventHandler		   XtAddRawEventHandler
	_libXol_XtAddTimeOut			   XtAddTimeOut
	_libXol_XtAppAddConverter		   XtAppAddConverter
	_libXol_XtAppCreateShell 		   XtAppCreateShell 
	_libXol_XtAppErrorMsg 	 		   XtAppErrorMsg  
	_libXol_XtAppWarningMsg  		   XtAppWarningMsg  
	_libXol_XtCallCallbacks			   XtCallCallbacks
	_libXol_XtCallConverter			   XtCallConverter
	_libXol_XtCallbackPopdown		   XtCallbackPopdown
	_libXol_XtCalloc			   XtCalloc
	_libXol_XtConfigureWidget		   XtConfigureWidget
	_libXol_XtConvert			   XtConvert
	_libXol_XtConvertCase  			   XtConvertCase
	_libXol_XtCreateApplicationContext	   XtCreateApplicationContext
	_libXol_XtCreateApplicationShell	   XtCreateApplicationShell
	_libXol_XtCreateManagedWidget		   XtCreateManagedWidget
	_libXol_XtCreatePopupShell		   XtCreatePopupShell
	_libXol_XtCreateWidget			   XtCreateWidget
	_libXol_XtCreateWindow			   XtCreateWindow
	_libXol__XtDefaultAppContext		   _XtDefaultAppContext
	_libXol_XtDisplay			   XtDisplay
	_libXol_XtDisplayToApplicationContext	   XtDisplayToApplicationContext
	_libXol_XtDestroyGC			   XtDestroyGC
	_libXol_XtDestroyWidget			   XtDestroyWidget
	_libXol_XtDisplayOfObject		   XtDisplayOfObject
	_libXol_XtError				   XtError
	_libXol_XtErrorMsg			   XtErrorMsg
	_libXol_XtFree				   XtFree
	_libXol__XtFreeEventTable		   _XtFreeEventTable
	_libXol_XtGetApplicationResources	   XtGetApplicationResources
	_libXol_XtGetGC				   XtGetGC
	_libXol_XtGetKeysymTable		   XtGetKeysymTable
	_libXol__XtGetProcessContext		   _XtGetProcessContext
	_libXol_XtGetSelectionValue		   XtGetSelectionValue
	_libXol_XtGetSubresources		   XtGetSubresources
	_libXol_XtGetSubvalues			   XtGetSubvalues
	_libXol_XtGetValues			   XtGetValues
	_libXol_XtGrabKey     			   XtGrabKey     
	_libXol_XtGrabKeyboard			   XtGrabKeyboard
	_libXol_XtHasCallbacks			   XtHasCallbacks
	_libXol_XtInitialize			   XtInitialize
	_libXol_XtIsRealized			   XtIsRealized
	_libXol_XtIsManaged			   XtIsManaged
	_libXol_XtIsSensitive			   XtIsSensitive
	_libXol_XtIsSubclass			   XtIsSubclass
	_libXol__XtIsSubclassOf			   _XtIsSubclassOf
	_libXol_XtMakeGeometryRequest		   XtMakeGeometryRequest
	_libXol_XtMakeResizeRequest		   XtMakeResizeRequest
	_libXol_XtMalloc			   XtMalloc
	_libXol_XtManageChild			   XtManageChild
	_libXol_XtManageChildren		   XtManageChildren
	_libXol_XtMergeArgLists			   XtMergeArgLists
	_libXol_XtMoveWidget			   XtMoveWidget
	_libXol_XtName      			   XtName      
	_libXol_XtOpenDisplay         		   XtOpenDisplay         
	_libXol_XtOverrideTranslations		   XtOverrideTranslations
	_libXol_XtOwnSelection			   XtOwnSelection
	_libXol_XtParseTranslationTable		   XtParseTranslationTable
	_libXol_XtPopdown			   XtPopdown
	_libXol_XtPopup				   XtPopup
	_libXol_XtQueryGeometry			   XtQueryGeometry
	_libXol_XtRealizeWidget			   XtRealizeWidget
	_libXol_XtRealloc			   XtRealloc
	_libXol_XtReleaseGC			   XtReleaseGC
	_libXol_XtRemoveAllCallbacks		   XtRemoveAllCallbacks
	_libXol_XtRemoveCallback		   XtRemoveCallback
	_libXol_XtRemoveEventHandler		   XtRemoveEventHandler
	_libXol_XtRemoveGrab			   XtRemoveGrab
	_libXol_XtRemoveRawEventHandler		   XtRemoveRawEventHandler
	_libXol_XtRemoveTimeOut			   XtRemoveTimeOut
	_libXol_XtResizeWidget			   XtResizeWidget
	_libXol_XtScreen			   XtScreen
	_libXol_XtScreenOfObject		   XtScreenOfObject
	_libXol_XtSetKeyboardFocus   		   XtSetKeyboardFocus
	_libXol_XtSetMappedWhenManaged		   XtSetMappedWhenManaged
	_libXol_XtSetSensitive			   XtSetSensitive
	_libXol_XtSetSubvalues			   XtSetSubvalues
	_libXol_XtSetTypeConverter 		   XtSetTypeConverter
	_libXol_XtSetValues			   XtSetValues
	_libXol__XtSortPerDisplayList		   _XtSortPerDisplayList
	_libXol_XtToolkitInitialize	   	   XtToolkitInitialize       
	_libXol_XtUngrabKey       		   XtUngrabKey       
	_libXol_XtUngrabKeyboard  		   XtUngrabKeyboard  
	_libXol_XtUnmanageChildren		   XtUnmanageChildren
	_libXol_XtWindow			   XtWindow
	_libXol_XtWindowOfObject		   XtWindowOfObject
	_libXol_XtWindowToWidget		   XtWindowToWidget
	_libXol__XtInherit			   _XtInherit

	_libXol_abort				abort
	_libXol_access				   access
	_libXol_bcopy				   bcopy
	_libXol_close				   close
	_libXol_ctermid				ctermid
	_libXol_cuserid				cuserid
	_libXol_exit				   exit
	_libXol_fclose				   fclose
	_libXol_fflush				fflush
	_libXol_fgetc				fgetc
	_libXol_fgets				   fgets
	_libXol_fopen				   fopen
	_libXol_fprintf				   fprintf
	_libXol_fputc				fputc
	_libXol_fputs				fputs
	_libXol_fread				   fread
	_libXol_free				   free
	_libXol_fscanf				   fscanf
	_libXol_fseek				   fseek
	_libXol_ftell				   ftell
	_libXol_fwrite				   fwrite
	_libXol_getuid				   getuid
	_libXol_getgid				   getgid
	_libXol_gets				gets
	_libXol_getw				getw
	_libXol_malloc				   malloc
	_libXol_memccpy				memccpy
	_libXol_memchr				memchr
	_libXol_memcmp				memcmp
	_libXol_memcpy				memcpy
	_libXol_memset				memset
	_libXol_open				   open
	_libXol_printf				   printf
	_libXol_puts				puts
	_libXol_putw				putw
	_libXol_read				   read
	_libXol_realloc				   realloc
	_libXol_rewind				rewind
	_libXol_scanf				scanf
	_libXol_setbuf				setbuf
	_libXol_setvbuf				setvbuf
	_libXol_sprintf				   sprintf
	_libXol_sscanf				   sscanf
	_libXol_strcat				   strcat
	_libXol_strchr				   strchr
	_libXol_strcmp				   strcmp
	_libXol_strcpy				   strcpy
	_libXol_strcspn				strcspn
	_libXol_strdup				strdup
	_libXol_strlen				   strlen
	_libXol_strncat				strncat
	_libXol_strncmp				strncmp
	_libXol_strncpy				   strncpy
	_libXol_strpbrk				strpbrk
	_libXol_strrchr				   strrchr
	_libXol_strspn				strspn
	_libXol_strtok				   strtok
	_libXol_strtod				   strtod
	_libXol_strtol				   strtol
	_libXol_system				system
	_libXol_tmpfile				tmpfile
	_libXol_tempnam				   tempnam
	_libXol_time				   time
	_libXol_times				   times
	_libXol_tmpnam				tmpnam
	_libXol_tolower				   tolower
	_libXol_toupper				   toupper
	_libXol_tzset				tzset
	_libXol_ungetc				ungetc
	_libXol_vfprintf			   vfprintf
	_libXol_vsprintf			   vsprintf
## put imported	data here
	_libXol__XtInheritTranslations		   _XtInheritTranslations
	_libXol_applicationShellClassRec   	   applicationShellClassRec
	_libXol_applicationShellWidgetClass	   applicationShellWidgetClass
	_libXol_compositeClassRec		   compositeClassRec
	_libXol_compositeWidgetClass		   compositeWidgetClass
	_libXol_constraintClassRec		   constraintClassRec
	_libXol_daylight			   daylight
	_libXol_errno				   errno
	_libXol__iob				   _iob
	_libXol__ctype				   _ctype
	_libXol_overrideShellWidgetClass	   overrideShellWidgetClass
	_libXol_rectObjClassRec			   rectObjClassRec
	_libXol_shellClassRec			   shellClassRec
	_libXol_shellWidgetClass		   shellWidgetClass
	_libXol_timezone			   timezone
	_libXol_topLevelShellClassRec		   topLevelShellClassRec
	_libXol_topLevelShellWidgetClass	   topLevelShellWidgetClass
	_libXol_transientShellClassRec		   transientShellClassRec
	_libXol_transientShellWidgetClass	   transientShellWidgetClass
	_libXol_widgetClass			   widgetClass
	_libXol_widgetClassRec			   widgetClassRec
	_libXol_wmShellClassRec			   wmShellClassRec
	_libXol_wmShellWidgetClass		   wmShellWidgetClass
	_libXol__XtperDisplayList		   _XtperDisplayList

#objects noload
	-lc_s
	-lnsl_s
	-lX11_s
#hide linker *

## put all externally available	variables here
#export	linker
	BANG
	WM_CHANGE_STATE
	WM_DECORATION_HINTS
	WM_DELETE_WINDOW
	WM_DISMISS
	WM_ICON_SIZE
	WM_PROTOCOLS
	WM_SAVE_YOURSELF
	WM_STATE
	WM_TAKE_FOCUS
	WM_WINDOW_MOVED
	_OL_COPY
	_OL_CUT
	_OL_DECOR_ADD
	_OL_DECOR_CLOSE
	_OL_DECOR_DEL
	_OL_DECOR_HEADER
	_OL_DECOR_PIN
	_OL_DECOR_RESIZE
	_OL_FM_QUEUE
	_OL_FM_REPLY
	_OL_HELP_KEY
	_OL_MENU_FULL
	_OL_MENU_LIMITED
	_OL_NONE
	_OL_PIN_STATE
	_OL_WIN_ATTR
	_OL_WIN_BUSY
	_OL_WIN_COLORS
	_OL_WSM_QUEUE
	_OL_WSM_REPLY
	_OL_WT_BASE
	_OL_WT_CMD
	_OL_WT_HELP
	_OL_WT_NOTICE
	_OL_WT_OTHER
	_OL_WT_PROP
	abbrevMenuButtonClassRec
	abbrevMenuButtonWidgetClass
	abbrevStackClassRec
	abbrevStackWidgetClass
	arrowClassRec
	arrowWidgetClass
	baseWindowShellClassRec
	baseWindowShellWidgetClass
	bulletinBoardClassRec
	bulletinBoardWidgetClass
	buttonClassRec
	buttonGadgetClass
	buttonGadgetClassRec
	buttonStackClassRec
	buttonStackWidgetClass
	buttonStackGadgetClassRec
	buttonStackGadgetClass
	buttonWidgetClass
	captionClassRec
	captionWidgetClass
	checkBoxClassRec
	checkBoxWidgetClass
	controlAreaWidgetClass
	controlClassRec
	eventObjClass
	eventObjClassRec
	exclusivesClassRec
	exclusivesWidgetClass
	flatCheckBoxClassRec
	flatCheckBoxWidgetClass
	flatExclusivesClassRec
	flatExclusivesWidgetClass
	flatNonexclusivesClassRec
	flatNonexclusivesWidgetClass
	flatClassRec
	flatWidgetClass
	footerPanelClassRec
	footerPanelWidgetClass
	formClassRec
	formWidgetClass
	helpClassRec
	helpWidgetClass
	listClassRec
	listPaneClassRec
	listPaneWidgetClass
	listWidgetClass
	magClassRec
	magWidgetClass
	managerClassRec
	managerWidgetClass
	menuButtonClassRec
	menuButtonWidgetClass
	menuButtonGadgetClassRec
	menuButtonGadgetClass
	menuShellClassRec
	menuShellWidgetClass
	nonexclusivesClassRec
	nonexclusivesWidgetClass
	noticeShellClassRec
	noticeShellWidgetClass
	oblongButtonClassRec
	oblongButtonWidgetClass
	oblongButtonGadgetClassRec
	oblongButtonGadgetClass
	popupWindowShellWidgetClass
	popupWindowShellClassRec
	primitiveClassRec
	primitiveWidgetClass
	pushpinClassRec
	pushpinWidgetClass
	rectButtonClassRec
	rectButtonWidgetClass
	scrollbarClassRec
	scrollbarWidgetClass
	scrolledWindowClassRec
	scrolledWindowWidgetClass
	scrollingListWidgetClass
	sliderClassRec
	sliderWidgetClass
	staticTextWidgetClass
	statictextClassRec
	stubClassRec
	stubWidgetClass
	textClassRec
	textFieldClassRec
	textFieldWidgetClass
	textPaneActionsTable
	textPaneClassRec
	textPaneWidgetClass
	textWidgetClass
	toplevelDisplay
	_OlApplicationName
	XtCAlignCaptions
	XtCAlignHorizontal
	XtCAlignVertical
	XtCAlignment
	XtCApplAddItem
	XtCApplDeleteItem
	XtCApplEditClose
	XtCApplEditOpen
	XtCApplTouchItem
	XtCApplUpdateView
	XtCApplViewItem
	XtCApply
	XtCAutoPopup
	XtCBeep
	XtCBlinkRate
	XtCBorderVisible
	XtCBusy
	XtCButtonType
	XtCCallbackProc
	XtCCaption
	XtCCaptionWidth
	XtCCenter
	XtCCenterLine
	XtCCharsVisible
	XtCClientData
	XtCComputeGeometries
	XtCContainerType
	XtCControlArea
	XtCCornerColor
	XtCCurrentPage
	XtCDampingFactor
	XtCData
	XtCDefault
	XtCDefaultData
	XtCDestroy
	XtCDim
	XtCDirection
	XtCDismiss
	XtCDisplayPosition
	XtCEditMode
	XtCEmanateWidget
	XtCExclusives
	XtCExpose
	XtCFocusWidget
	XtCFontColor
	XtCFontName
	XtCFooterPanel
	XtCForceHorizontalSB
	XtCForceVerticalSB
	XtCGetValuesHook
	XtCGranularity
	XtCGravity
	XtCGrow
	XtCHAutoScroll
	XtCHInitialDelay
	XtCHMenuPane
	XtCHPad
	XtCHRepeatRate
	XtCHScrollbar
	XtCHSliderMoved
	XtCHStepSize
	XtCHelpInfo
	XtCHorizontalSB
	XtCIconBorder
	XtCIconGravity
	XtCIconParking
	XtCInitialDelay
	XtCInitialX
	XtCInitialY
	XtCInitialize
	XtCInitializeHook
	XtCInputFocusColor
	XtCItemCount
	XtCItemFields
	XtCItemGravity
	XtCItemHeight
	XtCItemMaxHeight
	XtCItemMaxWidth
	XtCItemMinHeight
	XtCItemMinWidth
	XtCItemState
	XtCItemWidth
	XtCItems
	XtCItemsTouched
	XtCItemsVisible
	XtCLabelImage
	XtCLabelJustify
	XtCLabelPixmap
	XtCLabelTile
	XtCLabelType
	XtCLayout
	XtCLayoutHeight
	XtCLayoutType
	XtCLayoutWidth
	XtCLineSpace
	XtCLinesVisible
	XtCListType
	XtCLowerControlArea
	XtCManaged
	XtCMaximumSize
	XtCMeasure
	XtCMenu
	XtCMenuAttachQuery
	XtCMenuAugment
	XtCMenuMark
	XtCMenuName
	XtCMenuPane
	XtCMenuPositioner
	XtCMouseX
	XtCMouseY
	XtCMultiClickTimeout
	XtCNoneSet
	XtCNumItemFields
	XtCNumItems
	XtCPackedWidget
	XtCPackedWidgetList
	XtCPaneBackground
	XtCPaneForeground
	XtCPaneName
	XtCParentReset
	XtCPointerWarping
	XtCPostModifyNotification
	XtCPostSelect
	XtCPreview
	XtCPreviewWidget
	XtCPropertyChange
	XtCProportionLength
	XtCPushpin
	XtCPushpinDefault
	XtCPushpinWidget
	XtCQueryGeometry
	XtCRealize
	XtCRecomputeHeight
	XtCRecomputeSize
	XtCRecomputeWidth
	XtCReferenceStub
	XtCReferenceWidget
	XtCRefresh
	XtCRepeatRate
	XtCReset
	XtCResetDefault
	XtCResetFactory
	XtCResetSet
	XtCRevertButton
	XtCSameHeight
	XtCSameSize
	XtCSameWidth
	XtCScale
	XtCScroll
	XtCSelectDoesPreview
	XtCSelectable
	XtCSelectionColor
	XtCSet
	XtCSetDefaults
	XtCSetValues
	XtCSetValuesAlmost
	XtCSetValuesHook
	XtCShellBehavior
	XtCShowPage
	XtCSliderMax
	XtCSliderMin
	XtCSliderMoved
	XtCSliderValue
	XtCSource
	XtCSourceType
	XtCStrip
	XtCTabTable
	XtCTextArea
	XtCTextClearBuffer
	XtCTextCopyBuffer
	XtCTextEditWidget
	XtCTextField
	XtCTextGetInsertPoint
	XtCTextGetLastPos
	XtCTextInsert
	XtCTextReadSubStr
	XtCTextRedraw
	XtCTextReplace
	XtCTextSetInsertPoint
	XtCTextSetSource
	XtCTextUpdate
	XtCToggleState
	XtCToken
	XtCTouchItems
	XtCTraversalOn
	XtCTrigger
	XtCUnitType
	XtCUpdateView
	XtCUpperControlArea
	XtCUserData
	XtCVAutoScroll
	XtCVInitialDelay
	XtCVMenuPane
	XtCVPad
	XtCVRepeatRate
	XtCVSB
	XtCVScrollbar
	XtCVSliderMoved
	XtCVStepSize
	XtCVerification
	XtCVerify
	XtCVerticalSB
	XtCVerticalSBWidget
	XtCViewHeight
	XtCViewWidth
	XtCVisibleChildren
	XtCWindowBackground
	XtCWindowForeground
	XtCWindowLayering
	XtCWrap
	XtCWrapBreak
	XtCWrapForm
	XtCWrapMode
	XtCXAddWidth
	XtCXAttachOffset
	XtCXAttachRight
	XtCXOffset
	XtCXRefName
	XtCXRefWidget
	XtCXResizable
	XtCXVaryOffset
	XtCYAddHeight
	XtCYAttachBottom
	XtCYAttachOffset
	XtCYOffset
	XtCYRefName
	XtCYRefWidget
	XtCYResizable
	XtCYVaryOffset
	XtNadjustBtn
	XtNalignCaptions
	XtNalignHorizontal
	XtNalignVertical
	XtNalignment
	XtNapplAddItem
	XtNapplDeleteItem
	XtNapplEditClose
	XtNapplEditOpen
	XtNapplTouchItem
	XtNapplUpdateView
	XtNapplViewItem
	XtNapply
	XtNautoPopup
	XtNbeep
	XtNblinkRate
	XtNborderVisible
	XtNbottomMargin
	XtNbtnDown
	XtNbtnMotion
	XtNbtnUp
	XtNbusy
	XtNbuttonType
	XtNbuttons
	XtNcancelKey
	XtNcaption
	XtNcaptionWidth
	XtNcenter
	XtNcenterLine
	XtNcharsVisible
	XtNclientData
	XtNcomputeGeometries
	XtNconstrainBtn
	XtNcontainerType
	XtNcontrolArea
	XtNcopyKey
	XtNcornerColor
	XtNcurrentPage
	XtNcursorPosition
	XtNcutKey
	XtNdampingFactor
	XtNdata
	XtNdefault
	XtNdefaultData
	XtNdefaultProc
	XtNdestroy
	XtNdim
	XtNdirectManipulation
	XtNdirection
	XtNdiskSrc
	XtNdismiss
	XtNdisplayPosition
	XtNdragCursor
	XtNduplicateBtn
	XtNeditMode
	XtNemanateWidget
	XtNexclusives
	XtNexecute
	XtNexpose
	XtNfocusWidget
	XtNfontColor
	XtNfontName
	XtNfooterPanel
	XtNforceHorizontalSB
	XtNforceVerticalSB
	XtNgetValuesHook
	XtNgranularity
	XtNgravity
	XtNgrow
	XtNhAutoScroll
	XtNhInitialDelay
	XtNhMenuPane
	XtNhPad
	XtNhRepeatRate
	XtNhScrollbar
	XtNhSliderMoved
	XtNhStepSize
	XtNhelpInfo
	XtNhelpKey
	XtNhorizontalSB
	XtNiconBorder
	XtNiconGravity
	XtNiconParking
	XtNinitialDelay
	XtNinitialX
	XtNinitialY
	XtNinitialize
	XtNinitializeHook
	XtNinputFocusColor
	XtNitemCount
	XtNitemFields
	XtNitemGravity
	XtNitemHeight
	XtNitemMaxHeight
	XtNitemMaxWidth
	XtNitemMinHeight
	XtNitemMinWidth
	XtNitemState
	XtNitemWidth
	XtNitems
	XtNitemsTouched
	XtNitemsVisible
	XtNkeys
	XtNlabelImage
	XtNlabelJustify
	XtNlabelPixmap
	XtNlabelTile
	XtNlabelType
	XtNlayout
	XtNlayoutHeight
	XtNlayoutType
	XtNlayoutWidth
	XtNleaveVerification
	XtNleftMargin
	XtNlineSpace
	XtNlinesVisible
	XtNlowerControlArea
	XtNmanaged
	XtNmargin
	XtNmaximumSize
	XtNmeasure
	XtNmenu
	XtNmenuAttachQuery
	XtNmenuAugment
	XtNmenuBtn
	XtNmenuDefaultBtn
	XtNmenuMark
	XtNmenuName
	XtNmenuPane
	XtNmenuPositioner
	XtNmodifyVerification
	XtNmotionVerification
	XtNmouseX
	XtNmouseY
	XtNmultiClickTimeout
	XtNnextFieldKey
	XtNnoneSet
	XtNnumItemFields
	XtNnumItems
	XtNpackedWidget
	XtNpackedWidgetList
	XtNpanBtn
	XtNpaneBackground
	XtNpaneForeground
	XtNpaneName
	XtNparentReset
	XtNpasteKey
	XtNpointerWarping
	XtNposition
	XtNpostModifyNotification
	XtNpostSelect
	XtNprevFieldKey
	XtNpreview
	XtNpreviewWidget
	XtNpropertiesKey
	XtNpropertyChange
	XtNproportionLength
	XtNpushpin
	XtNpushpinDefault
	XtNpushpinIn
	XtNpushpinOut
	XtNpushpinWidget
	XtNqueryGeometry
	XtNrealize
	XtNrecomputeHeight
	XtNrecomputeSize
	XtNrecomputeWidth
	XtNreferenceStub
	XtNreferenceWidget
	XtNrefresh
	XtNrepeatRate
	XtNreset
	XtNresetDefault
	XtNresetFactory
	XtNresetSet
	XtNrevertButton
	XtNrightMargin
	XtNsameHeight
	XtNsameSize
	XtNsameWidth
	XtNscale
	XtNscroll
	XtNselect
	XtNselectBtn
	XtNselectDoesPreview
	XtNselectEnd
	XtNselectProc
	XtNselectStart
	XtNselectable
	XtNselectionColor
	XtNset
	XtNsetDefaults
	XtNsetValues
	XtNsetValuesAlmost
	XtNsetValuesHook
	XtNshellBehavior
	XtNshowPage
	XtNsliderMax
	XtNsliderMin
	XtNsliderMoved
	XtNsliderValue
	XtNsource
	XtNsourceType
	XtNstateChange
	XtNstopKey
	XtNstringSrc
	XtNstrip
	XtNtabTable
	XtNtextArea
	XtNtextClearBuffer
	XtNtextCopyBuffer
	XtNtextEditWidget
	XtNtextField
	XtNtextGetInsertPoint
	XtNtextGetLastPos
	XtNtextInsert
	XtNtextReadSubStr
	XtNtextRedraw
	XtNtextReplace
	XtNtextSetInsertPoint
	XtNtextSetSource
	XtNtextUpdate
	XtNtoggleState
	XtNtopMargin
	XtNtouchItems
	XtNtraversalOn
	XtNtrigger
	XtNundoKey
	XtNunitType
	XtNunselect
	XtNunselectProc
	XtNupdateView
	XtNupperControlArea
	XtNuserAddItems
	XtNuserData
	XtNuserDeleteItems
	XtNuserMakeCurrent
	XtNvAutoScroll
	XtNvInitialDelay
	XtNvMenuPane
	XtNvPad
	XtNvRepeatRate
	XtNvScrollbar
	XtNvSliderMoved
	XtNvStepSize
	XtNverification
	XtNverify
	XtNverticalSB
	XtNverticalSBWidget
	XtNviewHeight
	XtNviewWidth
	XtNvisibleChildren
	XtNvsb
	XtNwindowBackground
	XtNwindowForeground
	XtNwindowLayering
	XtNwrap
	XtNwrapBreak
	XtNwrapForm
	XtNwrapMode
	XtNxAddWidth
	XtNxAttachOffset
	XtNxAttachRight
	XtNxOffset
	XtNxRefName
	XtNxRefWidget
	XtNxResizable
	XtNxVaryOffset
	XtNyAddHeight
	XtNyAttachBottom
	XtNyAttachOffset
	XtNyOffset
	XtNyRefName
	XtNyRefWidget
	XtNyResizable
	XtNyVaryOffset
	XtRCallbackProc
	XtRGravity
	XtRGrow
	XtRIndirectFontStruct
	XtRIndirectPixel
	XtRLong
	XtROlBitMask
	XtROlDefine
	XtROlEditMode
	XtROlSourceType
	XtROlWrapMode
	XtRScroll
	XtRSourceType
	XtRWrap
	XtRWrapBreak
	XtRWrapForm
	XA_TIMESTAMP
	OL_PASTE_MSG
	XA_TARGETS
	XA_DELETE
	textEditClassRec
	textEditWidgetClass
	XtNdragCBType
	XtNendBoxes	
	XtNmenuButton     
	XtNmenuType        
	XtNmaxLabel        
	XtNminLabel        
	XtNresizeCorners
	XtNspan	
	XtNstopPosition
	XtNticks	
	XtNtickUnit	
	XtNwindowHeader    
	XtNwinType         
	XtCDragCBType      
	XtCEndBoxes      
	XtCMenuButton     
	XtCMenuType        
	XtCMaxLabel        
	XtCMinLabel        
	XtCResizeCorners
	XtCSpan	
	XtCStopPosition
	XtCTicks	
	XtCTickUnit	
	XtCWindowHeader    
	XtCWinType         
	XtN3d
	XtC3d
	_olgIs3d
