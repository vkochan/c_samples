#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>

MODULE_AUTHOR("Vadim Kochan <vadim4j@gmail.com>");
MODULE_DESCRIPTION("Concurrent counting demo");
MODULE_LICENSE("GPL");

/* place in memory where concurrency happens ... */
static volatile unsigned int count;

#ifndef TIMES_INC
#define TIMES_INC	1000000
#endif

#ifndef MAX_JOBS
#define MAX_JOBS	2
#endif

typedef struct {
	unsigned long count;
} my_lock_t;

#define MY_LOCK_INIT() { .count = 0L }

static inline void my_lock(my_lock_t *lock)
{
#ifdef CONFIG_MY_LOCK
	while (test_and_set_bit(0, &lock->count) != 0)
		;
#endif
}

static inline void my_unlock(my_lock_t *lock)
{
#ifdef CONFIG_MY_LOCK
	if (test_and_clear_bit(0, &lock->count) == 0)
		BUG();
#endif
}

static my_lock_t count_lock = MY_LOCK_INIT();

static void counter_func(struct work_struct *work)
{
	unsigned int cpu = smp_processor_id();
	int i;

	pr_info("Started job #%u\n", cpu);

	for (i = 0; i < TIMES_INC; i++) {
		my_lock(&count_lock);
		count++;
		my_unlock(&count_lock);
	}

	pr_info("finished job #%u\n", cpu);
}

static DECLARE_WORK(counter_work1, counter_func);
static DECLARE_WORK(counter_work2, counter_func);

static struct work_struct *jobs[] = {
	&counter_work1,
	&counter_work2,
};

static int jobs_init(void)
{
	unsigned int jobs_num = 0;
	unsigned int cpu;

	if (num_online_cpus() < MAX_JOBS) {
		pr_err("Need one more CPU!!!\n");
		return -EINVAL;
	}

	/* we are interested only in working cpus */
	for_each_online_cpu(cpu) {
		if (jobs_num >= MAX_JOBS)
			break;

		pr_info("Scheduling job #%u\n", cpu);

		schedule_work_on(cpu, jobs[jobs_num++]);
	}

	return 0;
}

static void jobs_finish(void)
{
	int i;

	for (i = 0; i < MAX_JOBS; i++)
		cancel_work_sync(jobs[i]);
}

static __init int counter_init(void)
{
	int ret;

	pr_info("Initializing concurrent counter module\n");

	ret = jobs_init();
	if (ret)
		return ret;

	return 0;
}

static __exit void counter_exit(void)
{
	pr_info("Exiting concurrent counter module\n");

	jobs_finish();

	pr_info("actual(count=%u), expected(count=%u)\n",
			count, TIMES_INC * MAX_JOBS);
}

module_init(counter_init);
module_exit(counter_exit);
