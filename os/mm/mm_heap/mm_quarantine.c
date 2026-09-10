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

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <debug.h>

#include <tinyara/mm/mm.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define QUARANTINE_SLOTS CONFIG_DEBUG_MM_QUARANTINE_CHUNKS

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: quarantine_poison_chunk
 *
 * Description:
 *   Poison a quarantine chunk with preceding+size as repeated pattern
 *   (if CONFIG_DEBUG_MM_UAF_METADATA_POISON is enabled) or standard poison.
 *   This unified function handles both cases internally.
 *
 ****************************************************************************/

static void quarantine_poison_chunk(FAR struct mm_allocnode_s *node)
{
	FAR void *start;
	size_t nbytes;
	size_t offset;
	size_t minimal_size;
	size_t i;

	/* Get data area at correct offset (SIZEOF_MM_FREENODE) */
	offset = MM_UAF_ALIGN_UP(SIZEOF_MM_FREENODE);
	start = (FAR void *)((FAR char *)node + offset);
	nbytes = (node->size > offset) ? node->size - offset : 0;

	if (nbytes == 0) {
		return;
	}

#ifdef CONFIG_DEBUG_MM_UAF_METADATA_POISON
	minimal_size = sizeof(node->preceding) + sizeof(node->size);

	if (nbytes >= minimal_size) {
		/* Repeat metadata pattern (preceding + size) throughout data area */
		for (i = 0; i + minimal_size <= nbytes; i += minimal_size) {
			memcpy((FAR uint8_t *)start + i, &node->preceding, sizeof(node->preceding));
			memcpy((FAR uint8_t *)start + i + sizeof(node->preceding),
				   &node->size, sizeof(node->size));
		}

		/* Fill remaining space with standard poison */
		if (i < nbytes) {
			mm_uaf_poison_range((FAR uint8_t *)start + i, nbytes - i);
		}
		return;
	}
#endif

	/* Standard poison (also fallback for small chunks) */
	mm_uaf_poison_range(start, nbytes);
}

/****************************************************************************
 * Name: quarantine_verify_chunk
 *
 * Description:
 *   Verify a quarantine chunk - checks preceding+size pattern
 *   (if CONFIG_DEBUG_MM_UAF_METADATA_POISON is enabled) or standard poison.
 *   This unified function handles both cases internally.
 *
 *   CRITICAL: Must be called BEFORE clearing MM_ALLOC_BIT.
 *
 ****************************************************************************/

static void quarantine_verify_chunk(FAR struct mm_allocnode_s *node)
{
	FAR void *start;
	size_t nbytes;
	size_t offset;
	size_t minimal_size;
	FAR uint8_t *copy;
	bool corrupted = false;

	/* Get data area at correct offset */
	offset = MM_UAF_ALIGN_UP(SIZEOF_MM_FREENODE);
	start = (FAR void *)((FAR char *)node + offset);
	nbytes = (node->size > offset) ? node->size - offset : 0;

	if (nbytes == 0) {
		return;
	}

#ifdef CONFIG_DEBUG_MM_UAF_METADATA_POISON
	minimal_size = sizeof(node->preceding) + sizeof(node->size);

	if (nbytes >= minimal_size) {
		copy = (FAR uint8_t *)start;

		/* Compare preceding field */
		if (memcmp(&node->preceding, copy, sizeof(node->preceding)) != 0) {
			corrupted = true;
		}

		/* Compare size field */
		if (memcmp(&node->size, copy + sizeof(node->preceding), sizeof(node->size)) != 0) {
			corrupted = true;
		}

		if (corrupted) {
			heap_dbg("ERROR: Quarantine metadata corruption detected\n");
			mm_dump_node(node, "QUARANTINE CORRUPTED");
			mfdbg("Expected: preceding=0x%08x, size=0x%08x\n", node->preceding, node->size);
			mfdbg("Found:    preceding=0x%08x, size=0x%08x\n",
				  *(FAR uint32_t *)copy, *(FAR uint32_t *)(copy + sizeof(node->preceding)));
#ifdef CONFIG_DEBUG_MM_UAF_PANIC
			PANIC();
#endif
		}
		return;
	}
#endif

	/* Standard verify (also fallback for small chunks) */
	mm_uaf_verify_range(node, start, nbytes);
}

/****************************************************************************
 * Name: quarantine_release_oldest
 *
 * Description:
 *   Take the oldest chunk out of the quarantine and give it back to the
 *   allocator. The poison pattern is checked on the way out: this is the
 *   point at which a write through a pointer freed long ago is found,
 *   because until now nothing else could have written to the chunk.
 *
 *   NOTES:
 *     (1) the quarantine must not be empty.
 *     (2) the caller must hold the MM semaphore.
 *
 ****************************************************************************/

static void quarantine_release_oldest(FAR struct mm_heap_s *heap)
{
	FAR struct mm_allocnode_s *node;

	DEBUGASSERT(heap->mm_qcount > 0);

	node = heap->mm_quarantine[heap->mm_qhead];
	heap->mm_quarantine[heap->mm_qhead] = NULL;
	heap->mm_qhead = (heap->mm_qhead + 1) % QUARANTINE_SLOTS;
	heap->mm_qcount--;
	heap->mm_qbytes -= node->size;

	/* STEP 1: Verify FIRST while MM_ALLOC_BIT is still set */
	quarantine_verify_chunk(node);

	/* STEP 2: THEN clear MM_ALLOC_BIT */
	node->preceding &= ~MM_ALLOC_BIT;

	/* STEP 3: Now chunk is in free format, proceed with coalesce */
	mm_free_coalesce(heap, (FAR struct mm_freenode_s *)node);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mm_quarantine_init
 *
 * Description:
 *   Reset the quarantine of a heap. Called from mm_initialize().
 *
 ****************************************************************************/

void mm_quarantine_init(FAR struct mm_heap_s *heap)
{
	int i;

	for (i = 0; i < QUARANTINE_SLOTS; i++) {
		heap->mm_quarantine[i] = NULL;
	}

	heap->mm_qhead = 0;
	heap->mm_qcount = 0;
	heap->mm_qbytes = 0;
}

/****************************************************************************
 * Name: mm_quarantine_contains
 *
 * Description:
 *   Test whether a chunk is currently held in the quarantine.
 *
 *   A held chunk keeps MM_ALLOC_BIT set, so it is indistinguishable from a
 *   live allocation by its header alone and the usual double free test in
 *   mm_free() cannot see it. This scan is what makes that case detectable,
 *   and it is also what stops a second free from entering the same chunk
 *   into the ring twice. The ring is small and bounded, so the scan is a
 *   short walk over an array of pointers.
 *
 *   NOTES:
 *     (1) the caller must hold the MM semaphore.
 *
 ****************************************************************************/

bool mm_quarantine_contains(FAR struct mm_heap_s *heap, FAR struct mm_allocnode_s *node)
{
	uint16_t i;
	uint16_t slot;

	for (i = 0; i < heap->mm_qcount; i++) {
		slot = (heap->mm_qhead + i) % QUARANTINE_SLOTS;
		if (heap->mm_quarantine[slot] == node) {
			return true;
		}
	}

	return false;
}

/****************************************************************************
 * Name: mm_quarantine_add
 *
 * Description:
 *   Offer a chunk which has just been freed to the quarantine.
 *
 *   Returns true when the chunk has been taken, in which case the caller
 *   must not touch it any further: it keeps MM_ALLOC_BIT set and stays out
 *   of the free list, so its address cannot be handed to anybody else until
 *   it ages out. Returns false when the chunk should be freed normally.
 *
 *   A chunk larger than CONFIG_DEBUG_MM_QUARANTINE_MAX_SIZE is never taken.
 *   Holding a large buffer costs more of a small heap than the extra
 *   detection is worth.
 *
 *   NOTES:
 *     (1) node->size must already be set to its final value.
 *     (2) MM_ALLOC_BIT must still be set in node->preceding.
 *     (3) the caller must hold the MM semaphore.
 *
 ****************************************************************************/

bool mm_quarantine_add(FAR struct mm_heap_s *heap, FAR struct mm_allocnode_s *node)
{
	uint16_t slot;

	if (node->size > CONFIG_DEBUG_MM_QUARANTINE_MAX_SIZE) {
		return false;
	}

	/* Make room. Releasing the oldest entries here is safe even though the
	 * caller's chunk may be adjacent to one of them: the caller's chunk still
	 * has MM_ALLOC_BIT set, so a released neighbour cannot merge into it.
	 */

	while (heap->mm_qcount >= QUARANTINE_SLOTS ||
		   heap->mm_qbytes + node->size > CONFIG_DEBUG_MM_QUARANTINE_BYTES) {
		if (heap->mm_qcount == 0) {
			/* The chunk on its own does not fit under the byte budget */

			return false;
		}

		quarantine_release_oldest(heap);
	}

	/* Lay down the pattern - unified function handles metadata internally */
	quarantine_poison_chunk(node);

	slot = (heap->mm_qhead + heap->mm_qcount) % QUARANTINE_SLOTS;
	heap->mm_quarantine[slot] = node;
	heap->mm_qcount++;
	heap->mm_qbytes += node->size;

	return true;
}

/****************************************************************************
 * Name: mm_quarantine_flush
 *
 * Description:
 *   Release everything the quarantine is holding and return the number of
 *   bytes handed back to the allocator.
 *
 *   This is what keeps the quarantine from turning into spurious allocation
 *   failures: malloc calls it before giving up, so memory which is only held
 *   back for debugging is always available to a real request.
 *
 *   NOTES:
 *     (1) the caller must hold the MM semaphore.
 *
 ****************************************************************************/

size_t mm_quarantine_flush(FAR struct mm_heap_s *heap)
{
	size_t released = heap->mm_qbytes;

	while (heap->mm_qcount > 0) {
		quarantine_release_oldest(heap);
	}

	return released;
}
