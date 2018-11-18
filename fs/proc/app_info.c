#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/export.h>
#include <misc/app_info.h>
#include <linux/slab.h>
#include <soc/qcom/smsm.h>
#include <linux/kthread.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
static struct kobject *set_appinfo_node_kobject = NULL;

extern int usb_update_thread(void *__unused);
static struct task_struct *update_task;
#define APP_NAME_VALUE_SPLIT_CHAR  "*"
#define SAMSUNG_ID    	1
#define ELPIDA_ID     		3
#define HYNIX_ID      		6
#define MICRON_ID		255

#define DDR_SIZE_ORDER_MAX 	35   //32Gbit , 4GB
#define DDR_SIZE_ORDER_MIN 	30   //1Gbit,125MB
#define DDR_SIZE_MAX 			(0x1<<(DDR_SIZE_ORDER_MAX - 30))
#define DDR_SIZE_MIN 			(0x1<<(DDR_SIZE_ORDER_MIN - 30))
#define SENSOR_MAX 				20

/** DDR types. */
typedef enum
{
  DDR_TYPE_LPDDR1,           	/**< Low power DDR1. */
  DDR_TYPE_LPDDR2 = 2,       	/**< Low power DDR2  set to 2 for compatibility*/
  DDR_TYPE_PCDDR2,           	/**< Personal computer DDR2. */
  DDR_TYPE_PCDDR3,           	/**< Personal computer DDR3. */

  DDR_TYPE_LPDDR3,           	/**< Low power DDR3. */

  DDR_TYPE_RESERVED,         	/**< Reserved for future use. */
  DDR_TYPE_UNUSED = 0x7FFFFFFF /**< For compatibility with deviceprogrammer(features not using DDR). */
} DDR_TYPE;

struct info_node
{
    char name[APP_INFO_NAME_LENTH];
    char value[APP_INFO_VALUE_LENTH];
    struct list_head entry;
};

struct sensor_node
{
    char name[APP_INFO_NAME_LENTH];
};

struct sensor_node sensor_type[SENSOR_MAX];

static LIST_HEAD(app_info_list);
static DEFINE_SPINLOCK(app_info_list_lock);

/* add app_info_node interface */
/* name and value use * to Separate */
/* buf like LP-Sensor*avago 9930,name will be LP-Sensor,value will be avago 9930 */
static ssize_t hw_set_app_info_node_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	char* AppStr = NULL;
	char AppName[APP_INFO_NAME_LENTH] = {'\0'};
	char AppValue[APP_INFO_VALUE_LENTH] = {'\0'};
	int name_lenth=0,value_lenth=0,buf_len=0,ret=-1;
	int i = 0;
	bool flag = false;
	char* strtok = NULL;
	buf_len = strlen(buf);
	AppStr = kmalloc(buf_len+5, GFP_KERNEL);
	if (!AppStr)
	{
		pr_err("%s:kmalloc fail!\n",__func__);
		return -1;
	}

	memset(AppStr, 0, buf_len+5);
	snprintf(AppStr, buf_len + 5 - 1, "%s", buf);
	strtok = strsep(&AppStr, APP_NAME_VALUE_SPLIT_CHAR);
	if (strtok != NULL)
	{
		name_lenth = strlen(strtok);
		memcpy(AppName,strtok,((name_lenth > (APP_INFO_NAME_LENTH-1))?(APP_INFO_NAME_LENTH-1):name_lenth));
	}
	else
	{
		pr_err("%s: buf name Invalid:%s", __func__,buf);
		goto split_fail_exit;
	}
	strtok = strsep(&AppStr, APP_NAME_VALUE_SPLIT_CHAR);
	if(strtok!=NULL)
	{
		value_lenth = strlen(strtok);
		memcpy(AppValue,strtok,((value_lenth > (APP_INFO_VALUE_LENTH-1))?(APP_INFO_VALUE_LENTH-1):value_lenth));
	}
	else
	{
		pr_err("%s: buf value Invalid:%s", __func__,buf);
		goto split_fail_exit;
	}
	for (i = 0; i < SENSOR_MAX; i++)
	{
		if (0 == strncmp(sensor_type[i].name, AppName,sizeof(AppName)))
		{
			flag = true;
			break;
		}
		else if (0 == strncmp(sensor_type[i].name, "",sizeof(AppName)))
		{
			strncpy(sensor_type[i].name, AppName, sizeof(AppName) - 1);
			break;
		}
	}

	if (!flag)
	{
		ret = app_info_set(AppName, AppValue);
		if (ret < 0)
		{
			pr_err("%s: app_info_set fail", __func__);
			goto split_fail_exit;
		}
	}

split_fail_exit:
	if (AppStr)
		kfree(AppStr);
	return size;
}
static struct kobj_attribute sys_set_appinfo_init = {
	.attr = {.name = "set_app_info_node", .mode = (S_IRUGO | S_IWUSR | S_IWGRP)},
	.show = NULL,
	.store = hw_set_app_info_node_store,
};

static int app_info_node_init(void)
{
	int err = -100;;
	set_appinfo_node_kobject = kobject_create_and_add("set_app_info", NULL);
	if (!set_appinfo_node_kobject)
	{
		pr_err("%s: create set_app_info folder error!", __func__);
		return -1;
	}

	err = sysfs_create_file(set_appinfo_node_kobject, &sys_set_appinfo_init.attr);
	if (err)
	{
		pr_err("%s: init set_appinfo_node_kobject file fail", __func__);
		return -1;
	}

	return 1;
}

int app_info_set(const char * name, const char * value)
{
    struct info_node *new_node = NULL;
    int name_lenth = 0;
    int value_lenth = 0;

    if(WARN_ON(!name || !value))
        return -1;

    name_lenth = strlen(name);
    value_lenth = strlen(value);

    new_node = kzalloc(sizeof(*new_node), GFP_KERNEL);
    if(new_node == NULL)
    {
        return -1;
    }

    memcpy(new_node->name,name,((name_lenth > (APP_INFO_NAME_LENTH-1))?(APP_INFO_NAME_LENTH-1):name_lenth));
    memcpy(new_node->value,value,((value_lenth > (APP_INFO_VALUE_LENTH-1))?(APP_INFO_VALUE_LENTH-1):value_lenth));

    spin_lock(&app_info_list_lock);
    list_add_tail(&new_node->entry,&app_info_list);
    spin_unlock(&app_info_list_lock);

    return 0;
}

EXPORT_SYMBOL(app_info_set);

static int app_info_proc_show(struct seq_file *m, void *v)
{
    struct info_node *node;

    list_for_each_entry(node, &app_info_list, entry)
    {
        seq_printf(m,"%-32s:%32s\n", node->name, node->value);
    }
    return 0;
}

static int app_info_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, app_info_proc_show, NULL);
}

static const struct file_operations app_info_proc_fops =
{
    .open		= app_info_proc_open,
    .read		= seq_read,
    .llseek		= seq_lseek,
    .release	= single_release,
};

/*
    Function to read the SMEM to get the lpDDR name
*/

void export_ddr_info(unsigned int ddr_vendor_id,unsigned int ddr_size, unsigned int ddr_type )
{
    char ddr_info_all[APP_INFO_VALUE_LENTH];
    char * ddr_info = NULL;
    char *SAMSUNG_DDR = "SAMSUNG";
    char *ELPIDA_DDR = "MICRON";
    char *HYNIX_DDR  = "HYNIX";
    char *MICRON_DDR  = "MICRON";
    char ddr_size_info[8];
    char *ddr_type_info;

	switch (ddr_vendor_id)
	{
		case SAMSUNG_ID:
			ddr_info = SAMSUNG_DDR;
			break;
		case ELPIDA_ID:
			ddr_info = ELPIDA_DDR;
			break;
		case HYNIX_ID:
			ddr_info = HYNIX_DDR;
			break;
		case MICRON_ID:
			ddr_info = MICRON_DDR;
			break;
		default:
			ddr_info = "UNKNOWN";
			break;
	}

	if( ddr_size >= DDR_SIZE_MIN && ddr_size <=DDR_SIZE_MAX ) //should be less than 4GB
	{
	    snprintf(ddr_size_info, 8, "%dGbit", ddr_size);
	}
	else
	{
	    snprintf(ddr_size_info , 8 , "UNKNOWN");
	}

	switch(ddr_type)
	{
		case DDR_TYPE_LPDDR1:
		    ddr_type_info = "LPDDR1";
		    break;
		case DDR_TYPE_LPDDR2:
		    ddr_type_info = "LPDDR2";
		    break;
		case DDR_TYPE_LPDDR3:
		    ddr_type_info = "LPDDR3";
		    break;
		default:
		    ddr_type_info = "UNKNOWN";
		    break;
	}

	snprintf(ddr_info_all,APP_INFO_VALUE_LENTH-1, "%s %s %s", ddr_info,ddr_size_info,ddr_type_info );
    /* Set the vendor name in app_info */
    if (app_info_set("ddr_vendor", ddr_info_all))
        pr_err("Error setting DDR vendor info\n");

    /* Print the DDR Name in the kmsg log */
    pr_err("DDR VENDOR is : %s", ddr_info_all);

    return;
}


void app_info_print_smem(void)
{
    unsigned int ddr_vendor_id = 0;
    /* read share memory and get DDR ID */
    smem_exten_huawei_paramater *smem = NULL;
    unsigned int ddr_size0 = 0,ddr_size1 = 0;
    unsigned int ddr_size=0;
    unsigned int ddr_type = 0;

    smem = smem_alloc(SMEM_ID_VENDOR1, sizeof(smem_exten_huawei_paramater),
						  0,
						  SMEM_ANY_HOST_FLAG);
    if (NULL == smem)
    {
        /* Set the vendor name in app_info */
        if (app_info_set("ddr_vendor", "UNKNOWN"))
            pr_err("Error setting DDR vendor name to UNKNOWN\n");
        pr_err("%s: SMEM Error, READING DDR VENDOR NAME", __func__);
        return;
    }

    ddr_vendor_id = smem->lpddrID;
    ddr_vendor_id &= 0xff;

    ddr_size = (smem->lpddrID >> 8) & 0xFF;
    ddr_type = (smem->lpddrID >> 24) & 0xFF;

    printk(KERN_ERR "ddr_info %x,%d,%d,%d--\n", smem->lpddrID,ddr_size1,ddr_size0, ddr_size );

    export_ddr_info(ddr_vendor_id, ddr_size, ddr_type );

    return;
}

static int __init proc_app_info_init(void)
{
    proc_create("app_info", 0, NULL, &app_info_proc_fops);
    app_info_print_smem();
    update_task = kthread_run(usb_update_thread, NULL, "usb_update");
    if (IS_ERR(update_task))
    {
        printk("usb_update thread run error \n");
    }
    app_info_node_init();
    memset(sensor_type, '\0',sizeof(struct sensor_node)*SENSOR_MAX);

    return 0;
}

module_init(proc_app_info_init);

