#ifndef __WULPUS_SPI__
#define __WULPUS_SPI__

#include "nrf_drv_spi.h"

#include "wulpus_common.h"

ret_code_t wp_spi_init(uint8_t *tx_buffer, uint16_t tx_length, uint8_t *rx_buffer, uint16_t rx_length);
const nrf_drv_spi_t *wp_spi_get_instance(void);

void wp_spi_set_buffer(uint8_t *buffer);

#endif // __WULPUS_SPI__