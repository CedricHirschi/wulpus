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

uint8_t *_wp_spi_rx_buffer = NULL;
uint8_t _wp_spi_rx_length = 0;
uint8_t *_wp_spi_tx_buffer = NULL;
uint8_t _wp_spi_tx_length = 0;

void _wp_spi_evt_handler(nrf_drv_spi_evt_t const *p_event, void *p_context)
{
  // Never called since NRF_DRV_SPI_FLAG_NO_XFER_EVT_HANDLER flag is used
  // NRF_LOG_WARNING("Event handler called, should not happen");

  // NRF_LOG_DEBUG("Event handler called");
  switch (p_event->type)
  {
    case NRF_DRV_SPI_EVENT_DONE:
      NRF_LOG_DEBUG("rx/tx length: %u/%u", p_event->data.done.rx_length, p_event->data.done.tx_length);
      break;
  }
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

  _wp_spi_tx_buffer = tx_buffer;
  _wp_spi_tx_length = tx_length;
  _wp_spi_rx_buffer = rx_buffer;
  _wp_spi_rx_length = rx_length;

  NRF_LOG_INFO("Initialized");

  return NRF_SUCCESS;
}

const nrf_drv_spi_t *wp_spi_get_instance(void)
{
  return &spi;
}

void wp_spi_set_buffer(uint8_t *buffer)
{
  spi.u.spim.p_reg->RXD.PTR = (uint32_t)buffer;
}

ret_code_t wp_spi_send_config(uint8_t const *buffer, uint8_t length)
{
  NRF_LOG_DEBUG("Sending config of length %u", length);
  memset(_wp_spi_tx_buffer, 0, _wp_spi_tx_length);
  memcpy(_wp_spi_tx_buffer, buffer, length);
  nrf_drv_spi_xfer_desc_t send_config_config = NRF_DRV_SPI_XFER_TX(_wp_spi_tx_buffer, _wp_spi_tx_length);
  WP_ERR_RET(nrf_drv_spi_xfer(&spi, &send_config_config, NULL));

  return NRF_SUCCESS;
}

ret_code_t wp_spi_init_reception(void)
{
  nrf_drv_spi_xfer_desc_t xfer = NRF_DRV_SPI_XFER_TRX(_wp_spi_tx_buffer, _wp_spi_tx_length, _wp_spi_rx_buffer, _wp_spi_rx_length);
  uint32_t flags = NRF_DRV_SPI_FLAG_HOLD_XFER | NRF_DRV_SPI_FLAG_RX_POSTINC | NRF_DRV_SPI_FLAG_REPEATED_XFER | NRF_DRV_SPI_FLAG_NO_XFER_EVT_HANDLER;
  WP_ERR_RET(nrf_drv_spi_xfer(&spi, &xfer, flags));

  return NRF_SUCCESS;
}

void wp_spi_stop_reception(void)
{
  nrf_drv_spi_abort(&spi);
}
