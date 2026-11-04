
#ifndef __INCLUDE_TINYARA_MM_KASAN_H
#define __INCLUDE_TINYARA_MM_KASAN_H

#include <tinyara/compiler.h>
#include <stddef.h>

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Cases the self test can run.
 *
 * Every case but the first is expected to be reported and, unless the
 * corresponding panic has been disabled, to assert. That is the pass
 * condition, not a failure of the test.
 */

enum kasan_selftest_e {
	KASAN_SELFTEST_INBOUNDS = 0,	/* Access inside the block. No report. */
	KASAN_SELFTEST_OVERFLOW_WRITE,	/* Write one byte past the block */
	KASAN_SELFTEST_OVERFLOW_READ,	/* Read one byte past the block */
	KASAN_SELFTEST_UNDERFLOW,	/* Write one byte before the block */
	KASAN_SELFTEST_USE_AFTER_FREE,	/* Write to a block already freed */
	KASAN_SELFTEST_STRADDLE,	/* Word write across the block end */
	KASAN_SELFTEST_MAX
};

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: kasan_selftest
 *
 * Description:
 *   Perform one deliberate memory error against the kernel heap so that the
 *   instrumentation can be shown to work. Without this there is no way to
 *   tell a system with no memory errors from a system where the
 *   instrumentation was never emitted; both are silent.
 *
 *   Lives outside mm/ because everything under mm/ is built with
 *   -fno-sanitize=kernel-address and so cannot raise a report.
 *
 *   Called directly from the test application: this is a flat build, so the
 *   application and the kernel are one image and one link.
 *
 * Input Parameters:
 *   testcase - one of enum kasan_selftest_e
 *
 * Returned Value:
 *   Zero when KASAN_SELFTEST_INBOUNDS completed with nothing reported, or a
 *   negated errno. The remaining cases are not expected to return at all.
 *
 ****************************************************************************/

#ifdef CONFIG_MM_KASAN_SELFTEST
int kasan_selftest(int testcase);
#endif

/****************************************************************************
 * Name: kasan_unpoison
 ****************************************************************************/

#if defined(CONFIG_MM_KASAN) || defined(CONFIG_MM_KASAN_INSTRUMENT_ALL)
FAR void *kasan_unpoison(FAR const void *addr, size_t size);
void kasan_init_early(void);

/****************************************************************************
 * Name: kasan_poison / kasan_register / kasan_unregister
 *
 * Description:
 *   Allocator hooks. kasan_register() carves the shadow out of the tail of
 *   a heap region and hands back the reduced size; kasan_unregister()
 *   releases the region. kasan_poison() marks a range as inaccessible, and
 *   is used by the allocator to close chunks off on free.
 *
 ****************************************************************************/

void kasan_poison(FAR const void *addr, size_t size);
FAR void *kasan_register(FAR void *addr, FAR size_t *size);
void kasan_unregister(FAR void *addr);

/****************************************************************************
 * Name: kasan_attach_heap / kasan_import
 *
 * Description:
 *   Import into this image's region table the region that the kernel
 *   memory manager registered for this heap and recorded in the heap
 *   structure itself. The user image compiles the same allocator but runs
 *   its own KASan runtime with its own table; without the import its hooks
 *   would leave the shared shadow stale and its own instrumented accesses
 *   would go unchecked. On the kernel side the region is already in the
 *   table and the call does nothing.
 *
 ****************************************************************************/

struct mm_heap_s;
void kasan_attach_heap(FAR struct mm_heap_s *heap);
void kasan_import(FAR void *desc);

/****************************************************************************
 * Name: kasan_start
 *
 * Description:
 *   Enable KASAN memory error checking. This function sets the internal
 *   gate variable that allows instrumented code to perform shadow memory
 *   checks on memory accesses.
 *
 *   Called automatically during boot after heap registration unless
 *   CONFIG_MM_KASAN_DISABLE_AT_BOOT is set. Can also be called later
 *   through a kasan command to enable checking at runtime.
 *
 ****************************************************************************/

void kasan_start(void);

/****************************************************************************
 * Name: kasan_stop
 *
 * Description:
 *   Disable KASAN memory error checking. This function clears the internal
 *   gate variable, causing all instrumented memory accesses to skip shadow
 *   memory checks and return immediately without reporting.
 *
 *   Useful for:
 *   - Disabling KASAN during boot to avoid false positives
 *   - Temporarily suspending checking during performance-critical sections
 *   - Re-enabling later through a kasan command when testing is needed
 *
 ****************************************************************************/

void kasan_stop(void);
bool kasan_is_armed(void);
#else
#define kasan_init_early()
#define kasan_start()
#define kasan_stop()
#define kasan_is_armed()		false
#define kasan_unpoison(addr, size)	((FAR void *)(addr))
#define kasan_poison(addr, size)
#define kasan_register(addr, size)	(NULL)
#define kasan_unregister(addr)
#define kasan_attach_heap(heap)
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_TINYARA_MM_KASAN_H */
