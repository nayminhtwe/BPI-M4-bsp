/*
 *  linux/include/linux/mmc/host.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Host driver specific definitions.
 */
#ifndef LINUX_MMC_HOST_H
#define LINUX_MMC_HOST_H

#include <linux/leds.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/fault-inject.h>
#include <linux/blkdev.h>

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/pm.h>

struct mmc_ios {
	unsigned int	clock;			/* clock rate */
	unsigned short	vdd;

/* vdd stores the bit number of the selected voltage range from below. */

	unsigned char	bus_mode;		/* command output mode */

#define MMC_BUSMODE_OPENDRAIN	1
#define MMC_BUSMODE_PUSHPULL	2

	unsigned char	chip_select;		/* SPI chip select */

#define MMC_CS_DONTCARE		0
#define MMC_CS_HIGH		1
#define MMC_CS_LOW		2

	unsigned char	power_mode;		/* power supply mode */

#define MMC_POWER_OFF		0
#define MMC_POWER_UP		1
#define MMC_POWER_ON		2
#define MMC_POWER_UNDEFINED	3

	unsigned char	bus_width;		/* data bus width */

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

	unsigned char	timing;			/* timing specification used */

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	3
#define MMC_TIMING_UHS_SDR25	4
#define MMC_TIMING_UHS_SDR50	5
#define MMC_TIMING_UHS_SDR104	6
#define MMC_TIMING_UHS_DDR50	7
#define MMC_TIMING_MMC_DDR52	8
#define MMC_TIMING_MMC_HS200	9
#define MMC_TIMING_MMC_HS400	10

	unsigned char	signal_voltage;		/* signalling voltage (1.8V or 3.3V) */

#define MMC_SIGNAL_VOLTAGE_330	0
#define MMC_SIGNAL_VOLTAGE_180	1
#define MMC_SIGNAL_VOLTAGE_120	2

	unsigned char	drv_type;		/* driver type (A, B, C, D) */

#define MMC_SET_DRIVER_TYPE_B	0
#define MMC_SET_DRIVER_TYPE_A	1
#define MMC_SET_DRIVER_TYPE_C	2
#define MMC_SET_DRIVER_TYPE_D	3

	bool enhanced_strobe;			/* hs400es selection */
};

#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
struct mmc_cmdq_host_ops {
	int (*enable)(struct mmc_host *host);
	void (*disable)(struct mmc_host *host, bool soft);
	int (*request)(struct mmc_host *host, struct mmc_request *mrq);
	void (*post_req)(struct mmc_host *host, struct mmc_request *mrq,
			 int err);
	int (*halt)(struct mmc_host *host, bool halt);
};
#endif

struct mmc_host_ops {
	/*
	 * It is optional for the host to implement pre_req and post_req in
	 * order to support double buffering of requests (prepare one
	 * request while another request is active).
	 * pre_req() must always be followed by a post_req().
	 * To undo a call made to pre_req(), call post_req() with
	 * a nonzero err condition.
	 */
	void	(*post_req)(struct mmc_host *host, struct mmc_request *req,
			    int err);
	void	(*pre_req)(struct mmc_host *host, struct mmc_request *req,
			   bool is_first_req);
	void	(*request)(struct mmc_host *host, struct mmc_request *req);

	/*
	 * Avoid calling the next three functions too often or in a "fast
	 * path", since underlaying controller might implement them in an
	 * expensive and/or slow way. Also note that these functions might
	 * sleep, so don't call them in the atomic contexts!
	 */

	/*
	 * Notes to the set_ios callback:
	 * ios->clock might be 0. For some controllers, setting 0Hz
	 * as any other frequency works. However, some controllers
	 * explicitly need to disable the clock. Otherwise e.g. voltage
	 * switching might fail because the SDCLK is not really quiet.
	 */
	void	(*set_ios)(struct mmc_host *host, struct mmc_ios *ios);

	/*
	 * Return values for the get_ro callback should be:
	 *   0 for a read/write card
	 *   1 for a read-only card
	 *   -ENOSYS when not supported (equal to NULL callback)
	 *   or a negative errno value when something bad happened
	 */
	int	(*get_ro)(struct mmc_host *host);

	/*
	 * Return values for the get_cd callback should be:
	 *   0 for a absent card
	 *   1 for a present card
	 *   -ENOSYS when not supported (equal to NULL callback)
	 *   or a negative errno value when something bad happened
	 */
	int	(*get_cd)(struct mmc_host *host);

	void	(*enable_sdio_irq)(struct mmc_host *host, int enable);

	/* optional callback for HC quirks */
	void	(*init_card)(struct mmc_host *host, struct mmc_card *card);

	int	(*start_signal_voltage_switch)(struct mmc_host *host, struct mmc_ios *ios);

	/* Check if the card is pulling dat[0:3] low */
	int	(*card_busy)(struct mmc_host *host);

	/* The tuning command opcode value is different for SD and eMMC cards */
	int	(*execute_tuning)(struct mmc_host *host, u32 opcode);

	/* Prepare HS400 target operating frequency depending host driver */
	int	(*prepare_hs400_tuning)(struct mmc_host *host, struct mmc_ios *ios);
	/* Prepare enhanced strobe depending host driver */
	void	(*hs400_enhanced_strobe)(struct mmc_host *host,
					 struct mmc_ios *ios);
	int	(*select_drive_strength)(struct mmc_card *card,
					 unsigned int max_dtr, int host_drv,
					 int card_drv, int *drv_type);
	void	(*hw_reset)(struct mmc_host *host);
	void	(*card_event)(struct mmc_host *host);

	/*
	 * Optional callback to support controllers with HW issues for multiple
	 * I/O. Returns the number of supported blocks for the request.
	 */
	int	(*multi_io_quirk)(struct mmc_card *card,
				  unsigned int direction, int blk_size);
	void    (*dqs_tuning)(struct mmc_host *host);
};

struct mmc_card;
struct device;

/*we port kernel 4.14-4.16 command queue function to kernel 4.9 for realtek eMMC 5.1 IP*/
#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
struct mmc_cmdq_req {
	unsigned int cmd_flags;
	u32 blk_addr;
	/* active mmc request */
	struct mmc_request	mrq;
	struct mmc_data		data;
	struct mmc_command	cmd;
#define DCMD		(1 << 0)
#define QBR		(1 << 1)
#define DIR		(1 << 2)
#define PRIO		(1 << 3)
#define REL_WR		(1 << 4)
#define DAT_TAG	(1 << 5)
#define FORCED_PRG	(1 << 6)
	unsigned int		cmdq_req_flags;
	int			tag; /* used for command queuing */
	u8			ctx_id;
};


struct mmc_cqe_ops {
        /* Allocate resources, and make the CQE operational */
        int     (*cqe_enable)(struct mmc_host *host, struct mmc_card *card);
        /* Free resources, and make the CQE non-operational */
        void    (*cqe_disable)(struct mmc_host *host);
        /*
         * Issue a read, write or DCMD request to the CQE. Also deal with the
         * effect of ->cqe_off().
         */
        int     (*cqe_request)(struct mmc_host *host, struct mmc_request *mrq);
        /* Free resources (e.g. DMA mapping) associated with the request */
        void    (*cqe_post_req)(struct mmc_host *host, struct mmc_request *mrq);
        /*
         * Prepare the CQE and host controller to accept non-CQ commands. There
         * is no corresponding ->cqe_on(), instead ->cqe_request() is required
         * to deal with that.
         */
        void    (*cqe_off)(struct mmc_host *host);
        /*
         * Wait for all CQE tasks to complete. Return an error if recovery
         * becomes necessary.
         */
        int     (*cqe_wait_for_idle)(struct mmc_host *host);
        /*
         * Notify CQE that a request has timed out. Return false if the request
         * completed or true if a timeout happened in which case indicate if
         * recovery is needed.
         */
        bool    (*cqe_timeout)(struct mmc_host *host, struct mmc_request *mrq,
                               bool *recovery_needed);
        /*
         * Stop all CQE activity and prepare the CQE and host controller to
         * accept recovery commands.
         */
        void    (*cqe_recovery_start)(struct mmc_host *host);
        /*
         * Clear the queue and call mmc_cqe_request_done() on all requests.
         * Requests that errored will have the error set on the mmc_request
         * (data->error or cmd->error for DCMD).  Requests that did not error
         * will have zero data bytes transferred.
         */
        void    (*cqe_recovery_finish)(struct mmc_host *host);
};
#endif

struct mmc_async_req {
	/* active mmc request */
	struct mmc_request	*mrq;
	/*
	 * Check error status of completed mmc request.
	 * Returns 0 if success otherwise non zero.
	 */
	int (*err_check) (struct mmc_card *, struct mmc_async_req *);
};

/**
 * struct mmc_slot - MMC slot functions
 *
 * @cd_irq:		MMC/SD-card slot hotplug detection IRQ or -EINVAL
 * @handler_priv:	MMC/SD-card slot context
 *
 * Some MMC/SD host controllers implement slot-functions like card and
 * write-protect detection natively. However, a large number of controllers
 * leave these functions to the CPU. This struct provides a hook to attach
 * such slot-function drivers.
 */
struct mmc_slot {
	int cd_irq;
	void *handler_priv;
};

#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
/**
 * mmc_cmdq_context_info - describes the contexts of cmdq
 * @active_reqs		requests being processed
 * @curr_state		state of cmdq engine
 * @req_starved		completion should invoke the request_fn since
 *			no tags were available
 */
struct mmc_cmdq_context_info {
	unsigned long	active_reqs; /* in-flight requests */
	unsigned long	curr_state;
#define	CMDQ_STATE_ERR 0
#define	CMDQ_STATE_DCMD_ACTIVE 1
#define	CMDQ_STATE_HALT 2
	/* no free tag available */
	unsigned long	req_starved;
	wait_queue_head_t	wait;
};
#endif

/**
 * mmc_context_info - synchronization details for mmc context
 * @is_done_rcv		wake up reason was done request
 * @is_new_req		wake up reason was new request
 * @is_waiting_last_req	mmc context waiting for single running request
 * @wait		wait queue
 * @lock		lock to protect data fields
 */
struct mmc_context_info {
	bool			is_done_rcv;
	bool			is_new_req;
	bool			is_waiting_last_req;
	wait_queue_head_t	wait;
	spinlock_t		lock;
};

struct regulator;
struct mmc_pwrseq;

struct mmc_supply {
	struct regulator *vmmc;		/* Card power supply */
	struct regulator *vqmmc;	/* Optional Vccq supply */
};

struct mmc_host {
	struct device		*parent;
	struct device		class_dev;
	int			index;
	const struct mmc_host_ops *ops;
#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
	const struct mmc_cmdq_host_ops *cmdq_ops;
#endif
	struct mmc_pwrseq	*pwrseq;
	unsigned int		f_min;
	unsigned int		f_max;
	unsigned int		f_init;
	u32			ocr_avail;
	u32			ocr_avail_sdio;	/* SDIO-specific OCR */
	u32			ocr_avail_sd;	/* SD-specific OCR */
	u32			ocr_avail_mmc;	/* MMC-specific OCR */
#ifdef CONFIG_PM_SLEEP
	struct notifier_block	pm_notify;
#endif
	u32			max_current_330;
	u32			max_current_300;
	u32			max_current_180;

#ifdef CONFIG_RTK_PLATFORM
	u32			mode; //for rtkemmc_execute_tuning usage
#endif /* CONFIG_RTK_PLATFORM */

#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

	u32			caps;		/* Host capabilities */

#define MMC_CAP_4_BIT_DATA	(1 << 0)	/* Can the host do 4 bit transfers */
#define MMC_CAP_MMC_HIGHSPEED	(1 << 1)	/* Can do MMC high-speed timing */
#define MMC_CAP_SD_HIGHSPEED	(1 << 2)	/* Can do SD high-speed timing */
#define MMC_CAP_SDIO_IRQ	(1 << 3)	/* Can signal pending SDIO IRQs */
#define MMC_CAP_SPI		(1 << 4)	/* Talks only SPI protocols */
#define MMC_CAP_NEEDS_POLL	(1 << 5)	/* Needs polling for card-detection */
#define MMC_CAP_8_BIT_DATA	(1 << 6)	/* Can the host do 8 bit transfers */
#define MMC_CAP_AGGRESSIVE_PM	(1 << 7)	/* Suspend (e)MMC/SD at idle  */
#define MMC_CAP_NONREMOVABLE	(1 << 8)	/* Nonremovable e.g. eMMC */
#define MMC_CAP_WAIT_WHILE_BUSY	(1 << 9)	/* Waits while card is busy */
#define MMC_CAP_ERASE		(1 << 10)	/* Allow erase/trim commands */
#define MMC_CAP_1_8V_DDR	(1 << 11)	/* can support */
						/* DDR mode at 1.8V */
#define MMC_CAP_1_2V_DDR	(1 << 12)	/* can support */
						/* DDR mode at 1.2V */
#define MMC_CAP_POWER_OFF_CARD	(1 << 13)	/* Can power off after boot */
#define MMC_CAP_BUS_WIDTH_TEST	(1 << 14)	/* CMD14/CMD19 bus width ok */
#define MMC_CAP_UHS_SDR12	(1 << 15)	/* Host supports UHS SDR12 mode */
#define MMC_CAP_UHS_SDR25	(1 << 16)	/* Host supports UHS SDR25 mode */
#define MMC_CAP_UHS_SDR50	(1 << 17)	/* Host supports UHS SDR50 mode */
#define MMC_CAP_UHS_SDR104	(1 << 18)	/* Host supports UHS SDR104 mode */
#define MMC_CAP_UHS_DDR50	(1 << 19)	/* Host supports UHS DDR50 mode */
#define MMC_CAP_DRIVER_TYPE_A	(1 << 23)	/* Host supports Driver Type A */
#define MMC_CAP_DRIVER_TYPE_C	(1 << 24)	/* Host supports Driver Type C */
#define MMC_CAP_DRIVER_TYPE_D	(1 << 25)	/* Host supports Driver Type D */
#define MMC_CAP_CMD_DURING_TFR	(1 << 29)	/* Commands during data transfer */
#define MMC_CAP_CMD23		(1 << 30)	/* CMD23 supported. */
#define MMC_CAP_HW_RESET	(1 << 31)	/* Hardware reset */

	u32			caps2;		/* More host capabilities */

#define MMC_CAP2_BOOTPART_NOACC	(1 << 0)	/* Boot partition no access */
#define MMC_CAP2_FULL_PWR_CYCLE	(1 << 2)	/* Can do full power cycle */
#define MMC_CAP2_HS200_1_8V_SDR	(1 << 5)        /* can support */
#define MMC_CAP2_HS200_1_2V_SDR	(1 << 6)        /* can support */
#define MMC_CAP2_HS200		(MMC_CAP2_HS200_1_8V_SDR | \
				 MMC_CAP2_HS200_1_2V_SDR)
#define MMC_CAP2_HC_ERASE_SZ	(1 << 9)	/* High-capacity erase size */
#define MMC_CAP2_CD_ACTIVE_HIGH	(1 << 10)	/* Card-detect signal active high */
#define MMC_CAP2_RO_ACTIVE_HIGH	(1 << 11)	/* Write-protect signal active high */
#define MMC_CAP2_PACKED_RD	(1 << 12)	/* Allow packed read */
#define MMC_CAP2_PACKED_WR	(1 << 13)	/* Allow packed write */
#define MMC_CAP2_PACKED_CMD	(MMC_CAP2_PACKED_RD | \
				 MMC_CAP2_PACKED_WR)
#define MMC_CAP2_NO_PRESCAN_POWERUP (1 << 14)	/* Don't power up before scan */
#define MMC_CAP2_HS400_1_8V	(1 << 15)	/* Can support HS400 1.8V */
#define MMC_CAP2_HS400_1_2V	(1 << 16)	/* Can support HS400 1.2V */
#define MMC_CAP2_HS400		(MMC_CAP2_HS400_1_8V | \
				 MMC_CAP2_HS400_1_2V)
#define MMC_CAP2_HSX00_1_2V	(MMC_CAP2_HS200_1_2V_SDR | MMC_CAP2_HS400_1_2V)
#define MMC_CAP2_SDIO_IRQ_NOTHREAD (1 << 17)
#define MMC_CAP2_NO_WRITE_PROTECT (1 << 18)	/* No physical write protect pin, assume that card is always read-write */
#define MMC_CAP2_NO_SDIO	(1 << 19)	/* Do not send SDIO commands during initialization */
#define MMC_CAP2_HS400_ES	(1 << 20)	/* Host supports enhanced strobe */
#define MMC_CAP2_NO_SD		(1 << 21)	/* Do not send SD commands during initialization */
#define MMC_CAP2_NO_MMC		(1 << 22)	/* Do not send (e)MMC commands during initialization */

/*we port kernel 4.14-4.16 command queue function to kernel 4.9 for realtek eMMC 5.1 IP*/
#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
#define MMC_CAP2_CQE		(1 << 23)	/* Has eMMC command queue engine */
#define MMC_CAP2_CQE_DCMD	(1 << 24)	/* CQE can issue a direct command */
#define MMC_CAP2_CMD_QUEUE	(1 << 25)	/* support eMMC command queue */
#endif
	mmc_pm_flag_t		pm_caps;	/* supported pm features */

	/* host specific block data */
	unsigned int		max_seg_size;	/* see blk_queue_max_segment_size */
	unsigned short		max_segs;	/* see blk_queue_max_segments */
	unsigned short		unused;
	unsigned int		max_req_size;	/* maximum number of bytes in one req */
	unsigned int		max_blk_size;	/* maximum size of one mmc block */
	unsigned int		max_blk_count;	/* maximum number of blocks in one req */
	unsigned int		max_busy_timeout; /* max busy timeout in ms */

	/* private data */
	spinlock_t		lock;		/* lock for claim and bus ops */

	struct mmc_ios		ios;		/* current io bus settings */

	/* group bitfields together to minimize padding */
	unsigned int		use_spi_crc:1;
	unsigned int		claimed:1;	/* host exclusively claimed */
	unsigned int		bus_dead:1;	/* bus has been released */
#ifdef CONFIG_MMC_DEBUG
	unsigned int		removed:1;	/* host is being removed */
#endif
	unsigned int		can_retune:1;	/* re-tuning can be used */
	unsigned int		doing_retune:1;	/* re-tuning in progress */
	unsigned int		retune_now:1;	/* do re-tuning at next req */
	unsigned int		retune_paused:1; /* re-tuning is temporarily disabled */

	int			rescan_disable;	/* disable card detection */
	int			rescan_entered;	/* used with nonremovable devices */

	int			need_retune;	/* re-tuning is needed */
	int			hold_retune;	/* hold off re-tuning */
	unsigned int		retune_period;	/* re-tuning period in secs */
	struct timer_list	retune_timer;	/* for periodic re-tuning */

	bool			trigger_card_event; /* card_event necessary */

	struct mmc_card		*card;		/* device attached to this host */

	wait_queue_head_t	wq;
	struct task_struct	*claimer;	/* task that has host claimed */
	int			claim_cnt;	/* "claim" nesting count */

	struct delayed_work	detect;
	int			detect_change;	/* card detect flag */
	struct mmc_slot		slot;

	const struct mmc_bus_ops *bus_ops;	/* current bus driver */
	unsigned int		bus_refs;	/* reference counter */

#ifdef CONFIG_RTK_PLATFORM
	unsigned int		bus_resume_flags;
	#define MMC_BUSRESUME_MANUAL_RESUME		(1 << 0)
	#define MMC_BUSRESUME_NEEDS_RESUME		(1 << 1)
#endif /* CONFIG_RTK_PLATFORM */

	unsigned int		sdio_irqs;
	struct task_struct	*sdio_irq_thread;
	bool			sdio_irq_pending;
	atomic_t		sdio_irq_thread_abort;

	mmc_pm_flag_t		pm_flags;	/* requested pm features */

	struct led_trigger	*led;		/* activity led */

#ifdef CONFIG_REGULATOR
	bool			regulator_enabled; /* regulator state */
#endif
	struct mmc_supply	supply;

	struct dentry		*debugfs_root;

	struct mmc_async_req	*areq;		/* active async req */
	struct mmc_context_info	context_info;	/* async synchronization info */

	/* Ongoing data transfer that allows commands during transfer */
	struct mmc_request	*ongoing_mrq;

#ifdef CONFIG_FAIL_MMC_REQUEST
	struct fault_attr	fail_mmc_request;
#endif

	unsigned int		actual_clock;	/* Actual HC clock rate */

	unsigned int		slotno;	/* used for sdio acpi binding */

	int			dsr_req;	/* DSR value is valid */
	u32			dsr;	/* optional driver stage (DSR) value */

/*we port kernel 4.14-4.16 command queue function to kernel 4.9 for realtek eMMC 5.1 IP*/
#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
	struct mmc_cmdq_context_info	cmdq_ctx;
	/*
	 * several cmdq supporting host controllers are extensions
	 * of legacy controllers. This variable can be used to store
	 * a reference to the cmdq extension of the existing host
	 * controller.
	 */
	void *cmdq_private;
	/* Command Queue Engine (CQE) support */
	const struct mmc_cqe_ops *cqe_ops;
	void			*cqe_private;
	int			cqe_qdepth;
	bool			cqe_enabled;
	bool			cqe_on;
#endif

#ifdef CONFIG_MMC_EMBEDDED_SDIO
	struct {
		struct sdio_cis			*cis;
		struct sdio_cccr		*cccr;
		struct sdio_embedded_func	*funcs;
		int				num_funcs;
	} embedded_sdio_data;
#endif

#ifdef CONFIG_BLOCK
	int			latency_hist_enabled;
	struct io_latency_state io_lat_read;
	struct io_latency_state io_lat_write;
#endif

#ifdef CONFIG_ARCH_RTD119X
	unsigned int        card_type_pre;
	#define CR_SD       0 //for card reader
	#define CR_EM       2 //for eMMC
#endif

	unsigned long		private[0] ____cacheline_aligned;
};

struct mmc_host *mmc_alloc_host(int extra, struct device *);
int mmc_add_host(struct mmc_host *);
void mmc_remove_host(struct mmc_host *);
void mmc_free_host(struct mmc_host *);
int mmc_of_parse(struct mmc_host *host);

#ifdef CONFIG_MMC_EMBEDDED_SDIO
extern void mmc_set_embedded_sdio_data(struct mmc_host *host,
				       struct sdio_cis *cis,
				       struct sdio_cccr *cccr,
				       struct sdio_embedded_func *funcs,
				       int num_funcs);
#endif

static inline void *mmc_priv(struct mmc_host *host)
{
	return (void *)host->private;
}

#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
static inline void *mmc_cmdq_private(struct mmc_host *host)
{
	return host->cmdq_private;
}
#endif

#define mmc_host_is_spi(host)	((host)->caps & MMC_CAP_SPI)

#define mmc_dev(x)	((x)->parent)
#define mmc_classdev(x)	(&(x)->class_dev)
#define mmc_hostname(x)	(dev_name(&(x)->class_dev))

#ifdef CONFIG_RTK_PLATFORM
#define mmc_bus_needs_resume(host) ((host)->bus_resume_flags & MMC_BUSRESUME_NEEDS_RESUME)
#define mmc_bus_manual_resume(host) ((host)->bus_resume_flags & MMC_BUSRESUME_MANUAL_RESUME)
int mmc_suspend_host(struct mmc_host *);
int mmc_resume_host(struct mmc_host *);
#endif /* CONFIG_RTK_PLATFORM */

int mmc_power_save_host(struct mmc_host *host);
int mmc_power_restore_host(struct mmc_host *host);

void mmc_detect_change(struct mmc_host *, unsigned long delay);
void mmc_request_done(struct mmc_host *, struct mmc_request *);
void mmc_command_done(struct mmc_host *host, struct mmc_request *mrq);

static inline void mmc_signal_sdio_irq(struct mmc_host *host)
{
	host->ops->enable_sdio_irq(host, 0);
	host->sdio_irq_pending = true;
	if (host->sdio_irq_thread)
		wake_up_process(host->sdio_irq_thread);
}

void sdio_run_irqs(struct mmc_host *host);

#ifdef CONFIG_REGULATOR
int mmc_regulator_get_ocrmask(struct regulator *supply);
int mmc_regulator_set_ocr(struct mmc_host *mmc,
			struct regulator *supply,
			unsigned short vdd_bit);
int mmc_regulator_set_vqmmc(struct mmc_host *mmc, struct mmc_ios *ios);
#else
static inline int mmc_regulator_get_ocrmask(struct regulator *supply)
{
	return 0;
}

static inline int mmc_regulator_set_ocr(struct mmc_host *mmc,
				 struct regulator *supply,
				 unsigned short vdd_bit)
{
	return 0;
}

static inline int mmc_regulator_set_vqmmc(struct mmc_host *mmc,
					  struct mmc_ios *ios)
{
	return -EINVAL;
}
#endif

int mmc_regulator_get_supply(struct mmc_host *mmc);

static inline int mmc_card_is_removable(struct mmc_host *host)
{
	return !(host->caps & MMC_CAP_NONREMOVABLE);
}

static inline int mmc_card_keep_power(struct mmc_host *host)
{
	return host->pm_flags & MMC_PM_KEEP_POWER;
}

static inline int mmc_card_wake_sdio_irq(struct mmc_host *host)
{
	return host->pm_flags & MMC_PM_WAKE_SDIO_IRQ;
}

static inline int mmc_host_cmd23(struct mmc_host *host)
{
	return host->caps & MMC_CAP_CMD23;
}

static inline int mmc_boot_partition_access(struct mmc_host *host)
{
	return !(host->caps2 & MMC_CAP2_BOOTPART_NOACC);
}

static inline int mmc_host_uhs(struct mmc_host *host)
{
	return host->caps &
		(MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
		 MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
		 MMC_CAP_UHS_DDR50);
}

static inline int mmc_host_packed_wr(struct mmc_host *host)
{
	return host->caps2 & MMC_CAP2_PACKED_WR;
}

#if defined(CONFIG_ARCH_RTD13xx) && defined(CONFIG_MMC_RTK_EMMC) && defined(CONFIG_MMC_RTK_EMMC_CMDQ)
static inline void mmc_host_set_halt(struct mmc_host *host)
{
	set_bit(CMDQ_STATE_HALT, &host->cmdq_ctx.curr_state);
}

static inline void mmc_host_clr_halt(struct mmc_host *host)
{
	clear_bit(CMDQ_STATE_HALT, &host->cmdq_ctx.curr_state);
}

static inline int mmc_host_halt(struct mmc_host *host)
{
	return test_bit(CMDQ_STATE_HALT, &host->cmdq_ctx.curr_state);
}
#endif
static inline int mmc_card_hs(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_SD_HS ||
		card->host->ios.timing == MMC_TIMING_MMC_HS;
}

static inline int mmc_card_uhs(struct mmc_card *card)
{
	return card->host->ios.timing >= MMC_TIMING_UHS_SDR12 &&
		card->host->ios.timing <= MMC_TIMING_UHS_DDR50;
}

static inline bool mmc_card_hs200(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_MMC_HS200;
}

static inline bool mmc_card_ddr52(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_MMC_DDR52;
}

static inline bool mmc_card_hs400(struct mmc_card *card)
{
	return card->host->ios.timing == MMC_TIMING_MMC_HS400;
}

static inline bool mmc_card_hs400es(struct mmc_card *card)
{
	return card->host->ios.enhanced_strobe;
}

void mmc_retune_timer_stop(struct mmc_host *host);

static inline void mmc_retune_needed(struct mmc_host *host)
{
	if (host->can_retune)
		host->need_retune = 1;
}

static inline void mmc_retune_recheck(struct mmc_host *host)
{
	if (host->hold_retune <= 1)
		host->retune_now = 1;
}

void mmc_retune_pause(struct mmc_host *host);
void mmc_retune_unpause(struct mmc_host *host);

#endif /* LINUX_MMC_HOST_H */
