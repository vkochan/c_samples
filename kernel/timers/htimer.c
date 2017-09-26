#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

#define MS_TO_NS(x) ((x) * 1000000)

static struct proc_dir_entry *ht_dir;

/* there variables are configurable via /proc/htimer/ entries */
static unsigned long delay_ms = 200L;
static int loop = 5;

enum htimer_var {
	HT_VAR_LOOP,
	HT_VAR_DELAY,
};

struct htimer_obj {
	wait_queue_head_t wait;
	struct seq_file *seq;
	struct hrtimer timer;
	unsigned long delay;
	int loops;
};

static int ht_var_proc_show(struct seq_file *m, void *v)
{
	switch ((long)m->private) {
	case HT_VAR_LOOP:
		seq_printf(m, "%d\n", loop);
		break;

	case HT_VAR_DELAY:
		seq_printf(m, "%lu\n", delay_ms);
		break;
	}

	return 0;
}

static int ht_var_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ht_var_proc_show, PDE_DATA(file_inode(file)));
}

static ssize_t ht_var_proc_write(struct file *file, const char __user *buf,
				 size_t count, loff_t *offp)
{
	enum htimer_var var;

	var = (enum htimer_var) PDE_DATA(file_inode(file));

	switch (var) {
	case HT_VAR_LOOP:
		{
			int val;

			if (kstrtoint_from_user(buf, count, 10, &val)) {
				pr_err("ht_var_proc_write: failed parse loop value\n");
				return -EINVAL;
			}
			loop = val;
		}
		break;

	case HT_VAR_DELAY:
		{
			unsigned long int val;

			if (kstrtoul_from_user(buf, count, 10, &val)) {
				pr_err("ht_var_proc_write: failed parse delay value\n");
				return -EINVAL;
			}
			delay_ms = val;
		}
		break;
	}

	return count;
}

static const struct file_operations ht_var_fops = {
	.open = ht_var_proc_open,
	.read = seq_read,
	.write = ht_var_proc_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static struct htimer_obj *htimer_create(struct seq_file *seq)
{
	struct htimer_obj *obj;

	obj = kmalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return NULL;

	obj->delay = delay_ms;
	obj->loops = loop;
	obj->seq = seq;

	init_waitqueue_head(&obj->wait);
	return obj;
}

static void htimer_destroy(struct htimer_obj *obj)
{
	hrtimer_cancel(&obj->timer);
	kfree(obj);
}

static void htimer_dump(struct seq_file *m)
{
	unsigned long j = jiffies;

	seq_printf(m, "%9li   %3li        %i        %6i    %i    %s\n",
		   j, 0L, in_interrupt() ? 1: 0, current->pid,
		   smp_processor_id(), current->comm);
}

static enum hrtimer_restart htimer_fn(struct hrtimer *t)
{
	struct htimer_obj *obj = container_of(t, struct htimer_obj, timer);

	if (--obj->loops) {
		htimer_dump(obj->seq);

		hrtimer_forward_now(t, ns_to_ktime(MS_TO_NS(obj->delay)));
		return HRTIMER_RESTART;
	}
	obj->loops = 0;

	wake_up_interruptible(&obj->wait);
	return HRTIMER_NORESTART;
}

static void htimer_run_and_wait(struct htimer_obj *obj)
{
	ktime_t kt;

	kt = ktime_set(0, MS_TO_NS(obj->delay));
	hrtimer_init(&obj->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	obj->timer.function = htimer_fn;

	pr_info("Starting timer to fire in %lld ms (%ld)\n",
			ktime_to_ms(obj->timer.base->get_time()) + obj->delay,
				jiffies);

	hrtimer_start(&obj->timer, kt, HRTIMER_MODE_REL);

	wait_event_interruptible(obj->wait, !obj->loops);
}

static int ht_timer_proc_show(struct seq_file *seq, void *v)
{
	struct htimer_obj *timer;
	int ret = 0;

	seq_puts(seq, "time          delta    in_irq?    pid    cpu    cmd\n");
	htimer_dump(seq);

	timer = htimer_create(seq);
	if (!timer)
		return -ENOMEM;

	htimer_run_and_wait(timer);

	if (signal_pending(current)) {
		ret = -ERESTARTSYS;
		goto out;
	}

out:
	htimer_destroy(timer);
	return ret;
}

static int ht_timer_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, ht_timer_proc_show, NULL);
}

static const struct file_operations ht_timer_fops = {
	.open = ht_timer_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int htimer_init(void)
{
	ht_dir = proc_mkdir("htimer", NULL);
	if (!ht_dir) {
		pr_err("Failed to create htimer proc dir\n");
		return -1;
	}

	proc_create_data("loop", 0644, ht_dir, &ht_var_fops, (void *)HT_VAR_LOOP);
	proc_create_data("delay", 0644, ht_dir, &ht_var_fops, (void *)HT_VAR_DELAY);
	proc_create("timer", 0, ht_dir, &ht_timer_fops);

	pr_info("HR timer initialized\n");

	return 0;
}

static void htimer_exit(void)
{
	proc_remove(ht_dir);

	pr_info("HR timer un-initialized\n");
}

module_init(htimer_init);
module_exit(htimer_exit);

MODULE_AUTHOR("Vadim Kochan <vadim4j@gmail.com>");
MODULE_DESCRIPTION("High resolution timer demo");
MODULE_LICENSE("GPL");
