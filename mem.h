#ifndef HW_H
#define HW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct memblock {
		unsigned mem_ref;
		unsigned bus_addr;
		uint8_t *virt_addr;
		unsigned num_pages;
};

struct hwinfo {
	uint32_t mem_flag;
	uint32_t periph_virt_base;
	uint32_t periph_phys_base;
};

uint32_t mem_virt_to_phys(void *virt, struct memblock* m);
int mem_open();
void mem_close(int mem_fd);
uint32_t* map_peripheral(int mem_fd, uint32_t base, uint32_t len);
void unmap_peripheral(void* addr, uint32_t len);
struct hwinfo get_model(unsigned mbox_board_rev);
struct memblock init_mem(int mbox_fd, int mem_fd, int num_pages, struct hwinfo* hwinfo);
void release_mem(int mbox_fd, struct memblock* m);

#ifdef __cplusplus
}
#endif

#endif // HW_H
