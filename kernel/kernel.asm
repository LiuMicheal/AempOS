
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               kernel.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


%include "sconst.inc"

; 导入函数
extern	cstart
extern	kernel_main
extern	exception_handler
extern	spurious_irq
extern	clock_handler
extern	disp_str
extern	delay
extern	irq_table
extern	page_fault_handler
extern	divide_error_handler	;added by xw, 18/12/22
extern	disp_int
;extern  schedule		;deleted by mingxuan 2019-1-20
extern  switch_pde
;extern	new_schedule	;added by mingxuan 2019-1-20 ;deleted by mingxuan 2019-3-5
extern  schedule		;modified by mingxuan 2019-3-5
extern	renew_tss		;added by mingxuan 2019-1-22

extern	do_down			;added by mingxuan 2019-3-29
extern  do_up			;added by mingxuan 2019-3-29

; 导入全局变量
extern	gdt_ptr
extern	idt_ptr
extern	p_proc_current
extern	tss
extern	disp_pos
extern	k_reenter
extern	sys_call_table
extern 	cr3_ready			;add by visual 2016.4.5
extern  p_proc_current
extern	p_proc_next			;added by xw, 18/4/26
extern	kernel_initial		;added by xw, 18/6/10

;extern  proc				;added by mingxuan 2019-1-11


bits 32

[SECTION .data]
clock_int_msg		db	"^", 0

[SECTION .bss]
StackSpace		resb	2 * 1024
StackTop:		; used only as irq-stack in minios. added by xw

; added by xw, 18/6/15
KernelStackSpace	resb	2 * 1024
KernelStackTop:	; used as stack of kernel itself
; ~xw

[section .text]	; 代码在此

global _start	; 导出 _start

;global restart
global restart_initial	;Added by xw, 18/4/21
global restart_restore	;Added by xw, 18/4/21
;global save_context
global sched			;Added by xw, 18/4/21
global sys_call
global read_cr2   ;//add by visual 2016.5.9
global refresh_page_cache ; // add by visual 2016.5.12
global halt  			;added by xw, 18/6/11
global get_arg			;added by xw, 18/6/18
global read_cr3    ;added by mingxuan 2018-10-17

global down_failed ;added by mingxuan 2019-4-1
global up_wakeup   ;added by mingxuan 2019-4-1

global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15


_start:
	; 此时内存看上去是这样的（更详细的内存情况在 LOADER.ASM 中有说明）：
	;              ┃                                    ┃
	;              ┃                 ...                ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■Page  Tables■■■■■■┃
	;              ┃■■■■■(大小由LOADER决定)■■■■┃ PageTblBase
	;    00101000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■Page Directory Table■■■■┃ PageDirBase = 1M
	;    00100000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃□□□□ Hardware  Reserved □□□□┃ B8000h ← gs
	;       9FC00h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■LOADER.BIN■■■■■■┃ somewhere in LOADER ← esp
	;       90000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■KERNEL.BIN■■■■■■┃
	;       80000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃■■■■■■■■KERNEL■■■■■■■┃ 30400h ← KERNEL 入口 (KernelEntryPointPhyAddr)
	;       30000h ┣━━━━━━━━━━━━━━━━━━┫
	;              ┋                 ...                ┋
	;              ┋                                    ┋
	;           0h ┗━━━━━━━━━━━━━━━━━━┛ ← cs, ds, es, fs, ss
	;
	;
	; GDT 以及相应的描述符是这样的：
	;
	;		              Descriptors               Selectors
	;              ┏━━━━━━━━━━━━━━━━━━┓
	;              ┃         Dummy Descriptor           ┃
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_C    (0～4G)     ┃   8h = cs
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_FLAT_RW   (0～4G)     ┃  10h = ds, es, fs, ss
	;              ┣━━━━━━━━━━━━━━━━━━┫
	;              ┃         DESC_VIDEO                 ┃  1Bh = gs
	;              ┗━━━━━━━━━━━━━━━━━━┛
	;
	; 注意! 在使用 C 代码的时候一定要保证 ds, es, ss 这几个段寄存器的值是一样的
	; 因为编译器有可能编译出使用它们的代码, 而编译器默认它们是一样的. 比如串拷贝操作会用到 ds 和 es.
	;
	;


	; 把 esp 从 LOADER 挪到 KERNEL
	; modified by xw, 18/6/15
	;mov	esp, StackTop	; 堆栈在 bss 段中
	mov		esp, KernelStackTop	; 堆栈在 bss 段中

	mov		dword [disp_pos], 0

	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
	call	cstart		; 在此函数中改变了gdt_ptr，让它指向新的GDT
	lgdt	[gdt_ptr]	; 使用新的GDT

	lidt	[idt_ptr]

	jmp		SELECTOR_KERNEL_CS:csinit
csinit:		; “这个跳转指令强制使用刚刚初始化的结构”——<<OS:D&I 2nd>> P90.

	;jmp 0x40:0
	;ud2

	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax

	;sti
	jmp	kernel_main

	;hlt


; 中断和异常 -- 硬件中断
; ---------------------------------
%macro	hwint_master	1
	;call save
	call save_int			;save registers and some other things. modified by xw, 17/12/11

	inc  dword [k_reenter]  ;If k_reenter isn't equal to 0, there is no switching to the irq-stack, 
							;which is performed in save_int. Added by xw, 18/4/21
	
	;此处应设置条件编译，区分单核和多核

	;单核情形，通过8259来屏蔽当前中断
	in	al, INT_M_CTLMASK	; `.
	or	al, (1 << %1)		;  | 屏蔽当前中断，不允许再发生时钟中断
	out	INT_M_CTLMASK, al	; /
	
	;单核情形，向8259发送EOI
	mov	al, EOI				; `. 置EOI位
	out	INT_M_CTL, al		; /
	
	;多核情形，
	;每进入时钟中断时，此时的cr3都是当前进程的cr3，不含有对0xFEE00000的映射
	;为了保证稍后向LAPIC发送EOI正确，都需要修改cr3的值为内核cr3值(0x200000)
	;added by mingxuan 2019-3-5
	mov eax, 0x200000	;需要修改cr3的值为内核cr3值(0x200000)
	mov cr3, eax 

	;多核情形，向LAPIC发送EOI
	;added by mingxuan 2019-3-4
	mov ebx, 0xFEE000B0	;0xFEE000B0是LAPIC中EOI寄存器的地址
	mov	eax, 0			; `. 置EOI位
	mov	[ebx], eax		; /

	sti							; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断					
	push %1						; `.
	call [irq_table + 4 * %1]	;  | 中断处理程序，即clock_handler()
	pop	ecx						; /
	
	;push eax					; 	┓				add by visual 2016.4.5
	;mov eax,[cr3_ready]		; 	┣改变cr3
	;mov cr3,eax				;	┃
	;pop eax					; 	┛
	
	cli
	dec dword [k_reenter]

	;单核情形，通过8259来恢复接受当前中断
	in	al, INT_M_CTLMASK	; `.
	and	al, ~(1 << %1)		;  | 恢复接受当前中断，又允许时钟中断发生
	out	INT_M_CTLMASK, al	; /

	ret
%endmacro


ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
	hwint_master	0

ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
;primary edition, commented by xw
;	push	%1
;	call	spurious_irq
;	add	esp, 4
;	hlt
;~xw

	;added by xw, 18/5/29
	call save_int			;save registers and some other things. 

	inc  dword [k_reenter]  ;If k_reenter isn't equal to 0, there is no switching to the irq-stack, 
							;which is performed in save_int. Added by xw, 18/4/21
	
	;此处应设置条件编译，区分单核和多核
	
	in	al, INT_S_CTLMASK	; `.
	or	al, (1 << (%1 - 8))		;  | 屏蔽当前中断
	out	INT_S_CTLMASK, al	; /

	;单核情形，向8259发送EOI
	mov	al, EOI				; `.
	out	INT_M_CTL, al		; / 置EOI位(master)
	nop						; `.一定注意：slave和master都要置EOI	
	out	INT_S_CTL, al		; / 置EOI位(slave)

	;多核情形，
	;每进入硬盘中断时，此时的cr3都是当前进程的cr3，不含有对0xFEE00000的映射
	;为了保证稍后向LAPIC发送EOI正确，都需要修改cr3的值为内核cr3值(0x200000)
	;added by mingxuan 2019-3-6
	mov eax, 0x200000	;需要修改cr3的值为内核cr3值(0x200000)
	mov cr3, eax 

	;多核情形，向LAPIC发送EOI
	;added by mingxuan 2019-3-6
	mov ebx, 0xFEE000B0	;0xFEE000B0是LAPIC中EOI寄存器的地址
	mov	eax, 0			; `. 置EOI位
	mov	[ebx], eax		; /

	sti							; CPU在响应中断的过程中会自动关中断，这句之后就允许响应新的中断
	push %1						; `.
	call [irq_table + 4 * %1]	;  | 中断处理程序
	pop	ecx						; /
	
	cli
	dec dword [k_reenter]
	in	al, INT_S_CTLMASK		; `.
	and	al, ~(1 << (%1 - 8))	;  | 恢复接受当前中断
	out	INT_S_CTLMASK, al		; /
	ret
	;~xw
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
	hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
	hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
	hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
	hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
	hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
	hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
	hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
	hwint_slave	15

;commented by xw, 18/12/18
;commented begin
;; 中断和异常 -- 异常
;divide_error:
;	push	0xFFFFFFFF	; no err code
;	push	0		; vector_no	= 0
;	jmp	exception
;single_step_exception:
;	push	0xFFFFFFFF	; no err code
;	push	1		; vector_no	= 1
;	jmp	exception
;nmi:
;	push	0xFFFFFFFF	; no err code
;	push	2		; vector_no	= 2
;	jmp	exception
;breakpoint_exception:
;	push	0xFFFFFFFF	; no err code
;	push	3		; vector_no	= 3
;	jmp	exception
;overflow:
;	push	0xFFFFFFFF	; no err code
;	push	4		; vector_no	= 4
;	jmp	exception
;bounds_check:
;	push	0xFFFFFFFF	; no err code
;	push	5		; vector_no	= 5
;	jmp	exception
;inval_opcode:
;	push	0xFFFFFFFF	; no err code
;	push	6		; vector_no	= 6
;	jmp	exception
;copr_not_available:
;	push	0xFFFFFFFF	; no err code
;	push	7		; vector_no	= 7
;	jmp	exception
;double_fault:
;	push	8		; vector_no	= 8
;	jmp	exception
;copr_seg_overrun:
;	push	0xFFFFFFFF	; no err code
;	push	9		; vector_no	= 9
;	jmp	exception
;inval_tss:
;	push	10		; vector_no	= A
;	jmp	exception
;segment_not_present:
;	push	11		; vector_no	= B
;	jmp	exception
;stack_exception:
;	push	12		; vector_no	= C
;	jmp	exception
;general_protection:
;	push	13		; vector_no	= D
;	jmp	exception
;page_fault:
;	;page_fault_origin:
;	;push	14		; vector_no	= E
;	;jmp exception
;
;	;add by visual 2016.4.18
;	pushad          ; `.
;    push    ds      ;  |
;    push    es      ;  | 保存原寄存器值
;    push    fs      ;  |
;    push    gs      ; /
;    mov     dx, ss
;    mov     ds, dx
;    mov     es, dx
;	mov		fs, dx							;value of fs and gs in user process is different to that in kernel
;	mov		dx, SELECTOR_VIDEO - 2			;added by xw, 18/6/20
;	mov		gs, dx
;	
;	mov 	eax,[esp + RETADR - P_STACKBASE]; 把压入的错误码放进eax
;	mov 	ebx,[esp + EIPREG - P_STACKBASE]; 把压入的eip放进ebx
;	mov 	ecx,[esp + CSREG  - P_STACKBASE]; 把压入的cs放进ecx
;	mov 	edx,[esp + EFLAGSREG  - P_STACKBASE]; 把压入的eflags放进edx	
;	
;	;inc     dword [k_reenter]        ;k_reenter++;   ;k_reenter only counts if irq reenters, xw, 18/4/20
;	;cmp     dword [k_reenter], 0     ;if(k_reenter ==0)
;	;jne     .inkernel                ;(k_reenter !=0)	
;	;mov	esp, StackTop			  ;deleted by xw, 17/12/11		
;	;.inkernel:
;	push edx	;压入eflags
;	push ecx	;压入cs
;	push ebx	;压入eip
;	push eax	;压入错误码
;	push	14		; vector_no	= E
;	;jmp	exception
;	;call	exception_handler edit by visual 2016.4.19
;	call page_fault_handler
;	
;	add esp, 20	;clear 5 arguments in stack, added by xw, 17/12/11	
;	;jmp restart
;	jmp restart_restore	;modified by xw, 17/12/11
;	
;copr_error:
;	push	0xFFFFFFFF	; no err code
;	push	16		; vector_no	= 10h
;	jmp	exception
;
;exception:
;	call	exception_handler
;	add	esp, 4*2	; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
;	hlt
;commented by xw, 18/12/18
;commented end

; ====================================================================================
;                                   exception
; ====================================================================================
;restructured exception-handling procedure
;added by xw, 18/12/18
%macro	exception_no_errcode	2
	push	0xFFFFFFFF			;no err code
	call	save_exception		;save registers and some other things. 
	mov		esi, esp			;esp points to pushed address of restart_exception at present
	add		esi, 4 * 17			;we use esi to help to fetch arguments of exception handler from the stack.
								;17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
	mov		eax, [esi]			;saved eflags
	push	eax
	mov		eax, [esi - 4]		;saved cs
	push	eax
	mov		eax, [esi - 4 * 2]	;saved eip
	push	eax
	mov		eax, [esi - 4 * 3]	;saved err code
	push	eax
	push	%1					;vector_no
	sti
	call	%2
	cli
	add		esp, 4 * 5			;clear arguments of exception handler in stack
	ret							;returned to 'restart_exception' procedure
%endmacro

%macro	exception_errcode	2
	call	save_exception		;save registers and some other things.
	mov		esi, esp			;esp points to pushed address of restart_exception at present
	add		esi, 4 * 17			;we use esi to help to fetch arguments of exception handler from the stack.
								;17 is calculated by: 4+8+retaddr+errcode+eip+cs+eflag=17
	mov		eax, [esi]			;saved eflags
	push	eax
	mov		eax, [esi - 4]		;saved cs
	push	eax
	mov		eax, [esi - 4 * 2]	;saved eip
	push	eax
	mov		eax, [esi - 4 * 3]	;saved err code
	push	eax
	push	%1					;vector_no
	sti
	call	%2
	cli
	add		esp, 4 * 5			;clear arguments of exception handler in stack
	ret							;returned to 'restart_exception' procedure
%endmacro

divide_error:					; vector_no	= 0
;	exception_no_errcode	0, exception_handler
	exception_no_errcode	0, divide_error_handler	;added by xw, 18/12/22

single_step_exception:			; vector_no	= 1
	exception_no_errcode	1, exception_handler

nmi:							; vector_no	= 2
	exception_no_errcode	2, exception_handler
	
breakpoint_exception:			; vector_no	= 3
	exception_no_errcode	3, exception_handler
	
overflow:						; vector_no	= 4
	exception_no_errcode	4, exception_handler
	
bounds_check:					; vector_no	= 5
	exception_no_errcode	5, exception_handler
	
inval_opcode:					; vector_no	= 6
	exception_no_errcode	6, exception_handler
	
copr_not_available:				; vector_no	= 7
	exception_no_errcode	7, exception_handler
	
double_fault:					; vector_no	= 8
	exception_errcode	8, exception_handler
	
copr_seg_overrun:				; vector_no	= 9
	exception_no_errcode	9, exception_handler
	
inval_tss:						; vector_no	= 10
	exception_errcode	10, exception_handler
	
segment_not_present:			; vector_no	= 11
	exception_errcode	11, exception_handler
	
stack_exception:				; vector_no	= 12
	exception_errcode	12, exception_handler
	
general_protection:				; vector_no	= 13
	exception_errcode	13, exception_handler
	
page_fault:						; vector_no	= 14
	exception_errcode	14, page_fault_handler
	
copr_error:						; vector_no	= 16
	exception_no_errcode	16, exception_handler

;environment saving when an exception occurs
;added by xw, 18/12/18
save_exception:
    pushad          ; `.
    push    ds      ;  |
    push    es      ;  | 保存原寄存器值
    push    fs      ;  |
    push    gs      ; /
   
    mov     dx, ss
    mov     ds, dx
    mov     es, dx

	;value of fs and gs in user process is different to that in kernel
	;mov	fs, dx					;deleted by mingxuan 2019-1-16	
	mov		dx, SELECTOR_VIDEO - 2	;added by xw, 18/6/20
	mov		gs, dx

    mov     esi, esp                     
	push    restart_exception
	jmp     [esi + RETADR - P_STACKBASE]	;the err code is in higher address than retaddr in stack, so there is
											;no need to modify the position jumped to, that is, 
											;"jmp [esi + RETADR - 4 - P_STACKBASE]" is actually wrong.

;added by xw, 18/12/18
restart_exception:
	call	sched
	pop		gs
	pop		fs
	pop		es
	pop		ds
	popad
	add		esp, 4 * 2	;clear retaddr and error code in stack
	iretd
	
; ====================================================================================
;                                   save
; ====================================================================================
;modified by xw, 17/12/11
;modified begin
;save:
;        pushad          ; `.
;        push    ds      ;  |
;        push    es      ;  | 保存原寄存器值
;        push    fs      ;  |
;        push    gs      ; /
;        mov     dx, ss
;        mov     ds, dx
;        mov     es, dx
;
;        mov     esi, esp                    ;esi = 进程表起始地址
;
;        inc     dword [k_reenter]           ;k_reenter++;
;        cmp     dword [k_reenter], 0        ;if(k_reenter ==0)
;        jne     .1                          ;{
;        mov     esp, StackTop               ;  mov esp, StackTop <--切换到内核栈
;        push    restart                     ;  push restart
;        jmp     [esi + RETADR - P_STACKBASE];  return;
;.1:                                         ;} else { 已经在内核栈，不需要再切换
;        push    restart_reenter             ;  push restart_reenter
;        jmp     [esi + RETADR - P_STACKBASE];  return;
;                                            ;}
save_int:
        pushad          ; `.
        push    ds      ;  |
        push    es      ;  | 保存原寄存器值
        push    fs      ;  |
        push    gs      ; /
		
		cmp	    dword [k_reenter], 0			;Added by xw, 18/4/19
		jnz		instack
		
		;mov	ebx, [p_proc_current]			;modified by mingxuan 2019-1-16
		mov		ebx, 0x4
		mov		edx, [fs:ebx]

		;mov	dword [ebx + ESP_SAVE_INT], esp	;xw save esp position in the kernel-stack of the process
		mov		dword [edx + ESP_SAVE_INT], esp	;modified by mingxuan 2019-1-16

        mov     dx, ss
        mov     ds, dx
        mov     es, dx
		;mov	fs, dx	;deleted by mingxuan 2019-1-16						
											;value of fs and gs in user process is different to that in kernel
		mov		dx, SELECTOR_VIDEO - 2		;added by xw, 18/6/20
		mov		gs, dx

        mov     esi, esp  		                                        
	    mov     esp, StackTop   ;switches to the irq-stack from current process's kernel stack 

		push    restart_int		;added by xw, 18/4/19
		jmp     [esi + RETADR - P_STACKBASE]
instack:						;already in the irq-stack
	 	push    restart_restore	;modified by xw, 18/4/19
;		jmp		[esp + RETADR - P_STACKBASE]
		jmp		[esp + 4 + RETADR - P_STACKBASE]	;modified by xw, 18/6/4
                             

save_syscall:			;can't modify EAX, for it contains syscall number
						;can't modify EBX, for it contains the syscall argument

        ;1>用户态上下文STACK_TRAME全部压入进程内核栈
		pushad          ; `.
        push    ds      ;  |
        push    es      ;  | 保存原寄存器值
        push    fs      ;  |
        push    gs      ; /

		push	ebx		;ebx中存储了进程传入系统调用的参数，先压栈保护 ;added by mingxuan 2019-2-27

		;2>获得当前进程指针
		;mov	edx,  [p_proc_current]	;modified by mingxuan 2019-1-11
		mov		ebx, 0x4
		mov		edx, [fs:ebx]

		pop		ebx		;ebx中存储了进程传入系统调用的参数，出栈恢复 ;added by mingxuan 2019-2-27
		
		;3>将当前进程内核栈的栈顶esp保存到pcb的esp_save_syscall字段，用于后期继续压入context
		mov		dword [edx + ESP_SAVE_SYSCALL], esp	;xw save esp position in the kernel-stack of the process

		;4>设置段寄存器为内核值
        mov     dx, ss
        mov     ds, dx
        mov     es, dx
		;mov	fs, dx					;deleted by mingxuan 2019-1-15
		mov		dx, SELECTOR_VIDEO - 2	;value of fs and gs in user process is different to that in kernel
		mov		gs, dx					;added by xw, 18/6/20

		;5>设置sys_call结束后进入restart_syscall
        mov     esi, esp                                 
	    push    restart_syscall				;设置sys_call结束后进入restart_syscall

		;6>返回继续执行sys_call
	    jmp     [esi + RETADR - P_STACKBASE];RETADR就是STACK_FRAME中的retaddr
		
;modified end

; ====================================================================================
;                                sched(process switch)
; ====================================================================================
sched:
;could be called by C function, you must save ebp, ebx, edi, esi, 
;for C function assumes that they stay unchanged. added by xw, 18/4/19

;save_context
		pushfd
		pushad			;modified by xw, 18/6/4
		cli

		;mov	ebx, [p_proc_current]	;modified by mingxuan 2019-1-11	
		mov		ebx, 0x4
		mov		eax, [fs:ebx]

		;mov	dword [ebx + ESP_SAVE_CONTEXT], esp	;modified by mingxuan 2019-1-11
		mov		dword [eax + ESP_SAVE_CONTEXT], esp

;schedule
		;call	schedule			;schedule is a C function, save eax, ecx, edx if you want them to stay unchanged.
		;call	new_schedule		;added by mingxuan 2019-1-20  ;deleted by mingxuan 2019-3-5
		call	schedule			;modified by mingxuan 2019-3-5

;prepare to run new process
		;mov	ebx,  [p_proc_next]			;modified by mingxuan 2019-1-11
		;mov	eax,  [p_proc_next]			;deleted by mingxuan 2019-1-20

		;mov	dword [p_proc_current], ebx ;modified by mingxuan 2019-1-11
		;mov	ebx,  0x4					;deleted by mingxuan 2019-1-20
		;mov	[fs:ebx], eax				;deleted by mingxuan 2019-1-20

		call	renew_env			;renew process executing environment

;restore_context
		;mov	ebx, [p_proc_current] ;modified by mingxuan 2019-1-11
		mov		ebx, 0x4
		mov		eax, [fs:ebx]

		;mov 	esp, [ebx + ESP_SAVE_CONTEXT] ;modified by mingxuan 2019-1-11
		mov 	esp, [eax + ESP_SAVE_CONTEXT]

;		sti		;popfd will be executed below, so sti is not needed. modified by xw, 18/12/27
		sti		;added by mingxuan 2019-1-26
		popad
		popfd
		ret

; ====================================================================================
;                        			renew_env
; ====================================================================================
;renew process executing environment. Added by xw, 18/4/19
renew_env:
		;1>切换至新进程的cr3
		call	switch_pde		;to change the global variable cr3_ready
		;deleted by mingxuan 2019-1-22
		;mov 	eax,[cr3_ready]	;to switch the page directory table
		;mov 	cr3,eax

		;2>获取当前进程指针
		;mov	eax, [p_proc_current]	;modified by mingxuan 2019-1-11
		mov		ebx, 0x4
		mov		eax, [fs:ebx]
		
		;3>加载当前进程的LDT选择子至LDTR
		lldt	[eax + P_LDT_SEL]				;load LDT
		
		;4>依据此时的内核状态，更新TSS中的esp0
		call	renew_tss	;给tss的esp0赋值 	;modified by mingxuan 2019-1-22
		;deleted by mingxuan 2019-1-22
		;lea	ebx, [eax + INIT_STACK_SIZE]
		;mov	dword [tss + TSS3_S_SP0], ebx  ;renew esp0

		ret

; ====================================================================================
;                                 sys_call
; ====================================================================================
sys_call:
;get syscall number from eax
;syscall that's called gets its argument from pushed ebx
;so we can't modify eax and ebx in save_syscall
	call	save_syscall	;save registers and some other things. modified by xw, 17/12/11

	sti
	push 	ebx							;push the argument the syscall need
	call    [sys_call_table + eax * 4]	;将参数压入堆栈后再调用函数			add by visual 2016.4.6
	add		esp, 4						;clear the argument in the stack, modified by xw, 17/12/11
	cli	

	;mov	edx, [p_proc_current]	;modified by mingxuan 2019-1-14
	mov		ebx, 0x4
	mov		edx, [fs:ebx]

	mov 	esi, [edx + ESP_SAVE_SYSCALL]
	mov     [esi + EAXREG - P_STACKBASE], eax	;the return value of C function is in EAX
	ret

; ====================================================================================
;				    restart
; ====================================================================================
restart_int:
	;1>获取当前进程指针
	;mov	eax, [p_proc_current]			;modified by mingxuan 2019-1-16
	mov		ebx, 0x4
	mov		eax, [fs:ebx]

	;2>恢复esp指向内核栈的栈顶，准备新压入context
	mov 	esp, [eax + ESP_SAVE_INT]		;switch back to the kernel stack from the irq-stack	

	cmp	    dword [kernel_initial], 0		;added by xw, 18/6/10
	jnz		restart_restore

	call	sched							;save current process's context, invoke schedule(), and then
											;switch to the chosen process's kernel stack and restore it's context
											;added by xw, 18/4/19
	jmp     restart_restore

restart_syscall:

	;1>获取当前进程指针
	;mov	eax, [p_proc_current]			;modified by mingxuan 2019-1-11
	mov		ebx, 0x4
	mov		eax, [fs:ebx]

	;2>恢复esp指向内核栈的栈顶，准备新压入context
	mov 	esp, [eax + ESP_SAVE_SYSCALL]	;xw	restore esp position

	call	sched							;added by xw, 18/4/26

	jmp 	restart_restore

;xw	restart_reenter:
restart_restore:	;从进程内核栈中弹出STACK_FRAME中的所有寄存器，进程进入用户态
	pop		gs
	pop		fs	   	
	pop		es
	pop		ds
	popad		   	;依次弹出edi,esi,ebp,ebx,edx,ecx,eax

	add		esp, 4 	;跳过STACK_FRAME中的retaddr

	;push    0x10
	;call    disp_int

	;hlt

	iretd		   	;依次弹出eip,cs,eflags,esp,ss

;to launch the first process in the os. added by xw, 18/4/19
restart_initial:
	;1>更新cr3、ldtr、tss中的esp0
	call	renew_env						;renew process executing environment
	
	;2>获取当前进程指针
	;mov	eax, [p_proc_current]			;modified by mingxuan 2019-1-11
	mov		ebx, 0x4
	mov		eax, [fs:ebx]

	;3>设置esp指向进程内核栈的栈顶
	mov 	esp, [eax + ESP_SAVE_INT]		;设置esp指向进程内核栈的栈顶

	jmp 	restart_restore 				;从进程内核栈中弹出STACK_FRAME中的所有寄存器，进程进入用户态

; ====================================================================================
;				    read_cr2				//add by visual 2016.5.9
; ====================================================================================	
read_cr2:
	mov eax,cr2
	ret
	
; ====================================================================================
;				    refresh_page_cache		//add by visual 2016.5.12
; ====================================================================================	
refresh_page_cache:
	mov eax,cr3
	mov cr3,eax
	ret
	
; ====================================================================================
;				    halt					//added by xw, 18/6/11			
; ====================================================================================
halt:
	hlt
	
; ====================================================================================
;				    u32 get_arg(void *uesp, int order)		//added by xw, 18/6/18			
; ====================================================================================
; used to get the specified argument of the syscall from user space stack
; @uesp: user space stack pointer
; @order: which argument you want to get
; @uesp+0: the number of args, @uesp+8: the first arg, @uesp+12: the second arg...
get_arg:
	push ebp
	mov ebp, esp
	push esi
	push edi
	mov esi, dword [ebp + 8]	;void *uesp
	mov edi, dword [ebp + 12]	;int order
	mov eax, dword [esi + edi * 4 + 4]
	pop edi
	pop esi
	pop ebp
	ret

; ====================================================================================
;				    read_cr3			//added by mingxuan 2018-10-17
; ====================================================================================	
read_cr3:
	mov eax,cr3
	ret

; ====================================================================================
;				    down_failed			//added by mingxuan 2019-3-28
; ====================================================================================	
down_failed:
	push edx
	push ecx
	call do_down
	pop  ecx
	pop  edx
	ret

; ====================================================================================
;				    up_wakeup			//added by mingxuan 2019-3-28
; ====================================================================================	
up_wakeup:
	push edx
	push ecx
	call do_up
	pop  ecx
	pop  edx
	ret

; ====================================================================================
;				    read_lock_failed			//added by mingxuan 2019-4-8
; ====================================================================================	
;读锁：持续等待另一进程对写锁的释放
read_lock_failed:
    lock
	inc eax 				;首先将rw加1
;下面4行代码形成一个自旋:何时跳出自旋？read_unlock将rw加1
read_lock_failed_loop:  
	rep
	nop	   					;相当于pause指令
    cmp eax, 1 				;将rw同立即数1进行比较
    js read_lock_failed_loop;如果不相等，则继续跳转到“1”标志位处继续循环比较，直到相等
    						;相等的时候，说明系统执行了读锁的释放函数，将rw变量加1了，具体可参看read_unlock()函数实现
    lock 
	dec eax 				;还需将rw减1，表示另一进程申请读锁成功，从而保证后续申请读写锁的进程的正确性
    js read_lock_failed
    ret
;rwlock暂无用处，若使用，还需调试

; ====================================================================================
;				    write_lock_failed			//added by mingxuan 2019-4-8
; ====================================================================================
;写锁：持续等待所有进程对读锁的释放
write_lock_failed:
    lock 
	add eax, 0x01000000 		;首先将rw变量加0x01000000
;下面4行代码形成一个自旋:何时跳出自旋？write_unlock将rw加0x01000000
write_lock_failed_loop:  
	rep
	nop 						;相当于pause指令
    cmp eax, 0x01000000			;将rw同立即数0x01000000进行比较
    jne write_lock_failed_loop	;如果不相等，则继续跳转到“1”标志位处继续循环比较，直到相等
    							;相等的时候，说明系统执行了写锁的释放函数，将rw变量加0x01000000了，具体可参看write_unlock()函数实现
    lock 
	sub eax, 0x01000000 		;还需将rw减0x01000000，表示另一进程申请写锁成功，从而保证后续申请读写锁的进程的正确性
    jnz write_lock_failed
    ret
;rwlock暂无用处，若使用，还需调试