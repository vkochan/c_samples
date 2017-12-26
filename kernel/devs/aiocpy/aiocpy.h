#ifndef __AIOCPY_H
#define __AIOCPY_H

#include <linux/uio.h>

struct aiocpy_iov {
	void		*dst;
	void		*src;
	size_t		len;
};

struct aiocpy_req {
	uint32_t		count;
	struct aiocpy_iov	*iovs;
};

#define AIOCPY_IOCTL_BASE		1

#define AIOCPY_IOCTL_CMD_SEND		1

#define AIOCPY_CMD_SEND _IOWR(AIOCPY_IOCTL_BASE, AIOCPY_IOCTL_CMD_SEND, struct aiocpy_req*)

#endif /* __AIOCPY_H */
