#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define BCM2708_PERI_BASE        0x20000000
#define GPIO_BASE                (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define SMI_BASE                (BCM2708_PERI_BASE + 0x600000) /* GPIO controller */

#define SMICS 0x00/4
#define SMIA	0x08/4
#define SMID	0x0C/4
#define SMIDSW0	0x14/4
#define SMIDSW1 0x1C/4
#define SMI_DCS 0x34/4

void *smi_map,*gpio_map;
volatile unsigned *smi,*gpio;

#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))



void init() {
	int mem_fd;
   /* open /dev/mem */
   if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
      printf("can't open /dev/mem \n");
      exit(-1);
   }
   smi_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      SMI_BASE         //Offset to SMI peripheral
   );
	printf("mapped %d decimal\n",SMI_BASE);
   /* mmap GPIO */
   gpio_map = mmap(
      NULL,             //Any adddress in our space will do
      BLOCK_SIZE,       //Map length
      PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
      MAP_SHARED,       //Shared with other processes
      mem_fd,           //File to map
      GPIO_BASE         //Offset to GPIO peripheral
   );
   close(mem_fd); //No need to keep mem_fd open after mmap
   if (smi_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)smi_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   smi = (volatile unsigned *)smi_map;
	gpio = (volatile unsigned *)gpio_map;
}

void main(int argc, char **argv) {
	init();

	char test[5];
	volatile uint8_t *bytemap = smi;
	test[3] = bytemap[0x44];
	test[2] = bytemap[0x45];
	test[1] = bytemap[0x46];
	test[0] = bytemap[0x47];
	test[4] = 0;
	printf("header is %s\n",test);
	
	smi[SMICS] &= ~1;
	smi[SMI_DCS] &= ~1;
	
	sleep(1);
	
	smi[SMICS] |= 1;
	smi[SMI_DCS] |= 1;

	smi[SMIDSW0] = 0x3f3f7f7f;
	smi[SMIDSW1] = 0x3f3f7f7f;

	int i;
	for (i=0; i<1024; i++) {
		smi[SMIA] = i%64;
		smi[SMID] = i;
	}
	int dump = open("dump.bin",O_CREAT|O_TRUNC|O_WRONLY);
	write(dump,bytemap,0x60);
	close(dump);
}
