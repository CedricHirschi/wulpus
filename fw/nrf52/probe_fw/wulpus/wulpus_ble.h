#ifndef __WULPUS_BLE__
#define __WULPUS_BLE__

#include "wulpus_common.h"

typedef void (*wp_ble_data_handler_t)(uint8_t const *, uint16_t);
typedef void (*wp_ble_conn_handler_t)(bool);

ret_code_t wp_ble_init(void);

ret_code_t wp_ble_add_data_handler(wp_ble_data_handler_t handler);
ret_code_t wp_ble_add_conn_handler(wp_ble_conn_handler_t handler);

ret_code_t wp_ble_advertising_start(void);
ret_code_t wp_ble_advertising_stop(void);

ret_code_t wp_ble_transmit(uint8_t *data, uint16_t length);

#endif // __WULPUS_BLE__