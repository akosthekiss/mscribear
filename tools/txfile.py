from __future__ import print_function

# import logging
# import time
import sys

import Adafruit_BluefruitLE as BLE
from Adafruit_BluefruitLE.services import UART
from xmodem import XMODEM


# Get the BLE provider for the current platform.
ble = BLE.get_provider()


# Main function implements the program logic so it can run in a background
# thread.  Most platforms require the main thread to handle GUI events and other
# asyncronous events like BLE actions.  All of the threading logic is taken care
# of automatically though and you just need to provide a main function that uses
# the BLE provider.
def main():
    # Clear any cached data because both bluez and CoreBluetooth have issues with
    # caching data and it going stale.
    ble.clear_cached_data()

    # Get the first available BLE network adapter and make sure it's powered on.
    adapter = ble.get_default_adapter()
    adapter.power_on()
    print('Using adapter: {0}'.format(adapter.name))

    # Disconnect any currently connected UART devices.  Good for cleaning up and
    # starting from a fresh state.
    print('Disconnecting any connected UART devices...')
    UART.disconnect_devices()

    # Scan for UART devices.
    print('Searching for UART device...')
    try:
        adapter.start_scan()
        # Search for the first UART device found (will time out after 60 seconds
        # but you can specify an optional timeout_sec parameter to change it).
        device = UART.find_device()
        if device is None:
            raise RuntimeError('Failed to find UART device!')
    finally:
        # Make sure scanning is stopped before exiting.
        adapter.stop_scan()

    print('Connecting to device {0} [{1}]...'.format(device.name, device.id))
    device.connect()  # Will time out after 60 seconds, specify timeout_sec parameter
                      # to change the timeout.

    # Once connected do everything else in a try/finally to make sure the device
    # is disconnected when done.
    try:
        # Wait for service discovery to complete for the UART service.  Will
        # time out after 60 seconds (specify timeout_sec parameter to override).
        print('Discovering services...')
        UART.discover(device)

        # Once service discovery is complete create an instance of the service
        # and start interacting with it.
        uart = UART(device)

        print('Transfering file %s with XMODEM...' % sys.argv[1])

        def send_bytes(data, timeout=1):
            size = len(data)

            # Write file content to the TX characteristic in 20 bytes chunks.
            # Note that this ignores the timeout.
            while data:
                chunk = data[:20]
                data = data[20:]
                uart.write(chunk)

                # print('send chunk: %r' % chunk)
                print('.', end='')
                sys.stdout.flush()
                # time.sleep(.1)

            print('o', end='')
            sys.stdout.flush()

            return size

        def recv_bytes(size, timeout=1):
            # Note that this ignores the size.
            return uart.read(timeout_sec=timeout) or None

        # logging.basicConfig(format='%(message)s')
        # logging.getLogger('xmodem.XMODEM').setLevel('DEBUG')

        modem = XMODEM(recv_bytes, send_bytes)
        with open(sys.argv[1], 'rb') as f:
            status = modem.send(f)
            print()
            print('Transfer {0}'.format('successful' if status else 'failed'))

    finally:
        # Make sure device is disconnected on exit.
        device.disconnect()


if __name__ == '__main__':
    # Initialize the BLE system.  MUST be called before other BLE calls!
    ble.initialize()

    # Start the mainloop to process BLE events, and run the provided function in
    # a background thread.  When the provided main function stops running, returns
    # an integer status code, or throws an error the program will exit.
    ble.run_mainloop_with(main)
