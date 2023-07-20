/**
 * fs_misc.h
 * This file contains miscellaneous defines and declarations associated with filesystem.
 * The code is added by zcr, and the file is added by xw. 18/6/17
 */

#ifndef	FS_MISC_H
#define	FS_MISC_H

/**
 * @struct dev_drv_map fs.h "include/sys/fs.h"
 * @brief  The Device_nr.\ - Driver_nr.\ MAP.
 */
struct dev_drv_map {
	int driver_nr; /**< The proc nr.\ of the device driver. */
};


#define	MAGIC_V1	0x111
/**
 * @struct super_block fs.h "include/fs.h"
 * @brief  The 2nd sector of the FS
 */
struct super_block {
	u32	magic;		  /**< Magic number */

	u32	nr_inodes;	      //文件系统中最多有多少个inode  //文件系统中最多允许有4096个inode，这样只需要1个扇区来做inode-map就足够了
	u32	nr_sects;	 	  //文件系统中最多有多少个扇区	  //来自于硬盘驱动程序

	u32	nr_imap_sects;	  //存储inode-map的扇区个数  		//只需要1个扇区来做inode-map就足够了
	u32	nr_smap_sects;	  //存储sector-map的扇区个数 

	u32	n_1st_sect;	 	  //第一个扇区的扇区号

	u32	nr_inode_sects;   //存储inode_array的扇区个数

	u32	root_inode;       //根目录区的inode编号 //根目录文件的inode编号为1

	/* 和inode相关的成员 */
	u32	inode_size;       //每个inode的大小为32Byte
	u32	inode_isize_off;  /**< Offset of `struct inode::i_size' */
	u32	inode_start_off;  /**< Offset of `struct inode::i_start_sect' */

	/* 和dir_ent相关的成员 */
	u32	dir_ent_size;     //每个目录项的大小为16Byte
	u32	dir_ent_inode_off;/**< Offset of `struct dir_entry::inode_nr' */
	u32	dir_ent_fname_off;/**< Offset of `struct dir_entry::name' */

	int	sb_dev; 	/**< the super block's home device */
};
#define	SUPER_BLOCK_SIZE	56	//单位为Byte，每个超级块的大小为32Byte

/**
 * @struct inode
 * @brief  i-node
 */
struct inode {
	u32	i_mode;			//文件类型
	u32	i_size;			//文件大小，单位是Byte
	u32	i_start_sect;	//该文件的起始扇区号
	u32	i_nr_sects;		//该文件占用的总的扇区数

	u8	_unused[16];	/**< Stuff for alignment */

	/* the following items are only present in memory */
	int	i_dev;
	int	i_cnt;	//此时有多少进程在共享该inode
	int	i_num;	//inode的编号
};
#define	INODE_SIZE	32	//单位为Byte，每个inode的大小为32Byte

/**
 * @struct dir_entry
 * @brief  Directory Entry
 */
struct dir_entry {
	int	inode_nr;				 //inode编号
	char name[MAX_FILENAME_LEN]; //文件名
};
#define	DIR_ENTRY_SIZE	sizeof(struct dir_entry) //每个dir_entry的大小为16Byte

/**
 * @struct file_desc
 * @brief  File Descriptor
 */
struct file_desc {
	int		fd_mode;	//记录这个fd的操作类型:读、写、既读又写
	int		fd_pos;		//记录读写到了文件的什么位置
	struct inode*	fd_inode;	//指向inode的指针
};

//以下都是读写扇区的宏
//modified by mingxuan 2019-1-16
#define RD_SECT(dev,sect_nr,fsbuf) rw_sector(DEV_READ, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE, \
				       proc2pid(proc),			\
				       fsbuf);

//modified by mingxuan 2019-1-16
#define WR_SECT(dev,sect_nr,fsbuf) rw_sector(DEV_WRITE, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(proc),				\
				       fsbuf);

//modified by mingxuan 2019-1-16
#define RD_SECT_SCHED(dev,sect_nr,fsbuf) rw_sector_sched(DEV_READ, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(proc),			\
				       fsbuf);

//modified by mingxuan 2019-1-16
#define WR_SECT_SCHED(dev,sect_nr,fsbuf) rw_sector_sched(DEV_WRITE, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(proc),				\
				       fsbuf);


/* deleted by mingxuan 2019-1-16
//#define RD_SECT(dev,sect_nr) rw_sector(DEV_READ,	//modified by xw, 18/12/27 
#define RD_SECT(dev,sect_nr,fsbuf) rw_sector(DEV_READ, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(p_proc_current),			\
				       fsbuf);
//#define WR_SECT(dev,sect_nr) rw_sector(DEV_WRITE, //modified by xw, 18/12/27
#define WR_SECT(dev,sect_nr,fsbuf) rw_sector(DEV_WRITE, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(p_proc_current),				\
				       fsbuf);

//added by xw, 18/8/27
//#define RD_SECT_SCHED(dev,sect_nr) rw_sector_sched(DEV_READ, //modified by xw, 18/12/27
#define RD_SECT_SCHED(dev,sect_nr,fsbuf) rw_sector_sched(DEV_READ, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(p_proc_current),			\
				       fsbuf);
//#define WR_SECT_SCHED(dev,sect_nr) rw_sector_sched(DEV_WRITE, //modified by xw, 18/12/27
#define WR_SECT_SCHED(dev,sect_nr,fsbuf) rw_sector_sched(DEV_WRITE, \
				       dev,				\
				       (sect_nr) * SECTOR_SIZE,		\
				       SECTOR_SIZE,  \
				       proc2pid(p_proc_current),				\
				       fsbuf);
//~xw
*/
#endif /* FS_MISC_H */
