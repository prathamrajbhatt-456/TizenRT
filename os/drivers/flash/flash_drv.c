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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <debug.h>

#include <tinyara/fs/fs.h>
#include <tinyara/flash_drv.h>

/* External flash APIs */
#if defined(CONFIG_AMEBASMART_TRUSTZONE)
extern int rtl_ss_flash_read(uint32_t address, uint32_t len, uint8_t *data, int en_display);
extern int rtl_verify_flash_protect(void);
#else
extern int bk_ss_flash_read(uint32_t address, uint32_t len, uint8_t *data, int en_display);
extern int bk_verify_flash_protect(void);
#endif
extern void ns_flash_erase(uint32_t address);

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int flash_open(FAR struct file *filep);
static int flash_close(FAR struct file *filep);
static int flash_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct file_operations g_flash_fops = {
    flash_open,      /* open */
    flash_close,     /* close */
    flash_read,            /* read - not used, use ioctl instead */
    flash_write,            /* write - not used, use ioctl instead */
    NULL,            /* seek */
    flash_ioctl,     /* ioctl */
#ifndef CONFIG_DISABLE_POLL
    NULL,            /* poll */
#endif
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: flash_open
 ****************************************************************************/

static int flash_open(FAR struct file *filep)
{
    FAR struct inode *inode = filep->f_inode;
    FAR struct flash_dev_s *dev = inode->i_private;

    if (!dev) {
        return -ENODEV;
    }

    if (dev->en_display) {
        fvdbg("Flash device opened: addr=0x%x, len=%u\n", dev->address, dev->len);
    }

    return OK;
}

/****************************************************************************
 * Name: flash_close
 ****************************************************************************/

static int flash_close(FAR struct file *filep)
{
    FAR struct inode *inode = filep->f_inode;
    FAR struct flash_dev_s *dev = inode->i_private;

    if (!dev) {
        return -ENODEV;
    }

    return OK;
}

/****************************************************************************
 * Name: flash_ioctl
 ****************************************************************************/

static int flash_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
    FAR struct inode *inode = filep->f_inode;
    FAR struct flash_dev_s *dev = inode->i_private;
    int ret = OK;
	lldbg("flash ioctl\n");
    printf("cmd %d\n", cmd);
    if (!dev) {
        return -ENODEV;
    }

    switch (cmd) {
        case FLASHIOC_READ: {
            FAR struct flash_io_s *io = (FAR struct flash_io_s *)arg;

            if (!io || !io->data || io->len == 0) {
                ret = -EINVAL;
                break;
            }

#if defined(CONFIG_AMEBASMART_TRUSTZONE)
            ret = rtl_ss_flash_read(io->address, io->len, io->data, io->en_display);
#elif defined(CONFIG_ARCH_CHIP_BK7239N)
            ret = bk_ss_flash_read(io->address, io->len, io->data, io->en_display);
		lldbg("bk ss flash read \n");
#endif
            break;
        }

        case FLASHIOC_WRITE: {
            FAR struct flash_io_s *io = (FAR struct flash_io_s *)arg;

            if (!io || !io->data || io->len == 0) {
                ret = -EINVAL;
                break;
            }

            if (dev->data && io->address < dev->len) {
                if (io->address + io->len > dev->len) {
                    io->len = dev->len - io->address;
                }
                memcpy(dev->data + io->address, io->data, io->len);
                ret = OK;
            } else {
                ret = -ENODEV;
            }
            break;
        }

        case FLASHIOC_ERASE: {
            FAR struct flash_io_s *io = (FAR struct flash_io_s *)arg;

            if (!io || io->len == 0) {
                ret = -EINVAL;
                break;
            }

            ns_flash_erase(io->address);
            ret = OK;
            break;
        }

        case FLASHIOC_VERIFY_PROTECT: {
            FAR int *protected = (FAR int *)arg;

            if (!protected) {
                ret = -EINVAL;
                break;
            }

#if defined(CONFIG_AMEBASMART_TRUSTZONE)
            *protected = rtl_verify_flash_protect();
#elif defined(CONFIG_ARCH_CHIP_BK7239N)
            *protected = bk_verify_flash_protect();
#endif
            ret = OK;
            break;
        }

        default:
            ret = -ENOTTY;
            break;
    }

    return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: flash_register
 ****************************************************************************/

int flash_register(FAR const char *path, FAR const struct flash_lowerops_s *ops,
                   FAR void *priv)
{
    FAR struct flash_dev_s *dev;
    int ret = OK;


    dev = (FAR struct flash_dev_s *)kmm_malloc(sizeof(struct flash_dev_s));
    if (!dev) {
        return -ENOMEM;
    }

    memset(dev, 0, sizeof(struct flash_dev_s));


    dev->address = 0;
    dev->len = 0;
    dev->en_display = 0;

    ret = register_driver(FLASH_DEV_PATH, &g_flash_fops, 0666, dev);
    if (ret < 0) {
        kmm_free(dev);
        return ret;
    }

    return OK;
}

/****************************************************************************
 * Name: flash_unregister
 ****************************************************************************/

int flash_unregister(FAR const char *path)
{
    FAR struct inode *inode;
    FAR struct flash_dev_s *dev;
    int ret = OK;

    if (!path) {
        return -EINVAL;
    }

    ret = inode_find(path, &inode);
    if (ret < 0) {
        return ret;
    }

    dev = (FAR struct flash_dev_s *)inode->i_private;

    ret = unregister_driver(path);
    if (ret < 0) {
        inode_release(inode);
        return ret;
    }

    kmm_free(dev);
    inode_release(inode);

    return OK;
}

/****************************************************************************
 * Name: flash_read
 ****************************************************************************/

ssize_t flash_read(FAR const char *path, uint32_t address, FAR uint8_t *buffer,
                   uint32_t size)
{
    struct file filep;
    struct flash_io_s io;
    int ret;

    if (!path || !buffer || size == 0) {
        return -EINVAL;
    }

    ret = file_open(&filep, path, O_RDWR);
    if (ret < 0) {
        return ret;
    }

    io.address = address;
    io.len = size;
    io.data = buffer;

    ret = file_ioctl(&filep, FLASHIOC_READ, (unsigned long)&io);

    file_close(&filep);

    if (ret < 0) {
        return ret;
    }

    return size;
}

/****************************************************************************
 * Name: flash_write
 ****************************************************************************/

ssize_t flash_write(FAR const char *path, uint32_t address,
                    FAR const uint8_t *buffer, uint32_t size)
{
    struct file filep;
    struct flash_io_s io;
    int ret;

    if (!path || !buffer || size == 0) {
        return -EINVAL;
    }

    ret = file_open(&filep, path, O_RDWR);
    if (ret < 0) {
        return ret;
    }

    io.address = address;
    io.len = size;
    io.data = (uint8_t *)buffer;

    ret = file_ioctl(&filep, FLASHIOC_WRITE, (unsigned long)&io);

    file_close(&filep);

    if (ret < 0) {
        return ret;
    }

    return size;
}

/****************************************************************************
 * Name: flash_erase
 ****************************************************************************/

int flash_erase(FAR const char *path, uint32_t sector, uint32_t count)
{
    struct file filep;
    struct flash_io_s io;
    int ret;

    if (!path) {
        return -EINVAL;
    }

    ret = file_open(&filep, path, O_RDWR);
    if (ret < 0) {
        return ret;
    }

    io.address = sector;
    io.len = count;
    io.data = NULL;

    ret = file_ioctl(&filep, FLASHIOC_ERASE, (unsigned long)&io);

    file_close(&filep);

    return ret;
}
