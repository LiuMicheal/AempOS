#include "type.h"
#include "const.h"
#include "protect.h"
#include "proc.h"   //modified by mingxuan 2019-5-13
#include "proto.h"
#include "string.h"
//#include "proc.h"
#include "global.h"
#include "msgqueue.h"
#include "cpu.h"    //added by mingxuan 2019-5-13

//for mag queue manage
PRIVATE MSG_QUEUE_MANAGE_PTR mqm_ptr = NULL;

//for ipc wait
PRIVATE int push_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS *p_proc);
PRIVATE int pop_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS **pp_proc);
PRIVATE int wake_up_idle_process(MSG_QUEUE_PTR mqp);

PRIVATE int push_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS *p_proc, int type);
PRIVATE int pop_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS **pp_proc, int type);
PRIVATE int wake_up_use_process(MSG_QUEUE_PTR mqp, int type);

PRIVATE int free_process_queue(P_QUEUE_NODE_PTR *pq_ptr);

//for process msg node
PRIVATE int init_msg_queue(int msqid);
PRIVATE int alloc_idle_queue(MSG_QUEUE_NODE_PTR *msg_idle_pp);
PRIVATE int free_queue(MSG_QUEUE_NODE_PTR *msg_queue_pp);
PRIVATE int pop_node(MSG_QUEUE_NODE_PTR *msg_queue_pp, MSG_QUEUE_NODE_PTR *msg_node_pp);
PRIVATE int push_node(MSG_QUEUE_NODE_PTR *msg_queue_pp, MSG_QUEUE_NODE_PTR msg_node_p);
PRIVATE int pop_use_node(MSG_QUEUE_NODE_PTR *msg_use_pp, MSG_QUEUE_NODE_PTR *msg_node_pp, int type);
PRIVATE int push_use_node(MSG_QUEUE_NODE_PTR *msg_use_pp, MSG_QUEUE_NODE_PTR msg_node_p);

PUBLIC int init_msg_queue_manage()
{
  int iflag = Begin_Int_Atomic();
  //mqm_ptr = (MSG_QUEUE_MANAGE_PTR)K_PHY2LIN(test_kmalloc(sizeof(MSG_QUEUE_MANAGE)));
  mqm_ptr = (MSG_QUEUE_MANAGE_PTR)K_PHY2LIN(do_kmalloc(sizeof(MSG_QUEUE_MANAGE)));  //modified by mingxuan 2019-5-13
  
  //mqm_ptr->mqm_keys = (int*)K_PHY2LIN(test_kmalloc(MSGMNI*sizeof(int)));
  mqm_ptr->mqm_keys = (int*)K_PHY2LIN(do_kmalloc(MSGMNI*sizeof(int)));  //modified by mingxuan 2019-5-13
  //mqm_ptr->mqm_queues_prior = (MSG_QUEUE_PTR)K_PHY2LIN(test_kmalloc(MSGMNI*sizeof(MSG_QUEUE_PTR)));
  mqm_ptr->mqm_queues_prior = (MSG_QUEUE_PTR)K_PHY2LIN(do_kmalloc(MSGMNI*sizeof(MSG_QUEUE_PTR))); //modified by mingxuan 2019-5-13
  //mqm_ptr->mqm_allocs = (int*)K_PHY2LIN(test_kmalloc(MSGMNI*sizeof(int)));
  mqm_ptr->mqm_allocs = (int*)K_PHY2LIN(do_kmalloc(MSGMNI*sizeof(int))); //modified by mingxuan 2019-5-13


  //init all msg_queue
  int i=0;
  for( i = 0; i < MSGMNI; i++) {
    mqm_ptr->mqm_allocs[i] = -1;
    mqm_ptr->mqm_keys[i] = 0;
    mqm_ptr->mqm_queues_prior[i] = NULL;
  }
  End_Int_Atomic(iflag);
  return 0;
}

PUBLIC int alloc_msg_queue(int key)
{ 
  int iflag = Begin_Int_Atomic();
  int i=0;
  int f=-1;
  int a=-1;
  for( i = 0; i < MSGMNI; i++) {
    
    if((mqm_ptr->mqm_allocs[i] != -1) && (mqm_ptr->mqm_keys[i] == key)) {
      f = i;
    }

    if(mqm_ptr->mqm_allocs[i] == -1) {
      a = i;
    }
  }

  if((f == -1) && (a != -1)) {
    mqm_ptr->mqm_allocs[a] = 1;
    mqm_ptr->mqm_keys[a] = key;
  }
  End_Int_Atomic(iflag);
  return (f != -1) ? f : a;
}

PUBLIC int free_msg_queue(int msqid)
{
  int iflag = Begin_Int_Atomic();
  if(msqid < 0 || msqid >= MSGMNI) {
    disp_color_str_clear_screen("free_msg_queue: msqid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  mqm_ptr->mqm_allocs[msqid] = -1;
  mqm_ptr->mqm_keys[msqid] = 0;
  if(mqm_ptr->mqm_queues_prior[msqid] != NULL) {

    MSG_QUEUE_PTR ptr = mqm_ptr->mqm_queues_prior[msqid];

    free_queue(&ptr->mq_idle_prior);
    free_queue(&ptr->mq_use_prior);
    free_process_queue(&ptr->mq_w_idle_prior);
    free_process_queue(&ptr->mq_w_use_prior);

    sys_free(ptr);
    
    mqm_ptr->mqm_queues_prior[msqid] = NULL;
  } 
  End_Int_Atomic(iflag);
  return 0;
}

PUBLIC int msg_queue_push_node(int msqid, int size, int type, char *buffer, int flag) 
{
  WAKE_UP_INTO: { }

  int iflag = Begin_Int_Atomic();
  if(msqid < 0 || msqid >= MSGMNI) {
    disp_color_str_clear_screen("msg_queue_push_node: msqid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if(mqm_ptr->mqm_allocs[msqid] == -1) {
    disp_color_str_clear_screen("msg_queue_push_node: msqid is not alloc\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if((mqm_ptr->mqm_queues_prior[msqid] == NULL) && (init_msg_queue(msqid) < 0)) {
    disp_color_str_clear_screen("msg_queue_push_node: init_msg_queue fault\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  MSG_QUEUE_PTR      mqp = mqm_ptr->mqm_queues_prior[msqid];
  MSG_QUEUE_NODE_PTR ptr = NULL;

  if( pop_node(&((*mqp).mq_idle_prior), &ptr) < 0) {
    if(flag == IPC_NOWAIT) {
      disp_color_str_clear_screen("msg_queue_push_node: push_node fault\n", 0x74);
      End_Int_Atomic(iflag);
      return -1;
    } else {
      //push_w_idle_process(&mqp->mq_w_idle_prior,p_proc_current);
      push_w_idle_process(&mqp->mq_w_idle_prior,proc); //modified by mingxuan 2019-5-13
      End_Int_Atomic(iflag);

      //while(!p_proc_current->task.waiting_to_ready) { delay(1); }
      while(!proc->task.waiting_to_ready) { delay(1); }
      //if(p_proc_current->task.waiting_to_ready == -1) return -1;
      if(proc->task.waiting_to_ready == -1) return -1;
      goto WAKE_UP_INTO;
    }
  }

  //copy
  int i=0;
  ptr->next = NULL;
  ptr->size = size > MSGMAX ? MSGMAX : size;
  ptr->type = type;
  for(i=0;(i<size && i<MSGMAX); i++) {
    ptr->buffer[i] = buffer[i];
  }

  push_use_node(&((*mqp).mq_use_prior), ptr);
  wake_up_use_process(mqp, type);
  End_Int_Atomic(iflag);
  return 0;
}

PUBLIC int msg_queue_pop_node(int msqid, int size, int type, char *buffer, int flag)
{
  WAKE_UP_INTO: { }
  int iflag = Begin_Int_Atomic();
  if(msqid < 0 || msqid >= MSGMNI) {
    disp_color_str_clear_screen("msg_queue_pop_node: msqid is not in range\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if(mqm_ptr->mqm_allocs[msqid] == -1) {
    disp_color_str_clear_screen("msg_queue_pop_node: msqid is not alloc\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  if((mqm_ptr->mqm_queues_prior[msqid] == NULL) && (init_msg_queue(msqid) < 0)) {
    disp_color_str_clear_screen("msg_queue_pop_node: init_msg_queue fault\n", 0x74);
    End_Int_Atomic(iflag);
    return -1;
  }

  MSG_QUEUE_PTR      mqp = mqm_ptr->mqm_queues_prior[msqid];
  MSG_QUEUE_NODE_PTR ptr = NULL;

  if(pop_use_node(&((*mqp).mq_use_prior), &ptr, type) < 0) {
    if(flag == IPC_NOWAIT) {
      disp_color_str_clear_screen("msg_queue_pop_node: don't have this type message\n", 0x74);
      End_Int_Atomic(iflag);
      return -1;
    } else {
      //push_w_use_process(&mqp->mq_w_use_prior, p_proc_current, type);
      push_w_use_process(&mqp->mq_w_use_prior, proc, type); //modified by mingxuan 2019-5-13
      End_Int_Atomic(iflag);
      
      //while(!p_proc_current->task.waiting_to_ready) { delay(1); }
      while(!proc->task.waiting_to_ready) { delay(1); } //modified by mingxuan 2019-5-13
      //if(p_proc_current->task.waiting_to_ready == -1) return -1;
      if(proc->task.waiting_to_ready == -1) return -1;  //modified by mingxuan 2019-5-13
      goto WAKE_UP_INTO;
    }
  }

  int i = 0;
  int message_size = (size > ptr->size) ? ptr->size : size;
  for(i=0; i<message_size; i++) {
    buffer[i] = ptr->buffer[i];
  }

  push_node(&((*mqp).mq_idle_prior), ptr);
  wake_up_idle_process(mqp);
  End_Int_Atomic(iflag);

  return message_size;
}

// PUBLIC void msg_queue_disp_run_queue(int pid)
// {
//   if(pid < 0 || pid >= NR_PCBS) {
//     disp_color_str_clear_screen("msg_queue_disp_run_queue: pid is not in range", 0x74);
//     return ;
//   }

//   PROCESS* p_proc = proc_table + pid;

//   if(p_proc->msg_queue_ptr == NULL) {
//     disp_color_str_clear_screen("msg_queue_disp_run_queue: don't have message", 0x74);
//     return ;
//   }
  
//   MSG_QUEUE_PTR mqp = (MSG_QUEUE_PTR)(p_proc->msg_queue_ptr);
//   MSG_QUEUE_NODE_PTR ptr = mqp->mq_use_prior;

//   disp_str_clear_screen(" disp run queue:");
//   spin_lock(&mqp->mq_use_lock);
//   for(; ptr!=NULL; ptr = ptr->next){
//     disp_str_clear_screen(" ");
//     disp_int(ptr->type);
//     disp_str_clear_screen(" ");
//     disp_str_clear_screen(ptr->buffer);
//   }
//   spin_unlock(&mqp->mq_use_lock);
// }

// PUBLIC void msg_queue_disp_idle_queue(int pid)
// {
//   if(pid < 0 || pid >= NR_PCBS) {
//     disp_color_str_clear_screen("msg_queue_disp_idle_queue: pid is not in range", 0x74);
//     return ;
//   }

//   PROCESS* p_proc = proc_table + pid;

//   if(p_proc->msg_queue_ptr == NULL) {
//     disp_color_str_clear_screen("msg_queue_disp_idle_queue: don't have message", 0x74);
//     return ;
//   }
  
//   MSG_QUEUE_PTR mqp = (MSG_QUEUE_PTR)(p_proc->msg_queue_ptr);
//   MSG_QUEUE_NODE_PTR ptr = mqp->mq_idle_prior;

//   disp_str_clear_screen(" disp idle queue:");
//   spin_lock(&mqp->mq_idle_lock);
//   for(; ptr!=NULL; ptr = ptr->next){
//     disp_str_clear_screen(" ");
//     disp_int(ptr->type);
//     disp_str_clear_screen(" ");
//     disp_str(ptr->buffer);
//   }
//   spin_unlock(&mqp->mq_idle_lock);
// }

PRIVATE int init_msg_queue(int msqid)
{
  if(msqid < 0 || msqid >= MSGMNI) {
    disp_color_str_clear_screen("init_msg_queue: msqid is not in range\n", 0x74);
    return -1;
  }

  if(mqm_ptr->mqm_queues_prior[msqid] != NULL) {
    disp_color_str_clear_screen("init_msg_queue: msg_queue has been init\n", 0x74);
    return -1;
  }

  //mqm_ptr->mqm_queues_prior[msqid] = (MSG_QUEUE_PTR)K_PHY2LIN(test_kmalloc(sizeof(MSG_QUEUE)));
  mqm_ptr->mqm_queues_prior[msqid] = (MSG_QUEUE_PTR)K_PHY2LIN(do_kmalloc(sizeof(MSG_QUEUE))); //modified by mingxuan 2019-5-13
  MSG_QUEUE_PTR mqp = (MSG_QUEUE_PTR)mqm_ptr->mqm_queues_prior[msqid];
  
  mqp->mq_idle_prior = NULL;
  alloc_idle_queue(&mqp->mq_idle_prior);
  
  mqp->mq_use_prior = NULL;

  mqp->mq_w_idle_prior = NULL;
  
  mqp->mq_w_use_prior = NULL;
  
  return 0;
}

PRIVATE int alloc_idle_queue(MSG_QUEUE_NODE_PTR *msg_idle_pp)
{
  int i=0;
  MSG_QUEUE_NODE_PTR *pptr = msg_idle_pp;
  for(i=0; i<MSGMNB; i++, pptr=&((**pptr).next)) {
    //(*pptr) = (MSG_QUEUE_NODE_PTR)(K_PHY2LIN(test_kmalloc(sizeof(MSG_QUEUE_NODE))));
    (*pptr) = (MSG_QUEUE_NODE_PTR)(K_PHY2LIN(do_kmalloc(sizeof(MSG_QUEUE_NODE))));  //modified by mingxuan 2019-5-13

    (**pptr).size = 0;
    (**pptr).type = 0;
    (**pptr).next = NULL;
    //(**pptr).buffer = (char*)(K_PHY2LIN(test_kmalloc(MSGMAX*sizeof(char))));
    (**pptr).buffer = (char*)(K_PHY2LIN(do_kmalloc(MSGMAX*sizeof(char))));  //modified by mingxuan 2019-5-13
  }
  return 0;
}

PRIVATE int free_queue(MSG_QUEUE_NODE_PTR *msg_queue_pp)
{
  MSG_QUEUE_NODE_PTR msg_node_p = NULL;
  while(pop_node(msg_queue_pp, &msg_node_p) == 0) {
    sys_free((*msg_node_p).buffer);
    sys_free(msg_node_p);
    msg_node_p = NULL;
  }
  return 0;
}

PRIVATE int pop_node(MSG_QUEUE_NODE_PTR *msg_queue_pp, MSG_QUEUE_NODE_PTR *msg_node_pp)
{
  MSG_QUEUE_NODE_PTR *pptr = msg_queue_pp;

  if((*pptr) == NULL) {
    return -1;
  }

  (*msg_node_pp) = (*pptr);
  (*pptr) = (**pptr).next;
  (**msg_node_pp).next = NULL;
  return 0;
}

PRIVATE int push_node(MSG_QUEUE_NODE_PTR *msg_queue_pp, MSG_QUEUE_NODE_PTR msg_node_p)
{
  MSG_QUEUE_NODE_PTR *pptr = msg_queue_pp;

  (*msg_node_p).size = 0;
  (*msg_node_p).type = 0;
  (*msg_node_p).next = (*pptr);
  (*pptr) = msg_node_p;

  return 0;
}

PRIVATE int push_use_node(MSG_QUEUE_NODE_PTR *msg_use_pp, MSG_QUEUE_NODE_PTR msg_node_p)
{
  MSG_QUEUE_NODE_PTR *pptr = msg_use_pp;

  while((*pptr) != NULL) {
    pptr = &((**pptr).next);
  }
  (*msg_node_p).next = NULL;
  (*pptr) = msg_node_p;

  return 0;
}

PRIVATE int pop_use_node(MSG_QUEUE_NODE_PTR *msg_use_pp, MSG_QUEUE_NODE_PTR *msg_node_pp, int type)
{
  MSG_QUEUE_NODE_PTR *pptr = msg_use_pp;
  
  while((*pptr) != NULL) {
    if((**pptr).type == type) {
      (*msg_node_pp) = (*pptr); 
      (*pptr) = (**pptr).next;
      (**msg_node_pp).next = NULL;
      return 0;
    }
    pptr = &((**pptr).next);
  }
  return -1;
}

PRIVATE int push_w_idle_process(P_QUEUE_NODE_PTR *w_idle_pp, PROCESS *p_proc)
{
  P_QUEUE_NODE_PTR *pptr = w_idle_pp;
  while((*pptr) != NULL) {
    pptr = &((**pptr).next);
  }
  
  //P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(test_kmalloc(sizeof(P_QUEUE_NODE))));
  P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(do_kmalloc(sizeof(P_QUEUE_NODE)))); //modified by mingxuan 2019-5-13
  
  ptr->data = p_proc;
  ptr->next = NULL;
  
  p_proc->task.stat = WAITING;
  p_proc->task.ticks = 0;
  p_proc->task.waiting_to_ready = 0;

  (*pptr) = ptr;

  disp_color_str_clear_screen("[into idle wait queue]:[pid ", 0x72);
  disp_int(p_proc->task.pid);
  disp_color_str_clear_screen("]\n", 0x72);

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

PRIVATE int wake_up_idle_process(MSG_QUEUE_PTR mqp)
{ 
  if(mqp->mq_w_idle_prior == NULL) return 0;
  
  PROCESS *p_proc;
  pop_w_idle_process(&mqp->mq_w_idle_prior, &p_proc);
  
  p_proc->task.stat = READY;
  p_proc->task.ticks = p_proc->task.priority;
  p_proc->task.waiting_to_ready = 1;
  
  disp_color_str_clear_screen("[into idle wake up]:[pid ", 0x72);
  disp_int(p_proc->task.pid);
  disp_color_str_clear_screen("]\n", 0x72);

  return 0;
}

PRIVATE int push_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS *p_proc, int type)
{
  P_QUEUE_NODE_PTR *pptr = w_use_pp;
  while((*pptr) != NULL) {
    pptr = &((**pptr).next);
  }
  
  //P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(test_kmalloc(sizeof(P_QUEUE_NODE))));
  P_QUEUE_NODE_PTR ptr = (P_QUEUE_NODE_PTR)(K_PHY2LIN(do_kmalloc(sizeof(P_QUEUE_NODE)))); //modified by mingxuan 2019-5-13
  
  ptr->data = p_proc;
  ptr->next = NULL;
  ptr->type = type;
  p_proc->task.stat = WAITING;
  p_proc->task.ticks = 0;
  p_proc->task.waiting_to_ready = 0;

  (*pptr) = ptr;

  disp_color_str_clear_screen("[into use wait queue]:[pid ", 0x72);
  disp_int(p_proc->task.pid);
  disp_color_str_clear_screen("]\n", 0x72);

  return 0;
}

PRIVATE int pop_w_use_process(P_QUEUE_NODE_PTR *w_use_pp, PROCESS **pp_proc, int type)
{
  P_QUEUE_NODE_PTR *pptr = w_use_pp;

  while((*pptr) != NULL) {
    if((**pptr).type == type) {
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

PRIVATE int wake_up_use_process(MSG_QUEUE_PTR mqp, int type)
{
  if(mqp->mq_w_use_prior == NULL) return 0;
  
  PROCESS *p_proc;
  if(pop_w_use_process(&mqp->mq_w_use_prior, &p_proc, type) == 0) {
    
    p_proc->task.stat = READY;
    p_proc->task.ticks = p_proc->task.priority;
    p_proc->task.waiting_to_ready = 1;

    disp_color_str_clear_screen("[into use wake up]:[pid ", 0x72);
    disp_int(p_proc->task.pid);
    disp_color_str_clear_screen("]\n", 0x72);
  }
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


