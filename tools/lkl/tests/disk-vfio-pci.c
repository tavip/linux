// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <lkl.h>
#include <lkl_host.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include "test.h"
#include "cla.h"

static struct {
	int printk;
	const char *fstype;
	const char *pciname;
} cla;

struct cl_arg args[] = {
	{ "pciname", 'n', "PCI device name (as %x:%x:%x.%x format)", 1,
	  CL_ARG_STR, &cla.pciname },
	{ 0 },
};

static char bootparams[128];

#define min(a, b) (a < b ? a : b)

static int lkl_test_blkdev(void)
{
	char dev_str[] = { "/dev/xxxxxxxx" };
	char buffer[64*1024];
	uint64_t size, read = 0;
	int err;
	int fd;

	snprintf(dev_str, sizeof(dev_str), "/dev/%08x", LKL_MKDEV(259, 0));

	err = lkl_sys_mknod(dev_str, LKL_S_IFBLK | 0600, LKL_MKDEV(259, 0));
	if (err < 0) {
		lkl_test_logf("mknod failed: %s\n", lkl_strerror(err));
		return TEST_FAILURE;
	}

	fd = lkl_sys_open(dev_str, LKL_O_RDONLY, 0);
	if (fd < 0) {
		lkl_test_logf("open failed: %s\n", lkl_strerror(fd));
		return TEST_FAILURE;
	}

	err = lkl_sys_ioctl(fd, LKL_BLKGETSIZE64, (unsigned long)&size);
	if (err < 0) {
		lkl_test_logf("BLKGETSIZE64 failed: %s\n", lkl_strerror(fd));
		lkl_sys_close(fd);
		return TEST_FAILURE;
	}

	while (read < size) {
		err = lkl_sys_read(fd, buffer,
				   min(sizeof(buffer), size - read));
		if (err <= 0) {
			lkl_test_logf("read failed: %s\n", lkl_strerror(err));
			lkl_sys_close(fd);
			return TEST_FAILURE;
		}
		read += err;
	}

	lkl_sys_close(fd);
	lkl_test_logf("read %" PRIu64 " bytes\n", read);

	return TEST_SUCCESS;
}

LKL_TEST_CALL(start_kernel, lkl_start_kernel, 0, bootparams);
LKL_TEST_CALL(stop_kernel, lkl_sys_halt, 0);

struct lkl_test tests[] = {
	LKL_TEST(start_kernel),
	LKL_TEST(blkdev),
	LKL_TEST(stop_kernel),
};

int main(int argc, const char **argv)
{
	int ret;

	if (parse_args(argc, argv, args) < 0)
		return -1;

	snprintf(bootparams, sizeof(bootparams),
		 "mem=16M loglevel=8 lkl_pci=vfio%s", cla.pciname);

	lkl_host_ops.print = lkl_test_log;

	if (lkl_init(&lkl_host_ops) < 0) {
		printf("%s\n", lkl_test_get_log());
		return 1;
	}

	ret = lkl_test_run(tests, sizeof(tests) / sizeof(struct lkl_test),
			"disk-vfio-pci");

	lkl_cleanup();

	return ret;
}
