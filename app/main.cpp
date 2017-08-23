#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/UARTService.h"

#include "jerryscript.h"
#include "jerryscript-ext/arg.h"
#include "jerryscript-ext/handler.h"

#include "peripherals.h"
#include "CStringBuffer.h"
#include "Morse.h"


static EventQueue queue;

static const char ble_device_name[] = "Nano2";
static UARTService* ble_uart_service_p;

static CStringBuffer strbuffer;
static jerry_value_t js_app_val;


static jerry_value_t js_handler_getled(const jerry_value_t func_obj_val, /**< function object */
                                       const jerry_value_t this_p, /**< this arg */
                                       const jerry_value_t args_p[], /**< function arguments */
                                       const jerry_length_t args_cnt) /**< number of function arguments */
{
    // usb.printf("[js] getled called\r\n");

    return jerry_create_boolean(led1 != 0);
}

static jerry_value_t js_handler_setled(const jerry_value_t func_obj_val, /**< function object */
                                       const jerry_value_t this_p, /**< this arg */
                                       const jerry_value_t args_p[], /**< function arguments */
                                       const jerry_length_t args_cnt) /**< number of function arguments */
{
    // usb.printf("[js] setled called\r\n");

    bool newvalue;
    jerryx_arg_t mapping[] = {
        jerryx_arg_boolean(&newvalue, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)
    };

    const jerry_value_t rv = jerryx_arg_transform_args(args_p, args_cnt, mapping, 1);
    if (jerry_value_has_error_flag(rv))
        return rv;

    led1 = newvalue ? 1 : 0;
    return jerry_create_undefined();
}

static void onHeartbeat(void)
{
    usb.printf("[usb] heartbeat\r\n");

    if (!jerry_value_has_error_flag(js_app_val))
        jerry_release_value(jerry_run(js_app_val));

    if (BLE::Instance().gap().getState().connected)
        ble_uart_service_p->writeString("[ble] heartbeat");
}

static void onBleConnect(const Gap::ConnectionCallbackParams_t *params)
{
    usb.printf("[ble] connect\r\n");

    Morse morse(led1);
    morse.puts("HI");

    strbuffer.clear();
}

static void onBleDataWritten(const GattWriteCallbackParams *params) {
    usb.printf("[ble] written\r\n");

    if (params->handle == ble_uart_service_p->getTXCharacteristicHandle())
        bool appended = strbuffer.append((const char *)params->data, params->len);
}

static void onBleDisconnect(const Gap::DisconnectionCallbackParams_t *params)
{
    usb.printf("[ble] disconnect\r\n");

    Morse morse(led1);
    morse.puts("CU");

    usb.printf("[strbuffer] %s\r\n", strbuffer.str());

    BLE::Instance().gap().startAdvertising();
}

static void onBleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    usb.printf("[ble] init complete\r\n");

    BLE& ble = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        /* In case of error, do sg meaningful */
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE)
        return;

    ble.gap().onConnection(onBleConnect);
    ble.gap().onDisconnection(onBleDisconnect);

    /* Setup primary service */
    ble_uart_service_p = new UARTService(ble);
    ble.gattServer().onDataWritten(onBleDataWritten);

    /* Setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
                                           (const uint8_t *)UARTServiceUUID_reversed, sizeof(UARTServiceUUID_reversed));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                           (const uint8_t *)ble_device_name, sizeof(ble_device_name));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();
}

static void onBleProcessEvents(BLE::OnEventsToProcessCallbackContext* context)
{
    queue.call(Callback<void()>(&BLE::Instance(), &BLE::processEvents));
}

int main()
{
    usb.printf("[app] initializing jerry...\r\n");
    jerry_init(JERRY_INIT_EMPTY);
    usb.printf("[app] ...success\r\n");

    usb.printf("[app] registering led handlers...\r\n");
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"getled", js_handler_getled));
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"setled", js_handler_setled));
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"print", jerryx_handler_print));
    usb.printf("[app] ...success\r\n");

    usb.printf("[app] parsing js code...\r\n");
    const char *js_app_str = "setled(!getled()); /*print('[script] switched')*/";
    js_app_val = jerry_parse((const jerry_char_t*)js_app_str, strlen(js_app_str), false);
    usb.printf("[app] ...success\r\n");

    usb.printf("[app] initializing ble...\r\n");
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(onBleProcessEvents);
    ble.init(onBleInitComplete);
    usb.printf("[app] ...success\r\n");

    Morse morse(led1);
    morse.puts("CQ ");

    usb.printf("[app] starting event queue...\r\n");
    queue.call_every(2000, onHeartbeat);
    queue.dispatch_forever();

    return 0;
}
