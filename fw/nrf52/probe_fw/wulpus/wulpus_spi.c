#include "wulpus_spi.h"

#include "nrf_drv_spi.h"

#include "wulpus_config.h"

#define NRF_LOG_MODULE_NAME wp_spi
#define NRF_LOG_LEVEL       4
#define NRF_LOG_INFO_COLOR  0
#define NRF_LOG_DEBUG_COLOR 5
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(0);


void _wp_spi_evt_handler(nrf_drv_spi_evt_t const *p_event,
                         void *                p_context)
{
  // Never called since NRF_DRV_SPI_FLAG_NO_XFER_EVT_HANDLER flag is used
  NRF_LOG_WARNING("Event handler called, should not happen");
}

ret_code_t wp_spi_init(uint8_t *tx_buffer, uint16_t tx_length, uint8_t *rx_buffer, uint16_t rx_length)
{
  // Initialize SPI instance
  nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
  spi_config.ss_pin = WULPUS_SPI_NUM_CS;
  spi_config.sck_pin = WULPUS_SPI_NUM_SCK;
  spi_config.miso_pin = WULPUS_SPI_NUM_MISO;
  spi_config.mosi_pin = WULPUS_SPI_NUM_MOSI;
  spi_config.frequency = NRF_SPI_FREQ_8M;
  spi_config.bit_order = NRF_SPI_BIT_ORDER_MSB_FIRST;
  spi_config.mode = NRF_SPI_MODE_1;
  WP_ERR_RET(nrf_drv_spi_init(&spi, &spi_config, _wp_spi_evt_handler, NULL));

  NRF_LOG_DEBUG("Initialized instance");

  // Set up a transfer using EasyDMA
  nrf_drv_spi_xfer_desc_t xfer = NRF_DRV_SPI_XFER_TRX(tx_buffer, tx_length, rx_buffer, rx_length);
  uint32_t flags = NRF_DRV_SPI_FLAG_HOLD_XFER | NRF_DRV_SPI_FLAG_RX_POSTINC | NRF_DRV_SPI_FLAG_REPEATED_XFER | NRF_DRV_SPI_FLAG_NO_XFER_EVT_HANDLER;

  WP_ERR_RET(nrf_drv_spi_xfer(&spi, &xfer, flags));

  NRF_LOG_DEBUG("Set up transfer");

  NRF_LOG_INFO("Initialized");

  return NRF_SUCCESS;
}

const nrf_drv_spi_t *wp_spi_get_instance(void)
{
  return &spi;
}

void wp_spi_set_buffer(uint8_t *buffer)
{
  NRF_SPIM0->RXD.PTR = (uint32_t)buffer;
}
