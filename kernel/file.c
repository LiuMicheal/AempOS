/********************************************
*    file.c 	//add by visual 2016.5.17
*目前是虚拟的文件读写
***********************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "cpu.h"	//added by mingxuan 2019-3-7


#define	BaseOfEchoFilePhyAddr	(K_PHY2LIN(0x7e00))   //目前就这一个文件
#define BaseOfInit1FilePhyAddr	(K_PHY2LIN(0x8e00))	  //added by mingxuan 2019-3-7
#define BaseOfInit2FilePhyAddr	(K_PHY2LIN(0x9e00))	  //added by mingxuan 2019-3-7
#define BaseOfInit3FilePhyAddr	(K_PHY2LIN(0xae00))	  //added by mingxuan 2019-3-7

static u32 position=0;

/*****************************************************
*				open 		//add by visual 2016.5.17
*目前没有什么用的open
******************************************************/
//PUBLIC u32 open(char* path,char* mode)
PUBLIC u32 fake_open(char* path,char* mode)		//modified by xw, 18/5/30
{
	position = 0;
	return 0;
}


/******************************************************
*				read		//add by visual 2016.5.17
********************************************************/
//PUBLIC u32 read(u32 fd,void* buffer,u32 size)
PUBLIC u32 fake_read(u32 fd,void* buffer,u32 size)	//modified by xw, 18/5/30
{
	//u32 addr_lin = BaseOfEchoFilePhyAddr + position;	//deleted by mingxuan 2019-3-7
	
	//modified by mingxuan 2019-3-14
	u32 addr_lin=0;
	switch (cpu->id)
	{
		case 0: addr_lin = BaseOfEchoFilePhyAddr + position; break;  //cpu0的init进程
		case 1: addr_lin = BaseOfInit1FilePhyAddr + position; break; //cpu1的init1进程
		case 2: addr_lin = BaseOfInit2FilePhyAddr + position; break; //cpu2的init2进程
		case 3: addr_lin = BaseOfInit3FilePhyAddr + position; break; //cpu3的init3进程
	
		default: break;
	}

	/*	//deleted by mingxuan 2019-3-14
	//modified by mingxuan 2019-3-7
	if(0 == cpu->id)
	{
		addr_lin = BaseOfEchoFilePhyAddr + position; //cpu0的init0进程
	}
	//else if(1 == cpu->id)
	else //modified by mingxuan 2019-3-14
	{
		addr_lin = BaseOfInit1FilePhyAddr + position; //cpu1的init1进程
	}
	*/

	position += size;
	memcpy(buffer,(void*)addr_lin,size);
	return 0;
}

/******************************************************
*				seek //add by visual 2016.5.17
*******************************************************/
//PUBLIC u32 seek(u32 pos)
PUBLIC u32 fake_seek(u32 pos)	//modified by xw, 18/5/30
{
	position = pos;
	return 0;
}

