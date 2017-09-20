#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/rbtree.h>

static struct rb_root tree = RB_ROOT;

struct interval_node {
	struct rb_node node;
	u32 min;
	u32 max;
};

static struct interval_node intervals[] = {
	{ .min = 3, .max = 6 },
	{ .min = 7, .max = 9 },
	{ .min = 10, .max = 15 },
	/* should be skipped */
	{ .min = 4, .max = 5 },
	/* should update [3,6] -> [1,6] */
	{ .min = 1, .max = 4 },
};

static void insert_interval(struct rb_root *root, struct interval_node *n)
{
	struct rb_node **new = &root->rb_node, *parent = NULL;

	while (*new) {
		struct interval_node *x = container_of(*new, struct
				interval_node, node);

		parent = *new;

		if (n->min > x->max) {
			new = &((*new)->rb_right);
		} else if (n->max < x->min) {
			new = &((*new)->rb_left);
		} else {
			u32 new_min = min(n->min, x->min);
			u32 new_max = max(n->max, x->max);

			/* update interval if needed */
			if (x->min != new_min || x->max != new_max) {
				pr_info("update:[%u,%u] -> [%u,%u]\n",
					x->min, x->max, new_min, new_max);

				x->min = new_min;
				x->max = new_max;
				return;
			}

			/* new interval already exists in the another one */
			pr_info("skip:[%u,%u]\n", n->min, n->max);
			return;
		}
	}

	rb_link_node(&n->node, parent, new);
	rb_insert_color(&n->node, root);
	pr_info("add:[%u,%u]\n", n->min, n->max);
}

static struct interval_node *search_interval(struct rb_root *root, u32 val)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct interval_node *n = container_of(node, struct
				interval_node, node);

		if (val < n->min)
			node = node->rb_left;
		else if (val > n->max)
			node = node->rb_right;
		else
			return n;
	}

	return NULL;
}

static int remove_interval(struct rb_root *root, u32 val)
{
	struct interval_node *interv;

	interv = search_interval(root, val);
	if (!interv)
		return -1;

	rb_erase(&interv->node, root);
	pr_info("removed:[%u,%u]\n", interv->min, interv->max);
	return 0;
}

static void print_tree(struct rb_root *root)
{
	struct rb_node *node;

	pr_info("\n");
	pr_info("********** dump tree begin **********\n");
	for (node = rb_first(root); node; node = rb_next(node)) {
		struct interval_node *inter = rb_entry(node,
				struct interval_node, node);

		pr_info("[%u,%u]\n", inter->min, inter->max);
	}
	pr_info("********** dump tree end **********\n\n");
}

static int interval_tree_init(void)
{
	struct interval_node *find;
	int i;

	pr_info("Interval Tree: init\n");
	pr_info("===================\n");

	for (i = 0; i < ARRAY_SIZE(intervals); i++) {
		insert_interval(&tree, &intervals[i]);
	}

	print_tree(&tree);

	find = search_interval(&tree, 1);
	if (find) {
		pr_info("found:[%u,%u]\n", find->min, find->max);
	}

	find = search_interval(&tree, 10);
	if (find) {
		pr_info("found:[%u,%u]\n", find->min, find->max);
	}

	if (remove_interval(&tree, 10)) {
		pr_err("failed to remove interval with value=%u\n", 10);
		return -1;
	}
	find = search_interval(&tree, 10);
	if (find) {
		pr_err("still exists:[%u,%u]\n", find->min, find->max);
	}

	print_tree(&tree);

	return 0;
}

static void interval_tree_exit(void)
{
	pr_info("Interval Tree: exit\n");
	pr_info("===================\n\n");
}

module_init(interval_tree_init);
module_exit(interval_tree_exit);

MODULE_AUTHOR("Vadim Kochan <vadim4j@gmail.com>");
MODULE_DESCRIPTION("Interval Tree Module");
MODULE_LICENSE("GPL");
