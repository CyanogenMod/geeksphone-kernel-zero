#define DEBUG  1
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <asm/atomic.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>
#include <linux/module.h>
#include <mach/oem_rapi_client.h>
#include <linux/semaphore.h>
ssize_t rpc_client_misc_read(struct file *filp,char __user *buf,size_t count, loff_t *f_pos)
{
	return 0;
}
struct file_operations misc_rpc_client_fops = 
{
	.owner = THIS_MODULE,
	.read = &rpc_client_misc_read,
};
struct miscdevice  misc_rpc_client =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = "misc_rpc_client",
	.fops = &misc_rpc_client_fops,
};

static int g_client_id[OEM_RAPI_CLIENT_EVENT_MAX]={0,0,0,0,0,};

static struct msm_rpc_client *g_client;
static DECLARE_MUTEX(g_client_mutex);
int oem_rpc_client_register(int id)
{
	printk(KERN_ERR "%s  ++++\n",__func__);
	if(id == OEM_RAPI_CLIENT_EVENT_NONE || id > (OEM_RAPI_CLIENT_EVENT_MAX - 1))
		return 0;
	if(g_client_id[id] == 0)	
	{
		g_client_id[id] = id;
		printk(KERN_ERR "%s  sucess\n",__func__);
		return id;
	}
	printk(KERN_ERR "%s  ----\n",__func__);
	return 0;

}
EXPORT_SYMBOL(oem_rpc_client_register);
void set_data_to_arm9(int id, char *in,int insize)
{
	struct oem_rapi_client_streaming_func_arg arg;
	struct oem_rapi_client_streaming_func_ret ret;
	int ret_val = 0;

	if(id == OEM_RAPI_CLIENT_EVENT_NONE || id > (OEM_RAPI_CLIENT_EVENT_MAX - 1) || in == NULL || insize > 128 || insize ==0)
		return ;
	if(g_client_id[id] == 0)
	{
		printk(KERN_ERR "\nunregister client id \n");
		return;
	}

	do	
	{
	
		arg.cb_func = NULL;
		arg.handle = (void *)0;
		arg.in_len = insize;
		arg.input = in;
		arg.out_len_valid = 0;
		arg.output_valid = 0;
		arg.output_size = 0;
		arg.event = id;
	
		ret.out_len =  NULL;
		ret.output = NULL;
		down(&g_client_mutex);
		ret_val = oem_rapi_client_streaming_function(g_client,&arg,&ret);
		up(&g_client_mutex);
	
	}while(0);
}
EXPORT_SYMBOL(set_data_to_arm9);
#define TEST_RPC_CLIENT_OEM
#ifdef TEST_RPC_CLIENT_OEM
void get_version_from_arm9()
{


	struct oem_rapi_client_streaming_func_arg arg;
	struct oem_rapi_client_streaming_func_ret ret;
	char *input;
	char output[128];
	uint32_t out_len = 128;
	int ret_val = 0;
	do	
	{
		//input = kmalloc(strlen("sending data to modem..."),GFP_KERNEL);
		//strcpy(input, "sending data to modem ...");

		arg.cb_func = NULL;
		arg.handle = (void *)0;
		arg.in_len = 0;
		arg.input = NULL;
		arg.out_len_valid = 1;
		arg.output_valid = 1;
		arg.output_size = 128;
		arg.event = 1;
	
		ret.out_len =  &out_len;
		ret.output = output;
		ret_val = oem_rapi_client_streaming_function(g_client,&arg,&ret);
		printk(KERN_ERR "\n!!!!!%s!!!!!\n",output);

	}while(0);
}
EXPORT_SYMBOL(get_version_from_arm9);
void set_version_to_arm9()
{


	struct oem_rapi_client_streaming_func_arg arg;
	struct oem_rapi_client_streaming_func_ret ret;
	char *input;
	char output[128];
	uint32_t out_len;
	int ret_val = 0;
	do	
	{
	
		input = kmalloc(strlen("sending data to modem..."),GFP_KERNEL);
		strcpy(input, "sendsssng data to modem ...");

set_data_to_arm9(2,input,strlen(input)+1);
return;
		arg.cb_func = NULL;
		arg.handle = (void *)0;
		arg.in_len = strlen(input) + 1;
		arg.input = input;
		arg.out_len_valid = 0;
		arg.output_valid = 0;
		arg.output_size = 0;
		arg.event = 2;
	
		ret.out_len =  NULL;
		ret.output = NULL;
		ret_val = oem_rapi_client_streaming_function(g_client,&arg,&ret);
	
	}while(0);
}
EXPORT_SYMBOL(set_version_to_arm9);
#endif
static int __init msm_rpc_client_init(void)
{
	pr_debug("%s: enter\n", __func__);
	g_client = oem_rapi_client_init();
	//oem_rpc_client_register(2);

	misc_register(&misc_rpc_client);

	return 0;
}

static void __exit msm_rpc_client_exit(void)
{
	oem_rapi_client_close();
	misc_deregister(&misc_rpc_client);
}

module_init(msm_rpc_client_init);
module_exit(msm_rpc_client_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("ian sim");
MODULE_DESCRIPTION("version driver ");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:version");
