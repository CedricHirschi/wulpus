import asyncio
import threading

from wulpus.connection.direct import WulpusDirect
from wulpus.connection.dongle import WulpusDongle

ACQ_LENGTH_SAMPLES = 400


class WulpusConnection:
    def __init__(self, type="dongle"):
        self.type = type

        self.loop = asyncio.new_event_loop()

        if type == "dongle":
            self.__connection__ = WulpusDongle()
        elif type == "direct":
            self.__connection__ = WulpusDirect(self.loop)
        else:
            raise TypeError(f"Unknown connection type: {type}")

        self.acq_length = ACQ_LENGTH_SAMPLES

        self.__do_get_available__ = False
        self.__available__ = None
        self.__do_open__ = False
        self.__opened__ = None
        self.__do_close__ = False
        self.__closed__ = None
        self.__do_send_config__ = False
        self.__config_sent__ = None
        self.__do_receive_data__ = False
        self.__data_received__ = None

        self.thread = threading.Thread(target=self.__run_loop)
        self.thread.start()

        if type == "direct":
            asyncio.run_coroutine_threadsafe(
                self.__connection__.init_async(), self.loop
            )

    def __run_loop(self):
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def get_available(self) -> list:
        future = asyncio.run_coroutine_threadsafe(
            self.__connection__.get_available(), self.loop
        )
        return future.result()

    def open(self, device) -> bool:
        self.device = device
        future = asyncio.run_coroutine_threadsafe(
            self.__connection__.open(device), self.loop
        )
        return future.result()

    def close(self) -> bool:
        future = asyncio.run_coroutine_threadsafe(
            self.__connection__.close(), self.loop
        )
        return future.result()

    def send_config(self, conf_bytes_pack: bytes) -> bool:
        self.conf_bytes_pack = conf_bytes_pack
        future = asyncio.run_coroutine_threadsafe(
            self.__connection__.send_config(conf_bytes_pack), self.loop
        )
        result = future.result()
        return result

    def receive_data(self) -> bytes:
        future = asyncio.run_coroutine_threadsafe(
            self.__connection__.receive_data(self.acq_length), self.loop
        )
        return future.result()

    def __del__(self):
        self.loop.call_soon_threadsafe(self.loop.stop)
        self.thread.join()
