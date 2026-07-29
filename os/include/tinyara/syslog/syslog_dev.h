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

#ifndef __INCLUDE_SYSLOG_DEV_H
#define __INCLUDE_SYSLOG_DEV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <sys/ioctl.h>
#include <tinyara/syslog/syslog_ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Syslog device path */

#define SYSLOG_DEVICE_PATH    "/dev/syslog"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: syslog_devinitialize
 *
 * Description:
 *   Initialize the syslog character device driver.
 *   This function registers /dev/syslog device that can be used
 *   to control syslog capture via ioctl calls.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSLOG_CHAR
int syslog_devinitialize(void);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_SYSLOG_DEV_H */
