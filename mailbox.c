#include "mailbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEVFILE_MBOX "/dev/vcio"

#define MAJOR_NUM 100
#define IOCTL_MBOX_PROPERTY _IOWR(MAJOR_NUM, 0, char *)

int mbox_open()
{
	int file_desc = open(DEVFILE_MBOX, 0);

	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVFILE_MBOX);
		abort();
	}

	return file_desc;
}

void mbox_close(int file_desc)
{
	close(file_desc);
}

static int mbox_property(int file_desc, unsigned* buf)
{
   int ret_val = ioctl(file_desc, IOCTL_MBOX_PROPERTY, buf);

   if (ret_val < 0) {
	  printf("MBOX_PROPERTY ioctl failed: %d\n", ret_val);
   }

   if (buf[1] != 0x80000000) {
	   printf("MBOX_PROPERTY Error");
   }

   return ret_val;
}

unsigned mem_alloc(int file_desc, unsigned size, unsigned align, unsigned flags)
{
	unsigned p[] = {4 * 9, 0, 0x3000c, 12, 12, size, align, flags, 0};
	mbox_property(file_desc, p);
	return p[5];
}

unsigned mem_free(int file_desc, unsigned handle)
{
   unsigned p[] = {4 * 7, 0, 0x3000f, 4, 4, handle, 0};
   mbox_property(file_desc, p);
   return p[5];
}

unsigned mem_lock(int file_desc, unsigned handle)
{
   unsigned p[] = {4 * 7, 0, 0x3000d, 4, 4, handle, 0};
   mbox_property(file_desc, p);
   return p[5];
}

unsigned mem_unlock(int file_desc, unsigned handle)
{
   unsigned p[] = {4 * 7, 0, 0x3000e, 4, 4, handle, 0};
   mbox_property(file_desc, p);
   return p[5];
}

unsigned get_firmware_revision(int file_desc)
{
   unsigned p[] = {4 * 7, 0, 0x10000, 4, 0, 0, 0};
   mbox_property(file_desc, p);
   return p[5];
}

unsigned get_board_model(int file_desc)
{
	unsigned p[] = {4 * 7, 0, 0x10001, 4, 0, 0, 0};
	mbox_property(file_desc, p);
	return p[5];
}

unsigned get_board_revision(int file_desc)
{
	unsigned p[] = {4 * 7, 0, 0x10002, 4, 0, 0, 0};
	mbox_property(file_desc, p);
	return p[5];
}

unsigned get_dma_channels(int file_desc)
{
	unsigned p[] = {4 * 7, 0, 0x60001, 4, 0, 0, 0};
	mbox_property(file_desc, p);
	return p[5];
}
