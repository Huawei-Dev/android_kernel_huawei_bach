#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/aio.h>
#include<linux/types.h>
#include<linux/mutex.h>
#include<linux/time.h>
#include<linux/store_exception.h>

static DEFINE_MUTEX(lock);

/**
 *  name: the name of this command
 *  msg: concrete command string to write to /dev/log/exception
 *  return: on success return the bytes writed successfully, on error return -1
 *
*/
int store_exception(char *path, char* name, char* msg, int len)
{
    mm_segment_t oldfs;
    struct file *file;
    struct iovec vec;
    int ret = 0;
    char log_path[255] = {0};

    if(NULL == path || NULL == name || NULL == msg)
    {
        pr_err("store_exception: the arguments invalidate\n");
        return -1;
    }

    snprintf(log_path, sizeof(log_path), "%s%s", path, name); 
    pr_info("store_exception: log_path:%s  msg:%s\n",log_path, msg);
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    file = filp_open(log_path, O_CREAT|O_RDWR, 0664);
    if(!file || IS_ERR(file))
    {
        pr_err("store_exception: Failed to access\n");
        set_fs(oldfs);
        return -1;
    }

    mutex_lock(&lock);
    vfs_truncate(&file->f_path,  0);
    vec.iov_base = msg;
    vec.iov_len = len;
    ret = vfs_writev(file, &vec, 1, &file->f_pos);
    mutex_unlock(&lock);

    if(ret < 0)
    {
        pr_err("store_exception: Failed to write to /dev/log/\n");
        filp_close(file, NULL);
        set_fs(oldfs);
        return -1;
    }

    pr_info("store_exception: success\n");
    filp_close(file, NULL);
    set_fs(oldfs);
    return ret;
}
#
