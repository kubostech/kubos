/*
 * Copyright (C) 2017 Kubos Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <dbus/dbus.h>
#include "evented-control/ecp.h"

#define TEST_PUB_INTERFACE "org.KubOS.TestPublisher"
#define TEST_PUB_PATH "/org/KubOS/TestPublisher"

#define TEST_PUB_SIGNAL "TestSignal"

typedef ECPStatus (*test_signal_cb)(int16_t num);

typedef struct
{
    ECPMessageHandler super;
    test_signal_cb    cb;
} ECPTestSignalMessageHandler;

ECPStatus on_test_signal(ECPContext * context, test_signal_cb cb);
