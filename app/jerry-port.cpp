#include "mbed.h"
#include "jerryscript-port.h"


static DigitalOut led1(LED1);


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
