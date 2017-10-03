#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define MAX_COUNT	100000

volatile static bool may_exit = false;
volatile static bool is_done = false;
volatile static int value;
static int count1, count2;

static int stats[100000] = {0};

static void *thread1(void *arg) {
	while (count2++ < MAX_COUNT) {
		is_done = true;
		value = 10;
	}

	return NULL;
}

static void *thread2(void *arg) {
	while (count1 < MAX_COUNT && !may_exit) {
		while (!is_done && !may_exit)
			;

		if (may_exit)
			return NULL;

		stats[count2] = value;
		is_done = false;
		value = 11;
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t tid1, tid2;
	int i;

	pthread_create(&tid1, NULL, thread1, NULL);
	pthread_create(&tid2, NULL, thread2, NULL);

	pthread_join(tid1, NULL);
	may_exit = true;
	pthread_join(tid2, NULL);

	for (i = 0; i < MAX_COUNT - 1; i++) {
		if (stats[i] == 11)
			printf("stats[%d] is reordered\n", i);
	}

	return 0;
}
