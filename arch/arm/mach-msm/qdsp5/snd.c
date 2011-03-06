/* arch/arm/mach-msm/qdsp5/snd.c
 *
 * interface to "snd" service on the baseband cpu
 *
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/msm_audio.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <asm/ioctls.h>
#include <mach/board.h>
#include <mach/msm_rpcrouter.h>
#include <mach/debug_mm.h>
#include <mach/qdsp5/snd_adie.h>
#include "audmgr.h"

struct snd_ctxt {
	struct mutex lock;
	int opened;
	struct msm_rpc_endpoint *ept;
	struct msm_snd_endpoints *snd_epts;
	struct audmgr audmgr;
};

struct snd_sys_ctxt {
	struct mutex lock;
	int opened;
	struct msm_rpc_endpoint *ept;
    int adie_client;
};

static struct snd_sys_ctxt the_snd_sys = {
    .adie_client = -1,
};

static struct snd_ctxt the_snd;

#define RPC_SND_PROG	0x30000002
#define RPC_SND_CB_PROG	0x31000002

#define RPC_SND_VERS                    0x00020001

#define SND_SET_DEVICE_PROC 2
#define SND_SET_VOLUME_PROC 3
#define SND_AVC_CTL_PROC 29
#define SND_AGC_CTL_PROC 30
#define SND_ACM_DIAG_REQ_PROC 37

struct rpc_snd_set_device_args {
	uint32_t device;
	uint32_t ear_mute;
	uint32_t mic_mute;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_set_volume_args {
	uint32_t device;
	uint32_t method;
	uint32_t volume;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_avc_ctl_args {
	uint32_t avc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_agc_ctl_args {
	uint32_t agc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct snd_set_device_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_device_args args;
};

struct snd_set_volume_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_volume_args args;
};

struct snd_avc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_avc_ctl_args args;
};

struct snd_agc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_agc_ctl_args args;
};

struct snd_endpoint *get_snd_endpoints(int *size);

static inline int check_mute(int mute)
{
	return (mute == SND_MUTE_MUTED ||
		mute == SND_MUTE_UNMUTED) ? 0 : -EINVAL;
}

static int get_endpoint(struct snd_ctxt *snd, unsigned long arg)
{
	int rc = 0, index;
	struct msm_snd_endpoint ept;

	if (copy_from_user(&ept, (void __user *)arg, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid read pointer\n");
		return -EFAULT;
	}

	index = ept.id;
	if (index < 0 || index >= snd->snd_epts->num) {
		MM_ERR("snd_ioctl get endpoint: invalid index!\n");
		return -EINVAL;
	}

	ept.id = snd->snd_epts->endpoints[index].id;
	strncpy(ept.name,
		snd->snd_epts->endpoints[index].name,
		sizeof(ept.name));

	if (copy_to_user((void __user *)arg, &ept, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid write pointer\n");
		rc = -EFAULT;
	}

	return rc;
}

struct acm_cmd_struct_header
{
  uint16_t cmd_id; /**< a value from acm_cmd_code_enum*/
  uint32_t cmd_buf_length;/**< buffer length excluding header size*/
} __attribute__((__packed__));

struct acm_diag_pkg
{
  struct acm_cmd_struct_header header;
  uint8_t buf[0];
} __attribute__((__packed__));

struct rpc_snd_acm_diag_args {
    uint32_t diag_pkg_size;
    struct acm_diag_pkg diag_pkg;
} __attribute__((__packed__));

struct snd_acm_diag_req {
	struct rpc_request_hdr hdr;
	struct rpc_snd_acm_diag_args args;
} __attribute__((__packed__));

struct snd_acm_diag_rep {
	struct rpc_reply_hdr hdr;
	struct rpc_snd_acm_diag_args args;
} __attribute__((__packed__));

static int get_acm_diag_pkg(
    uint16_t cmd_id, 
    uint8_t *data, uint32_t len, 
    struct acm_diag_pkg **pkg) 
{
    int ret = -ENOMEM;
    uint32_t size, pad_len = 0;
    
    size = sizeof(struct acm_cmd_struct_header) + len;
    if (size & 0x3)
    {
        pad_len = 4 - (size & 0x3);
    }
    size += pad_len;
    
	*pkg = kmalloc(size, GFP_KERNEL);
    if (*pkg) {
        memset(*pkg, 0, size);
        (*pkg)->header.cmd_id = cmd_id;
        (*pkg)->header.cmd_buf_length = len + pad_len;
        if (data)
            memcpy((*pkg)->buf, data, len);
        ret = 0;
    }
    
    return ret;
}

void dump_mem(const char *name, unsigned long addr,
		     unsigned long size) 
 {
    unsigned long i, end;
    char str[16*3 + 1];

    end = addr + size;
	printk("%s(0x%08lx to 0x%08lx size %ld)\n", name, addr, end, size);
    
    while (addr < end) {
		printk("%04lx:", addr & 0xffff);
        memset(str, 0, sizeof(str));
        for (i = 0; i < 16; i++) {
			sprintf(str + i * 3, " %02x", *(uint8_t*)addr);
            addr++;
            if (addr >= end) {
                break;
            }
        }
		printk("%s\n", str);
    }
}

static int req_acm_diag_pkg(
    struct msm_rpc_endpoint *ept,
    struct acm_diag_pkg *req_pkg_ptr)
{
    int rc = -ENOMEM;
    int pkg_size, msg_size;
    struct snd_acm_diag_req *req_msg_ptr = 0;
    struct snd_acm_diag_rep *rep_msg_ptr = 0;
    
    msg_size = sizeof(struct snd_acm_diag_req) + req_pkg_ptr->header.cmd_buf_length;
    
	req_msg_ptr = kmalloc(msg_size, GFP_KERNEL);
    memset(req_msg_ptr, 0xFF, msg_size);
    
    if (req_msg_ptr) {
        rep_msg_ptr = req_msg_ptr;
        pkg_size = sizeof(struct acm_diag_pkg) + req_pkg_ptr->header.cmd_buf_length;
        memcpy(&req_msg_ptr->args.diag_pkg, req_pkg_ptr, pkg_size);
        req_msg_ptr->args.diag_pkg_size = cpu_to_be32(pkg_size);
        
        // MM_INFO("req_cmd_buf_length:%d\n", req_pkg_ptr->header.cmd_buf_length);        
        // MM_INFO("req_diag_pkg_size:%d\n", pkg_size);        
        // MM_INFO("msg_size:%d\n", msg_size);  
        
        // dump_mem("req_pkg_ptr", (unsigned long)req_pkg_ptr, pkg_size);
        dump_mem("req_msg_ptr", (unsigned long)req_msg_ptr, msg_size);
        
        rc = msm_rpc_call_reply(ept,
        	SND_ACM_DIAG_REQ_PROC,
        	req_msg_ptr, msg_size, rep_msg_ptr, msg_size, 5 * HZ);
        // dump_mem("rep_msg_ptr", (unsigned long)rep_msg_ptr, msg_size);
        pkg_size = be32_to_cpu(rep_msg_ptr->args.diag_pkg_size);
        memcpy(req_pkg_ptr, &rep_msg_ptr->args.diag_pkg, pkg_size);
        dump_mem("rep_pkg_ptr", (unsigned long)req_pkg_ptr, pkg_size);
    }

    kfree(req_msg_ptr);
    return rc;
}

static long snd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct snd_set_device_msg dmsg;
	struct snd_set_volume_msg vmsg;
	struct snd_avc_ctl_msg avc_msg;
	struct snd_agc_ctl_msg agc_msg;
	struct audmgr_config audmgr_cfg;

	struct msm_snd_device_config dev;
	struct msm_snd_volume_config vol;
    struct acm_diag_req acm_diag_req;
    struct acm_diag_pkg *acm_req_pkg;
	struct snd_ctxt *snd = file->private_data;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	uint32_t avc, agc;

	mutex_lock(&snd->lock);
	switch (cmd) {
    case AUDIO_ENABLE_SND_DEVICE:
		MM_INFO("AUDIO_ENABLE_SND_DEVICE\n");
    	/* Codec / method configure to audmgr client */
    	audmgr_cfg.tx_rate = RPC_AUD_DEF_SAMPLE_RATE_8000;
    	audmgr_cfg.rx_rate = RPC_AUD_DEF_SAMPLE_RATE_8000;
    	audmgr_cfg.def_method = RPC_AUD_DEF_METHOD_VOICE;
		audmgr_cfg.codec = RPC_AUD_DEF_CODEC_VOC_UMTS;
    	audmgr_cfg.snd_method = RPC_SND_METHOD_VOICE;
    	rc = audmgr_enable(&snd->audmgr, &audmgr_cfg);
        break;
    case AUDIO_DISABLE_SND_DEVICE:
		MM_INFO("AUDIO_DISABLE_SND_DEVICE\n");
		rc = audmgr_disable(&snd->audmgr);
        break;
	case SND_SET_DEVICE:
		if (copy_from_user(&dev, (void __user *) arg, sizeof(dev))) {
			MM_ERR("set device: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		dmsg.args.device = cpu_to_be32(dev.device);
		dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
		dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
		if (check_mute(dev.ear_mute) < 0 ||
				check_mute(dev.mic_mute) < 0) {
			MM_ERR("set device: invalid mute status\n");
			rc = -EINVAL;
			break;
		}
		dmsg.args.cb_func = -1;
		dmsg.args.client_data = 0;

		MM_INFO("snd_set_device %d %d %d\n", dev.device,
				dev.ear_mute, dev.mic_mute);

		rc = msm_rpc_call(snd->ept,
			SND_SET_DEVICE_PROC,
			&dmsg, sizeof(dmsg), 5 * HZ);
		break;

	case SND_SET_VOLUME:
		if (copy_from_user(&vol, (void __user *) arg, sizeof(vol))) {
			MM_ERR("set volume: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		vmsg.args.device = cpu_to_be32(vol.device);
		vmsg.args.method = cpu_to_be32(vol.method);
		if (vol.method != SND_METHOD_VOICE) {
			MM_ERR("set volume: invalid method\n");
			rc = -EINVAL;
			break;
		}

		vmsg.args.volume = cpu_to_be32(vol.volume);
		vmsg.args.cb_func = -1;
		vmsg.args.client_data = 0;

		MM_INFO("snd_set_volume %d %d %d\n", vol.device,
				vol.method, vol.volume);

		rc = msm_rpc_call(snd->ept,
			SND_SET_VOLUME_PROC,
			&vmsg, sizeof(vmsg), 5 * HZ);
		break;

	case SND_AVC_CTL:
		if (get_user(avc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((avc != 1) && (avc != 0)) {
			rc = -EINVAL;
			break;
		}

		avc_msg.args.avc_ctl = cpu_to_be32(avc);
		avc_msg.args.cb_func = -1;
		avc_msg.args.client_data = 0;

		MM_INFO("snd_avc_ctl %d\n", avc);

		rc = msm_rpc_call(snd->ept,
			SND_AVC_CTL_PROC,
			&avc_msg, sizeof(avc_msg), 5 * HZ);
		break;

	case SND_AGC_CTL:
		if (get_user(agc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((agc != 1) && (agc != 0)) {
			rc = -EINVAL;
			break;
		}
		agc_msg.args.agc_ctl = cpu_to_be32(agc);
		agc_msg.args.cb_func = -1;
		agc_msg.args.client_data = 0;

		MM_INFO("snd_agc_ctl %d\n", agc);

		rc = msm_rpc_call(snd->ept,
			SND_AGC_CTL_PROC,
			&agc_msg, sizeof(agc_msg), 5 * HZ);
		break;

	case SND_GET_NUM_ENDPOINTS:
		if (copy_to_user((void __user *)arg,
				&snd->snd_epts->num, sizeof(unsigned))) {
			MM_ERR("get endpoint: invalid pointer\n");
			rc = -EFAULT;
		}
		break;

	case SND_GET_ENDPOINT:
		rc = get_endpoint(snd, arg);
		break;

    case AUDIO_SET_AUX_PGA_GAIN:
		rc = adie_svc_config_adie_block(snd_sys->adie_client, AUX_PGA_GAIN, arg);
        break;

    case SND_ACM_DIAG_REQ:
        do {
            acm_req_pkg = 0;
    		if (copy_from_user(&acm_diag_req, 
                    (void __user *)arg, sizeof(acm_diag_req))) {
    			MM_ERR("acm diag req: invalid pointer\n");
    			rc = -EFAULT;
                break;
    		}
            MM_INFO("cmd_id:%d, size:%d\n", acm_diag_req.cmd, acm_diag_req.size);
            
            rc = get_acm_diag_pkg(acm_diag_req.cmd, 0, acm_diag_req.size, &acm_req_pkg);
            if (rc)
            {
    			MM_ERR("acm diag req: get req diag pkg failed\n");
                break;
            }
    		if (copy_from_user(acm_req_pkg->buf, 
                    (void __user *)acm_diag_req.buf,
                    acm_diag_req.size)) {
    			MM_ERR("acm diag req: invalid buf pointer\n");
    			rc = -EFAULT;
                break;
    		}
                       
            rc = req_acm_diag_pkg(snd->ept, acm_req_pkg);
            if (rc < 0) {
    			MM_ERR("acm diag req: req_acm_diag_pkg failed(%d)\n", rc);
                break;
            }
            
    		if (copy_to_user((void __user *)acm_diag_req.buf,
                    acm_req_pkg->buf,
                    acm_diag_req.size)) {
    			MM_ERR("acm diag req: invalid buf pointer\n");
    			rc = -EFAULT;
                break;
    		}            
        } while (0);
        kfree(acm_req_pkg);
        break;
	default:
		MM_ERR("unknown command\n");
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&snd->lock);

	return rc;
}

static int snd_release(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = file->private_data;
	int rc;

	mutex_lock(&snd->lock);
	audmgr_disable(&snd->audmgr);
	audmgr_close(&snd->audmgr);    
	rc = msm_rpc_close(snd->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd->ept = NULL;
	snd->opened = 0;
	mutex_unlock(&snd->lock);
	return 0;
}
static int snd_sys_release(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	rc = msm_rpc_close(snd_sys->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd_sys->ept = NULL;
    snd_sys->opened = 0;
	mutex_unlock(&snd_sys->lock);
	return rc;
}
static int snd_open(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = &the_snd;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd->lock);
	if (snd->opened == 0) {
		if (snd->ept == NULL) {
			snd->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS, 0);
			if (IS_ERR(snd->ept)) {
				rc = PTR_ERR(snd->ept);
				snd->ept = NULL;
				MM_ERR("failed to connect snd svc\n");
				goto err;
			}
		}

    	rc = audmgr_open(&snd->audmgr);
        if (rc < 0) { 
    		MM_ERR("failed to open audmgr\n");
    		goto err;
        }

        if (snd_sys->adie_client < 0) {
        	snd_sys->adie_client = adie_svc_get();
        	if (snd_sys->adie_client < 0) {
            	rc = -ENODEV;
    			MM_ERR("failed to get adie svc\n");
    			goto err;
        	}
        }
        
		file->private_data = snd;
		snd->opened = 1;
	} else {
		MM_ERR("snd already opened\n");
		rc = -EBUSY;
	}

err:
	mutex_unlock(&snd->lock);
	return rc;
}
static int snd_sys_open(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	if (!snd_sys->opened) {
		snd_sys->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
			RPC_SND_VERS, 0);
		if (IS_ERR(snd_sys->ept)) {
			rc = PTR_ERR(snd_sys->ept);
			snd_sys->ept = NULL;
			MM_ERR("failed to connect snd svc\n");
			goto err;
		}
        snd_sys->opened = 1;
	} else
		MM_DBG("snd already opened\n");

err:
	mutex_unlock(&snd_sys->lock);
	return rc;
}

static struct file_operations snd_fops = {
	.owner		= THIS_MODULE,
	.open		= snd_open,
	.release	= snd_release,
	.unlocked_ioctl	= snd_ioctl,
};

struct miscdevice snd_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_snd",
	.fops	= &snd_fops,
};

static long snd_agc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_agc_ctl_msg agc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	agc_msg.args.agc_ctl = cpu_to_be32(arg);
	agc_msg.args.cb_func = -1;
	agc_msg.args.client_data = 0;

	MM_DBG("snd_agc_ctl %ld,%d\n", arg, agc_msg.args.agc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AGC_CTL_PROC,
		&agc_msg, sizeof(agc_msg), 5 * HZ);
	return rc;
}

static long snd_avc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_avc_ctl_msg avc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	avc_msg.args.avc_ctl = cpu_to_be32(arg);

	avc_msg.args.cb_func = -1;
	avc_msg.args.client_data = 0;

	MM_DBG("snd_avc_ctl %ld,%d\n", arg, avc_msg.args.avc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AVC_CTL_PROC,
		&avc_msg, sizeof(avc_msg), 5 * HZ);
	return rc;
}

static ssize_t snd_agc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_agc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_agc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_avc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_avc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_avc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static long snd_vol_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_volume_msg vmsg;
	struct msm_snd_volume_config vol;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &vol.device, &vol.method, &vol.volume);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <method> \
				<volume>\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.device = cpu_to_be32(vol.device);
	vmsg.args.method = cpu_to_be32(vol.method);
	if (vol.method != SND_METHOD_VOICE) {
		MM_ERR("snd_ioctl set volume: invalid method\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.volume = cpu_to_be32(vol.volume);
	vmsg.args.cb_func = -1;
	vmsg.args.client_data = 0;

	MM_DBG("snd_set_volume %d %d %d\n", vol.device, vol.method,
			vol.volume);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_VOLUME_PROC,
		&vmsg, sizeof(vmsg), 5 * HZ);
	return rc;
}

static long snd_dev_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_device_msg dmsg;
	struct msm_snd_device_config dev;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &dev.device, &dev.ear_mute, &dev.mic_mute);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <ear_mute> \
				<mic_mute>\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.device = cpu_to_be32(dev.device);
	dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
	dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
	if (check_mute(dev.ear_mute) < 0 ||
			check_mute(dev.mic_mute) < 0) {
		MM_ERR("snd_ioctl set device: invalid mute status\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.cb_func = -1;
	dmsg.args.client_data = 0;

	MM_INFO("snd_set_device %d %d %d\n", dev.device, dev.ear_mute,
			dev.mic_mute);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_DEVICE_PROC,
		&dmsg, sizeof(dmsg), 5 * HZ);
	return rc;
}

static ssize_t snd_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_dev_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_vol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_vol_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static long snd_aux_pga_gain(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int value;
	int rc = 0;

	rc = sscanf(arg, "%d", &value);
	if (rc != 1) {
		MM_ERR("Invalid arguments. Usage: <gain>\n");
		rc = -EINVAL;
		return rc;
	}

	MM_INFO("snd_aux_pga_gain %d\n", value);

	rc = adie_svc_config_adie_block(snd_sys->adie_client, AUX_PGA_GAIN, value);
	return rc;
}

static ssize_t snd_aux_pga_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	if (snd_sys->adie_client < 0) {
		snd_sys->adie_client = adie_svc_get();
    	if (snd_sys->adie_client < 0) {
        	rc = -ENODEV;
    		MM_ERR("failed to get adie svc\n");
			return rc;
    	}
    }
    
	mutex_lock(&snd_sys->lock);
	status = snd_aux_pga_gain(buf);
	mutex_unlock(&snd_sys->lock);

	return status ? : size;
}

static DEVICE_ATTR(agc, S_IWUSR | S_IRUGO,
		NULL, snd_agc_store);

static DEVICE_ATTR(avc, S_IWUSR | S_IRUGO,
		NULL, snd_avc_store);

static DEVICE_ATTR(device, S_IWUSR | S_IRUGO,
		NULL, snd_dev_store);

static DEVICE_ATTR(volume, S_IWUSR | S_IRUGO,
		NULL, snd_vol_store);

static DEVICE_ATTR(aux_pga, S_IWUSR | S_IRUGO,
		NULL, snd_aux_pga_store);

static int snd_probe(struct platform_device *pdev)
{
	struct snd_ctxt *snd = &the_snd;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_init(&snd->lock);
	mutex_init(&snd_sys->lock);
	snd_sys->ept = NULL;
	snd->snd_epts = (struct msm_snd_endpoints *)pdev->dev.platform_data;
	rc = misc_register(&snd_misc);
	if (rc)
		return rc;

	rc = device_create_file(snd_misc.this_device, &dev_attr_agc);
	if (rc) {
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_avc);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_device);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_volume);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_device);
		misc_deregister(&snd_misc);
		return rc;
    }
    
	rc = device_create_file(snd_misc.this_device, &dev_attr_aux_pga);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_device);
		device_remove_file(snd_misc.this_device,
						&dev_attr_volume);
		misc_deregister(&snd_misc);
	}

	return rc;
}

static struct platform_driver snd_plat_driver = {
	.probe = snd_probe,
	.driver = {
		.name = "msm_snd",
		.owner = THIS_MODULE,
	},
};

static int __init snd_init(void)
{
	return platform_driver_register(&snd_plat_driver);
}

module_init(snd_init);
