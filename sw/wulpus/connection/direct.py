"""
Copyright (C) 2023 ETH Zurich. All rights reserved.
Author: Cedric Hirschi, ETH Zurich
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0
"""

import asyncio
from itertools import count, takewhile
from typing import Iterator

import bleak as ble
import numpy as np
from wulpus.connection.device import _WulpusConnectionDevice

NORDIC_UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
NORDIC_UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
NORDIC_UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BYTES_PER_XFER = 201
NUMBER_OF_XFERS = 4
MEAS_START_OF_FRAME_MASK = 0xFF

# This is a magic number
# There is a stray byte at index 197 in the received RF data, which is removed
# The exact origin of this byte is unknown
JMP_IDX = 197


def sliced(data: bytes, n: int) -> Iterator[bytes]:
    """
    Slice a bytes object into chunks of n bytes.
    """
    return takewhile(len, (data[i : i + n] for i in count(0, n)))


class WulpusDirect:
    """
    Class representing a direct BLE connection.
    """

    def __init__(self, loop=None):
        self.device = None
        self.client = None

        self.loop = loop

        self.nus = None
        self.rx_char = None

        self.frame_buffer = bytearray()  # Hold 804 bytes (1 frame) of data
        self.frame_ready = None

        self.count_packets = (
            0  # Count the number of packets received (4 packets per frame)
        )

    async def init_async(self):
        self.frame_ready = asyncio.Event()

    def __notification_handler(self, sender, data):
        # Check if it is the first (of the four) BLE packets
        if data[0] == MEAS_START_OF_FRAME_MASK and len(data) == 202:
            # print("<", end="")
            self.count_packets = 1

            self.frame_buffer = bytearray()
            self.frame_buffer.extend(data[1:])

        elif 0 < self.count_packets < NUMBER_OF_XFERS and self.count_packets:
            self.count_packets += 1
            # print("-", end="")

            self.frame_buffer.extend(data)

            if self.count_packets == NUMBER_OF_XFERS:
                # print(">", end=" ")
                self.frame_ready.set()
                self.count_packets = 0

        else:
            # Not a valid frame, pass
            # print("?", end=" ")
            pass

        # # Check if data includes the start of an acquisition (0xFF, 0x00, 0x00, 0x00)
        # if not self.frame_ready.is_set() and b"\xff\x00\x00\x00" in data:
        #     # print(">", end=" ")
        #     self.frame_buffer = bytearray()
        #     self.frame_buffer.extend(data)
        #     self.frame_ready.set()

        # elif self.frame_ready.is_set():
        #     self.frame_buffer.extend(data)

        # self.data_accumulator.extend(data)
        # # Check if we have received the expected amount of data
        # print('Data received:', data)
        # if self.data_accumulator.endswith(b'START\n'):
        # if b'START\n' in self.data_accumulator:
        #     self.data_received_event.set()

    async def get_available(self):
        """
        Get a list of available devices. A device needs a .description attribute for the GUI.
        """

        devices = []

        # print("Scanning for devices...")

        devices = await ble.BleakScanner.discover(return_adv=True)

        # print("Found", len(devices), "devices")

        devices = [devices[key] for key in devices]

        devices = [
            device
            for device in devices
            if NORDIC_UART_SERVICE_UUID.lower() in device[1].service_uuids
        ]

        devices = [
            _WulpusConnectionDevice(device[0], device[1].local_name)
            for device in devices
        ]

        return devices

    async def open(self, device: _WulpusConnectionDevice):
        """
        Open the device connection.
        """
        self.device = device.device
        self.client = ble.BleakClient(self.device)

        try:
            await self.client.connect(timeout=20.0)
        except Exception as e:
            print("Error connecting:", e)
            return False

        try:
            await self.client.start_notify(
                NORDIC_UART_TX_CHAR_UUID, self.__notification_handler
            )
        except Exception as e:
            print("Error starting TX notifications:", e)
            await self.close()
            return False

        self.nus = self.client.services.get_service(NORDIC_UART_SERVICE_UUID)
        if self.nus is None:
            await self.close()
            return False

        self.rx_char = self.nus.get_characteristic(NORDIC_UART_RX_CHAR_UUID)
        if self.rx_char is None:
            await self.close()
            return False

        return True

    async def close(self):
        """
        Close the device connection.
        """

        if self.client is None:
            self.device = None
            self.nus = None
            self.rx_char = None
            return True

        if not self.client.is_connected:
            self.device = None
            self.client = None
            self.nus = None
            self.rx_char = None
            return True

        try:
            await self.client.disconnect()
            self.device = None
            self.client = None
            self.nus = None
            self.rx_char = None
            return True
        except Exception as e:
            print("Error disconnecting:", e)
            return False

    async def send_config(self, conf_bytes_pack: bytes):
        """
        Send a configuration package to the device.
        """
        if self.client is None or not self.client.is_connected:
            return False

        # print("Sending config...")

        # Clear the data accumulator
        self.frame = bytearray()
        self.frame_ready.clear()

        try:
            await self.client.write_gatt_char(
                self.rx_char, conf_bytes_pack, response=False
            )
            # print('Sending config of size', len(conf_bytes_pack), 'with an MTU of', self.rx_char.max_write_without_response_size)
            # for s in sliced(conf_bytes_pack, self.rx_char.max_write_without_response_size):
            #     print('  Sending', len(s), 'bytes')
            #     await self.client.write_gatt_char(self.rx_char, s, response=False)

            # print("Config sent")

            return True
        except Exception as e:
            print("Error sending config:", e)
            return False

    def __get_rf_data_and_info__(self, bytes_arr: bytes):
        # remove stray byte from bytes_arr
        # bytes_arr = bytes_arr[:105] + bytes_arr[106:]

        tx_rx_id = bytes_arr[0]
        # print(tx_rx_id, end=" ")
        acq_nr = np.frombuffer(bytes_arr[1:3], dtype="<u2")[0]
        # print(acq_nr, end=" ")

        # print(len(bytes_arr), tx_rx_id, acq_nr, end=" ")

        # rf_arr = np.frombuffer(bytes_arr[3:803], dtype="<i2")

        # Remove sample at index 197
        rf_arr_bytes = bytes([*bytes_arr[3 : 3 + JMP_IDX], *bytes_arr[JMP_IDX + 4 :]])
        rf_arr = np.frombuffer(rf_arr_bytes, dtype="<i2")

        # print(len(rf_arr), np.mean(rf_arr))
        # rf_arr = rf_arr[:800]

        return rf_arr, acq_nr, tx_rx_id

    async def receive_data(self, acq_length: int):
        if self.client is None or not self.client.is_connected:
            return None

        # print("Receiving data...")

        try:
            # Wait for the frame to be ready
            # print("w", end=" ")
            # print(self.frame_ready)

            await self.frame_ready.wait()
            self.frame_ready.clear()

            response = self.frame_buffer

            # await self.frame_ready.wait()

            # while len(self.frame_buffer) < acq_length * 2 + 5:
            #     print(f"l{len(self.frame_buffer)}", end=" ")
            #     await asyncio.sleep(0.01)

            # print("r", end=" ")
            # print(f"[{self.frame_buffer[:20]}", end="] ")

            # response = self.frame_buffer[: acq_length * 2 + 5]

            # print("Data received:", len(response))

            result = self.__get_rf_data_and_info__(response)

            if result[0].shape[0] == 400:
                return result
            else:
                # print(f"Error: expected length 400, got {result[0].shape[0]}")
                # print(result[0])
                return None

        except Exception as e:
            print("Error receiving:", e)
            return None
