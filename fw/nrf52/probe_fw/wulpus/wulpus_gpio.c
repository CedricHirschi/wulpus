#include "wulpus_gpio.h"

#include "nrfx_gpiote.h"

#include "wulpus_config.h"

#define NRF_LOG_MODULE_NAME wp_gpio
#define NRF_LOG_LEVEL       4
#define NRF_LOG_INFO_COLOR  0
#define NRF_LOG_DEBUG_COLOR 5
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


wp_gpio_data_handler_t _wp_gpio_data_handlers[WULPUS_GPIO_MAX_DATA_HANDLERS];
size_t _wp_gpio_data_handlers_num = 0;


void _wp_gpio_data_ready_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  NRF_LOG_DEBUG("Data ready handler called: %u callbacks", _wp_gpio_data_handlers_num);

  for (size_t i = 0; i < _wp_gpio_data_handlers_num; i++)
  {
    _wp_gpio_data_handlers[i]();
  }
}

ret_code_t wp_gpio_init(void)
{
  WP_ERR_RET(nrfx_gpiote_init());
  
#if WULPUS_GPIO_LED_ENABLE
  // Initialize the on-board LED
  nrfx_gpiote_out_config_t out_config = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false);
  WP_ERR_RET(nrfx_gpiote_out_init(WULPUS_GPIO_NUM_LED, &out_config));
  WULPUS_GPIO_LED_INVERT ? nrfx_gpiote_out_set(WULPUS_GPIO_NUM_LED) : nrfx_gpiote_out_clear(WULPUS_GPIO_NUM_LED);
  
  NRF_LOG_DEBUG("Initialized LED%s", WULPUS_GPIO_LED_INVERT ? " (inverted)" : "");
#endif

  // Initialize the BLE connected output
  nrfx_gpiote_out_config_t out_config_ble_ready = NRFX_GPIOTE_CONFIG_OUT_SIMPLE(false);
  WP_ERR_RET(nrfx_gpiote_out_init(WULPUS_GPIO_NUM_BLE_CONN, &out_config_ble_ready));
  nrfx_gpiote_out_clear(WULPUS_GPIO_NUM_BLE_CONN);

  NRF_LOG_DEBUG("Initialized BLE connected output");

  // Initialize the data ready input
  nrfx_gpiote_in_config_t in_config_data_ready = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
  in_config_data_ready.pull = NRF_GPIO_PIN_NOPULL;
  WP_ERR_RET(nrfx_gpiote_in_init(WULPUS_GPIO_NUM_DATA_READY, &in_config_data_ready, _wp_gpio_data_ready_handler));
  nrfx_gpiote_in_event_enable(WULPUS_GPIO_NUM_DATA_READY, true);

  NRF_LOG_DEBUG("Initialized data ready input");

  NRF_LOG_INFO("Initialized");

  return NRF_SUCCESS;
}

ret_code_t wp_gpio_add_data_handler(wp_gpio_data_handler_t handler)
{
  if (_wp_gpio_data_handlers_num >= WULPUS_GPIO_MAX_DATA_HANDLERS) return NRF_ERROR_CONN_COUNT;

  _wp_gpio_data_handlers[_wp_gpio_data_handlers_num++] = handler;

  NRF_LOG_DEBUG("Added data handler: %u/%u", _wp_gpio_data_handlers_num, WULPUS_GPIO_MAX_DATA_HANDLERS);

  return NRF_SUCCESS;
}

void wp_gpio_led_indicate(bool on)
{
  if (on ^ WULPUS_GPIO_LED_INVERT)
  {
    nrfx_gpiote_out_set(WULPUS_GPIO_NUM_LED);
  }
  else
  {
    nrfx_gpiote_out_clear(WULPUS_GPIO_NUM_LED);
  }

  NRF_LOG_DEBUG("LED turned %s", on ? "on" : "off");
}

void wp_gpio_led_toggle(void)
{
  nrfx_gpiote_out_toggle(WULPUS_GPIO_NUM_LED);

  NRF_LOG_DEBUG("LED toggled");
}

void wp_gpio_ble_conn_indicate(bool ready)
{
  if (ready)
  {
    nrfx_gpiote_out_set(WULPUS_GPIO_NUM_BLE_CONN);
  }
  else
  {
    nrfx_gpiote_out_clear(WULPUS_GPIO_NUM_BLE_CONN);
  }

  NRF_LOG_DEBUG("BLE indicated %s", ready ? "ready" : "not ready");
}