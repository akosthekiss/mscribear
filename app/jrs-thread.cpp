// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#include "jrs-thread.h"

#include "mbed.h"

#include "jerryscript.h"
#include "jerryscript-ext/arg.h"
#include "jerryscript-ext/handler.h"

#include "Buffer.h"
#include "main.h"


#define JRS_BUFFER_SIGNAL 0x1

static Thread jrs_thread;
static Buffer jrs_buffer;
static const char * const jrs_default_str = "while (true) { setled(!getled()); wait(0.5); }";


static jerry_value_t jrs_handler_getled(const jerry_value_t func_obj_val, /**< function object */
                                        const jerry_value_t this_p, /**< this arg */
                                        const jerry_value_t args_p[], /**< function arguments */
                                        const jerry_length_t args_cnt) /**< number of function arguments */
{
    // usb.printf("[jrs] getled called\r\n");

    return jerry_create_boolean(led1 != 0);
}

static jerry_value_t jrs_handler_setled(const jerry_value_t func_obj_val, /**< function object */
                                        const jerry_value_t this_p, /**< this arg */
                                        const jerry_value_t args_p[], /**< function arguments */
                                        const jerry_length_t args_cnt) /**< number of function arguments */
{
    // usb.printf("[jrs] setled called\r\n");

    bool state;
    jerryx_arg_t mapping[] = {
        jerryx_arg_boolean(&state, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)
    };

    const jerry_value_t rv = jerryx_arg_transform_args(args_p, args_cnt, mapping, 1);
    if (jerry_value_has_error_flag(rv))
        return rv;

    led1 = state ? 1 : 0;
    return jerry_create_undefined();
}

static jerry_value_t jrs_handler_wait(const jerry_value_t func_obj_val, /**< function object */
                                      const jerry_value_t this_p, /**< this arg */
                                      const jerry_value_t args_p[], /**< function arguments */
                                      const jerry_length_t args_cnt) /**< number of function arguments */
{
    // usb.printf("[jrs] wait called\r\n");

    double usecs;
    jerryx_arg_t mapping[] = {
        jerryx_arg_number(&usecs, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)
    };

    const jerry_value_t rv = jerryx_arg_transform_args(args_p, args_cnt, mapping, 1);
    if (jerry_value_has_error_flag(rv))
        return rv;

    wait(usecs);
    return jerry_create_undefined();
}

static jerry_value_t jrs_stop(void *user_p)
{
    return jrs_buffer.size() != 0 ? jerry_create_string((const jerry_char_t*)"JS updated") : jerry_create_undefined();
}

static void jrs_entry(void)
{
    if (jrs_buffer.size() == 0)
        jrs_buffer.append(jrs_default_str, strlen(jrs_default_str));
    jrs_thread.signal_set(JRS_BUFFER_SIGNAL);

    while (true) {
        usb.printf("[jrs] waiting for js code\r\n");
        jrs_thread.signal_wait(JRS_BUFFER_SIGNAL);

        usb.printf("[jrs] initializing jerryscript\r\n");
        jerry_init(JERRY_INIT_EMPTY);

        usb.printf("[jrs] registering external function handlers\r\n");
        jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"getled", jrs_handler_getled));
        jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"setled", jrs_handler_setled));
        jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"print", jerryx_handler_print));
        jerry_release_value(jerryx_handler_register_global((const jerry_char_t*)"wait", jrs_handler_wait));

        usb.printf("[jrs] registering stop callback\r\n");
        jerry_set_vm_exec_stop_callback(jrs_stop, NULL, 0);

        usb.printf("[jrs] parsing js code\r\n");
        jerry_value_t jrs_app_val = jerry_parse((const jerry_char_t*)jrs_buffer.ptr(), jrs_buffer.size(), false);
        jrs_buffer.clear();

        if (jerry_value_has_error_flag(jrs_app_val)) {
            usb.printf("[jrs] parse returned an error\r\n");
        }
        else {
            usb.printf("[jrs] running js code\r\n");
            jerry_value_t jrs_result_val = jerry_run(jrs_app_val);
            if (jerry_value_has_error_flag(jrs_result_val))
                usb.printf("[jrs] run returned an error\r\n");
            jerry_release_value(jrs_result_val);
        }

        jerry_release_value(jrs_app_val);

        usb.printf("[jrs] cleaning up jerryscript\r\n");
        jerry_cleanup();
    }
}

void jrs_start_thread(void)
{
    jrs_thread.start(jrs_entry);
}

void jrs_take_buffer(Buffer &buffer)
{
    jrs_buffer.take(buffer);
    if (jrs_buffer.size() != 0)
        jrs_thread.signal_set(JRS_BUFFER_SIGNAL);
}
