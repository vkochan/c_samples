#include <stdio.h>

#define MIN_NUM 0
#define MAX_NUM 1000

static int fib(int n)
{
	int F[MAX_NUM + 1];
	int i;

	F[0] = 0;
	F[1] = 1;
	F[2] = 1;

	for (i = 3; i <= n; i++)
		F[i] = F[i - 2] + F[i - 1];

	return F[n];
}

int main(int argc, char **argv)
{
	int n = 0;

	scanf("%d", &n);
	if (n < MIN_NUM) {
		fprintf(stderr, "Invalid number (must be >= %d)\n", MIN_NUM);
		return -1;
	}
	if (n > 1000) {
		fprintf(stderr, "Invalid number (must be < %d)\n", MAX_NUM);
		return -1;
	}

	printf("%d\n", fib(n));
	return 0;
}
