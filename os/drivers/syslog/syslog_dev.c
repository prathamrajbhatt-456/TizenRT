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

/****************************************************************************
 * drivers/syslog/syslog_dev.c
 *
 * Character device driver for syslog control via ioctl
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <debug.h>

#include <tinyara/fs/fs.h>
#include <tinyara/kmalloc.h>
#include <tinyara/syslog/syslog.h>
#include <tinyara/syslog/syslog_ioctl.h>
#include <tinyara/syslog/syslog_dev.h>

/* Debug macros - use printf since syslogerr/sysloginfo may not be available */
#ifndef syslogerr
#define syslogerr(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif
#ifndef sysloginfo
#define sysloginfo(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SYSLOG_DEV_NAME "syslog"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Syslog device private data */

typedef struct syslog_dev_s {
	int open_count;
	sem_t lock;
} syslog_dev_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

static syslog_dev_t g_syslog_dev = {
	.open_count = 0,
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int syslog_dev_open(FAR struct file *filep);
static int syslog_dev_close(FAR struct file *filep);
static ssize_t syslog_dev_read(FAR struct file *filep, FAR char *buffer, size_t buflen);
static ssize_t syslog_dev_write(FAR struct file *filep, const char *buffer, size_t buflen);
static int syslog_dev_ioctl(FAR struct file *filep, int cmd, unsigned long arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: syslog_dev_open
 ****************************************************************************/

static int syslog_dev_open(FAR struct file *filep)
{
	int ret = OK;

	ret = sem_wait(&g_syslog_dev.lock);
	if (ret < 0) {
		return -get_errno();
	}

	g_syslog_dev.open_count++;
	filep->f_priv = &g_syslog_dev;

	sem_post(&g_syslog_dev.lock);
	return OK;
}

/****************************************************************************
 * Name: syslog_dev_close
 ****************************************************************************/

static int syslog_dev_close(FAR struct file *filep)
{
	int ret = OK;

	ret = sem_wait(&g_syslog_dev.lock);
	if (ret < 0) {
		return -get_errno();
	}

	if (g_syslog_dev.open_count > 0) {
		g_syslog_dev.open_count--;
	}

	filep->f_priv = NULL;

	sem_post(&g_syslog_dev.lock);
	return OK;
}

/****************************************************************************
 * Name: syslog_dev_read
 ****************************************************************************/

static ssize_t syslog_dev_read(FAR struct file *filep, FAR char *buffer, size_t buflen)
{
	/* Read not supported for syslog control device */
	return -ENOSYS;
}

/****************************************************************************
 * Name: syslog_dev_write
 ****************************************************************************/

static ssize_t syslog_dev_write(FAR struct file *filep, const char *buffer, size_t buflen)
{
	/* Write not supported for syslog control device */
	return -ENOSYS;
}

/****************************************************************************
 * Name: syslog_dev_ioctl
 ****************************************************************************/

static int syslog_dev_ioctl(FAR struct file *filep, int cmd, unsigned long arg)
{
	int ret = OK;

	switch (cmd) {
#ifdef CONFIG_SYSLOG_FILE
	case SYSLOGIOC_START_CAPTURE: {
		syslog_capture_start_t *capture_arg;
		
		if (arg == 0) {
			return -EINVAL;
		}
		
		capture_arg = (syslog_capture_start_t *)arg;
		
		/* Validate path */
		if (capture_arg->path[0] == '\0') {
			return -EINVAL;
		}
		
		ret = syslog_capture_start(capture_arg->path);
		break;
	}

	case SYSLOGIOC_STOP_CAPTURE: {
		ret = syslog_capture_stop();
		break;
	}

	case SYSLOGIOC_GET_STATUS: {
		bool *status;
		
		if (arg == 0) {
			return -EINVAL;
		}
		
		status = (bool *)arg;
		*status = syslog_is_capturing();
		break;
	}
#endif /* CONFIG_SYSLOG_FILE */

	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* File operations structure */

static const struct file_operations g_syslog_fops = {
	syslog_dev_open,    /* open */
	syslog_dev_close,   /* close */
	syslog_dev_read,    /* read */
	syslog_dev_write,   /* write */
	NULL,               /* seek */
	syslog_dev_ioctl,   /* ioctl */
#ifndef CONFIG_DISABLE_POLL
	NULL,               /* poll */
#endif
#ifndef CONFIG_DISABLE_PSEUDOFS_OPERATIONS
	NULL,               /* unlink */
#endif
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: syslog_devinitialize
 ****************************************************************************/

int syslog_devinitialize(void)
{
	int ret;

	/* Initialize semaphore */
	sem_init(&g_syslog_dev.lock, 0, 1);

	/* Register the character device */
	ret = register_driver(SYSLOG_DEVICE_PATH, &g_syslog_fops, 0666, NULL);
	if (ret < 0) {
		syslogerr("Failed to register syslog device driver: %d\n", ret);
		return ret;
	}

	sysloginfo("Syslog device driver registered at %s\n", SYSLOG_DEVICE_PATH);
	return OK;
}
