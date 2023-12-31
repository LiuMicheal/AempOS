/// zcr copy from chapter9/d fs/main.c and modified it.

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
#include "cpu.h"	   //added by mingxuan 2019-1-16
#include "semaphore.h" //added by mingxuan 2019-4-1

/* FSBUF_SIZE is defined as macro in fs_const.h.
 * The physical address space 6MB~7MB is used as fs buffer in Orange's, but we can't use this
 * space directly in minios. We allocate the fs buffer space in kernel initialization stage. 
 * modified by xw, 18/6/15
 */
// PUBLIC const int	FSBUF_SIZE	= 0x100000;
// PUBLIC u8 * fsbuf = (u8*)0x600000;

//MESSAGE	fs_msg;		//deleted by xw, 18/8/27
//PROCESS* pcaller;		//deleted by xw, 18/8/27

/* fsbuf is a global memory area, could cause data race when multiple processes access 
 * disk by using read()/write() concurrently. so delete it and use local variables instead.
 * added by xw, 18/12/27
 */
//PRIVATE u8* fsbuf;

//added by xw, 18/8/28
/* data */
PRIVATE struct inode	   *root_inode;
PRIVATE struct file_desc   f_desc_table[NR_FILE_DESC];
PRIVATE struct inode 	   inode_table[NR_INODE]; 		//存放内存中所有inode的内存缓冲区，对inode的操作就是对inode_table的操作
PRIVATE struct super_block super_block[NR_SUPER_BLOCK]; //存放内存中所有super_block的内存缓冲区，对sb的操作就是对super_block的操作

/* functions */
PRIVATE void mkfs();
PRIVATE void read_super_block(int dev);
PRIVATE struct super_block* get_super_block(int dev);

PRIVATE int do_open(MESSAGE *fs_msg);
PRIVATE int do_close(int fd);
PRIVATE int do_lseek(MESSAGE *fs_msg);
PRIVATE int do_rdwt(MESSAGE *fs_msg);
PRIVATE int do_unlink(MESSAGE *fs_msg);

PRIVATE int real_open(const char *pathname, int flags);
PRIVATE int real_close(int fd);
PRIVATE int real_read(int fd, void *buf, int count);
PRIVATE int real_write(int fd, const void *buf, int count);
PRIVATE int real_unlink(const char *pathname);
PRIVATE int real_lseek(int fd, int offset, int whence);

PRIVATE int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf);
PRIVATE int rw_sector_sched(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf);

PRIVATE int strip_path(char * filename, const char * pathname, struct inode** ppinode);
PRIVATE int search_file(char *path);
PRIVATE struct inode* create_file(char *path, int flags);
PRIVATE struct inode* get_inode(int dev, int num);
PRIVATE struct inode* get_inode_sched(int dev, int num);
PRIVATE struct inode* new_inode(int dev, int inode_nr, int start_sect);
PRIVATE void put_inode(struct inode *pinode);
PRIVATE void sync_inode(struct inode *p);
PRIVATE void new_dir_entry(struct inode *dir_inode,int inode_nr,char *filename);
PRIVATE int alloc_imap_bit(int dev);
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc);

PRIVATE int memcmp(const void *s1, const void *s2, int n);
PRIVATE int strcmp(const char *s1, const char *s2);
//~xw

/// zcr added
PUBLIC void init_fs() 
{
	disp_str("Initializing file system...  ");
	
	//allocate fs buffer. added by xw, 18/6/15
	//fsbuf = (u8*)K_PHY2LIN(sys_kmalloc(FSBUF_SIZE)); //deleted by xw, 18/12/27

	int i;
	for (i = 0; i < NR_FILE_DESC; i++)
		memset(&f_desc_table[i], 0, sizeof(struct file_desc));
	for (i = 0; i < NR_INODE; i++)
		memset(&inode_table[i], 0, sizeof(struct inode));
	struct super_block * sb = super_block;
	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
		sb->sb_dev = NO_DEV;
	
	/* load super block of ROOT */
	read_super_block(ROOT_DEV);		//将ROOT设备的超级块从硬盘读入内存(调用了hd_rdwt)
	sb = get_super_block(ROOT_DEV); //得到给定设备的超级块指针
	disp_str("Superblock Address:");
	disp_int(sb);
	disp_str(" \n");

	if(sb->magic != MAGIC_V1)
	{
		mkfs();
		disp_str("Make file system Done.\n");
		for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
			sb->sb_dev = NO_DEV;
		read_super_block(ROOT_DEV);
	}

	root_inode = get_inode(ROOT_DEV, ROOT_INODE); //最终调用了hd_rdwt(&driver_msg)
}

/*****************************************************************************
 *                                mkfs
 *****************************************************************************/
/**
 * <Ring 1> Make a available Orange'S FS in the disk. It will
 *          - Write a super block to sector 1.
 *          - Create three special files: dev_tty0, dev_tty1, dev_tty2
 *          - Create the inode map
 *          - Create the sector map
 *          - Create the inodes of the files
 *          - Create `/', the root directory
 *****************************************************************************/
PRIVATE void mkfs()
{
	MESSAGE driver_msg;
	int i, j;

	int bits_per_sect = SECTOR_SIZE * 8; //4096为每一个扇区的bit数，512*8=4096bits

	//local array, to substitute global fsbuf. added by xw, 18/12/27
	char fsbuf[SECTOR_SIZE]; //一个扇区大小(512Byte)的缓冲区

	/********************************************************/
	/*    第一步：向硬盘驱动程序索取ROOT_DEV的起始扇区和总的扇区个数    */
	/********************************************************/
	/* get the geometry of ROOTDEV */
	struct part_info geo; //存储ROOT_DEV的起始扇区和扇区个数

	driver_msg.type		= DEV_IOCTL;
	driver_msg.DEVICE	= MINOR(ROOT_DEV);
	driver_msg.REQUEST	= DIOCTL_GET_GEO;
	driver_msg.BUF		= &geo;
	//driver_msg.PROC_NR= proc2pid(p_proc_current); //deleted by mingxuan 2019-1-16
	driver_msg.PROC_NR	= proc2pid(proc); 			//modified by mingxuan 2019-1-16

	hd_ioctl(&driver_msg); //执行过后，在part_info geo中就存储了hd_info中的part_info logical
						   //其中，geo.base为起始扇区，geo.size为文件系统中总共的扇区数目

	/********************************/
	/*    第二步：建立super_block     */
	/********************************/
	struct super_block sb;
	sb.magic	  	  = MAGIC_V1;
	sb.nr_inodes	  = bits_per_sect;	//文件系统中最多允许有4096个inode，这样只需要1个扇区来做inode-map就足够了，同时意味着文件系统最多容纳4096个文件
	sb.nr_imap_sects  = 1; //文件系统中最多允许有4096个inode，这样只需要1个扇区来做inode-map就足够了

	sb.nr_inode_sects = sb.nr_inodes * INODE_SIZE / SECTOR_SIZE; //存储inode_array的扇区个数=4096*32/512=256个

	sb.nr_sects	      = geo.size; //硬盘驱动程序，文件系统中总共的扇区个数为40257(0x9D41)个
	sb.nr_smap_sects  = sb.nr_sects / bits_per_sect + 1; //40527/4096+1=10个

	sb.n_1st_sect	  = 1 + 1 + sb.nr_imap_sects + sb.nr_smap_sects + sb.nr_inode_sects; //第一个数据扇区的扇区号=1+1+1+10+256=269个扇区=0xA01800

	sb.root_inode	  = ROOT_INODE; //根目录文件的inode编号为1

	/* 和inode相关的成员 */
	sb.inode_size	  = INODE_SIZE; 	//每个inode的大小为32Byte
	struct inode x;
	sb.inode_isize_off= (int)&x.i_size - (int)&x;
	sb.inode_start_off= (int)&x.i_start_sect - (int)&x;

	/* 和dir_ent相关的成员 */
	sb.dir_ent_size	  = DIR_ENTRY_SIZE; //每个dir_ent的大小为16Byte
	struct dir_entry de;
	sb.dir_ent_inode_off = (int)&de.inode_nr - (int)&de;
	sb.dir_ent_fname_off = (int)&de.name - (int)&de;

	memset(fsbuf, 0x90, SECTOR_SIZE);//除了使用的前56Byte外，其余各字节都初始化成0x90，这样可以顺便测试一下写扇区功能
	memcpy(fsbuf, &sb,  SUPER_BLOCK_SIZE);

	/* write the super block */
	WR_SECT(ROOT_DEV, 1, fsbuf);	//added by xw, 18/12/27

	/********************************/
	/*    第三步：建立inode-map       */
	/********************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 0; i < (NR_CONSOLES + 2); i++) //循环5次：第0个inode保留置1，第1个是ROOT_INODE置1，三个dev_tty都置1
		fsbuf[0] |= 1 << i;

	//WR_SECT(ROOT_DEV, 2);
	WR_SECT(ROOT_DEV, 2, fsbuf);	//modified by xw, 18/12/27

	/*********************************/
	/*    第四步：建立sector-map       */
	/*********************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	int nr_sects = NR_DEFAULT_FILE_SECTS + 1; //根目录占据NR_DEFAULT_FILE_SECTS个扇区+第0位是保留位 = 2049个扇区

	for (i = 0; i < nr_sects / 8; i++)
		fsbuf[i] = 0xFF;

	for (j = 0; j < nr_sects % 8; j++)
		fsbuf[i] |= (1 << j);

	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects, fsbuf);	//modified by xw, 18/12/27

	/* zeromemory the rest sector-map */
	memset(fsbuf, 0, SECTOR_SIZE);
	for (i = 1; i < sb.nr_smap_sects; i++)
		WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + i, fsbuf);	//modified by xw, 18/12/27

	/*********************************/
	/*    第五步：写入inode_array      */
	/*********************************/
	/* inode of `/' */
	memset(fsbuf, 0, SECTOR_SIZE);
	struct inode *pi = (struct inode*)fsbuf;//指向fsbuf的第一项(32Byte/项),此时的fsbuf用来缓存inode
	pi->i_mode = I_DIRECTORY;
	pi->i_size = DIR_ENTRY_SIZE * 4; /* 4 files: (预定义四个文件)
					  * `.',
					  * `dev_tty0', `dev_tty1', `dev_tty2',
					  */
	pi->i_start_sect = sb.n_1st_sect;
	pi->i_nr_sects = NR_DEFAULT_FILE_SECTS;	//每个文件的默认大小是2048个扇区=2048*512=1MB

	/* inode of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) 
	{//依次填写inode
		pi = (struct inode*)(fsbuf + (INODE_SIZE * (i + 1))); //指向fsbuf的下一项(32Byte/项)，此时的fsbuf用来缓存inode
		pi->i_mode = I_CHAR_SPECIAL;
		pi->i_size = 0;
		pi->i_start_sect = MAKE_DEV(DEV_CHAR_TTY, i);
		pi->i_nr_sects = 0;
	}
	WR_SECT(ROOT_DEV, 2 + sb.nr_imap_sects + sb.nr_smap_sects, fsbuf);	//modified by xw, 18/12/27

	/***********************************************************/
	/*      第六步：建立根目录文件(根目录文件中存储的全是目录项)       */
	/***********************************************************/
	memset(fsbuf, 0, SECTOR_SIZE);
	struct dir_entry *pde = (struct dir_entry *)fsbuf; //指向fsbuf的第一项(16Byte/项),此时的fsbuf用来缓存dir_entry

	pde->inode_nr = 1;
	strcpy(pde->name, ".");

	/* dir entries of `/dev_tty0~2' */
	for (i = 0; i < NR_CONSOLES; i++) 
	{//依次填写dir_entry
		pde++;
		pde->inode_nr = i + 2; /* dev_tty0's inode_nr is 2 */
		switch(i) 
		{
			case 0:	
				strcpy(pde->name, "dev_tty0"); 
				break;
			case 1:
				strcpy(pde->name, "dev_tty1"); 
				break;
			case 2:
				strcpy(pde->name, "dev_tty2"); 
				break;
		}
	}
	WR_SECT(ROOT_DEV, sb.n_1st_sect, fsbuf);	//modified by xw, 18/12/27
}

/*****************************************************************************
 *                                rw_sector
 *****************************************************************************/
/**
 * <Ring 1> R/W a sector via messaging with the corresponding driver.
 * 
 * @param io_type  DEV_READ or DEV_WRITE
 * @param dev      device nr
 * @param pos      Byte offset from/to where to r/w.
 * @param bytes    r/w count in bytes.
 * @param proc_nr  To whom the buffer belongs.
 * @param buf      r/w buffer.
 * 
 * @return Zero if success.
 *****************************************************************************/
/// zcr: change the "u64 pos" to "int pos"
PRIVATE int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	
	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);

	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;

	hd_rdwt(&driver_msg);
	return 0;
}


//added by xw, 18/8/27
PRIVATE int rw_sector_sched(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf)
{
	MESSAGE driver_msg;
	
	driver_msg.type		= io_type;
	driver_msg.DEVICE	= MINOR(dev);

	driver_msg.POSITION	= pos;
	driver_msg.CNT		= bytes;	/// hu is: 512
	driver_msg.PROC_NR	= proc_nr;
	driver_msg.BUF		= buf;
	
	hd_rdwt_sched(&driver_msg);
	return 0;
}
//~xw

// added by zcr from chapter9/e/lib/open.c and modified it.

/*****************************************************************************
 *                                open
 *****************************************************************************/
/**
 * open/create a file.
 * 
 * @param pathname  The full path of the file to be opened/created.
 * @param flags     O_CREAT, O_RDWR, etc.
 * 
 * @return File descriptor if successful, otherwise -1.
 *****************************************************************************/
//open is a syscall interface now. added by xw, 18/6/18
// PUBLIC int open(const char *pathname, int flags)
PRIVATE int real_open(const char *pathname, int flags)
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;

	fs_msg.type	= OPEN;
	fs_msg.PATHNAME	= (void*)pathname;
	fs_msg.FLAGS	= flags;
	fs_msg.NAME_LEN	= strlen(pathname);
	//fs_msg.source = proc2pid(p_proc_current);	//deleted by mingxuan 2019-1-16
	fs_msg.source = proc2pid(proc);	//modified by mingxuan 2019-1-16

	//int fd = do_open();	//modified by xw, 18/8/27
	int fd = do_open(&fs_msg);

	return fd;
}

/*****************************************************************************
 *                                do_open
 *****************************************************************************/
/**
 * Open a file and return the file descriptor.
 * 
 * @return File descriptor if successful, otherwise a negative error code.
 *****************************************************************************/
/// zcr modified.
PRIVATE int do_open(MESSAGE *fs_msg)
{
	/*caller_nr is the process number of the */
	int fd = -1;		/* return value */
	char pathname[MAX_PATH];

	/* 第一步：get parameters from the message */
	int flags = fs_msg->FLAGS;		 /* access mode */
	int name_len = fs_msg->NAME_LEN; /* length of filename */
	int src = fs_msg->source;		 /* caller proc nr. */
	phys_copy((void*)va2la(src, pathname),
			  (void*)va2la(src, fs_msg->PATHNAME), 
			  name_len);
	pathname[name_len] = 0;

	/* 第二步：find a free slot in PROCESS::filp[] */
	int i;
	//for (i = 0; i < NR_FILES; i++) {
	for (i = 3; i < NR_FILES; i++) 
	{// 0, 1, 2 are reserved for stdin, stdout, stderr. modified by xw, 18/8/28 
		//if (p_proc_current->task.filp[i] == 0) { //deleted by mingxuan 2019-1-16
		if (proc->task.filp[i] == 0) { 			   //modified by mingxuan 2019-1-16
			fd = i;
			break;
		}
	}
	if ((fd < 0) || (fd >= NR_FILES)) //错误处理
	{
		disp_str("filp[] is full (PID:");
		//disp_int(proc2pid(p_proc_current)); //deleted by mingxuan 2019-1-16
		disp_int(proc2pid(proc)); 			  //modified by mingxuan 2019-1-16
		disp_str(")\n");
	}

	/* 第三步：find a free slot in f_desc_table[] */
	for (i = 0; i < NR_FILE_DESC; i++)
		if (f_desc_table[i].fd_inode == 0) 
		{
			f_desc_table[i].fd_inode = -1;	//to decrease the chance of two process finding the same free slot
											//a lock should be used here in the future. added by xw, 18/12/28 
			break;
		}
	if (i >= NR_FILE_DESC) //错误处理
	{
		disp_str("f_desc_table[] is full (PID:");
		//disp_int(proc2pid(p_proc_current)); //deleted by mingxuan 2019-1-16
		disp_int(proc2pid(proc)); //modified by mingxuan 2019-1-16
		disp_str(")\n");
	}

	int inode_nr = search_file(pathname); 
	struct inode *pin = 0;
	//根据输入的属性位进行检查
	if (flags & O_CREAT) //用户要求新建一个文件
	{
		if (inode_nr) //如果inode编号不为0
		{
			disp_str("file exists.\n");	/// zcr
			return -1;
		}
		else 
		{
			pin = create_file(pathname, flags);
			//disp_str("create a new file done.\n");
		}
	}
	else //该文件已存在，用户不要求新建一个文件
	{
		char filename[MAX_PATH]; //去掉路径的纯的文件名 //strip_path的输出
		struct inode *dir_inode; //目录文件的inode    //strip_path的输出
		if (strip_path(filename, pathname, &dir_inode) != 0) //执行后，dir_inode已是目录文件的inode，同时filename是纯文件名
			return -1;
		pin = get_inode_sched(dir_inode->i_dev, inode_nr);	//modified by xw, 18/8/28
		//disp_str("get the i-node of a file already exists.\n");
	}

	//将PCB的flip[fd]、f_desc_table[i]、inode三者连接
	if (pin) 
	{
		/* 第一步：connects proc with file_descriptor */
		//p_proc_current->task.filp[fd] = &f_desc_table[i]; //deleted by mingxuan 2019-1-16
		proc->task.filp[fd] = &f_desc_table[i];

		/* 第二步：connects file_descriptor with inode */
		f_desc_table[i].fd_inode = pin;
		f_desc_table[i].fd_mode = flags;
		f_desc_table[i].fd_pos = 0;

		// 第三步：根据文件类型采取不同处理
		int imode = pin->i_mode & I_TYPE_MASK;
		if (imode == I_CHAR_SPECIAL) //情况一：字符特殊文件
		{
			MESSAGE driver_msg;
			int dev = pin->i_start_sect;
			hd_open(MINOR(dev));
		}
		else if (imode == I_DIRECTORY) //情况二：目录文件
		{
			if(pin->i_num != ROOT_INODE) 
			{
				disp_str("Panic: pin->i_num != ROOT_INODE");
			}
		}
		else //情况三：普通文件
		{
			if(pin->i_mode != I_REGULAR)
			{
				disp_str("Panic: pin->i_mode != I_REGULAR");
			}
		}
	}
	else 
	{
		return -1;
	}

	return fd;
}

/*****************************************************************************
 *                                create_file
 *****************************************************************************/
/**
 * Create a file and return it's inode ptr.
 *
 * @param[in] path   The full path of the new file
 * @param[in] flags  Attribiutes of the new file
 *
 * @return           Ptr to i-node of the new file if successful, otherwise 0.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
PRIVATE struct inode *create_file(char *path, int flags)
{
	disp_str("path: ");
	disp_str(path);
	disp_str("  ");

	char filename[MAX_PATH];
	struct inode *dir_inode;
	if (strip_path(filename, path, &dir_inode) != 0) //执行后，dir_inode已是目录文件的inode，同时filename是纯文件名
		return 0;

	int inode_nr = alloc_imap_bit(dir_inode->i_dev); //从inode-map中申请1位
	disp_str("inode_nr: ");
	disp_int(inode_nr);
	disp_str("    ");

	int free_sect_nr = alloc_smap_bit(dir_inode->i_dev, NR_DEFAULT_FILE_SECTS); //从sector-map中申请NR_DEFAULT_FILE_SECTS(2048)个bit位

	struct inode *newino = new_inode(dir_inode->i_dev, inode_nr, free_sect_nr); //从inode_array中申请一个新的inode

	new_dir_entry(dir_inode, newino->i_num, filename); //从目录文件中新申请一个dir_ent

	return newino;
}

/// zcr copied from ch9/e/fs/misc.c

/*****************************************************************************
 *                                memcmp
 *****************************************************************************/
/**
 * Compare memory areas.
 * 
 * @param s1  The 1st area.
 * @param s2  The 2nd area.
 * @param n   The first n bytes will be compared.
 * 
 * @return  an integer less than, equal to, or greater than zero if the first
 *          n bytes of s1 is found, respectively, to be less than, to match,
 *          or  be greater than the first n bytes of s2.
 *****************************************************************************/
PRIVATE int memcmp(const void * s1, const void *s2, int n)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = (const char *)s1;
	const char * p2 = (const char *)s2;
	int i;
	for (i = 0; i < n; i++,p1++,p2++) {
		if (*p1 != *p2) {
			return (*p1 - *p2);
		}
	}
	return 0;
}

/*****************************************************************************
 *                                strcmp
 *****************************************************************************/
/**
 * Compare two strings.
 * 
 * @param s1  The 1st string.
 * @param s2  The 2nd string.
 * 
 * @return  an integer less than, equal to, or greater than zero if s1 (or the
 *          first n bytes thereof) is  found,  respectively,  to  be less than,
 *          to match, or be greater than s2.
 *****************************************************************************/
PRIVATE int strcmp(const char * s1, const char *s2)
{
	if ((s1 == 0) || (s2 == 0)) { /* for robustness */
		return (s1 - s2);
	}

	const char * p1 = s1;
	const char * p2 = s2;

	for (; *p1 && *p2; p1++,p2++) {
		if (*p1 != *p2) {
			break;
		}
	}

	return (*p1 - *p2);
}


/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * Search the file and return the inode_nr.
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
PRIVATE int search_file(char * path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode *dir_inode;
	if (strip_path(filename, path, &dir_inode) != 0) //执行后，dir_inode已是目录文件的inode，同时filename是纯文件名
		return 0;

	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;
	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;							   //目录文件的起始扇区
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE; //目录文件占据的扇区总数
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE; 			   //目录文件中dir_ent的个数
	
	int m = 0;
	struct dir_entry *pde;   //目录项指针 //pde的意思是p_dir_ent
	char fsbuf[SECTOR_SIZE]; //local array, to substitute global fsbuf. added by xw, 18/12/27 //内核缓冲区

	//遍历目录文件占据的每一个扇区，读入内存缓冲区fsbuf
	for (i = 0; i < nr_dir_blks; i++) 
	{
		//读扇区到内核缓冲区，3个参数分别为：设备号，扇区号，内存缓冲区
		RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27

		pde = (struct dir_entry *)fsbuf;
		//遍历目录文件中的每一个目录项dir_entry，取出其中的文件名来做匹配
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) 
		{
			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inode_nr; //返回该文件的inode编号
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}

	/* file not found */
	return 0;
}

/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS v1.0, all files are stored in the root directory.
 * There is no sub-folder thing.
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname should begin with at most one `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname.
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 * 
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
PRIVATE int strip_path(char * filename, const char * pathname, struct inode** ppinode)
{
	const char * s = pathname;
	char * t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {		/* check each character */
		if (*s == '/')
			return -1;
		*t++ = *s++;
		/* if filename is too long, just truncate it */
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = root_inode;

	return 0;
}

/*****************************************************************************
 *                                read_super_block
 *****************************************************************************/
/**
 * <Ring 1> Read super block from the given device then write it into a free
 *          super_block[] slot.
 * 
 * @param dev  From which device the super block comes.
 *****************************************************************************/
//将super_block的信息从硬盘读到内存中的super_block[]缓冲区
PRIVATE void read_super_block(int dev)
{
	int i;
	MESSAGE driver_msg;
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27

	driver_msg.type		= DEV_READ;
	driver_msg.DEVICE	= MINOR(dev);
	driver_msg.POSITION	= SECTOR_SIZE * 1;
	driver_msg.BUF		= fsbuf;
	driver_msg.CNT		= SECTOR_SIZE;
	//driver_msg.PROC_NR= proc2pid(p_proc_current);///TASK_A	//deleted by mingxuan 2019-1-16
	driver_msg.PROC_NR	= proc2pid(proc);///TASK_A	//modified by mingxuan 2019-1-16

	hd_rdwt(&driver_msg);

	acquire(&super_block_lock);	//added by mingxuan 2019-3-21

	/* find a free slot in super_block[] */
	//super_block[]是内存缓冲区,类似于inode_table
	for (i = 0; i < NR_SUPER_BLOCK; i++)
		if (super_block[i].sb_dev == NO_DEV)
			break;
	if (i == NR_SUPER_BLOCK)
		disp_str("Panic: super_block slots used up");	/// zcr modified.

	// assert(i == 0); /* currently we use only the 1st slot */

	struct super_block *psb = (struct super_block*)fsbuf;

	super_block[i] = *psb;
	super_block[i].sb_dev = dev;

	release(&super_block_lock); //added by mingxuan 2019-3-21

}


/*****************************************************************************
 *                                get_super_block
 *****************************************************************************/
/**
 * <Ring 1> Get the super block from super_block[].
 * 
 * @param dev Device nr.
 * 
 * @return Super block ptr.
 *****************************************************************************/
// PUBLIC struct super_block * get_super_block(int dev)
// {
// 	struct super_block * sb = super_block;
// 	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
// 		if (sb->sb_dev == dev)
// 			return sb;

// 	disp_str("Panic: super block of devie ");
// 	disp_int(dev);
// 	disp_str(" not found.\n");

// 	return 0;
// }

/// zcr(using hu's method.)
PRIVATE struct super_block *get_super_block(int dev)
{
	struct super_block *sb = super_block;

	acquire(&super_block_lock);	//added by mingxuan 2019-3-21

	for (; sb < &super_block[NR_SUPER_BLOCK]; sb++)
	{
		if (sb->sb_dev == dev)
		{
			release(&super_block_lock); //added by mingxuan 2019-3-21
			return sb;
		}
	}

	release(&super_block_lock); //added by mingxuan 2019-3-21

	return 0;
}



/*****************************************************************************
 *                                get_inode
 *****************************************************************************/
/**
 * <Ring 1> Get the inode ptr of given inode nr. A cache -- inode_table[] -- is
 * maintained to make things faster. If the inode requested is already there,
 * just return it. Otherwise the inode will be read from the disk.
 * 
 * @param dev Device nr.
 * @param num I-node nr.
 * 
 * @return The inode ptr requested.
 *****************************************************************************/
//功能：如果一个inode已经被读入inode_table，那么下一次再需要用该inode时
//就不需要再进行一次磁盘I/O，直接从inode_table中拿出来用即可
PRIVATE struct inode * get_inode(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode *p;
	struct inode *q = 0;

	acquire(&inode_table_lock);	//added by mingxuan 2019-3-20

	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) //查找内存中的inode_table
	{
		if (p->i_cnt) 
		{//该inode已经被读入inode_table，不需要再进行一次磁盘I/O，直接从inode_table中拿出来用即可
			if ((p->i_dev == dev) && (p->i_num == num)) 
			{
				p->i_cnt++; /* this is the inode we want */ //一旦一个inode读入，i_cnt就自加

				//release(&inode_table_lock); //added by mingxuan 2019-3-20

				return p;   //表明我们需要的inode已经在inode_table缓冲区中了
			}
		}
		else //i_cnt为0，p还没有被进程打开(p是一个没有用过的inode)，表明inode_table中的此项可以分配给新读入的inode
		{//该inode还没有被读入inode_table，接下来先进行磁盘I/O，方便下一次再需要访问该inode时直接从inode_table中拿出来用即可
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		disp_str("Panic: the inode table is full");

	//以下3个赋值都修改了inode_table
	q->i_dev = dev;
	q->i_num = num;	//inode的编号
	q->i_cnt = 1;	//用作该inode下一次进入inode_table循环中做判断

	struct super_block *sb = get_super_block(dev); //得到给定设备的超级块指针
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) / (SECTOR_SIZE / INODE_SIZE)); //该inode在inode_array中的扇区号
	char fsbuf[SECTOR_SIZE];	 //local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT(dev, blk_nr, fsbuf); //added by xw, 18/12/27 //将硬盘上该inode所在的整个扇区读到内存中

	struct inode * pinode = (struct inode*)((u8*)fsbuf + ((num - 1 ) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE); //从内存中整个扇区只获取到该inode的信息
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;

	release(&inode_table_lock); //added by mingxuan 2019-3-20

	return q; //返回指向inode_table中对应表项的指针
}

//added by xw, 18/8/27
//功能：如果一个inode已经被读入inode_table，那么下一次再需要用该inode时
//就不需要再进行一次磁盘I/O，直接从inode_table中拿出来用即可
PRIVATE struct inode *get_inode_sched(int dev, int num)
{
	if (num == 0)
		return 0;

	struct inode *p;
	struct inode *q = 0;

	acquire(&inode_table_lock);	//added by mingxuan 2019-3-20 //deleted by mingxuan 2019-3-29

	for (p = &inode_table[0]; p < &inode_table[NR_INODE]; p++) //查找内存中的inode_table
	{
		if (p->i_cnt) /* not a free slot */
		{//该inode已经被读入inode_table，不需要再进行一次磁盘I/O，直接从inode_table中拿出来用即可
			if ((p->i_dev == dev) && (p->i_num == num)) 
			{
				p->i_cnt++; /* this is the inode we want */ //一旦一个inode读入，i_cnt就自加 

				release(&inode_table_lock); //added by mingxuan 2019-3-20 //deleted by mingxuan 2019-3-29

				return p;  //表明我们需要的inode已经在inode_table缓冲区中了
			}
		}
		else /* a free slot */
		{//i_cnt为0，p还没有被进程打开(p是一个没有用过的inode)，表明inode_table中的此项可以分配给新读入的inode
		 //该inode还没有被读入inode_table，接下来先进行磁盘I/O，方便下一次再需要访问该inode时直接从inode_table中拿出来用即可
			if (!q) /* q hasn't been assigned yet */
				q = p; /* q <- the 1st free slot */
		}
	}

	if (!q)
		disp_str("Panic: the inode table is full");

	//以下3个赋值都修改了inode_table
	q->i_dev = dev;
	q->i_num = num;	//inode的编号
	q->i_cnt = 1;	//用作该inode下一次进入inode_table循环中做判断

	struct super_block *sb = get_super_block(dev); //得到给定设备的超级块指针
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((num - 1) / (SECTOR_SIZE / INODE_SIZE)); //该inode在inode_array中的扇区号
	char fsbuf[SECTOR_SIZE];	       //local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT_SCHED(dev, blk_nr, fsbuf); //added by xw, 18/12/27 //将硬盘上该inode所在的整个扇区读到内存中

	struct inode *pinode = (struct inode*)((u8*)fsbuf + ((num - 1 ) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE); //从内存中整个扇区只获取到该inode的信息
	//以下4个赋值同时也修改了inode_table
	q->i_mode = pinode->i_mode;
	q->i_size = pinode->i_size;
	q->i_start_sect = pinode->i_start_sect;
	q->i_nr_sects = pinode->i_nr_sects;

	release(&inode_table_lock); //added by mingxuan 2019-3-20 //deleted by mingxuan 2019-3-29

	return q; //返回指向inode_table中对应表项的指针
}

/*****************************************************************************
 *                                put_inode
 *****************************************************************************/
/**
 * Decrease the reference nr of a slot in inode_table[]. When the nr reaches
 * zero, it means the inode is not used any more and can be overwritten by
 * a new inode.
 * 
 * @param pinode I-node ptr.
 *****************************************************************************/
PRIVATE void put_inode(struct inode *pinode)
{
	// assert(pinode->i_cnt > 0);
	pinode->i_cnt--;
}

/*****************************************************************************
 *                                sync_inode
 *****************************************************************************/
/**
 * <Ring 1> Write the inode back to the disk. Commonly invoked as soon as the
 *          inode is changed.
 * 
 * @param p I-node ptr.
 *****************************************************************************/
PRIVATE void sync_inode(struct inode *p)
{
	struct inode *pinode;
	struct super_block *sb = get_super_block(p->i_dev);
	int blk_nr = 1 + 1 + sb->nr_imap_sects + sb->nr_smap_sects + ((p->i_num - 1) / (SECTOR_SIZE / INODE_SIZE));
	char fsbuf[SECTOR_SIZE];				//local array, to substitute global fsbuf. added by xw, 18/12/27

	acquire(&sync_inode_lock);	//added by mingxuan 2019-3-25

	RD_SECT_SCHED(p->i_dev, blk_nr, fsbuf);	//modified by xw, 18/12/27

	pinode = (struct inode*)((u8*)fsbuf + (((p->i_num - 1) % (SECTOR_SIZE / INODE_SIZE)) * INODE_SIZE));
	pinode->i_mode = p->i_mode;
	pinode->i_size = p->i_size;
	pinode->i_start_sect = p->i_start_sect;
	pinode->i_nr_sects = p->i_nr_sects;
	WR_SECT_SCHED(p->i_dev, blk_nr, fsbuf);	//modified by xw, 18/12/27

	release(&sync_inode_lock);	//deleted by mingxuan 2019-3-25
}

/// added by zcr (from ch9/e/fs/open.c)
/*****************************************************************************
 *                                new_inode
 *****************************************************************************/
/**
 * Generate a new i-node and write it to disk.
 * 
 * @param dev  Home device of the i-node.
 * @param inode_nr  I-node nr.
 * @param start_sect  Start sector of the file pointed by the new i-node.
 * 
 * @return  Ptr of the new i-node.
 *****************************************************************************/
PRIVATE struct inode *new_inode(int dev, int inode_nr, int start_sect)
{
	struct inode *new_inode = get_inode_sched(dev, inode_nr); //modified by xw, 18/8/28 //在内存缓冲区inode_table中查找inode

	new_inode->i_mode = I_REGULAR;
	new_inode->i_size = 0;
	new_inode->i_start_sect = start_sect;
	new_inode->i_nr_sects = NR_DEFAULT_FILE_SECTS;

	new_inode->i_dev = dev;
	new_inode->i_cnt = 1;
	new_inode->i_num = inode_nr;

	/* write to the inode array */
	sync_inode(new_inode);	//一旦内存中inode_table中的值发生改变，就立即写入磁盘 //inode_table是存放内存中所有inode的内存缓冲区

	return new_inode;
}

/*****************************************************************************
 *                                new_dir_entry
 *****************************************************************************/
/**
 * Write a new entry into the directory.
 * 
 * @param dir_inode  I-node of the directory.
 * @param inode_nr   I-node nr of the new file.
 * @param filename   Filename of the new file.
 *****************************************************************************/
PRIVATE void new_dir_entry(struct inode *dir_inode,int inode_nr,char *filename)
{
	/* write the dir_entry */
	int dir_blk0_nr = dir_inode->i_start_sect;							//目录文件的起始扇区号
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;  //目录文件占据的扇区总数
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE; 			//目录文件中dir_ent的个数

	int m = 0;
	struct dir_entry *pde;	//目录项指针 //pde的意思是p_dir_ent
	struct dir_entry *new_de = 0;

	int i, j;
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27

	for (i = 0; i < nr_dir_blks; i++) 
	{//遍历目录文件占据的每一个扇区，读入内存缓冲区fsbuf
		RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27

		pde = (struct dir_entry *)fsbuf;
		//遍历目录文件中的每一个目录项dir_entry
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) 
		{
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == 0) 
			{ /* it's a free slot */
				new_de = pde;
				break;
			}
		}
		if (m > nr_dir_entries ||/* all entries have been iterated or */
		    new_de)              /* free slot is found */
			break;
	}
	if (!new_de) { /* reached the end of the dir */
		new_de = pde;
		dir_inode->i_size += DIR_ENTRY_SIZE;
	}

	//给新申请到的dir_ent赋值
	new_de->inode_nr = inode_nr;
	strcpy(new_de->name, filename);

	/* write dir block -- ROOT dir block */
	WR_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf); //modified by xw, 18/12/27 //write dir block

	/* update dir inode */
	sync_inode(dir_inode); //write dir_ent inode
}


/*****************************************************************************
 *                                alloc_imap_bit
 *****************************************************************************/
/**
 * Allocate a bit in inode-map.
 * 
 * @param dev  In which device the inode-map is located.
 * 
 * @return  I-node nr.
 *****************************************************************************/
PRIVATE int alloc_imap_bit(int dev)
{
	int inode_nr = 0;
	int i, j, k;

	int imap_blk0_nr = 1 + 1; /* 1 boot sector & 1 super block */
	char fsbuf[SECTOR_SIZE];  //local array, to substitute global fsbuf. added by xw, 18/12/27

	struct super_block *sb = get_super_block(dev);

	//acquire(&alloc_imap_bit_lock);	//added by mingxuan 2019-3-20
	down(&inode_table_sem);				//added by mingxuan 2019-4-1
	
	for (i = 0; i < sb->nr_imap_sects; i++) 
	{
		// RD_SECT(dev, imap_blk0_nr + i);				/// zcr: place the result in fsbuf?
		RD_SECT_SCHED(dev, imap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27

		for (j = 0; j < SECTOR_SIZE; j++) 
		{
			/* skip `11111111' bytes */
			//if (fsbuf[j] == 0xFF)
			if (fsbuf[j] == '\xFF')		//modified by xw, 18/12/28
				continue;

			/* skip `1' bits */
			for (k = 0; ((fsbuf[j] >> k) & 1) != 0; k++) {}

			/* i: sector index; j: byte index; k: bit index */
			inode_nr = (i * SECTOR_SIZE + j) * 8 + k;
			fsbuf[j] |= (1 << k);

			/* write the bit to imap */
			WR_SECT_SCHED(dev, imap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
			break;
		}

		//release(&alloc_imap_bit_lock);	//added by mingxuan 2019-3-20
		up(&inode_table_sem);				//added by mingxuan 2019-4-1

		return inode_nr;
	}

	/* no free bit in imap */
	disp_str("Panic: inode-map is probably full.\n");

	//release(&alloc_imap_bit_lock);	//added by mingxuan 2019-3-20
	up(&inode_table_sem);				//added by mingxuan 2019-4-1

	return 0;
}

/*****************************************************************************
 *                                alloc_smap_bit
 *****************************************************************************/
/**
 * Allocate a bit in sector-map.
 * 
 * @param dev  In which device the sector-map is located.
 * @param nr_sects_to_alloc  How many sectors are allocated.
 * 
 * @return  The 1st sector nr allocated.
 *****************************************************************************/
PRIVATE int alloc_smap_bit(int dev, int nr_sects_to_alloc)
{
	/* int nr_sects_to_alloc = NR_DEFAULT_FILE_SECTS; */

	int i; /* sector index */
	int j; /* byte index */
	int k; /* bit index */

	int free_sect_nr = 0;
	char fsbuf[SECTOR_SIZE]; //一个扇区大小(512Byte)的缓冲区 //added by xw, 18/12/27

	//acquire(&alloc_smap_bit_lock);	//added by mingxuan 2019-3-20

	struct super_block *sb = get_super_block(dev);
	int smap_blk0_nr = 1 + 1 + sb->nr_imap_sects; //1+1+存储inode-map的扇区个数 

	acquire(&alloc_smap_bit_lock);	//added by mingxuan 2019-3-22

	for (i = 0; i < sb->nr_smap_sects; i++) 
	{ /* smap_blk0_nr + i : current sect nr. */
		//每次从磁盘上读入1个smap扇区,1个smap扇区只够存储2个文件
		RD_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf); //modified by xw, 18/12/27

		/* byte offset in current sect */
		for (j = 0; j < SECTOR_SIZE && nr_sects_to_alloc > 0; j++) 
		{//对该扇区上的每一个Byte作分析
			k = 0;
			if (!free_sect_nr) 
			{
				/* loop until a free bit is found */
				//if (fsbuf[j] == 0xFF) continue;
				if (fsbuf[j] == '\xFF') continue; //modified by xw, 18/12/28 //表示该Byte已经被占用
				for (; ((fsbuf[j] >> k) & 1) != 0; k++) {} //检测该Byte中的每一bit是否都是0 //该循环是为了得到k值
				free_sect_nr = (i * SECTOR_SIZE + j) * 8 + k - 1 + sb->n_1st_sect;
			}

			for (; k < 8; k++) //默认每个文件占据2048个扇区，也就是要占据smap的2048个bit位，也就是0.5个smap扇区
			{ /* repeat till enough bits are set */
				fsbuf[j] |= (1 << k);
				if (--nr_sects_to_alloc == 0)
					break;
			}
		}

		//将smap信息写回磁盘
		if (free_sect_nr) /* free bit found, write the bits to smap */
			WR_SECT_SCHED(dev, smap_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27

		if (nr_sects_to_alloc == 0)
			break;
	}

	release(&alloc_smap_bit_lock);	//added by mingxuan 2019-3-20

	return free_sect_nr;
}

/// zcr added and modified from ch9/e/fs.open.c and close.c
/*****************************************************************************
 *                                close
 *****************************************************************************/
/**
 * Close a file descriptor.
 * 
 * @param fd  File descriptor.
 * 
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
//close is a syscall interface now. added by xw, 18/6/18
// PUBLIC int close(int fd)
PRIVATE int real_close(int fd)
{
	return do_close(fd);	// terrible(always returns 0)
}

/*****************************************************************************
 *                                do_close
 *****************************************************************************/
/**
 * Handle the message CLOSE.
 * 
 * @return Zero if success.
 *****************************************************************************/
PRIVATE int do_close(int fd)
{
	PROCESS *p = proc; //用于gdb调试时显示proc的值 //added by mingxuan 2019-3-18

	//put_inode(p_proc_current->task.filp[fd]->fd_inode);	//deleted by mingxuan 2019-1-16
	put_inode(proc->task.filp[fd]->fd_inode);	//modified by mingxuan 2019-1-16

	//p_proc_current->task.filp[fd]->fd_inode = 0;	//deleted by mingxuan 2019-1-16
	proc->task.filp[fd]->fd_inode = 0;
	//p_proc_current->task.filp[fd] = 0;	//deleted by mingxuan 2019-1-16
	proc->task.filp[fd] = 0;

	return 0;
}

/// zcr copied from ch9/f/fs/read_write.c and modified it.

/*****************************************************************************
 *                                read
 *****************************************************************************/
/**
 * Read from a file descriptor.
 * 
 * @param fd     File descriptor.
 * @param buf    Buffer to accept the bytes read.
 * @param count  How many bytes to read.
 * 
 * @return  On success, the number of bytes read are returned.
 *          On error, -1 is returned.
 *****************************************************************************/
//read is a syscall interface now. added by xw, 18/6/18
// PUBLIC int read(int fd, void *buf, int count)
PRIVATE int real_read(int fd, void *buf, int count)
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.type = READ;
	fs_msg.FD   = fd;
	fs_msg.BUF  = buf;
	fs_msg.CNT  = count;
	//fs_msg.source = proc2pid(p_proc_current); //deleted by mingxuan 2019-1-16
	fs_msg.source = proc2pid(proc); //modified by mingxuan 2019-1-16

	// send_recv(BOTH, TASK_FS, &msg);
	do_rdwt(&fs_msg);

	return fs_msg.CNT;
}

/*****************************************************************************
 *                                write
 *****************************************************************************/
/**
 * Write to a file descriptor.
 * 
 * @param fd     File descriptor.
 * @param buf    Buffer including the bytes to write.
 * @param count  How many bytes to write.
 * 
 * @return  On success, the number of bytes written are returned.
 *          On error, -1 is returned.
 *****************************************************************************/
//write is a syscall interface now. added by xw, 18/6/18
// PUBLIC int write(int fd, const void *buf, int count)
PRIVATE int real_write(int fd, const void *buf, int count)
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.type = WRITE;
	fs_msg.FD   = fd;
	fs_msg.BUF  = (void*)buf;
	fs_msg.CNT  = count;
	//fs_msg.source = proc2pid(p_proc_current);	//deleted by mingxuan 2019-1-16
	fs_msg.source = proc2pid(proc); //modified by mingxuan 2019-1-16

	// send_recv(BOTH, TASK_FS, &msg);
	/// zcr added
	do_rdwt(&fs_msg);
	
	return fs_msg.CNT;
}


/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 *
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 * 
 * @return How many bytes have been read/written.
 *****************************************************************************/
PRIVATE int do_rdwt(MESSAGE *fs_msg)
{
	//系统调用read的原型：n = read(fd, bufr, rd_bytes);
	//系统调用write的原型：n = write(fd, bufw, strlen(bufw));
	int fd = fs_msg->FD;	  /**< file descriptor. */
	void *buf = fs_msg->BUF;  //buf是在用户进程中定义的
	int len = fs_msg->CNT;	  /**< r/w bytes */ //写时就是strlen(buf)
	int src = fs_msg->source; /* caller proc nr. */

	//if (!(p_proc_current->task.filp[fd]->fd_mode & O_RDWR)) //deleted by mingxuan 2019-1-16
	if (!(proc->task.filp[fd]->fd_mode & O_RDWR)) //modified by mingxuan 2019-1-16
		return -1;

	//int pos = p_proc_current->task.filp[fd]->fd_pos; //deleted by mingxuan 2019-1-16
	int pos = proc->task.filp[fd]->fd_pos; //开始读写的位置，记录读写到文件的哪个位置 //modified by mingxuan 2019-1-16

	//struct inode * pin = p_proc_current->task.filp[fd]->fd_inode; //deleted by mingxuan 2019-1-16
	struct inode *pin = proc->task.filp[fd]->fd_inode; //modified by mingxuan 2019-1-16

	int imode = pin->i_mode & I_TYPE_MASK;
	if (imode == I_CHAR_SPECIAL) //读写字符特殊设备文件
	{
		int t = fs_msg->type == READ ? DEV_READ : DEV_WRITE;
		fs_msg->type = t;

		int dev = pin->i_start_sect;
		// assert(MAJOR(dev) == 4);
		/// zcr added
		if(MAJOR(dev) != 4) {
			disp_str("Error: MAJOR(dev) == 4\n");
		}

		fs_msg->DEVICE	= MINOR(dev);
		fs_msg->BUF	= buf;
		fs_msg->CNT	= len;
		fs_msg->PROC_NR	= src;

		hd_rdwt_sched(&fs_msg);		//modified by xw, 18/8/27

		return fs_msg->CNT;
	}
	else //读写普通文件
	{
		int pos_end; //结束读写的位置
		if (fs_msg->type == READ)
			pos_end = min(pos + len, pin->i_size); //读操作时pos_end不能超过文件已有的大小
		else /* WRITE */
			pos_end = min(pos + len, pin->i_nr_sects * SECTOR_SIZE); //写操作时pos_end不能超过为文件分配的最大空间

		//通过pos和pos_end计算出读/写操作所涉及的扇区边界
		int off = pos % SECTOR_SIZE;
		int rw_sect_min = pin->i_start_sect + (pos>>SECTOR_SIZE_SHIFT);
		int rw_sect_max = pin->i_start_sect + (pos_end>>SECTOR_SIZE_SHIFT);

		//modified by xw, 18/12/27
		//int chunk = min(rw_sect_max - rw_sect_min + 1,
		//		FSBUF_SIZE >> SECTOR_SIZE_SHIFT);
		int chunk = min(rw_sect_max - rw_sect_min + 1, //chunk最大不能超过为fsbuf分配的空间(rw_sect_max - rw_sect_min + 1)
				SECTOR_SIZE >> SECTOR_SIZE_SHIFT); //chunk最小为一个扇区

		int bytes_rw = 0;		 //已经读写完成的Byte	
		int bytes_left = len;	 //需要被读写的剩余Byte
		char fsbuf[SECTOR_SIZE]; //local array, to substitute global fsbuf. added by xw, 18/12/27

		int i;
		for (i = rw_sect_min; i <= rw_sect_max; i += chunk) 
		{
			/* read/write this amount of bytes every time */
			int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);

			//注意：读写操作都要先将目标扇区读出
			//因为写操作可以在文件的任意位置进行，所以以文件为单位的上下文需要先行读出
			//rw_sector_sched(int io_type, int dev, int pos, int bytes, int proc_nr, void* buf)
			rw_sector_sched(DEV_READ,			//modified by xw, 18/8/27
				  pin->i_dev,					//设备号
				  i * SECTOR_SIZE,				//读写指针的位置
				  chunk * SECTOR_SIZE,			//每次读写的数据量为多少Byte
				  //proc2pid(p_proc_current),	//当前进程的pid	//deleted by mingxuan 2019-1-16
				  proc2pid(proc),				//当前进程的pid	//modified by mingxuan 2019-1-16
				  fsbuf);						//将扇区中的数据读到fsbuf

			if (fs_msg->type == READ) 
			{
				phys_copy((void*)va2la(src, buf + bytes_rw),	 //目的
					  //(void*)va2la(proc2pid(p_proc_current), fsbuf + off), 				  //deleted by mingxuan 2019-1-16
					  (void*)va2la(proc2pid(proc), fsbuf + off), //源  //buf是在用户进程中定义的  //modified by mingxuan 2019-1-16
					  bytes); //每次读写的数据量为多少Byte
			}
			else /* WRITE */
			{
				//phys_copy((void*)va2la(proc2pid(p_proc_current), fsbuf + off),       //deleted by mingxuan 2019-1-16
				phys_copy((void*)va2la(proc2pid(proc), fsbuf + off),//目的 			  	//modified by mingxuan 2019-1-16
					  (void*)va2la(src, buf + bytes_rw),			//源 //buf是在用户进程中定义的 
					  bytes); //每次读写的数据量为多少Byte

				rw_sector_sched(DEV_WRITE,		 //modified by xw, 18/8/27
					  pin->i_dev,				 //设备号
					  i * SECTOR_SIZE,			 //读写指针的位置
					  chunk * SECTOR_SIZE,		 //每次读写的数据量为多少Byte
					  //proc2pid(p_proc_current),//当前进程的pid  //deleted by mingxuan 2019-1-16
					  proc2pid(proc), 			 //当前进程的pid  //modified by mingxuan 2019-1-16
					  fsbuf);					 //将fsbuf中的数据写到扇区中 
			}

			off = 0;
			bytes_rw += bytes;  //已经读写完成的Byte
			//p_proc_current->task.filp[fd]->fd_pos += bytes; //deleted by mingxuan 2019-1-16
			proc->task.filp[fd]->fd_pos += bytes; 			  //modified by mingxuan 2019-1-16
			bytes_left -= bytes;//需要被读写的剩余Byte
		}

		//if (p_proc_current->task.filp[fd]->fd_pos > pin->i_size) { //deleted by mingxuan 2019-1-16
		if (proc->task.filp[fd]->fd_pos > pin->i_size)				 //modified by mingxuan 2019-1-16 
		{	
			/* update inode::size */
			//pin->i_size = p_proc_current->task.filp[fd]->fd_pos; //deleted by mingxuan 2019-1-16
			pin->i_size = proc->task.filp[fd]->fd_pos; 			   //modified by mingxuan 2019-1-16

			/* write the updated i-node back to disk */
			sync_inode(pin); //一旦内存中inode的值发生了变化，就立即写入硬盘
		}

		return bytes_rw;
	}
}

/// zcr copied from ch9/h/lib/unlink.c and modified it

/*****************************************************************************
 *                                unlink
 *****************************************************************************/
/**
 * Delete a file.
 * 
 * @param pathname  The full path of the file to delete.
 * 
 * @return Zero if successful, otherwise -1.
 *****************************************************************************/
//unlink is a syscall interface now. added by xw, 18/6/19
// PUBLIC int unlink(const char * pathname)
PRIVATE int real_unlink(const char * pathname)
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.type   = UNLINK;
	fs_msg.PATHNAME	= (void*)pathname;
	fs_msg.NAME_LEN	= strlen(pathname);
	//fs_msg.source = proc2pid(p_proc_current);	//deleted by mingxuan 2019-1-16
	fs_msg.source = proc2pid(proc);	//modified by mingxuan 2019-1-16

	// send_recv(BOTH, TASK_FS, &msg);

	// return fs_msg.RETVAL;
	/// zcr added
	return do_unlink(&fs_msg);
}

/// zcr copied from the ch9/h/fs/link.c and modified it
/*****************************************************************************
 *                                do_unlink
 *****************************************************************************/
/**
 * Remove a file.
 *
 * @note We clear the i-node in inode_array[] although it is not really needed.
 *       We don't clear the data bytes so the file is recoverable.
 * 
 * @return On success, zero is returned.  On error, -1 is returned.
 *****************************************************************************/
PRIVATE int do_unlink(MESSAGE *fs_msg)
{
	char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len = fs_msg->NAME_LEN;	/* length of filename */
	int src = fs_msg->source;	/* caller proc nr. */
	// assert(name_len < MAX_PATH);
	//phys_copy((void*)va2la(proc2pid(p_proc_current), pathname), //deleted by mingxuan 2019-1-16
	phys_copy((void*)va2la(proc2pid(proc), pathname), //modified by mingxuan 2019-1-16
		  (void*)va2la(src, fs_msg->PATHNAME),
		  name_len);
	pathname[name_len] = 0;

	if (strcmp(pathname , "/") == 0) {
		/// zcr
		disp_str("FS:do_unlink():: cannot unlink the root\n");
		return -1;
	}

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		/// zcr
		disp_str("FS::do_unlink():: search_file() returns invalid inode: ");
		disp_str(pathname);
		disp_str("\n");
		return -1;
	}

	char filename[MAX_PATH];
	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0) //执行后，dir_inode已是目录文件的inode，同时filename是纯文件名
		return -1;

	struct inode *pin = get_inode_sched(dir_inode->i_dev, inode_nr);	//modified by xw, 18/8/28

	if (pin->i_mode != I_REGULAR) { /* can only remove regular files */
		// printl("cannot remove file %s, because it is not a regular file.\n", pathname);
		/// zcr
		disp_str("cannot remove file ");
		disp_str(pathname);
		disp_str(", because it is not a regular file.\n");
		return -1;
	}

	if (pin->i_cnt > 1) {	/* the file was opened */
		// printl("cannot remove file %s, because pin->i_cnt is %d.\n", pathname, pin->i_cnt);
		/// zcr
		disp_str("cannot remove file ");
		disp_str(pathname);
		disp_str(", because pin->i_cnt is ");
		disp_int(pin->i_cnt);
		disp_str("\n");
		return -1;
	}

	struct super_block *sb = get_super_block(pin->i_dev);

	/*************************/
	/* free the bit in i-map */
	/*************************/
	int byte_idx = inode_nr / 8;
	int bit_idx = inode_nr % 8;
	// assert(byte_idx < SECTOR_SIZE);	/* we have only one i-map sector */
	/* read sector 2 (skip bootsect and superblk): */
	char fsbuf[SECTOR_SIZE];	//local array, to substitute global fsbuf. added by xw, 18/12/27
	RD_SECT_SCHED(pin->i_dev, 2, fsbuf);		//modified by xw, 18/12/27
	// assert(fsbuf[byte_idx % SECTOR_SIZE] & (1 << bit_idx));
	fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << bit_idx);
	WR_SECT_SCHED(pin->i_dev, 2, fsbuf);	//modified by xw, 18/12/27

	/**************************/
	/* free the bits in s-map */
	/**************************/
	/*
	 *           bit_idx: bit idx in the entire i-map
	 *     ... ____|____
	 *                  \        .-- byte_cnt: how many bytes between
	 *                   \      |              the first and last byte
	 *        +-+-+-+-+-+-+-+-+ V +-+-+-+-+-+-+-+-+
	 *    ... | | | | | |*|*|*|...|*|*|*|*| | | | |
	 *        +-+-+-+-+-+-+-+-+   +-+-+-+-+-+-+-+-+
	 *         0 1 2 3 4 5 6 7     0 1 2 3 4 5 6 7
	 *  ...__/
	 *      byte_idx: byte idx in the entire i-map
	 */
	bit_idx  = pin->i_start_sect - sb->n_1st_sect + 1;
	byte_idx = bit_idx / 8;
	int bits_left = pin->i_nr_sects;
	int byte_cnt = (bits_left - (8 - (bit_idx % 8))) / 8;

	/* current sector nr. */
	int s = 2  /* 2: bootsect + superblk */
		+ sb->nr_imap_sects + byte_idx / SECTOR_SIZE;

	RD_SECT_SCHED(pin->i_dev, s, fsbuf);		//modified by xw, 18/12/27

	int i;
	/* clear the first byte */
	for (i = bit_idx % 8; (i < 8) && bits_left; i++,bits_left--) {
		// assert((fsbuf[byte_idx % SECTOR_SIZE] >> i & 1) == 1);
		fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << i);
	}

	/* clear bytes from the second byte to the second to last */
	int k;
	i = (byte_idx % SECTOR_SIZE) + 1;	/* the second byte */
	for (k = 0; k < byte_cnt; k++,i++,bits_left-=8) {
		if (i == SECTOR_SIZE) {
			i = 0;
			WR_SECT_SCHED(pin->i_dev, s, fsbuf);		//modified by xw, 18/12/27
			RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);		//modified by xw, 18/12/27
		}
		// assert(fsbuf[i] == 0xFF);
		fsbuf[i] = 0;
	}

	/* clear the last byte */
	if (i == SECTOR_SIZE) {
		i = 0;
		WR_SECT_SCHED(pin->i_dev, s, fsbuf);			//modified by xw, 18/12/27
		RD_SECT_SCHED(pin->i_dev, ++s, fsbuf);			//modified by xw, 18/12/27
	}
	unsigned char mask = ~((unsigned char)(~0) << bits_left);
	// assert((fsbuf[i] & mask) == mask);
	fsbuf[i] &= (~0) << bits_left;
	WR_SECT_SCHED(pin->i_dev, s, fsbuf);				//modified by xw, 18/12/27

	/***************************/
	/* clear the i-node itself */
	/***************************/
	pin->i_mode = 0;
	pin->i_size = 0;
	pin->i_start_sect = 0;
	pin->i_nr_sects = 0;
	sync_inode(pin);
	/* release slot in inode_table[] */
	put_inode(pin);

	/************************************************/
	/* set the inode-nr to 0 in the directory entry */
	/************************************************/
	int dir_blk0_nr = dir_inode->i_start_sect;							//目录文件的起始扇区号
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;  //目录文件占据的扇区总数
	int nr_dir_entries = dir_inode->i_size / DIR_ENTRY_SIZE;            //目录文件中dir_ent的个数

	int m = 0;
	struct dir_entry *pde = 0; //目录项指针 //pde的意思是p_dir_ent
	int flg = 0;
	int dir_size = 0;

	//遍历目录文件占据的每一个扇区，读入内存缓冲区fsbuf
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27

		pde = (struct dir_entry *)fsbuf;
		int j;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (++m > nr_dir_entries)
				break;

			if (pde->inode_nr == inode_nr) {
				/* pde->inode_nr = 0; */
				memset(pde, 0, DIR_ENTRY_SIZE);
				WR_SECT_SCHED(dir_inode->i_dev, dir_blk0_nr + i, fsbuf);	//modified by xw, 18/12/27
				flg = 1;
				break;
			}

			if (pde->inode_nr != INVALID_INODE)
				dir_size += DIR_ENTRY_SIZE;
		}

		if (m > nr_dir_entries || /* all entries have been iterated OR */
		    flg) /* file is found */
			break;
	}
	// assert(flg);
	if (m == nr_dir_entries) { /* the file is the last one in the dir */
		dir_inode->i_size = dir_size;
		sync_inode(dir_inode);
	}

	return 0;
}

/// zcr defined
//lseek is a syscall interface now. added by xw, 18/6/18
// PUBLIC int lseek(int fd, int offset, int whence)
PRIVATE int real_lseek(int fd, int offset, int whence)
{
	//added by xw, 18/8/27
	MESSAGE fs_msg;
	
	fs_msg.FD = fd;
	fs_msg.OFFSET = offset;
	fs_msg.WHENCE = whence;

	return do_lseek(&fs_msg);
}
/// zcr copied from ch9/j/fs/open.c
/*****************************************************************************
 *                                do_lseek
 *****************************************************************************/
/**
 * Handle the message LSEEK.
 * 
 * @return The new offset in bytes from the beginning of the file if successful,
 *         otherwise a negative number.
 *****************************************************************************/
PRIVATE int do_lseek(MESSAGE *fs_msg)
{
	int fd = fs_msg->FD;
	int off = fs_msg->OFFSET;
	int whence = fs_msg->WHENCE;

	//int pos = p_proc_current->task.filp[fd]->fd_pos;	//deleted by mingxuan 2019-1-16
	int pos = proc->task.filp[fd]->fd_pos;	//modified by mingxuan 2019-1-16
	//int f_size = p_proc_current->task.filp[fd]->fd_inode->i_size;	//deleted by mingxuan 2019-1-16
	int f_size = proc->task.filp[fd]->fd_inode->i_size;	//modified by mingxuan 2019-1-16

	switch (whence) {
	case SEEK_SET:
		pos = off;
		break;
	case SEEK_CUR:
		pos += off;
		break;
	case SEEK_END:
		pos = f_size + off;
		break;
	default:
		return -1;
		break;
	}
	if ((pos > f_size) || (pos < 0)) {
		return -1;
	}
	//p_proc_current->task.filp[fd]->fd_pos = pos;	//deleted by mingxuan 2019-1-16
	proc->task.filp[fd]->fd_pos = pos;	//modified by mingxuan 2019-1-16	
	return pos;
}

//added by xw, 18/6/18
PUBLIC int sys_open(void *uesp)
{
	return real_open(get_arg(uesp, 1),
					 get_arg(uesp, 2));
}

PUBLIC int sys_close(void *uesp)
{
	return real_close(get_arg(uesp, 1));
}

PUBLIC int sys_read(void *uesp)
{
	return real_read(get_arg(uesp, 1),
					 get_arg(uesp, 2),
					 get_arg(uesp, 3));
}

PUBLIC int sys_write(void *uesp)
{
	return real_write(get_arg(uesp, 1),
					  get_arg(uesp, 2),
					  get_arg(uesp, 3));
}

PUBLIC int sys_lseek(void *uesp)
{
	return real_lseek(get_arg(uesp, 1),
					  get_arg(uesp, 2),
					  get_arg(uesp, 3));
}
//~xw, 18/6/18

//added by xw, 18/6/19
PUBLIC int sys_unlink(void *uesp)
{
	return real_unlink(get_arg(uesp, 1));
}
