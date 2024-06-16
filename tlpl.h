#include <assert.h>
#include <stdint.h>

#ifndef TLPL_LOG_DATA_T
#define TLPL_LOG_DATA_T int
#endif

#ifndef TLPL_LOG_BATCH_CALLOC_F

#include <stdio.h>
#include <stdlib.h>

#define TLPL_LOG_BATCH_CALLOC_F(size, file, line) ({					\
	void *_ = calloc(1, size);							\
	if (!_) {									\
		fprintf(stderr, "Fatal error at %s:%d: Can't allocate a log batch.",	\
			file, line);							\
		exit(-1);								\
	}										\
	_;										\
})

#endif /* TLPL_LOG_BATCH_CALLOC_F */

#ifndef TLPL_LOG_BATCH_SIZE
#define TLPL_LOG_BATCH_SIZE 1024 * 1024 - 1
#endif

struct tlpl_log {
	struct timespec t;
	uint32_t allocations;
	uint32_t retries;
	TLPL_LOG_DATA_T data;
};

struct tlpl_log_batch_header {
	struct tlpl_log_batch *next;
	/** Count of written log entries. */
	size_t size;
};

struct tlpl_log_batch {
	struct tlpl_log_batch_header header;
	struct tlpl_log logs[TLPL_LOG_BATCH_SIZE];
};

struct tlpl {
	struct tlpl_log_batch *batch_first;
	struct tlpl_log_batch *batch_current;
};

static struct tlpl_log_batch *
tlpl_log_batch_new()
{
	struct tlpl_log_batch *result =
		TLPL_LOG_BATCH_CALLOC_F(sizeof(*result), __FILE__, __LINE__);
	if (result == NULL)
		return NULL;
	return result;
}

static int
tlpl_init(struct tlpl *tlpl)
{
	tlpl->batch_first = tlpl_log_batch_new();
	if (tlpl->batch_first == NULL)
		return 0;
	tlpl->batch_current = tlpl->batch_first;
	return 1;
}

static int
tlpl_log(struct tlpl *restrict tlpl, TLPL_LOG_DATA_T data)
{
	struct tlpl_log_batch *batch = tlpl->batch_current;
	if (batch->header.size == TLPL_LOG_BATCH_SIZE) {
		struct tlpl_log_batch *next = tlpl_log_batch_new();
		if (next == NULL)
			return 0;
		batch->header.next = next;
		tlpl->batch_current = next;
		batch = next;
	}
	size_t i = batch->header.size++;
	clock_gettime(CLOCK_MONOTONIC, &batch->logs[i].t);
	batch->logs[i].data = data;
	return 1;
}
