
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* 
 * To make things more direct. In the headers below, if a variable declaration has a EXTERN prefix,
 * the variable will be defined here.
 * added by xw, 18/6/17
 */
// #define GLOBAL_VARIABLES_HERE
#include "const.h"
#undef	EXTERN	//EXTERN could have been defined as extern in const.h
#define	EXTERN	//redefine EXTERN as nothing
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "spinlock.h"	//added by mingxuan 2019-1-16
#include "semaphore.h"  //added by mingxuan 2019-3-28

/* save the execution environment of each cpu, which doesn't belong to any process.
 * added by xw, 18/6/1
 */
//PUBLIC	PROCESS			cpu_table[NR_CPUS];		  //deleted by mingxuan 2019-2-27
PUBLIC	PROCESS			proc_table[NR_PCBS];		  //edit by visual 2016.4.5	

//PUBLIC	char		task_stack[STACK_SIZE_TOTAL]; //delete  by viusal 2016.4.28

/* //deleted by mingxuan 2019-4-11
PUBLIC	TASK			task_table[NR_TASKS] = {{TestA, STACK_SIZE_TASK, "TestA"},				
												{TestB, STACK_SIZE_TASK, "TestB"},	
												{task_tty, STACK_SIZE_TTY, "tty"},	
												{hd_service, STACK_SIZE_TASK, "hd_service"}};	//deleted by mingxuan 2019-1-11
*/

//modified by mingxuan 2019-4-11
PUBLIC	TASK			task_table[NR_TASKS] = {//pid=0x4~0x7，用作每颗cpu执行load_balance
												{migration, STACK_SIZE_TASK, "migration"},	
												{migration1, STACK_SIZE_TASK, "migration1"},  
												{migration2, STACK_SIZE_TASK, "migration2"},
												{migration3, STACK_SIZE_TASK, "migration3"},

												//pid=0x8~0xB
												{TestA, STACK_SIZE_TASK, "TestA"},				
												{TestB, STACK_SIZE_TASK, "TestB"},	
												{task_tty, STACK_SIZE_TTY, "tty"},	
												{hd_service, STACK_SIZE_TASK, "hd_service"},

												//pid=0xC~0xF，预留位置以后用
												{Task, 0, "TASK9"},
												{Task, 0, "TASK10"},
												{Task, 0, "TASK11"},
												{Task, 0, "TASK12"}};	


PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {	sys_get_ticks, 									//1st
														sys_get_pid,		//add by visual 2016.4.6
														sys_kmalloc,		//add by visual 2016.4.6 
														sys_kmalloc_4k,		//add by visual 2016.4.7
														sys_malloc,			//add by visual 2016.4.7	//5th
														sys_malloc_4k,		//add by visual 2016.4.7 
														sys_free,			//add by visual 2016.4.7 
														sys_free_4k,		//add by visual 2016.4.7 
														sys_fork,			//add by visual 2016.4.8 
														sys_pthread,		//add by visual 2016.4.11	//10th
														sys_udisp_int,		//add by visual 2016.5.16 
														sys_udisp_str,		//add by visual 2016.5.16
														sys_exec,			//add by visual 2016.5.16
														sys_yield,			//added by xw
													    sys_sleep,			//added by xw				//15th
													    sys_print_E,		//added by xw
													    sys_print_F,		//added by xw
														sys_open,			//added by xw, 18/6/18
													    sys_close,			//added by xw, 18/6/18
													    sys_read,			//added by xw, 18/6/18		//20th
													    sys_write,			//added by xw, 18/6/18
													    sys_lseek,			//added by xw, 18/6/18
														sys_unlink,			//added by xw, 18/6/19		//23th
														sys_get_cpuid,		//added by mingxuan 2019-3-1
														
														sys_msgsnd,			//added by mingxuan 2019-5-13 //25th
														sys_msgrcv, 		//added by mingxuan 2019-5-13
														sys_msgget, 		//added by mingxuan 2019-5-13
														sys_msgctl,			//added by mingxuan 2019-5-13

														sys_boxget, 		//added by mingxuan 2019-5-14 
														sys_boxdel,			//added by mingxuan 2019-5-14 //30th
														sys_boxsnd, 		//added by mingxuan 2019-5-14
														sys_boxrcv  		//added by mingxuan 2019-5-14 
														};//更新sys_call_table后牢记也要更新NR_SYS_CALL


PUBLIC 	struct spinlock ptlock;	 //proc_table锁  //added by mingxuan 2019-1-16
PUBLIC	struct spinlock cr3lock; //cr3_ready锁	 //added by mingxuan 2019-1-21

PUBLIC 	struct spinlock	allocPCBlock; //对alloc_PCB这一公共过程加锁	//added by mingxuan 2019-3-5

/* 和文件系统有关的锁 */
PUBLIC  struct spinlock inode_table_lock;    //added by mingxuan 2019-3-20
PUBLIC  struct spinlock f_desc_table_lock;   //added by mingxuan 2019-3-20
PUBLIC  struct spinlock super_block_lock;	 //added by mingxuan 2019-3-21
PUBLIC	struct spinlock alloc_imap_bit_lock; //added by mingxuan 2019-3-20
PUBLIC	struct spinlock alloc_smap_bit_lock; //added by mingxuan 2019-3-20
PUBLIC	struct spinlock	sync_inode_lock;	//added by mingxuan 2019-3-25

PUBLIC  struct spinlock load_balance_lock;	//added by mingxuan 2019-4-3

/*
PUBLIC  struct spinlock	execlock;	//added by mingxuan 2019-3-7
PUBLIC  struct spinlock forklock;   //added by mingxuan 2019-3-12
PUBLIC  struct spinlock pthreadlock; //added by mingxuan 2019-3-14
*/

/* 信号量 */
PUBLIC  struct semaphore inode_table_sem; //added by mingxuan 2019-3-28