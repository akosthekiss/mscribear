function CurrentTimeService(device, currentTimeCharacteristic) {
    this.device = device;
    this.currentTimeCharacteristic = currentTimeCharacteristic;
}

CurrentTimeService.SERVICE_UUID           = '00001805-0000-1000-8000-00805f9b34fb';
CurrentTimeService.CURRENT_TIME_CHAR_UUID = '00002a2b-0000-1000-8000-00805f9b34fb';

// Promise<CurrentTimeService> connect(BluetoothDevice device, optional Log log);
CurrentTimeService.connect = function(device, log={ println: _ => {} }) {
    var currentTimeCharacteristic = null;

    log.println('Getting Current Time service...');
    return device.gatt.getPrimaryService(CurrentTimeService.SERVICE_UUID)
        .then(service => {
            log.println('Getting CTS characteristics...');
            return service.getCharacteristic(CurrentTimeService.CURRENT_TIME_CHAR_UUID);
        })
        .then(characteristic => {
            currentTimeCharacteristic = characteristic;
            log.println('CTS Current Time characteristic obtained');
        })
        .then(_ => {
            return new CurrentTimeService(device, currentTimeCharacteristic);
        });
}

// Promise<Date> readCurrentTime();
CurrentTimeService.prototype.readCurrentTime = function() {
    return this.currentTimeCharacteristic.readValue()
        .then(value => {
            value = dataviewish(value);
            return new Date(Date.UTC(
                // value.getUint16(0, true),
                value.getUint8(0) | (value.getUint8(1) << 8),
                value.getUint8(2) - 1,
                value.getUint8(3),
                value.getUint8(4),
                value.getUint8(5),
                value.getUint8(6)
            ));
        });
}

// Promise<void> writeCurrentTime(Date date);
CurrentTimeService.prototype.writeCurrentTime = function(date) {
    var value = new Uint8Array([
        date.getUTCFullYear() & 0xff, date.getUTCFullYear() >> 8,
        date.getUTCMonth() + 1,
        date.getUTCDate(),
        date.getUTCHours(),
        date.getUTCMinutes(),
        date.getUTCSeconds(),
        0, 0, 0
    ]);
    return this.currentTimeCharacteristic.writeValue(value);
}
