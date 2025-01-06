#ifndef __WULPUS_PPI__
#define __WULPUS_PPI__

#include "nrf_drv_spi.h"

#include "wulpus_common.h"

typedef void (*wp_ppi_end_handler_t)(void);

ret_code_t wp_ppi_init(const nrf_drv_spi_t *spi_instance);

ret_code_t wp_ppi_add_end_handler(wp_ppi_end_handler_t handler);

void wp_ppi_start_transfer(void);
void wp_ppi_stop_transfer(void);

#endif // __WULPUS_PPI__