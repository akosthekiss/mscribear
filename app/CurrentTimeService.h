#ifndef CURRENTTIMESERVICE_H
#define CURRENTTIMESERVICE_H

#include "mbed.h"
#include "ble/BLE.h"


/**
 * A Ticker-less (partial) implementation of the BLE Current Time Service.
 *
 * Current Time Characteristic is R/W. Every read returns the current value of
 * the RTC, while a write sets the RTC.
 * Note: The value of the characteristic is NOT updated periodically, only on
 * read requests.
 */
class CurrentTimeService {
public:
    CurrentTimeService(BLE &ble)
        : _ble(ble)
        , _currentTimeData()
        , _currentTimeCharacteristic(GattCharacteristic::UUID_CURRENT_TIME_CHAR, _currentTimeData, CURRENT_TIME_DATA_SIZE, CURRENT_TIME_DATA_SIZE,
                                     GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {
        _currentTimeCharacteristic.setReadAuthorizationCallback(this, &CurrentTimeService::readAuthorizationCallback);

        GattCharacteristic *charTable[] = {&_currentTimeCharacteristic};
        GattService currentTimeService(GattService::UUID_CURRENT_TIME_SERVICE, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

        ble.addService(currentTimeService);
        ble.onDataWritten(this, &CurrentTimeService::onDataWritten);
    }

protected:
    void onDataWritten(const GattWriteCallbackParams *params)
    {
        if (params->handle == _currentTimeCharacteristic.getValueAttribute().getHandle()) {
            // NOTE: assume params->len >= 7
            struct tm now_tm = { 0 };
            now_tm.tm_year  = *(uint16_t *)&params->data[0] - 1900;
            now_tm.tm_mon = params->data[2] - 1;
            now_tm.tm_mday = params->data[3];
            now_tm.tm_hour = params->data[4];
            now_tm.tm_min = params->data[5];
            now_tm.tm_sec = params->data[6];

            set_time(mktime(&now_tm));
        }
    }

    void readAuthorizationCallback(GattReadAuthCallbackParams *params)
    {
        time_t now;
        time(&now);
        struct tm *now_tm = localtime(&now);

        memset(_currentTimeData, 0, CURRENT_TIME_DATA_SIZE);

        *(uint16_t *)&_currentTimeData[0] = now_tm->tm_year + 1900;
        _currentTimeData[2] = now_tm->tm_mon + 1;
        _currentTimeData[3] = now_tm->tm_mday;
        _currentTimeData[4] = now_tm->tm_hour;
        _currentTimeData[5] = now_tm->tm_min;
        _currentTimeData[6] = now_tm->tm_sec;
        _currentTimeData[7] = now_tm->tm_wday == 0 ? 7 : now_tm->tm_wday;

        // See GattCharacteristic::authorizeRead
        // "If the read is approved, a new value can be provided by setting the params->data pointer and params->len fields."
        params->data = _currentTimeData;
        params->len = CURRENT_TIME_DATA_SIZE;
        params->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }

    enum { CURRENT_TIME_DATA_SIZE = 10 };

    BLE &_ble;
    uint8_t _currentTimeData[CURRENT_TIME_DATA_SIZE];
    GattCharacteristic _currentTimeCharacteristic;
};

#endif /* CURRENTTIMESERVICE_H */
