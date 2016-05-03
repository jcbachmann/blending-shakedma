#include "mem.h"

#include "mailbox.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#define MEM_FLAG_DISCARDABLE (1 << 0) /* can be resized to 0 at any time. Use for cached data */
#define MEM_FLAG_NORMAL (0 << 2) /* normal allocating alias. Don't use from ARM */
#define MEM_FLAG_DIRECT (1 << 2) /* 0xC alias uncached */
#define MEM_FLAG_COHERENT (2 << 2) /* 0x8 alias. Non-allocating in L2 but coherent */
#define MEM_FLAG_L1_NONALLOCATING (MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) /* Allocating in L2 */
#define MEM_FLAG_ZERO (1 << 4)  /* initialise buffer to all zeros */
#define MEM_FLAG_NO_INIT (1 << 5) /* don't initialise (default is initialise to all ones */
#define MEM_FLAG_HINT_PERMALOCK (1 << 6) /* Likely to be locked for long periods of time. */

/* New Board Revision format:
SRRR MMMM PPPP TTTT TTTT VVVV

S scheme (0=old, 1=new)
R RAM (0=256, 1=512, 2=1024)
M manufacturer (0='SONY',1='EGOMAN',2='EMBEST',3='UNKNOWN',4='EMBEST')
P processor (0=2835, 1=2836)
T type (0='A', 1='B', 2='A+', 3='B+', 4='Pi 2 B', 5='Alpha', 6='Compute Module')
V revision (0-15)
*/
#define BOARD_REVISION_SCHEME_MASK (0x1 << 23)
#define BOARD_REVISION_SCHEME_OLD (0x0 << 23)
#define BOARD_REVISION_SCHEME_NEW (0x1 << 23)
#define BOARD_REVISION_RAM_MASK (0x7 << 20)
#define BOARD_REVISION_MANUFACTURER_MASK (0xF << 16)
#define BOARD_REVISION_MANUFACTURER_SONY (0 << 16)
#define BOARD_REVISION_MANUFACTURER_EGOMAN (1 << 16)
#define BOARD_REVISION_MANUFACTURER_EMBEST (2 << 16)
#define BOARD_REVISION_MANUFACTURER_UNKNOWN (3 << 16)
#define BOARD_REVISION_MANUFACTURER_EMBEST2 (4 << 16)
#define BOARD_REVISION_PROCESSOR_MASK (0xF << 12)
#define BOARD_REVISION_PROCESSOR_2835 (0 << 12)
#define BOARD_REVISION_PROCESSOR_2836 (1 << 12)
#define BOARD_REVISION_TYPE_MASK (0xFF << 4)
#define BOARD_REVISION_TYPE_PI1_A (0 << 4)
#define BOARD_REVISION_TYPE_PI1_B (1 << 4)
#define BOARD_REVISION_TYPE_PI1_A_PLUS (2 << 4)
#define BOARD_REVISION_TYPE_PI1_B_PLUS (3 << 4)
#define BOARD_REVISION_TYPE_PI2_B (4 << 4)
#define BOARD_REVISION_TYPE_ALPHA (5 << 4)
#define BOARD_REVISION_TYPE_CM (6 << 4)
#define BOARD_REVISION_REV_MASK (0xF)

#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1<<PAGE_SHIFT)

uint32_t mem_virt_to_phys(void *virt, struct memblock* m)
{
	uint32_t offset = (uint8_t *)virt - m->virt_addr;

	return m->bus_addr + offset;
}

int mem_open()
{
	int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);

	if (mem_fd < 0) {
		printf("Failed to open /dev/mem: %m\n");
		abort();
	}

	return mem_fd;
}

void mem_close(int mem_fd)
{
	close(mem_fd);
}

uint32_t* map_peripheral(int mem_fd, uint32_t base, uint32_t len)
{
	void* vaddr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, base);

	if (vaddr == MAP_FAILED) {
		printf("Failed to map peripheral at 0x%08x: %m\n", base);
		abort();
	}

	return vaddr;
}

void unmap_peripheral(void* addr, uint32_t len)
{
	if (munmap(addr, len)) {
		printf("Failed to unmap peripheral\n");
		abort();
	}
}

/*
 * determine which pi model we are running on
 */
struct hwinfo get_model(unsigned mbox_board_rev)
{
	int board_model = 0;

	if ((mbox_board_rev & BOARD_REVISION_SCHEME_MASK) == BOARD_REVISION_SCHEME_NEW) {
		if ((mbox_board_rev & BOARD_REVISION_TYPE_MASK) == BOARD_REVISION_TYPE_PI2_B) {
			board_model = 2;
		} else {
			// no Pi 2, we assume a Pi 1
			board_model = 1;
		}
	} else {
		// if revision scheme is old, we assume a Pi 1
		board_model = 1;
	}

	struct hwinfo i;

	switch(board_model) {
		case 1:
			i.periph_virt_base = 0x20000000;
			i.periph_phys_base = 0x7e000000;
			i.mem_flag         = MEM_FLAG_DIRECT | MEM_FLAG_ZERO;
			break;
		case 2:
			i.periph_virt_base = 0x3f000000;
			i.periph_phys_base = 0x7e000000;
			i.mem_flag         = MEM_FLAG_DIRECT | MEM_FLAG_ZERO;
			break;
		default:
			printf("Unable to detect Board Model from board revision: %#x", mbox_board_rev);
			abort();
			break;
	}

	return i;
}

struct memblock init_mem(int mbox_fd, int mem_fd, int num_pages, struct hwinfo* hwinfo)
{
	struct memblock m;
	m.num_pages = num_pages;
	/* Use the mailbox interface to the VC to ask for physical memory */
	m.mem_ref = mem_alloc(mbox_fd, num_pages << PAGE_SHIFT, PAGE_SIZE, hwinfo->mem_flag);
	/* TODO: How do we know that succeeded? */
	printf("mem_ref %u\n", m.mem_ref);
	m.bus_addr = mem_lock(mbox_fd, m.mem_ref);
	printf("bus_addr = %#x\n", m.bus_addr);
	m.virt_addr = (uint8_t*)map_peripheral(mem_fd, BUS_TO_PHYS(m.bus_addr), num_pages << PAGE_SHIFT);
	printf("virt_addr %p\n", m.virt_addr);
	fflush(stdout);
	return m;
}

void release_mem(int mbox_fd, struct memblock* m)
{
	unmap_peripheral(m->virt_addr, m->num_pages << PAGE_SHIFT);
	mem_unlock(mbox_fd, m->mem_ref);
	mem_free(mbox_fd, m->mem_ref);
}
