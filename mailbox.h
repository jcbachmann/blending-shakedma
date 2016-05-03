#ifndef MAILBOX_H
#define MAILBOX_H

#ifdef __cplusplus
extern "C" {
#endif

int mbox_open();
void mbox_close(int file_desc);
unsigned mem_alloc(int file_desc, unsigned size, unsigned align, unsigned flags);
unsigned mem_free(int file_desc, unsigned handle);
unsigned mem_lock(int file_desc, unsigned handle);
unsigned mem_unlock(int file_desc, unsigned handle);
unsigned get_firmware_revision(int file_desc);
unsigned get_board_model(int file_desc);
unsigned get_board_revision(int file_desc);
unsigned get_dma_channels(int file_desc);

#ifdef __cplusplus
}
#endif

#endif // MAILBOX_H
