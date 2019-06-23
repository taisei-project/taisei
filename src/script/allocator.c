/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "allocator.h"
#include "util.h"

#include <unistd.h>
#include <sys/mman.h>

typedef uint8_t mem_t;

#define LA_ALLOC_MINEXPONENT 4
#define LA_ALLOC_MINSIZE (1 << LA_ALLOC_MINEXPONENT)
#define LA_ALIGNMENT (alignof(max_align_t))
#define LA_ALIGN alignas(LA_ALIGNMENT)
#define LA_ALIGNED(ptr) ASSUME_ALIGNED(ptr, LA_ALIGNMENT)

// #define LA_DEBUG(...) log_debug(__VA_ARGS__)
#define LA_DEBUG(...) ((void)0)

typedef struct block {
	LA_ALIGN struct block *next;
} block_t;

typedef struct blocklist {
	block_t *head;
	// block_t *tail;
} blocklist_t;

#ifndef NDEBUG
#define LA_STATS
#endif

static struct la {
	mem_t **pages;
	blocklist_t *free_blocks;
	size_t page_size;
	size_t num_pages;

#ifdef LA_STATS
	struct {
		size_t small_used;
		size_t large_used;
	} stats;
#endif
} la;

#ifdef LA_STATS
#define LA_IFSTATS(...) __VA_ARGS__
#else
#define LA_IFSTATS(...)
#endif

static_assert(LA_ALLOC_MINSIZE >= LA_ALIGNMENT, "LA_ALLOC_MINEXPONENT is too small to guarantee alignment, please adjust");
static_assert(LA_ALLOC_MINSIZE >= sizeof(block_t), "LA_ALLOC_MINEXPONENT is too small to contain a block header, please adjust");

INLINE size_t get_page_size(void) {
	return sysconf(_SC_PAGESIZE);
}

INLINE block_t *as_block(mem_t *mem) {
	return CASTPTR_ASSUME_ALIGNED(mem, block_t);
}

void lmemstatstr(char *buffer, size_t bufsize) {
#ifdef LA_STATS
	snprintf(buffer, bufsize, "%zu pages totaling %zukb, of which %zukb is in use, plus %zukb used on large allocations: total %zukb in use out of %zukb reserved (%.4g%%)",
		la.num_pages,
		(la.num_pages * la.page_size) / 1024,
		la.stats.small_used / 1024,
		la.stats.large_used / 1024,
		(la.stats.small_used + la.stats.large_used) / 1024,
		(la.num_pages * la.page_size + la.stats.large_used) / 1024,
		100 * (double)(la.stats.small_used + la.stats.large_used) / (double)(la.num_pages * la.page_size + la.stats.large_used)
	);
#else
	*buffer = 0;
#endif
}

static void la_memstat(void) {
	/*
	LA_DEBUG("%zu pages totaling %zukb, of which %zukb is in use, plus %zukb used on large allocations: total %zukb in use out of %zukb reserved (%.4g%%)",
		la.num_pages,
		(la.num_pages * la.page_size) / 1024,
		la.stats.small_used / 1024,
		la.stats.large_used / 1024,
		(la.stats.small_used + la.stats.large_used) / 1024,
		(la.num_pages * la.page_size + la.stats.large_used) / 1024,
		100 * (double)(la.stats.small_used + la.stats.large_used) / (double)(la.num_pages * la.page_size + la.stats.large_used)
	);
	*/
}

size_t malloc_usable_size (const void *ptr);

INLINE void *large_alloc(size_t size) {
	void *ptr = malloc(size);
	LA_IFSTATS(
		la.stats.large_used += malloc_usable_size(ptr);
		la_memstat();
	)
	return ptr;
}

INLINE void *large_calloc(size_t num, size_t size) {
	void *ptr = calloc(num, size);
	LA_IFSTATS(
		la.stats.large_used += malloc_usable_size(ptr);
		la_memstat();
	)
	return ptr;
}

INLINE void *large_realloc(void *ptr, size_t size) {
	LA_IFSTATS(la.stats.large_used -= malloc_usable_size(ptr);)
	ptr = realloc(ptr, size);
	LA_IFSTATS(la.stats.large_used += malloc_usable_size(ptr);)
	la_memstat();
	return ptr;
}

INLINE void large_free(void *ptr) {
	LA_IFSTATS(la.stats.large_used -= malloc_usable_size(ptr);)
	free(ptr);
	la_memstat();
}

INLINE uint32_t bin_index(uint32_t sz) {
    if(sz <= LA_ALLOC_MINSIZE) return 0;
	return 32u - LA_ALLOC_MINEXPONENT - (uint32_t)__builtin_clz(sz - 1);
}

attr_nonnull(1, 2) attr_used
static void push_block(blocklist_t *restrict list, block_t *restrict block) {
	block->next = LA_ALIGNED(list->head);
	list->head = LA_ALIGNED(block);
}

attr_nonnull(1)
static block_t *pop_block(blocklist_t *restrict list) {
	block_t *blk = LA_ALIGNED(list->head);

	if(blk) {
		list->head = LA_ALIGNED(blk->next);
		return blk;
	}

	return NULL;
}

attr_nonnull(1, 2)
static void slice_page(mem_t *page, blocklist_t *restrict blocklist, uint32_t blocksize) {
	block_t *blocks = as_block(page);
	block_t *end = as_block(page + la.page_size);
	const uint32_t step = blocksize / (uint32_t)sizeof(block_t);

	LA_DEBUG("page=%p  blocklist=%p  blocksize=%u  step=%u", (void*)page, (void*)blocklist, blocksize, step);

	assert(blocklist->head == NULL);

	blocklist->head = blocks;
	block_t *tail = blocks;

	for(block_t *block = blocks + step; block < end; block += step) {
		tail->next = block;
		tail = block;
	}

	tail->next = NULL;
}

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS 0x20
#endif

attr_returns_nonnull
static mem_t *fetch_page(void) {
	return mmap(NULL, la.page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	// return large_alloc(la.page_size);
}

static void free_page(mem_t *page) {
	munmap(page, la.page_size);
	// large_free(page);
}

attr_nonnull(1)
static void add_page(blocklist_t *restrict blocklist, uint32_t blocksize) {
	mem_t *page = fetch_page();
	slice_page(page, blocklist, blocksize);
	la.pages = large_realloc(la.pages, (++la.num_pages) * sizeof(*la.pages));
	la.pages[la.num_pages - 1] = page;
	la_memstat();
}

attr_returns_nonnull
static block_t *alloc_block(uint32_t blocksize) {
	if(blocksize > la.page_size) {
		LA_DEBUG("defer to large_alloc (%u)", blocksize);
		return large_alloc(blocksize);
	}

	uint32_t bin = bin_index(blocksize);
	blocklist_t *freelist = la.free_blocks + bin;
	block_t *blk = pop_block(freelist);

	LA_IFSTATS(la.stats.small_used += (1 << (bin + LA_ALLOC_MINEXPONENT));)
	la_memstat();

	if(blk) {
		return blk;
	}

	add_page(freelist, 1 << (bin + LA_ALLOC_MINEXPONENT));
	blk = pop_block(freelist);
	assume(blk != NULL);
	return blk;
}

static void free_block(block_t *block, size_t blocksize) {
	if(blocksize > la.page_size) {
		LA_DEBUG("%p defer to large_free (%zu)", (void*)block, blocksize);
		large_free(block);
		return;
	}

	if(block) {
		blocklist_t *restrict freelist = la.free_blocks + bin_index(blocksize);
		LA_IFSTATS(la.stats.small_used -= (1 << (bin_index(blocksize) + LA_ALLOC_MINEXPONENT));)
		la_memstat();
		push_block(freelist, block);
	}
}

attr_returns_nonnull attr_nonnull(1)
static block_t *realloc_block(block_t *block, size_t oldsize, size_t newsize) {
	if(oldsize > la.page_size && newsize > la.page_size) {
		LA_DEBUG("%p defer to large_alloc (%zu -> %zu)", (void*)block, oldsize, newsize);
		return large_realloc(block, newsize);
	}

	uint32_t oldbin = bin_index(oldsize);
	uint32_t newbin = bin_index(newsize);

	if(oldbin == newbin) {
		return block;
	}

	block_t *newblock = alloc_block(newsize);
	memcpy(newblock, block, oldsize < newsize ? oldsize : newsize);
	free_block(block, oldsize);

	return newblock;
}

void *lalloc(void *restrict userdata, void *restrict ptr, size_t oldsize, size_t newsize) {
	if(newsize == 0) {
		free_block(ptr, oldsize);
		return NULL;
	}

	if(ptr == NULL) {
		return alloc_block(newsize);
	}

	return realloc_block(ptr, oldsize, newsize);
}

void lalloc_init(void) {
	la.page_size = get_page_size();
	la.free_blocks = large_calloc(bin_index(la.page_size) + 1, sizeof(*la.free_blocks));
}

void lalloc_shutdown(void) {
	large_free(la.free_blocks);

	for(uint i = 0; i < la.num_pages; ++i) {
		free_page(la.pages[i]);
	}

	large_free(la.pages);
	memset(&la, 0, sizeof(la));
}

