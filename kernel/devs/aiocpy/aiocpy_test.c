#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "aiocpy.h"

#define BUF_SIZE	(4096 * 4)

int main(int argc, char **argv)
{
	struct aiocpy_req aio_req;
	struct aiocpy_iov aio_vec;
	uint8_t src[BUF_SIZE];
	uint8_t dst[BUF_SIZE];
	int fd;
	int i;

	fd = open("/dev/aiocpy", O_RDWR);
	if (fd < 0) {
		printf("Cannot open device file...\n");
		return -1;
	}

	/* fill up the src buffer with 0,1,2...BUF_SIZE-1 */
	for (i = 0; i < BUF_SIZE; i++)
		src[i] = i;

	memset(dst, 0, BUF_SIZE);

	aio_vec.len = BUF_SIZE - 20;
	aio_vec.dst = dst + 20;
	aio_vec.src = src;

	aio_req.iovs = &aio_vec;
	aio_req.count = 1;

	ioctl(fd, AIOCPY_CMD_SEND, (struct aiocpy_req *) &aio_req);

	if (memcmp(src, dst + 20, BUF_SIZE - 20) != 0)
		printf("[FAIL] dst does not match src\n");
	else
		printf("[OK] test passed\n");

	close(fd);
}
