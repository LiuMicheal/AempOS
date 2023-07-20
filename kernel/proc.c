
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
#include "cpu.h" 	  //added by mingxuan 2019-1-11
#include "spinlock.h" //added by mingxuan 2019-1-16
#include "x86.h"	  //added by mingxuan 2019-4-12


void init_ready_queue(struct ready_queue *rq);	//added by mingxuan 2019-4-3

/*************************************************************************
多颗CPU的初始化部分
return 0 if there is no error, or return -1.
added by xw, 18/6/2
***************************************************************************/
PUBLIC int cpuinit()
{
	struct cpu *c;
	int i;

	for(c = cpulist; c < cpulist + cpunumber; c++)
	{	
		c->started = 0; //added by mingxuan 2019-4-1

		//结构体中嵌套结构体时，一定要单独定义内层结构体，对其初始化，否则会造成野指针
		//added by mingxuan 2019-4-1
		gdt_ptr_list[c->id].gdt_base = 0x0;
		gdt_ptr_list[c->id].gdt_limit = (u16)0x0;
		c->gdt_ptr = &gdt_ptr_list[c->id];

		//初始化就绪队列
		//for (i=0; i < N_READY_PROC; i++)  //deleted by mingxuan 2019-4-3      
      	//	c->ready_proc[i] = -1;			//deleted by mingxuan 2019-4-3
		init_ready_queue(&c->ready);	//modified by mingxuan 2019-4-3
		c->nr_ready_proc = 0;
		
	}	
	
	return 1;
}


/*************************************************************************
进程初始化部分
return 0 if there is no error, or return -1.
moved from kernel_main() by xw, 18/5/26
***************************************************************************/
PUBLIC int initialize_processes()
{
	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	u16			selector_ldt	= SELECTOR_LDT_FIRST;	
	char* 		p_regs;		    //point to registers in the new kernel stack, added by xw, 17/12/11
	task_f		eip_context;	//a funtion pointer, added by xw, 18/4/18
	/*************************************************************************
	*进程初始化部分 	edit by visual 2016.5.4 
	***************************************************************************/
	int pid;
	u32 AddrLin,pte_addr_phy_temp,addr_phy_temp,err_temp;//edit by visual 2016.5.9
	
	/* set common fields in PCB. added by xw, 18/5/25 */
	p_proc = proc_table;
	for( pid=0 ; pid<NR_PCBS ; pid++ )
	{
		//some operations
		p_proc++;
	}
	
	p_proc = proc_table;

	//1>规定proc_table的前4个为4个initial，即4个0#
	//目前是4个initial，分别为cpu0的initial(),cpu1的initial1(),cpu2的initial2(),cpu3的initial3()
	for( pid=0; pid<NR_INITIALS ; pid++ )
	{//initial 进程的初始化				//add by visual 2016.5.17
		/*************基本信息*********************************/
		//strcpy(p_proc->task.p_name,"initial");		//名称 //deleted by mingxuan 2019-3-7
		//modified by mingxuan 2019-3-14
		switch (pid)									//名称
		{
			case INITIAL_PID: strcpy(p_proc->task.p_name,"initial"); break;
			case INITIAL1_PID: strcpy(p_proc->task.p_name,"initial1"); break;
			case INITIAL2_PID: strcpy(p_proc->task.p_name,"initial2"); break;
			case INITIAL3_PID: strcpy(p_proc->task.p_name,"initial3"); break;
			default: break;
		}
		p_proc->task.pid = pid;							//pid
		p_proc->task.stat = READY;  					//状态
		
		/**************LDT*********************************/
		p_proc->task.ldt_sel = selector_ldt;
		memcpy(&p_proc->task.ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->task.ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		
		/**************寄存器初值**********************************/
		p_proc->task.regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		//p_proc->task.regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK; //deleted by mingxuan 2019-2-27
		p_proc->task.regs.fs	= (SELECTOR_KERNEL_FS & SA_RPL_MASK)| RPL_TASK; //modified by mingxuan 2019-1-11
																				//为了[fs:0]表示当前cpu，[fs:4]表示当前进程		
		p_proc->task.regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)| RPL_TASK;
		p_proc->task.regs.eflags = 0x1202; /* IF=1, IOPL=1 */
		//p_proc->task.cr3 在页表初始化中处理
		
		
		/**************线性地址布局初始化**********************************/	//edit by visual 2016.5.25
		p_proc->task.memmap.text_lin_base = 0;	//initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
		p_proc->task.memmap.text_lin_limit = 0;	//initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
		p_proc->task.memmap.data_lin_base = 0;	//initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
		p_proc->task.memmap.data_lin_limit= 0;	//initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
		p_proc->task.memmap.vpage_lin_base = VpageLinBase;					//保留内存基址
		p_proc->task.memmap.vpage_lin_limit = VpageLinBase;					//保留内存界限
		p_proc->task.memmap.heap_lin_base = HeapLinBase;						//堆基址
		p_proc->task.memmap.heap_lin_limit = HeapLinBase;						//堆界限	
		p_proc->task.memmap.stack_lin_base = StackLinBase;						//栈基址
		p_proc->task.memmap.stack_lin_limit = StackLinBase - 0x4000;					//栈界限（使用时注意栈的生长方向）
		p_proc->task.memmap.arg_lin_base = ArgLinBase;						//参数内存基址
		p_proc->task.memmap.arg_lin_limit = ArgLinBase;						//参数内存界限
		p_proc->task.memmap.kernel_lin_base = KernelLinBase;					//内核基址
		p_proc->task.memmap.kernel_lin_limit = KernelLinBase + KernelSize;		//内核大小初始化为8M
		
		/*************************进程树信息初始化***************************************/
		p_proc->task.info.type = TYPE_PROCESS;			//当前是进程还是线程
		p_proc->task.info.real_ppid = -1;  	//亲父进程，创建它的那个进程
		p_proc->task.info.ppid = -1;			//当前父进程	
		p_proc->task.info.child_p_num = 0;	//子进程数量
		//p_proc->task.info.child_process[NR_CHILD_MAX];//子进程列表
		p_proc->task.info.child_t_num = 0;		//子线程数量
		//p_proc->task.info.child_thread[NR_CHILD_MAX];//子线程列表	
		p_proc->task.info.text_hold = 1;			//是否拥有代码
		p_proc->task.info.data_hold = 1;			//是否拥有数据
		
		
		/***************初始化PID进程页表*****************************/
		if( 0 != init_page_pte(pid) )
		{
			disp_color_str("kernel_main Error:init_page_pte",0x74);
			return -1;
		}
		//pde_addr_phy_temp = get_pde_phy_addr(pid);//获取该进程页目录物理地址	//edit by visual 2016.5.19
		
		/****************代码数据*****************************/
		//p_proc->task.regs.eip= (u32)initial;//进程入口线性地址	//deleted by mingxuan 2019-3-7
		//modified by mingxuan 2019-3-14
		switch (pid)
		{
			case INITIAL_PID:  p_proc->task.regs.eip= (u32)initial; break;
			case INITIAL1_PID: p_proc->task.regs.eip= (u32)initial1; break;
			case INITIAL2_PID: p_proc->task.regs.eip= (u32)initial2; break;
			case INITIAL3_PID: p_proc->task.regs.eip= (u32)initial3; break;
			default: break;
		}

		/****************栈（此时堆、栈已经区分，以后实验会重新规划堆的位置）*****************************/
		p_proc->task.regs.esp=(u32)StackLinBase;			//栈地址最高处	
		for( AddrLin=StackLinBase ; AddrLin>p_proc->task.memmap.stack_lin_limit ; AddrLin-=num_4K )
		{//栈
			err_temp = lin_mapping_phy(	AddrLin,//线性地址
										MAX_UNSIGNED_INT,//物理地址		//edit by visual 2016.5.19
										pid,//进程pid	//edit by visual 2016.5.19
										PG_P  | PG_USU | PG_RWW,//页目录的属性位
										PG_P  | PG_USU | PG_RWW);//页表的属性位
			if( err_temp!=0 )
			{
				disp_color_str("kernel_main Error:lin_mapping_phy",0x74);
				return -1;
			}
								
		}
		
		/***************copy registers data to kernel stack****************************/
		//copy registers data to the bottom of the new kernel stack
		//added by xw, 17/12/11
		p_regs = (char*)(p_proc + 1);
		p_regs -= P_STACKTOP;
		memcpy(p_regs, (char*)p_proc, 18 * 4);
		
		/***************some field about process switch****************************/
		p_proc->task.esp_save_int = p_regs; 			 //initialize esp_save_int, added by xw, 17/12/11
		p_proc->task.esp_save_context = p_regs - 10 * 4; //when the process is chosen to run for the first time, 
														 //sched() will fetch value from esp_save_context
		eip_context = restart_restore;
		*(u32*)(p_regs - 4) = (u32)eip_context;			//initialize EIP in the context, so the process can
														//start run. added by xw, 18/4/18
		*(u32*)(p_regs - 8) = 0x1202;	//initialize EFLAGS in the context, IF=1, IOPL=1. xw, 18/4/20
		
		/***************变量调整****************************/
		p_proc++;
		selector_ldt += 1 << 3;
	}

	//2>规定proc_table的5~8个为4个migration内核线程，用作检测负载均衡
	for(; pid<NR_INITIALS+NR_MIGRATIONS ; pid++ )
	{
		/*************基本信息*********************************/
		switch (pid)									//名称
		{
			case MIGRATION_PID: strcpy(p_proc->task.p_name,"migration"); break;
			case MIGRATION1_PID: strcpy(p_proc->task.p_name,"migration1"); break;
			case MIGRATION2_PID: strcpy(p_proc->task.p_name,"migration2"); break;
			case MIGRATION3_PID: strcpy(p_proc->task.p_name,"migration3"); break;
			default: break;
		}
		p_proc->task.pid = pid;							//pid
		//p_proc->task.stat =	READY;  				//状态 //deleted by mingxuan 2019-4-12
		p_proc->task.stat =	IDLE;  						//状态 //暂时设置为IDLE，当所有cpu完成初始化时，修改为READY //modified by mingxuan 2019-4-12
		
		/**************LDT*********************************/
		p_proc->task.ldt_sel = selector_ldt;
		memcpy(&p_proc->task.ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->task.ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		
		/**************寄存器初值**********************************/
		p_proc->task.regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		//p_proc->task.regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK; //deleted by mingxuan 2019-1-11
		p_proc->task.regs.fs	= (SELECTOR_KERNEL_FS & SA_RPL_MASK)| RPL_TASK; //modified by mingxuan 2019-1-11
																				//为了[fs:0]表示当前cpu，[fs:4]表示当前进程
		p_proc->task.regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)| RPL_TASK;
		p_proc->task.regs.eflags = 0x1202; /* IF=1, IOPL=1 */
		//p_proc->task.cr3 在页表初始化中处理
		
		/**************线性地址布局初始化**********************************///	add by visual 2016.5.4
		/**************task的代码数据大小及位置暂时是不会用到的，所以没有初始化************************************/
		p_proc->task.memmap.heap_lin_base = HeapLinBase;
		p_proc->task.memmap.heap_lin_limit = HeapLinBase;				//堆的界限将会一直动态变化
		p_proc->task.memmap.stack_child_limit = StackLinLimitMAX;		//add by visual 2016.5.27
		p_proc->task.memmap.stack_lin_base = StackLinBase;
		p_proc->task.memmap.stack_lin_limit = StackLinBase - 0x4000;		//栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小
		p_proc->task.memmap.kernel_lin_base = KernelLinBase;
		p_proc->task.memmap.kernel_lin_limit = KernelLinBase + KernelSize;	//内核大小初始化为8M		//add  by visual 2016.5.10
		
		/***************初始化PID进程页表*****************************/
		if( 0 != init_page_pte(pid) )
		{
			disp_color_str("kernel_main Error:init_page_pte",0x74);
			return -1;
		}
		
		/****************代码数据*****************************/
		p_proc->task.regs.eip= (u32)p_task->initial_eip;//进程入口线性地址		edit by visual 2016.5.4
		
		/****************栈（此时堆、栈已经区分，以后实验会重新规划堆的位置）*****************************/
		p_proc->task.regs.esp=(u32)StackLinBase;			//栈地址最高处	
		for( AddrLin=StackLinBase ; AddrLin>p_proc->task.memmap.stack_lin_limit ; AddrLin-=num_4K )
		{//栈
			err_temp = lin_mapping_phy(	AddrLin,//线性地址						//add by visual 2016.5.9
										MAX_UNSIGNED_INT,//物理地址					//edit by visual 2016.5.19
										pid,//进程pid							//edit by visual 2016.5.19
										PG_P  | PG_USU | PG_RWW,//页目录的属性位
										PG_P  | PG_USU | PG_RWW);//页表的属性位
			if( err_temp!=0 )
			{
				disp_color_str("kernel_main Error:lin_mapping_phy",0x74);
				return -1;
			}					
		}
		
		/***************copy registers data to kernel stack****************************/
		//copy registers data to the bottom of the new kernel stack
		//added by xw, 17/12/11
		p_regs = (char*)(p_proc + 1);
		p_regs -= P_STACKTOP; //P_STACKTOP就是(18 * 4)
		memcpy(p_regs, (char*)p_proc, 18 * 4);
		p_proc->task.esp_save_int = p_regs; 
		
		/***************some field about process switch****************************/
		//p_proc->task.esp_save_int = p_regs; 			 //initialize esp_save_int, added by xw, 17/12/11
		p_proc->task.esp_save_context = p_regs - 10 * 4; //when the process is chosen to run for the first time, 
														 //sched() will fetch value from esp_save_context
		eip_context = restart_restore;
		*(u32*)(p_regs - 4) = (u32)eip_context;	//initialize EIP in the context, so the process can
												//start run. added by xw, 18/4/18
		*(u32*)(p_regs - 8) = 0x1202;	        //initialize EFLAGS in the context, IF=1, IOPL=1. xw, 18/4/20
		
		/***************变量调整****************************/
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	//3>对前NR_TASKS个PCB初始化,且状态为READY(生成的进程)
	//目前是12个NR_TASKS，分别为4个migration，TestA，TestB，kb_service，hd_service，4个预留以后使用
	for(; pid<NR_INITIALS+NR_TASKS ; pid++ )
	{
		/*************基本信息*********************************/
		strcpy(p_proc->task.p_name, p_task->name);		//名称
		p_proc->task.pid = pid;							//pid
		p_proc->task.stat = READY;  						//状态
		
		/**************LDT*********************************/
		p_proc->task.ldt_sel = selector_ldt;
		memcpy(&p_proc->task.ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->task.ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		//added by mingxuan 2019-1-11
		//memcpy(&p_proc->task.ldts[2], &gdt[SELECTOR_KERNEL_FS >> 3],sizeof(DESCRIPTOR));
		//p_proc->task.ldts[2].attr1 = DA_DRW | PRIVILEGE_TASK << 5;

		
		/**************寄存器初值**********************************/
		p_proc->task.regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		//p_proc->task.regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK; //deleted by mingxuan 2019-1-11
		p_proc->task.regs.fs	= (SELECTOR_KERNEL_FS & SA_RPL_MASK)| RPL_TASK; //modified by mingxuan 2019-1-11
																				//为了[fs:0]表示当前cpu，[fs:4]表示当前进程
		p_proc->task.regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_TASK;
		p_proc->task.regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)| RPL_TASK;
		p_proc->task.regs.eflags = 0x1202; /* IF=1, IOPL=1 */
		//p_proc->task.cr3 在页表初始化中处理
		
		
		/**************线性地址布局初始化**********************************///	add by visual 2016.5.4
		/**************task的代码数据大小及位置暂时是不会用到的，所以没有初始化************************************/
		p_proc->task.memmap.heap_lin_base = HeapLinBase;
		p_proc->task.memmap.heap_lin_limit = HeapLinBase;				//堆的界限将会一直动态变化
		p_proc->task.memmap.stack_child_limit = StackLinLimitMAX;		//add by visual 2016.5.27
		p_proc->task.memmap.stack_lin_base = StackLinBase;
		p_proc->task.memmap.stack_lin_limit = StackLinBase - 0x4000;		//栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小
		p_proc->task.memmap.kernel_lin_base = KernelLinBase;
		p_proc->task.memmap.kernel_lin_limit = KernelLinBase + KernelSize;	//内核大小初始化为8M		//add  by visual 2016.5.10
		
		/***************初始化PID进程页表*****************************/
		if( 0 != init_page_pte(pid) )
		{
			disp_color_str("kernel_main Error:init_page_pte",0x74);
			return -1;
		}
		
		/****************代码数据*****************************/
		p_proc->task.regs.eip= (u32)p_task->initial_eip;//进程入口线性地址		edit by visual 2016.5.4
		
		/****************栈（此时堆、栈已经区分，以后实验会重新规划堆的位置）*****************************/
		p_proc->task.regs.esp=(u32)StackLinBase;			//栈地址最高处	
		for( AddrLin=StackLinBase ; AddrLin>p_proc->task.memmap.stack_lin_limit ; AddrLin-=num_4K )
		{//栈
			err_temp = lin_mapping_phy(	AddrLin,//线性地址						//add by visual 2016.5.9
										MAX_UNSIGNED_INT,//物理地址					//edit by visual 2016.5.19
										pid,//进程pid							//edit by visual 2016.5.19
										PG_P  | PG_USU | PG_RWW,//页目录的属性位
										PG_P  | PG_USU | PG_RWW);//页表的属性位
			if( err_temp!=0 )
			{
				disp_color_str("kernel_main Error:lin_mapping_phy",0x74);
				return -1;
			}					
		}
		
		/***************copy registers data to kernel stack****************************/
		//copy registers data to the bottom of the new kernel stack
		//added by xw, 17/12/11
		p_regs = (char*)(p_proc + 1);
		p_regs -= P_STACKTOP; //P_STACKTOP就是(18 * 4)
		memcpy(p_regs, (char*)p_proc, 18 * 4);
		p_proc->task.esp_save_int = p_regs; 
		
		/***************some field about process switch****************************/
		//p_proc->task.esp_save_int = p_regs; 			 //initialize esp_save_int, added by xw, 17/12/11
		p_proc->task.esp_save_context = p_regs - 10 * 4; //when the process is chosen to run for the first time, 
														 //sched() will fetch value from esp_save_context
		eip_context = restart_restore;
		*(u32*)(p_regs - 4) = (u32)eip_context;	//initialize EIP in the context, so the process can
												//start run. added by xw, 18/4/18
		*(u32*)(p_regs - 8) = 0x1202;	        //initialize EFLAGS in the context, IF=1, IOPL=1. xw, 18/4/20
		
		/***************变量调整****************************/
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}
	
	//4>对用户PCB初始化,(名称,pid,stat,LDT选择子),状态为IDLE
	for( 	; pid<NR_PCBS ; pid++ )
	{
		/*************基本信息*********************************/
		strcpy(p_proc->task.p_name, "USER");		//名称
		p_proc->task.pid = pid;							//pid
		p_proc->task.stat = IDLE;  						//状态
		
		/**************LDT*********************************/
		p_proc->task.ldt_sel = selector_ldt;
		memcpy(&p_proc->task.ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[0].attr1 = DA_C | PRIVILEGE_USER << 5;
		memcpy(&p_proc->task.ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],sizeof(DESCRIPTOR));
		p_proc->task.ldts[1].attr1 = DA_DRW | PRIVILEGE_USER << 5;
		
		/**************寄存器初值**********************************/
		p_proc->task.regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_USER;
		p_proc->task.regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_USER;
		p_proc->task.regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_USER;
		//p_proc->task.regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_USER;	//deleted by mingxuan 2019-2-27
		p_proc->task.regs.fs	= (SELECTOR_KERNEL_FS & SA_RPL_MASK)| RPL_USER; //modified by mingxuan 2019-1-11
																				//为了[fs:0]表示当前cpu，[fs:4]表示当前进程	
		p_proc->task.regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)| SA_TIL | RPL_USER;
		p_proc->task.regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)| RPL_USER;
		p_proc->task.regs.eflags = 0x0202; /* IF=1, 倒数第二位恒为1 */
		
		/****************页表、代码数据、堆栈*****************************/
		//无
		
		/***************copy registers data to kernel stack****************************/
		//copy registers data to the bottom of the new kernel stack
		//added by xw, 17/12/11
		p_regs = (char*)(p_proc + 1);
		p_regs -= P_STACKTOP;
		memcpy(p_regs, (char*)p_proc, 18 * 4);
		
		/***************some field about process switch****************************/
		p_proc->task.esp_save_int = p_regs; //initialize esp_save_int, added by xw, 17/12/11
		//p_proc->task.save_type = 1;
		p_proc->task.esp_save_context = p_regs - 10 * 4; //when the process is chosen to run for the first time, 
														//sched() will fetch value from esp_save_context
		eip_context = restart_restore;
		*(u32*)(p_regs - 4) = (u32)eip_context;			//initialize EIP in the context, so the process can
														//start run. added by xw, 18/4/18
		*(u32*)(p_regs - 8) = 0x1202;	//initialize EFLAGS in the context, IF=1, IOPL=1. xw, 18/4/20
		
		/***************变量调整****************************/
		p_proc++;
		selector_ldt += 1 << 3;
	}

	//modified by mingxuan 2019-3-5
	/* //deleted by mingxuan 2019-3-13
	proc_table[0].task.ticks = proc_table[0].task.base_ticks = 1;	
	proc_table[1].task.ticks = proc_table[1].task.base_ticks = 1;		
	proc_table[2].task.ticks = proc_table[2].task.base_ticks = 1;
	proc_table[3].task.ticks = proc_table[3].task.base_ticks = 1;	//added by xw, 18/8/27
	proc_table[4].task.ticks = proc_table[4].task.base_ticks = 1; //NR_TASKS+0为cpu0的initial进程 //added by mingxuan 2019-2-27
	proc_table[5].task.ticks = proc_table[5].task.base_ticks = 1; //NR_TASKS+1为cpu1的initial进程 //added by mingxuan 2019-2-27
	*/

	//modified by mingxuan 2019-3-13
	/* //deleted by mingxuan 2019-4-11
	int i;
	for(i=0; i<NR_TASKS; i++)
	{
		proc_table[i].task.ticks = proc_table[i].task.base_ticks = 1;
	}
	for(; i<NR_TASKS + NCPU; i++)
	{
		proc_table[i].task.ticks = proc_table[i].task.base_ticks = 1;
	}
	*/

	//modified by mingxuan 2019-4-11
	int i;
	for(i=0; i<NR_INITIALS; i++)
	{
		proc_table[i].task.ticks = proc_table[i].task.base_ticks = 1;
	}
	for(; i<NR_INITIALS+NR_TASKS; i++)
	{
		proc_table[i].task.ticks = proc_table[i].task.base_ticks = 1;
	}

	/* When the first process begin running, a clock-interruption will happen immediately.
	 * If the first process's initial ticks is 1, it won't be the first process to execute its
	 * user code. Thus, it's will look weird, for proc_table[0] don't output first.
	 * added by xw, 18/4/19
	 */
	proc_table[0].task.ticks = 2;
	
	//proc_table[1].task.ticks = 2;	//proc_table[0]是cpu0的first process，proc_table[1]是cpu1的first process

	balance_ready_proc(); //将进程平均分发给不同的CPU //added by mingxuan 2019-2-27
	//balance_ready_proc(); //将系统进程和initial平均分发给不同的CPU，用户进程暂不做分发 //added by mingxuan 2019-2-27
	
	return 0;
}

/*======================================================================*
                              schedule
 *======================================================================*/
/*	//deleted by mingxuan 2019-1-20
PUBLIC void schedule()
{
	PROCESS* p;
	int	 greatest_ticks = 0;
	
	//deleted by mingxuan 2019-1-11
	//if (p_proc_current->task.stat == READY && p_proc_current->task.ticks > 0) {		
	//	p_proc_next = p_proc_current;	//added by xw, 18/4/26
	//	return;
	//}

	//acquire(&ptlock);	//added by mingxuan 2019-1-16

	//modified by mingxuan 2019-1-11
	if (proc->task.stat == READY && proc->task.ticks > 0) {		
		p_proc_next = proc;

		//release(&ptlock);	//added by mingxuan 2019-1-17

		return;
	}

	while (!greatest_ticks) 
	{
		for (p = proc_table; p < proc_table+NR_PCBS; p++)		//edit by visual 2016.4.5
		{
			if (p->task.stat == READY && p->task.ticks > greatest_ticks)  //edit by visual 2016.4.5
			{
				greatest_ticks = p->task.ticks;
				// p_proc_current = p;
				p_proc_next	= p;	//modified by xw, 18/4/26
			}

		}

		if (!greatest_ticks) 
		{
			for (p = proc_table; p < proc_table+NR_PCBS; p++) //edit by visual 2016.4.5
			{
				p->task.ticks = p->task.priority;
			}
		}
	
	}

	//release(&ptlock);	//added by mingxuan 2019-1-16
}
*/

//以后实现各种CPU负载均衡算法
//added by mingxuan 2019-1-24
PUBLIC void balance_ready_proc()
{
	PROCESS *p;
	struct cpu *c;	//added by mingxuan 2019-3-13

	/* //deleted by mingxuan 2019-3-13
	struct cpu *c0;
	struct cpu *c1;

	c0 = cpulist;
	c1 = cpulist + 1;
	*/

	/*	//deleted by mingxuan 2019-3-6
	int i=0,j=0;
	for (p = proc_table; p < proc_table+NR_TASKS+NR_CPUS; p++)	//仅分配TestA~D和两个initial
	{
		if(0 == (p->task.pid % 2))  //pid为偶数，放入cpu0
      		c0->ready_proc[i++] = p->task.pid;
    	else //pid为奇数，放入cpu1
      		c1->ready_proc[j++] = p->task.pid;
  	}
	*/

	//modified by mingxuan 2019-3-6
	//暂时先全部分配给cpu0
	
	/*//deleted by mingxuan 2019-4-11
	c = cpulist;
	for (p = proc_table; p < proc_table+NR_TASKS; p++)	//分配TestA~C+hd_service
	{
      	//c->ready_proc[i++] = p->task.pid;	//deleted by mingxuan 2019-4-3
		add_ready_queue(&c->ready, p);	//modified by mingxuan 2019-4-3
		c->nr_ready_proc++;
  	}
	*/
	
	/* //deleted by mingxuan 2019-4-3
	c->ready_proc[INITIAL_PID] = INITIAL_PID;
	//c0->ready_proc[4] = INITIAL_PID;  // 0x5是cpu0的initial()

	//modified by mingxuan 2019-3-14
	for(c = cpulist + 1; c < cpulist + cpunumber; c++)
	{	
		if(1 == c->id) 		c->ready_proc[0] = INITIAL1_PID;
		else if(2 == c->id) c->ready_proc[0] = INITIAL2_PID;
		else if(3 == c->id) c->ready_proc[0] = INITIAL3_PID;
	}	
	//分配cpu0和cpu1各自的initial进程
	//c0->ready_proc[4] = INITIAL_PID;  //deleted by mingxuan 2019-3-13
	//c1->ready_proc[0] = INITIAL1_PID; //deleted by mingxuan 2019-3-13 
	*/

	/* //deleted by mingxuan 2019-4-11
	//接下来分配各自的initial进程
	for (; p < proc_table+NR_TASKS+NCPU, c < cpulist+NCPU; p++,c++)
	{
		add_ready_queue(&c->ready, p);
		c->nr_ready_proc++;
	}
	*/

	//modified by mingxuan 2019-4-11
	//首先分配各自的initial进程
	for (c = cpulist, p = proc_table; p < proc_table+NR_INITIALS, c < cpulist+NCPU; p++, c++)
	{
		add_ready_queue(&c->ready, p);
		c->nr_ready_proc++;
	}
	//接下来分配各自的migration内核线程
	for (c = cpulist; p < proc_table+NR_INITIALS+NR_MIGRATIONS, c < cpulist+NCPU; p++, c++) //NCPU也是migration内核线程的个数
	{
		add_ready_queue(&c->ready, p);
		c->nr_ready_proc++;
	}
	//接下TestA，TestB，kb_service，hd_service，这4个内核线程假设先都分给cpu0
	for (c = cpulist; p < proc_table+NR_INITIALS+NR_MIGRATIONS+4; p++) //4的含义是TestA，TestB，kb_service，hd_service，这4个内核线程
	{
		add_ready_queue(&c->ready, p);	//modified by mingxuan 2019-4-3
		c->nr_ready_proc++;
	}


}

//每颗CPU在自己的就绪队列ready_proc中搜索，代替已有的sched_next
//added by mingxuan 2019-1-24
PUBLIC PROCESS *cpu_schedule(void)
{

	//balance_ready_proc();	//每次调度都要进行一次负载均衡 //deleted by mingxuan 2019-2-27

	PROCESS* p;
	PROCESS* p_return;       //只是一个中间变量而已 //added by mingxuan 2019-1-20
	int  greatest_ticks = 0; //控制while循环，在选中一个进程后，退出while循环

	//此处引入两个cpu局部变量,仅仅用于方便gdb查看ready_proc
	//added by mingxuan 2019-2-28
	struct cpu *c = cpu;
	int cpuid = cpu->id;

	PROCESS *p_proc = proc; //加入*p这个局部变量，仅仅用于方便gdb查看当前进程proc

  	//决定是否继续运行当前的进程，当前进程的ticks＞0时，仍然选中当前进程
 	if (proc->task.stat == READY && proc->task.ticks > 0)
	{  
		p = proc;         		// by Gu
    	return (PROCESS *)p;    // by Gu
	}

	//acquire(&ptlock);	//added by mingxuan 2019-3-14

  	int i; //每颗cpu遍历自己的ready_proc时的循环变量
  	while(!greatest_ticks)
  	{ 
    	//每颗cpu遍历自己的ready_proc
		/*	//deleted by mingxuan 2019-4-3
		for(i=0; i<N_READY_PROC; i++)//循环比较谁的ticks最大，选中ticks最大的进程
    	{
      		p = proc_table + cpu->ready_proc[i];	//added by mingxuan 2019-1-24

      		if (p->task.stat == READY && p->task.ticks > greatest_ticks)  //edit by visual 2016.4.5
      		{ 
        		greatest_ticks = p->task.ticks;
        		p_return = p; //只是一个中间变量而已  //added by mingxuan 2019-1-20
     		}
    	}
		*/
		struct ready_proc *p_mov = cpu->ready.front;
		while(p_mov != NULL)
    	{
      		//p = proc_table + cpu->ready_proc[i];	//added by mingxuan 2019-1-24
			p = p_mov->proc;

      		if (p->task.stat == READY && p->task.ticks > greatest_ticks)  //edit by visual 2016.4.5
      		{ 
        		greatest_ticks = p->task.ticks;
        		p_return = p; //只是一个中间变量而已  //added by mingxuan 2019-1-20
     		}

			p_mov = p_mov->next;
    	}	

		//每颗cpu遍历自己的ready_proc后，发现所有进程的ticks都为0
    	//当所有的进程的ticks都变为0之后，再把各自的ticks赋值成各自的priority
    	if (!greatest_ticks) 
    	{
			/* //deleted by mingxuan 2019-4-3
      		for(i=0; i<N_READY_PROC; i++)
      		{
        		p = proc_table + cpu->ready_proc[i];  //added by mingxuan 2019-1-24
        		//p->task.ticks = p->task.priority;	  //deleted by mingxuan 2019-3-5
				p->task.ticks = p->task.base_ticks;
      		}
			*/
			struct ready_proc *p_mov = cpu->ready.front;
			while(p_mov != NULL)
			{
				p = p_mov->proc;
				p->task.ticks = p->task.base_ticks;

				p_mov = p_mov->next;
			}

    	}	
	}

	//release(&ptlock);	//added by mingxuan 2019-3-14

  return (PROCESS  *)p_return;
}

/* //deleted by mingxuan 2019-2-25
//以后实现各种调度算法，就是实现这个函数，返回选中进程PCB的指针		by Gu
//added by mingxuan 2019-1-20
PUBLIC PROCESS *sched_next()
{
	PROCESS* p;
	PROCESS* p_return; //只是一个中间变量而已	//added by mingxuan 2019-1-20

	int	 greatest_ticks = 0; //控制while循环，在选中一个进程后，退出while循环

	//当前进程的ticks＞0时，仍然选中当前进程
	if (proc->task.stat == READY && proc->task.ticks > 0) {	
		//p->task.stat = RUNNING;	//added by mingxuan 2019-1-24	
		p = proc;  				// by Gu
		return (PROCESS  *)p;   // by Gu
	}

	
	//当前进程的ticks=0时，查找其他进程
	while (!greatest_ticks) 
	{
		//deleted by mingxuan 2019-1-24
		for (p = proc_table; p < proc_table+NR_PCBS; p++)				  //edit by visual 2016.4.5
		{
			//当查找到一个ticks最大的进程
			if (p->task.stat == READY && p->task.ticks > greatest_ticks)  //edit by visual 2016.4.5
			{	
				greatest_ticks = p->task.ticks;
				// p_proc_current = p;	//modified by xw, 18/4/26
				// p_proc_next	= p;	//deleted by mingxuan 2019-1-20
				p->task.stat = RUNNING;	//added by mingxuan 2019-1-24
				p_return = p;	//只是一个中间变量而已	//added by mingxuan 2019-1-20

			}
		}	

		//当所有的进程的ticks都变为0之后，再把各自的ticks赋值成各自的priority
		if (!greatest_ticks) 
		{
			for (p = proc_table; p < proc_table+NR_PCBS; p++) //edit by visual 2016.4.5
			{
				p->task.ticks = p->task.priority;
				//p->task.stat = READY;	//added by mingxuan 2019-1-24
			}
		}
	}

	return (PROCESS  *)p_return;   // by Gu
}
*/

//added by mingxuan 2019-1-20
//PUBLIC void new_schedule()	//deleted by mingxuan 2019-3-5
PUBLIC void schedule()	//modified by mingxuan 2019-3-5
{
	//acquire(&ptlock);	//added by mingxuan 2019-1-20 //deleted by mingxuan 2019-3-13
	
	//p_proc_next = sched_next();	//deleted by mingxuan 2019-1-24
	//每颗CPU在自己的就绪队列ready_proc中搜索
	p_proc_next = cpu_schedule();	//modified by mingxuan 2019-1-24

	//deleted by mingxuan 2019-3-7
	//以下两句在此处无法处理一颗CPU上只有一个进程在调度的情形
	//if(proc->task.stat == RUNNING) 
	//	proc->task.stat = READY;

  	p_proc_next ->task.stat = RUNNING;
	
	//也可以适应如果一颗CPU上只有一个进程在调度
	//modified by mingxuan 2019-3-7
	if(proc->task.stat == RUNNING) 
	//if(proc->task.stat == RUNNING || SLEEPING) 
		proc->task.stat = READY;

  	proc = p_proc_next;
  	
	//release(&ptlock);	//added by mingxuan 2019-1-20 //deleted by mingxuan 2019-3-13
  	return;
}

/*======================================================================*
                           alloc_PCB  add by visual 2016.4.8
 *======================================================================*/
PUBLIC PROCESS* alloc_PCB()
{//分配PCB表
	 PROCESS* p;
	 int i;

	 acquire(&allocPCBlock);	//added by mingxuan 2019-3-14

	 //p = proc_table + NR_K_PCBS;	//跳过前NR_K_PCBS个 //deleted by mingxuan 2019-3-14
	 p = proc_table + NR_TASKS + NR_INITIALS; //跳过前NR_TASKS + NR_INITIALS个
	 //for(i=NR_K_PCBS; i<NR_PCBS; i++)
	 for(i=NR_TASKS + NR_INITIALS; i<NR_PCBS; i++) //前NR_TASKS + NR_INITIALS个一定是READY状态
	 {
	 	if(p->task.stat==IDLE) break;
		p++;	
	 }

	 if(i>=NR_PCBS)	return 0;   //NULL
	 else	return p;
}


/*======================================================================*
                           free_PCB  add by visual 2016.4.8
 *======================================================================*/
PUBLIC void free_PCB(PROCESS *p)
{//释放PCB表
	p->task.stat=IDLE;
}

/*======================================================================*
        对就绪队列的操作
	added by mingxuan 2019-4-3
 *======================================================================*/
//added by mingxuan 2019-4-3
void init_ready_queue(struct ready_queue *rq)
{
	rq->front = rq->rear = NULL;
}

//迁移到目的cpu就绪队列的队尾入队
//added by mingxuan 2019-4-3
void add_ready_queue(struct ready_queue *rq, PROCESS *p_proc)
{
	struct ready_proc *p;
	p = (struct ready_proc *)K_PHY2LIN(do_kmalloc(sizeof(struct ready_proc)));

	p->proc = p_proc;
	p->next = NULL;
	p->prev = NULL; //added by mingxuan 2019-4-12

	if(rq->rear == NULL) //原先的队列为空
		rq->front = rq->rear = p;
	else 
	{
		p->prev = rq->rear;	//added by mingxuan 2019-4-12
		rq->rear->next = p;
		rq->rear = p;
	}
}

//从cpu->ready_queue的给定位置出队
//added by mingxuan 2019-4-12
void add_ready_queue_index(struct ready_queue *rq, PROCESS *p_proc, int index)
{
	struct ready_proc *p;
	p = (struct ready_proc *)K_PHY2LIN(do_kmalloc(sizeof(struct ready_proc)));

	p->proc = p_proc;
	p->next = NULL;
	p->prev = NULL;	

	if(rq->rear == NULL) //原先的队列为空
		rq->front = rq->rear = p;
	else 
	{
		struct ready_proc *p_mov = rq->front;
		int cnt = 0;
		
		while(p_mov != NULL)
    	{
			if(cnt == (index-1))
			{
				p->next = p_mov->next;
				p->prev = p_mov;
				p_mov->next->prev = p;
				p_mov->next = p;
				break;
			}	
			p_mov = p_mov->next;
			cnt++;
    	}	
	}
}


//从源cpu->ready_queue的第一个位置出队(该函数的语法还需调整，暂时用不到)
//added by mingxuan 2019-4-3
int remove_ready_queue(struct ready_queue *rq, struct ready_proc **p)
{
	if(rq->rear == NULL)
		return 0;	//empty

	//*p = rq->front;
	if(rq->front == rq->rear) //only one node
		rq->front = rq->rear = NULL;
	else
		rq->front = rq->front->next;
	
	do_free(p);

	return 1;
}

//从cpu->ready_queue的给定位置出队
//added by mingxuan 2019-4-12
int remove_ready_queue_index(struct ready_queue *rq, struct ready_proc **p, int index) //index从0开始
{
	if(rq->rear == NULL)
		return 0;	//empty

	if(rq->front == rq->rear) //only one node
	{
		rq->front = rq->rear = NULL;
		do_free(p); //暂时先不考虑free
	}
	else
	{
		struct ready_proc *p_mov = rq->front;
		int cnt = 0;
		
		while(p_mov != NULL)
    	{
			if(cnt == index)
			{
				p_mov->prev->next = p_mov->next;
				p_mov->next->prev = p_mov->prev;
				break;
			}	
			p_mov = p_mov->next;
			cnt++;
    	}	
		do_free(p);
	}
	
}


/**********************************************************
*				insert_cpu_ready_proc		//added by mingxuan 2019-2-28
*将子进程挂载到和父进程同一个CPU的就绪队列上
*************************************************************/
/*
PUBLIC int insert_cpu_ready_proc(u32 pid, u32 cpuid)
{
	struct cpu *c;
	extern cpunumber;
	int i; //每颗cpu遍历自己的ready_proc时的循环变量
	
	//acquire(&insert_ready_proc_lock); //added by mingxuan 2019-3-14
	for(c = cpulist; c < cpulist + cpunumber; c++) //遍历cpu列表
	{	
		if(cpuid == c->id)
		{
			for(i=0; i<N_READY_PROC; i++) //每颗cpu遍历自己的ready_proc
			{
				if(-1 == c->ready_proc[i]) //该位置为空
				{
					c->ready_proc[i] = pid;
					break;
				}
			}
		}
	}
	//release(&insert_ready_proc_lock); //added by mingxuan 2019-3-14
}
*/


/*======================================================================*
                           yield and sleep
 *======================================================================*/
//used for processes to give up the CPU
PUBLIC void sys_yield()
{
	//disp_int(cpu->id);
	//disp_str(" ");
	//p_proc_current->task.ticks--;

	//acquire(&yieldlock);

	//p_proc_current->task.ticks = 0;	//deleted by mingxuan 2019-1-13
	proc->task.ticks = 0; 				//modified by mingxuan 2019-1-13
	
	//save_context();
	sched();	//Modified by xw, 18/4/19	//deleted by mingxuan 2019-1-25
	//暂时删除sched，防止yeild中的sched和restart_syscall中的sched混淆，给调试带来困难 mingxuan

	//release(&yieldlock);
}

//used for processes to sleep for n ticks
PUBLIC void sys_sleep(int n)
{
	int ticks0;
	
	ticks0 = ticks;
	//p_proc_current->task.channel = &ticks;	//deleted by mingxuan 2019-1-16
	proc->task.channel = &ticks;				//modified by mingxuan 2019-1-16
	
	while(ticks - ticks0 < n){
		//p_proc_current->task.stat = SLEEPING;	//deleted by mingxuan 2019-1-16
		proc->task.stat = SLEEPING;				//modified by mingxuan 2019-1-16

		//save_context();
		sched();	//Modified by xw, 18/4/19
	}
}

/*invoked by clock-interrupt handler to wakeup 
 *processes sleeping on ticks.
 */
PUBLIC void sys_wakeup(void *channel)
{
	PROCESS *p;
	
	//acquire(&wakeuplock);	//added by mingxuan 2019-3-5

	for(p = proc_table; p < proc_table + NR_PCBS; p++){
		if(p->task.stat == SLEEPING && p->task.channel == channel){
			p->task.stat = READY;
		}
	}

	//release(&wakeuplock);	//added by mingxuan 2019-3-5
}

//根据LDT中描述符的索引来求得描述符所指向的段的基地址
//added by zcr
PUBLIC int ldt_seg_linear(PROCESS *p, int idx)
{
	struct s_descriptor * d = &p->task.ldts[idx];
	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

/*****************************************************************************
 *				  va2la
 *****************************************************************************/
/**
 * @return The linear address for the given virtual address.
 *****************************************************************************/
PUBLIC void* va2la(int pid, void* va)
{
	if(kernel_initial == 1){
		return va;
	}
	
	PROCESS* p = &proc_table[pid];
	u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va; //线性地址=段基址+虚拟地址
	
	return (void*)la;
}
//~zcr


/**********************************************************
*				load_balance		//added by mingxuan 2019-4-3
*就绪队列的简单负载均衡
*************************************************************/

struct cpu *find_busiest_cpu()
{
	struct cpu *c;
	struct cpu *busiest_c;
	int load, max_load = 0;
	
	//遍历cpulist，找出load最大的cpu，即就绪队列中进程个数最多的cpu
	for(c = cpulist; c < cpulist + NCPU; c++)
	{
		load = c->nr_ready_proc; //当前就绪队列中进程的个数

		if (load > max_load) 
		{
			max_load = load;
			busiest_c = c;		
		}
	}

	return busiest_c;
}

int move_proc_num(struct cpu *des_c, struct cpu *src_c)
{
	return (src_c->nr_ready_proc - des_c->nr_ready_proc) / 2;
}


//从源cpu就绪队列的队头出队，迁移到目的cpu就绪队列的队尾入队
//特别注意：每颗CPU的就绪队列的前3个proc分别为0#、migration、init，这3个不做移动
void move_ready_proc(struct cpu *des_c, struct cpu *src_c)
{
	struct ready_proc *p;
	int i;

	//每颗cpu都是从第3个以后开始考虑负载均衡
	p = src_c->ready.front; 
	for(i=0; i<NR_STABLE_PROC; i++)
		p = p->next;

	//从源cpu就绪队列的队头出队
	//remove_ready_queue(&src_c->ready, &p);
	//remove_ready_queue_index(&src_c->ready, &p, NR_STABLE_PROC); //modified by mingxuan 2019-4-12

	p->prev->next = p->next;
	p->next->prev = p->prev;

	src_c->nr_ready_proc--;

	p->prev = des_c->ready.rear;
	p->next = NULL;
	des_c->ready.rear->next = p;
	des_c->ready.rear = p;

	//迁移到目的cpu就绪队列的队尾入队
	//add_ready_queue(&des_c->ready, p);
	//add_ready_queue_index(&des_c->ready, p, NR_STABLE_PROC); //modified by mingxuan 2019-4-12
	des_c->nr_ready_proc++;

	/* //deleted by mingxuan 2019-5-7
	disp_color_str("[",0x70);
	disp_color_str("Move", 0x70);
	disp_int(p->proc->task.pid);
	disp_color_str("form", 0x70);
	disp_int(src_c->id);
	disp_color_str("to", 0x70);
	disp_int(des_c->id);
	disp_color_str("]",0x70);
	*/

}


//第一步：从cpulist中找出最繁忙的一个cpu(繁忙程度暂时用就绪队列中的进程个数来衡量，后期可再优化)
//第二步：从该cpu上迁移几个进程到本cpu上(迁移个数暂时是取两颗cpu就绪队列中进程数的平均值，后期可再优化)
void load_balance()
{
	struct cpu *busiest_cpu;
	int nr_mov_proc;		 //迁移进程的个数
	int i;

	acquire(&load_balance_lock); //added by mingxuan 2019-4-3

	busiest_cpu = find_busiest_cpu();
	if(busiest_cpu == cpu) //如果最繁忙的是自己
	{
		release(&load_balance_lock); //added by mingxuan 2019-4-3
		return;
	}

	nr_mov_proc = move_proc_num(cpu, busiest_cpu); //计算应该迁移多少个进程
	for(i=0 ;i < nr_mov_proc; i++)
	{
		move_ready_proc(cpu, busiest_cpu);
	}

	release(&load_balance_lock); //added by mingxuan 2019-4-3

	return;
}