#include <stdio.h>
#include <pthread.h>

pthread_t th0, th1;

int count = 100000;
int val = 0;

void *inc_nonatomic(void *arg)
{
	int i;

	for (i = 0; i < count; i++) {
		val++;
	}

	return NULL;
}

void *inc_atomic(void *arg)
{
	int i;

	for (i = 0; i < count; i++) {
		__sync_fetch_and_add(&val, 1);
	}

	return NULL;
}

int run_threads(void * (*inc)(void *arg))
{
	int err = 0;

	err = pthread_create(&th0, NULL, inc, NULL);
	if (err) {
		fprintf(stderr, "Failed create thread th0\n");
		return err;
	}

	err = pthread_create(&th1, NULL, inc, NULL);
	if (err) {
		fprintf(stderr, "Failed create thread th1\n");
		return err;
	}

	return 0;
}

int wait_threads(void)
{
	int err;

	err = pthread_join(th0, NULL);
	if (err) {
		fprintf(stderr, "Failed join for thread th0\n");
	}

	err = pthread_join(th1, NULL);
	if (err) {
		fprintf(stderr, "Failed join for thread th1\n");
		return err;
	}

	return err;
}

int main(int argc, char **argv)
{
	int err = 0;

	printf("Run non-atomic incrementing with 2 threads:\n");

	if (run_threads(inc_nonatomic))
		return -1;
	if (wait_threads())
		return -1;

	printf("\tActual=%d, Expected=%d\n", val, count * 2);

	printf("\n");
	printf("Run atomic incrementing with 2 threads:\n");

	val = 0;

	if (run_threads(inc_atomic))
		return -1;
	if (wait_threads())
		return -1;

	printf("\tActual=%d, Expected=%d\n", val, count * 2);
	return 0;
}
