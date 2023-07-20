
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "cpu.h"	   //added by mingxuan 2018-12-21
#include "x86.h"	   //added by mingxuan 2018-12-21
#include "spinlock.h"  //added by mingxuan 2019-1-16
#include "semaphore.h" //added by mingxuan 2019-3-28


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	int error;

	//zcr added(清屏)
	disp_pos = 0;
	for (int i = 0; i < 25; i++) {
		for (int j = 0; j < 80; j++) {
			disp_str(" ");
		}
	}
	disp_pos = 0;

	disp_str("-----Kernel Initialization Begins-----\n");
	kernel_initial = 1;	//kernel is in initial state. added by xw, 18/5/31
	
	init();//内存管理模块的初始化  add by liang 
	
	mpinit(); //检测CPU个数并将检测到的CPU存入一个全局的数组  added by mingxuan 2018-11-8
	
	lapicinit(); //BSP初始化自己的LAPIC  added by mingxuan 2018-11-8

	ioapicinit(); //added by mingxuan 2019-3-4

	cpuinit(); //包含初始化cpu的ready_queue //modified by mingxuan 2019-4-1

	gdtinit(); //把BSP的GDT都放在自己的CPU结构里

	//initlock(&ptlock, "ptlock"); //初始化proc_table的spinlock			 //added by mingxuan 2019-1-16
	initlock(&cr3lock, "cr3lock");//cr3_ready是全局变量，给cr3_ready加锁	   //added by mingxuan 2019-1-21
	initlock(&allocPCBlock, "allocPCBlock");		   //added by mingxuan 2019-3-5

	/*和文件系统有关的锁*/
	initlock(&inode_table_lock, "inode_table_lock");   //added by mingxuan 2019-3-20
	initlock(&f_desc_table_lock, "f_desc_table_lock"); //added by mingxuan 2019-3-20
	initlock(&super_block_lock, "super_block_lock");   //added by mingxuan 2019-3-21
	initlock(&alloc_imap_bit_lock, "alloc_imap_bit_lock");	//added by mingxuan 2019-3-20
	initlock(&alloc_smap_bit_lock, "alloc_smap_bit_lock");	//added by mingxuan 2019-3-20
	initlock(&sync_inode_lock, "sync_inode_lock");	//added by mingxuan 2019-3-25

	/*和负载均衡有关的锁*/
	initlock(&load_balance_lock, "load_balance_lock");	//added by mingxuan 2019-4-3

	/*信号量*/
	init_sema(&inode_table_sem, 1);	//added by mingxuan 2019-3-28

	/*
	initlock(&execlock, "execlock");	//added by mingxuan 2019-3-7
	initlock(&forklock, "forklock"); //added by mingxuan 2019-3-12
	initlock(&pthreadlock, "pthreadlock"); //added by mingxuan 2019-3-14
	*/

	//initialize PCBs, added by xw, 18/5/26
	error = initialize_processes();
	if(error != 0)
		return error;
	
	k_reenter = 0;	//record nest level of only interruption! it's different from Orange's.
					//usage modified by xw
	ticks = 0;		//initialize system-wide ticks

	//startothers();	//added by mingxuan 2019-3-10

	//p_proc_current = cpu_table;	//deleted by mingxuan 2019-1-16
	//proc = cpu_table;	//modified by mingxuan 2019-1-16	//deleted by mingxuan 2019-2-27

	init_msg_queue_manage(); //added by mingxuan 2019-5-13
	init_box();				 //added by mingxuan 2019-5-14
	
	init_clock(); //初始化8259A时钟 //added by mingxuan 2019-3-10
	init_keyboard(); //added by mingxuan 2019-3-8
	init_hd();	/* initialize hd-irq and hd rdwt queue */

	/* enable interrupt, we should read information of some devices by interrupt.
	 * Note that you must have initialized all devices ready before you enable
	 * interrupt. added by xw
	 */
	enable_int();	//deleted by mingxuan 2018-11-8

    /***********************************************************************
	open hard disk and initialize file system
	coded by zcr on 2017.6.10. added by xw, 18/5/31
	************************************************************************/
	hd_open(MINOR(ROOT_DEV));	//ROOT_DEV是根设备的设备号，MINOR(ROOT_DEV)为设备的次设备号
	init_fs();					//deleted by mingxuan 2018-11-8

	/* we don't want interrupt happens before processes run.
	 * added by xw, 18/5/31
	 */
	disable_int();	//modified by mingxuan 2019-3-6

	/*************************************************************************
	*BSP发送SIPI唤醒AP	
	*added by mingxuan 2018-12-20
	**************************************************************************/
	startothers();	//added by mingxuan 2018-12-20

	/*************************************************************************
	*第一个进程开始启动执行
	**************************************************************************/
	/* we don't want interrupt happens before processes run.
	 * added by xw, 18/5/31
	 */
	//disable_int();	//deleted by mingxuan 2018-11-8
	
	//disp_str("\n-----BSP Processes Begin-----\n");

	/* linear address 0~8M will no longer be mapped to physical address 0~8M.
	 * note that disp_xx can't work after this function is invoked until processes runs.
	 * add by visual 2016.5.13; moved by xw, 18/5/30
	 */
	clear_kernel_pagepte_low();		//deleted by mingxuan 2019-3-4

	//p_proc_current = proc_table; //deleted by mingxuan 2019-1-11	
	proc = proc_table;			   //modified by mingxuan 2019-1-11

	kernel_initial = 0;		//kernel initialization is done. added by xw, 18/5/31
	restart_initial();	//modified by xw, 18/4/19

	while(1){}
}


/*======================================================================*
                             mpenter		added by mingxuan 2018-11-19
 *======================================================================*/
void mpenter(void)
{
	struct cpu *c; //added by mingxuan 2019-1-9

	ap_pgdirinit(); //AP初始化自己的页机制
	lapicinit(); //AP初始化自己的LAPIC
	gdtinit(); ////把AP的GDT都放在自己的CPU结构里
	ap_idtinit(); //仅仅是lidt

	//added by mingxuan 2019-3-14
	disp_str("cpu ");
	disp_int(cpu->id);
	disp_str(" start!\n");
	//disp_str("ap enter!\n");

	/*************************************************************************
	*AP的第一个进程开始启动执行
	**************************************************************************/
	//disp_str("\n-----AP Processes Begin-----\n");

	kernel_initial = 0;	//控制进程是否调度

	//开启第一个进程
	//p_proc_current = proc_table; //modified by mingxuan 2019-1-13
	//proc = proc_table + 1; //deleted by mingxuan 2019-3-7
	//proc = proc_table + NR_TASKS + cpu->id; //modified by mingxuan 2019-3-13
	proc = proc_table + cpu->id; //modified by mingxuan 2019-4-11

	//xchg(&(cpulist + 1)->started, 1); //modified by mingxuan 2019-1-13 //deleted by mingxuan 2019-3-13
	xchg(&(cpulist + cpu->id)->started, 1); //modified by mingxuan 2019-1-13

	//进入第一个进程
	restart_initial();	//modified by xw, 18/4/19

	disp_str("schedule finish!\n");

	//xchg(&(cpulist + 1)->started, 1); //tell startothers() we're up
}

/*======================================================================*
                             gdtinit()		added by mingxuan 2018-11-29
 *======================================================================*/
void gdtinit(void)
{
//拷贝gdt,设置ap可以访问自己的CPUID
	struct cpu *c;
	
	//1>获取当前CPU的ID号
	c = &cpulist[cpunum()];//c就指向当前运行的cpu
	c->id = cpunum();//cpunum()可获取当前cpuid

	//2>嵌入汇编，读gdtr的内容到gdt_ptr
	sgdt(c->id); //把gdtr中的内容加载到c->gdt_ptr

	//3>拷贝临时GDT：c->gdt_ptr(旧GDT)到cpu->gdt
	memcpy(&c->gdt, c->gdt_ptr->gdt_base, c->gdt_ptr->gdt_limit);
	c->gdt_ptr->gdt_base = (u32)&c->gdt;
	c->gdt_ptr->gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;

	//4>嵌入汇编，将gdt_ptr载入gdtr,完成GDT的拷贝
	lgdt(&c->gdt, c->gdt_ptr->gdt_limit);
	
	//暂时借用bsp的gdt
	//lgdt(&gdt, GDT_SIZE * sizeof(DESCRIPTOR) - 1);

	/*修改显存描述符*/ //add by visual 2016.5.12
	init_descriptor(&c->gdt[INDEX_VIDEO],
					K_PHY2LIN(0x0B8000),
					0x0ffff,
					DA_DRW | DA_DPL3);	//此处为什么要将显存的修改为DPL3？


	/* 填充 GDT 中 TSS 这个描述符 */
	//我也不知道为什么这里就必须重新设置GDT中的tss描述符
	/*	//deleted by mingxuan 2019-1-22
	memset(&tss, 0, sizeof(tss));
	tss.ss0		= SELECTOR_KERNEL_DS;
	init_descriptor(&c->gdt[INDEX_TSS],
			vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
			sizeof(tss) - 1,
			DA_386TSS);
	tss.iobase	= sizeof(tss);	//没有I/O许可位图

	ltr(SELECTOR_TSS); //加载TR寄存器
	*/

	/* 填充 GDT 中 TSS 这个描述符 */
	//modified by mingxuan 2019-1-22
	memset(&c->tss, 0, sizeof(tss));
	c->tss.ss0		= SELECTOR_KERNEL_DS; //给tss的sp0赋值
	init_descriptor(&c->gdt[INDEX_TSS],
			vir2phys(seg2phys(SELECTOR_KERNEL_DS), &c->tss), //gdt中描述符所指向的段
			sizeof(tss) - 1,
			DA_386TSS);
	c->tss.iobase	= sizeof(tss);	//没有I/O许可位图

	ltr(SELECTOR_TSS); //加载TR寄存器
	

	//设置fs寄存器和gdt的描述符，便能直接通过cpu和proc变量访问当前CPU的结构体和运行进程
	//added by mingxuan 2019-1-11
	init_descriptor(&c->gdt[INDEX_CPU],
			vir2phys(seg2phys(SELECTOR_KERNEL_DS), &c->cpu),
			8,
			DA_DRW | DA_DPL3); //模仿GS段的特权级，设置为DPL3

	loadfs(INDEX_CPU << 3);

	lgdt(&c->gdt, c->gdt_ptr->gdt_limit); //added by mingxuan 2019-1-15

	cpu = c;
	proc = 0;

	//lgdt(&c->gdt, c->gdt_ptr->gdt_limit); //added by mingxuan 2019-1-15

	// 填充 GDT 中进程的 LDT 的描述符
	int i;
	PROCESS* p_proc	= proc_table;
	u16 selector_ldt = INDEX_LDT_FIRST << 3;
	//for(i=0;i<NR_TASKS;i++){		
	for(i=0;i<NR_PCBS;i++){										//edit by visual 2016.4.5	
		init_descriptor(&c->gdt[selector_ldt>>3],
				vir2phys(seg2phys(SELECTOR_KERNEL_DS),proc_table[i].task.ldts),
				LDT_SIZE * sizeof(DESCRIPTOR) - 1,
				DA_LDT);
		p_proc++;
		selector_ldt += 1 << 3;
	}
	
}

/*======================================================================*
                             ap_idtinit()		added by mingxuan 2018-12-25
 *======================================================================*/
void ap_idtinit(void)
{
	//bsp和ap共用一个IDT
	lidt(&idt, IDT_SIZE * sizeof(GATE) - 1);
	//ltr(SELECTOR_TSS);
	//如何验证idt加载完成？

}


/*======================================================================*
                             startothers	added by mingxuan 2018-10-23
 *======================================================================*/
void startothers(void)
{
	extern u32 _binary_kernel_entryother_bin_start[], _binary_kernel_entryother_bin_size[]; //added by mingxuan 2018-11-13
	
	struct cpu *c;
	extern cpunumber;//cpu总数

	u32 ap_boot_addr = 0x7000; //物理内存的0x7000固定为AP的启动地址

	//把堆栈放在0x7000-4的位置
	u32 ap_stack;
	ap_stack = do_kmalloc_4k();
	ap_stack = ap_stack + num_4K - num_4B + 0xc0000000; //注意：由于堆栈是倒着生长，所以要加4K才能从高地址到低地址使用kmalloc申请的这张物理页
							    //此处必须加3G，因为此时栈地址是低端，在切换到进程cr3后只有高端映射，所以必须提前将栈地址设为高地址
	*(u32 *)(ap_boot_addr - 4) = ap_stack;

	//把mpenter入口地址放在0x7000-8的位置
	*(u32 *)(ap_boot_addr - 8) = mpenter;

	//把页目录表首地址放在0x7000-12的位置
	u32 bsp_cr3;
	bsp_cr3 = read_cr3();
	*(u32 *)(ap_boot_addr - 12) = bsp_cr3;

	//拷贝entryother的二进制代码，放置到内存地址0x7000处
	memcpy(ap_boot_addr, _binary_kernel_entryother_bin_start, _binary_kernel_entryother_bin_size); //modified by mingxuan 2018-11-13

	for(c = cpulist + 1; c < cpulist + cpunumber; c++)	//cpulist[0] is BSP, cpulist[1] is AP1, (cpulist + 1) is AP1
	{	

		//BSP向AP发送SIPI启动信号
		lapicstartap(c->id, ap_boot_addr);		//add by mingxuan 2018-10-17

		//延迟，等待AP完成mpenter
		while(c->started == 0)
		;
			
	}	
	
}


/*======================================================================*
                             ap_pgdirinit		added by mingxuan 2018-11-26
 *======================================================================*/
void ap_pgdirinit(void)
{

	u32 bsp_cr3 = read_cr3();//获取bsp页目录表

	u32 pgdir_phy;
	pgdir_phy = (u32)do_kmalloc_4k();//申请一页作为ap页目录表
	memcpy(pgdir_phy, bsp_cr3, num_4K);//将bsp页目录表全部内容拷贝到ap页目录表
					   //注意：此时ap页目录表项全部指向bsp的页表，之后会修改它们指向ap页表

	u32 page_phy1;//此页用来映射0~4M与3G～3G+4M
	u32 page_phy2;//此页映射4M～8M与3G+4M～3G+8M
	u32 i = 0,j = KernelLinBase;//i表示线性地址0~8M，j表示线性地址3G～3G+8M

	for(i=0,j = KernelLinBase; i<KernelSize,j<KernelLinBase+KernelSize; i+=num_4M,j+=num_4M)
	{//本循环共执行2趟：第1趟完成映射0~4M与3G～3G+4M，第2趟完成映射4M～8M与3G+4M～3G+8M

		page_phy1 = (u32)do_kmalloc_4k();//申请一页作为ap页表，用来映射0~4M(第2趟循环映射4M～8M）
		memcpy(page_phy1, (*((u32*)bsp_cr3+get_pde_index(i)) & 0xFFFFF000), num_4K);//将bsp相应页表的全部内容拷贝到ap页表(第2趟同理)
		*((u32*)pgdir_phy+get_pde_index(i)) = (page_phy1 & 0xFFFFF000) | PG_P | PG_USU | PG_RWW | PG_A ;//修改ap页目录表项指向ap页表

		page_phy2 = (u32)do_kmalloc_4k();//申请一页作为ap页表，用来映射3G～3G+4M(第2趟循环映射3G+4M～3G+8M）
		memcpy(page_phy2, page_phy1, num_4K);//由于0~4M与3G～3G+4M的映射关系完全相同，直接拷贝即可(第2趟同理)
		*((u32*)pgdir_phy+get_pde_index(j)) = (page_phy2 & 0xFFFFF000) | PG_P | PG_USU | PG_RWW | PG_A ;//修改ap页目录表项指向ap页表
	}//至此完成ap页目录表和ap的4张页表的全部赋值

	//将ap页目录的首地址赋给cr3，完成页机制初始化
	lcr3(pgdir_phy);

}








