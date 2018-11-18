/////////////////////////////////
//测试库调用总入口为static ssize_t ftxxxx_ftslibtest_sample(void),仅供参考
//测试库调用总入口为static ssize_t ftxxxx_ftslibtest_sample(void),仅供参考
//测试库调用总入口为static ssize_t ftxxxx_ftslibtest_sample(void),仅供参考
//测试库调用总入口为static ssize_t ftxxxx_ftslibtest_sample(void),仅供参考
//测试库调用总入口为static ssize_t ftxxxx_ftslibtest_sample(void),仅供参考
//测试库调用总入口为static ssize_t ftxxxx_ftslibtest_sample(void),仅供参考
///////////////////////////////////


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>

#include <linux/i2c.h>//iic
#include <linux/delay.h>//msleep
#include "test_lib.h"


//配置文件存放目录定义
#define FTXXXX_INI_FILEPATH "/mnt/sdcard/"  
//获取配置文件大小, 用于分配内存读取配置
static int ftxxxx_GetInISize(char *config_name)
{
	struct file *pfile = NULL;
	struct inode *inode = NULL;
	//unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));

	sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH, config_name);

	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);

	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	//magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);

	return fsize;
}
//读取配置到内存
static int ftxxxx_ReadInIData(char *config_name, char *config_buf)
{
	struct file *pfile = NULL;
	struct inode *inode = NULL;
	//unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	loff_t pos = 0;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH, config_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	//magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, config_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}
//保存测试数据到SD卡 etc.
static int ftxxxx_SaveTestData(char *file_name, char *data_buf, int iLen)
{
	struct file *pfile = NULL;
	
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FTXXXX_INI_FILEPATH, file_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_CREAT|O_RDWR, 0);
	if (IS_ERR(pfile)) {
		pr_err("error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(pfile, data_buf, iLen, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}

//读取,解析配置文件,初始化测试变量
static int ftxxxx_get_testparam_from_ini(char *config_name)
{
	char *filedata = NULL;

	int inisize = ftxxxx_GetInISize(config_name);

	pr_info("inisize = %d \n ", inisize);
	if (inisize <= 0) {
		pr_err("%s ERROR:Get firmware size failed\n", __func__);
		return -EIO;
	}

	filedata = kmalloc(inisize + 1, GFP_ATOMIC);
		
	if (ftxxxx_ReadInIData(config_name, filedata)) {
		pr_err("%s() - ERROR: request_firmware failed\n", __func__);
		kfree(filedata);
		return -EIO;
	} else {
		pr_info("ftxxxx_ReadInIData successful\n");
	}

	set_param_data(filedata);
	return 0;
}

//平台相关的I2C读函数,根据实际平台更改.
//平台相关的I2C读函数,根据实际平台更改.
int focal_i2c_Read(unsigned char *writebuf,
		    int writelen, unsigned char *readbuf, int readlen)
{
	int ret;

	if(NULL == g_i2c_adap)//全局IIC适配器指针,使用前必须初始化
	{
		printk("i2c_adapter = NULL in function:%s\n", __func__);
		return -1;
	}

	if (writelen > 0) {//write and read
		struct i2c_msg msgs[] = {
			{
			 .addr = usSlaveAddr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = usSlaveAddr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(g_i2c_adap, msgs, 2);
		if (ret < 0)
			printk("%s: i2c read error.\n", __func__);
	} else {//read only
		struct i2c_msg msgs[] = {
			{
			 .addr = usSlaveAddr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(g_i2c_adap, msgs, 1);
		if (ret < 0)
			printk("%s:i2c read error.\n", __func__);
	}
	return ret;
}
//平台相关的I2C写函数,根据实际平台更改.
//平台相关的I2C写函数,根据实际平台更改.
/*write data by i2c*/
int focal_i2c_Write(unsigned char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
		 .addr = usSlaveAddr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	if(NULL == g_i2c_adap) //全局IIC适配器指针,使用前必须初始化
	{
		printk("i2c_adapter = NULL in function:%s\n", __func__);
		return -1;
	}
	else if(NULL == writebuf)
	{
		printk("writebuf = NULL in function:%s\n", __func__);
		return -1;
	}
	else if(writelen<=0)
	{
		printk("writelen <= 0 in function:%s\n", __func__);
		return -1;
	}

	ret = i2c_transfer(g_i2c_adap, msg, 1);//write data only
	if (ret < 0)
		printk("%s i2c write error.\n", __func__);

	return ret;
}
/////////////////////////////////
//测试库调用总入口,仅供参考
//测试库调用总入口,仅供参考
//测试库调用总入口,仅供参考
//测试库调用总入口,仅供参考
//测试库调用总入口,仅供参考
//测试库调用总入口,仅供参考
///////////////////////////////////
static ssize_t ftxxxx_ftslibtest_sample(void)
{
	/* place holder for future use */
    	char cfgname[128];
	char *testdata = NULL;
	int iTestDataLen=0;//库中测试数据实际长度,用于保存到文件

	testdata = kmalloc(1024*80, GFP_ATOMIC);//用于获取存放在库中的测试数据,注意分配空间大小.
	if(NULL == testdata)
	{
		printk("kmalloc failed in function:%s\n", __func__);
		return -1;
	}

	memset(cfgname, 0, sizeof(cfgname));
	//test.ini 文件由量产测试软件配置生产(FT Multiple Test)
	sprintf(cfgname, "%s", "test.ini");//存放在SD卡目录中的测试配置文件名,可根据实际情况更改路径或名字.
	cfgname[8] = '\0';

	//mutex_lock(&g_device_mutex);
    ///********************************************************************************
	init_i2c_write_func(focal_i2c_Write);//初始化平台相关的I2C写函数(以上实现只是示例), 传递给测试库进行写操作
	init_i2c_read_func(focal_i2c_Read);//初始化平台相关的I2C读函数(以上实现只是示例), 传递给测试库进行读操作
	///********************************************************************************
	
	if(ftxxxx_get_testparam_from_ini(cfgname) <0)//第一步,读取解析配置文件.
		printk("get testparam from ini failure\n");
	else {		
		if(true == start_test_tp())//第二步,根据测试配置开始测试
			printk("tp test pass\n");
		else
			printk("tp test failure\n");
		
		iTestDataLen = get_test_data(testdata);//第三步,获取测试库中的测试数据
		printk("%s\n", testdata);
		ftxxxx_SaveTestData("testdata.csv", testdata, iTestDataLen);
		free_test_param_data();//第四步,释放内存等...
	}
		
	//mutex_unlock(&g_device_mutex);
	if(NULL != testdata) kfree(testdata);

	return 1;
}
