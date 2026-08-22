/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
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
#include <errno.h>
#include <debug.h>

#include <tinyara/os_api_test_drv.h>
#include <tinyara/sched.h>
#ifdef CONFIG_SMP
#include <string.h>
#include <tinyara/irq.h>
#include "sched/sched.h"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct tcb_s *tcb;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int test_get_self_pid(unsigned long arg)
{
	tcb = sched_self();
	if (tcb == NULL) {
		return ERROR;
	}
	return tcb->pid;
}

static int test_is_alive_thread(unsigned long arg)
{
	tcb = sched_gettcb((pid_t)arg);
	if (tcb == NULL) {
		dbg("sched_gettcb failed. errno : %d\n", get_errno());
		return ERROR;
	}
	return OK;
}

static int test_get_tcb_adj_stack_size(unsigned long arg)
{
	tcb = sched_gettcb((pid_t)arg);
	if (tcb == NULL) {
		dbg("sched_gettcb failed. errno : %d\n", get_errno());
		return ERROR;
	}
	return tcb->adj_stack_size;
}

static void test_sched_foreach(unsigned long arg)
{
	sched_foreach((void *)arg, NULL);
}

#ifdef CONFIG_SMP
/****************************************************************************
 * Name: test_sched_copyname
 ****************************************************************************/

static void test_sched_copyname(char *dest, FAR struct tcb_s *tcb)
{
#if CONFIG_TASK_NAME_SIZE > 0
	strncpy(dest, tcb->name, TEST_CPUSTATE_NAMELEN - 1);
	dest[TEST_CPUSTATE_NAMELEN - 1] = '\0';
#else
	dest[0] = '\0';
#endif
}

/****************************************************************************
 * Name: test_sched_cpustate
 *
 * Description:
 *   Take a consistent snapshot of the SMP placement state: what each CPU is
 *   running, what is waiting unassigned in g_readytorun, and which CPU
 *   sched_select_cpu() would hand an unpinned task to right now.
 *
 *   The whole snapshot is taken inside one critical section so that the CPU
 *   slots, the ready-to-run list and the select result all describe the same
 *   instant.  Taking them separately would let the lists move underneath the
 *   reader and produce a picture that never actually existed.
 *
 *   This reads scheduler state and nothing more; no list is modified and no
 *   task is moved.
 *
 ****************************************************************************/

static int test_sched_cpustate(unsigned long arg)
{
	FAR struct test_cpustate_s *st = (FAR struct test_cpustate_s *)arg;
	FAR struct tcb_s *tcb;
	irqstate_t flags;
	int n;
	int i;

	if (st == NULL) {
		return -EINVAL;
	}

	memset(st, 0, sizeof(struct test_cpustate_s));

	flags = enter_critical_section();

	st->ncpus = CONFIG_SMP_NCPUS;
	st->caller_cpu = (uint8_t)this_cpu();
	st->active_mask = (uint32_t)g_active_cpus_mask;
	st->sched_locked = sched_islocked_global() ? 1 : 0;

	/* What sched_select_cpu() would return for a task allowed on every CPU.
	 * This is the decision that places an unpinned task, so it is the value
	 * that explains where such a task ends up.
	 */

	st->select_all = (int8_t)sched_select_cpu((cpu_set_t)((1 << CONFIG_SMP_NCPUS) - 1));

	/* Per CPU: the running task and the depth of its assigned list.  The
	 * idle_only flag is the exact test sched_select_cpu() applies, recorded
	 * here so that its decision can be checked against its input.
	 */

	for (i = 0; i < CONFIG_SMP_NCPUS; i++) {
		tcb = (FAR struct tcb_s *)g_assignedtasks[i].head;
		if (tcb == NULL) {
			continue;
		}

		st->cpu[i].pid = tcb->pid;
		st->cpu[i].priority = tcb->sched_priority;
		st->cpu[i].state = tcb->task_state;
		st->cpu[i].idle_only = (tcb->flink == NULL) ? 1 : 0;
		test_sched_copyname(st->cpu[i].name, tcb);

		for (n = 0; tcb != NULL && n < 255; tcb = (FAR struct tcb_s *)tcb->flink) {
			n++;
		}

		st->cpu[i].nassigned = (uint8_t)n;
	}

	/* The unassigned runnable tasks, in the order the scheduler sees them. */

	n = 0;
	for (tcb = (FAR struct tcb_s *)g_readytorun.head; tcb != NULL;
	     tcb = (FAR struct tcb_s *)tcb->flink) {
		if (n < TEST_CPUSTATE_MAXRTR) {
			st->rtr[n].pid = tcb->pid;
			st->rtr[n].priority = tcb->sched_priority;
			st->rtr[n].affinity = (uint32_t)tcb->affinity;
			test_sched_copyname(st->rtr[n].name, tcb);
		}

		if (++n >= 255) {
			break;
		}
	}

	st->nrtr = (uint8_t)n;
	st->nrtr_reported = (uint8_t)(n < TEST_CPUSTATE_MAXRTR ? n : TEST_CPUSTATE_MAXRTR);

	leave_critical_section(flags);

	return OK;
}
#endif							/* CONFIG_SMP */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int test_sched(int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	switch (cmd) {
	case TESTIOC_GET_SELF_PID:
		ret = test_get_self_pid(arg);
		break;
	case TESTIOC_IS_ALIVE_THREAD:
		ret = test_is_alive_thread(arg);
		break;
	case TESTIOC_GET_TCB_ADJ_STACK_SIZE:
		ret = test_get_tcb_adj_stack_size(arg);
		break;
	case TESTIOC_SCHED_FOREACH:
		test_sched_foreach(arg);
		break;
#ifdef CONFIG_SMP
	case TESTIOC_SCHED_CPUSTATE:
		ret = test_sched_cpustate(arg);
		break;
#endif
	}
	return ret;
}
