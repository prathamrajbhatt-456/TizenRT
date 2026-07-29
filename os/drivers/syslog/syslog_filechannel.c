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
 * drivers/syslog/syslog_filechannel.c
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <tinyara/syslog/syslog.h>
#include <tinyara/kmalloc.h>
#include <tinyara/fs/fs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define OPEN_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define OPEN_MODE  (S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_SYSLOG_FILE_SEPARATE
/****************************************************************************
 * Name: log_separate
 *
 * Description:
 *   Add a separator line to the log file to distinguish between different
 *   log sessions (e.g., system reboots).
 *
 ****************************************************************************/

static void log_separate(FAR const char *log_file)
{
	struct file fp;

	if (file_open(&fp, log_file, (O_WRONLY | O_APPEND | O_CLOEXEC)) < 0) {
		return;
	}

	file_write(&fp, "\n\n", 2);

	file_close(&fp);
}
#endif

#if CONFIG_SYSLOG_FILE_ROTATIONS > 0
/****************************************************************************
 * Name: log_rotate
 *
 * Description:
 *   Rotate the log file if it exceeds the size limit.
 *
 ****************************************************************************/

static void log_rotate(FAR const char *log_file)
{
	int i;
	off_t size;
	struct stat f_stat;
	size_t name_size;
	FAR char *rotate_to;
	FAR char *rotate_from;

	/* Get the size of the current log file. */

	if (stat(log_file, &f_stat) < 0) {
		return;
	}

	size = f_stat.st_size;

	/* If it does not exceed the limit we are OK. */

	if (size < CONFIG_SYSLOG_FILE_SIZE_LIMIT) {
		return;
	}

	/* Rotated file names. */

	name_size = strlen(log_file) + 8;
	rotate_to = kmm_malloc(name_size);
	rotate_from = kmm_malloc(name_size);
	if ((rotate_to == NULL) || (rotate_from == NULL)) {
		goto end;
	}

	/* Rotate the logs. */

	for (i = (CONFIG_SYSLOG_FILE_ROTATIONS - 1); i > 0; i--) {
		snprintf(rotate_to, name_size, "%s.%d", log_file, i);
		snprintf(rotate_from, name_size, "%s.%d", log_file, i - 1);

		rename(rotate_from, rotate_to);
	}

	snprintf(rotate_to, name_size, "%s.0", log_file);

	rename(log_file, rotate_to);

end:
	kmm_free(rotate_to);
	kmm_free(rotate_from);
}
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: syslog_file_channel
 *
 * Description:
 *   Configure to use a file in a mounted file system at 'devpath' as the
 *   SYSLOG channel.
 *
 *   This function sets up file-based logging by:
 *   - Rotating the log file if rotation is enabled and size limit exceeded
 *   - Adding a separator between log sessions if configured
 *   - Opening the file for append-mode writing
 *
 *   File SYSLOG channels differ from other SYSLOG channels in that they
 *   cannot be established until after fully booting and mounting the target
 *   file system.  This function would need to be called from board-specific
 *   bring-up logic AFTER mounting the file system containing 'devpath'.
 *
 *   SYSLOG data generated prior to calling syslog_file_channel will, of
 *   course, not be included in the file.
 *
 *   NOTE: interrupt level SYSLOG output will be lost in this case unless
 *   the interrupt buffer is used.
 *
 * Input Parameters:
 *   devpath - The full path to the file to be used for SYSLOG output.
 *     This may be an existing file or not.  If the file exists,
 *     syslog_file_channel() will append new SYSLOG data to the end of the
 *     file.  If it does not, then syslog_file_channel() will create the
 *     file.
 *
 * Returned Value:
 *   Returns OK (0) on success; A negated errno value is returned on failure.
 *
 ****************************************************************************/

#ifdef CONFIG_SYSLOG_FILE
int syslog_file_channel(FAR const char *devpath)
{
	irqstate_t flags;
	int ret;

	/* Validate input parameters */

	if (devpath == NULL) {
		return -EINVAL;
	}

	/* Disable pre-emption to prevent re-entry while we configure the
	 * file channel.
	 */

	flags = enter_critical_section();

	/* Store the file path for use by syslog_reopen() and syslog_putc() */

	g_syslog_file_path = devpath;

	/* Rotate the log file, if needed. */

#if CONFIG_SYSLOG_FILE_ROTATIONS > 0
	log_rotate(devpath);
#endif

	/* Separate the old log entries. */

#ifdef CONFIG_SYSLOG_FILE_SEPARATE
	log_separate(devpath);
#endif

	/* Re-open syslog with the new file path. This will close the existing
	 * syslog device (if any) and open the file for logging.
	 */

	ret = syslog_reopen();

	leave_critical_section(flags);

	return ret;
}
#endif /* CONFIG_SYSLOG_FILE */
