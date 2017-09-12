// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#ifndef EXTENDEDUARTSERVICE_H
#define EXTENDEDUARTSERVICE_H_

#include "ble/services/UARTService.h"


class ExtendedUARTService : public UARTService {
public:
    ExtendedUARTService(BLE &_ble)
        : UARTService(_ble)
    {
    }

    void flush()
    {
        if (ble.getGapState().connected) {
            if (sendBufferIndex != 0) {
                ble.gattServer().write(getRXCharacteristicHandle(), static_cast<const uint8_t *>(sendBuffer), sendBufferIndex);
                sendBufferIndex = 0;
            }
        }
    }

    int writeChar(int c) {
        return (write(&c, 1) == 1) ? 1 : EOF;
    }
};

#endif /* EXTENDEDUARTSERVICE_H */
