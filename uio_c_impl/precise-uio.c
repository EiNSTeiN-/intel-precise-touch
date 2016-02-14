
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <errno.h>

#define MAP_SIZE 4000

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
		if (1) printf("write_pci: write %02d bytes at offset 0x%02X  >> ", sz, offset); 
		dump_hex(data, sz); printf("\n"); 
		return err; 
}

/* read_pci_config */
int read_pci(int configfd, void *data, off_t offset, unsigned char sz){
		int err; 
		err = pread(configfd, data, sz, offset);
		if (err < 0 ) printf("read_pci: Error reading %d bytes at offset %04X\n", offset); 
		if (1) printf("read_pci : read  %02d bytes at offset 0x%02X  >> ", sz, offset); 
		dump_hex(data, sz); printf("\n"); 
		return err; 
}


int main()
{

	int uiofd; 
	int configfd;
	int err; 
	int i; 
	unsigned icount; 
	unsigned char command_high; 
	unsigned char command_low; 


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





	// Get Vendor/device, for fun 
	uint16_t vendor,device;
	read_pci(configfd, &vendor, 0x00, 2); 
	read_pci(configfd, &device, 0x02, 2); 
	printf("Vendor:%04X Device:%04X\n", vendor, device ); 

	// Command & status 
	uint16_t command, status; 
	read_pci(configfd, &command, 0x04, 2);
	read_pci(configfd, &status,  0x06, 2);
	printf("Command:%04X Status:%04X\n", command, status ); 



uint16_t tmp16; 
uint32_t tmp32; 

//initialisation ? 
read_pci(configfd, &tmp32, 0x50,4);
read_pci(configfd, &tmp32, 0x54,4);
tmp16=0x8;
write_pci(configfd, &tmp16, 0x54,2);
read_pci(configfd, &tmp16, 0x54,2);



	// Int line 
	uint8_t int_line=0xa; 
	write_pci(configfd, &int_line, 0x3e, 1);


	// Unknown init
	uint16_t unk1=0x08; 
	write_pci(configfd, &unk1, 0x54, 2);

	// BASE_ADDR
	uint32_t addr=0; 
	read_pci(configfd, &addr, 0x10, 4);
	write_pci(configfd, &addr, 0x10, 4);
	addr=0;
	write_pci(configfd, &addr, 0x14, 4);
	write_pci(configfd, &addr, 0x18, 4);
	write_pci(configfd, &addr, 0x1c, 4);
	write_pci(configfd, &addr, 0x20, 4);
	write_pci(configfd, &addr, 0x24, 4);
	write_pci(configfd, &addr, 0x28, 4);
	write_pci(configfd, &addr, 0x30, 4);
	write_pci(configfd, &int_line, 0x3c, 1); // Send again intline
	
        // PCI Cache Size and Latency
	uint8_t tmp8=0;
	write_pci(configfd, &tmp8, 0xc, 1);
	write_pci(configfd, &tmp8, 0xd, 1);

	// Commands 
	command=0x400; // Fixme => Just correct the BUS MASTER + MEM IO bits) 
	write_pci(configfd, &command, 0x4, 2);
	read_pci(configfd, &command, 0x4, 2);
	command=0x406; // Fixme => Just correct the BUS MASTER + MEM IO bits) 
	write_pci(configfd, &command, 0x4, 2);


	// Status
	status=0xf900;
	write_pci(configfd, &status, 0x6, 2);

	read_pci(configfd, &command, 0x4, 2);
	write_pci(configfd, &command, 0x4, 2);
	uint16_t unk8e; 
	read_pci(configfd, &unk8e, 0x8e, 2);
	write_pci(configfd, &unk8e, 0x8e, 2);

	read_pci(configfd, &command, 0x4, 2);








	for(i=0;; i++){

		command=0x6; 
		write_pci(configfd, &command, 0x4, 2);
		/* debug */

		/* wait for int */
		//printf("Wait for int\n");
		err = read(uiofd, &icount, 4); 
		if (err != 4) {
			perror("uio read:"); 
		}
		printf("region0 + 0x4  : 0x%08X \n", data[1]);  //(+0x4)
		//printf("region0 + 0x8  : 0x%08X \n", data[2]);  //(+0x4)
		printf("region0 + 0xc  : 0x%08X \n", data[3]);  //(+0xc)
		printf("region0 + 0x800: 0x%08X \n", data[200]); //(+0x800) 

	}
}
