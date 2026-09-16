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
 * examples/hello/hello_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
#include <stdio.h>
#include <syslog.h>
#include <debug.h>
#include <tinyara/logctl.h>

/****************************************************************************
 * hello_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int hello_main(int argc, char *argv[])
#endif
{
	printf("Hello, World!!\n");

	/* ============================================================
	 * LOGCTL Filtering Test
	 * ============================================================
	 * The following messages go through different log paths.
	 * All of them check logctl_is_enabled(LOGCTL_MODULE_COMMON).
	 *
	 * Test procedure:
	 *   1. Run "hello" with common ENABLED → all messages appear
	 *   2. "logctl disable common" then "hello" → syslog/logm messages
	 *      disappear, printf stays
	 *   3. "logctl enable common" then "hello" → all messages reappear
	 * ============================================================ */

	printf("\n=== LOGCTL Filtering Test ===\n");

	/* --- syslog() path (lib_syslog.c → vsyslog) --- */
	syslog(LOG_INFO, "[syslog] INFO message - filtered by logctl\n");
	syslog(LOG_DEBUG, "[syslog] DEBUG message - filtered by logctl\n");
	syslog(LOG_ERR, "[syslog] ERROR message - filtered by logctl\n");

	/* --- lowsyslog() path (lib_lowsyslog.c → lowvsyslog) --- */
	lowsyslog(LOG_INFO, "[lowsyslog] INFO message - filtered by logctl\n");
	lowsyslog(LOG_ERR, "[lowsyslog] ERROR message - filtered by logctl\n");

	/* --- dbg() macro path ---
	 * When CONFIG_LOGM is enabled: dbg() → logm() → logm_internal() [has logctl check]
	 * When CONFIG_LOGM is disabled: dbg() → syslog() [has logctl check]
	 * Either way, filtered by logctl.
	 */
	dbg("[dbg] ERROR-level debug message - filtered by logctl\n");
	wdbg("[wdbg] WARN-level debug message - filtered by logctl\n");
	vdbg("[vdbg] INFO-level debug message - filtered by logctl\n");

	/* --- Module-specific debug macros ---
	 * These also go through dbg()/wdbg()/vdbg() → syslog or logm.
	 * They are filtered by logctl (checks LOGCTL_MODULE_COMMON).
	 */

#ifdef CONFIG_DEBUG_MM_ERROR
	mdbg("[mdbg] MM ERROR debug message - filtered by logctl\n");
#endif
#ifdef CONFIG_DEBUG_FS_ERROR
	fdbg("[fdbg] FS ERROR debug message - filtered by logctl\n");
#endif
#ifdef CONFIG_DEBUG_FS_WARN
	fwdbg("[fWdbg] FS WARN debug message - filtered by logctl\n");
#endif
#ifdef CONFIG_DEBUG_NET_ERROR
	ndbg("[ndbg] NET ERROR debug message - filtered by logctl\n");
#endif
#ifdef CONFIG_DEBUG_BLE_ERROR
	bledbg("[bledbg] BLE ERROR debug message - filtered by logctl\n");
#endif
#ifdef CONFIG_DEBUG_SMP_ERROR
	smpdbg("[smpdbg] SMP ERROR debug message - filtered by logctl\n");
#endif
#ifdef CONFIG_DEBUG_PM_ERROR
	pmdbg("[pmdbg] PM ERROR debug message - filtered by logctl\n");
#endif

	/* --- printf() path - NOT filtered by logctl --- */
	printf("[printf] This message is NOT filtered by logctl\n");

	/* ============================================================
	 * Per-Module Enable/Disable Test
	 * ============================================================
	 * Each module's log is printed only if that specific module
	 * is enabled via logctl. Use "logctl disable <module>" to
	 * suppress individual modules.
	 *
	 * Test:
	 *   "logctl disable mm"   → mm line disappears
	 *   "logctl disable fs"    → fs line disappears
	 *   "logctl disable net"   → net line disappears
	 *   "logctl disable_all"   → all lines disappear
	 *   "logctl enable_all"    → all lines reappear
	 * ============================================================ */
	printf("\n--- Per-Module Filtering Test ---\n");

	if (logctl_is_enabled(LOGCTL_MODULE_COMMON)) {
		printf("[common] Common module log - 'logctl disable common' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_MM)) {
		printf("[mm] Memory management log - 'logctl disable mm' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_FS)) {
		printf("[fs] File system log - 'logctl disable fs' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_NET)) {
		printf("[net] Network log - 'logctl disable net' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_AUDIO)) {
		printf("[audio] Audio log - 'logctl disable audio' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_BLE)) {
		printf("[ble] BLE log - 'logctl disable ble' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_SCHED)) {
		printf("[sched] Scheduler log - 'logctl disable sched' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_PM)) {
		printf("[pm] Power management log - 'logctl disable pm' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_TASH)) {
		printf("[tash] TASH log - 'logctl disable tash' to suppress\n");
	}
	if (logctl_is_enabled(LOGCTL_MODULE_WLAN)) {
		printf("[wlan] WLAN log - 'logctl disable wlan' to suppress\n");
	}

	printf("\n=== End of LOGCTL Test ===\n\n");

	return 0;
}
