#include "mbed.h"

#include "jerryscript.h"
#include "jerryscript-ext/arg.h"
#include "jerryscript-ext/handler.h"
#include "jerryscript-port.h"

#include "string.h"


DigitalOut led1(LED1);
Serial pc(USBTX, USBRX);

static jerry_value_t
js_handler_getled(const jerry_value_t func_obj_val, /**< function object */
                  const jerry_value_t this_p, /**< this arg */
                  const jerry_value_t args_p[], /**< function arguments */
                  const jerry_length_t args_cnt) /**< number of function arguments */
{
    pc.printf("[js] getled called\r\n");
    return jerry_create_boolean(led1 != 0);
}

static jerry_value_t
js_handler_setled(const jerry_value_t func_obj_val, /**< function object */
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

bool jerry_port_get_time_zone(jerry_time_zone_t *tz_p) /**< [out] time zone structure to fill */
{
    return false;
}

double jerry_port_get_current_time(void)
{
    return 0.0;
}

void jerry_port_fatal(jerry_fatal_code_t code)
{
    while (true) {
        led1 = !led1;
        wait(0.25);
    }
}

void jerry_port_log(jerry_log_level_t level, /**< log level */
                    const char *format, /**< format string */
                    ...)  /**< parameters */
{
}

extern "C" {
void HardFault_Handler() {
    __ASM volatile("BKPT #01");
    while (true);
}
}

// main() runs in its own thread in the OS
int main() {
    wait(5.0);

    pc.printf("[app] initializing jerry...\r\n");
    jerry_init(JERRY_INIT_EMPTY);
    pc.printf("[app] ...success\r\n");

    pc.printf("[app] registering led handlers...\r\n");
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"setled", js_handler_setled));
    jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"getled", js_handler_getled));
    pc.printf("[app] ...success\r\n");

    pc.printf("[app] parsing js code...\r\n");
    const char *switchled = "setled(!getled());";
    jerry_value_t switchled_val = jerry_parse((const jerry_char_t*)switchled, strlen(switchled), false);
    pc.printf("[app] ...success\r\n");

    while (true) {
        if (!jerry_value_has_error_flag(switchled_val))
            jerry_release_value(jerry_run(switchled_val));
        wait(0.5);

        pc.printf("[app] heartbeat\r\n");

        if (!jerry_value_has_error_flag(switchled_val))
            jerry_release_value(jerry_run(switchled_val));
        wait(0.5);
    }
}
