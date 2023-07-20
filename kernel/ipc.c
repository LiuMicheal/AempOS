/*****************************************************
*			ipc.c			//add by yangguang 2017.11.18
*系统调用消息队列 功能实现部分sys_msgget sys_msgsnd sys_msgrcv sys_msgctl
*系统调用信箱通信 功能实现部分sys_boxget sys_boxdel sys_boxsnd sys_boxrcv
********************************************************/
#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"   //modified by mingxuan 2019-5-13
#include "proto.h"
#include "string.h"
//#include "proc.h"
#include "global.h"
#include "cpu.h"    //added by mingxuan 2019-5-13

/**********************************************************
*		sys_msgsnd			//add by yangguang 2017.11.18
*系统调用sys_msgsnd的具体实现部分
*************************************************************/
PUBLIC int sys_msgsnd(u32 proc_esp)
{
  int iflag = Begin_Int_Atomic();
  int   msqid;
  int  *ptr;
  int   size;
  int   flag;
  int   type;
  char *buffer;

  //异常处理
  if(argint(proc_esp, 0, &msqid) < 0
    || argptr(proc_esp, 1, (void*)&ptr) < 0
    || argint(proc_esp, 2, &size) < 0
    || argint(proc_esp, 3, &flag) < 0) {
      disp_color_str_clear_screen("msgsnd: parameter failed!\n",0x74);
      End_Int_Atomic(iflag);
      return -1;
  }

  type = *ptr; 
  ptr++;
  buffer = (char*)(*ptr);
  End_Int_Atomic(iflag);
  return msg_queue_push_node(msqid, size, type, buffer, flag);
}

/**********************************************************
*		sys_msgrcv			//add by yangguang 2017.11.18
*系统调用sys_msgrcv的具体实现部分
*************************************************************/
PUBLIC int sys_msgrcv(u32 proc_esp)
{
  int iflag = Begin_Int_Atomic();
  int   msqid;
  int  *ptr;
  int   size;
  int   type;
  int   flag;
  char *buffer;
 
  //异常处理
  if(argint(proc_esp, 0, &msqid) < 0
    || argptr(proc_esp, 1, (void*)&ptr) < 0
    || argint(proc_esp, 2, &size) < 0
    || argint(proc_esp, 3, &type) < 0
    || argint(proc_esp, 4, &flag) < 0) {
      disp_color_str_clear_screen("msgrcv: parameter failed!\n",0x74);
      End_Int_Atomic(iflag);
      return -1;
  }

  ptr++;
  buffer = (char*)(*ptr);
  End_Int_Atomic(iflag);

  return msg_queue_pop_node(msqid, size, type, buffer, flag);
}

/**********************************************************
*		sys_msgget			//add by yangguang 2017.11.18
*系统调用sys_msgget的具体实现部分
*************************************************************/
PUBLIC int sys_msgget(u32 proc_esp)
{
  //disp_str("sys_msgget ");
  //while(1);

  int iflag = Begin_Int_Atomic();
  int key;

  if(argint(proc_esp, 0, &key) < 0) {
    disp_color_str_clear_screen("msgget: parameter failed!\n",0x74);
    End_Int_Atomic(iflag);
    return -1;
  }
  End_Int_Atomic(iflag);
  return alloc_msg_queue(key);
}

/**********************************************************
*		sys_msgctl			//add by yangguang 2017.11.18
*系统调用sys_msgctl的具体实现部分
*************************************************************/
PUBLIC int sys_msgctl(u32 proc_esp)
{ 
  int iflag = Begin_Int_Atomic();
  int msqid;
  int cmd;

  if(argint(proc_esp, 0, &msqid) < 0
    || argint(proc_esp, 1, &cmd) < 0) {
    disp_color_str_clear_screen("msgctl: parameter failed!\n",0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  End_Int_Atomic(iflag);
  if(cmd == IPC_RMID) {
    return free_msg_queue(msqid);
  } else return -1;
}

/**********************************************************
*		sys_boxget			// add by yangguang 2017.12.03
*系统调用sys_boxget的具体实现部分
*************************************************************/
PUBLIC int sys_boxget()
{
  //return alloc_box(p_proc_current - proc_table);
  return alloc_box(proc - proc_table); //modified by mingxuan 2019-5-13
}

/**********************************************************
*		sys_boxdel			// add by yangguang 2017.12.03
*系统调用sys_boxdel的具体实现部分
*************************************************************/
PUBLIC int sys_boxdel()
{
  //return free_box(p_proc_current - proc_table);
  return free_box(proc - proc_table); //modified by mingxuan 2019-5-13
}

/**********************************************************
*		sys_boxsnd			// add by yangguang 2017.12.03
*系统调用sys_boxsnd的具体实现部分
*************************************************************/
PUBLIC int sys_boxsnd(u32 proc_esp)
{
  int iflag = Begin_Int_Atomic();
  int    pid;
  char  *buffer;
  int    size;
  int    flag;

  if(argint(proc_esp, 0, &pid) < 0
    || argptr(proc_esp, 1, (void*)&buffer) < 0
    || argint(proc_esp, 2, &size) < 0
    || argint(proc_esp, 3, &flag) < 0) {
      disp_color_str_clear_screen("boxsnd: parameter failed!\n",0x74);
      End_Int_Atomic(iflag);
      return -1;
  }
  End_Int_Atomic(iflag);
  return box_push_node(pid, buffer, size, flag);
}

/**********************************************************
*		sys_boxsnd			// add by yangguang 2017.12.03
*系统调用sys_boxsnd的具体实现部分
*************************************************************/
PUBLIC int sys_boxrcv(u32 proc_esp)
{
  int iflag = Begin_Int_Atomic();
  int    pid;
  char  *buffer;
  int    size;
  int    flag;

  if(argint(proc_esp, 0, &pid) < 0
    || argptr(proc_esp, 1, (void*)&buffer) < 0
    || argint(proc_esp, 2, &size) < 0
    || argint(proc_esp, 3, &flag) < 0) {
      disp_color_str_clear_screen("boxrcv: parameter failed!\n",0x74);
      End_Int_Atomic(iflag);
      return -1;
  }
  End_Int_Atomic(iflag);
  return box_pop_node(pid, buffer, size, flag);;
}