
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#define MAP_SIZE 4000

struct pcidev {
	uint16_t vendor_id;  		//0x00
	uint16_t device_id;  		//0x02
	uint16_t command;    		//0x04
	uint16_t status;     		//0x06
	uint8_t cache_line_size;	//0x0c
	uint8_t latency_timer;		//0x0d
	uint8_t header_type; 		//0x0e
	uint8_t bist; 			//0x0f
	uint32_t addr0; 		//0x10
	uint32_t addr1; 		//0x14
	uint32_t addr2; 		//0x18
	uint32_t addr3; 		//0x1C
	uint32_t addr4; 		//0x20
	uint32_t addr5; 		//0x24
	uint32_t rom; 			//0x30
	uint32_t cardbus_cis; 		//0x28
	uint8_t intline;		//0x3c
	uint8_t intpin;			//0x3d
	uint8_t cap_offset_first;	//0x34
	uint16_t cap_val1;		//0x50???
	uint16_t cap_unk;		//0x54???
	uint16_t unk8e;			//0x8e???
};
	

void dump_hex(void* buf, unsigned char sz){
	int i; 
	unsigned char* b = (unsigned char*) buf; 
	for (i=0; i<sz; i++) { 
		printf("%02X ", *(b+i));
	}
}

/* write_pci_config */
int write_pci(int configfd, void *data, off_t offset, unsigned char sz){
		int err; 
		err = pwrite(configfd, data, sz, offset);
		if (err < 0 ) printf("write_pci: Error sending %d bytes at offset %04X\n", offset); 
		if (0) {
			printf("write_pci: write %02d bytes at offset 0x%02X  >> ", sz, offset); 
			dump_hex(data, sz); printf("\n"); 
		}
		return err; 
}

/* read_pci_config */
int read_pci(int configfd, void *data, off_t offset, unsigned char sz){
		int err; 
		err = pread(configfd, data, sz, offset);
		if (err < 0 ) printf("read_pci: Error reading %d bytes at offset %04X\n", offset); 
		if (0) {
			printf("read_pci : read  %02d bytes at offset 0x%02X  >> ", sz, offset); 
			dump_hex(data, sz); printf("\n"); 
		}
		return err; 
}

void dump_pci(struct pcidev *pdev)
{
	printf("** Vendor:%04X Device:%04X\n", pdev->vendor_id, pdev->device_id ); 
	printf("** Command:%04X Status:%04X\n", pdev->command, pdev->status ); 
	printf("** Cache line size:%02X latency_timer:%02X header_type:%02X \n", pdev->cache_line_size, pdev->latency_timer, pdev->header_type ); 
	printf("** addr0:%08X addr1:%08X addr2:%08X\n", pdev->addr0, pdev->addr1, pdev->addr2 ); 
	printf("** addr3:%08X addr4:%08X addr5:%08X\n", pdev->addr3, pdev->addr4, pdev->addr5 ); 
	printf("** addr_rom:%08X\n", pdev->rom); 
	printf("** capability_list_offset:%01X\n", pdev->cap_offset_first); 
	printf("** capability_val1:%02X\n", pdev->cap_val1); 
	printf("** capability_unk:%02X\n", pdev->cap_unk); 
	printf("** unk_8e:%02X\n", pdev->unk8e); 
}

void read_pci(int configfd, struct pcidev *pdev)
{

	// Get Vendor/device/command/status
	read_pci(configfd, &pdev->vendor_id, 0x00, 2); 
	read_pci(configfd, &pdev->device_id, 0x02, 2); 
	read_pci(configfd, &pdev->command, 0x04, 2); 
	read_pci(configfd, &pdev->status, 0x06, 2); 
	read_pci(configfd, &pdev->cache_line_size, 0x0c, 1); 
	read_pci(configfd, &pdev->latency_timer, 0x0d, 1); 
	read_pci(configfd, &pdev->header_type, 0x0d, 1); 
	read_pci(configfd, &pdev->addr0, 0x10, 4);
	read_pci(configfd, &pdev->addr1, 0x14, 4);
	read_pci(configfd, &pdev->addr2, 0x18, 4);
	read_pci(configfd, &pdev->addr3, 0x1C, 4);
	read_pci(configfd, &pdev->addr4, 0x20, 4);
	read_pci(configfd, &pdev->addr5, 0x24, 4);
	read_pci(configfd, &pdev->cardbus_cis, 0x28, 4);
	read_pci(configfd, &pdev->rom, 0x30, 4);
	read_pci(configfd, &pdev->cap_offset_first, 0x34, 1);
	read_pci(configfd, &pdev->cap_val1, pdev->cap_offset_first, 2);
	read_pci(configfd, &pdev->cap_unk, 0x54, 2);
	read_pci(configfd, &pdev->unk8e, 0x8e, 2);
}


int main()
{

	int uiofd; 
	int configfd;
	int err; 
	int i; 
	unsigned icount; 
	pcidev pdev; 


	/* Opening UIO interfaces for the device */ 
	printf("Opening /dev/uio0\n"); 
	uiofd = open("/dev/uio0", O_RDONLY); 
	if (uiofd < 0) {
		perror("uio open /dev/uio0:"); 
		return errno; 
	}
	printf("Opening /sys/class/uio/uio0/device/config\n"); 
	configfd = open("/sys/class/uio/uio0/device/config", O_RDWR); 
	if (configfd < 0 ) {
		perror("uio config open /sys/class/uio/uio0/device/config:"); 
		return errno; 
	}
	printf("Opening /sys/class/uio/uio0/device/resource0\n"); 
	int resfd = open("/sys/class/uio/uio0/device/resource0", O_RDWR); 
	if (resfd < 0) {
		perror("uio open /sys/class/uio/uio0/device/resource0:"); 
		return errno; 
	}
	/* mmap the ressource0 */
	uint32_t *data = (uint32_t*) mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, resfd, 0); 
	if (data == ((uint32_t *) -1)){
		printf("Can't mmap the device\n"); 
		perror("mmap failed:");
		return(1); 
	}
	printf("Memory mapped to 0x%lx\n",(unsigned long) data);




	// read pci and display 
	read_pci(configfd, &pdev);
	dump_pci(&pdev); 


	// Let's init
/*
	pdev.cap_unk=0x08;
	write_pci(configfd, &pdev.cap_unk, 0x54,2);

	pdev.command=0x400; write_pci(configfd, &pdev.command, 0x04,2);

	write_pci(configfd, &pdev.addr0, 0x10, 4);
	write_pci(configfd, &pdev.addr1, 0x14, 4);
	write_pci(configfd, &pdev.addr2, 0x18, 4);
	write_pci(configfd, &pdev.addr3, 0x1C, 4);
	write_pci(configfd, &pdev.addr4, 0x20, 4);
	write_pci(configfd, &pdev.addr5, 0x24, 4);
	write_pci(configfd, &pdev.rom, 0x30, 4);

	pdev.intline=0x05; // DO nothing now 
	write_pci(configfd, &pdev.intline, 0x3e, 1);
	pdev.command=0x400; write_pci(configfd, &pdev.command, 0x04,2);
	pdev.command=0x406; write_pci(configfd, &pdev.command, 0x04,2);
	pdev.status=0xf900; write_pci(configfd, &pdev.status, 0x06,2);
	pdev.command=0x406; write_pci(configfd, &pdev.command, 0x04,2);
	pdev.unk8e=0x80; write_pci(configfd, &pdev.unk8e, 0x8e,2);

*/
	
//	pdev.command=0x400; 	write_pci(configfd, &pdev.command, 0x04,2);
//	pdev.cache_line_size=0; write_pci(configfd, &pdev.cache_line_size, 0x0c, 1);
//	pdev.latency_timer=0;   write_pci(configfd, &pdev.latency_timer, 0x0d, 1);
//	pdev.intline=0x05;  	write_pci(configfd, &pdev.intline, 0x3e, 1);
//	pdev.command=0x400; 	write_pci(configfd, &pdev.command, 0x04,2);
//	pdev.command=0x406; 	write_pci(configfd, &pdev.command, 0x04,2);
//	pdev.status=0xf900; 	write_pci(configfd, &pdev.status, 0x06,2);
//	pdev.command=0x406; 	write_pci(configfd, &pdev.command, 0x04,2);



	read_pci(configfd, &pdev);
	dump_pci(&pdev); 

	int pold=0;

	for(i=0;; i++){

	//	data[1]=0x8000000a; 
	//	data[0x200]=0x0; 
	//	data[1]=0x8000000c; 

	//	printf("region0+0x0  : 0x%08X \n", data[0]);  //(+0x4)
	//	printf("region0+0x4  : 0x%08X \n", data[1]);  //(+0x4)
	//	printf("region0+0x8  : 0x%08X \n", data[2]);  //(+0x4)
	//	printf("region0+0xc  : 0x%08X \n", data[3]);  //(+0xc)
	//	printf("region0+0x800  : 0x%08X \n", data[0x200]);  //(+0x800)

		pdev.command = 0x06; write_pci(configfd, &pdev.command, 0x4, 2); // Enable ints
		sleep(2); 
		err = read(uiofd, &icount, 4); 
		printf("GOT INT %d \n", icount);
		if (err != 4) {
			perror("uio read error!"); 
			return(1); 
		}
		pold=icount; 




	}
}





