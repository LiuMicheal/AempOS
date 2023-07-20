/******************************************************
*	init1进程		added by mingxuan 2019-3-14
*******************************************************/

#include "stdio.h"

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
			//udisp_str("child1 ");
			udisp_int(get_pid());
			//udisp_int(++global);

			//yield();

			i=10000000;
			while(--i){}
		}
	} 

	pid2 = fork();
	if (pid2 == 0)
	{	
		while(1)
		{
			//udisp_str("child2 ");
			udisp_int(get_pid());
			//udisp_int(--global);

			//yield();

			i=10000000;
			while(--i){}
		}
	} 

	pid3 = fork();
	if (pid3 == 0)
	{	
		while(1)
		{
			//udisp_str("child3 ");
			udisp_int(get_pid());

			//yield();

			i=10000000;
			while(--i){}
		}
	} 

	while(1)
	{
		//udisp_str("father ");
		udisp_int(get_pid());

		//yield();

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
		//udisp_str("pthread1 ");
		udisp_int(get_pid());
		//udisp_str(" ");
		
		//yield();

		//udisp_int(++global);
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
		//udisp_str("pthread2 ");
		udisp_int(get_pid());
		//udisp_str(" ");
		
		//yield();

		//udisp_int(--global);
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
		//udisp_str("pthread3 ");
		udisp_int(get_pid());
		//udisp_str(" ");
		
		//yield();

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
		//udisp_str("father ");
		//udisp_str(" ");
		udisp_int(get_pid());
		//udisp_int(get_cpuid());
		//udisp_str(" ");

		//yield();

		i=10000000;
		while(--i){}			
	}

	while(1);

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
		//udisp_str(" ");	

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
		//udisp_str(" ");
		
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
		//udisp_str(" ");
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
		//udisp_str(" ");
		i=10000000;
		while(--i){}
	}
	return 0;
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
					//udisp_str("child3 ");
					udisp_int(get_pid());
					//udisp_str(" ");
					//udisp_int(++global);

					//yield();

					i=10000000;
					while(--i){}		
				}
			}
			else
			{
				while(1)
				{
					//udisp_str("child2 ");
					udisp_int(get_pid());
					//udisp_str(" ");
					//udisp_int(--global);

					//yield();

					i=10000000;
					while(--i){}		
				}	
			}
		}
		else
		{
			while(1)
			{
				//udisp_str("child1 ");
				udisp_int(get_pid());
				//udisp_str(" ");

				//yield();

				i=10000000;
				while(--i){}
			}
		}
	}
	else
	{
		while(1)
		{
			//udisp_str("father ");
			udisp_int(get_pid());
			//udisp_str(" ");

			//yield();

			i=10000000;
			while(--i){}
		}		
	}	
	
	return ;
}
*/

/*======================================================================*
        Syscall Fork Test in MP 测试在fork中嵌套pthread的功能(含共享变量测试)
added by mingxuan 2019-2-28
 *======================================================================*/
/*
int global = 0x500;

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
		//udisp_int(get_cpuid());
		//udisp_str("init3 ");

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
	
	//延时:init1需要等待cpu0执行restart_initial进去调度
	int j=100000000;
	for(;j>0;j--);

	int fd;
	int i, n;
	const int rd_bytes = 4;
	char filename[MAX_FILENAME_LEN+1] = "I3-0";	//modified by mingxuan 2019-3-11
	char bufr[5];
	const char bufw[] = "C3-0"; //modified by mingxuan 2019-3-12

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
       	 Syscall Fork Test in MP 测试负载均衡(5次fork)(含共享变量测试)
added by mingxuan 2019-3-1
 *======================================================================*/
/*
int global = 0x100;
void main(int arg,char *argv[])
{
	int pid1=0, pid2=0, pid3=0, pid4=0, pid5=0, pid6=0;
	int i;
	//int global = 0x100;
 
	pid1 = fork(); 
	if (pid1 == 0)
	{	
		while(1)
		{
			//udisp_str("child1 ");
			udisp_int(get_pid());
			//udisp_int(++global);

			i=10000000;
			while(--i){}
		}
	} 

	pid2 = fork();
	if (pid2 == 0)
	{	
		while(1)
		{
			//udisp_str("child2 ");
			udisp_int(get_pid());
			//udisp_int(--global);

			i=10000000;
			while(--i){}
		}
	} 

	pid3 = fork();
	if (pid3 == 0)
	{	
		while(1)
		{
			//udisp_str("child3 ");
			udisp_int(get_pid());

			i=10000000;
			while(--i){}
		}
	} 

	pid4 = fork();
	if (pid4 == 0)
	{	
		while(1)
		{
			//udisp_str("child3 ");
			udisp_int(get_pid());

			i=10000000;
			while(--i){}
		}
	} 

	pid5 = fork();
	if (pid5 == 0)
	{	
		while(1)
		{
			//udisp_str("child3 ");
			udisp_int(get_pid());

			i=10000000;
			while(--i){}
		}
	} 

	pid6 = fork();
	if (pid6 == 0)
	{	
		while(1)
		{
			//udisp_str("child3 ");
			udisp_int(get_pid());

			i=10000000;
			while(--i){}
		}
	} 

	while(1)
	{
		//udisp_str("father ");
		udisp_int(get_pid());

		i=10000000;
		while(--i){}			
	}
	
	while(1);

	return ;
}
*/