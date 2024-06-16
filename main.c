#include <stdio.h>
#include <pthread.h>

#ifndef THREAD_COUNT
#define THREAD_COUNT 8
#endif

#ifndef LOGS_PER_THREAD
#define LOGS_PER_THREAD 10000000ULL
#endif

#ifndef LOG_BATCH_SIZE
#define LOG_BATCH_SIZE 1000000ULL
#endif

/** Semi-lock-free performance log. */
#ifdef SLFPL

#define SLFPL_LOG_BATCH_SIZE LOG_BATCH_SIZE
#include "slfpl.h"

static struct slfpl slfpl;

static void logger_init() {
	slfpl_init(&slfpl);
}

static void logger_init_local() {
	return;
}

static void logger_log(int data) {
	slfpl_log(&slfpl, data);
}

/** Thread-local performance log. */
#else

#define TLPL_LOG_BATCH_SIZE LOG_BATCH_SIZE
#include "tlpl.h"

static _Thread_local struct tlpl tlpl;

static void logger_init() {
	return;
}

static void logger_init_local() {
	tlpl_init(&tlpl);
}

static void logger_log(int data) {
	tlpl_log(&tlpl, data);
}

#endif

static double seconds_between(struct timespec *t0, struct timespec *t1) {
	double sec_1 = (double)t0->tv_sec + (double)t0->tv_nsec / 1000000000.0;
	double sec_2 = (double)t1->tv_sec + (double)t1->tv_nsec / 1000000000.0;
	return sec_2 - sec_1;
}

static void *thread_f(void *arg) {
	logger_init_local();
	for (size_t i = 0; i < LOGS_PER_THREAD; i++)
		logger_log(i);
	return NULL;
}

int main() {
	logger_init();
	struct timespec begin, end;
	pthread_t threads[THREAD_COUNT];
	clock_gettime(CLOCK_MONOTONIC, &begin);
	for (size_t i = 0; i < THREAD_COUNT; ++i)
		pthread_create(&threads[i], NULL, thread_f, NULL);
	for (size_t i = 0; i < THREAD_COUNT; ++i)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double secs = seconds_between(&begin, &end);
	double lps = (double)(LOGS_PER_THREAD * THREAD_COUNT) / secs;
	printf("Logs per second: %.0f\n", lps);
}
