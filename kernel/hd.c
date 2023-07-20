/// zcr copy whole file from Orange's and the file was modified.

/*************************************************************************//**
 *****************************************************************************
 * @file   hd.c
 * @brief  Hard disk (winchester) driver.
 * The `device nr' in this file means minor device nr.
 * @author Forrest Y. Yu
 * @date   2005~2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fs_misc.h"
#include "cpu.h"	//added by mingxuan 2019-1-16

//added by xw, 18/8/28
PRIVATE HDQueue hdque;
PRIVATE volatile int hd_int_waiting_flag;
PRIVATE	u8 hd_status;
PRIVATE	u8 hdbuf[SECTOR_SIZE * 2];	//硬盘读写缓冲区

PRIVATE	struct hd_info hd_info[1];	//记录硬盘的分区信息，每个硬盘应有一个hd_info结构
									//由于目前虚拟机只装了一块硬盘，所以只给一个成员

//与硬盘请求队列操作相关的函数
PRIVATE void init_hd_queue(HDQueue *hdq);
PRIVATE void in_hd_queue(HDQueue *hdq, RWInfo *p);
PRIVATE int  out_hd_queue(HDQueue *hdq, RWInfo **p);


PRIVATE void hd_rdwt_real(RWInfo *p);

PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent *entry);
PRIVATE void partition(int device, int style);
PRIVATE void print_hdinfo(struct hd_info *hdi);
PRIVATE void hd_identify(int drive);
PRIVATE void print_identify_info(u16 *hdinfo);
PRIVATE void hd_cmd_out(struct hd_cmd *cmd);

PRIVATE void inform_int();
PRIVATE void interrupt_wait();
PRIVATE void hd_handler(int irq);
PRIVATE int  waitfor(int mask, int val, int timeout);
//~xw

//功能：由设备次设备号得到驱动器号(由于只定义了一个硬盘，所以这里的启动器号一定是0)
//一个驱动对应多个主设备号，一个主设备号对应多个次设备号
#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
			 dev / NR_PRIM_PER_DRIVE : \
			 (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

/*****************************************************************************
 *                                init_hd
 *****************************************************************************/
/**
 * <Ring 1> Check hard drive, set IRQ handler, enable IRQ and initialize data
 *          structures.
 *****************************************************************************/
PUBLIC void init_hd()
{
	int i;

	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);	//8259A主片的IRQ2引脚用来连接从片
	enable_irq(AT_WINI_IRQ);	//8259A从片的IRQ14用来连接AT温盘

	//多核情形所特有
	//ioapicenable(AT_WINI_IRQ, 0);	  //打开ioapic的硬盘中断引脚，设置cpu0接受硬盘中断  //added by mingxuan 2019-3-7
	ioapicenable(IO_HD_IRQ, CPU0_ID); //打开ioapic的硬盘中断引脚，设置cpu0接受硬盘中断	//modified by mingxuan 2019-5-7

	for (i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	hd_info[0].open_cnt = 0;
	
	//init hd rdwt queue. added by xw, 18/8/27
	init_hd_queue(&hdque);	//为实现异步硬盘读写初始化请求队列hdque
}

/*****************************************************************************
 *                                hd_open
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_OPEN message. It identify the drive
 * of the given device and read the partition table of the drive if it
 * has not been read.
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PUBLIC void hd_open(int device) //传入的是一个次设备号MINOR(ROOT_DEV)
{
	disp_str("Read hd information...  ");
	
	/* Get the number of drives from the BIOS data area */
	u8 * pNrDrives = (u8*)(0x475);	//获取BIOS中检测到的系统中硬盘数量
	disp_str("NrDrives:");
	disp_int(*pNrDrives);
	disp_str("\n");
	
	int drive = DRV_OF_DEV(device);//由设备次设备号得到驱动器号(由于只定义了一个硬盘，所以这里的驱动器号一定是0)
	hd_identify(drive);

	if (hd_info[drive].open_cnt++ == 0) //判断hd_info的open_cnt成员是否为0，并将其自加
	{
		partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY); //查询硬盘分区表，将硬盘分区信息填入hd_info结构
		//print_hdinfo(&hd_info[drive]);	//deleted by mingxuan 2019-3-12
	}
}

/*****************************************************************************
 *                                hd_close
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_CLOSE message. 
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PUBLIC void hd_close(int device)
{
	int drive = DRV_OF_DEV(device); //由设备次设备号得到驱动器号(由于只定义了一个硬盘，所以这里的驱动器号一定是0)

	hd_info[drive].open_cnt--;
}


/*****************************************************************************
 *                                hd_rdwt
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_READ and DEV_WRITE message.
 * 
 * @param p Message ptr.
 *****************************************************************************/
PUBLIC void hd_rdwt(MESSAGE * p)
{
	int drive = DRV_OF_DEV(p->DEVICE); //由设备次设备号得到驱动器号(由于只定义了一个硬盘，所以这里的驱动器号一定是0)

	u64 pos = p->POSITION;

	//We only allow to R/W from a SECTOR boundary:

	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT);	// pos / SECTOR_SIZE
	int logidx = (p->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
	sect_nr += p->DEVICE < MAX_PRIM ?
		hd_info[drive].primary[p->DEVICE].base :
		hd_info[drive].logical[logidx].base;

	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
	cmd.command	= (p->type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd); //向硬盘发送读/写命令

	int bytes_left = p->CNT;
	void * la = (void*)va2la(p->PROC_NR, p->BUF); //进程的内核缓冲区指针la

	while (bytes_left) {
		int bytes = min(SECTOR_SIZE, bytes_left);
		if (p->type == DEV_READ) //读磁盘 
		{
			interrupt_wait(); //等待硬盘找扇区，读扇区上的数据到REG_DATA
			port_read(REG_DATA, hdbuf, SECTOR_SIZE); //将REG_DATA里的数据读到缓冲区hdbuf(全局变量)

			//hdbuf is in kernel space, no need to do address transferring. xw, 18/8/26
			//phys_copy(la, (void*)va2la(proc2pid(p_proc_current), hdbuf), bytes);
			phys_copy(la, hdbuf, bytes); //将缓冲区hdbuf(全局变量)中的数据读到进程指针la
		}
		else //写磁盘
		{
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				disp_str("hd writing error.");

			//modified by xw, 18/8/25
			//port_write(REG_DATA, la, bytes);
			phys_copy(hdbuf, la, bytes); //将进程指针la里的数据写到缓冲区hdbuf(全局变量)

			port_write(REG_DATA, hdbuf, SECTOR_SIZE); //将缓冲区hdbuf(全局变量)里的数据写到REG_DATA里
			interrupt_wait(); //等待硬盘找扇区，写REG_DATA上的数据到扇区
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}


//是一个内核进程,按照一定原则从读写请求队列中依次取出请求，从而实现对硬盘的串行访问
//这个进程一直被调度着
//added by xw, 18/8/26
PUBLIC void hd_service()
{
	RWInfo *rwinfo;
	
	while(1)
	{
		//若队列不为空，the hd queue is not empty when out_hd_queue return 1.
		while(out_hd_queue(&hdque, &rwinfo)) //第一步：从全局硬盘请求队列中取出请求赋给rwinfo
		{
			//第二步：调用hd_rdwt_real()处理rwinfo请求
			hd_rdwt_real(rwinfo); 

			//第三步：hd_rdwt_real()成功返回后，唤醒对应的睡眠进程
			rwinfo->proc->task.stat = READY; 
		}

		//若队列为空，则调用yield()主动放弃CPU
		yield();
		
		//disp_str("H ");		//added by mingxuan 2019-3-6
		int i;
		for(i=10000000; i>0; i--) ;
		//milli_delay(100);	//added by mingxuan 2019-3-6
	}
	
}

PRIVATE void hd_rdwt_real(RWInfo *p)
{
	int drive = DRV_OF_DEV(p->msg->DEVICE); //由设备次设备号得到驱动器号(由于只定义了一个硬盘，所以这里的驱动器号一定是0)

	u64 pos = p->msg->POSITION;

	//We only allow to R/W from a SECTOR boundary:

	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT);	// pos / SECTOR_SIZE //得到扇区号
	int logidx = (p->msg->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;	//得到逻辑分区号
	sect_nr += p->msg->DEVICE < MAX_PRIM ?
		hd_info[drive].primary[p->msg->DEVICE].base :
		hd_info[drive].logical[logidx].base;

	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count		= (p->msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low		= sect_nr & 0xFF;
	cmd.lba_mid		= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device		= MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
	cmd.command		= (p->msg->type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd); //向硬盘发送读/写命令

	int bytes_left = p->msg->CNT; //需要被读写的剩余Byte
	void *la = p->kbuf;	//attention here! //进程的内核缓冲区指针la

	while (bytes_left) 
	{
		int bytes = min(SECTOR_SIZE, bytes_left);

		if (p->msg->type == DEV_READ) //读磁盘
		{
			interrupt_wait(); //等待硬盘找扇区，读扇区上的数据到REG_DATA
			port_read(REG_DATA, hdbuf, SECTOR_SIZE); //将REG_DATA里的数据读到缓冲区hdbuf(全局变量)
			phys_copy(la, hdbuf, bytes); //将缓冲区hdbuf(全局变量)中的数据读到进程指针la
		}
		else //写磁盘
		{
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				disp_str("hd writing error.");

			phys_copy(hdbuf, la, bytes); //将进程指针la里的数据写到缓冲区hdbuf(全局变量)
			port_write(REG_DATA, hdbuf, SECTOR_SIZE); //将缓冲区hdbuf(全局变量)里的数据写到REG_DATA里
			interrupt_wait(); //等待硬盘找扇区，写REG_DATA上的数据到扇区
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}

PUBLIC void hd_rdwt_sched(MESSAGE *p)
{
	RWInfo rwinfo; //硬盘读写请求
	struct memfree hdque_buf;
	int size = p->CNT;

	void *buffer; //内核数据缓冲区
	buffer = (void*)K_PHY2LIN(sys_kmalloc(size));

	//第一步：将进程的硬盘读写请求进行打包成rwinfo
	rwinfo.msg = p;
	rwinfo.kbuf = buffer;
	//rwinfo.proc = p_proc_current;	//deleted by mingxuan 2019-1-16
	rwinfo.proc = proc;	//modified by mingxuan 2019-1-16
	
	if (p->type == DEV_READ) //读操作
	{
		//第二步：将硬盘读写请求rwinfo插入到请求队列hdque
		in_hd_queue(&hdque, &rwinfo);

		//第三步：改变进程PCB内的状态字段，使进程进入SLEEPING状态
		proc->task.channel = &hdque; //modified by mingxuan 2019-1-16
		proc->task.stat = SLEEPING; //modified by mingxuan 2019-1-16
		//p_proc_current->task.channel = &hdque; //deleted by mingxuan 2019-1-16
		//p_proc_current->task.stat = SLEEPING; //deleted by mingxuan 2019-1-16

		//第四步：调用sched()主动放弃CPU
		sched();

		//第五步：从buffer中读数据到进程空间
		phys_copy(p->BUF, buffer, p->CNT);
	} 
	else //写操作
	{
		//第二步：从进程空间中读数据到buffer
		phys_copy(buffer, p->BUF, p->CNT);

		//第三步：将硬盘读写请求rwinfo插入到请求队列hdque
		in_hd_queue(&hdque, &rwinfo);

		//第四步：改变进程PCB内的状态字段，使进程进入SLEEPING状态
		proc->task.channel = &hdque; //modified by mingxuan 2019-1-16
		proc->task.stat = SLEEPING;	//modified by mingxuan 2019-1-16
		//p_proc_current->task.channel = &hdque; //deleted by mingxuan 2019-1-16
		//p_proc_current->task.stat = SLEEPING;	//deleted by mingxuan 2019-1-16

		//第四步：调用sched()主动放弃CPU
		//如果只有一个进程？忙等待
		sched();
	}
	
	hdque_buf.addr = K_LIN2PHY((u32)buffer);
	hdque_buf.size = size;

	sys_free(&hdque_buf);
}

PUBLIC void init_hd_queue(HDQueue *hdq)
{
	hdq->front = hdq->rear = NULL;
}

PRIVATE void in_hd_queue(HDQueue *hdq, RWInfo *p)
{
	p->next = NULL;
	if(hdq->rear == NULL) {	//put in the first node
		hdq->front = hdq->rear = p;
	} else {
		hdq->rear->next = p;
		hdq->rear = p;
	}
}

PRIVATE int out_hd_queue(HDQueue *hdq, RWInfo **p)
{
	if (hdq->rear == NULL)
		return 0;	//empty
	
	*p = hdq->front;
	if (hdq->front == hdq->rear) {	//put out the last node
		hdq->front = hdq->rear = NULL;
	} else {
		hdq->front = hdq->front->next;
	}
	return 1;	//not empty
}
//~xw

/*****************************************************************************
 *                                hd_ioctl
 *****************************************************************************/
/**
 * <Ring 1> This routine handles the DEV_IOCTL message.
 * 
 * @param p  Ptr to the MESSAGE.
 *****************************************************************************/
PUBLIC void hd_ioctl(MESSAGE *p)
{
	int device = p->DEVICE;
	int drive = DRV_OF_DEV(device); //由设备次设备号得到驱动器号(由于只定义了一个硬盘，所以这里的驱动器号一定是0)

	struct hd_info *hdi = &hd_info[drive];

	if (p->REQUEST == DIOCTL_GET_GEO) //把请求的设备的起始扇区和扇区数目返回给调用者
	{
		void *dst = va2la(p->PROC_NR, p->BUF);
		//void *src = va2la(proc2pid(p_proc_current),  //deleted by mingxuan 2019-1-16
		void *src = va2la(proc2pid(proc), 			   //modified by mingxuan 2019-1-16
				   device < MAX_PRIM ?
				   &hdi->primary[device] :
				   &hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE]);

		phys_copy(dst, src, sizeof(struct part_info)); //struct hd_info *hdi中的struct part_info logical -> struct part_info geo
	}
	else {
		// assert(0);
	}
}

/*****************************************************************************
 *                                get_part_table
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive.
 * 
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sect_nr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hd_cmd_out(&cmd);
	interrupt_wait();

	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,				//PARTITION_TABLE_OFFSET是硬盘分区表在引导扇区中的偏移
	       sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

/*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 * 
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
PRIVATE void partition(int device, int style)
{
	int i;
	int drive = DRV_OF_DEV(device);
	struct hd_info * hdi = &hd_info[drive];

	struct part_ent part_tbl[NR_SUB_PER_DRIVE];

	if (style == P_PRIMARY) 
	{
		get_part_table(drive, drive, part_tbl); //获取硬盘分区表

		int nr_prim_parts = 0;
		for (i = 0; i < NR_PART_PER_DRIVE; i++) { /* 0~3 */
			if (part_tbl[i].sys_id == NO_PART) 
				continue;

			nr_prim_parts++;
			int dev_nr = i + 1;		  /* 1~4 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect; //填充hd_info结构
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;	//填充hd_info结构

			if (part_tbl[i].sys_id == EXT_PART) /* extended */
				partition(device + dev_nr, P_EXTENDED);
		}
	}
	else if (style == P_EXTENDED) 
	{
		int j = device % NR_PRIM_PER_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

		for (i = 0; i < NR_SUB_PER_PART; i++) {
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */

			get_part_table(drive, s, part_tbl); //获取硬盘分区表

			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect; //填充hd_info结构
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;		//填充hd_info结构

			s = ext_start_sect + part_tbl[1].start_sect;

			/* no more logical partitions
			   in this extended partition */
			if (part_tbl[1].sys_id == NO_PART)
				break;
		}
	}
	else {
		// assert(0);
	}
}

/*****************************************************************************
 *                                print_hdinfo
 *****************************************************************************/
/**
 * <Ring 1> Print disk info.
 * 
 * @param hdi  Ptr to struct hd_info.
 *****************************************************************************/
PRIVATE void print_hdinfo(struct hd_info * hdi)
{
	int i;
	for (i = 0; i < NR_PART_PER_DRIVE + 1; i++) 
	{
		if(i == 0) {	
			disp_str(" ");
		}
		else {
			disp_str("     ");
		}
		disp_str("PART_");
		disp_int(i);
		disp_str(": base ");
		disp_int(hdi->primary[i].base);
		disp_str("), size");
		disp_int(hdi->primary[i].size);
		disp_str(" (in sector)\n");
	}
	for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
		if (hdi->logical[i].size == 0)
			continue;
		disp_str("         ");
		disp_int(i);
		disp_str(": base ");
		disp_int(hdi->logical[i].base);
		disp_str(", size ");
		disp_int(hdi->logical[i].size);
		disp_str(" (in sector)\n");
	}
}

/*****************************************************************************
 *                                hd_identify
 *****************************************************************************/
/**
 * <Ring 1> Get the disk information.
 * 
 * @param drive  Drive Nr.
 *****************************************************************************/
PRIVATE void hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0); //设置REG_DATA
	cmd.command = ATA_IDENTIFY; //设置REG_STATUS
	hd_cmd_out(&cmd);

	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

	print_identify_info((u16 *)hdbuf);

	u16 *hdinfo = (u16 *)hdbuf;

	hd_info[drive].primary[0].base = 0;
	/* Total Nr of User Addressable Sectors */
	hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

/*****************************************************************************
 *                            print_identify_info
 *****************************************************************************/
/**
 * <Ring 1> Print the hdinfo retrieved via ATA_IDENTIFY command.
 * 
 * @param hdinfo  The buffer read from the disk i/o port.
 *****************************************************************************/
PRIVATE void print_identify_info(u16* hdinfo)
{
	int i, k;
	char s[64];

	struct iden_info_ascii {
		int idx;
		int len;
		char * desc;
	} iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
		     {27, 40, "HD Model"} /* Model number in ASCII */ };

	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) {
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++) {
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		// printl("%s: %s\n", iinfo[k].desc, s);
		disp_str(iinfo[k].desc);
		disp_str(":");
		disp_str(s);
		disp_str("\n");
	}

	int capabilities = hdinfo[49];
	// printl("LBA supported: %s\n", (capabilities & 0x0200) ? "Yes" : "No");
	disp_str("LBA supported:");
	if((capabilities & 0x0200))
		disp_str("YES  ") ;
	else disp_str("NO  ");
	// disp_str("\n");

	int cmd_set_supported = hdinfo[83];
	// printl("LBA48 supported: %s\n", (cmd_set_supported & 0x0400) ? "Yes" : "No");
	disp_str("LBA48 supported:");
	if((cmd_set_supported & 0x0400))
		disp_str("YES  ");
	else disp_str("NO  ");
	// disp_str("\n");

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	// printl("HD size: %dMB\n", sectors * 512 / 1000000);
	disp_str("HD size:");
	disp_int(sectors * 512 / 1000000);
	disp_str("MB\n");
}

/*****************************************************************************
 *                                hd_cmd_out
 *****************************************************************************/
/**
 * <Ring 1> Output a command to HD controller.
 * 
 * @param cmd  The command struct ptr.
 *****************************************************************************/
PRIVATE void hd_cmd_out(struct hd_cmd* cmd)
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT)) //先判断REG_STATUS的BSY位，只有这位为0时才能发送命令
		disp_str("hd error.");

	/* Activate the Interrupt Enable (nIEN) bit */
	out_byte(REG_DEV_CTRL, 0); //先通过REG_DEV_CTRL打开中断

	/* Load required parameters in the Command Block Registers */
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);

	/* Write the command code to the Command Register */
	out_byte(REG_CMD,     cmd->command); //一旦写入REG_CMD，命令就被硬盘执行了
}

/*****************************************************************************
 *                                interrupt_wait
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 * 
 *****************************************************************************/
/// modified by zcr(using Huper's method.)
// PUBLIC void interrupt_wait()
// {
// 	while(hd_int_waiting_flag) {
// 		// milli_delay(20);/// waiting for the harddisk interrupt.
// 	}
// 	hd_int_waiting_flag = 1;
// }

//	/*
PRIVATE void interrupt_wait()
{
	while(hd_int_waiting_flag) {
		// milli_delay invoke syscall get_ticks, so we can't use it here.
		// for this scene, just do nothing is OK. modified by xw, 18/6/1
		
		//milli_delay(5);/// waiting for the harddisk interrupt.
	}
	hd_int_waiting_flag = 1;
}
//	*/

	/*
//added by xw, 18/8/16
PUBLIC void interrupt_wait_sched()
{
	while(hd_int_waiting_flag){
		sched();
	}
	hd_int_waiting_flag = 1;
}	
//	*/


/*****************************************************************************
 *                                waitfor
 *****************************************************************************/
/**
 * <Ring 1> Wait for a certain status.
 * 
 * @param mask    Status mask.
 * @param val     Required status.
 * @param timeout Timeout in milliseconds.
 * 
 * @return One if sucess, zero if timeout.
 *****************************************************************************/
PRIVATE int waitfor(int mask, int val, int timeout)
{
	//we can't use syscall get_ticks before process run. modified by xw, 18/5/31
	/*
	int t = get_ticks();
	
	while(((get_ticks() - t) * 1000 / HZ) < timeout)
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;
	*/
	int t = sys_get_ticks();
	
	while(((sys_get_ticks() - t) * 1000 / HZ) < timeout){
		if ((in_byte(REG_STATUS) & mask) == val)
			return 1;
	}
	
	return 0;
}

/*****************************************************************************
 *                                hd_handler
 *****************************************************************************/
/**
 * <Ring 0> Interrupt handler.
 * 
 * @param irq  IRQ nr of the disk interrupt.
 *****************************************************************************/
PRIVATE void hd_handler(int irq)	//硬盘中断处理程序
{
	/*
	 * Interrupts are cleared when the host
	 *   - reads the Status Register,
	 *   - issues a reset, or
	 *   - writes to the Command Register.
	 */
	hd_status = in_byte(REG_STATUS); //通过读Status寄存器来恢复中断响应
	inform_int();	//通知驱动程序可以继续向下执行
	
	/* There is two stages - in kernel intializing or in process running.
	 * Some operation shouldn't be valid in kernel intializing stage.
	 * added by xw, 18/6/1
	 */
	if(kernel_initial == 1){
		return;
	}
	
	//some operation only for process
	
	return;
}

/*****************************************************************************
 *                                inform_int
 *****************************************************************************/
PRIVATE void inform_int()
{
	hd_int_waiting_flag = 0;
	return;
}
