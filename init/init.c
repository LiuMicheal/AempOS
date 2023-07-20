/******************************************************
*	init进程	add by visual 2016.5.16
*******************************************************/

#include "stdio.h"

//int global = 0;	//deleted by mingxuan 2019-3-1
//int global = 0x100;	//modified by mingxuan 2019-3-1

/*	//deleted by mingxuan 2019-3-3
char *str2,*str3;


void pthread_test3()
{
	int i;
	str2 = malloc(10);
	*(str2+0) = 'M';
	*(str2+1) = 'a';
	*(str2+2) = '\0';
	
	while(1)
	{
		if(str3!=0)
		{
			udisp_str("pth3");
			(*(str2+1)) += 1;
			udisp_str(str3);
			udisp_str(" ");
		}		
		i=10000;
		while(--i){}
	}
}


void pthread_test2()
{
	int i;
	str3 = malloc(10);
	*(str3+0) = 'M';
	*(str3+1) = 'z';
	*(str3+2) = '\0';
	
	pthread(pthread_test3);	
	while(1)
	{
		if(str2!=0)
		{
			udisp_str("pth2");
			(*(str3+1)) -=1;
			udisp_str(str2);
			udisp_str(" ");
		}
		
		i=10000;
		while(--i){}
	}
}

void pthread_test1()
{
	int i;
	pthread(pthread_test2);
	while(1)
	{
		udisp_str("pth1");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
}
*/


/*======================================================================*
                          Syscall Pthread Test
added by xw, 18/4/27
 *======================================================================*/
/*
int main(int arg,char *argv[])
{
	int i=0;
	
	pthread(pthread_test1);
	while(1)
	{
		udisp_str("init");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
	return 0;
}
*/

/*======================================================================*
                          Syscall Fork Test
added by xw, 18/4/27
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	int i=0;
	
	fork();
	while(1)
	{
		udisp_str("init");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
	return ;
}
*/

/*======================================================================*
                           Syscall Exec Test
added by xw, 18/4/27
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		udisp_str("init");
		udisp_int(++global);
		udisp_str(" ");
		i=10000;
		while(--i){}
	}
	return ;
}
*/	

/*======================================================================*
                           Syscall Yield Test
added by xw, 18/8/16
 *======================================================================*/
	/*
void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		udisp_str("U( ");
		yield();
		udisp_str(") ");
		i=10000;
		while(--i){}
	}
	return ;
}
//	*/

/*======================================================================*
                           Syscall Sleep Test
added by xw, 18/8/16
 *======================================================================*/
	/*
void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		udisp_str("U( ");
		udisp_str("[");
		udisp_int(get_ticks());
		udisp_str("] ");
		sleep(5);
		udisp_str("[");
		udisp_int(get_ticks());
		udisp_str("] ");
		udisp_str(") ");
		i=10000;
		while(--i){}
	}
	return ;
}
//	*/

/*======================================================================*
                           File System Test
added by xw, 18/6/19
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
//	while (1) {}
	
	int fd;
	int i, n;
	const int rd_bytes = 4;
	char filename[MAX_FILENAME_LEN+1] = "blah";
	char bufr[5];
	const char bufw[] = "abcde";

	udisp_str("\n(U)");
	fd = open(filename, O_CREAT | O_RDWR);	
	
	if(fd != -1) {
		udisp_str("File created: ");
		udisp_str(filename);
		udisp_str(" (fd ");
		udisp_int(fd);
		udisp_str(")\n");	
		
		n = write(fd, bufw, strlen(bufw));
		if(n != strlen(bufw)) {
			udisp_str("Write error\n");
		}
		
		close(fd);
	}

	udisp_str("(U)");
	fd = open(filename, O_RDWR);
	udisp_str("   ");
	udisp_str("File opened. fd: ");
	udisp_int(fd);
	udisp_str("\n");

	udisp_str("(U)");
	int lseek_status = lseek(fd, 1, SEEK_SET);
	udisp_str("Return value of lseek is: ");
	udisp_int(lseek_status);
	udisp_str("  \n");

	udisp_str("(U)");
	n = read(fd, bufr, rd_bytes);
	if(n != rd_bytes) {
		udisp_str("Read error\n");
	}
	bufr[n] = 0;
	udisp_str("Bytes read: ");
	udisp_str(bufr);
	udisp_str("\n");

	close(fd);
	
	while (1) {
	}
	
	return;
}
*/

/*======================================================================*
                   Syscall Fork Test in MP 测试CPU并行
added by mingxuan 2019-2-27
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	int i=0;
	int pid=0;
	//acquire(&initlock); //added by mingxuan 2019-2-27
	
	pid = fork();
	while(1)
	{
	
	if(6==pid)
	{
		while(1)
		{
			udisp_str("child1 ");
			//udisp_int(++global);
			udisp_str(" ");

			i=100000000;
			while(--i){}
		}
	}
	else if(7==pid)
	{
		while(1)
		{
			udisp_str("child2 ");
			//udisp_int(++global);
			udisp_str(" ");

			i=100000000;
			while(--i){}
		}
	}
	else
	{
			udisp_str("father");
			//udisp_int(++global);
			udisp_str(" ");

			i=1000000;
			while(--i){}
	}

	}
	//release(&initlock); //added by mingxuan 2019-2-27

	return ;
}
*/

/*======================================================================*
                   Syscall Fork Test in MP 测试fork功能
added by mingxuan 2019-2-27
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	int i=0;
	int pid=0;
	
	pid = fork();
	while(1)
	{
		//子进程
		if(0==pid)
		{
			//udisp_str("child ");
			udisp_int(get_pid());
			//udisp_int(get_cpuid());
			udisp_str(" ");

			i=10000000;
			while(--i){}
		}
		else
		{
			while(1)
			{
				//udisp_str("father ");
				udisp_int(get_pid());
				//udisp_int(get_cpuid());
				udisp_str(" ");

				i=10000000;
				while(--i){}
			}		
		}	

	}

	return ;
}
*/

/*
//下面这段代码为什么执行错误?
//added by mingxuan 2019-2-28
void main(int arg,char *argv[])
{
	int pid=0;
 
	pid = fork(); 
    if (pid < 0) udisp_str("error ");
	else if (pid == 0) 
		udisp_str("child "); 
	else 
		udisp_str("father ");

	return ;
}
*/


/*======================================================================*
                   Syscall Fork Test in MP 测试pthread功能
added by mingxuan 2019-2-28
 *======================================================================*/
/*
void pthread_test()
{
	int i;
	while(1)
	{
		udisp_str("pthread ");
		udisp_int(get_pid());
		udisp_str(" ");

		i=10000;
		while(--i){}
	}
}

void main(int arg,char *argv[])
{
	int i=0;
	int pid=0;
	
	pid = pthread(pthread_test);
	while(1)
	{
		udisp_str("main ");
		udisp_int(get_pid());
		udisp_str(" ");

		i=10000000;
		while(--i){}
	}

	return ;
}
*/


/*======================================================================*
             Syscall Fork Test in MP 测试fork中共享变量的问题
added by mingxuan 2019-3-1
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	int pid=0;
	int i;
	//int global = 0x100;
 
	pid = fork(); 
	while(1){
		if (pid == 0)
		{
			udisp_str("child ");
			udisp_int(global++);
			//udisp_int(get_cpuid());
			udisp_str(" ");

			i=10000000;
			while(--i){}
		} 
		else 
		{
			udisp_str("father ");
			udisp_int(global--);
			//udisp_int(get_cpuid());
			udisp_str(" ");

			i=10000000;
			while(--i){}			
		}

	}

	return ;
}
*/

/*======================================================================*
            Syscall Fork Test in MP 测试pthread功能中共享变量的问题
added by mingxuan 2019-2-28
 *======================================================================*/
/*
void pthread_test()
{
	int i;
	while(1)
	{
		udisp_str("pthread ");
		udisp_int(global++);
		udisp_str(" ");

		i=10000000;
		while(--i){}
	}
}

void main(int arg,char *argv[])
{
	int i=0;
	int pid=0;
	
	//int global = 0x100;
	pid = pthread(pthread_test);
	while(1)
	{
		udisp_str("main ");
		udisp_int(global--);
		udisp_str(" ");

		i=10000000;
		while(--i){}
	}

	return ;
}
*/

/*======================================================================*
       	 Syscall Fork Test in MP 测试多次fork(3次)(含共享变量测试)
added by mingxuan 2019-3-1
 *======================================================================*/
/*
int global = 0x100;
void main(int arg,char *argv[])
{
	int pid1=0, pid2=0, pid3=0;
	int i;
	//int global = 0x100;
 
	pid1 = fork(); 
	if (pid1 == 0)
	{	
		while(1)
		{
			udisp_str("child1 ");
			udisp_int(get_pid());
			udisp_int(++global);

			yield();

			i=10000000;
			while(--i){}
		}
	} 

	pid2 = fork();
	if (pid2 == 0)
	{	
		while(1)
		{
			udisp_str("child2 ");
			udisp_int(get_pid());
			udisp_int(--global);

			yield();

			i=10000000;
			while(--i){}
		}
	} 

	pid3 = fork();
	if (pid3 == 0)
	{	
		while(1)
		{
			udisp_str("child3 ");
			udisp_int(get_pid());

			yield();

			i=10000000;
			while(--i){}
		}
	} 

	while(1)
	{
		udisp_str("father ");
		udisp_int(get_pid());

		yield();

		i=10000000;
		while(--i){}			
	}
	
	while(1);

	return ;
}
*/

/*======================================================================*
       	 Syscall Fork Test in MP 测试多次pthread(3次)(含共享变量测试)
added by mingxuan 2019-3-1
 *======================================================================*/
/*
int global = 0x100;

void pthread_test1()
{
	int i;
	while(1)
	{
		udisp_str("pthread1 ");
		udisp_int(get_pid());
		udisp_str(" ");
		
		yield();

		udisp_int(++global);
		//udisp_str(" ");

		i=10000000;
		while(--i){}
	}
}

void pthread_test2()
{
	int i;
	while(1)
	{
		udisp_str("pthread2 ");
		udisp_int(get_pid());
		udisp_str(" ");
		
		yield();

		udisp_int(--global);
		//udisp_str(" ");

		i=10000000;
		while(--i){}
	}
}

void pthread_test3()
{
	int i;
	while(1)
	{
		udisp_str("pthread3 ");
		udisp_int(get_pid());
		udisp_str(" ");
		
		yield();

		//udisp_int(++global);
		//udisp_str(" ");

		i=10000000;
		while(--i){}
	}
}

void main(int arg,char *argv[])
{
	int i;

	pthread(pthread_test1);
	pthread(pthread_test2);
	pthread(pthread_test3);

	while(1)
	{
		udisp_str("father ");
		//udisp_str(" ");
		udisp_int(get_pid());
		//udisp_int(get_cpuid());
		//udisp_str(" ");

		yield();

		i=10000000;
		while(--i){}			
	}

	while(1);

	return ;
}
*/


/*======================================================================*
        syscall Fork Test in MP 测试嵌套fork(子进程再fork)(含共享变量测试)
added by mingxuan 2019-3-1
 *======================================================================*/
/*
int global = 0x100;
void main(int arg,char *argv[])
{
	int i=0;
	int pid1=0;//用于父进程fork()
	int pid2=0;//用于子进程fork()
	int pid3=0;//用于子进程的子进程fork()
	
	pid1=fork();
	if(0==pid1)
	{
		pid2=fork();
		if(0==pid2)
		{	
			pid3=fork();
			if(0==pid3)
			{
				while(1)
				{
					udisp_str("child3 ");
					udisp_int(get_pid());
					udisp_str(" ");
					udisp_int(++global);

					yield();

					i=10000000;
					while(--i){}		
				}
			}
			else
			{
				while(1)
				{
					udisp_str("child2 ");
					udisp_int(get_pid());
					udisp_str(" ");
					udisp_int(--global);

					yield();

					i=10000000;
					while(--i){}		
				}	
			}
		}
		else
		{
			while(1)
			{
				udisp_str("child1 ");
				udisp_int(get_pid());
				udisp_str(" ");

				yield();

				i=10000000;
				while(--i){}
			}
		}
	}
	else
	{
		while(1)
		{
			udisp_str("father ");
			udisp_int(get_pid());
			udisp_str(" ");

			yield();

			i=10000000;
			while(--i){}
		}		
	}	

	//}

	return ;
}
*/


/*======================================================================*
        syscall Fork Test in MP 测试嵌套pthread(子线程再pthread)(含共享变量测试)
added by mingxuan 2019-3-12
 *======================================================================*/
/*
void pthread_test3()
{
	int i;
	while(1)
	{
		//udisp_str("pth3");
		udisp_int(get_pid());			
		udisp_str(" ");	

		i=10000000;
		while(--i){}
	}
}

void pthread_test2()
{
	int i;
	pthread(pthread_test3);	
	while(1)
	{

		//udisp_str("pth2");
		udisp_int(get_pid());	
		udisp_str(" ");
		
		i=10000000;
		while(--i){}
	}
}

void pthread_test1()
{
	int i;
	pthread(pthread_test2);
	while(1)
	{
		//udisp_str("pth1");
		udisp_int(get_pid());
		udisp_str(" ");
		i=10000000;
		while(--i){}
	}
}

int main(int arg,char *argv[])
{
	int i=0;
	
	pthread(pthread_test1);
	while(1)
	{
		//udisp_str("init");
		udisp_int(get_pid());	
		udisp_str(" ");
		i=10000000;
		while(--i){}
	}
	return 0;
}
*/

/*======================================================================*
        Syscall Fork Test in MP 测试在fork中嵌套pthread的功能(含共享变量测试)
added by mingxuan 2019-3-14
 *======================================================================*/
/*
int global = 0x100;

void pthread_test()
{
	int i;
	while(1)
	{
		//udisp_str("pthread ");
		udisp_int(get_pid());
		udisp_str(" ");
		//udisp_int(++global);
		
		//yield();

		i=10000000;
		while(--i){}
	}
}


void main(int arg,char *argv[])
{
	int i=0;
	int pid1=0;//用于fork()
	
	pid1=fork();
	//子进程
	if(0==pid1)
	{
		pthread(pthread_test);
		while(1)
		{
			//udisp_str("child ");
			udisp_int(get_pid());
			udisp_str(" ");
			//udisp_int(--global);

			//yield();

			i=10000000;
			while(--i){}
		}
	}
	else
	{
		//pthread(pthread_test);
		while(1)
		{
			//udisp_str("father ");
			udisp_int(get_pid());
			udisp_str(" ");
			//udisp_int(++global);

			//yield();

			i=10000000;
			while(--i){}
		}		
	}	

	return ;
}
*/


/*======================================================================*
                Syscall Exec Test(最普通的情况)
added by mingxuan 2019-3-5
 *======================================================================*/

void main(int arg,char *argv[])
{
	int i=0;
	
	while(1)
	{
		
		//udisp_int(get_pid());
		//udisp_str("init0 ");

		for(i=10000000; i>0; i--) ;
	}
	return ;
}


/*======================================================================*
                    File System Test in MP
added by mingxuan 2019-3-6
 *======================================================================*/
/*
void main(int arg,char *argv[])
{
	
	int fd;
	int i, n;
	const int rd_bytes = 4;
	char filename[MAX_FILENAME_LEN+1] = "I0-0";	//modified by mingxuan 2019-3-11
	char bufr[5];
	const char bufw[] = "C0-0"; //modified by mingxuan 2019-3-12
	
	//udisp_str("\n(U)");
	fd = open(filename, O_CREAT | O_RDWR);	

	if(fd != -1)
	{
		udisp_str("File created: ");
		udisp_str(filename);
		//udisp_str(" (fd ");
		//udisp_int(fd);
		//udisp_str(")\n");	
		n = write(fd, bufw, strlen(bufw));
		if(n != strlen(bufw)) {
			udisp_str("Write error\n");
		}
		close(fd);
	}
	
	
	//udisp_str("(U)");
	fd = open(filename, O_RDWR);
	
	udisp_str("   ");
	udisp_str("File opened. fd: ");
	udisp_int(fd);
	//udisp_str("\n");

	//udisp_str("(U)");
	int lseek_status = lseek(fd, 0, SEEK_SET);
	//udisp_str("Return value of lseek is: ");
	//udisp_int(lseek_status);
	//udisp_str("  \n");

	//udisp_str("(U)");
	n = read(fd, bufr, rd_bytes);
	if(n != rd_bytes) {
		udisp_str("Read error\n");
	}
	bufr[n] = 0;
	udisp_str("Bytes read: ");
	udisp_str(bufr);
	//udisp_str("\n");

	close(fd);
	

	while (1) {
	}
	
	return;
}
*/

/*======================================================================*
                    消息队列的测试(同一颗cpu上)
added by mingxuan 2019-5-13
 *======================================================================*/
/*
#define MSGQUEUE 5151
#define MSGTYPE 7676

struct ipc_message {
	int  type;
	char *ptr;
};

struct Message {
	int value;
};

void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / 100) < milli_sec) {}
}

void Send() {

    int msqid = msgget(MSGQUEUE);

    struct ipc_message msg_snd;
	  //msg_snd.type = MSGTYPE - 10 + get_pid(); //deleted by mingxuan 2019-5-14
	  msg_snd.type = 7676;
	  msg_snd.ptr  = (char*)malloc(sizeof(struct Message));
	  //((struct Message *)msg_snd.ptr)->value = get_pid();	//deleted by mingxuan 2019-5-14
      ((struct Message *)msg_snd.ptr)->value = 0;

    while(1) {
      
      if(msgsnd(msqid, &(msg_snd), sizeof(struct Message), IPC_WAIT)<0) {
        udisp_str("proc[");
        udisp_int(get_pid());
        udisp_str("] send_ms failed!\n");
        continue;
      }	
			
      udisp_str("proc[");
      udisp_int(get_pid());
	  udisp_str("] send_ms:[type ");
      udisp_int(msg_snd.type);
      udisp_str(" value ");
	  udisp_int(((struct Message *)msg_snd.ptr)->value);
	  udisp_str("]\n");

	  ((struct Message *)msg_snd.ptr)->value += 1;

	  //milli_delay(20);	//快发慢收
	  milli_delay(2000); //慢发快收
    }

    while(1) {}
}

void Recv() {

  int msqid = msgget(MSGQUEUE);

  struct ipc_message msg_rcv;
  //msg_rcv.type = MSGTYPE - 10 + get_pid(); //deleted by mingxuan 2019-5-14
  msg_rcv.type = 7676;
  msg_rcv.ptr  = (char*)malloc(sizeof(struct Message));
  //if(msg_rcv.type % 2 == 0) {msg_rcv.type++;} else {msg_rcv.type--;} //deleted by mingxuan 2019-5-14

  while(1) {
      if(msgrcv(msqid, &(msg_rcv), sizeof(struct Message), msg_rcv.type, IPC_WAIT) < 0) {
        udisp_str("proc[");
        udisp_int(get_pid());
        udisp_str("] recv failed!\n");
        continue;
      }
			
      udisp_str("proc[");
      udisp_int(get_pid());
	  udisp_str("] recv_ms:[type ");
      udisp_int(msg_rcv.type);
      udisp_str(" value ");
	  udisp_int(((struct Message *)msg_rcv.ptr)->value);
	  udisp_str("]\n");

	  //milli_delay(2000); //快发慢收
	  milli_delay(20); //慢发快收
  }

  while(1) {}
}

void Del() {
  
  int msqid = msgget(MSGQUEUE);

  msgctl(msqid, IPC_RMID);

  while(1) {}
}


int main(int arg,char *argv[])
{
  //fork();
  //fork();
  //fork();

  //milli_delay(10000);  
  
  pthread(Send);
  Recv();

  while(1) ;
  return 0;
}
*/

/*======================================================================*
                    邮箱系统的测试(同一颗cpu上)
added by mingxuan 2019-5-14
 *======================================================================*/
/*
struct Message {
	int value;
};

void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / 100) < milli_sec) {}
}

//子线程
void rcv()
{
  struct Message msg;
  while(1) {
      	//boxrcv(0xA, &(msg), sizeof(struct Message), IPC_WAIT); //deleted by mingxuan 2019-5-15
		boxrcv(get_pid()-2, &(msg), sizeof(struct Message), IPC_WAIT);

        udisp_str("thread[");
        udisp_int(get_pid());
				udisp_str("] recv_mail:[");
        udisp_str("value ");
				udisp_int(msg.value);
				udisp_str("]\n");

      milli_delay(1000);
    }
}

int main(int arg,char *argv[])
{
  int pid = fork();

  if(pid == 0) {//子进程

    boxget();

    //建立9个子线程协助接收邮件
    pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    //pthread((void*)rcv);
    
    struct Message msg;

    while(1) {
      boxrcv(get_pid()-1, &(msg), sizeof(struct Message), IPC_WAIT);

        udisp_str("proc[");
        udisp_int(get_pid());
				udisp_str("] recv_mail:[");
        udisp_str("value ");
				udisp_int(msg.value);
				udisp_str("]\n");

      milli_delay(1000);
    }

  } 
  else {//父进程
    
    struct Message msg;
    msg.value = 0;

    milli_delay(2000);

    while(1) {
      int rv = boxsnd(pid, &(msg), sizeof(struct Message), IPC_WAIT);

        udisp_str("proc[");
        udisp_int(get_pid());
				udisp_str("] send_mail:[");
        udisp_str("value ");
				udisp_int(msg.value);
				udisp_str("]\n");

      msg.value++;

      //milli_delay(5000);
    }
  }

	while(1) {};
	return 0;
}
*/


/*======================================================================*
                    消息队列的测试(不同cpu上)-发送方(接收方为init1)
added by mingxuan 2019-5-15
 *======================================================================*/
//#include "stdio.h"
/*
#define MSGQUEUE 5151
#define MSGTYPE 7676

struct ipc_message {
	int  type;
	char *ptr;
};

struct Message {
	int value;
};

void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / 100) < milli_sec) {}
}

void Send() {

    int msqid = msgget(MSGQUEUE);

    struct ipc_message msg_snd;
	  //msg_snd.type = MSGTYPE - 10 + get_pid(); //deleted by mingxuan 2019-5-14
	  msg_snd.type = 7676;
	  msg_snd.ptr  = (char*)malloc(sizeof(struct Message));
	  //((struct Message *)msg_snd.ptr)->value = get_pid();	//deleted by mingxuan 2019-5-14
      ((struct Message *)msg_snd.ptr)->value = 0;

    while(1) {
      
      if(msgsnd(msqid, &(msg_snd), sizeof(struct Message), IPC_WAIT)<0) {
        udisp_str("proc[");
        udisp_int(get_pid());
        udisp_str("] send_ms failed!\n");
        continue;
      }	
			
      udisp_str("proc[");
      udisp_int(get_pid());
	  udisp_str("] send_ms:[type ");
      udisp_int(msg_snd.type);
      udisp_str(" value ");
	  udisp_int(((struct Message *)msg_snd.ptr)->value);
	  udisp_str("]\n");

	  ((struct Message *)msg_snd.ptr)->value += 1;

	  milli_delay(20);	   //快发慢收
	  //milli_delay(2000); //慢发快收
    }

    while(1) {}
}

void Del() {
  
  int msqid = msgget(MSGQUEUE);

  msgctl(msqid, IPC_RMID);

  while(1) {}
}


int main(int arg,char *argv[])
{
  //fork();
  //fork();
  //fork();

  //milli_delay(10000);  
  
  //pthread(Send);
  //Recv();
  Send();

  while(1) ;
  return 0;
}
*/

/*======================================================================*
                    邮箱系统的测试-发送方(接收方为init1)
added by mingxuan 2019-5-14
 *======================================================================*/
/*
#define INIT1_PID 0x10	//added by mingxuan 2019-5-15

struct Message {
	int value;
};

void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / 100) < milli_sec) {}
}

int main(int arg,char *argv[])
{
	struct Message msg;
    msg.value = 0;

    milli_delay(2000);

    while(1) {
      int rv = boxsnd(INIT1_PID, &(msg), sizeof(struct Message), IPC_WAIT);

      udisp_str("proc[");
      udisp_int(get_pid());
	  udisp_str("] send_mail:[");
      udisp_str("value ");
	  udisp_int(msg.value);
	  udisp_str("]\n");

      msg.value++;

      //milli_delay(5000);
	}

	while(1) {};
	return 0;
}
*/

/*======================================================================*
                    消息队列的测试(不同cpu)-同时收发(另一方在init1)
added by mingxuan 2019-5-15
 *======================================================================*/
/*
#define MSGQUEUE 5151
#define MSGTYPE_A2B 7676
#define MSGTYPE_B2A 7677

struct ipc_message {
	int  type;
	char *ptr;
};

struct Message {
	int value;
};

void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / 100) < milli_sec) {}
}

//A
void A() {

    int msqid = msgget(MSGQUEUE);

    struct ipc_message msg_snd;
	msg_snd.type = MSGTYPE_A2B;
	msg_snd.ptr  = (char*)malloc(sizeof(struct Message));
    ((struct Message *)msg_snd.ptr)->value = 0;

	struct ipc_message msg_rcv;
	msg_rcv.type = MSGTYPE_B2A;
	msg_rcv.ptr  = (char*)malloc(sizeof(struct Message));

    while(1) {
      
	  //发送部分
      if(msgsnd(msqid, &(msg_snd), sizeof(struct Message), IPC_WAIT)<0) {
        udisp_str("proc[");
        udisp_int(get_pid());
        udisp_str("] send_ms failed!\n");
        continue;
      }	
			
      udisp_str("proc[");
      udisp_int(get_pid());
	  udisp_str("] send_ms:[type ");
      udisp_int(msg_snd.type);
      udisp_str(" value ");
	  udisp_int(((struct Message *)msg_snd.ptr)->value);
	  udisp_str("]\n");

	  ((struct Message *)msg_snd.ptr)->value += 1;


	  //接收部分
	  if(msgrcv(msqid, &(msg_rcv), sizeof(struct Message), msg_rcv.type, IPC_WAIT) < 0) {
        udisp_str("proc[");
        udisp_int(get_pid());
        udisp_str("] recv failed!\n");
        continue;
      }

      udisp_str("proc[");
      udisp_int(get_pid());
	  udisp_str("] recv_ms:[type ");
      udisp_int(msg_rcv.type);
      udisp_str(" value ");
	  udisp_int(((struct Message *)msg_rcv.ptr)->value);
	  udisp_str("]\n");

	  milli_delay(200);
    }

    while(1) {}
}

void Del() {
  
  int msqid = msgget(MSGQUEUE);

  msgctl(msqid, IPC_RMID);

  while(1) {}
}

int main(int arg,char *argv[])
{ 
  
  A();

  while(1) ;
  return 0;
}
*/