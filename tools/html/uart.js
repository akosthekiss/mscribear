function UART(log) {
    this.log = log;

    this.device = null;
    this.txCharacteristic = null;
    this.rxCharacteristic = null;
}

UART.SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
UART.TX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';
UART.RX_CHAR_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';

UART.prototype.connect = function(onReceived) {
    var uart = this;
    var log = uart.log ? uart.log : { println: _ => {} };

    log.println('Requesting a device with UART service...');
    return navigator.bluetooth.requestDevice({filters: [{services: [UART.SERVICE_UUID]}]})
        .then(device => {
            function onDisconnected(event) {
                log.println('Device disconnected');

                if (uart.device)
                    uart.device.removeEventListener('gattserverdisconnected', onDisconnected);
                if (uart.rxCharacteristic)
                    uart.rxCharacteristic.removeEventListener('characteristicvaluechanged', onReceived);

                uart.txCharacteristic = null;
                uart.rxCharacteristic = null;
            };

            uart.device = device;
            uart.device.addEventListener('gattserverdisconnected', onDisconnected);

            log.println('Connecting to device ' + uart.device.name + ' [' + uart.device.id + ']...');
            return uart.device.gatt.connect();
        })
        .then(server => {
            log.println('Getting UART service...');
            return server.getPrimaryService(UART.SERVICE_UUID);
        })
        .then(service => {
            log.println('Getting UART characteristics...');
            return Promise.all([
                service.getCharacteristic(UART.RX_CHAR_UUID)
                    .then(characteristic => {
                        uart.rxCharacteristic = characteristic;
                        log.println('UART RX characteristic obtained');
                        return uart.rxCharacteristic.startNotifications()
                            .then(_ => {
                                log.println('UART RX notifications started');
                                uart.rxCharacteristic.addEventListener('characteristicvaluechanged', onReceived);
                            });
                    }),
                service.getCharacteristic(UART.TX_CHAR_UUID)
                    .then(characteristic => {
                        uart.txCharacteristic = characteristic;
                        log.println('UART TX characteristic obtained');
                    }),
            ]);
        });
}

UART.prototype.disconnect = function() {
    if (this.device && this.device.gatt.connected) {
        log.println('Disconnecting from device...');
        this.device.gatt.disconnect();
    }
}
