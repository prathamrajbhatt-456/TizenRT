/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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
 * apps/system/capture_logs/capture_logs.c
 *
 * TASH command to control syslog capture to file via ioctl
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/stat.h>

#include <tinyara/syslog/syslog_ioctl.h>

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

#define DEFAULT_LOG_PATH "/mnt/logs/syslog.txt"
#define DEFAULT_LOG_DIR "/mnt/logs"
#define SYSLOG_DEVICE_PATH "/dev/syslog"

/****************************************************************************
 * Private Function: ensure_log_directory
 ****************************************************************************/

static int ensure_log_directory(void)
{
	struct stat st;

	if (stat(DEFAULT_LOG_DIR, &st) == 0) {
		/* Directory already exists */
		return OK;
	}

	/* Create the directory */
	if (mkdir(DEFAULT_LOG_DIR, 0777) != 0) {
		printf("Warning: Failed to create %s directory (errno: %d)\n", DEFAULT_LOG_DIR, errno);
		/* Continue anyway - syslog_file_channel may still work */
	}
	return OK;
}

/****************************************************************************
 * Private Function: capture_logs_usage
 ****************************************************************************/

static void capture_logs_usage(void)
{
	printf("Usage: capture_logs <start|stop|status>\n");
	printf("  start  - Start capturing syslogs to %s\n", DEFAULT_LOG_PATH);
	printf("  stop   - Stop capturing syslogs\n");
	printf("  status - Show current capture status\n");
}

/****************************************************************************
 * Public Function: capture_logs_main
 ****************************************************************************/

int capture_logs_main(int argc, char **argv)
{
	int fd = -1;
	int ret;
	syslog_capture_start_t capture_arg;

	if (argc < 2) {
		capture_logs_usage();
		return ERROR;
	}

	/* Open the syslog control device */
	fd = open(SYSLOG_DEVICE_PATH, O_RDWR);
	if (fd < 0) {
		printf("Failed to open %s device (errno: %d)\n", SYSLOG_DEVICE_PATH, errno);
		printf("Make sure CONFIG_SYSLOG_FILE and syslog device driver are enabled\n");
		return ERROR;
	}

	if (strcmp(argv[1], "start") == 0) {
		/* Ensure the log directory exists */
		ensure_log_directory();

		/* Prepare capture arguments */
		strncpy(capture_arg.path, DEFAULT_LOG_PATH, SYSLOG_MAX_PATH - 1);
		capture_arg.path[SYSLOG_MAX_PATH - 1] = '\0';

		printf("Starting syslog capture to %s...\n", DEFAULT_LOG_PATH);
		ret = ioctl(fd, SYSLOGIOC_START_CAPTURE, (unsigned long)&capture_arg);
		if (ret < 0) {
			printf("Failed to start syslog capture (error: %d)\n", ret);
			close(fd);
			return ERROR;
		}
		printf("Syslog capture started successfully\n");
	} else if (strcmp(argv[1], "stop") == 0) {
		printf("Stopping syslog capture...\n");
		ret = ioctl(fd, SYSLOGIOC_STOP_CAPTURE, 0);
		if (ret < 0) {
			printf("Failed to stop syslog capture (error: %d)\n", ret);
			close(fd);
			return ERROR;
		}
		printf("Syslog capture stopped successfully\n");
	} else if (strcmp(argv[1], "status") == 0) {
		bool status = false;

		ret = ioctl(fd, SYSLOGIOC_GET_STATUS, (unsigned long)&status);
		if (ret < 0) {
			printf("Failed to get capture status (error: %d)\n", ret);
			close(fd);
			return ERROR;
		}

		if (status) {
			printf("Syslog capture is RUNNING\n");
			printf("Log file: %s\n", DEFAULT_LOG_PATH);
		} else {
			printf("Syslog capture is STOPPED\n");
		}
	} else {
		close(fd);
		capture_logs_usage();
		return ERROR;
	}

	close(fd);
	return OK;
}

#ifdef CONFIG_BUILD_KERNEL
/****************************************************************************
 * Public Function: capture_logs_register
 ****************************************************************************/

void capture_logs_register(void)
{
	/* This function is called during system initialization */
}
#endif
