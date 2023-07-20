/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            box.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Yang Guang, 2017
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef	_BOX_H_
#define	_BOX_H_

//邮箱结点
typedef struct box_node {
  int pid;
  int size;
  char *buffer;
  struct box_node *next;
}BOX_NODE, *BOX_NODE_PTR;

/* PROCESS QUEUE */
typedef struct p_queue_node {
  int  pid;
	PROCESS             *data;
	struct p_queue_node *next;
}P_QUEUE_NODE, *P_QUEUE_NODE_PTR;

//邮箱
typedef struct box {
  BOX_NODE_PTR b_idle_prior; //可用信槽头指针
  BOX_NODE_PTR b_use_prior; //占用信槽头指针 
  P_QUEUE_NODE_PTR   b_w_idle_prior; //等待空闲队列
  P_QUEUE_NODE_PTR   b_w_use_prior; //等待占用队列
}BOX, *BOX_PTR;

//用于管理系统的邮箱
typedef struct box_manage {
  BOX_PTR       *bm_box_prior;
}BOX_MANAGE, *BOX_MANAGE_PTR;

#endif /* _BOX_H_ */