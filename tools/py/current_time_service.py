# Copyright (c) 2017 Akos Kiss.
#
# Licensed under the BSD 3-Clause License
# <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
# This file may not be copied, modified, or distributed except
# according to those terms.

import datetime
import struct
import uuid

import Adafruit_BluefruitLE as BLE
from Adafruit_BluefruitLE.services.servicebase import ServiceBase


CURRENT_TIME_SERVICE_UUID = uuid.UUID('00001805-0000-1000-8000-00805F9B34FB')
CURRENT_TIME_CHAR_UUID    = uuid.UUID('00002A2B-0000-1000-8000-00805F9B34FB')


class CurrentTimeService(ServiceBase):

    # Configure expected services and characteristics
    ADVERTISED = [CURRENT_TIME_SERVICE_UUID]
    SERVICES = [CURRENT_TIME_SERVICE_UUID]
    CHARACTERISTICS = [CURRENT_TIME_CHAR_UUID]

    def __init__(self, device):
        self._service = device.find_service(CURRENT_TIME_SERVICE_UUID)
        self._current_time = self._service.find_characteristic(CURRENT_TIME_CHAR_UUID)

    @property
    def current_time(self):
        data = self._current_time.read_value()
        year, month, day, hour, minute, second, _, _, _ = struct.unpack('<HBBBBBBBB', data)
        return datetime.datetime(year, month, day, hour, minute, second)

    @current_time.setter
    def current_time(self, value):
        data = struct.pack('<HBBBBBBBB', value.year, value.month, value.day, value.hour, value.minute, value.second, 0, 0, 0)
        self._current_time.write_value(data)
