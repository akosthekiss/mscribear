#include "blue.h"

#include "mbed.h"
#include "ble/BLE.h"

#include "Buffer.h"
#include "ExtendedUARTService.h"
#include "jrs-thread.h"
#include "main.h"
#include "XMODEMReceiver.h"


static const char blue_device_name[] = "Nano2";
static ExtendedUARTService* blue_uart_service;

static XMODEMReceiver *blue_xmodem;
static Buffer blue_xmodem_buffer;


static void blue_xmodem_send(char byte)
{
    if (BLE::Instance().gap().getState().connected)
    {
        // usb.printf("[blue] xmodem sent: %x\r\n", (int)byte);
        blue_uart_service->writeChar(byte);
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
    usb.printf("[blue] characteristic written\r\n");

    if (params->handle == blue_uart_service->getTXCharacteristicHandle()) {
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

    /* Setup primary service */
    usb.printf("[blue] initializing uart service\r\n");
    blue_uart_service = new ExtendedUARTService(ble);
    ble.gattServer().onDataWritten(blue_characteristic_written);

    /* Setup advertising */
    usb.printf("[blue] configuring and starting advertising\r\n");
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                           (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                           (const uint8_t *)blue_device_name, sizeof(blue_device_name));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* ms */
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
