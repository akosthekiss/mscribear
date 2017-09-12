// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#ifndef XMODEMRECEIVER_H
#define XMODEMRECEIVER_H

#include <cstring>

#include "Buffer.h"


/**
 * Push implementation of an XMODEM128-CRC receiver.
 */
class XMODEMReceiver {
public:
    enum ErrorCode {
        ERROR_REMOTECANCEL,
        ERROR_RETRYEXCEED,
        ERROR_OUTOFMEM
    };

    typedef void (*SendCallback)(char byte);
    typedef void (*SuccessCallback)();
    typedef void (*ErrorCallback)(ErrorCode error);

    /**
     * @param buf the buffer to receive the file into.
     * @param doSend callback for transmitting a reply to the sender (MUST NOT
     *    be NULL).
     * @param onSuccess callback notified on a successful file transfer (if not
     *    NULL).
     * @param onError callback notified on a failed transfer, be it cancelled by
     *    the sender or because of some other error (if not NULL).
     * @param retry_limit
     */
    XMODEMReceiver(Buffer &buf, SendCallback doSend, SuccessCallback onSuccess=NULL, ErrorCallback onError=NULL, unsigned int retry_limit=20)
        : _buf(buf)
        , _doSend(doSend)
        , _onSuccess(onSuccess)
        , _onError(onError)
        , _retry_limit(retry_limit)
    {
        reset();
    }

    void request()
    {
      if (_buf.size() == 0 && _packet_size == 0)
          _doSend('C');
    }

    void reset()
    {
        _buf.clear();
        _packet_size = 0;
        _packet_number = 1;
        _retry_count = 0;

        request();
    }

    /**
     * @param data the received data in a byte array.
     * @param size the size of the received data in bytes.
     * @return the number of bytes consumed from data.
     */
    unsigned int dataReceived(const char *data, unsigned int size)
    {
        unsigned int data_size = size;

        while (size > 0) {
            // Ensure that packet starts with a valid byte: skip invalid bytes.
            if (_packet_size == 0 && *data != CONTROL_SOH && *data != CONTROL_EOT && *data != CONTROL_CAN) {
                data++;
                size--;
                continue;
            }

            // Accumulate received data.
            unsigned int remaining = PACKET_SIZE - _packet_size;
            unsigned int tocopy = remaining >= size ? size : remaining;
            std::memcpy(_packet_buffer + _packet_size, data, tocopy);

            data += tocopy;
            size -= tocopy;
            _packet_size += tocopy;

            if (*_packet_buffer == CONTROL_CAN) {
                // The remote part cancelled the transmission.
                _transferCancelled();
                return 1;
            }

            if (*_packet_buffer == CONTROL_EOT) {
                // End of transmission.
                _transferEnded();
                return 1;
            }

            if (_packet_size == PACKET_SIZE) {
                _packet_size = 0;
                if (!_packetReceived()) {
                    _retry_count++;
                    break;
                }
            }
        }

        if (_retry_count > _retry_limit) {
            _doSend(CONTROL_CAN);
            _doSend(CONTROL_CAN);
            _doSend(CONTROL_CAN);

            _notifyError(ERROR_RETRYEXCEED);
        }
        else {
            request();
        }

        return data_size - size;
    }

private:
    void _notifySuccess()
    {
        if (_onSuccess)
            _onSuccess();
    }

    void _notifyError(ErrorCode error)
    {
        if (_onError)
            _onError(error);

        reset();
    }

    bool _packetReceived()
    {
        char *pack_buf = _packet_buffer + 1;

        // Check block number
        if (*pack_buf++ != _packet_number) {
            _doSend(CONTROL_NAK);
            return false;
        }
        if (*pack_buf++ != (unsigned char)~_packet_number) {
            _doSend(CONTROL_NAK);
            return false;
        }

        // Check CRC
        unsigned int chk = 0;
        for (unsigned int i = 0; i < PAYLOAD_SIZE; i++) {
            chk = chk ^ *pack_buf++ << 8;
            for (unsigned int j = 0; j < 8; j++)
                chk = chk & 0x8000 ? chk << 1 ^ 0x1021 : chk << 1;
        }
        chk &= 0xFFFF;

        if (*pack_buf++ != ((chk >> 8) & 0xFF)) {
            _doSend(CONTROL_NAK);
            return false;
        }
        if (*pack_buf++ != (chk & 0xFF)) {
            _doSend(CONTROL_NAK);
            return false;
        }

        _retry_count = 0;
        _packet_number++;

        // Acknowledge and consume packet.
        if (!_buf.append(_packet_buffer + 3, PAYLOAD_SIZE)) {
            // Not enough memory, force cancel and return.
            _doSend(CONTROL_CAN);
            _doSend(CONTROL_CAN);
            _doSend(CONTROL_CAN);

            _notifyError(ERROR_OUTOFMEM);

            return false;
        }

        _doSend(CONTROL_ACK);
        return true;
    }

    void _transferEnded()
    {
        _doSend(CONTROL_ACK);

        const char *ptr = _buf.ptr();
        unsigned int size = _buf.size();
        unsigned int pad_size = 0;
        while (pad_size <= size && ptr[size - pad_size - 1] == CONTROL_EOF)
            pad_size++;
        _buf.chop(pad_size);

        _notifySuccess();
    }

    void _transferCancelled()
    {
        _doSend(CONTROL_ACK);

        _notifyError(ERROR_REMOTECANCEL);
    }

    enum { PAYLOAD_SIZE = 128, PACKET_SIZE = PAYLOAD_SIZE + 5 };

    enum {
        CONTROL_SOH = 0x01,
        CONTROL_EOT = 0x04,
        CONTROL_ACK = 0x06,
        CONTROL_NAK = 0x15,
        CONTROL_CAN = 0x18,
        CONTROL_EOF = 0x1A
    };

    Buffer &_buf;

    SendCallback _doSend;
    SuccessCallback _onSuccess;
    ErrorCallback _onError;

    unsigned int _retry_limit;
    unsigned int _retry_count;

    char _packet_buffer[PACKET_SIZE];
    unsigned int _packet_size;
    unsigned char _packet_number;
};

#endif /* XMODEMRECEIVER_H */
