#include <stdio.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

#ifndef SWAP
#define SWAP(a, b) { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; }
#endif

static int partition(int *array, int from, int to)
{
	int pivot = array[to];
	int part_idx = from;
	int i;

	for (i = from; i < to; i++) {
		if (array[i] <= pivot) {
                    SWAP(array[part_idx], array[i]);
		    part_idx++;
		}
	}
	SWAP(array[part_idx], array[to]);

	return part_idx;
}

static void quick_sort(int *array, int from, int to)
{
	if (from < to) {
		int idx = partition(array, from, to);

		quick_sort(array, from, idx - 1);
		quick_sort(array, idx + 1, to);
	}
}

static void print_array(int *array, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		printf("%d,", array[i]);
}

static void test_sort(int *array, size_t len)
{
	printf("Before:");
	print_array(array, len);
	printf("\n");

	quick_sort(array, 0, len - 1);

	printf("After:");
	print_array(array, len);
	printf("\n\n");
}

int main(int argc, char **argv)
{
	int array[] = { 1, 6, 3, 9, 11, 5, 4, 7, 2};
	int array2[] = { 9, 1, 3, 8, 11, 5, 4, 7, 21};

	test_sort(array, ARRAY_SIZE(array));
	test_sort(array2, ARRAY_SIZE(array));

	return 0;
}
