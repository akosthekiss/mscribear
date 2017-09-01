var log = new Log(document.querySelector('#log'));
var device = null;
var cts = null;
var uart = null;

function disconnect() {
    if (device && device.gatt.connected) {
        log.println('Disconnecting from device...');
        device.gatt.disconnect();
    }
}

function xmodemSend(buf) {
    function loop(i=0) {
        if (i < buf.byteLength) {
            return uart.txCharacteristic.writeValue(buf.subarray(i, i + 20))
                .then(_ => {
                    log.print('.');
                    return loop(i + 20);
                });
        }
    }
    loop()
        .then(_ => {
            log.print('o');
        })
        .catch(error => {
            log.println();
            log.println('UART TX error: ' + error);
        });
}

function xmodemSuccess() {
    log.println();
    log.println('Transfer successful');
    disconnect();
}

function xmodemError() {
    log.println();
    log.println('Transfer failed');
    disconnect();
}

function txjsStart(content) {
    log.clear();
    log.println('Requesting a device with UART service...');
    return navigator.bluetooth.requestDevice({filters: [{services: [UART.SERVICE_UUID]}],
                                              optionalServices: [CurrentTimeService.SERVICE_UUID]})
        .then(_device => {
            device = _device;
            log.println('Connecting to device ' + device.name + ' [' + device.id + ']...');
            return device.gatt.connect();
        })
        .then(_ => {
            return CurrentTimeService.connect(device, log);
        })
        .then(_cts => {
            cts = _cts;
            log.println('Resetting RTC...');
            return cts.readCurrentTime();
        })
        .then(date => {
            log.println('RTC was ' + date.toUTCString());
            var now = new Date();
            log.println('RTC reset to ' + now.toUTCString());
            return cts.writeCurrentTime(now);
        })
        .then(_ => {
            var xmodem = new XMODEMSender(content, xmodemSend, xmodemSuccess, xmodemError);
            return UART.connect(device, event => xmodem.dataReceived(event.target.value), log);
        })
        .then(_uart => {
            uart = _uart;
            log.println('Transfering code with XMODEM...');
        })
        .catch(error => {
            log.println('Error: ' + error);
        });
}

function txjsAbort() {
    log.println();
    log.println('Aborting transfer...');
    disconnect();
}

function stringToUint8Array(str) {
    var arr = new Uint8Array(new ArrayBuffer(str.length));
    for (var i = 0; i < str.length; i++)
        arr[i] = str.charCodeAt(i);
    return arr;
}

function checkWBT() {
    if (navigator.bluetooth)
        return true;

    log.println('Error: WebBluetooth API is not available');
    return false;
}

document.querySelector('#open').addEventListener('change', event => {
    var reader = new FileReader();
    reader.onload = function(e) { document.querySelector('#editor').value = e.target.result; };
    reader.readAsText(event.target.files[0]);
});

document.querySelector('#tx').addEventListener('click', event => {
    event.stopPropagation();
    event.preventDefault();
    if (checkWBT())
        txjsStart(stringToUint8Array(document.querySelector('#editor').value));
});

document.querySelector('#abort').addEventListener('click', event => {
    event.stopPropagation();
    event.preventDefault();
    if (checkWBT())
        txjsAbort();
});
