#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

#ifndef SLFPL_LOG_DATA_T
#define SLFPL_LOG_DATA_T int
#endif

#ifndef SLFPL_LOG_BATCH_CALLOC_F

#include <stdio.h>
#include <stdlib.h>

#define SLFPL_LOG_BATCH_CALLOC_F(size, file, line) ({					\
	void *_ = calloc(1, size);							\
	if (!_) {									\
		fprintf(stderr, "Fatal error at %s:%d: Can't allocate a log batch.",	\
			file, line);							\
		exit(-1);								\
	}										\
	_;										\
})

#endif /* SLFPL_LOG_BATCH_CALLOC_F */

#ifndef SLFPL_LOG_BATCH_SIZE
#define SLFPL_LOG_BATCH_SIZE 1024 * 1024 - 1
#endif

struct slfpl_log {
	struct timespec t;
	uint32_t allocations;
	uint32_t retries;
	SLFPL_LOG_DATA_T data;
};

struct slfpl_log_batch_header {
	struct slfpl_log_batch *next;
	/** The new log batch allocation guard. */
	pthread_mutex_t mutex;
	/** The new log batch allocation readiness signal. */
	pthread_cond_t cond;
	/** Count of written log entries. */
	_Atomic size_t size;
};

struct slfpl_log_batch {
	struct slfpl_log_batch_header header;
	struct slfpl_log logs[SLFPL_LOG_BATCH_SIZE];
};

struct slfpl {
	unsigned allocation_failed;
	struct slfpl_log_batch *batch_first;
	struct slfpl_log_batch *_Atomic batch_current;
};

static struct slfpl_log_batch *
slfpl_log_batch_new()
{
	struct slfpl_log_batch *result =
		SLFPL_LOG_BATCH_CALLOC_F(sizeof(*result), __FILE__, __LINE__);
	if (result == NULL)
		return NULL;
	pthread_mutex_init(&result->header.mutex, NULL);
	pthread_cond_init(&result->header.cond, NULL);
	return result;
}

static int
slfpl_init(struct slfpl *slfpl)
{
	slfpl->batch_first = slfpl_log_batch_new();
	if (slfpl->batch_first == NULL)
		return 0;
	atomic_store_explicit(&slfpl->batch_current, slfpl->batch_first, memory_order_relaxed);
	return 1;
}

static int
slfpl_log(struct slfpl *restrict slfpl, SLFPL_LOG_DATA_T data)
{
	struct slfpl_log_batch *batch = atomic_load_explicit(&slfpl->batch_current, memory_order_relaxed);
	size_t i = 0;
	unsigned allocations = 0;
	unsigned retries = 0;

	for (;;) {
		i = atomic_fetch_add_explicit(&batch->header.size, 1, memory_order_relaxed);
		if (i < SLFPL_LOG_BATCH_SIZE) {
			clock_gettime(CLOCK_MONOTONIC, &batch->logs[i].t);
			batch->logs[i].allocations = allocations;
			batch->logs[i].retries = retries;
			batch->logs[i].data = data;
			return 1;
		} else if (i == SLFPL_LOG_BATCH_SIZE) {
			/* We're responsible for allocating a new log batch. */
			struct slfpl_log_batch *next = slfpl_log_batch_new();
			pthread_mutex_lock(&batch->header.mutex);
			if (next == NULL) {
				slfpl->allocation_failed = 1;
				pthread_cond_broadcast(&batch->header.cond);
				pthread_mutex_unlock(&batch->header.mutex);
				return 0;
			}
			batch->header.next = next;
			atomic_store_explicit(&slfpl->batch_current, next, memory_order_relaxed);
			pthread_cond_broadcast(&batch->header.cond);
			pthread_mutex_unlock(&batch->header.mutex);
			batch = next;
			allocations++;
			continue;
		} else {
			assert(i > SLFPL_LOG_BATCH_SIZE);
			/* Wait until the new log batch is allocated. XXX: can the waiter lock first, and what happens then if can? */
			pthread_mutex_lock(&batch->header.mutex);
			while (batch->header.next == NULL && !slfpl->allocation_failed)
				pthread_cond_wait(&batch->header.cond, &batch->header.mutex);
			if (slfpl->allocation_failed) {
				pthread_mutex_unlock(&batch->header.mutex);
				return 0;
			}
			pthread_mutex_unlock(&batch->header.mutex);
			batch = batch->header.next;
			retries++;
			continue;
		}
	}
	/* Unreachable. */
	assert(0);
	return 0;
}
