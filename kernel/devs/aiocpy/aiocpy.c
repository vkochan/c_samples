#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/bitmap.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/pagemap.h>

#include "aiocpy.h"

#define DEVICE_NAME "aiocpy"

#define MAX_NUM_PAGES		((PAGE_SIZE / sizeof(struct page *)) / 2)

static struct page **src_pages;
static struct page **dst_pages;

static int maj_num;

struct page_addr {
	unsigned long offs;
	struct page *page;
};

struct tx_req {
	struct page_addr pg_src;
	struct page_addr pg_dst;
	unsigned long len;
};

struct aiocpy_desc {
	struct list_head	entry;
	unsigned long		dst_addr;
	unsigned long		src_addr;
	size_t			length;
};

static int aiocpy_open(struct inode *, struct file *);
static int aiocpy_release(struct inode *, struct file *);
static long aiocpy_ioctl(struct file *, unsigned int, unsigned long);

static inline unsigned long get_page_offset(unsigned long addr)
{
	return addr & (PAGE_SIZE - 1);
}

static inline int get_num_pages(unsigned long addr, size_t len)
{
	unsigned long end = PAGE_ALIGN(addr + len);
	unsigned long start = addr - get_page_offset(addr);

	return (end - start) >> PAGE_SHIFT;
}

static int tx_req_send(struct tx_req *req)
{
	u8 *dst, *src;

	src = kmap(req->pg_src.page);
	if (!src) {
		pr_err("failed to map src page to kernel\n");
		return -1;
	}

	dst = kmap(req->pg_dst.page);
	if (!dst) {
		pr_err("failed to map dst page to kernel\n");
		return -1;
	}

	src += req->pg_src.offs;
	dst += req->pg_dst.offs;

	memcpy(dst, src, req->len);

	kunmap(req->pg_src.page);
	kunmap(req->pg_dst.page);

	if (!PageReserved(req->pg_dst.page))
		SetPageDirty(req->pg_dst.page);

	return 0;
}

static inline unsigned long min_tx_len(unsigned long dst, unsigned long src)
{
	return min(PAGE_SIZE - dst, PAGE_SIZE - src);
}

static int aiocpy_send_req(struct aiocpy_req *req)
{
	int err = 0;
	int i;

	/* request tx desc per each iov */
	for (i = 0; i < req->count; i++) {
		struct aiocpy_iov iov;
		unsigned long dst;
		unsigned long src;
		int src_pg_i = 0;
		int dst_pg_i = 0;
		int src_pg_num;
		int dst_pg_num;
		int length = 0;

		copy_from_user(&iov, &req->iovs[i], sizeof(iov));

		dst = (unsigned long) iov.dst;
		src = (unsigned long) iov.src;

		dst_pg_num = get_num_pages(dst, iov.len);
		if (dst_pg_num > MAX_NUM_PAGES) {
			pr_err("requested dst pages num bigger than max %ld\n", MAX_NUM_PAGES);
			err = -ENOMEM;
			goto out;
		}
		src_pg_num = get_num_pages(src, iov.len);
		if (src_pg_num > MAX_NUM_PAGES) {
			pr_err("requested src pages num bigger than max %ld\n", MAX_NUM_PAGES);
			err = -ENOMEM;
			goto out;
		}

		down_read(&current->mm->mmap_sem);
		if (dst_pg_num != get_user_pages(dst, dst_pg_num, FOLL_FORCE | FOLL_WRITE, dst_pages, NULL)) {

			up_read(&current->mm->mmap_sem);
			pr_err("could not get dst user pages\n");
			err = -ENOMEM;
			goto out;
		}
		if (src_pg_num != get_user_pages(src, src_pg_num, FOLL_FORCE, src_pages, NULL)) {

			up_read(&current->mm->mmap_sem);
			release_pages(dst_pages, dst_pg_num, 0);
			pr_err("could not get src user pages\n");
			err = -ENOMEM;
			goto out;
		}

		while (length < iov.len) {
			struct tx_req tx;

			tx.pg_src.offs = get_page_offset(src);
			tx.pg_dst.offs = get_page_offset(dst);
			tx.pg_src.page = src_pages[src_pg_i];
			tx.pg_dst.page = dst_pages[dst_pg_i];
			tx.len = min_tx_len(tx.pg_dst.offs, tx.pg_src.offs);
			tx.len = min(iov.len - length, tx.len);

			length += tx.len;

			if (tx.pg_src.offs + tx.len >= PAGE_SIZE)
				src_pg_i++;
			if (tx.pg_dst.offs + tx.len >= PAGE_SIZE)
				dst_pg_i++;

			src += tx.len;
			dst += tx.len;

			pr_info("copying: src=%lx, dst=%lx, len=%lu\n", src, dst, tx.len);

			err = tx_req_send(&tx);
			if (err) {
				release_pages(src_pages, src_pg_num, 0);
				release_pages(dst_pages, dst_pg_num, 0);
				up_read(&current->mm->mmap_sem);

				pr_err("failed send tx req\n");
				goto out;
			}
		}
		release_pages(src_pages, src_pg_num, 0);
		release_pages(dst_pages, dst_pg_num, 0);
		up_read(&current->mm->mmap_sem);
	}

out:
	return err;
}

static struct file_operations aiocpy_fops =
{
	.open = aiocpy_open,
	.release = aiocpy_release,
	.unlocked_ioctl = aiocpy_ioctl,
};

static int aiocpy_open(struct inode *inodep, struct file *filep)
{
	return 0;
}

static long aiocpy_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct aiocpy_req req;

	switch (cmd) {
	case AIOCPY_CMD_SEND:
		copy_from_user(&req, (struct aiocpy_req *) arg, sizeof(req));
		return aiocpy_send_req(&req);

	default:
		pr_err("Invalid ioctl: %d\n", cmd);
		return -EINVAL;
	}

	return 0;
}

static int aiocpy_release(struct inode *inodep, struct file *filep)
{
	return 0;
}

static int pages_list_init(void)
{
	src_pages = (struct page **) __get_free_page(GFP_KERNEL);
	if (!src_pages) {
		pr_err("failed to allocate pages list\n");
		return -ENOMEM;
	}

	dst_pages = src_pages + MAX_NUM_PAGES;

	return 0;
}

static void pages_list_destroy(void)
{
	if (src_pages)
		free_page((long unsigned int) src_pages);
}

static __init int aiocpy_init(void)
{
	int err = 0;

	err = pages_list_init();
	if (err)
		goto out;

	maj_num = register_chrdev(0, DEVICE_NAME, &aiocpy_fops);
	if (maj_num < 0) {
		pr_err("failed to register a major number\n");
		err = maj_num;
		goto err_dev;
	}

	return 0;

err_dev:
	pages_list_destroy();
out:
	return err;
}

static __exit void aiocpy_exit(void)
{
	unregister_chrdev(maj_num, DEVICE_NAME);
	pages_list_destroy();
}

module_init(aiocpy_init);
module_exit(aiocpy_exit);

MODULE_AUTHOR("Vadim Kochan <vadim.kochan@petcube.com>");
MODULE_DESCRIPTION("Generic async memory copy driver");
MODULE_LICENSE("GPL");
