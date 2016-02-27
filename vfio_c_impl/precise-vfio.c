#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>



#include <linux/vfio.h>
#include <sys/ioctl.h>


int main(){
         int container, group, device, i;
         struct vfio_group_status group_status = { .argsz = sizeof(group_status) };
         struct vfio_iommu_type1_info iommu_info = { .argsz = sizeof(iommu_info) };
         struct vfio_iommu_type1_dma_map dma_map = { .argsz = sizeof(dma_map) };
         struct vfio_device_info device_info = { .argsz = sizeof(device_info) };
 
         /* Create a new container */
         container = open("/dev/vfio/vfio", O_RDWR);
         group = open("/dev/vfio/6", O_RDWR);
 
	if (ioctl(container, VFIO_GET_API_VERSION) != VFIO_API_VERSION){
		printf("! Unknown API error");
		return(0); 
	}
	if (!ioctl(container, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU))
	{
		printf("! %s don't support IOMMU driver we want!\n");
		return(0); 
	}
 
         /* Test the group is viable and available */
         ioctl(group, VFIO_GROUP_GET_STATUS, &group_status);
         if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)){
		printf("! Group is not viable (ie, not all devices bound for vfio\n");
		return(0);
	 }
 
         /* Add the group to the container */
         ioctl(group, VFIO_GROUP_SET_CONTAINER, &container);
 
         /* Enable the IOMMU model we want */
         ioctl(container, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU);
 
         /* Get addition IOMMU info */
         ioctl(container, VFIO_IOMMU_GET_INFO, &iommu_info);
 
 
         /* Get a file descriptor for the device */
         device = ioctl(group, VFIO_GROUP_GET_DEVICE_FD, "0000:00:16.4");
 
         /* Test and setup the device */
         ioctl(device, VFIO_DEVICE_GET_INFO, &device_info);
	printf("info: regions:%x irq:%d\n", device_info.num_regions, device_info.num_irqs); 
 
         for (i = 0; i < device_info.num_regions; i++) {
                 struct vfio_region_info reg = { .argsz = sizeof(reg) };
 
                 reg.index = i;
 
                 ioctl(device, VFIO_DEVICE_GET_REGION_INFO, &reg);
 
                 /* Setup mappings... read/write offsets, mmaps
                  * For PCI devices, config space is a region */
         }
 
         for (i = 0; i < device_info.num_irqs; i++) {
                 struct vfio_irq_info irq = { .argsz = sizeof(irq) };
 
                 irq.index = i;
 
                 ioctl(device, VFIO_DEVICE_GET_IRQ_INFO, &irq);
 
                 /* Setup IRQs... eventfds, VFIO_DEVICE_SET_IRQS */
         }

	ioctl(device, VFIO_MSI_SETUP, 0x8c); 

 
         /* Gratuitous device reset and go... */
         ioctl(device, VFIO_DEVICE_RESET);
}
