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

import numpy as np
import asyncio
from itertools import count, takewhile
from typing import Iterator

import bleak as ble
import bleak.backends.device as ble_device
import bleak.backends.characteristic as ble_char


NORDIC_UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
NORDIC_UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
NORDIC_UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"



def sliced(data: bytes, n: int) -> Iterator[bytes]:
    """
    Slice a bytes object into chunks of n bytes.
    """
    return takewhile(len, (data[i:i+n] for i in count(0, n)))


class WulpusDirect:
    """
    Class representing a direct BLE connection.
    """


    def __init__(self):
        
        self.device = None
        self.client = None

        self.nus = None
        self.rx_char = None


    async def get_available(self):
        """
        Get a list of available devices. A device needs a .description attribute for the GUI.
        """

        devices = []

        devices = await ble.BleakScanner.discover()

        devices = [device for device in devices if NORDIC_UART_SERVICE_UUID.lower() in device.details['props'].get('UUIDs', [])]
            
        return devices


    # def __rx_handler(self, gatt_char: ble_char.BleakGATTCharacteristic, data: bytearray):
    #     print('Received data: ', data)


    async def open(self, device:ble_device.BLEDevice):
        """
        Open the device connection.
        """
        self.device = device
        self.client = ble.BleakClient(device)

        try:
            await self.client.connect(timeout=20.0)
        except Exception as e:
            print('Error connecting:', e)
            return False
        
        # try:
        #     await self.client.start_notify(NORDIC_UART_TX_CHAR_UUID, self.__rx_handler)
        # except Exception as e:
        #     print('Error starting TX notifications:', e)
        #     await self.close()
        #     return False
        
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
            print('Error disconnecting:', e)
            return False
    
    
    async def send_config(self, conf_bytes_pack:bytes):
        """
        Send a configuration package to the device.
        """
        if self.client is None or not self.client.is_connected:
            return False

        try:
            for s in sliced(conf_bytes_pack, self.rx_char.max_write_without_response_size):
                await self.client.write_gatt_char(self.rx_char, s, response=False)
            return True
        except Exception as e:
            print('Error sending config:', e)
            return False
    

    def __get_rf_data_and_info__(self, bytes_arr:bytes):
    
        rf_arr = np.frombuffer(bytes_arr[7:], dtype='<i2')    
        tx_rx_id = bytes_arr[4]
        acq_nr = np.frombuffer(bytes_arr[5:7], dtype='<u2')[0]

        return rf_arr, acq_nr, tx_rx_id
    
    
    async def receive_data(self):
        if self.client is None or not self.client.is_connected:
            return None

        data_accumulator = bytearray()
        data_received_event = asyncio.Event()

        def notification_handler(sender, data):
            data_accumulator.extend(data)
            # Check if we have received the expected amount of data
            if data_accumulator.endswith(b'START\n'):
                data_received_event.set()

        try:
            await self.client.start_notify(NORDIC_UART_TX_CHAR_UUID, notification_handler)
            await data_received_event.wait()
            await self.client.stop_notify(NORDIC_UART_TX_CHAR_UUID)

            if len(data_accumulator) == 0:
                return None
            elif data_accumulator[-6:] == b'START\n':
                # Now process the accumulated data
                # Assuming you know how much data to expect after 'START\n'
                expected_data_length = self.acq_length * 2 + 7
                while len(data_accumulator) < expected_data_length:
                    # Wait for more data (you may need to adjust your logic here)
                    await asyncio.sleep(0.1)
                response = data_accumulator[-expected_data_length:]
                return self.__get_rf_data_and_info__(response)
            else:
                return None
        except Exception as e:
            print('Error receiving:', e)
            return None