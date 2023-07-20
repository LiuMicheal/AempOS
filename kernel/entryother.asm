; ====================================================================================
;			entryother.asm			//add by mingxuan 2018-10-23
; ====================================================================================	

%include	"pm.inc"
%include	"load.inc"	added by mingxuan 2018-12-20


	jmp	ap_start

[section .s16]
[bits 16]
; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------
;                                                段基址            段界限      属性
LABEL_GDT:				Descriptor             0,               0, 0							; 空描述符
LABEL_DESC_FLAT_C:		Descriptor             0,         0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K	; 0 ~ 4G
LABEL_DESC_FLAT_RW:		Descriptor             0,      	  0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K	; 0 ~ 4G
LABEL_DESC_VIDEO:		Descriptor	 	 0B8000h,          0ffffh, DA_DRW						; 显存首地址
;LABEL_DESC_CPU:			Descriptor    	  61980h,           00bbh, DA_DRW | DA_DPL0				; CPU段地址
; GDT ------------------------------------------------------------------------------------------------------------------------------------------------------------

GdtLen		equ	$ - LABEL_GDT
GdtPtr		dw	GdtLen - 1			; 段界限
		dd	BaseOfEntryother + LABEL_GDT	; 基地址

; GDT 选择子 ---------------------------------------------------------------------------------- 
SelectorFlatC		equ	LABEL_DESC_FLAT_C	- LABEL_GDT
SelectorFlatRW		equ	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo		equ	LABEL_DESC_VIDEO	- LABEL_GDT
;SelectorCPU		equ	LABEL_DESC_CPU		- LABEL_GDT
; GDT 选择子 ----------------------------------------------------------------------------------


ap_start:

	mov	ax, cs
	mov	ds, ax
	mov	es, ax
	mov	ss, ax

	; 加载 GDTR
	lgdt	[GdtPtr]

	; 关中断
	cli

	; 打开地址线A20
	in	al, 92h
	or	al, 00000010b
	out	92h, al

	; 准备切换到保护模式
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

	; 真正进入保护模式
	jmp	dword SelectorFlatC :(BaseOfEntryother + LABEL_SEG_CODE32)


[section .s32]				; 32 位代码段. 由实模式跳入.
[bits	32]

LABEL_SEG_CODE32:
	mov	ax, SelectorVideo
	mov	gs, ax			; 视频段选择子(目的)
	
	;mov	ax, SelectorCPU		; added by mingxuan 2018-12-24
	;mov	fs, ax			; CPU通过fs段获取自身ID和当前进程

	mov	ax, SelectorFlatRW
	mov	ds, ax
	mov	es, ax
	mov	ss, ax

	;打印字母P，标识AP成功进入保护模式
	mov	edi, (80 * 3 + 60) * 2	; 屏幕第 3 行, 第 60 列。
	mov	ah, 0Ch			; 0000: 黑底    1100: 红字
	mov	al, 'P'
	mov	[gs:edi], ax

	;xor	eax, eax
	;mov	ax, 0x20
	;ltr	ax

	;暂时设置页映射——直接和BSP共用一套页机制
	mov	eax, [BaseOfEntryother - 12]
	mov	cr3, eax
	
	;打开页机制开关
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax

	;设置堆栈,该堆栈是由bsp通过kmalloc得到
	mov	esp, [BaseOfEntryother - 4]
	
	;在(BaseOfEntryother-4)的位置找到mpenter函数的入口地址	
	mov	eax, [BaseOfEntryother - 8]
	
	;调用内核的mpenter函数
	call	eax
	;call	SelectorFlatC : 0x31392	   ;另一种调用内核C函数的方式

	; 到此停止
	jmp	$




