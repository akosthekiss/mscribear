var log = new Log(document.querySelector('#log'));
var uart = new UART(log);

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
    uart.disconnect();
}

function xmodemError() {
    log.println();
    log.println('Transfer failed');
    uart.disconnect();
}

function txjsStart(content) {
    log.clear();
    var xmodem = new XMODEMSender(content, xmodemSend, xmodemSuccess, xmodemError);
    uart.connect(event => xmodem.dataReceived(event.target.value))
        .then(_ => {
            log.println('Transfering code with XMODEM...');
        })
        .catch(error => {
            log.println('Connection failed: ' + error);
        });
}

function txjsAbort() {
    log.println();
    log.println('Aborting transfer...');
    uart.disconnect();
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
