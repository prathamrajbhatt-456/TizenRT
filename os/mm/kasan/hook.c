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
 * os/mm/kasan/hook.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * Derived from Apache NuttX mm/kasan/hook.c at commit b4aa94d.
 *
 * Differences from the original, all deliberate:
 *
 *  1. Reports go to lowsyslog() rather than to _alert(). TizenRT's lldbg()
 *     expands to nothing unless CONFIG_DEBUG, CONFIG_DEBUG_ERROR and
 *     CONFIG_ARCH_LOWPUTC are all set, which would leave KASan panicking
 *     with no output at all in a plain build.
 *  2. The shadow dump formats its own hex rather than calling sprintf() on
 *     a 256 byte stack buffer, and prints ".." for any byte outside a
 *     registered region rather than dereferencing it. Upstream reads 80
 *     bytes either side of the faulting address unconditionally, which can
 *     fault a second time while reporting on a target without an MMU.
 *  3. The ANSI colour escapes are dropped; TizenRT consoles are raw UART.
 *  4. predict_false() is not a TizenRT macro, so a local one is used.
 *  5. The watchpoint support, kasan_debugpoint(), the dynamic init hooks
 *     and __sanitizer_annotate_contiguous_container() are omitted. Nothing
 *     in TizenRT drives them, and the compiler does not emit references to
 *     them with the flag set this port uses.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <stdint.h>
#include <syslog.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/mm/kasan.h>
#include <tinyara/mm/mm.h>
#include <tinyara/binfmt/elf.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Reporting primitive.
 *
 * lowsyslog() is used directly rather than lldbg(), because lldbg() is
 * compiled away unless the debug options happen to be enabled, and a report
 * that prints nothing before panicking is worse than useless. lowsyslog()
 * is safe from interrupt context and falls back to syslog() on a target
 * without CONFIG_ARCH_LOWPUTC.
 *
 * generic.c is included below and uses this macro, so it must be defined
 * first.
 */

#define kasan_alert(format, ...)	\
	lowsyslog(LOG_ERR, "KASAN: " format, ##__VA_ARGS__)

/* Reports raised from instrumented code must reach the console in every
 * image, but the right channel differs between them and the discriminator
 * is the same one lib_lowoutstream.c uses:
 *
 *  - Kernel image (__KERNEL__): lowsyslog() wraps up_putc() — direct UART,
 *    no buffering, no semaphores, safe from interrupt handlers. dbg() would
 *    go through syslog() -> the buffered stdout FILE, and taking its
 *    semaphore from an interrupt handler asserts in sem_wait().
 *  - User image (protected build, no __KERNEL__): lowoutstream_putc()
 *    compiles to nothing, so lowsyslog() prints nothing at all — user-image
 *    reports were invisible on this channel. dbg() -> syslog() writes
 *    through the line-buffered stdout device, which flushes on the '\n'
 *    that terminates every report line. User-image code only ever runs in
 *    task context, so the semaphore use is safe there.
 *
 * Allocator-side warnings keep kasan_alert() (lowsyslog): they can be
 * raised from interrupt context in the kernel image via the deferred free
 * list, and they never need the user-image channel.
 */

#if defined(CONFIG_BUILD_PROTECTED) && !defined(__KERNEL__)
#define kasan_report_alert(format, ...)	\
	dbg("KASAN: " format, ##__VA_ARGS__)
#else
#define kasan_report_alert(format, ...)	\
	lowsyslog(LOG_ERR, "KASAN: " format, ##__VA_ARGS__)
#endif



/* TizenRT has no predict_false(). Note that inline_function in TizenRT is a
 * bare attribute rather than a full storage class as it is upstream, so it
 * cannot be used as a prefix here.
 */

#define kasan_predict_false(x)		__builtin_expect(!!(x), 0)

#ifdef CONFIG_MM_KASAN_GENERIC
#include "generic.c"
#else
/* MM_KASAN_NONE: the entry points are still emitted and called, so that the
 * cost of instrumentation can be measured on its own, but nothing is ever
 * reported and no shadow exists to dump.
 */

#define kasan_is_poisoned(addr, size)		false

/* The bit argument is consumed so that a caller which declares a variable
 * solely to pass its address here does not trip -Wunused-variable.
 */

#define kasan_mem_to_shadow(addr, size, bit)	((void)(bit), NULL)
#endif

/****************************************************************************
 * Name: kasan_attach_heap
 *
 * Description:
 *   Import into this image's region table the region that the kernel
 *   memory manager registered for this heap and recorded in the heap
 *   structure. See kasan.h. With no shadow mode there is no table to
 *   import into and the call does nothing.
 *
 ****************************************************************************/

void kasan_attach_heap(FAR struct mm_heap_s *heap)
{
#ifdef CONFIG_MM_KASAN_GENERIC
	if (heap != NULL) {
		kasan_import(heap->mm_kasan_region);
	}
#else
	(void)heap;
#endif
}

#define KASAN_INIT_VALUE		0xcafe

/* Number of bytes shown either side of the faulting address, and the number
 * of bytes per dump line.
 */

#define KASAN_DUMP_BYTES		64
#define KASAN_DUMP_PER_LINE		16

/* Enough for the address, the separator and KASAN_DUMP_PER_LINE two digit
 * bytes with a space and a bracket allowance each.
 */

#define KASAN_DUMP_LINE_MAX		(KASAN_DUMP_PER_LINE * 4 + 8)

#ifdef CONFIG_MM_KASAN_DISABLE_READ_PANIC
#define MM_KASAN_DISABLE_READ_PANIC	1
#else
#define MM_KASAN_DISABLE_READ_PANIC	0
#endif

#ifdef CONFIG_MM_KASAN_DISABLE_WRITE_PANIC
#define MM_KASAN_DISABLE_WRITE_PANIC	1
#else
#define MM_KASAN_DISABLE_WRITE_PANIC	0
#endif

#ifdef CONFIG_MM_KASAN_INSTRUMENT

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The compiler emits a call to one of these for every instrumented access.
 * The _noabort forms are used when the sanitizer is built in recovering
 * mode, which is the default for kernel-address; the plain forms are used
 * with -fno-sanitize-recover=kernel-address. Both are defined so that the
 * build does not depend on which of the two the toolchain selects.
 */

#define DEFINE_ASAN_LOAD_STORE(size)					\
	void __asan_report_load##size##_noabort(FAR void *addr)		\
	{								\
		kasan_report(addr, size, false, __builtin_return_address(0)); \
	}								\
	void __asan_report_store##size##_noabort(FAR void *addr)	\
	{								\
		kasan_report(addr, size, true, __builtin_return_address(0)); \
	}								\
	void __asan_load##size##_noabort(FAR void *addr)		\
	{								\
		kasan_check_report(addr, size, false, __builtin_return_address(0)); \
	}								\
	void __asan_store##size##_noabort(FAR void *addr)		\
	{								\
		kasan_check_report(addr, size, true, __builtin_return_address(0)); \
	}								\
	void __asan_load##size(FAR void *addr)				\
	{								\
		kasan_check_report(addr, size, false, __builtin_return_address(0)); \
	}								\
	void __asan_store##size(FAR void *addr)				\
	{								\
		kasan_check_report(addr, size, true, __builtin_return_address(0)); \
	}

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Checking is off until this holds KASAN_INIT_VALUE. Instrumented code runs
 * from the first instruction of the image, long before any heap exists, so
 * this gate is what makes an image wide -fsanitize=kernel-address safe
 * without excluding the whole boot path from instrumentation.
 */

static uint32_t g_region_init;

static const char g_kasan_hex[] = "0123456789abcdef";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: kasan_show_memory
 *
 * Description:
 *   Dump the bytes around the faulting address, marking the poisoned ones.
 *
 *   Bytes which fall outside every registered region are shown as "..".
 *   They are never dereferenced: on a target without an MMU a read of an
 *   unmapped address would fault again from inside the report.
 *
 ****************************************************************************/

static void kasan_show_memory(FAR const uint8_t *addr, size_t size)
{
	FAR const uint8_t *start;
	FAR const uint8_t *p;
	char buffer[KASAN_DUMP_LINE_MAX];
	unsigned int bit;
	int i;
	int n;

	/* Round down to a line boundary and step back, without letting the
	 * pointer wrap when the faulting address is near zero.
	 */

	start = (FAR const uint8_t *)(((uintptr_t)addr) & ~(uintptr_t)(KASAN_DUMP_PER_LINE - 1));
	if ((uintptr_t)start >= KASAN_DUMP_BYTES) {
		start -= KASAN_DUMP_BYTES;
	} else {
		start = NULL;
	}

	kasan_report_alert("memory around the faulty address:\n");

	for (p = start; p < start + 2 * KASAN_DUMP_BYTES; p += KASAN_DUMP_PER_LINE) {
		n = 0;

		for (i = 0; i < KASAN_DUMP_PER_LINE; i++) {
			FAR const uint8_t *q = p + i;

			/* Mark the faulting range with brackets so it can be picked
			 * out of the line without counting columns.
			 */

			buffer[n++] = (q == addr) ? '[' : ((q == addr + size) ? ']' : ' ');

			if (kasan_mem_to_shadow(q, 1, &bit) == NULL) {
				/* Not covered by any region: do not touch it. */

				buffer[n++] = '.';
				buffer[n++] = '.';
			} else {
				buffer[n++] = g_kasan_hex[(*q >> 4) & 0xf];
				buffer[n++] = g_kasan_hex[*q & 0xf];
			}
		}

		buffer[n] = '\0';
		kasan_report_alert("%p:%s\n", p, buffer);
	}
}

/****************************************************************************
 * Name: kasan_report_image
 *
 * Description:
 *   Tell which image the reported PC belongs to and name the addr2line ELF
 *   that resolves it, so the address printed by the "detected from" line
 *   can be symbolized without hunting the runtime load base in the boot
 *   log:
 *
 *   - Kernel image: linked at its runtime address, so the raw PC resolves
 *     directly with tinyara.axf. For user-image PCs the binfmt section
 *     table is consulted and the printed offset from the image text start
 *     is load-position independent; resolve it as addr2line -e
 *     build/output/bin/<bin_name>_dbg (link_text_vaddr + offset), where
 *     the link text vaddr is a build constant.
 *   - User image: the whole image is one loadable binary, so the offset
 *     from its own (link-time constant) text start is printed directly;
 *     resolve it as addr2line -e build/output/bin/<CONFIG_COMMON_BINARY_NAME>_dbg
 *     -f -C <offset> - the dbg ELF is a relocatable object with .text at
 *     vaddr 0, so the offset can be fed to addr2line as is.
 *
 ****************************************************************************/

/* The kernel variant is used both when __KERNEL__ is defined (protected /
 * loadable kernel pass) and for flat builds: in a flat build os/mm/Makefile
 * compiles this file without -D__KERNEL__ (only the protected kbin pass
 * gets $(KDEFINE)), but there is no separate user image at all - every PC
 * including TASH lives in tinyara.axf. */

#if defined(__KERNEL__) || !defined(CONFIG_BUILD_PROTECTED)
extern uint32_t _stext;			/* linker-script symbols: kernel text bounds */
extern uint32_t _etext;

static void kasan_report_image(FAR void *return_address)
{
	uintptr_t va = (uintptr_t)return_address & ~(uintptr_t)1;	/* mask Thumb bit */

#ifdef CONFIG_APP_BINARY_SEPARATION
	uint32_t text_base = 0;
	uint32_t text_size = 0;
	FAR const char *bin_name = NULL;
	int idx = elf_find_image_by_addr(va, &text_base, &text_size, &bin_name);

	if (idx >= 0) {
		/* The *_dbg ELF is a relocatable object with .text linked at
		 * vaddr 0, so the offset from the runtime text start is also
		 * its virtual address inside that ELF: addr2line resolves it
		 * as given, no load-base math needed. */

		kasan_report_alert("pc belongs to image %d \"%s\" (text 0x%08x size %u), offset from text start +0x%x (addr2line -e build/output/bin/%s_dbg -f -C 0x%x)\n", idx, bin_name ? bin_name : "?", text_base, text_size, (uint32_t)(va - text_base), bin_name ? bin_name : "?", (uint32_t)(va - text_base));
		return;
	}
#endif

	if ((uintptr_t)&_stext <= va && va < (uintptr_t)&_etext) {
		/* tinyara.axf is linked at its runtime address: the raw PC resolves directly. */

		kasan_report_alert("pc belongs to kernel image (addr2line -e build/output/bin/tinyara.axf -f -C 0x%x)\n", (uint32_t)va);
		return;
	}

	kasan_report_alert("pc not in any loaded image\n");
}
#else
/* The userspace kasan hooks live in the common binary, whose dbg ELF is
 * named by CONFIG_COMMON_BINARY_NAME in the shared .config - the same name
 * os/dutils.sh uses for build/output/bin/<name>_dbg. The dbg ELF is a
 * relocatable object with .text at vaddr 0, so the offset printed here can
 * be fed to addr2line as is. */

#ifdef CONFIG_SUPPORT_COMMON_BINARY
#define KASAN_IMAGE_ELF "build/output/bin/" CONFIG_COMMON_BINARY_NAME "_dbg"
#else
#define KASAN_IMAGE_ELF "this image's <bin_name>_dbg"
#endif

extern uint32_t _stext_flash;		/* userspace linker script: this image's text bounds */
extern uint32_t _etext_flash;

static void kasan_report_image(FAR void *return_address)
{
	uintptr_t va = (uintptr_t)return_address & ~(uintptr_t)1;	/* mask Thumb bit */

	if ((uintptr_t)&_stext_flash <= va && va < (uintptr_t)&_etext_flash) {
		kasan_report_alert("pc belongs to this user image, offset from text start +0x%x (addr2line -e %s -f -C 0x%x)\n", (uint32_t)(va - (uintptr_t)&_stext_flash), KASAN_IMAGE_ELF, (uint32_t)(va - (uintptr_t)&_stext_flash));
		return;
	}

	/* Outside this image the PC is normally the kernel image. Kernel
	 * symbols are not visible to the user image, so its range cannot be
	 * checked here, but tinyara.axf is linked at its runtime address, so
	 * the raw PC resolves directly. */

	kasan_report_alert("pc not in this user image, may be kernel image (addr2line -e build/output/bin/tinyara.axf -f -C 0x%x)\n", (uint32_t)va);
}
#endif

/****************************************************************************
 * Name: kasan_report
 *
 * Description:
 *   Print the report and, unless the corresponding panic has been disabled,
 *   assert so that the crash dump is taken at the faulting instruction.
 *
 ****************************************************************************/

static void kasan_report(FAR const void *addr, size_t size, bool is_write, FAR void *return_address)
{
	bool dump_only = (is_write && MM_KASAN_DISABLE_WRITE_PANIC) || (!is_write && MM_KASAN_DISABLE_READ_PANIC);

	/* Stop checking before printing. The report path itself walks memory,
	 * and a second report from inside the first would recurse.
	 */

	kasan_stop();

	kasan_report_alert("invalid %s of size %zu at address %p\n", is_write ? "write" : "read", size, addr);
	kasan_report_alert("detected from %p\n", return_address);
	kasan_report_image(return_address);

	kasan_show_memory(addr, size);

	if (dump_only) {
#ifdef CONFIG_FRAME_POINTER
		/* dump_stack() is only declared when frame pointers are kept.
		 * Without them there is no backtrace to give, and the caller
		 * address printed above is all the context available.
		 */

		dump_stack();
#endif

		/* Resume checking: the caller has chosen to carry on running. */

		kasan_start();
	} else {
		/* The panic this branch used to take is disabled in the ported
		 * configuration. Say so on the report channel rather than with a
		 * bare lldbg(): a user image detection printed through the wrong
		 * channel and never reached the console.
		 */

		kasan_report_alert("panic disabled, continuing execution\n");

		/* Resume checking since we're continuing execution.
		 * Without this, reporting stays off after the first detection.
		 */
		kasan_start();
	}
}

/****************************************************************************
 * Name: kasan_check_report
 *
 * Description:
 *   The hot path. Called from every instrumented load and store, so the
 *   ordering of the tests matters: the gate is checked first and is false
 *   for the whole of early boot.
 *
 ****************************************************************************/

static inline void kasan_check_report(FAR const void *addr, size_t size, bool is_write, FAR void *return_address)
{
	if (kasan_predict_false(size == 0 || g_region_init != KASAN_INIT_VALUE)) {
		return;
	}
#ifndef CONFIG_MM_KASAN_DISABLE_NULL_POINTER_CHECK
	if (kasan_predict_false(addr == NULL)) {
		kasan_report(addr, size, is_write, return_address);
		return;
	}
#endif

	if (kasan_predict_false(kasan_is_poisoned(addr, size))) {
		kasan_report(addr, size, is_write, return_address);
	}
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: kasan_init_early
 ****************************************************************************/

#if defined(CONFIG_MM_KASAN) || defined(CONFIG_MM_KASAN_INSTRUMENT_ALL)
void kasan_init_early(void)
{
	/* Ensure KASAN checking is held off until a heap region is registered.
	 * The gate variable is in .bss and already zero, but this call makes
	 * the initialization explicit and independent of .bss clearing order.
	 */

	g_region_init = 0;
}

/****************************************************************************
 * Name: kasan_start
 ****************************************************************************/

void kasan_start(void)
{
	g_region_init = KASAN_INIT_VALUE;
}

/****************************************************************************
 * Name: kasan_stop
 ****************************************************************************/

void kasan_stop(void)
{
	g_region_init = 0;
}

/****************************************************************************
 * Name: kasan_is_armed
 *
 * Description:
 *   Whether a check would be reported right now. Reads the same gate the
 *   instrumentation entry points test, so it answers for reporting only: the
 *   shadow map is maintained whether this is true or false.
 *
 ****************************************************************************/

bool kasan_is_armed(void)
{
	return g_region_init == KASAN_INIT_VALUE;
}

#endif

/****************************************************************************
 * Name: __asan_handle_no_return
 *
 * Description:
 *   Emitted before a call to a noreturn function. With stack instrumentation
 *   disabled there is no fake stack to unpoison, so there is nothing to do.
 *
 ****************************************************************************/

void __asan_handle_no_return(void)
{
}

/****************************************************************************
 * Name: __asan_register_globals / __asan_unregister_globals
 *
 * Description:
 *   Only referenced when the image is built with asan-globals=1, which this
 *   port does not do. Defined so that turning the parameter on for an
 *   experiment links rather than failing, with globals simply unchecked.
 *
 ****************************************************************************/

void __asan_register_globals(FAR void *ptr, size_t size)
{
	(void)ptr;
	(void)size;
}

void __asan_unregister_globals(FAR void *ptr, size_t size)
{
	(void)ptr;
	(void)size;
}

/****************************************************************************
 * Name: __asan_loadN / __asan_storeN and the fixed size variants
 ****************************************************************************/

void __asan_report_load_n_noabort(FAR void *addr, size_t size)
{
	kasan_report(addr, size, false, __builtin_return_address(0));
}

void __asan_report_store_n_noabort(FAR void *addr, size_t size)
{
	kasan_report(addr, size, true, __builtin_return_address(0));
}

void __asan_loadN_noabort(FAR void *addr, size_t size)
{
	kasan_check_report(addr, size, false, __builtin_return_address(0));
}

void __asan_storeN_noabort(FAR void *addr, size_t size)
{
	kasan_check_report(addr, size, true, __builtin_return_address(0));
}

void __asan_loadN(FAR void *addr, size_t size)
{
	kasan_check_report(addr, size, false, __builtin_return_address(0));
}

void __asan_storeN(FAR void *addr, size_t size)
{
	kasan_check_report(addr, size, true, __builtin_return_address(0));
}

DEFINE_ASAN_LOAD_STORE(1)
DEFINE_ASAN_LOAD_STORE(2)
DEFINE_ASAN_LOAD_STORE(4)
DEFINE_ASAN_LOAD_STORE(8)
DEFINE_ASAN_LOAD_STORE(16)

#endif							/* CONFIG_MM_KASAN_INSTRUMENT */
