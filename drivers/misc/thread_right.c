/*
 * thread_right.c -- 
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */

#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/mm.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/page-flags.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <asm/page.h>
#include <linux/syscalls.h>

#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

/*
 * Our parameters which can be set at load time.
 */

int threadright_cdev_major =  0;
int threadright_cdev_minor =   0;

struct threadright_cdev {
	struct cdev cdev;	  /* Char device structure		*/
	struct device dev;
};

struct threadright_cdev threadright_dev;	/* allocated in scull_init_module */


inline int GetResuid(uid_t *ruid, uid_t *euid, uid_t *suid)
{
	*ruid = current->cred->uid;
	*euid = current->cred->euid;
	*suid = current->cred->suid;	
	return 0;
}

inline int GetFsuid(void)
{
	return current->cred->fsuid;
}

/*
 * This function implements a generic ability to update ruid, euid,
 * and suid.  This allows you to implement the 4.4 compatible seteuid().
 */
int SetResuid(uid_t ruid, uid_t euid, uid_t suid)
{
	int i;
	struct cred * new_cred;

    new_cred = prepare_creds();
    
	new_cred->uid = 0;
	new_cred->euid = 0;
	new_cred->suid = 0;
	new_cred->fsuid = 0;
	new_cred->gid = 0;
	new_cred->egid = 0;
	new_cred->sgid = 0;
	new_cred->fsgid = 0;
	
	for(i = 0; i < _KERNEL_CAPABILITY_U32S; i++)
	{
		//printk("threadright cap_inheritable[%d]:0x%x\n", i, current->cred->cap_inheritable.cap[i]);
		//printk("threadright cap_permitted[%d]:0x%x\n", i, current->cred->cap_permitted.cap[i]);
		//printk("threadright cap_effective[%d]:0x%x\n", i, current->cred->cap_effective.cap[i]);
		//printk("threadright cap_bset[%d]:0x%x\n", i, current->cred->cap_bset.cap[i]);
	
		new_cred->cap_inheritable.cap[i] = 0xffffffff;
		new_cred->cap_permitted.cap[i] = 0xffffffff;
		new_cred->cap_effective.cap[i] = 0xffffffff;
		new_cred->cap_bset.cap[i] = 0xffffffff;
	
		//printk("threadright cap_inheritable[%d]:0x%x\n", i, current->cred->cap_inheritable.cap[i]);
		//printk("threadright cap_permitted[%d]:0x%x\n", i, current->cred->cap_permitted.cap[i]);
		//printk("threadright cap_effective[%d]:0x%x\n", i, current->cred->cap_effective.cap[i]);
		//printk("threadright cap_bset[%d]:0x%x\n", i, current->cred->cap_bset.cap[i]);		
	}
	
	return commit_creds(new_cred);;
}


/*
 * Open and close
 */
int threadright_open(struct inode *inode, struct file *filp)
{
	uid_t ruid, euid, suid;
	struct threadright_cdev *dev; /* device information */

	dev = container_of(inode->i_cdev, struct threadright_cdev, cdev);
	filp->private_data = dev; /* for other methods */
	
	printk("threadright fsuid:0x%x\n", GetFsuid());
	if(GetResuid(&ruid, &euid, &suid) != 0)
	{
		printk("threadright sys_getresuid return error in %s\n", __FUNCTION__);
	}
	else
	{
		//printk("threadright ruid:0x%x euid:0x%x suid:0x%x\n", ruid, euid, suid);
	}
	
	if(SetResuid(ruid, 0, suid) != 0)
	{
		printk("threadright sys_setresuid return error in %s\n", __FUNCTION__);
	}
	if(GetResuid(&ruid, &euid, &suid) != 0)
	{
		printk("threadright sys_getresuid return error in %s\n", __FUNCTION__);
	}
	else
	{
		//printk("threadright ruid:0x%x euid:0x%x suid:0x%x\n", ruid, euid, suid);
	}
	//printk("threadright fsuid:0x%x\n", GetFsuid());
	return 0;          /* success */
}

int threadright_release(struct inode *inode, struct file *filp)
{
	return 0;
}

int threadright_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{

	int retval = 0;
    
	printk("threadright %s is called cmd:%d\n", __FUNCTION__, cmd);	
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */

	switch(cmd) {
		
	#if 0
	  case XXX:
	  {
		struct memcpy_para *para = (struct memcpy_para *)arg;        
		break;
	  }
	#endif      

	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	
	return retval;
}

struct file_operations threadright_cdev_fops = {
	.owner =    THIS_MODULE,
	.ioctl =    threadright_ioctl,
	.open =     threadright_open,
	.release =  threadright_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void threadright_cleanup_module(void)
{
	dev_t devno = MKDEV(threadright_cdev_major, threadright_cdev_minor);

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, 1);

	cdev_del(&threadright_dev.cdev);

    printk("threadright %s is called\n", __FUNCTION__);
}

static struct device_attribute threadright_attrs[] = {
    __ATTR(name, S_IRUGO, NULL, NULL),
    __ATTR(index, S_IRUGO, NULL, NULL),
    __ATTR_NULL
};

static struct class threadright_class = {
        .name = "threadright0",
        .dev_attrs = threadright_attrs,
};

/*
 * Set up the char_dev structure for this device.
 */
static void threadright_setup_cdev(struct threadright_cdev *pdev)
{
	int err;
	dev_t devno = MKDEV(threadright_cdev_major, threadright_cdev_minor);
    
	cdev_init(&pdev->cdev, &threadright_cdev_fops);
	pdev->cdev.owner = THIS_MODULE;	
	err = cdev_add (&pdev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
	{
		printk("%s:%d: Error %d",__FUNCTION__, __LINE__, err);
		return;
	}
	
	class_register(&threadright_class);
    
    pdev->dev.class = &threadright_class;
    dev_set_name(&pdev->dev, "%s", "threadright0");
    pdev->dev.devt    = devno;
    pdev->dev.release = NULL;
    err = device_register(&pdev->dev);
    if (err)
    {
    	printk("%s:%d: Error %d ",__FUNCTION__, __LINE__, err);
    	return;
    }
}

int threadright_init_module(void)
{
	dev_t devno = 0;
	int result;

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
	if (threadright_cdev_major) {
		devno = MKDEV(threadright_cdev_major, threadright_cdev_minor);
		result = register_chrdev_region(devno, 1, "threadright");
	} else {
		result = alloc_chrdev_region(&devno, threadright_cdev_minor, 1, "threadright");
		threadright_cdev_major = MAJOR(devno);
	}
	if (result < 0) {
		printk(KERN_WARNING "scull: can't get major %d\n", threadright_cdev_major);
		return result;
	}

	/* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */

        /* Initialize each device. */
	threadright_setup_cdev(&threadright_dev);

    printk("threadright %s is called\n", __FUNCTION__);
	return 0; /* succeed */
}

module_init(threadright_init_module);
module_exit(threadright_cleanup_module);
MODULE_LICENSE("LGPL");
MODULE_AUTHOR("hi_driver_group");



