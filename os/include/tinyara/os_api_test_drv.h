/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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

#ifndef __INCLUDE_TINYARA_OS_API_TEST_DRV_H
#define __INCLUDE_TINYARA_OS_API_TEST_DRV_H

/* This file will be used to provide definitions to support
 * OS API test case framework
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdint.h>
#include <sys/types.h>
#include <tinyara/fs/ioctl.h>

#ifdef CONFIG_DRIVERS_OS_API_TEST

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration
 *
 * CONFIG_DRIVERS_OS_API_TEST - Enables OS API test driver support
 */

/* IOCTL Commands */
/* The OS API test module uses ioctl commands to identify which
 * test cases are to be run. The ioctl command may be accompanied by and arguement to
 * indicate which particular API  in the module is to be tested or which particular
 * test scenario is to be run
 *
 * TESTIOC_TEST_DRIVER_ANALOG - Run test cases for os/drivers/analog module
 *
 *   ioctl argument:  Integer (enum or DEFINE value) indicating the particular test case that is to be run
 *
 */

#define TESTIOC_ANALOG                         _TESTIOC(1)
#define TESTIOC_GET_SIG_FINDACTION_ADD         _TESTIOC(2)
#define TESTIOC_GET_SELF_PID                   _TESTIOC(3)
#define TESTIOC_IS_ALIVE_THREAD                _TESTIOC(4)
#define TESTIOC_GET_TCB_SIGPROCMASK            _TESTIOC(5)
#define TESTIOC_GET_TCB_ADJ_STACK_SIZE         _TESTIOC(6)
#define TESTIOC_SCHED_FOREACH                  _TESTIOC(8)
#define TESTIOC_SIGNAL_PAUSE                   _TESTIOC(9)
#define TESTIOC_CLOCK_ABSTIME2TICKS_TEST       _TESTIOC(10)
#define TESTIOC_TIMER_INITIALIZE_TEST          _TESTIOC(11)
#define TESTIOC_SEM_TICK_WAIT_TEST             _TESTIOC(12)
#define TESTIOC_TASK_REPARENT                  _TESTIOC(13)
#if defined(CONFIG_SCHED_HAVE_PARENT) && defined(CONFIG_SCHED_CHILD_STATUS)
#define TESTIOC_GROUP_ADD_FINED_REMOVE_TEST    _TESTIOC(14)
#define TESTIOC_GROUP_ALLOC_FREE_TEST          _TESTIOC(15)
#define TESTIOC_GROUP_EXIT_CHILD_TEST          _TESTIOC(16)
#define TESTIOC_GROUP_REMOVECHILDREN_TEST      _TESTIOC(17)
#endif
#define TESTIOC_TASK_INIT_TEST                 _TESTIOC(18)
#define TESTIOC_COMPRESSION_TEST	        _TESTIOC(19)
#ifdef CONFIG_EXAMPLES_MEM_PROTECT_TEST
#define TESTIOC_MEM_PROTECTTEST			_TESTIOC(20)
#endif
#ifdef CONFIG_ARMV8M_TRUSTZONE
#define TESTIOC_TZ				_TESTIOC(21)
#endif
#ifdef CONFIG_EXAMPLES_STACK_PROTECTION
#define TESTIOC_KTHREAD_STACK_PROTECTION_TEST	_TESTIOC(22)
#endif
#ifdef CONFIG_TC_NET_PBUF
#define TESTIOC_NET_PBUF			_TESTIOC(23)
#endif
#if defined(CONFIG_AUTOMOUNT_USERFS) && defined(CONFIG_EXAMPLES_TESTCASE_FILESYSTEM)
#define TESTIOC_GET_FS_PARTNO			_TESTIOC(24)
#endif

#ifdef CONFIG_SMP
#define TESTIOC_SCHED_CPUSTATE			_TESTIOC(25)
#endif

#define OS_API_TEST_DRVPATH	"/dev/os_api_test"

#ifdef CONFIG_SMP
/* Snapshot of the SMP scheduler's placement state.
 *
 * TESTIOC_SCHED_CPUSTATE fills one of these from inside a critical section so
 * that every field describes the same instant.  It answers a single question:
 * when an unassigned task is waiting in g_readytorun, why was it not given to
 * an idle CPU?
 *
 * Read only.  Nothing here changes scheduler state.
 */

#define TEST_CPUSTATE_MAXRTR	8
#define TEST_CPUSTATE_NAMELEN	16

/* One entry per CPU, describing the head of g_assignedtasks[cpu] -- that is,
 * the task the CPU is actually executing.
 */

struct test_cpu_slot_s {
	pid_t pid;
	uint8_t priority;
	uint8_t state;				/* tstate_t of the running task */
	uint8_t nassigned;			/* Length of g_assignedtasks[cpu] */
	uint8_t idle_only;			/* head->flink == NULL, i.e. only IDLE here */
	char name[TEST_CPUSTATE_NAMELEN];
};

/* One entry per waiting task in g_readytorun, in priority order. */

struct test_rtr_slot_s {
	pid_t pid;
	uint8_t priority;
	uint32_t affinity;
	char name[TEST_CPUSTATE_NAMELEN];
};

struct test_cpustate_s {
	uint8_t ncpus;				/* CONFIG_SMP_NCPUS */
	uint8_t caller_cpu;			/* CPU the caller is running on */
	uint32_t active_mask;			/* g_active_cpus_mask */
	uint8_t sched_locked;			/* sched_islocked_global() */
	int8_t select_all;			/* sched_select_cpu(all CPUs) */
	uint8_t nrtr;				/* Length of g_readytorun */
	uint8_t nrtr_reported;			/* Entries filled in rtr[] */
	struct test_cpu_slot_s cpu[CONFIG_SMP_NCPUS];
	struct test_rtr_slot_s rtr[TEST_CPUSTATE_MAXRTR];
};
#endif							/* CONFIG_SMP */

/****************************************************************************
 * Public Data
 ****************************************************************************/

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
 * Name: os_api_test_drv_register
 *
 * Description:
 *   This function creates a device node like "/dev/os_api_test" which will be used
 *   by the tests that execute OS(kernel, network and fs) side APIs
 *
 *
 ****************************************************************************/

void os_api_test_drv_register(void);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* CONFIG_DRIVERS_OS_API_TEST */
#endif							/* __INCLUDE_TINYARA_OS_API_TEST_DRV_H */
