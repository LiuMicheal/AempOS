
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               load.inc
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


BaseOfLoader		equ	 09000h	; LOADER.BIN 被加载到的位置 ----  段地址
OffsetOfLoader		equ	  0100h	; LOADER.BIN 被加载到的位置 ---- 偏移地址

BaseOfLoaderPhyAddr	equ	BaseOfLoader * 10h	; LOADER.BIN 被加载到的位置 ---- 物理地址 (= BaseOfLoader * 10h)

;modified by xw, 18/6/12
; BaseOfKernelFile	equ	 08000h	; KERNEL.BIN 被加载到的位置 ----  段地址

;to support the kernel.bin larger than 64KB, copied from Orange's_9_h, mingxuan, 2019-1-25
;modified by mingxuan 2019-1-25
;BaseOfKernelFile	equ	 07000h	; 07000h表示kernel.bin的大小为(2*64)KB ;deleted by mingxuan 2019-1-25
BaseOfKernelFile	equ	 06000h	; 06000h表示kernel.bin的大小为(3*64)KB ;modified by mingxuan 2019-1-25
;BaseOfKernelFile	equ	 05000h	; 06000h表示kernel.bin的大小为(4*64)KB ;modified by mingxuan 2019-3-29
OffsetOfKernelFile	equ	     0h	; KERNEL.BIN 被加载到的位置 ---- 偏移地址

BaseOfEchoFile	equ		  07E0h	; ECHO.BIN 被加载到的位置 ----  段地址
OffsetOfEchoFile	equ	     0h	; ECHO.BIN 被加载到的位置 ---- 偏移地址

;08E0h这个地址是我随便选的,用于加载cpu1的init1
;added by mingxuan 2019-3-14
BaseOfInit1File	equ		  08E0h	; INIT1.BIN 被加载到的位置 ----  段地址
OffsetOfInit1File	equ	     0h	; INIT1.BIN 被加载到的位置 ---- 偏移地址

;08E0h这个地址是我随便选的,用于加载cpu2的init2
;added by mingxuan 2019-3-14
BaseOfInit2File	equ		  09E0h	; INIT2.BIN 被加载到的位置 ----  段地址
OffsetOfInit2File	equ	     0h	; INIT2.BIN 被加载到的位置 ---- 偏移地址

;08E0h这个地址是我随便选的,用于加载cpu3的init1
;added by mingxuan 2019-3-14
BaseOfInit3File	equ		  0AE0h	; INIT2.BIN 被加载到的位置 ----  段地址
OffsetOfInit3File	equ	     0h	; INIT2.BIN 被加载到的位置 ---- 偏移地址


BaseOfKernelFilePhyAddr	equ	BaseOfKernelFile * 10h
;BaseOfEchoFilePhyAddr	equ	BaseOfKernelFile * 10h	;deleted by mingxuan 2019-3-7
BaseOfEchoFilePhyAddr	equ	BaseOfEchoFile * 10h	;modified by mingxuan 2019-3-7
BaseOfInit1FilePhyAddr  equ	BaseOfInit1File * 10h	;added by mingxuan 2019-3-7
BaseOfInit2FilePhyAddr  equ	BaseOfInit2File * 10h	;added by mingxuan 2019-3-14
BaseOfInit3FilePhyAddr  equ	BaseOfInit3File * 10h	;added by mingxuan 2019-3-14


KernelEntryPointPhyAddr	equ	0C0030400h	; 注意：1、必须与 MAKEFILE 中参数 -Ttext 的值相等!! edit by visual 2016.5.10
						; 2、这是个地址而非仅仅是个偏移，如果 -Ttext 的值为 0x400400，则它的值也应该是 0x400400。

PageDirBase		equ	200000h	; 页目录开始地址:		2M
PageTblBase		equ	201000h	; 页表开始地址:			2M + 4K

PageTblNumAddr		equ 500h;页表数量放在这个位置	delete by visual 2016.4.28

FMIBuff			equ	007ff000h

;added by mingxuan 2018-12-20
BaseOfEntryother	equ	07000h	;added by mingxuan 2018-12-20
StackOfAP1		equ	06000h	;added by mingxuan 2018-12-20
