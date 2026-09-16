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
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <tinyara/logctl.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_LOGCTL

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Runtime enable flags for each module */
static bool g_module_enabled[LOGCTL_MODULE_MAX];

/* Module names for status display */
static const char *g_module_names[] = {
	[LOGCTL_MODULE_COMMON] = "common",
	[LOGCTL_MODULE_AUDIO] = "audio",
	[LOGCTL_MODULE_BINFMT] = "binfmt",
	[LOGCTL_MODULE_BINARY_COMPRESSION] = "binary_compression",
	[LOGCTL_MODULE_BINMGR] = "binmgr",
	[LOGCTL_MODULE_FS] = "fs",
	[LOGCTL_MODULE_LIB] = "lib",
	[LOGCTL_MODULE_LOGDUMP] = "logdump",
	[LOGCTL_MODULE_MM] = "mm",
	[LOGCTL_MODULE_NET] = "net",
	[LOGCTL_MODULE_BLE] = "ble",
	[LOGCTL_MODULE_SMP] = "smp",
	[LOGCTL_MODULE_PM] = "pm",
	[LOGCTL_MODULE_SCHED] = "sched",
	[LOGCTL_MODULE_SYSCALL] = "syscall",
	[LOGCTL_MODULE_TASH] = "tash",
	[LOGCTL_MODULE_WIFICSI] = "wificsi",
	[LOGCTL_MODULE_EVENTLOOP] = "eventloop",
	[LOGCTL_MODULE_MEDIA] = "media",
	[LOGCTL_MODULE_PREFERENCE] = "preference",
	[LOGCTL_MODULE_ST_THINGS] = "st_things",
	[LOGCTL_MODULE_TASK_MANAGER] = "task_manager",
	[LOGCTL_MODULE_REBOOT_REASON] = "reboot_reason",
	[LOGCTL_MODULE_UI] = "ui",
	[LOGCTL_MODULE_ANALOG] = "analog",
	[LOGCTL_MODULE_I2S] = "i2s",
	[LOGCTL_MODULE_MIPI_DSI] = "mipi_dsi",
	[LOGCTL_MODULE_LCD] = "lcd",
	[LOGCTL_MODULE_SPI] = "spi",
	[LOGCTL_MODULE_TIMER] = "timer",
	[LOGCTL_MODULE_TOUCH] = "touch",
	[LOGCTL_MODULE_USB] = "usb",
	[LOGCTL_MODULE_VIDEO] = "video",
	[LOGCTL_MODULE_WLAN] = "wlan",
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void logctl_init(void)
{
	int i;

	/* Start with all modules disabled by default */
	for (i = 0; i < LOGCTL_MODULE_MAX; i++) {
		g_module_enabled[i] = false;
	}

	/* Enable modules based on compile-time CONFIG_DEBUG_* settings.
	 * Only modules whose debug config is enabled at build time will
	 * have logging enabled at boot. User can later enable/disable
	 * any module at runtime via TASH commands.
	 */
#ifdef CONFIG_DEBUG
	g_module_enabled[LOGCTL_MODULE_COMMON] = true;
#endif
#ifdef CONFIG_DEBUG_AUDIO
	g_module_enabled[LOGCTL_MODULE_AUDIO] = true;
#endif
#ifdef CONFIG_DEBUG_BINFMT
	g_module_enabled[LOGCTL_MODULE_BINFMT] = true;
#endif
#ifdef CONFIG_DEBUG_BINARY_COMPRESSION
	g_module_enabled[LOGCTL_MODULE_BINARY_COMPRESSION] = true;
#endif
#ifdef CONFIG_DEBUG_BINMGR
	g_module_enabled[LOGCTL_MODULE_BINMGR] = true;
#endif
#ifdef CONFIG_DEBUG_FS
	g_module_enabled[LOGCTL_MODULE_FS] = true;
#endif
#ifdef CONFIG_DEBUG_LIB
	g_module_enabled[LOGCTL_MODULE_LIB] = true;
#endif
#ifdef CONFIG_DEBUG_LOGDUMP
	g_module_enabled[LOGCTL_MODULE_LOGDUMP] = true;
#endif
#ifdef CONFIG_DEBUG_MM
	g_module_enabled[LOGCTL_MODULE_MM] = true;
#endif
#ifdef CONFIG_DEBUG_NET
	g_module_enabled[LOGCTL_MODULE_NET] = true;
#endif
#ifdef CONFIG_DEBUG_BLE
	g_module_enabled[LOGCTL_MODULE_BLE] = true;
#endif
#ifdef CONFIG_DEBUG_SMP
	g_module_enabled[LOGCTL_MODULE_SMP] = true;
#endif
#ifdef CONFIG_DEBUG_PM
	g_module_enabled[LOGCTL_MODULE_PM] = true;
#endif
#ifdef CONFIG_DEBUG_SCHED
	g_module_enabled[LOGCTL_MODULE_SCHED] = true;
#endif
#ifdef CONFIG_DEBUG_SYSCALL
	g_module_enabled[LOGCTL_MODULE_SYSCALL] = true;
#endif
#ifdef CONFIG_DEBUG_TASH
	g_module_enabled[LOGCTL_MODULE_TASH] = true;
#endif
#ifdef CONFIG_DEBUG_WIFICSI
	g_module_enabled[LOGCTL_MODULE_WIFICSI] = true;
#endif
#ifdef CONFIG_DEBUG_EVENTLOOP
	g_module_enabled[LOGCTL_MODULE_EVENTLOOP] = true;
#endif
#ifdef CONFIG_DEBUG_MEDIA
	g_module_enabled[LOGCTL_MODULE_MEDIA] = true;
#endif
#ifdef CONFIG_DEBUG_PREFERENCE
	g_module_enabled[LOGCTL_MODULE_PREFERENCE] = true;
#endif
#ifdef CONFIG_DEBUG_ST_THINGS
	g_module_enabled[LOGCTL_MODULE_ST_THINGS] = true;
#endif
#ifdef CONFIG_DEBUG_TASK_MANAGER
	g_module_enabled[LOGCTL_MODULE_TASK_MANAGER] = true;
#endif
#ifdef CONFIG_DEBUG_REBOOT_REASON
	g_module_enabled[LOGCTL_MODULE_REBOOT_REASON] = true;
#endif
#ifdef CONFIG_DEBUG_UI
	g_module_enabled[LOGCTL_MODULE_UI] = true;
#endif
#ifdef CONFIG_DEBUG_ANALOG
	g_module_enabled[LOGCTL_MODULE_ANALOG] = true;
#endif
#ifdef CONFIG_DEBUG_I2S
	g_module_enabled[LOGCTL_MODULE_I2S] = true;
#endif
#ifdef CONFIG_DEBUG_MIPI_DSI
	g_module_enabled[LOGCTL_MODULE_MIPI_DSI] = true;
#endif
#ifdef CONFIG_DEBUG_LCD
	g_module_enabled[LOGCTL_MODULE_LCD] = true;
#endif
#ifdef CONFIG_DEBUG_SPI
	g_module_enabled[LOGCTL_MODULE_SPI] = true;
#endif
#ifdef CONFIG_DEBUG_TIMER
	g_module_enabled[LOGCTL_MODULE_TIMER] = true;
#endif
#ifdef CONFIG_DEBUG_TOUCH
	g_module_enabled[LOGCTL_MODULE_TOUCH] = true;
#endif
#ifdef CONFIG_DEBUG_USB
	g_module_enabled[LOGCTL_MODULE_USB] = true;
#endif
#ifdef CONFIG_DEBUG_VIDEO
	g_module_enabled[LOGCTL_MODULE_VIDEO] = true;
#endif
#ifdef CONFIG_DEBUG_WLAN
	g_module_enabled[LOGCTL_MODULE_WLAN] = true;
#endif
}

int logctl_enable(enum logctl_module_e module)
{
	if (module >= LOGCTL_MODULE_MAX) {
		return -EINVAL;
	}
	g_module_enabled[module] = true;
	return 0;
}

int logctl_disable(enum logctl_module_e module)
{
	if (module >= LOGCTL_MODULE_MAX) {
		return -EINVAL;
	}
	g_module_enabled[module] = false;
	return 0;
}

bool logctl_is_enabled(enum logctl_module_e module)
{
	if (module >= LOGCTL_MODULE_MAX) {
		return false;
	}
	return g_module_enabled[module];
}

void logctl_enable_all(void)
{
	int i;
	for (i = 0; i < LOGCTL_MODULE_MAX; i++) {
		g_module_enabled[i] = true;
	}
}

void logctl_disable_all(void)
{
	int i;
	for (i = 0; i < LOGCTL_MODULE_MAX; i++) {
		g_module_enabled[i] = false;
	}
}

int logctl_get_status(char *buffer, int bufsize)
{
	int offset = 0;
	int i;
	
	offset += snprintf(buffer + offset, bufsize - offset, "Module Status:\n");
	offset += snprintf(buffer + offset, bufsize - offset, "----------------\n");
	
	for (i = 0; i < LOGCTL_MODULE_MAX; i++) {
		if (g_module_names[i] != NULL) {
			offset += snprintf(buffer + offset, bufsize - offset, "%-20s: %s\n", 
				g_module_names[i], g_module_enabled[i] ? "enabled" : "disabled");
		}
	}
	
	return offset;
}

#endif /* CONFIG_LOGCTL */
