#include "mbed.h"

#include "jerryscript.h"
#include "jerryscript-ext/arg.h"
#include "jerryscript-ext/handler.h"

#include "string.h"


static DigitalOut led1(LED1);
static Serial pc(USBTX, USBRX);

static EventQueue queue;

static jerry_value_t js_app_val;


static jerry_value_t js_handler_getled(const jerry_value_t func_obj_val, /**< function object */
                                       const jerry_value_t this_p, /**< this arg */
                                       const jerry_value_t args_p[], /**< function arguments */
                                       const jerry_length_t args_cnt) /**< number of function arguments */
{
    pc.printf("[js] getled called\r\n");
    return jerry_create_boolean(led1 != 0);
}

static jerry_value_t js_handler_setled(const jerry_value_t func_obj_val, /**< function object */
                                       const jerry_value_t this_p, /**< this arg */
                                       const jerry_value_t args_p[], /**< function arguments */
                                       const jerry_length_t args_cnt) /**< number of function arguments */
{
    pc.printf("[js] setled called\r\n");

    bool newvalue;
    jerryx_arg_t mapping[] = {
        jerryx_arg_boolean(&newvalue, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)
    };

    const jerry_value_t rv = jerryx_arg_transform_args(args_p, args_cnt, mapping, 1);
    if (jerry_value_has_error_flag(rv)) {
        return rv;
    }

    led1 = newvalue ? 1 : 0;
    return jerry_create_undefined();
}

static void blinkCallback(void)
{
    if (!jerry_value_has_error_flag(js_app_val))
        jerry_release_value(jerry_run(js_app_val));

    pc.printf("[cb] heartbeat\r\n");
}

int main()
{
    pc.printf("[app] initializing jerry...\r\n");
    jerry_init(JERRY_INIT_EMPTY);
    pc.printf("[app] ...success\r\n");

    pc.printf("[app] registering led handlers...\r\n");
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"setled", js_handler_setled));
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"getled", js_handler_getled));
    pc.printf("[app] ...success\r\n");

    pc.printf("[app] parsing js code...\r\n");
    const char *js_app_str = "setled(!getled());";
    js_app_val = jerry_parse((const jerry_char_t*)js_app_str, strlen(js_app_str), false);
    pc.printf("[app] ...success\r\n");

    queue.call_every(500, blinkCallback);
    queue.dispatch_forever();

    return 0;
}
