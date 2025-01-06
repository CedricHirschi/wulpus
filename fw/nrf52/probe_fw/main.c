/** 
 * @file main.c
 *
 * @brief WULPUS firmware main file
 *
 * This file contains the main source code for the firmware of the WULPUS probe developed at ETH Zürich.
 *
 * @author Cédric Hirschi
 *
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_util_platform.h"

#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "wulpus_common.h"
#include "wulpus_gpio.h"
#include "wulpus_spi.h"
#include "wulpus_ppi.h"
#include "wulpus_ble.h"
#include "wulpus_config.h"

static void handle_idle_state(void);

#define FRAME_SIZE (WULPUS_NUMBER_OF_XFERS * WULPUS_BYTES_PER_XFER)

uint8_t tx_buffer[FRAME_SIZE];
uint8_t rx_buffer[FRAME_SIZE * WULPUS_NUM_BUFFERED_FRAMES];

size_t rx_buffer_head = 0;
size_t rx_buffer_tail = 0;

// Called when data ready signal is received from MSP430 (main/in_pin_handler)
void gpio_data_ready_handler(void)
{
  // Set new RX buffer
  wp_spi_set_buffer(rx_buffer + rx_buffer_head * FRAME_SIZE);

  // Start the four SPI transactions
  wp_ppi_start_transfer();
}

// Called when all SPI transfers for one packet are done (us_spi/counter_cc0_event_handler)
void ppi_end_handler(void)
{
  rx_buffer_head = (rx_buffer_head + 1) % WULPUS_NUM_BUFFERED_FRAMES;
  if (rx_buffer_head == rx_buffer_tail)
  {
    NRF_LOG_WARNING("RX Buffer overflow!");
  }
}

// Called when connection status of the bluetooth connection changes (new)
void ble_conn_handler(bool connected)
{
  // TODO: Stop transfers here? Send config package to MSP430?
  if (!connected)
  {
    // Stop any running transfers
    wp_ppi_stop_transfer();
  }
}

// Called when data is received via NUS (us_ble/nus_data_handler)
void ble_data_handler(uint8_t const *data, uint16_t length)
{
  // Stop any running transfers
  wp_ppi_stop_transfer();

  // Copy received command from python to the SPI transmit buffer
  memcpy(tx_buffer, data, length);

  // Clear the BLE buffers to send US data with the received configuration
  rx_buffer_head = 0;
  rx_buffer_tail = 0;
}

// Checks if there are any frames queued to be processed and processes them
void handle_pending_frames(void)
{
  if (rx_buffer_tail != rx_buffer_head)
  {
    APP_ERROR_CHECK(wp_ble_transmit(rx_buffer + rx_buffer_tail * FRAME_SIZE + 0 * WULPUS_BYTES_PER_XFER + 1, WULPUS_BYTES_PER_XFER + 1));
    APP_ERROR_CHECK(wp_ble_transmit(rx_buffer + rx_buffer_tail * FRAME_SIZE + 1 * WULPUS_BYTES_PER_XFER, WULPUS_BYTES_PER_XFER));
    APP_ERROR_CHECK(wp_ble_transmit(rx_buffer + rx_buffer_tail * FRAME_SIZE + 2 * WULPUS_BYTES_PER_XFER, WULPUS_BYTES_PER_XFER));
    APP_ERROR_CHECK(wp_ble_transmit(rx_buffer + rx_buffer_tail * FRAME_SIZE + 3 * WULPUS_BYTES_PER_XFER, WULPUS_BYTES_PER_XFER));

    rx_buffer_tail = (rx_buffer_tail + 1) % WULPUS_NUM_BUFFERED_FRAMES;
  }
}

/**
 * @brief Main application function
 *
 */
int main(void)
{
  // Initialize logging and power management
  APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
  NRF_LOG_DEFAULT_BACKENDS_INIT();
  APP_ERROR_CHECK(nrf_pwr_mgmt_init());
  NRF_LOG_INFO("Hello, world!");
  
  // Initialize GPIO
  APP_ERROR_CHECK(wp_gpio_init());
  APP_ERROR_CHECK(wp_gpio_add_data_handler(gpio_data_ready_handler));
  
  // Initialize SPI
  APP_ERROR_CHECK(wp_spi_init(tx_buffer, WULPUS_BYTES_PER_XFER, rx_buffer, WULPUS_BYTES_PER_XFER));
  
  // Initialize PPI
  APP_ERROR_CHECK(wp_ppi_init(wp_spi_get_instance()));
  APP_ERROR_CHECK(wp_ppi_add_end_handler(ppi_end_handler));
  
  // Initialize BLE
  APP_ERROR_CHECK(wp_ble_init());
  APP_ERROR_CHECK(wp_ble_add_conn_handler(wp_gpio_ble_conn_indicate));
  APP_ERROR_CHECK(wp_ble_add_conn_handler(ble_conn_handler));
  APP_ERROR_CHECK(wp_ble_add_data_handler(ble_data_handler));

  // Start advertising
  APP_ERROR_CHECK(wp_ble_advertising_start());

  // Enter main loop
  while (1)
  {
    handle_pending_frames();
    handle_idle_state();
  }
}

/**
 * @brief Function for handling the idle state in the main loop
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 *
 */
static void handle_idle_state(void)
{
  if (NRF_LOG_PROCESS() == false)
  {
    nrf_pwr_mgmt_run();
  }
}
