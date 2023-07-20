#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"   //modified by mingxuan 2019-5-14
#include "proto.h"
#include "string.h"
//#include "proc.h"
#include "global.h"
#include "box.h"
#include "cpu.h"  //added by mingxuan 2019-5-14

//for box manage
PRIVATE BOX_MANAGE_PTR bm_ptr = NULL;

//process
PRIVATE int free_process_queue(P_QUEUE_NODE_PTR *pq_ptr);
PRIVATE int pop_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS **pp_proc);
PRIVATE int push_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS *p_proc);
PRIVATE int pop_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS **pp_proc, int pid);
PRIVATE int push_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS *p_proc, int pid);
PRIVATE int wake_up_idle_process(BOX_PTR bp);
PRIVATE int wake_up_use_process(BOX_PTR bp, int pid);

//box
PRIVATE int alloc_idle_box(BOX_NODE_PTR *box_idle_pp);
PRIVATE int free_queue(BOX_NODE_PTR *box_pp);
PRIVATE int pop_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR *box_node_pp);
PRIVATE int push_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR box_node_p);
PRIVATE int push_use_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR box_node_p);
PRIVATE int pop_use_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR *box_node_pp, int pid);

//for debug
PRIVATE void box_disp_run_queue(int pid)
{
 if(pid < 0 || pid >= NR_PCBS) {
    disp_color_str_clear_screen("box_disp_run_queue: pid is not in range\n", 0x74);
    return ;
  }

  if(proc_table[pid].task.info.type == TYPE_THREAD) {
    pid = proc_table[pid].task.info.ppid;
  }

  if(bm_ptr->bm_box_prior[pid] == NULL) {
    disp_color_str_clear_screen("box_disp_run_queue: box not init\n", 0x74);
    return ;
  }
  
  BOX_PTR bp = bm_ptr->bm_box_prior[pid];
  BOX_NODE_PTR ptr = bp->b_use_prior;

  disp_str_clear_screen("disp run queue:[ ");
  for(; ptr!=NULL; ptr = ptr->next){
    disp_int(ptr->pid);
    disp_str_clear_screen(" ");
    disp_str(ptr->buffer);
    disp_str_clear_screen(" ");
  }
  disp_str_clear_screen("]\n");
}

PRIVATE void box_disp_idle_queue(int pid)
{
  if(pid < 0 || pid >= NR_PCBS) {
    disp_color_str_clear_screen("box_disp_idle_queue: pid is not in range\n", 0x74);
    return ;
  }

  if(proc_table[pid].task.info.type == TYPE_THREAD) {
    pid = proc_table[pid].task.info.ppid;
  }

  if(bm_ptr->bm_box_prior[pid] == NULL) {
    disp_color_str_clear_screen("box_disp_idle_queue: box not init\n", 0x74);
    return ;
  }
  
  BOX_PTR bp = bm_ptr->bm_box_prior[pid];
  BOX_NODE_PTR ptr = bp->b_idle_prior;

  disp_str_clear_screen("disp idle queue:[ ");
  for(; ptr!=NULL; ptr = ptr->next){
    disp_int(ptr->pid);
    disp_str_clear_screen(" ");
  }
  disp_str_clear_screen("]\n");
}

PUBLIC int init_box()
{
  int iflag = Begin_Int_Atomic();
  //bm_ptr = (BOX_MANAGE_PTR)K_PHY2LIN(test_kmalloc(sizeof(BOX_MANAGE)));
  bm_ptr = (BOX_MANAGE_PTR)K_PHY2LIN(do_kmalloc(sizeof(BOX_MANAGE))); //modified by mingxuan 2019-5-14
  //bm_ptr->bm_box_prior = (BOX_PTR*)K_PHY2LIN(test_kmalloc(NR_PCBS*sizeof(BOX_PTR)));
  bm_ptr->bm_box_prior = (BOX_PTR*)K_PHY2LIN(do_kmalloc(NR_PCBS*sizeof(BOX_PTR)));  //modified by mingxuan 2019-5-14

  int i = 0;
  for(;i < NR_PCBS; i++) {
    bm_ptr->bm_box_prior[i] = NULL;
  }
  End_Int_Atomic(iflag);
  return 0;
}

PUBLIC int alloc_box(int pid)
{ 
  int iflag = Begin_Int_Atomic();
  if(pid < 0 || pid >= NR_PCBS) {
    disp_color_str_clear_screen("alloc_box: pid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if(proc_table[pid].task.info.type == TYPE_THREAD) {
    pid = proc_table[pid].task.info.ppid;
  }

  if(bm_ptr->bm_box_prior[pid] != NULL) {
    disp_color_str_clear_screen("alloc_box: box has been init\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  //bm_ptr->bm_box_prior[pid] = (BOX_PTR)K_PHY2LIN(test_kmalloc(sizeof(BOX)));
  bm_ptr->bm_box_prior[pid] = (BOX_PTR)K_PHY2LIN(do_kmalloc(sizeof(BOX)));  //modified by mingxuan 2019-5-14
  BOX_PTR bp = bm_ptr->bm_box_prior[pid];
  
  bp->b_idle_prior = NULL;
  alloc_idle_box(&bp->b_idle_prior);
  bp->b_use_prior = NULL;
  bp->b_w_idle_prior = NULL;
  bp->b_w_use_prior = NULL;
  
  End_Int_Atomic(iflag);
  return 0;  
}

PUBLIC int free_box(int pid)
{
  int iflag = Begin_Int_Atomic();
  if(pid <0 || pid >= NR_PCBS) {
    disp_color_str_clear_screen("free_box: pid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if(proc_table[pid].task.info.type == TYPE_THREAD) {
    pid = proc_table[pid].task.info.ppid;
  }

  if(bm_ptr->bm_box_prior[pid] != NULL) {
    BOX_PTR bp = bm_ptr->bm_box_prior[pid];

    free_queue(&bp->b_idle_prior);
    free_queue(&bp->b_use_prior);
    free_process_queue(&bp->b_w_idle_prior);
    free_process_queue(&bp->b_w_use_prior);

    sys_free(bp);
    bm_ptr->bm_box_prior[pid] = NULL;
  }

  End_Int_Atomic(iflag);
  return 0;
}

PUBLIC int box_push_node(int pid, char *buffer, int size, int flag)
{
  WAKE_UP_INTO: { }

  int iflag = Begin_Int_Atomic();
  if(pid <0 || pid >= NR_PCBS) {
    disp_color_str_clear_screen("box_push_node: pid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if(proc_table[pid].task.info.type == TYPE_THREAD) {
    pid = proc_table[pid].task.info.ppid;
  }

  if(bm_ptr->bm_box_prior[pid] == NULL) {
    disp_color_str_clear_screen("box_push_node: this box is not get\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  BOX_PTR bp = bm_ptr->bm_box_prior[pid];
  BOX_NODE_PTR ptr = NULL;

  if(pop_node(&(bp->b_idle_prior), &ptr) < 0) {
    if(flag == IPC_NOWAIT) {
      disp_color_str_clear_screen("box_push_node: push_node fault\n", 0x74);
      End_Int_Atomic(iflag);
      return -1;
    } else {
      //push_w_idle_process(&(bp->b_w_idle_prior),p_proc_current);
      push_w_idle_process(&(bp->b_w_idle_prior),proc);  //modified by mingxuan 2019-5-14
      End_Int_Atomic(iflag);

      //while(!p_proc_current->task.waiting_to_ready) { delay(1); }
      while(!proc->task.waiting_to_ready) { delay(1); } //modified by mingxuan 2019-5-14
      //if(p_proc_current->task.waiting_to_ready == -1) return -1;
      if(proc->task.waiting_to_ready == -1) return -1;  //modified by mingxuan 2019-5-14
      goto WAKE_UP_INTO;
    }
  }

  int i=0;
  ptr->next = NULL;
  ptr->size = size > BOXMAX ? BOXMAX : size;
  //ptr->pid = p_proc_current - proc_table;
  ptr->pid = proc - proc_table; //modified by mingxuan 2019-5-14
  for(i=0;(i<size && i<BOXMAX); i++) {
    ptr->buffer[i] = buffer[i];
  }

  push_use_node(&(bp->b_use_prior), ptr);
  wake_up_use_process(bp, ptr->pid);
  End_Int_Atomic(iflag);
  return 0;
}

PUBLIC int box_pop_node(int pid, char *buffer, int size, int flag)
{
  WAKE_UP_INTO: { }
  int iflag = Begin_Int_Atomic();
  if(pid <0 || pid >= NR_PCBS) {
    disp_color_str_clear_screen("box_pop_node: pid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  //int id = p_proc_current - proc_table;
  int id = proc - proc_table; //modified by mingxuan 2019-5-14
  if(proc_table[id].task.info.type == TYPE_THREAD) {
    id = proc_table[id].task.info.ppid;
  }

  if(bm_ptr->bm_box_prior[id] == NULL) {
    disp_color_str_clear_screen("box_pop_node: this box is not get\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  BOX_PTR bp = bm_ptr->bm_box_prior[id];
  BOX_NODE_PTR ptr = NULL;

  if(pop_use_node(&(bp->b_use_prior), &ptr, pid) < 0) {
    if(flag == IPC_NOWAIT) {
      disp_color_str_clear_screen("box_pop_node: don't have this type message\n", 0x74);
      End_Int_Atomic(iflag);
      return -1;
    } else {
      //push_w_use_process(&bp->b_w_use_prior, p_proc_current, pid);
      push_w_use_process(&bp->b_w_use_prior, proc, pid);  //modified by mingxuan 2019-5-14
      End_Int_Atomic(iflag);
      
      //while(!p_proc_current->task.waiting_to_ready) { delay(1); }
      while(!proc->task.waiting_to_ready) { delay(1); } //modified by mingxuan 2019-5-14
      //if(p_proc_current->task.waiting_to_ready == -1) return -1;
      if(proc->task.waiting_to_ready == -1) return -1;  //modified by mingxuan 2019-5-14
      goto WAKE_UP_INTO;
    }
  }

  int i = 0;
  int message_size = (size > ptr->size) ? ptr->size : size;
  for(i=0; i<message_size; i++) {
    buffer[i] = ptr->buffer[i];
  }

  push_node(&(bp->b_idle_prior), ptr);
  wake_up_idle_process(bp);
  End_Int_Atomic(iflag);
  return 0;
}

PRIVATE int alloc_idle_box(BOX_NODE_PTR *box_idle_pp)
{
  int i=0;
  BOX_NODE_PTR *pptr = box_idle_pp;
  for(i=0; i<BOXMNB; i++, pptr=&((**pptr).next)) {
    //(*pptr) = (BOX_NODE_PTR)(K_PHY2LIN(test_kmalloc(sizeof(BOX_NODE))));
    (*pptr) = (BOX_NODE_PTR)(K_PHY2LIN(do_kmalloc(sizeof(BOX_NODE))));  //modified by mingxuan 2019-5-14

    (**pptr).pid = 0;
    (**pptr).size = 0;
    (**pptr).next = NULL;
    //(**pptr).buffer = (char*)(K_PHY2LIN(test_kmalloc(BOXMAX*sizeof(char))));
    (**pptr).buffer = (char*)(K_PHY2LIN(do_kmalloc(BOXMAX*sizeof(char))));  //modified by mingxuan 2019-5-14
  }
  return 0;
}

PRIVATE int free_queue(BOX_NODE_PTR *box_pp)
{
  BOX_NODE_PTR box_node_p = NULL;
  while(pop_node(box_pp, &box_node_p) == 0) {
    sys_free((*box_node_p).buffer);
    sys_free(box_node_p);
    box_node_p = NULL;
  }
  return 0;
}

PRIVATE int pop_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR *box_node_pp)
{
  BOX_NODE_PTR *pptr = box_pp;

  if((*pptr) == NULL) {
    return -1;
  }

  (*box_node_pp) = (*pptr);
  (*pptr) = (**pptr).next;
  (**box_node_pp).next = NULL;
  return 0;
}

PRIVATE int free_process_queue(P_QUEUE_NODE_PTR *pq_ptr)
{
  PROCESS *p_proc = NULL;
  char *p_reg = NULL;
  while(pop_w_idle_process(pq_ptr, &p_proc) == 0) {
    p_proc->task.stat = READY;
    p_proc->task.ticks = p_proc->task.priority;
    p_proc->task.waiting_to_ready = -1;
    p_proc = NULL;
  }
  return 0;
}

PRIVATE int pop_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS **pp_proc)
{
  P_QUEUE_NODE_PTR *pptr = w_idle_pp;

  if((*pptr) == NULL) {
    return -1;
  }

  P_QUEUE_NODE_PTR ptr;
  ptr = (*pptr);
  (*pp_proc) = ptr->data;
  (*pptr) = (**pptr).next;
  ptr->next = NULL;

  sys_free(ptr);
  return 0;
}

PRIVATE int push_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS *p_proc)
{
  P_QUEUE_NODE_PTR *pptr = w_idle_pp;
  while((*pptr) != NULL) {
    pptr = &((**pptr).next);
  }
  
  //P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(test_kmalloc(sizeof(P_QUEUE_NODE))));
  P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(do_kmalloc(sizeof(P_QUEUE_NODE)))); //modified by mingxuan 2019-5-14
  
  ptr->data = p_proc;
  ptr->next = NULL;
  p_proc->task.stat = WAITING;
  p_proc->task.ticks = 0;
  p_proc->task.waiting_to_ready = 0;

  (*pptr) = ptr;
  return 0;
}

PRIVATE int push_use_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR box_node_p)
{
  BOX_NODE_PTR *pptr = box_pp;

  while((*pptr) != NULL) {
    pptr = &((**pptr).next);
  }
  (*box_node_p).next = NULL;
  (*pptr) = box_node_p;
  return 0;
}

PRIVATE int wake_up_use_process(BOX_PTR bp, int pid)
{
  if(bp->b_w_use_prior == NULL) return 0;

  PROCESS *p_proc;
  if(pop_w_use_process(&bp->b_w_use_prior, &p_proc, pid) == 0) {
    p_proc->task.stat = READY;
    p_proc->task.ticks = p_proc->task.priority;
    p_proc->task.waiting_to_ready = 1;

    disp_color_str_clear_screen("[into use wake up]:[pid ", 0x72);
    disp_int(p_proc->task.pid);
    disp_color_str_clear_screen("]\n", 0x72);
  }
  
  return 0;
}

PRIVATE int pop_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS **pp_proc, int pid)
{
  P_QUEUE_NODE_PTR *pptr = w_use_pp;

  while((*pptr) != NULL) {
    if((**pptr).pid == pid) {
      P_QUEUE_NODE_PTR ptr;
      ptr = (*pptr);
      (*pp_proc) = ptr->data;
      (*pptr) = (**pptr).next;
      ptr->next = NULL;
      sys_free(ptr);
      return 0;
    }
    pptr = &((**pptr).next);
  }
  return -1;
}

PRIVATE int pop_use_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR *box_node_pp, int pid)
{
  BOX_NODE_PTR *pptr = box_pp;
  while((*pptr) != NULL) {
    if((**pptr).pid == pid) {
      (*box_node_pp) = (*pptr); 
      (*pptr) = (**pptr).next;
      (**box_node_pp).next = NULL;
      return 0;
    }
    pptr = &((**pptr).next);
  }
  return -1;
}

PRIVATE int push_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS *p_proc, int pid)
{
  P_QUEUE_NODE_PTR *pptr = w_use_pp;
  while((*pptr) != NULL) {
    pptr = &((**pptr).next);
  }
  
  //P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(test_kmalloc(sizeof(P_QUEUE_NODE))));
  P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(do_kmalloc(sizeof(P_QUEUE_NODE)))); //modified by mingxuan 2019-5-14
  
  ptr->data = p_proc;
  ptr->next = NULL;
  ptr->pid = pid;
  p_proc->task.stat = WAITING;
  p_proc->task.ticks = 0;
  p_proc->task.waiting_to_ready = 0;

  (*pptr) = ptr;
  return 0;
}

PRIVATE int push_node(BOX_NODE_PTR *box_pp, BOX_NODE_PTR box_node_p)
{
  BOX_NODE_PTR *pptr = box_pp;

  (*box_node_p).size = 0;
  (*box_node_p).pid = 0;
  (*box_node_p).next = (*pptr);
  (*pptr) = box_node_p;

  return 0;
}

PRIVATE int wake_up_idle_process(BOX_PTR bp)
{
  if(bp->b_w_idle_prior == NULL) return 0;
  
  PROCESS *p_proc;
  pop_w_idle_process(&bp->b_w_idle_prior, &p_proc);
  
  p_proc->task.stat = READY;
  p_proc->task.ticks = p_proc->task.priority;
  p_proc->task.waiting_to_ready = 1;

  disp_color_str_clear_screen("[into idle wake up]:[pid ", 0x72);
  disp_int(p_proc->task.pid);
  disp_color_str_clear_screen("]\n", 0x72);
  return 0;
}