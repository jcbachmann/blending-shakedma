#ifndef REG_H
#define REG_H

#define CYCLE_TIME_US	600
#define SAMPLE_US		100
#define NUM_SAMPLES		((1<<23)+(1<<21))
#define NUM_CBS			1

#define NUM_PAGES		((NUM_CBS * sizeof(dma_cb_t) + NUM_SAMPLES * 4 + \
	PAGE_SIZE - 1) >> PAGE_SHIFT)

#define DMA_BASE(x)		((x).periph_virt_base + 0x00007000)
#define DMA_CHAN_SIZE	0x100 /* size of register space for a single DMA channel */
#define DMA_CHAN_MAX	14  /* number of DMA Channels we have... actually,  are 15there... but channel fifteen is mapped at a different DMA_BASE, so we leave that one alone */
#define DMA_CHAN_NUM	6  /* the DMA Channel we are using, NOTE: DMA Ch 0 seems to be used by X... better not use it ;) */
#define PWM_BASE_OFFSET 0x0020C000
#define PWM_BASE(x)		((x).periph_virt_base + PWM_BASE_OFFSET)
#define PWM_PHYS_BASE(x)	((x).periph_phys_base + PWM_BASE_OFFSET)
#define PWM_LEN			0x28
#define CLK_BASE_OFFSET 0x00101000
#define CLK_BASE(x)		((x).periph_virt_base + CLK_BASE_OFFSET)
#define CLK_LEN			0xA8
#define GPIO_BASE_OFFSET 0x00200000
#define GPIO_BASE(x)	((x).periph_virt_base + GPIO_BASE_OFFSET)
#define GPIO_PHYS_BASE(x)	((x).periph_phys_base + GPIO_BASE_OFFSET)
#define GPIO_LEN		0x100
#define PCM_BASE_OFFSET 0x00203000
#define PCM_BASE(x)		((x).periph_virt_base + PCM_BASE_OFFSET)
#define PCM_PHYS_BASE(x)	((x).periph_phys_base + PCM_BASE_OFFSET)
#define PCM_LEN			0x24

#define DMA_NO_WIDE_BURSTS	(1<<26)
#define DMA_WAIT_RESP		(1<<3)
#define DMA_D_DREQ		(1<<6)
#define DMA_PER_MAP(x)		((x)<<16)
#define DMA_WAITS(x)		((x&0x1F)<<21)
#define DMA_END			(1<<1)
#define DMA_RESET		(1<<31)
#define DMA_INT			(1<<2)
#define DMA_SRC_INC			(1<<8)
#define DMA_DEST_INC			(1<<4)
#define DMA_DEST_WIDTH		(1<<5)
#define DMA_BURST_LENGTH(x)	(((x)&0xF)<<12)

#define DMA_CS			(0x00/4)
#define DMA_CONBLK_AD		(0x04/4)
#define DMA_DEBUG		(0x20/4)

#define GPIO_FSEL0		(0x00/4)
#define GPIO_SET0		(0x1c/4)
#define GPIO_CLR0		(0x28/4)
#define GPIO_LEV0		(0x34/4)
#define GPIO_PULLEN		(0x94/4)
#define GPIO_PULLCLK		(0x98/4)

#define GPIO_MODE_IN		0
#define GPIO_MODE_OUT		1

#define PWM_CTL			(0x00/4)
#define PWM_DMAC		(0x08/4)
#define PWM_RNG1		(0x10/4)
#define PWM_FIFO		(0x18/4)

#define PWMCLK_CNTL		40
#define PWMCLK_DIV		41

#define PWMCTL_MODE1		(1<<1)
#define PWMCTL_PWEN1		(1<<0)
#define PWMCTL_CLRF		(1<<6)
#define PWMCTL_USEF1		(1<<5)

#define PWMDMAC_ENAB		(1<<31)
#define PWMDMAC_THRSHLD		((4<<8)|(4<<0))

#define PCM_CS_A		(0x00/4)
#define PCM_FIFO_A		(0x04/4)
#define PCM_MODE_A		(0x08/4)
#define PCM_RXC_A		(0x0c/4)
#define PCM_TXC_A		(0x10/4)
#define PCM_DREQ_A		(0x14/4)
#define PCM_INTEN_A		(0x18/4)
#define PCM_INT_STC_A		(0x1c/4)
#define PCM_GRAY		(0x20/4)

#define PCMCLK_CNTL		38
#define PCMCLK_DIV		39

typedef struct {
uint32_t info, src, dst, length,
stride, next, pad[2];
} dma_cb_t;

#endif // REG_H
