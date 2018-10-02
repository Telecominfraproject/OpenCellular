/* GPLv3+ to read sysmobts-v2 revD or later EEPROM from userspace */


#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>


/* Can read a 16bit at24 eeprom with 8192 byte in storage (24c64) */
static int dump_eeprom(int fd, int out)
{
#define STEP 8192
#define SIZE 8192
	uint8_t buf[STEP + 2];
	int rc = 0;
	int i;

	for (i = 0; i < SIZE; i += STEP) {
		/* write the address */
		buf[0] = i >> 8;
		buf[1] = i;
		rc = write(fd, buf, 2);
		if (rc != 2) {
			fprintf(stderr, "writing address failed: %d/%d/%s\n", rc, errno, strerror(errno));
			return 1;
		}

		/* execute step amount of reads */
		rc = read(fd, buf, STEP);
		if (rc != STEP) {
			fprintf(stderr, "Failed to read: %d/%d/%s\n", rc, errno, strerror(errno));
			return 1;
		}

		write(out, buf, STEP);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int i2c_fd, out_fd;
	char *filename = "/dev/i2c-1";
	char *out_file = "eeprom.out";
	int addr = 0x50;
	int rc;

	i2c_fd = open(filename, O_RDWR);
	if (i2c_fd < 0) {
		fprintf(stderr, "Failed to open i2c device %d/%d/%s\n",
			i2c_fd, errno, strerror(errno));
		return EXIT_FAILURE;
	}

	/* force using that address it is already bound with a driver */
	rc = ioctl(i2c_fd, I2C_SLAVE_FORCE, addr);
	if (rc < 0) {
		fprintf(stderr, "Failed to claim i2c device %d/%d/%s\n",
			rc, errno, strerror(errno));
		return EXIT_FAILURE;
	}

	if (argc >= 2)
		out_file = argv[1];
	out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd < 0) {
		fprintf(stderr, "Failed to open out device %s %d/%d/%s\n",
			out_file, rc, errno, strerror(errno));
		return EXIT_FAILURE;
	}

	if (dump_eeprom(i2c_fd, out_fd) != 0) {
		unlink(out_file);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
