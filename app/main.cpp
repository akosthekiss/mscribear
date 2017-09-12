// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#include "main.h"

#include "mbed.h"

#include "blue.h"
#include "jrs-thread.h"
#include "Morse.h"


DigitalOut led1(LED1, 0);
Serial usb(USBTX, USBRX);

EventQueue queue;


int main()
{
    usb.printf("[main] starting\r\n");
    Morse morse(led1);
    morse.puts("HI");

    blue_init();
    jrs_start_thread();

    usb.printf("[main] starting event queue\r\n");
    queue.dispatch_forever();

    return 0;
}
