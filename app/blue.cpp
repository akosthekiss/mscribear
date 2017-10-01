// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#include "blue.h"

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/UARTService.h"

#include "Buffer.h"
#include "CurrentTimeService.h"
#include "jrs-thread.h"
#include "main.h"
#include "XMODEMReceiver.h"


static const char blue_device_name[] = "Nano2";
static UARTService *blue_uart_service;
static CurrentTimeService *blue_current_time_service;

static XMODEMReceiver *blue_xmodem;
static Buffer blue_xmodem_buffer;


static void blue_xmodem_send(char byte)
{
    if (BLE::Instance().gap().getState().connected)
    {
        // usb.printf("[blue] xmodem sent: %x\r\n", (int)byte);
        blue_uart_service->write(&byte, 1);
        blue_uart_service->flush();
    }
}

static void blue_xmodem_received(void)
{
    usb.printf("[blue] xmodem received: %.*s\r\n", blue_xmodem_buffer.size(), blue_xmodem_buffer.ptr());

    jrs_take_buffer(blue_xmodem_buffer);
}

static void blue_xmodem_failed(XMODEMReceiver::ErrorCode error)
{
    usb.printf("[blue] xmodem failed: %d\r\n", (int)error);
}

static void blue_xmodem_request(void)
{
    if (BLE::Instance().gap().getState().connected) {
        blue_xmodem->request();
        queue.call_in(3000, blue_xmodem_request);
    }
}

static void blue_connected(const Gap::ConnectionCallbackParams_t *params)
{
    usb.printf("[blue] connected\r\n");

    blue_xmodem->reset();
    queue.call_in(3000, blue_xmodem_request);
}

static void blue_characteristic_written(const GattWriteCallbackParams *params) {
    if (params->handle == blue_uart_service->getTXCharacteristicHandle()) {
        usb.printf("[blue] uart tx characteristic written\r\n");
        // usb.printf("      %.*s\r\n", params->len, (const char *)params->data);
        unsigned int n = blue_xmodem->dataReceived((const char *)params->data, params->len);
        // usb.printf("      [%d]\r\n", n);
    }
}

static void blue_disconnected(const Gap::DisconnectionCallbackParams_t *params)
{
    usb.printf("[blue] disconnected\r\n");

    BLE::Instance().gap().startAdvertising();
}

static void blue_init_completed(BLE::InitializationCompleteCallbackContext *params)
{
    BLE& ble = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        /* In case of error, do sg meaningful */
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE)
        return;

    usb.printf("[blue] registering connection callbacks\r\n");
    ble.gap().onConnection(blue_connected);
    ble.gap().onDisconnection(blue_disconnected);

    /* Setup services */
    usb.printf("[blue] initializing uart service\r\n");
    blue_uart_service = new UARTService(ble);
    ble.gattServer().onDataWritten(blue_characteristic_written);

    usb.printf("[blue] initializing current time service\r\n");
    blue_current_time_service = new CurrentTimeService(ble);

    /* Setup advertising */
    usb.printf("[blue] configuring and starting advertising\r\n");

    uint16_t uuid16_list[] = { GattService::UUID_CURRENT_TIME_SERVICE };

    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                           (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS,
                                           (const uint8_t *)uuid16_list, sizeof(uuid16_list));
    // NOTE: the name might not fit in the advertising payload anymore, so put
    //       it into the scan response
    // NOTE: don't try to split the payload in a different way, keep all 128 and
    //       16 bit service UUIDS in the advertising payload, BluefruitLE/OSX
    //       may not be able to see all of them (Android BLE stack handles that
    //       case correctly, too)
    ble.gap().accumulateScanResponse(GapAdvertisingData::COMPLETE_LOCAL_NAME,
                                     (const uint8_t *)blue_device_name, sizeof(blue_device_name));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* multiples of 0.625ms */
    ble.gap().startAdvertising();
}

static void blue_process_events(BLE::OnEventsToProcessCallbackContext* context)
{
    queue.call(Callback<void()>(&BLE::Instance(), &BLE::processEvents));
}

void blue_init(void)
{
    usb.printf("[blue] initializing ble\r\n");
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(blue_process_events);
    ble.init(blue_init_completed);

    usb.printf("[blue] initializing xmodem\r\n");
    blue_xmodem = new XMODEMReceiver(blue_xmodem_buffer, blue_xmodem_send, blue_xmodem_received, blue_xmodem_failed);
}
