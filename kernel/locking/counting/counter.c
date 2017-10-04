#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>

MODULE_AUTHOR("Vadim Kochan <vadim4j@gmail.com>");
MODULE_DESCRIPTION("Concurrent counting demo");
MODULE_LICENSE("GPL");

static volatile unsigned int count;

#ifndef TIMES_INC
#define TIMES_INC	1000000
#endif

#ifndef MAX_JOBS
#define MAX_JOBS	2
#endif

typedef struct {
	unsigned long locked;
} my_lock_t;

#define MY_LOCK_INIT() { .locked = 0 }

static inline void my_lock(my_lock_t *lock)
{
#ifdef CONFIG_MY_LOCK
	/* indication of that we owned the lock is that previous
	 * state == 0 which means unlocked, otherwise the lock is taken
	 * by someone else and we need to busy-wait till the lock will be
	 * cleared back by my_unlock(x) */
	while (test_and_set_bit(0, &lock->locked) != 0)
		cpu_relax();
#endif
}

static inline void my_unlock(my_lock_t *lock)
{
#ifdef CONFIG_MY_LOCK
	/* clear back the lock to 0, which will allow to own the lock by waiting
	 * CPUs */
	if (test_and_clear_bit(0, &lock->locked) == 0)
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

static unsigned int jobs_num;

static int jobs_init(void)
{
	unsigned int cpus_num = num_online_cpus();
	unsigned int cpu;

	if (cpus_num < MAX_JOBS) {
		pr_err("Need %u more CPUs!!!\n", MAX_JOBS - cpus_num);
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

	for (i = 0; i < jobs_num; i++)
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
