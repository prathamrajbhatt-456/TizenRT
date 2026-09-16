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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <apps/shell/tash.h>
#include <tinyara/logctl.h>

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

/* ---- TASH command registration ---- */

static int logctl_tash(int argc, char **args);

const static tash_cmdlist_t logctl_tashcmds[] = {
	{"logctl", logctl_tash, TASH_EXECMD_SYNC},
	{NULL, NULL, 0}
};

void logctl_register_tashcmds(void)
{
	tash_cmdlist_install(logctl_tashcmds);
}

/* ---- TASH command handler ---- */

static void logctl_usage(void)
{
	fprintf(stdout, "[LOGCTL USAGE]\n");
	fprintf(stdout, "usage: logctl <command> [module_name]\n");
	fprintf(stdout, "\ncommands:\n");
	fprintf(stdout, "    list                    - Show all modules status\n");
	fprintf(stdout, "    enable <module>         - Enable logging for a module\n");
	fprintf(stdout, "    disable <module>        - Disable logging for a module\n");
	fprintf(stdout, "    enable_all              - Enable logging for all modules\n");
	fprintf(stdout, "    disable_all             - Disable logging for all modules\n");
	fprintf(stdout, "\nmodule names:\n");
	fprintf(stdout, "    common, audio, binfmt, binary_compression, binmgr, fs,\n");
	fprintf(stdout, "    lib, logdump, mm, net, ble, smp, pm, sched, syscall,\n");
	fprintf(stdout, "    tash, wificsi, eventloop, media, preference, st_things,\n");
	fprintf(stdout, "    task_manager, reboot_reason, ui, analog, i2s,\n");
	fprintf(stdout, "    mipi_dsi, lcd, spi, timer, touch, usb, video, wlan\n");
}

static int logctl_find_module(const char *name)
{
	if (strcmp(name, "common") == 0) return LOGCTL_MODULE_COMMON;
	if (strcmp(name, "audio") == 0) return LOGCTL_MODULE_AUDIO;
	if (strcmp(name, "binfmt") == 0) return LOGCTL_MODULE_BINFMT;
	if (strcmp(name, "binary_compression") == 0) return LOGCTL_MODULE_BINARY_COMPRESSION;
	if (strcmp(name, "binmgr") == 0) return LOGCTL_MODULE_BINMGR;
	if (strcmp(name, "fs") == 0) return LOGCTL_MODULE_FS;
	if (strcmp(name, "lib") == 0) return LOGCTL_MODULE_LIB;
	if (strcmp(name, "logdump") == 0) return LOGCTL_MODULE_LOGDUMP;
	if (strcmp(name, "mm") == 0) return LOGCTL_MODULE_MM;
	if (strcmp(name, "net") == 0) return LOGCTL_MODULE_NET;
	if (strcmp(name, "ble") == 0) return LOGCTL_MODULE_BLE;
	if (strcmp(name, "smp") == 0) return LOGCTL_MODULE_SMP;
	if (strcmp(name, "pm") == 0) return LOGCTL_MODULE_PM;
	if (strcmp(name, "sched") == 0) return LOGCTL_MODULE_SCHED;
	if (strcmp(name, "syscall") == 0) return LOGCTL_MODULE_SYSCALL;
	if (strcmp(name, "tash") == 0) return LOGCTL_MODULE_TASH;
	if (strcmp(name, "wificsi") == 0) return LOGCTL_MODULE_WIFICSI;
	if (strcmp(name, "eventloop") == 0) return LOGCTL_MODULE_EVENTLOOP;
	if (strcmp(name, "media") == 0) return LOGCTL_MODULE_MEDIA;
	if (strcmp(name, "preference") == 0) return LOGCTL_MODULE_PREFERENCE;
	if (strcmp(name, "st_things") == 0) return LOGCTL_MODULE_ST_THINGS;
	if (strcmp(name, "task_manager") == 0) return LOGCTL_MODULE_TASK_MANAGER;
	if (strcmp(name, "reboot_reason") == 0) return LOGCTL_MODULE_REBOOT_REASON;
	if (strcmp(name, "ui") == 0) return LOGCTL_MODULE_UI;
	if (strcmp(name, "analog") == 0) return LOGCTL_MODULE_ANALOG;
	if (strcmp(name, "i2s") == 0) return LOGCTL_MODULE_I2S;
	if (strcmp(name, "mipi_dsi") == 0) return LOGCTL_MODULE_MIPI_DSI;
	if (strcmp(name, "lcd") == 0) return LOGCTL_MODULE_LCD;
	if (strcmp(name, "spi") == 0) return LOGCTL_MODULE_SPI;
	if (strcmp(name, "timer") == 0) return LOGCTL_MODULE_TIMER;
	if (strcmp(name, "touch") == 0) return LOGCTL_MODULE_TOUCH;
	if (strcmp(name, "usb") == 0) return LOGCTL_MODULE_USB;
	if (strcmp(name, "video") == 0) return LOGCTL_MODULE_VIDEO;
	if (strcmp(name, "wlan") == 0) return LOGCTL_MODULE_WLAN;
	return -1;
}

static int logctl_list(int argc, char **args)
{
	char buffer[2048];
	logctl_get_status(buffer, sizeof(buffer));
	fprintf(stdout, "%s\n", buffer);
	return 0;
}

static int logctl_enable_cmd(int argc, char **args)
{
	int module;

	if (argc < 3) {
		fprintf(stderr, "Error: module name required\n");
		fprintf(stderr, "Usage: logctl enable <module_name>\n");
		return -1;
	}

	module = logctl_find_module(args[2]);
	if (module < 0) {
		fprintf(stderr, "Error: unknown module '%s'\n", args[2]);
		return -1;
	}

	logctl_enable((enum logctl_module_e)module);
	fprintf(stdout, "Enabled logging for module '%s'\n", args[2]);
	return 0;
}

static int logctl_disable_cmd(int argc, char **args)
{
	int module;

	if (argc < 3) {
		fprintf(stderr, "Error: module name required\n");
		fprintf(stderr, "Usage: logctl disable <module_name>\n");
		return -1;
	}

	module = logctl_find_module(args[2]);
	if (module < 0) {
		fprintf(stderr, "Error: unknown module '%s'\n", args[2]);
		return -1;
	}

	logctl_disable((enum logctl_module_e)module);
	fprintf(stdout, "Disabled logging for module '%s'\n", args[2]);
	return 0;
}

static int logctl_enable_all_cmd(int argc, char **args)
{
	logctl_enable_all();
	fprintf(stdout, "Enabled logging for all modules\n");
	return 0;
}

static int logctl_disable_all_cmd(int argc, char **args)
{
	logctl_disable_all();
	fprintf(stdout, "Disabled logging for all modules\n");
	return 0;
}

static int logctl_tash(int argc, char **args)
{
	if (argc < 2) {
		logctl_usage();
		return 0;
	}

	if (strcmp(args[1], "--help") == 0 || strcmp(args[1], "-h") == 0) {
		logctl_usage();
		return 0;
	}

	if (strcmp(args[1], "list") == 0) {
		return logctl_list(argc, args);
	}

	if (strcmp(args[1], "enable") == 0) {
		return logctl_enable_cmd(argc, args);
	}

	if (strcmp(args[1], "disable") == 0) {
		return logctl_disable_cmd(argc, args);
	}

	if (strcmp(args[1], "enable_all") == 0) {
		return logctl_enable_all_cmd(argc, args);
	}

	if (strcmp(args[1], "disable_all") == 0) {
		return logctl_disable_all_cmd(argc, args);
	}

	fprintf(stderr, "Error: unknown command '%s'\n", args[1]);
	logctl_usage();
	return -1;
}
