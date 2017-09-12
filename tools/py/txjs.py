# Copyright (c) 2017 Akos Kiss.
#
# Licensed under the BSD 3-Clause License
# <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
# This file may not be copied, modified, or distributed except
# according to those terms.

from __future__ import print_function

import argparse
import datetime
# import logging
import sys
import traceback

import Adafruit_BluefruitLE as BLE
from Adafruit_BluefruitLE.services import UART
from xmodem import XMODEM

from current_time_service import CurrentTimeService


def main():
    # Process command line arguments.
    parser = argparse.ArgumentParser(description='TXJS: XMODEM over UART over BLE')
    parser.add_argument('file', metavar='FILE', help='script file to upload')

    args = parser.parse_args()

    # Get the BLE provider for the current platform and initialize BLE system.
    ble = BLE.get_provider()
    ble.initialize()

    # To be executed in a background thread while OS main loop processes async
    # BLE events
    def ble_main():
        # Do everything in a try/except to ensure that we can get a proper
        # backtrace in case of an error. (The BLE main loop implementation can
        # conceal it.)
        try:
            # Clear any cached data because all providers have issues with
            # caching data and it going stale.
            ble.clear_cached_data()

            # Get the first available BLE network adapter and make sure it's
            # powered on.
            adapter = ble.get_default_adapter()
            adapter.power_on()
            print('Using adapter {0}'.format(adapter.name))

            # Disconnect any currently connected UART devices for cleaning up
            # and starting from a fresh state.
            print('Disconnecting any connected UART devices...')
            UART.disconnect_devices()

            # Scan for devices.
            print('Searching for a device with UART service...')
            try:
                adapter.start_scan()
                # Search for the first UART device found (will time out after
                # 60 seconds).
                device = UART.find_device()
                if device is None:
                    raise RuntimeError('Failed to find device!')
            finally:
                # Make sure scanning is stopped before exiting.
                adapter.stop_scan()

            print('Connecting to device {0} [{1}]...'.format(device.name, device.id))
            # Will time out after 60 seconds.
            device.connect()

            # Once connected do everything else in a try/finally to make sure
            # the device is disconnected when done.
            try:
                # Wait for service discoveries to complete. Each will time out
                # after 60 seconds.
                print('Discovering services...')
                UART.discover(device)
                CurrentTimeService.discover(device)

                # Once service discovery is complete, create instances of the
                # services and start interacting with them.
                uart = UART(device)
                cts = CurrentTimeService(device)

                # Setting current time should come first because it seems that
                # after the UART transfer the disconnect happens too fast and
                # the write to current time characteristic cannot finish.
                print('Resetting RTC...')

                print('RTC was {0:%a, %d %b %Y %H:%M:%S GMT}'.format(cts.current_time))
                now = datetime.datetime.utcnow()
                cts.current_time = now
                print('RTC reset to {0:%a, %d %b %Y %H:%M:%S GMT}'.format(now))

                # Now transfer the file with XMODEM over UART.
                print('Transfering file {0} with XMODEM...'.format(args.file))

                # Write file content to the TX characteristic in 20 bytes
                # chunks. Note that this implementation ignores the timeout.
                def send_bytes(data, timeout=1):
                    size = len(data)

                    while data:
                        chunk = data[:20]
                        data = data[20:]
                        uart.write(chunk)

                        print('.', end='')
                        sys.stdout.flush()

                    print('o', end='')
                    sys.stdout.flush()

                    return size

                # Note that this ignores the size.
                def recv_bytes(size, timeout=1):
                    return uart.read(timeout_sec=timeout) or None

                # logging.basicConfig(format='%(message)s')
                # logging.getLogger('xmodem.XMODEM').setLevel('DEBUG')

                modem = XMODEM(recv_bytes, send_bytes)
                with open(args.file, 'rb') as f:
                    status = modem.send(f)
                    print()
                    print('Transfer {0}'.format('successful' if status else 'failed'))

            finally:
                # Make sure device is disconnected on exit.
                device.disconnect()

        except:
            traceback.print_exc()
            raise

    # Start the OS main loop to process BLE events, and run ble_main in a
    # background thread.
    ble.run_mainloop_with(ble_main)


if __name__ == '__main__':
    main()
