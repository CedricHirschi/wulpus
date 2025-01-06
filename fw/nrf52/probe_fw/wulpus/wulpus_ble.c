#include "wulpus_ble.h"

#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_nus.h"
#include "app_timer.h"

#include "wulpus_config.h"

#define NRF_LOG_MODULE_NAME wp_ble
#define NRF_LOG_LEVEL       4
#define NRF_LOG_INFO_COLOR  0
#define NRF_LOG_DEBUG_COLOR 5
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

#define APP_BLE_CONN_CFG_TAG            1                                                         /**< A tag identifying the SoftDevice BLE configuration. */

#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                                /**< UUID type for the Nordic UART Service (vendor specific). */

#define APP_BLE_OBSERVER_PRIO           3                                                         /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(WULPUS_BLE_MIN_CONN_INTERVAL, UNIT_1_25_MS) /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(WULPUS_BLE_MAX_CONN_INTERVAL, UNIT_1_25_MS) /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                                         /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)                           /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                                     /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                                    /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                                         /**< Number of attempts before giving up the connection parameter negotiation. */


wp_ble_data_handler_t _wp_ble_data_handlers[WULPUS_BLE_MAX_DATA_HANDLERS];
size_t _wp_ble_data_handlers_num = 0;
wp_ble_conn_handler_t _wp_ble_conn_handlers[WULPUS_BLE_MAX_CONN_HANDLERS];
size_t _wp_ble_conn_handlers_num = 0;

BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};


static void _wp_ble_nus_handler(ble_nus_evt_t *p_evt)
{

    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        NRF_LOG_INFO("Received %u bytes from NUS", p_evt->params.rx_data.length);
        NRF_LOG_HEXDUMP_INFO(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);

        for (size_t i = 0; i < _wp_ble_data_handlers_num; i++)
        {
          _wp_ble_data_handlers[i](p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
        }
    }

}

static void _wp_ble_conn_params_handler(ble_conn_params_evt_t *p_evt)
{
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        APP_ERROR_CHECK(sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE));
    }
}

static void _wp_ble_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

static void _wp_ble_adv_evt_handler(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            break;
        case BLE_ADV_EVT_IDLE:
            APP_ERROR_CHECK(sd_power_system_off());
            break;
        default:
            break;
    }
}

static void _wp_ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            APP_ERROR_CHECK(nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle));

            for (size_t i = 0; i < _wp_ble_conn_handlers_num; i++)
            {
              _wp_ble_conn_handlers[i](true);
            }

            break;

        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected");
            // LED indication will be changed when advertising starts.
            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            for (size_t i = 0; i < _wp_ble_conn_handlers_num; i++)
            {
              _wp_ble_conn_handlers[i](false);
            }

            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            APP_ERROR_CHECK(sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys));
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            APP_ERROR_CHECK(sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL));
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            APP_ERROR_CHECK(sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0));
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            APP_ERROR_CHECK(sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            APP_ERROR_CHECK(sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
            break;

        default:
            // No implementation needed.
            break;
    }
}

void _wp_ble_gatt_evt_handler(nrf_ble_gatt_t *p_gatt, nrf_ble_gatt_evt_t const *p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
        NRF_LOG_DEBUG("Data len is set to 0x%X(%d)", m_ble_nus_max_data_len, m_ble_nus_max_data_len);
    }
    NRF_LOG_DEBUG("ATT MTU exchange completed. central 0x%x peripheral 0x%x",
                  p_gatt->att_mtu_desired_central,
                  p_gatt->att_mtu_desired_periph);
}

ret_code_t wp_ble_init(void)
{
  // Initialize Timer
  // ------------------------------------------------------
  WP_ERR_RET(app_timer_init());

  NRF_LOG_DEBUG("Initialized timer");

  // Initialize Bluetooth Stack
  // ------------------------------------------------------
  WP_ERR_RET(nrf_sdh_enable_request());

  // Configure the BLE stack using the default settings.
  // Fetch the start address of the application RAM.
  uint32_t ram_start = 0;
  WP_ERR_RET(nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start));

  // Enable BLE stack.
  WP_ERR_RET(nrf_sdh_ble_enable(&ram_start));

  // Register a handler for BLE events.
  NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, _wp_ble_evt_handler, NULL);

  NRF_LOG_DEBUG("Initialized bluetooth stack");

  // Initialize GAP Parameters
  // ------------------------------------------------------
  ble_gap_conn_params_t   gap_conn_params;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  WP_ERR_RET(sd_ble_gap_device_name_set(&sec_mode,
                                        (const uint8_t *) WULPUS_BLE_DEVICE_NAME,
                                        strlen(WULPUS_BLE_DEVICE_NAME)));

  memset(&gap_conn_params, 0, sizeof(gap_conn_params));

  gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
  gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
  gap_conn_params.slave_latency     = SLAVE_LATENCY;
  gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

  WP_ERR_RET(sd_ble_gap_ppcp_set(&gap_conn_params));

  NRF_LOG_DEBUG("Initialized GAP parameters");

  // Initialize GATT
  // ------------------------------------------------------
  WP_ERR_RET(nrf_ble_gatt_init(&m_gatt, _wp_ble_gatt_evt_handler));

  WP_ERR_RET(nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE));

  NRF_LOG_DEBUG("Initialized GATT");

  // Initialize BLE Services
  // ------------------------------------------------------
  ble_nus_init_t     nus_init;
  nrf_ble_qwr_init_t qwr_init = {0};

  // Initialize Queued Write Module.
  qwr_init.error_handler = _wp_ble_error_handler;

  WP_ERR_RET(nrf_ble_qwr_init(&m_qwr, &qwr_init));

  // Initialize NUS.
  memset(&nus_init, 0, sizeof(nus_init));

  nus_init.data_handler = _wp_ble_nus_handler;

  WP_ERR_RET(ble_nus_init(&m_nus, &nus_init));

  NRF_LOG_DEBUG("Initialized BLE services");

  // Initialize Advertising
  // ------------------------------------------------------
  ble_advertising_init_t init;
  memset(&init, 0, sizeof(init));

  init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
  init.advdata.include_appearance = false;
  init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

  init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
  init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

  init.config.ble_adv_fast_enabled  = true;
  init.config.ble_adv_fast_interval = WULPUS_BLE_ADV_INTERVAL;
  init.config.ble_adv_fast_timeout  = WULPUS_BLE_ADV_DURATION;
  init.evt_handler = _wp_ble_adv_evt_handler;

  WP_ERR_RET(ble_advertising_init(&m_advertising, &init));

  ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);

  NRF_LOG_DEBUG("Initialized advertising");

  // Initialize Connection Parameters
  // ------------------------------------------------------
  ble_conn_params_init_t cp_init;
  memset(&cp_init, 0, sizeof(cp_init));

  cp_init.p_conn_params                  = NULL;
  cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
  cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
  cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
  cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
  cp_init.disconnect_on_fail             = false;
  cp_init.evt_handler                    = _wp_ble_conn_params_handler;
  cp_init.error_handler                  = _wp_ble_error_handler;

  WP_ERR_RET(ble_conn_params_init(&cp_init));

  NRF_LOG_DEBUG("Initialized connection parameters");

  NRF_LOG_INFO("Initialized");

  return NRF_SUCCESS;
}

ret_code_t wp_ble_add_data_handler(wp_ble_data_handler_t handler)
{
  if (_wp_ble_data_handlers_num >= WULPUS_BLE_MAX_DATA_HANDLERS) return NRF_ERROR_CONN_COUNT;

  _wp_ble_data_handlers[_wp_ble_data_handlers_num++] = handler;

  NRF_LOG_DEBUG("Added data handler: %u/%u", _wp_ble_data_handlers_num, WULPUS_BLE_MAX_DATA_HANDLERS);

  return NRF_SUCCESS;
}

ret_code_t wp_ble_add_conn_handler(wp_ble_conn_handler_t handler)
{
  if (_wp_ble_conn_handlers_num >= WULPUS_BLE_MAX_CONN_HANDLERS) return NRF_ERROR_CONN_COUNT;

  _wp_ble_conn_handlers[_wp_ble_conn_handlers_num++] = handler;

  NRF_LOG_DEBUG("Added connection handler: %u/%u", _wp_ble_conn_handlers_num, WULPUS_BLE_MAX_CONN_HANDLERS);

  return NRF_SUCCESS;
}

ret_code_t wp_ble_advertising_start(void)
{
  WP_ERR_RET(ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST));

  NRF_LOG_INFO("Started advertising");

  return NRF_SUCCESS;
}

ret_code_t wp_ble_advertising_stop(void)
{
  NRF_LOG_INFO("Stopped advertising");

  return NRF_SUCCESS;
}

ret_code_t wp_ble_transmit(uint8_t *data, uint16_t length)
{
  ret_code_t err_code;
  do
  {
    err_code = ble_nus_data_send(&m_nus, data, &length, m_conn_handle);
    if (!((err_code == NRF_ERROR_INVALID_STATE) || (err_code == NRF_ERROR_RESOURCES) || (err_code == NRF_ERROR_NOT_FOUND)))
    {
      APP_ERROR_CHECK(err_code);
    }

  } while (err_code == NRF_ERROR_RESOURCES);

  return err_code;
}