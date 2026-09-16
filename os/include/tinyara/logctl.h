/****************************************************************************
 *
 * Copyright 2026 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#ifndef __INCLUDE_LOGCTL_H
#define __INCLUDE_LOGCTL_H

#include <stdbool.h>

enum logctl_module_e {
    LOGCTL_MODULE_COMMON = 0,
    LOGCTL_MODULE_AUDIO,
    LOGCTL_MODULE_BINFMT,
    LOGCTL_MODULE_BINARY_COMPRESSION,
    LOGCTL_MODULE_BINMGR,
    LOGCTL_MODULE_FS,
    LOGCTL_MODULE_LIB,
    LOGCTL_MODULE_LOGDUMP,
    LOGCTL_MODULE_MM,
    LOGCTL_MODULE_NET,
    LOGCTL_MODULE_BLE,
    LOGCTL_MODULE_SMP,
    LOGCTL_MODULE_PM,
    LOGCTL_MODULE_SCHED,
    LOGCTL_MODULE_SYSCALL,
    LOGCTL_MODULE_TASH,
    LOGCTL_MODULE_WIFICSI,
    LOGCTL_MODULE_EVENTLOOP,
    LOGCTL_MODULE_MEDIA,
    LOGCTL_MODULE_PREFERENCE,
    LOGCTL_MODULE_ST_THINGS,
    LOGCTL_MODULE_TASK_MANAGER,
    LOGCTL_MODULE_REBOOT_REASON,
    LOGCTL_MODULE_UI,
    LOGCTL_MODULE_ANALOG,
    LOGCTL_MODULE_I2S,
    LOGCTL_MODULE_MIPI_DSI,
    LOGCTL_MODULE_LCD,
    LOGCTL_MODULE_SPI,
    LOGCTL_MODULE_TIMER,
    LOGCTL_MODULE_TOUCH,
    LOGCTL_MODULE_USB,
    LOGCTL_MODULE_VIDEO,
    LOGCTL_MODULE_WLAN,
    LOGCTL_MODULE_MAX
};

void logctl_init(void);
int logctl_enable(enum logctl_module_e module);
int logctl_disable(enum logctl_module_e module);
bool logctl_is_enabled(enum logctl_module_e module);
int logctl_get_status(char *buffer, int bufsize);
void logctl_enable_all(void);
void logctl_disable_all(void);
void logctl_register_tashcmds(void);

#endif /* __INCLUDE_LOGCTL_H */
