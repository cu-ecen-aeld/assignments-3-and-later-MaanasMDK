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
#include "aesdchar.h"
#include "linux/slab.h"
#include "linux/string.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("MaanasMDK"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
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

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev;
    struct aesd_buffer_entry *tmp_buf;
    int tmp_buf_count = 0;
    size_t offset_bytes;
    dev = filp->private_data;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */

    // lock the read access
    mutex_lock(&aesd_device.lock);

    tmp_buf = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &offset_bytes);
    // check if count doesn't exist
    if( tmp_buf == NULL )
    {
        *f_pos = 0;
        goto cleanup;
    }

    // check if tmp_buf has required count
    if( (tmp_buf->size - offset_bytes) < count )
    {
        *f_pos += tmp_buf->size - offset_bytes;
        tmp_buf_count = tmp_buf->size - offset_bytes;
    }
    else
    {
        *f_pos += count;
        tmp_buf_count = count;
    }

    // copy to user buffer
    if( copy_to_user(buf, tmp_buf->buffptr+offset_bytes, tmp_buf_count))
    {
        retval = -EFAULT;
        goto cleanup;
    }

    retval = tmp_buf_count;

    cleanup : mutex_unlock(&aesd_device.lock);
    
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    char *tmp_buf;
    bool packet_flag = false;
    struct aesd_dev *dev;
    int packet_len = 0;
    struct aesd_buffer_entry write_entry;
    char *ret;
    int i;
    ssize_t retval = 0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
    dev = filp->private_data;

    mutex_lock(&aesd_device.lock);

    // malloc a new buffer
    tmp_buf = (char *)kmalloc(count, GFP_KERNEL);
    if( tmp_buf == NULL )
    {
        retval = -ENOMEM;
        goto end;
    }

    // copy from user buffer to new buffer
    if(copy_from_user(tmp_buf, buf, count))
    {
        retval = -EFAULT;
        goto free_and_end;
    }

    // check for newline char 
    for(i=0; i< count; i++)
    {
        if(tmp_buf[i] == '\n')
        {
            packet_flag = true;
            packet_len = i+1;
            break;
        }
    }

    // copy the packet
    if( dev->buff_size == 0 )
    {
        dev->buff_ptr = (char *)kmalloc(count, GFP_KERNEL);
        if( dev->buff_ptr == NULL )
        {
            retval = -ENOMEM;
            goto free_and_end;
        }
        memcpy(dev->buff_ptr, tmp_buf, count);
        dev->buff_size += count;
    }
    else
    {
        int extra;
        if(packet_flag)
        {
            extra = packet_len;
        }
        else
        {
            extra = count;
        }
        dev->buff_ptr = (char *)krealloc(dev->buff_ptr, dev->buff_size + extra , GFP_KERNEL);
        if( dev->buff_ptr == NULL )
        {
            retval = -ENOMEM;
            goto free_and_end;
        }
        memcpy(dev->buff_ptr + dev->buff_size, tmp_buf, extra);
        dev->buff_size += extra;
    }

    // add entry to circular buffer
    if(packet_flag)
    {
        write_entry.buffptr = dev->buff_ptr;
        write_entry.size = dev->buff_size;
        ret = aesd_circular_buffer_add_entry(&dev->buffer, &write_entry);
        if( ret != NULL )
        {
            kfree(ret);
        }
        dev->buff_size = 0;
    }
    retval = count;
    free_and_end : kfree(tmp_buf);
    end : mutex_unlock(&aesd_device.lock);
    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
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
    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    int index;
    struct aesd_buffer_entry *ptr;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    AESD_CIRCULAR_BUFFER_FOREACH(ptr, &aesd_device.buffer, index)
    {
        kfree(ptr->buffptr);
    }
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
