#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/rwsem.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>

MODULE_AUTHOR("Vadim Kochan <vadim4j@gmail.com>");
MODULE_DESCRIPTION("Concurrent r/w semaphor demo");
MODULE_LICENSE("GPL");

#ifndef MAX_READERS
#define MAX_READERS	3
#endif

#ifndef MAX_WRITERS
#define MAX_WRITERS	1
#endif

#define MAX_THRDS	(MAX_READERS + MAX_WRITERS)

static struct task_struct *thrds[MAX_THRDS];
unsigned int thrds_num;

static atomic_t readers = ATOMIC_INIT(0);
static DECLARE_RWSEM(sem_stat);

static inline void reader_lock(void)
{
	down_read(&sem_stat);

	atomic_inc(&readers);
	barrier();
	udelay(100);
}

static inline void reader_unlock(void)
{
	pr_info("active readers(%d)\n", atomic_read(&readers));
	barrier();
	atomic_dec(&readers);
	up_read(&sem_stat);
}

static inline void writer_lock(void)
{
	down_write(&sem_stat);
	udelay(100);
}

static inline void writer_unlock(void)
{
	up_write(&sem_stat);
}

static int reader_thread(void *data)
{
	pr_info("started: %s\n", current->comm);

	while (!kthread_should_stop()) {
		reader_lock();
		reader_unlock();
	};

	pr_info("finished: %s\n", current->comm);
	return 0;
}

static int writer_thread(void *data)
{
	pr_info("started: %s\n", current->comm);

	while (!kthread_should_stop()) {
		writer_lock();
		writer_unlock();
	};

	pr_info("finished: %s\n", current->comm);
	return 0;
}

static void stop_threads(void)
{
	int i;

	for (i = 0; i < thrds_num; i++) {
		if (kthread_stop(thrds[i]))
			pr_err("failed to stop thread%d\n", i);
	}
}

static int create_threads(void)
{
	unsigned int cpus_num = num_online_cpus();
	unsigned int cpu;
	unsigned int i;

	if (cpus_num < MAX_THRDS) {
		pr_err("Need %u more CPUs!!!\n", MAX_THRDS - thrds_num);
		return -EINVAL;
	}

	/* we are interested only in working cpus */
	for_each_online_cpu(cpu) {
		struct task_struct *task;

		if (thrds_num >= MAX_THRDS)
			break;

		if (thrds_num >= MAX_READERS) {
			task = kthread_create(writer_thread, NULL,
					"krwsem_writer%u", thrds_num -
					MAX_READERS);
		} else {
			task = kthread_create(reader_thread, NULL,
					"krwsem_reader%u", thrds_num);
		}

		if (IS_ERR(task)) {
			stop_threads();

			pr_err("failed to start thread%u\n", thrds_num);
			return -1;
		}

		kthread_bind(task, cpu);
		thrds[thrds_num++] = task;
	}

	/* wake up threads in separate loop to have less delay between create &
	 * start of each thread */
	for (i = 0; i < thrds_num; i++)
		wake_up_process(thrds[i]);

	return 0;
}

static __init int rwsem_init(void)
{
	int err;

	pr_info("RWSem init\n");

	if ((err = create_threads()))
		return err;

	return 0;
}

static __exit void rwsem_exit(void)
{
	pr_info("RWSem exit\n");

	stop_threads();
}

module_init(rwsem_init);
module_exit(rwsem_exit);
