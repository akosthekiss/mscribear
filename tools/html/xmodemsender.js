function XMODEMSender(buf, doSend, onSuccess, onError, retry_limit=20) {
    this.buf = buf;
    this.doSend = doSend;
    this.onSuccess = onSuccess;
    this.onError = onError;
    this.retry_limit = retry_limit;

    this.retry_count = 0;
    this.sequence = 0;
}

XMODEMSender.PAYLOAD_SIZE = 128;
XMODEMSender.PACKET_SIZE  = 128 + 5;

XMODEMSender.CONTROL_SOH = 0x01;
XMODEMSender.CONTROL_EOT = 0x04;
XMODEMSender.CONTROL_ACK = 0x06;
XMODEMSender.CONTROL_NAK = 0x15;
XMODEMSender.CONTROL_CAN = 0x18;
XMODEMSender.CONTROL_EOF = 0x1a;
XMODEMSender.CONTROL_CRC = 0x43;

XMODEMSender.prototype.dataReceived = function(data) {
    if (data.byteLength != 1
        || (this.sequence == 0 && data.getUint8(0) != XMODEMSender.CONTROL_CRC)
        || (this.sequence > 0 && data.getUint8(0) != XMODEMSender.CONTROL_ACK)) {
        this.retry_count++;

        if (this.retry_count > this.retry_limit) {
            this.notifyError();
            return;
        }

        if (data.getUint8(0) == XMODEMSender.CONTROL_CAN)
            this.retry_count = this.retry_limit;

        if (this.sequence == 0)
            return;
    }
    else {
        this.sequence++;
    }

    this.retry_count = 0;

    var last_sequence = Math.floor((this.buf.length + XMODEMSender.PAYLOAD_SIZE - 1) / XMODEMSender.PAYLOAD_SIZE);
    if (this.sequence == last_sequence + 1) {
        this.doSend(new Uint8Array([XMODEMSender.CONTROL_EOT]));
        return;
    }
    if (this.sequence > last_sequence + 1) {
        this.notifySuccess();
        return;
    }

    var packet = new Uint8Array(XMODEMSender.PACKET_SIZE);
    packet[0] = XMODEMSender.CONTROL_SOH;
    packet[1] = this.sequence % 0x100;
    packet[2] = 0xff - packet[1];

    var payload = this.buf.subarray(XMODEMSender.PAYLOAD_SIZE * (this.sequence - 1), XMODEMSender.PAYLOAD_SIZE * this.sequence);
    packet.set(payload, 3);
    for (var i = payload.length; i < XMODEMSender.PAYLOAD_SIZE; i++)
        packet[3 + i] = XMODEMSender.CONTROL_EOF;

    var crc = 0;
    for (var i = 0; i < XMODEMSender.PAYLOAD_SIZE; i++) {
        crc = crc ^ (packet[3 + i] << 8);
        for (var j = 0; j < 8; j++)
            crc = (crc & 0x8000) != 0 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    crc &= 0xffff;
    packet[3 + XMODEMSender.PAYLOAD_SIZE] = crc >> 8;
    packet[3 + XMODEMSender.PAYLOAD_SIZE + 1] = crc & 0xff;

    this.doSend(packet);
}

XMODEMSender.prototype.reset = function() {
    this.sequence = 0;
    this.retry_count = 0;
}

XMODEMSender.prototype.notifySuccess = function() {
    if (this.onSuccess)
        this.onSuccess();
    this.reset();
}

XMODEMSender.prototype.notifyError = function() {
    var can = new Uint8Array([XMODEMSender.CONTROL_CAN]);
    this.doSend(can);
    this.doSend(can);
    this.doSend(can);

    if (this.onError)
        this.onError();
    this.reset();
}
