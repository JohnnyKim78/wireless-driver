#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/spi/spi.h>

#include "linux_wlan_common.h"

#define USE_SPI_DMA     0       /* johnny add */

#ifdef WILC_ASIC_A0
 #if defined(PLAT_PANDA_ES_OMAP4460)
  #define MIN_SPEED 12000000
  #define MAX_SPEED 24000000
 #elif defined(PLAT_WMS8304)
  #define MIN_SPEED 12000000
  #define MAX_SPEED 24000000 /* 4000000 */
 #elif defined(CUSTOMER_PLATFORM)
/*
  TODO : define Clock speed under 48M.
 *
 * ex)
 * #define MIN_SPEED 24000000
 * #define MAX_SPEED 48000000
 */
 #else
  #define MIN_SPEED 24000000
  #define MAX_SPEED 48000000
 #endif
#else /* WILC_ASIC_A0 */
/* Limit clk to 6MHz on FPGA. */
 #define MIN_SPEED 6000000
 #define MAX_SPEED 6000000
#endif /* WILC_ASIC_A0 */

static uint32_t SPEED = MIN_SPEED;

struct spi_device *wilc_spi_dev;
void linux_spi_deinit(void *vp);

static int __init wilc_bus_probe(struct spi_device *spi)
{

	PRINT_D(BUS_DBG, "spiModalias: %s\n", spi->modalias);
	PRINT_D(BUS_DBG, "spiMax-Speed: %d\n", spi->max_speed_hz);
	wilc_spi_dev = spi;

	printk("Driver Initializing success\n");
	return 0;
}

static int __exit wilc_bus_remove(struct spi_device *spi)
{

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id wilc1000_of_match[] = {
	{ .compatible = "atmel,wilc_spi", },
	{}
};
MODULE_DEVICE_TABLE(of, wilc1000_of_match);
#endif

struct spi_driver wilc_bus __refdata = {
	.driver = {
		.name = MODALIAS,
#ifdef CONFIG_OF
		.of_match_table = wilc1000_of_match,
#endif
	},
	.probe =  wilc_bus_probe,
	.remove = __exit_p(wilc_bus_remove),
};


void linux_spi_deinit(void *vp)
{

	spi_unregister_driver(&wilc_bus);

	SPEED = MIN_SPEED;
	PRINT_ER("@@@@@@@@@@@@ restore SPI speed to %d @@@@@@@@@\n", SPEED);

}



int linux_spi_init(void *vp)
{
	int ret = 1;
	static int called;


	if (called == 0) {
		called++;
		ret = spi_register_driver(&wilc_bus);
	}

	/* change return value to match WILC interface */
	(ret < 0) ? (ret = 0) : (ret = 1);

	return ret;
}

#if defined (NM73131_0_BOARD)

int linux_spi_write(uint8_t *b, uint32_t len)
{

	int ret;

	if (len > 0 && b != NULL) {
		struct spi_message msg;
		PRINT_D(BUS_DBG, "Request writing %d bytes\n", len);
		struct spi_transfer tr = {
			.tx_buf = b,
			.len = len,
			.speed_hz = SPEED,
			.delay_usecs = 0,
		};

		spi_message_init(&msg);
		spi_message_add_tail(&tr, &msg);
		ret = spi_sync(wilc_spi_dev, &msg);
		if (ret < 0) {
			PRINT_ER("SPI transaction failed\n");
		}

	} else {
		PRINT_ER("can't write data with the following length: %d\n", len);
		PRINT_ER("FAILED due to NULL buffer or ZERO length check the following length: %d\n", len);
		ret = -1;
	}

	/* change return value to match WILC interface */
	(ret < 0) ? (ret = 0) : (ret = 1);


	return ret;
}

#else
int linux_spi_write(uint8_t *b, uint32_t len)
{

	int ret;
	struct spi_message msg;

	if (len > 0 && b != NULL) {
		struct spi_transfer tr = {
			.tx_buf = b,
			.len = len,
			.speed_hz = SPEED,
			.delay_usecs = 0,
		};
		char *r_buffer = kzalloc(len, GFP_KERNEL);
		if (!r_buffer) {
			PRINT_ER("Failed to allocate memory for r_buffer\n");
		}
		tr.rx_buf = r_buffer;
		PRINT_D(BUS_DBG, "Request writing %d bytes\n", len);

		memset(&msg, 0, sizeof(msg));
		spi_message_init(&msg);
		spi_message_add_tail(&tr,&msg);

		ret = spi_sync(wilc_spi_dev,&msg);
		if(ret < 0){
			PRINT_ER( "SPI transaction failed\n");
		}

		kfree(r_buffer);
	} else {
		PRINT_ER("can't write data with the following length: %d\n", len);
		PRINT_ER("FAILED due to NULL buffer or ZERO length check the following length: %d\n", len);
		ret = -1;
	}

	/* change return value to match WILC interface */
	(ret < 0) ? (ret = 0) : (ret = 1);


	return ret;
}

#endif

#if defined (NM73131_0_BOARD)

int linux_spi_read(unsigned char *rb, unsigned long rlen)
{

	int ret;

	if (rlen > 0) {
		struct spi_message msg;
		struct spi_transfer tr = {
			.rx_buf = rb,
			.len = rlen,
			.speed_hz = SPEED,
			.delay_usecs = 0,

		};

		spi_message_init(&msg);
		spi_message_add_tail(&tr, &msg);
		ret = spi_sync(wilc_spi_dev, &msg);
		if (ret < 0) {
			PRINT_ER("SPI transaction failed\n");
		}
	} else {
		PRINT_ER("can't read data with the following length: %ld\n", rlen);
		ret = -1;
	}
	/* change return value to match WILC interface */
	(ret < 0) ? (ret = 0) : (ret = 1);

	return ret;
}
#else
int linux_spi_read(unsigned char *rb, unsigned long rlen)
{

	int ret;

	if (rlen > 0) {
		struct spi_message msg;
		struct spi_transfer tr = {
			.rx_buf = rb,
			.len = rlen,
			.speed_hz = SPEED,
			.delay_usecs = 0,

		};
		char *t_buffer = kzalloc(rlen, GFP_KERNEL);
		if (!t_buffer) {
			PRINT_ER("Failed to allocate memory for t_buffer\n");
		}
		tr.tx_buf = t_buffer;

		memset(&msg, 0, sizeof(msg));
		spi_message_init(&msg);
/* [[ johnny add */
		msg.spi = wilc_spi_dev;
		msg.is_dma_mapped = USE_SPI_DMA;
/* ]] */
		spi_message_add_tail(&tr, &msg);

		ret = spi_sync(wilc_spi_dev, &msg);
		if (ret < 0) {
			PRINT_ER("SPI transaction failed\n");
		}
		kfree(t_buffer);
	} else {
		PRINT_ER("can't read data with the following length: %ld\n", rlen);
		ret = -1;
	}
	/* change return value to match WILC interface */
	(ret < 0) ? (ret = 0) : (ret = 1);

	return ret;
}

#endif

int linux_spi_write_read(unsigned char *wb, unsigned char *rb, unsigned int rlen)
{

	int ret;

	if (rlen > 0) {
		struct spi_message msg;
		struct spi_transfer tr = {
			.rx_buf = rb,
			.tx_buf = wb,
			.len = rlen,
			.speed_hz = SPEED,
			.bits_per_word = 8,
			.delay_usecs = 0,

		};

		memset(&msg, 0, sizeof(msg));
		spi_message_init(&msg);
		msg.spi = wilc_spi_dev;
		msg.is_dma_mapped = USE_SPI_DMA;

		spi_message_add_tail(&tr, &msg);
		ret = spi_sync(wilc_spi_dev, &msg);
		if (ret < 0) {
			PRINT_ER("SPI transaction failed\n");
		}
	} else {
		PRINT_ER("can't read data with the following length: %d\n", rlen);
		ret = -1;
	}
	/* change return value to match WILC interface */
	(ret < 0) ? (ret = 0) : (ret = 1);

	return ret;
}

int linux_spi_set_max_speed(void)
{
	SPEED = MAX_SPEED;

	PRINT_INFO(BUS_DBG, "@@@@@@@@@@@@ change SPI speed to %d @@@@@@@@@\n", SPEED);
	return 1;
}
