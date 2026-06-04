/****************************************************************************
 *
 * Copyright 2025 Samsung Electronics All Rights Reserved.
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

#ifndef __INCLUDE_FLASH_DRV_H
#define __INCLUDE_FLASH_DRV_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <tinyara/fs/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Flash driver path */
#define FLASH_DEV_PATH "/dev/flash0"

/* Flash read/write ioctl structure */
struct flash_io_s {
    uint32_t address;    /* Flash address */
    uint32_t len;        /* Length of data */
    uint8_t *data;       /* Data buffer */
    int en_display;      /* Enable display flag */
};

/* Flash IOCTL commands - defined in fs/ioctl.h */
/* FLASHIOC_READ, FLASHIOC_WRITE, FLASHIOC_ERASE, FLASHIOC_VERIFY_PROTECT */

/* Flash error codes */
#define FLASH_OK             0
#define FLASH_ERR_IO        -1
#define FLASH_ERR_INVAL     -2
#define FLASH_ERR_NOMEM     -3
#define FLASH_ERR_BUSY      -4
#define FLASH_ERR_LOCKED    -5

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* Forward reference */
struct file;

/* Flash device structure */
struct flash_dev_s {
    uint32_t address;      /* Flash address */
    uint32_t len;          /* Flash length */
    uint8_t *data;         /* Data buffer */
    int en_display;        /* Enable display flag */
};

/* Lower-half flash operations - hardware specific implementation */
struct flash_lowerops_s {
    /* Read data from flash */
    int (*read)(FAR void *priv, uint32_t address, FAR uint8_t *buffer, uint32_t size);

    /* Write data to flash */
    int (*write)(FAR void *priv, uint32_t address, FAR const uint8_t *buffer, uint32_t size);

    /* Erase flash sector(s) */
    int (*erase)(FAR void *priv, uint32_t sector, uint32_t count);
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: flash_register
 *
 * Description:
 *   Register the flash driver with the system.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) on success; A negated errno value on failure.
 *
 ****************************************************************************/

int flash_register();

/****************************************************************************
 * Name: flash_unregister
 *
 * Description:
 *   Unregister the flash driver from the system.
 *
 * Input Parameters:
 *   path - Device path (e.g., "/dev/flash0")
 *
 * Returned Value:
 *   Zero (OK) on success; A negated errno value on failure.
 *
 ****************************************************************************/

int flash_unregister(FAR const char *path);

/****************************************************************************
 * Name: flash_read
 *
 * Description:
 *   Read data from flash memory.
 *
 * Input Parameters:
 *   path     - Device path
 *   address  - Byte address to read from
 *   buffer   - Buffer to store read data
 *   size     - Number of bytes to read
 *
 * Returned Value:
 *   Number of bytes read on success; A negated errno value on failure.
 *
 ****************************************************************************/

ssize_t flash_read(FAR const char *path, uint32_t address, FAR uint8_t *buffer,
                   uint32_t size);

/****************************************************************************
 * Name: flash_write
 *
 * Description:
 *   Write data to flash memory.
 *
 * Input Parameters:
 *   path     - Device path
 *   address  - Byte address to write to
 *   buffer   - Buffer containing data to write
 *   size     - Number of bytes to write
 *
 * Returned Value:
 *   Number of bytes written on success; A negated errno value on failure.
 *
 ****************************************************************************/

ssize_t flash_write(FAR const char *path, uint32_t address,
                    FAR const uint8_t *buffer, uint32_t size);

/****************************************************************************
 * Name: flash_erase
 *
 * Description:
 *   Erase flash sector(s).
 *
 * Input Parameters:
 *   path    - Device path
 *   sector  - Starting sector number to erase
 *   count   - Number of sectors to erase
 *
 * Returned Value:
 *   Zero (OK) on success; A negated errno value on failure.
 *
 ****************************************************************************/

int flash_erase(FAR const char *path, uint32_t sector, uint32_t count);

#ifdef __cplusplus
}
#undef EXTERN
#endif

#endif /* __INCLUDE_FLASH_DRV_H */
