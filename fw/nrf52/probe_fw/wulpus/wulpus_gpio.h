#ifndef __WULPUS_GPIO__
#define __WULPUS_GPIO__

#include "wulpus_common.h"

#include "nrfx_gpiote.h"

typedef void (*wp_gpio_data_handler_t)(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

ret_code_t wp_gpio_init(void);

ret_code_t wp_gpio_add_data_handler(wp_gpio_data_handler_t handler);

void wp_gpio_led_indicate(bool on);
void wp_gpio_led_toggle(void);
void wp_gpio_ble_conn_indicate(bool ready);

#endif // __WULPUS_GPIO__