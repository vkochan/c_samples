#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

struct data_node {
	int id;
};

static struct kmem_cache *node_cache;
static long nodes_cached;

static inline struct data_node *node_alloc(void)
{
	struct data_node *n;

	n = (struct data_node *) kmem_cache_alloc(node_cache, GFP_KERNEL);
	if (!n) {
		pr_err("Failed to alloc node\n");
		return NULL;
	}

	pr_info("Allocated node:id=%d\n", n->id);
	return n;
}

static inline void node_free(struct data_node *n)
{
	kmem_cache_free(node_cache, n);
	pr_info("Freed node:id=%d\n", n->id);
}

static void node_ctor(void *p)
{
	struct data_node *n = (struct data_node *) p;

	memset(n, 0, sizeof(*n));
	n->id = nodes_cached++;

	pr_info("Cached new node:id=%d\n", n->id);
}

static int kcache_test(void)
{
	struct data_node *n1 = NULL, *n2 = NULL, *n3 = NULL;
	int err = 0;

	n1 = node_alloc();
	if (!n1) {
		err = -ENOMEM;
		goto out;
	}

	n2 = node_alloc();
	if (!n2) {
		err = -ENOMEM;
		goto out;
	}

	n3 = node_alloc();
	if (!n3) {
		err = -ENOMEM;
		goto out;
	}

	node_free(n2);

	/* we should got first free node (id=1) */
	n2 = node_alloc();
	if (!n2) {
		err = -ENOMEM;
		goto out;
	}

out:
	if (n1)
		node_free(n1);
	if (n2)
		node_free(n2);
	if (n3)
		node_free(n3);

	return err;
}

static int kcache_ctor_init(void)
{
	int err;

	pr_info("kcache_ctor: init\n");

	node_cache = kmem_cache_create("my_node_cache", sizeof(struct data_node),
			0, SLAB_RECLAIM_ACCOUNT, node_ctor);
	if (!node_cache) {
		pr_err("Failed to create node cache\n");
		return -ENOMEM;
	}

	err = kcache_test();
	if (err) {
		kmem_cache_destroy(node_cache);
		node_cache = NULL;
		return err;
	}

	return 0;
}

static void kcache_ctor_exit(void)
{
	pr_info("kcache_ctor: exit\n");
	kmem_cache_destroy(node_cache);
}

module_init(kcache_ctor_init);
module_exit(kcache_ctor_exit);

MODULE_AUTHOR("Vadim Kochan <vadim4j@gmail.com>");
MODULE_DESCRIPTION("kmem_cache_t constructor example");
MODULE_LICENSE("GPL");
