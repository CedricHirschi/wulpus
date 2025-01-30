#include "wulpus_ppi.h"

#include "nrfx_timer.h"
#include "nrfx_ppi.h"

#include "wulpus_config.h"
#include "wulpus_spi.h"

#define NRF_LOG_MODULE_NAME wp_ppi
#define NRF_LOG_LEVEL       4
#define NRF_LOG_INFO_COLOR  0
#define NRF_LOG_DEBUG_COLOR 5
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


// TIMER3: Used to start SPI transfers at regular intervals
const nrfx_timer_t tim_timeout = NRFX_TIMER_INSTANCE(3);
// TIMER4: Used in Counter mode to count number of completed transfers and stop the SPI after certain number of transfers
const nrfx_timer_t tim_counter = NRFX_TIMER_INSTANCE(4);

wp_ppi_end_handler_t _wp_ppi_end_handlers[WULPUS_PPI_MAX_END_HANDLERS];
size_t _wp_ppi_end_handlers_num = 0;

// Task and event addresses for SPI transactions
uint32_t task_spi_start_addr;
uint32_t event_spi_end_addr;
uint32_t event_timeout_addr;
uint32_t task_cnt_count_addr;


void _wp_ppi_tim_timeout_handler(nrf_timer_event_t event_type, void *p_context)
{
  // Handler must be declared and passed to nrfx_timer_init(),
  // but is not used if nrfx_timer_extended_compare() is called with enable_int flag = false
  NRF_LOG_WARNING("Timeout handler called, should not happen");
}

void _wp_ppi_tim_counter_handler(nrf_timer_event_t event_type, void *p_context)
{
  // Stop timers and thus SPI transfers
  wp_ppi_stop_transfer();
  wp_spi_stop_reception();

  NRF_LOG_DEBUG("Counter handler called: %u callbacks", _wp_ppi_end_handlers_num);

  for (size_t i = 0; i < _wp_ppi_end_handlers_num; i++)
  {
    _wp_ppi_end_handlers[i]();
  }
}

ret_code_t wp_ppi_init(const nrf_drv_spi_t *spi_instance)
{
  // Initialize Timers
  // ------------------------------------------------------
  // Initialize timer to start SPI transfers in given intervals
  nrfx_timer_config_t tim_timeout_config = NRFX_TIMER_DEFAULT_CONFIG;
  WP_ERR_RET(nrfx_timer_init(&tim_timeout, &tim_timeout_config, _wp_ppi_tim_timeout_handler));

  uint32_t tim_timeout_ticks = nrfx_timer_us_to_ticks(&tim_timeout, WULPUS_SPI_PACKET_INTERVAL);
  nrfx_timer_extended_compare(&tim_timeout, NRF_TIMER_CC_CHANNEL0, tim_timeout_ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

  event_timeout_addr = nrfx_timer_event_address_get(&tim_timeout, NRF_TIMER_EVENT_COMPARE0);

  // Initialize timer to count SPI transfers
  nrfx_timer_config_t tim_counter_config = NRFX_TIMER_DEFAULT_CONFIG;
  tim_counter_config.mode = NRF_TIMER_MODE_COUNTER;
  WP_ERR_RET(nrfx_timer_init(&tim_counter, &tim_counter_config, _wp_ppi_tim_counter_handler));

  nrfx_timer_extended_compare(&tim_counter, NRF_TIMER_CC_CHANNEL0, WULPUS_NUMBER_OF_XFERS, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
  
  task_cnt_count_addr = nrfx_timer_task_address_get(&tim_counter, NRF_TIMER_TASK_COUNT);

  NRF_LOG_DEBUG("Initialized timers");

  // Initialize PPI channels
  // ------------------------------------------------------
  nrf_ppi_channel_t channel_start_spi;
  nrf_ppi_channel_t channel_end_spi;

  task_spi_start_addr = nrf_drv_spi_start_task_get(spi_instance);
  event_spi_end_addr = nrf_drv_spi_end_event_get(spi_instance);
  
  // Timer 3 CC causes SPI to start
  WP_ERR_RET(nrfx_ppi_channel_alloc(&channel_start_spi));
  
  WP_ERR_RET(nrfx_ppi_channel_assign(channel_start_spi, event_timeout_addr, task_spi_start_addr));
  

  // SPI end event causes timer counter 4 to increment
  WP_ERR_RET(nrfx_ppi_channel_alloc(&channel_end_spi));
  
  WP_ERR_RET(nrfx_ppi_channel_assign(channel_end_spi, event_spi_end_addr, task_cnt_count_addr));
  
  // Enable both configured PPI channels
  WP_ERR_RET(nrfx_ppi_channel_enable(channel_start_spi));
  WP_ERR_RET(nrfx_ppi_channel_enable(channel_end_spi));

  NRF_LOG_DEBUG("Initialized channels");

  NRF_LOG_INFO("Initialized");

  return NRF_SUCCESS;
}

ret_code_t wp_ppi_add_end_handler(wp_ppi_end_handler_t handler)
{
  if (_wp_ppi_end_handlers_num >= WULPUS_BLE_MAX_CONN_HANDLERS) return NRF_ERROR_CONN_COUNT;

  _wp_ppi_end_handlers[_wp_ppi_end_handlers_num++] = handler;

  NRF_LOG_DEBUG("Added end handler: %u/%u", _wp_ppi_end_handlers_num, WULPUS_BLE_MAX_CONN_HANDLERS);

  return NRF_SUCCESS;
}

void wp_ppi_start_transfer(void)
{
  // First, reset the timers to get clean situation
  nrfx_timer_clear(&tim_timeout);
  nrfx_timer_clear(&tim_counter);

  // Then, enable the timers
  nrfx_timer_enable(&tim_timeout);
  nrfx_timer_enable(&tim_counter);

  NRF_LOG_DEBUG("Enabled transfer");
}

void wp_ppi_stop_transfer(void)
{
  // Disable the timers
  // We don't reset the timers here
  nrfx_timer_disable(&tim_timeout);
  nrfx_timer_disable(&tim_counter);

  NRF_LOG_DEBUG("Disabled transfer");
}