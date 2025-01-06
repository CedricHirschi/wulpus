#ifndef __WULPUS_CONFIG__
#define __WULPUS_CONFIG__


//    ____  _     _____ 
//   | __ )| |   | ____|
//   |  _ \| |   |  _|  
//   | |_) | |___| |___ 
//   |____/|_____|_____|
//                      
#define WULPUS_BLE_DEVICE_NAME        "WULPUS_PROBE_19"     /**< Name of device. Will be included in the advertising data. */
#define WULPUS_BLE_ADV_INTERVAL       64                    /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define WULPUS_BLE_ADV_DURATION       18000                 /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define WULPUS_BLE_MIN_CONN_INTERVAL  20                    /**< Minimum acceptable connection interval (20 ms), Connection interval uses 1.25 ms units. */
#define WULPUS_BLE_MAX_CONN_INTERVAL  75                    /**< Maximum acceptable connection interval (75 ms), Connection interval uses 1.25 ms units. */
#define WULPUS_BLE_MAX_DATA_HANDLERS  5                     /**< Maximum amount of data handlers. */
#define WULPUS_BLE_MAX_CONN_HANDLERS  5                     /**< Maximum amount of connection handlers. */


//     ____ ____ ___ ___  
//    / ___|  _ \_ _/ _ \ 
//   | |  _| |_) | | | | |
//   | |_| |  __/| | |_| |
//    \____|_|  |___\___/ 
//                        
#define WULPUS_GPIO_NUM_LED           17 /**< GPIO number of the on-board LED. */
#define WULPUS_GPIO_NUM_BLE_CONN      18 /**< GPIO number of the BLE connected output. */
#define WULPUS_GPIO_NUM_DATA_READY    13 /**< GPIO number of the data ready input. */
#define WULPUS_GPIO_MAX_DATA_HANDLERS 5  /**< Maximum amount of data ready handlers. */
#define WULPUS_GPIO_LED_ENABLE        1  /**< If LED is enabled. */
#define WULPUS_GPIO_LED_INVERT        1  /**< If LED is inverted (nRF52DK): 1, 0 (WULPUS) else. */


//    ____  ____ ___ 
//   / ___||  _ \_ _|
//   \___ \| |_) | | 
//    ___) |  __/| | 
//   |____/|_|  |___|
//                   
#define WULPUS_SPI_NUM_CS           7
#define WULPUS_SPI_NUM_SCK          8
#define WULPUS_SPI_NUM_MISO         9
#define WULPUS_SPI_NUM_MOSI         10
#define WULPUS_SPI_PACKET_INTERVAL  300


//    ____  ____ ___ 
//   |  _ \|  _ \_ _|
//   | |_) | |_) | | 
//   |  __/|  __/| | 
//   |_|   |_|  |___|
//                   
#define WULPUS_PPI_MAX_END_HANDLERS 5

//    __  __    _    ___ _   _ 
//   |  \/  |  / \  |_ _| \ | |
//   | |\/| | / _ \  | ||  \| |
//   | |  | |/ ___ \ | || |\  |
//   |_|  |_/_/   \_\___|_| \_|
//                             
#define WULPUS_NUMBER_OF_XFERS      4
#define WULPUS_BYTES_PER_XFER       201
#define WULPUS_NUM_BUFFERED_FRAMES  35

#endif // __WULPUS_CONFIG__
