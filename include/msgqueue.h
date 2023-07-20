/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            msgqueue.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Yang Guang, 2017
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef	_MSGQUEUE_H_
#define	_MSGQUEUE_H_

//#include "proc.h"

//消息队列结点
typedef struct msg_queue_node {
  int    type;
  int    size;
  char*  buffer;
  struct msg_queue_node *next;
}MSG_QUEUE_NODE, *MSG_QUEUE_NODE_PTR;

/* PROCESS QUEUE */
typedef struct p_queue_node {
  int  type;
	PROCESS             *data;
	struct p_queue_node *next;
}P_QUEUE_NODE, *P_QUEUE_NODE_PTR;

//消息队列，用于管理和回收结点
typedef struct msg_queue {
  MSG_QUEUE_NODE_PTR mq_idle_prior; //消息空闲队列头指针
  MSG_QUEUE_NODE_PTR mq_use_prior; //消息占用队列头指针 
  P_QUEUE_NODE_PTR   mq_w_idle_prior; //等待空闲队列
  P_QUEUE_NODE_PTR   mq_w_use_prior; //等待占用队列
}MSG_QUEUE, *MSG_QUEUE_PTR;

//用于管理系统的消息队列
typedef struct msg_queue_manage {
  int           *mqm_allocs;
  int           *mqm_keys;
  MSG_QUEUE_PTR *mqm_queues_prior;
}MSG_QUEUE_MANAGE, *MSG_QUEUE_MANAGE_PTR;

#endif /* _MSGQUEUE_H_ */