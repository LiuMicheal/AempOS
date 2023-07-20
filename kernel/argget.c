#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"   //modified by mingxuan 2019-5-13
#include "proto.h"
#include "string.h"
//#include "proc.h"
#include "global.h"
#include "cpu.h"    //added by mingxuan 2019-5-13

//利用进程的esp，获取系统调用函数参数
int fetchint(u32 addr, int *ip) {
  
  //if(addr < p_proc_current->task.memmap.stack_lin_limit) {
  if(addr < proc->task.memmap.stack_lin_limit) {  //modified by mingxuan 2019-5-13
      disp_color_str_clear_screen("fetchint: failed!\n",0x74);
      return -1;
  }

  *ip = *(int*)(addr);
  return 0;
} 

PUBLIC int argint(u32 proc_esp, int n, int *ip) {
  return fetchint(proc_esp + 4*n + 4, ip); //"+4" for esp
}

PUBLIC int argptr(u32 proc_esp, int n, char **pp) {
  int ptr_addr;

  if(argint(proc_esp, n, &ptr_addr) < 0) {
    disp_color_str_clear_screen("argptr: get ptr_addr failed!\n",0x74);
    return -1;
  }

  if((u32)ptr_addr >= KernelLinBase) {
    disp_color_str_clear_screen("argptr: ptr_addr higher KernelLinBase!\n", 0x74);
    return -1;
  }

  *pp = (char*)ptr_addr;
  return 0;
}
