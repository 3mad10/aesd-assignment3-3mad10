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
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("3mad10"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .unlocked_ioctl =   aesd_ioctl,
    .llseek =   aesd_llseek,
};


static bool has_newline(const char *buffer, size_t length) {
    const char *ptr = buffer;
    for(int i = 0; i < length; i++) {
        PDEBUG("current char : %c", buffer[i]);
        if(buffer[i] == '\n') 
        {
            PDEBUG("found new line");
            return true;
        }
    }
    return false;
}

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev* dev;
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
    struct aesd_dev* dev;
    struct aesd_buffer_entry* read_entry;
    size_t entry_byte_off = 0;
    size_t bytes_to_copy = 0;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    dev = (struct aesd_dev*) filp->private_data;

    if (mutex_lock_interruptible(&dev->lock))
    {
        return -ERESTARTSYS;
    }

    read_entry = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_byte_off);
    if (!read_entry)
    {
        retval = 0;
        goto out;
    }

    bytes_to_copy = read_entry->size - entry_byte_off;
    if (bytes_to_copy > count) {
        bytes_to_copy = count;
    }

    if(copy_to_user(buf, read_entry->buffptr + entry_byte_off, bytes_to_copy))
    {
        retval = -EFAULT;
        goto out;
    }

    retval = bytes_to_copy;
    *f_pos += bytes_to_copy;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    char* kernel_buff;
    const char* popped_entry = NULL;
    struct aesd_dev* dev = (struct aesd_dev*) filp->private_data;
    bool new_line_exist = false;
    char *new_buffptr = NULL;
    size_t i;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&dev->lock))
    {
        return -ERESTARTSYS;
    }

    kernel_buff = kmalloc(count ,GFP_KERNEL);
    if (!kernel_buff) {
        retval = -ENOMEM;
        goto out;
    }

    if(copy_from_user(kernel_buff, buf, count))
    {
        retval = -EFAULT;
        kfree(kernel_buff);
        goto out;
    }

    new_line_exist = has_newline(kernel_buff, count);

    if (dev->new_entry == NULL) {
        dev->new_entry = kmalloc(sizeof(struct aesd_buffer_entry), GFP_KERNEL);
        if (!dev->new_entry) {
            retval = -ENOMEM;
            kfree(kernel_buff);
            goto out;
        }
        dev->new_entry->buffptr = kernel_buff;
        dev->new_entry->size = count;
    } else {
        new_buffptr = krealloc(dev->new_entry->buffptr, dev->new_entry->size + count, GFP_KERNEL);
        if (!new_buffptr) {
            retval = -ENOMEM;
            kfree(kernel_buff);
            goto out;
        }
        dev->new_entry->buffptr = new_buffptr;
        for(i = 0; i < count; i++)
        {
            dev->new_entry->buffptr[dev->new_entry->size+i] = kernel_buff[i];
        }
        dev->new_entry->size += count;
        kfree(kernel_buff);
    }

    if (new_line_exist) {
        popped_entry = aesd_circular_buffer_add_entry(&dev->buffer, dev->new_entry);
        if (popped_entry) {
            kfree((void*)popped_entry);
        }
        kfree(dev->new_entry);
        dev->new_entry = NULL;
    }

    retval = count;
    *f_pos += retval; 

out:
    mutex_unlock(&dev->lock);
    return retval;
}

loff_t get_buffer_size(struct aesd_circular_buffer* buffer)
{
    loff_t buffer_size = 0;

    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        
        if(buffer->entry[i].buffptr != NULL)
        {
            buffer_size += buffer->entry[i].size;
        }
        
    }
    return buffer_size;
}

loff_t aesd_llseek(struct file * filp, loff_t off, int whence)
{
    struct aesd_dev* dev = (struct aesd_dev*) filp->private_data;
    loff_t newpos;
    loff_t buffer_size;
    buffer_size = get_buffer_size(&dev->buffer);
    
    newpos = fixed_size_llseek(filp, off, whence, buffer_size);
    if (newpos >= 0)
    {
        if (mutex_lock_interruptible(&dev->lock))
        {
            return -ERESTARTSYS;
        }
        filp->f_pos = newpos;
        dev->buffer.in_offs = newpos;
        dev->buffer.out_offs = newpos;
        mutex_unlock(&dev->lock);
    }
    return newpos;
}

long aesd_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
    return 0;
}

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
    printk(KERN_INFO "In Init of Char Driver \n");
    /**
     * TODO: initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);

    result = aesd_setup_cdev(&aesd_device);
    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    
    struct aesd_circular_buffer* circular_buffer = &aesd_device.buffer;
    struct aesd_buffer_entry* entry = aesd_device.new_entry;
    cdev_del(&aesd_device.cdev);
    printk(KERN_INFO "In Deinit of Char Driver \n");
    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        kfree(circular_buffer->entry[i].buffptr);
        circular_buffer->entry[i].size = 0;
    }
    if(entry) {
        if(entry->buffptr) 
        {
            kfree(entry->buffptr);
        }
        entry->size = 0;
    }
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
