
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            const.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef	_ORANGES_CONST_H_
#define	_ORANGES_CONST_H_

/*最大整数定义*/
#define MAX_UNSIGNED_INT 0xFFFFFFFF		//最大的无符号整形
#define MAX_INT 0x7FFFFFFF			//最大的整形数

/* EXTERN */
#define	EXTERN	extern	/* EXTERN is defined as extern except in global.c */

/* 函数类型 */
#define	PUBLIC		/* PUBLIC is the opposite of PRIVATE */
#define	PRIVATE	static	/* PRIVATE x limits the scope of x */

/* Boolean */
#define	TRUE	1
#define	FALSE	0

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128
#define	IDT_SIZE	256

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3
/* RPL */
#define	RPL_KRNL	SA_RPL0
#define	RPL_TASK	SA_RPL1
#define	RPL_USER	SA_RPL3

/* 8259A interrupt controller ports. */
#define	INT_M_CTL	0x20	/* I/O port for interrupt controller         <Master> */
#define	INT_M_CTLMASK	0x21	/* setting bits in this port disables ints   <Master> */
#define	INT_S_CTL	0xA0	/* I/O port for second interrupt controller  <Slave>  */
#define	INT_S_CTLMASK	0xA1	/* setting bits in this port disables ints   <Slave>  */

/* 8253/8254 PIT (Programmable Interval Timer) */
#define TIMER0         0x40 /* I/O port for timer channel 0 */
#define TIMER_MODE     0x43 /* I/O port for timer mode control */
#define RATE_GENERATOR 0x34 /* 00-11-010-0 :
			     * Counter0 - LSB then MSB - rate generator - binary
			     */
#define TIMER_FREQ     1193182L/* clock frequency for timer in PC and AT */
#define HZ             100  /* clock freq (software settable on IBM-PC) */

/* Hardware interrupts */
#define	NR_IRQ		16	/* Number of IRQs */
#define	CLOCK_IRQ	0
#define	KEYBOARD_IRQ	1
#define	CASCADE_IRQ	2	/* cascade enable for 2nd AT controller */
#define	ETHER_IRQ	3	/* default ethernet interrupt vector */
#define	SECONDARY_IRQ	3	/* RS232 interrupt vector for port 2 */
#define	RS232_IRQ	4	/* RS232 interrupt vector for port 1 */
#define	XT_WINI_IRQ	5	/* xt winchester */
#define	FLOPPY_IRQ	6	/* floppy disk */
#define	PRINTER_IRQ	7
#define	AT_WINI_IRQ	14	/* at winchester */

/* Hardware interrupts by IOAPIC*/
//added by mingxuan 2019-5-7
#define IO_CLOCK_IRQ  	2
#define IO_KEYBOARD_IRQ	1
#define IO_HD_IRQ	  	14

/* system call */
//#define NR_SYS_CALL     23	//last modified by xw, 18/6/19
//#define NR_SYS_CALL     24    //添加了一个get_cpuid  //modified by mingxuan 2019-3-1
//#define NR_SYS_CALL     28	//添加了4个和消息队列有关的系统调用	//added by mingxuan 2019-5-14
#define NR_SYS_CALL       32	//添加了4个和邮箱通信有关的系统调用	//added by mingxuan 2019-5-14

/*页表相关*/
#define	PageTblNumAddr		0x500	//页表数量放在这个位置,必须与load.inc中一致					add by visual 2016.5.11
#define KernelPageTblAddr	0x200000 //内核页表物理地址，必须与load.inc中一致			add by visual 2016.5.17
/*线性地址描述*/	//edit by visual 2016.5.25
#define	KernelSize			0x800000 			//内核的大小//add by visual 2016.5.10
#define K_PHY2LIN(x)		((x)+0xC0000000)	//内核中物理地址转线性地址		//add by visual 2016.5.10
#define K_LIN2PHY(x)		((x)-0xC0000000)	//added by xw, 18/8/27
#define num_4B	0x4			//4B大小
#define num_1K	0x400		//1k大小
#define num_4K	0x1000		//4k大小
#define num_4M	0x400000	//4M大小
#define TextLinBase 			((u32)0x0) 						//进程代码的起始地址，这是参考值，具体以elf描述为准
#define TextLinLimitMAX   		(TextLinBase+0x20000000)  	//大小：512M，这是参考值，具体以elf描述为准，
#define DataLinBase 			TextLinLimitMAX 			//进程数据的起始地址，这是参考值，具体以elf描述为准
#define DataLinLimitMAX   		(DataLinBase+0x20000000)  	//大小：512M，这是参考值，具体以elf描述为准，但是代码和数据长度总和不能超过这个值
#define VpageLinBase 			DataLinLimitMAX 			//保留内存起始地址
#define VpageLinLimitMAX  		(VpageLinBase+0x8000000-num_4K) //大小：128M-4k
#define SharePageBase			VpageLinLimitMAX			//共享页线性地址，执行fork\pthread的时候用,共享页必须4K对齐
#define SharePageLimit			(SharePageBase+num_4K)		//大小：4k
#define HeapLinBase 			SharePageLimit	 			//堆的起始地址
#define HeapLinLimitMAX  		(HeapLinBase+0x40000000)  	//大小：1G
#define StackLinLimitMAX		HeapLinLimitMAX				//栈的大小： 1G-128M-4K（注意栈的基址和界限方向）
#define StackLinBase			(ArgLinBase-num_4B)			//=(StackLinLimitMAX+1G-128M-4K-4B)栈的起始地址,放在参数位置之前（注意堆栈的增长方向）
#define ArgLinBase 				(KernelLinBase-0x1000)		//参数存放位置起始地址，放在3G前，暂时还没没用到
#define ArgLinLimitMAX  		KernelLinBase  				//=(ArgLinBase+0x1000)大小：4K。
#define	KernelLinBase			0xC0000000 					//内核线性起始地址(有0x30400的偏移)
#define	KernelLinLimitMAX		(KernelLinBase+0x40000000) 	//大小：1G

/***************目前线性地址布局*****************************		edit by visual 2016.5.25
*				进程代码		0 ~ 512M ,限制大小为512M
*				进程数据		512M ~ 1G，限制大小为512M
*				进程保留内存（以后可能存放虚页表和其他一些信息） 1G ~ 1G+128M，限制大小为128M,共享页放在这个位置	
*				进程堆			1G+128M ~ 2G+128M，限制大小为1G				
*				进程栈			2G+128M ~ 3G-4K,限制大小为 1G-128M-4K
*				进程参数		3G-4K~3G，限制大小为4K
*				内核			3G~4G，限制大小为1G
***********************************************************/

//#define ShareTblLinAddr			(KernelLinLimitMAX-0x1000)	//公共临时共享页，放在内核最后一个页表的最后一项上
	
/*分页机制常量的定义,必须与load.inc中一致*/				//add by visual 2016.4.5		
#define	PG_P		1	// 页存在属性位
#define	PG_RWR		0	// R/W 属性位值, 读/执行
#define	PG_RWW		2	// R/W 属性位值, 读/写/执行
#define	PG_USS		0	// U/S 属性位值, 系统级
#define	PG_USU		4	// U/S 属性位值, 用户级

#define PG_A		32	// Accessed属性位值 added by mingxuan 2018-12-20

//#define PG_PS		64	// PS属性位值，4K页
#define PG_PS		128	// PS属性位值，4K页 modified by mingxuan 2018-12-20

//和CPU相关     //added by mingxuan 2019-1-22
//#define NCPU             2  // maximum number of CPUs
#define NCPU           4  //modified by mingxuan 2019-3-13	
//#define N_READY_PROC 3  // 每颗CPU的就绪队列中的进程数    //deleted by mingxuan 2019-2-27
//#define N_READY_PROC 8  // 每颗CPU的就绪队列中的进程数等于pcb的个数    //modified by mingxuan 2019-2-28


//键盘相关
//added by mingxuan 2019-3-8
/* AT keyboard */
/* 8042 ports */
#define KB_DATA		0x60	/* I/O port for keyboard data
					Read : Read Output Buffer
					Write: Write Input Buffer(8042 Data&8048 Command) */
#define KB_CMD		0x64	/* I/O port for keyboard command
					Read : Read Status Register
					Write: Write Input Buffer(8042 Command) */

//显示相关
//added by mingxuan 2019-3-8
/* VGA */
#define	CRTC_ADDR_REG	0x3D4	/* CRT Controller Registers - Addr Register */
#define	CRTC_DATA_REG	0x3D5	/* CRT Controller Registers - Data Register */
#define	START_ADDR_H	0xC	/* reg index of video mem start addr (MSB) */
#define	START_ADDR_L	0xD	/* reg index of video mem start addr (LSB) */
#define	CURSOR_H	0xE	/* reg index of cursor position (MSB) */
#define	CURSOR_L	0xF	/* reg index of cursor position (LSB) */
#define	V_MEM_BASE	0xB8000	/* base of color video memory */
#define	V_MEM_SIZE	0x8000	/* 32K: B8000H -> BFFFFH */


/* ipc.c */
//added by mingxuan 2019-5-13
#define IPC_NOWAIT 0
#define IPC_WAIT   1

#define IPC_RMID   0 

#define MSGMNB 0xA //一个消息队列最多拥有的结点数 10个
#define MSGMAX 0x80 //一条消息结点中的缓冲区最大长度 128字节
#define MSGMNI 0xA //系统中最多的消息队列数量

#define BOXMNB 0xA 
#define BOXMAX 0x80

/* klib.c */
//added by mingxuan 2019-5-13
#define DISP_POS_MAX    (80*25*2)

#endif /* _ORANGES_CONST_H_ */
