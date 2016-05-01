#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

#define barrier() __asm__ __volatile__("mfence" ::: "memory")
/* #define barrier() __sync_synchronize() */

static sem_t thread0_start;
static sem_t thread0_end;
static pthread_t th0;

static sem_t thread1_start;
static sem_t thread1_end;
static pthread_t th1;

static volatile bool quit = false;
static volatile int a,b,x,y;

static void *no_barrier_x(void *arg)
{
	while (!quit) {
		sem_wait(&thread0_start);
		usleep(50);
		a = 1;
		x = b;
		sem_post(&thread0_end);
	}

	return NULL;
}

static void *no_barrier_y(void *arg)
{
	while (!quit) {
		sem_wait(&thread1_start);
		usleep(30);
		b = 1;
		y = a;
		sem_post(&thread1_end);
	}

	return NULL;
}

static void *barrier_x(void *arg)
{
	while (!quit) {
		sem_wait(&thread0_start);
		usleep(50);
		a = 1;
		barrier();
		x = b;
		sem_post(&thread0_end);
	}

	return NULL;
}

static void *barrier_y(void *arg)
{
	while (!quit) {
		sem_wait(&thread1_start);
		usleep(30);
		b = 1;
		barrier();
		y = a;
		sem_post(&thread1_end);
	}

	return NULL;
}

int run_test(void * (* test1)(void *), void * (* test2)(void *))
{
	int count = 100000;
	int err = 0;
	int i;

	quit = false;

	sem_init(&thread0_start, 0, 0);
	sem_init(&thread0_end, 0, 0);
	sem_init(&thread1_start, 0, 0);
	sem_init(&thread1_end, 0, 0);

	err = pthread_create(&th0, NULL, test1, NULL);
	if (err) {
		fprintf(stderr, "Failed create thread th0\n");
		return err;
	}

	err = pthread_create(&th1, NULL, test2, NULL);
	if (err) {
		fprintf(stderr, "Failed create thread th1\n");
		return err;
	}

	for (i = 0; i < count; i++) {
		a = b = x = y = 0;

		if (i == (count - 1))
			quit = true;

		sem_post(&thread0_start);
		sem_post(&thread1_start);
		sem_wait(&thread1_end);
		sem_wait(&thread0_end);

		if (x == 0 && y == 0) {
			printf("\tMemory re-ordering on try(%d): x == 0 && y == 0\n", i);
			quit = true;
			sem_post(&thread0_start);
			sem_post(&thread1_start);
			break;
		}
	}

	quit = true;

	err = pthread_join(th0, NULL);
	if (err) {
		fprintf(stderr, "Failed join for thread th0\n");
	}
	err = pthread_join(th1, NULL);
	if (err) {
		fprintf(stderr, "Failed join for thread th1\n");
	}

	return err;
}

int main(int argc, char **argv)
{
	int err;

	printf("Run threads w/o memory barrier\n");
	err = run_test(no_barrier_x, no_barrier_y);
	if (err)
		return -1;

	printf("...........................\n");

	printf("Run threads with memory barrier\n");
	err = run_test(barrier_x, barrier_y);
	if (err)
		return -1;

	return 0;
}
