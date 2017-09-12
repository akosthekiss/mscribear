// Copyright (c) 2017 Akos Kiss.
//
// Licensed under the BSD 3-Clause License
// <LICENSE.md or https://opensource.org/licenses/BSD-3-Clause>.
// This file may not be copied, modified, or distributed except
// according to those terms.

#ifndef JRS_THREAD_H
#define JRS_THREAD_H

#include "Buffer.h"


void jrs_start_thread(void);
void jrs_take_buffer(Buffer &buffer);

#endif /* JRS_THREAD_H */
