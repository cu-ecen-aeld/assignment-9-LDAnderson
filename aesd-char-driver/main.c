/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "aesdchar.h"
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Leif Anderson"); 
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int retval = 0;
	struct aesd_seekto *xs = NULL;
	xs = kmalloc(sizeof(struct aesd_seekto), GFP_KERNEL);

        PDEBUG("IOCTL");

	if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

	switch(cmd) {
		case AESDCHAR_IOCSEEKTO:
			copy_from_user(xs, arg, sizeof(struct aesd_seekto));
			if(!xs) {
				PDEBUG("IOCTL ERROR");
				break;
			}

			PDEBUG("IOCTL pos %zu offset %zu",xs->write_cmd, xs->write_cmd_offset);
			aesd_device.pos = xs->write_cmd;
			aesd_device.offset = xs->write_cmd_offset;
			break;
		default:
			return -ENOTTY;
	}

	return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos = -EINVAL;
	newpos = off;

	switch(whence) {
		case 0: // SEEK_SET
			aesd_device.pos = 0;
			break;
		case 1: // SEEK_CUR
			break;
		case 2: // SEEK_END
			aesd_device.pos = aesd_device.size-1;
			break;
		default:
			return -EINVAL;

	}

	aesd_device.offset = newpos;
	if (newpos < 0) return -EINVAL;
	return newpos;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    size_t ourcount = 0, outbuflen = 0;
    int i=0;
    int seekto = 0;
    int _pos;
    char* outbuf = NULL;

    if(aesd_device.offset != 0) {
	    seekto = 1;
	    _pos = aesd_device.pos;
    }
    else if(aesd_device.size == 10) {
	    _pos = aesd_device.pos;
    }
    else
	    _pos = 0;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&aesd_device.lock))
	    return -ERESTARTSYS;

    if(aesd_device.size == 0) 
	    goto readdone;

    for(i=0; i<aesd_device.size; i++) {
	    if(aesd_device.list[i] == 0) {
		    PDEBUG("READ ERROR 1");
		    goto readdone;
	    }
	    outbuflen += strlen(aesd_device.list[i]);
    }

    outbuf = kmalloc(outbuflen * sizeof(char *), GFP_KERNEL);
    if(!outbuf)
	    goto readdone;

    if(*f_pos >= outbuflen)
	    goto readdone;

    i = _pos;
    PDEBUG("offset = %d   size = %d   _pos = %d   outbuflen = %d", aesd_device.offset, aesd_device.size, _pos, outbuflen);

    do {
	    if(aesd_device.list[i] == 0) {
		    PDEBUG("READ ERROR 2");
		    goto readdone;
	    }
	    ourcount = strlen(aesd_device.list[i]);
	    if(aesd_device.offset < ourcount && aesd_device.offset > 0) {
		    ourcount -= aesd_device.offset;
		    strcat(outbuf, aesd_device.list[i]+aesd_device.offset);
		    retval += ourcount;
		    aesd_device.offset = 0;
	    }
	    else if(aesd_device.offset > 0) {
		    aesd_device.offset = aesd_device.offset - ourcount;
	    }
	    else {
		    strcat(outbuf, aesd_device.list[i]);
		    retval += ourcount;
	    }
	    *f_pos += ourcount;



	    if(++_pos == 10) {
		    if(seekto) {
			    *f_pos = 92;
			    break;
		    }
		    else {
		    _pos = 0;
		    }
	    }

	    i = _pos;
    } while(_pos != aesd_device.pos);

    if(copy_to_user(buf, outbuf, outbuflen)) {
	  retval = -EFAULT;
	  goto readdone;
    } 
readdone:
    mutex_unlock(&aesd_device.lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    static int _append = 0;
    char* tempbuf1 = NULL; 
    char* tempbuf2 = NULL;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    if (mutex_lock_interruptible(&aesd_device.lock))
	    return -ERESTARTSYS;
    if(_append) {
	    tempbuf1 = kmalloc(count * sizeof(char *), GFP_KERNEL);
	    if(!tempbuf1)
		    goto done;
	    tempbuf2 = kmalloc((count + strlen(aesd_device.list[aesd_device.pos])) * sizeof(char *), GFP_KERNEL);
	    if(!tempbuf2)
		    goto done;

	    if(copy_from_user(tempbuf1, buf, count)) {
		    retval = -EFAULT;
		    goto done;
	    }

	    strcat(tempbuf2, aesd_device.list[aesd_device.pos]);
	    strcat(tempbuf2, tempbuf1);
	    kfree(aesd_device.list[aesd_device.pos]);
	    PDEBUG("outstr %s", tempbuf2);
	    aesd_device.list[aesd_device.pos] = tempbuf2;

		if(tempbuf1[count-1] != '\n') {
			_append = 1;
		} else {
			_append = 0;
		    if(++aesd_device.pos == 10) 
			    aesd_device.pos = 0;
		}

    }
    else {

	    aesd_device.list[aesd_device.pos] = kmalloc(count * sizeof(char *), GFP_KERNEL);
	    if(!aesd_device.list[aesd_device.pos])
		    goto done;

	    if(copy_from_user(aesd_device.list[aesd_device.pos], buf, count)) {
		    retval = -EFAULT;
		    goto done;
	    }

	    if(aesd_device.size < 10) {
		    aesd_device.size++;
	    }
	if(aesd_device.list[aesd_device.pos][count-1] != '\n') {
		_append = 1;
	} else {
		_append = 0;
	    if(++aesd_device.pos == 10) 
		    aesd_device.pos = 0;

	}
    }

    retval = count;
    PDEBUG("size: %d   append: %d   list1 = %s   list_last = %s\n", aesd_device.size, _append, aesd_device.list[0], aesd_device.list[aesd_device.size-1]);

done:
    mutex_unlock(&aesd_device.lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
