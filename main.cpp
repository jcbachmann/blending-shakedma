#include <iostream>
#include <string>
#include "mem.h"
#include "mailbox.h"
#include "reg.h"
#include "LineInputHandler.hpp"
#include "sched.h"

static uint32_t gpio_get_mode(uint32_t* gpio, uint32_t pin)
{
	return ((gpio[GPIO_FSEL0 + pin/10] >> ((pin % 10) * 3))) & 7;
}

static void gpio_set_mode(uint32_t* gpio, uint32_t pin, uint32_t mode)
{
	uint32_t fsel = gpio[GPIO_FSEL0 + pin/10];

	fsel &= ~(7 << ((pin % 10) * 3));
	fsel |= mode << ((pin % 10) * 3);
	gpio[GPIO_FSEL0 + pin/10] = fsel;
}

static void gpio_set(uint32_t* gpio, int pin, int level)
{
	gpio[level ? GPIO_SET0 : GPIO_CLR0] = 1 << pin;
}

std::string lockpath(char* arg)
{
	std::string lockpath("/var/lock/");
	char* arg2 = strdup(arg);
	lockpath += basename(arg2);
	free(arg2);
	return lockpath;
}

void ensureSingle(const char* path)
{
	int lockfile = open(path, O_CREAT | O_EXCL | O_RDWR);
	if (lockfile < 0) {
		fprintf(stderr, "Another instance is running!\n");
		exit(1);
	}
}

void ensureSingleEnd(const char* path)
{
	unlink(path);
}

static void udelay(int us)
{
	struct timespec ts = { 0, us * 1000 };

	nanosleep(&ts, NULL);
}

void addClear(dma_cb_t*& ccb, uint32_t mask, struct memblock* memblock, struct hwinfo* hwinfo)
{
	ccb->src = mem_virt_to_phys(&ccb->pad[0], memblock);
	ccb->dst = GPIO_PHYS_BASE(*hwinfo) + 4*GPIO_CLR0;
	ccb->info = 0;
	ccb->length = 4;
	ccb->stride = 0;
	ccb->next = mem_virt_to_phys(ccb+1, memblock);
	ccb->pad[0] = mask;
	ccb->pad[1] = 0;
	ccb++;
}

void addSet(dma_cb_t*& ccb, uint32_t mask, struct memblock* memblock, struct hwinfo* hwinfo)
{
	ccb->src = mem_virt_to_phys(&ccb->pad[0], memblock);
	ccb->dst = GPIO_PHYS_BASE(*hwinfo) + 4*GPIO_SET0;
	ccb->info = 0;
	ccb->length = 4;
	ccb->stride = 0;
	ccb->next = mem_virt_to_phys(ccb+1, memblock);
	ccb->pad[0] = mask;
	ccb->pad[1] = 0;
	ccb++;
}

void addDelay(dma_cb_t*& ccb, int us, struct memblock* memblock, struct hwinfo* hwinfo)
{
	ccb->src = mem_virt_to_phys(&ccb->pad[0], memblock);
	ccb->dst = PWM_PHYS_BASE(*hwinfo) + 4*PWM_RNG1;
	ccb->info = 0;
	ccb->length = 4;
	ccb->stride = 0;
	ccb->next = mem_virt_to_phys(ccb+1, memblock);
	ccb->pad[0] = us;
	ccb->pad[1] = 0;
	ccb++;
	ccb->src = mem_virt_to_phys(&ccb->pad[0], memblock);
	ccb->dst = PWM_PHYS_BASE(*hwinfo) + 4*PWM_FIFO;
	ccb->info = DMA_D_DREQ | DMA_PER_MAP(5);
	ccb->length = 4;
	ccb->stride = 0;
	ccb->next = mem_virt_to_phys(ccb+1, memblock);
	ccb->pad[0] = 0;
	ccb->pad[1] = 0;
	ccb++;
}

int main(int argc, char** argv)
{
	const int pinEnab = 18;
	const int pinPos = 27;
	const int pinNeg = 22;

	bool running = false;

	std::string lock = lockpath(argv[0]);
	ensureSingle(lock.c_str());
	LineInputHandler lih;

	int mbox_fd = mbox_open();
	unsigned board_rev = get_board_revision(mbox_fd);
	struct hwinfo hwinfo = get_model(board_rev);

	int mem_fd = mem_open();
	struct memblock memblock = init_mem(mbox_fd, mem_fd, 1, &hwinfo);

	uint32_t* gpio = map_peripheral(mem_fd, GPIO_BASE(hwinfo), GPIO_LEN);
	uint32_t* clk = map_peripheral(mem_fd, CLK_BASE(hwinfo), CLK_LEN);
	uint32_t* pwm = map_peripheral(mem_fd, PWM_BASE(hwinfo), PWM_LEN);
	uint32_t* dma_all = map_peripheral(mem_fd, DMA_BASE(hwinfo), (DMA_CHAN_MAX+1) * DMA_CHAN_SIZE);
	uint32_t* dma = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(dma_all) + DMA_CHAN_NUM * DMA_CHAN_SIZE);

	dma_cb_t* cb = reinterpret_cast<dma_cb_t*>(memblock.virt_addr);
	dma_cb_t* ccb = cb;
	uint32_t* lowdur1;
	uint32_t* lowdur2;
	uint32_t* highdur1;
	uint32_t* highdur2;
	// addDelay value is for following block

	highdur1 = &ccb->pad[0];
	addDelay(ccb, 1000, &memblock, &hwinfo);
	addSet(ccb, 1<<pinPos, &memblock, &hwinfo);
	addSet(ccb, 1<<pinEnab, &memblock, &hwinfo);

	lowdur1 = &ccb->pad[0];
	addDelay(ccb, 1000, &memblock, &hwinfo);
	addClear(ccb, 1<<pinEnab | 1<<pinPos, &memblock, &hwinfo);

	highdur2 = &ccb->pad[0];
	addDelay(ccb, 1000, &memblock, &hwinfo);
	addSet(ccb, 1<<pinNeg, &memblock, &hwinfo);
	addSet(ccb, 1<<pinEnab, &memblock, &hwinfo);

	lowdur2 = &ccb->pad[0];
	addDelay(ccb, 1000, &memblock, &hwinfo);
	addClear(ccb, 1<<pinEnab | 1<<pinNeg, &memblock, &hwinfo);

	ccb--;
	ccb->next = mem_virt_to_phys(&cb[0], &memblock);

	gpio_set_mode(gpio, pinEnab, GPIO_MODE_OUT);
	gpio_set_mode(gpio, pinPos, GPIO_MODE_OUT);
	gpio_set_mode(gpio, pinNeg, GPIO_MODE_OUT);

	std::string line;
	while (lih >> line) {
		std::istringstream is(line);
		uint32_t lowdur, highdur;
		if (is >> lowdur >> highdur) {
			if (lowdur > 9 && highdur > 9 && lowdur + highdur < 100000) {
				*lowdur1 = lowdur;
				*highdur1 = highdur;
				*lowdur2 = lowdur;
				*highdur2 = highdur;
				if (!running) {
					pwm[PWM_CTL] = 0;
					clk[PWMCLK_CNTL] = 0x5A000000 | (PWMCLK_CNTL & ~(1 << 4));
					while (clk[PWMCLK_CNTL] & (1 << 7));
					clk[PWMCLK_DIV] = 0x5A000000 | (500<<12); // Clockdiv: 500
					clk[PWMCLK_CNTL] = 0x5A000006; // PLLD 500 MHz
					clk[PWMCLK_CNTL] = 0x5A000016;
					while (!(clk[PWMCLK_CNTL] & (1 << 7)));

					dma[DMA_CS] = DMA_RESET;
					udelay(10);
					dma[DMA_CS] = DMA_INT | DMA_END;
					dma[DMA_CONBLK_AD] = mem_virt_to_phys(cb, &memblock);
					dma[DMA_DEBUG] = 7;
					dma[DMA_CS] = 0x10880001;

					pwm[PWM_CTL] = (1<<0) | (1<<1) | (1<<2) | (1<<5) | (1<<6);
					pwm[PWM_DMAC] = (1<<31) | (7<<0) | (7<<8);
					pwm[PWM_RNG1] = (1<<25);

					running = true;
				}
				std::cout << line << std::endl;
			} else if (highdur == 0) {
				ccb->next = 0;
				while (dma[DMA_CS] & 0x00000001) {
					sched_yield();
				}
				dma[DMA_CS] = DMA_RESET;
				udelay(10);
				running = false;
				ccb->next = mem_virt_to_phys(&cb[0], &memblock);
				std::cout << line << std::endl;
			}
		}
	}

	ccb->next = 0;
	while (dma[DMA_CS] & 0x00000001) {
		sched_yield();
	}
	dma[DMA_CS] = DMA_RESET;
	udelay(10);

	release_mem(mbox_fd, &memblock);
	mem_close(mem_fd);
	mbox_close(mbox_fd);

	unlink(lock.c_str());
}
