I found several material differences. Three can independently prevent normal operation.

1. Wrong first BLE packet offset

    The original sends the first notification from frame byte 0. The rewrite starts from byte 1 while still sending 202 bytes: [rewrite main.c](probe_fw/main.c:128).

    The MSP430 puts the `0xFF` frame marker at byte 0, and the dongle requires that marker: [dongle handler](peripheral/US_probe_dongle_firmware/us_ble.c:240).

    Consequently, the dongle will not recognize rewritten frames. The `+ 1` is incorrect.

2. SPI ring-buffer selection is overwritten

    The rewrite:

    - Selects a ring-buffer slot with `wp_spi_set_buffer()`.
    - Then calls `wp_spi_init_reception()`: [main.c](probe_fw/main.c:62).
    - That function constructs the SPI descriptor using `_wp_spi_rx_buffer`, which always points to the beginning of the entire buffer: [wulpus_spi.c](probe_fw/wulpus/wulpus_spi.c:82).

    The SPI driver reprograms `RXD.PTR` from that descriptor, undoing the selected slot. Data therefore repeatedly lands in slot zero while the ring-buffer head advances through other, zero/stale slots.

    The original prepares the repeated transfer once, then updates `RXD.PTR` immediately before starting the timers: [original main.c](ble_peripheral/US_probe_nRF52_firmware/main.c:131).

3. BLE-ready GPIO has changed meaning

    The original asserts GPIO 25 only after:

    - BLE connection
    - a two-second negotiation delay
    - receipt of the host configuration

    See [original main.c](ble_peripheral/US_probe_nRF52_firmware/main.c:189).

    The rewrite asserts it immediately upon the GAP connection event: [rewrite main.c](probe_fw/main.c:164).

    The MSP430 interprets that pin as permission to request its configuration: [MSP430 main.c](fw/msp430/wulpus_msp430_firmware/main.c:157). It can therefore start SPI traffic before the configuration, PHY negotiation, service discovery, or NUS notification enablement is complete.

4. Expected BLE errors become fatal

    `wp_ble_transmit()` returns `NRF_ERROR_INVALID_STATE` and `NRF_ERROR_NOT_FOUND`, even though it internally considers them ignorable: [wulpus_ble.c](probe_fw/wulpus/wulpus_ble.c:337).

    The caller then passes that return value to `APP_ERROR_CHECK`: [main.c](probe_fw/main.c:128). Early frames—especially those caused by the premature ready GPIO—can therefore fault/reset the firmware before notifications are enabled.

    The original explicitly ignores those errors: [original us_ble.c](ble_peripheral/US_probe_nRF52_firmware/us_ble.c:530).

5. Default device name is incompatible with the dongle

    The rewrite advertises `WULPUS_PROBE_19`: [wulpus_config.h](probe_fw/wulpus/wulpus_config.h:11).

    The stock dongle filters specifically for `WULPUS_PROBE_0`: [dongle us_ble.c](peripheral/US_probe_dongle_firmware/us_ble.c:84).

    Unless the dongle was built for probe 19, it will never connect.

6. BLE performance profile was reduced

    The original requests:

    - 7.5 ms connection interval
    - 2 Mbit PHY
    - early parameter negotiation

    The rewrite uses 20–75 ms intervals and does not initiate the 2 Mbit PHY switch. Compare [original settings](ble_peripheral/US_probe_nRF52_firmware/us_ble.c:88) and [rewrite settings](probe_fw/wulpus/wulpus_config.h:14).

The receive buffer also dropped from 35 frames to 5. This substantially increases overflow risk.

Additional differences include unsynchronized immediate SPI transfers for configuration/restart, an unbounded copy into the rewrite’s 201-byte TX buffer, and data-ready interrupts on both GPIO edges instead of rising edges only.

The first fixes to prioritize are the BLE packet offset, SPI receive-buffer ordering, ready-GPIO handshake, and transmit error propagation. No files were changed.