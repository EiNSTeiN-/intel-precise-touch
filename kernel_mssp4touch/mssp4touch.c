/*
 * file : mssp4touch.c 
 * desc : Module for Microsoft Surface Pro 4 touchscreen pci device
 *
 * notes: This is a skeleton version of "kvm_mssp4touch.c" by 
 *        Cam Macdonell <cam@cs.ualberta.ca>, Copyright 2009, GPLv2
 *        See git://gitorious.org/nahanni/guest-code.git
 *
 * Olivier Fauchon, ofauchon2204@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "%s:%d:: " fmt, __func__, __LINE__
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/ratelimit.h>

#define MSSP4TOUCH_DEV_NAME "mssp4touch"
#define MSSP4TOUCH_IRQ_ID "mssp4touch_irq_id"
/* ============================================================
 *                         PCI SPECIFIC 
 * ============================================================ */
#include <linux/pci.h>

static struct {
        /* (mmio) control registers i.e. the "register memory region" */
        void __iomem    *regs_base_addr;
        resource_size_t regs_start;
        resource_size_t regs_len;
        /* irq handling */
        unsigned int    irq;
} mssp4touch_dev;

static struct pci_device_id mssp4touch_id_table[] = {
        { PCI_DEVICE(0x8086, 0x9d3e) },
        { 0, },
};

MODULE_DEVICE_TABLE (pci, mssp4touch_id_table);


/*  relevant control register offsets */
enum {
        IntrMask        = 0x00,    /* Interrupt Mask */
        IntrStatus      = 0x04,    /* Interrupt Status */
};




static irqreturn_t mssp4touch_interrupt (int irq, void *dev_id)
{
        u32 status;

        if (unlikely(strcmp((char*)dev_id, MSSP4TOUCH_IRQ_ID)))
                return IRQ_NONE;

        status = readl(mssp4touch_dev.regs_base_addr + IntrStatus);
        if (!status || (status == 0xFFFFFFFF))
                return IRQ_NONE;




	pr_info_ratelimited("int_status:0x%04x addr+0:0x%08x addr+0x4:0x%08x addr+0x08:0x%08x addr+0xc:0x%08x addr+0x800:0x%08x ",
		status,
		readl(mssp4touch_dev.regs_base_addr + 0x000),
		readl(mssp4touch_dev.regs_base_addr + 0x004),
		readl(mssp4touch_dev.regs_base_addr + 0x008),
		readl(mssp4touch_dev.regs_base_addr + 0x00c),
		readl(mssp4touch_dev.regs_base_addr + 0x800));

	//writel(0x0, mssp4touch_dev.regs_base_addr + 0x800); 


        return IRQ_HANDLED;
}






static int mssp4touch_probe(struct pci_dev *pdev,
                         const struct pci_device_id *pdev_id) 
{

        int err;
        struct device *dev = &pdev->dev;

	printk(KERN_INFO "mssp4touch probe!\n");

        if((err = pci_enable_device(pdev))){
                dev_err(dev, "pci_enable_device probe error %d for device %s\n",
                        err, pci_name(pdev));
                return err;
        }

        if((err = pci_request_regions(pdev, MSSP4TOUCH_DEV_NAME)) < 0){
                dev_err(dev, "pci_request_regions error %d\n", err);
                goto pci_disable;
        }

        /* bar0: control registers */
        mssp4touch_dev.regs_start =  pci_resource_start(pdev, 0);
        mssp4touch_dev.regs_len = pci_resource_len(pdev, 0);
        mssp4touch_dev.regs_base_addr = pci_iomap(pdev, 0, 0x0); //0x0 to access to full bar

        if (!mssp4touch_dev.regs_base_addr) {
                dev_err(dev, "cannot ioremap registers of size %lu\n",
                             (unsigned long)mssp4touch_dev.regs_len);
                goto pci_release;
        }

        /* interrupts: set all masks */
        writel(0xffffffff, mssp4touch_dev.regs_base_addr + IntrMask);
        if (request_irq(pdev->irq, mssp4touch_interrupt, IRQF_SHARED,
                                        MSSP4TOUCH_DEV_NAME, MSSP4TOUCH_IRQ_ID)) 
                dev_err(dev, "request_irq %d error\n", pdev->irq);

        dev_info(dev, "regs iomap base = 0x%lx, irq = %u\n",
                        (unsigned long)mssp4touch_dev.regs_base_addr, pdev->irq);
        dev_info(dev, "regs_addr_start = 0x%lx regs_len = %lu\n",
                        (unsigned long)mssp4touch_dev.regs_start, 
                        (unsigned long)mssp4touch_dev.regs_len);
        return 0;

pci_release:
        pci_release_regions(pdev);
pci_disable:
        pci_disable_device(pdev);
        return -EBUSY;
}




static void mssp4touch_remove(struct pci_dev* pdev)
{

        free_irq(pdev->irq, MSSP4TOUCH_IRQ_ID);
        pci_iounmap(pdev, mssp4touch_dev.regs_base_addr);
        //pci_iounmap(pdev, mssp4touch_dev.data_base_addr);
        pci_release_regions(pdev);
        pci_disable_device(pdev);

}

static struct pci_driver mssp4touch_pci_driver = {
        .name      = MSSP4TOUCH_DEV_NAME, 
        .id_table  = mssp4touch_id_table,
        .probe     = mssp4touch_probe,
        .remove    = mssp4touch_remove,
};

/* ============================================================
 *                    THE FILE OPS
 * ============================================================ */
static int mssp4touch_major;
#define MSSP4TOUCH_MINOR 0

static int mssp4touch_mmap(struct file *filp, struct vm_area_struct * vma)
{
/*
        unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

        if((offset + (vma->vm_end - vma->vm_start)) > mssp4touch_dev.data_mmio_len)
            return -EINVAL;

        offset += (unsigned long)mssp4touch_dev.data_mmio_start;

        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

        if(io_remap_pfn_range(vma, vma->vm_start,
                              offset >> PAGE_SHIFT,
                              vma->vm_end - vma->vm_start,
                              vma->vm_page_prot))
                return -EAGAIN;

*/
        return 0;
}

static int mssp4touch_open(struct inode * inode, struct file * filp)
{
         if (MINOR(inode->i_rdev) != MSSP4TOUCH_MINOR) {
                pr_info("minor: %d\n", MSSP4TOUCH_MINOR);
                return -ENODEV;
         }
         return 0;
}

static int mssp4touch_release(struct inode * inode, struct file * filp)
{
         return 0;
}

static const struct file_operations mssp4touch_ops = {
        .owner   = THIS_MODULE,
        .open    = mssp4touch_open,
        .mmap    = mssp4touch_mmap,
        .release = mssp4touch_release,
};

/* ============================================================
 *                  MODULE INIT/EXIT
 * ============================================================ */
#define MSSP4TOUCH_DEVS_NUM 1          /* number of devices */
static struct cdev cdev;            /* char device abstraction */ 
static struct class *mssp4touch_class; /* linux device model */

static int __init mssp4touch_init (void)
{
        int err = -ENOMEM;

        /* obtain major */
        dev_t mjr = MKDEV(mssp4touch_major, 0);
        if((err = alloc_chrdev_region(&mjr, 0, MSSP4TOUCH_DEVS_NUM, MSSP4TOUCH_DEV_NAME)) < 0){
                pr_err("alloc_chrdev_region error\n");
                return err;
        }
        mssp4touch_major = MAJOR(mjr);

        /* connect fops with the cdev */
        cdev_init(&cdev, &mssp4touch_ops);
        cdev.owner = THIS_MODULE;

        /* connect major/minor to the cdev */
        {
                dev_t devt;
                devt = MKDEV(mssp4touch_major, MSSP4TOUCH_MINOR);

                if((err = cdev_add(&cdev, devt, 1))){
                        pr_err("cdev_add error\n");
                        goto unregister_dev;
                }
        }

        /* populate sysfs entries */
        if(!(mssp4touch_class = class_create(THIS_MODULE, MSSP4TOUCH_DEV_NAME))){
                pr_err("class_create error\n");
                goto del_cdev;
        }

        /* create udev '/dev' node */
        {
                dev_t devt = MKDEV(mssp4touch_major, MSSP4TOUCH_MINOR);   
                if(!(device_create(mssp4touch_class, NULL, devt, NULL, MSSP4TOUCH_DEV_NAME"%d", MSSP4TOUCH_MINOR))){
                        pr_err("device_create error\n");
                        goto destroy_class;
                }
        }

        /* register pci device driver */
        if((err = pci_register_driver(&mssp4touch_pci_driver)) < 0){
                pr_err("pci_register_driver error\n");
                goto exit;
        }

        return 0;

exit:
        device_destroy(mssp4touch_class, MKDEV(mssp4touch_major, MSSP4TOUCH_MINOR));
destroy_class:
        class_destroy(mssp4touch_class);
del_cdev:
        cdev_del(&cdev);
unregister_dev:
        unregister_chrdev_region(MKDEV(mssp4touch_major, MSSP4TOUCH_MINOR), MSSP4TOUCH_DEVS_NUM);

        return err;
}

static void __exit mssp4touch_fini(void)
{
        pci_unregister_driver (&mssp4touch_pci_driver);
        device_destroy(mssp4touch_class, MKDEV(mssp4touch_major, MSSP4TOUCH_MINOR));
        class_destroy(mssp4touch_class);
        cdev_del(&cdev);
        unregister_chrdev_region(MKDEV(mssp4touch_major, MSSP4TOUCH_MINOR), MSSP4TOUCH_DEVS_NUM);
}

module_init(mssp4touch_init);
module_exit(mssp4touch_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module for Microsoft Surface Pro 4 touchscreen pci device");



