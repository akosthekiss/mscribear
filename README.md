# MScriBear: JerryScript Experiment with ARM mbed OS on RedBear BLE Nano v2

[![Build Status](https://travis-ci.org/akosthekiss/mscribear.svg?branch=master)](https://travis-ci.org/akosthekiss/mscribear)

MScriBear is an application experiment combining the lightweight JavaScript engine
[JerryScript](https://github.com/jerryscript-project/jerryscript) with the
[ARM mbed OS](https://github.com/ARMmbed/mbed-os) platform, especially targeting the
[RedBear BLE Nano v2](https://github.com/redbear/nRF5x/tree/master/nRF52832) device.

The project aims at making use of the Bluetooth Low Energy capabilities of the
BLE Nano v2 device and implements:
- an UART service, enabling over-the-air JavaScript upload with the help of the
  XMODEM-CRC protocol, and
- the Current Time service with support for writing the Current Time characteristic,
  enabling the OTA setting of the RTC (mostly because the device restarts the RTC
  from Unix epoch at each reset).

After each successful XMODEM file transfer, the uploaded content is parsed and
executed by the JavaScript engine (in a clean ES5.1 runtime environment, first
terminating the previously uploaded and still running JS program, if any).


## Building and Flashing the Project

The project can be built with the [ARM mbed Command Line Interface](https://github.com/ARMmbed/mbed-cli).
Setting it up is described in detail in that repository but basically it requires
the Python 2-based `mbed` command line tool and a compiler tool chain. So:
- Install Python 2 and `pip`. (The steps for installing Python are platform dependent.
  Installation steps for `pip` are described in its [documentation](https://pip.pypa.io/en/stable/).
  But both may already by available on your system.)
- Install the mbed CLI by `pip install mbed-cli`.
- Download and install the
  [GNU Embedded Toolchain for ARM](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads).

Once the basic tools are installed, clone this project and perform the following
steps in the root directory of the project:
- Deploy the project, i.e., let mbed CLI download the (C++) library dependencies
  by `mbed deploy`.
- Install the extra Python dependencies of the mbed OS (just downloaded by the
  previous deploy step) by `pip install -r mbed-os/requirements.txt`.

As a final step, the binary can actually be built:
- Issue `mbed compile` to create the final ELF and HEX files under
  `BUILD/RBLAB_BLENANO2/GCC_ARM/`.

If you are using a DAPLink companion board together with the BLE Nano v2 device,
flashing the binary is as easy as copying the built file to a mounted drive.
- On macOS, this will probably be `cp ./BUILD/RBLAB_BLENANO2/GCC_ARM/mscribear.hex /Volumes/DAPLINK/`.


## Uploading JS Files OTA

The project comes with two tools that have similar functionality and can upload
JS files over BLE to the device to be executed by the JerryScript engine.

### Uploading from Command Line

The command line tool is written in Python and is available at `tools/py/txjs.py`.
The upload tool has its own dependencies on top of the already installed ones, so:
- Install the mandatory dependencies by `pip install -r tools/py/requirements.txt`.
- Perform the platform-specific installation steps of the
  [Adafruit-BluefruitLE](https://github.com/adafruit/Adafruit_Python_BluefruitLE)
  Python package (i.e., either ensure that a proper version of the native BlueZ
  package is installed, on Linux, or ensure that the [PyObjC](http://pythonhosted.org/pyobjc/)
  Python package is installed, on macOS).

Once everything is in place, you can use the Python script to upload a JS program
using XMODEM-CRC protocol implemented over BLE UART service. Simple examples are
available in the `examples/` directory. E.g.,

```sh
python tools/py/txjs.py examples/blink.js
```

### Uploading from Browser

The HTML application has no external dependencies but a browser supporting the
[Web Bluetooth](https://webbluetoothcg.github.io/web-bluetooth/) specification.
As of now, only [Google Chrome](https://www.google.com/chrome/) implements all the
required features, but [Servo](https://servo.org) is also ramping up.

To upload a JS program from the browser, open `tools/html/txjs.html`, edit the
code within the text area or load a file from the local file system by pressing
`Open`, and finally upload the code with the `TX` button.


## Compatibility

* The project is being developed on macOS Sierra 10.12.
* The project is also build-tested on Ubuntu 14.04 by Travis CI.


## Copyright and Licensing

Licensed under the BSD 3-Clause [License](LICENSE.md).
