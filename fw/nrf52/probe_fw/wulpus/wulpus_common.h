#ifndef __WULPUS_COMMON__
#define __WULPUS_COMMON__

#include <stdbool.h>

#include "app_error.h"

#define WP_ERR_RET(func) \
  do { \
    ret_code_t _wp_err_ret_code = func; \
    if (NRF_SUCCESS != _wp_err_ret_code) return _wp_err_ret_code; \
  } while (0);

#endif // __WULPUS_COMMON__