/*
 * Copyright (C) 2017 Nico Huber <nico.h@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define HW_FILE_READ	0x01
#define HW_FILE_WRITE	0x02

static size_t stat_length(size_t *const len, const size_t off, const int fd)
{
	struct stat statbuf;

	if (*len == 0 && !fstat(fd, &statbuf)) {
		if (statbuf.st_size == 0)
			errno = EINVAL;
		else if (statbuf.st_size - (off_t)off <= SIZE_MAX)
			*len = (size_t)(statbuf.st_size - (off_t)off);
		else
			*len = SIZE_MAX;
	}

	return *len;
}

static int map_file(uint64_t *const addr,
		    const char *const path,
		    size_t len,
		    const size_t off,
		    const uint32_t mode)
{
	const int prot = (mode & HW_FILE_READ ? PROT_READ : 0) |
			 (mode & HW_FILE_WRITE ? PROT_WRITE : 0);

	if (mode & ~(HW_FILE_READ | HW_FILE_WRITE) ||
	    !(mode & (HW_FILE_READ | HW_FILE_WRITE)))
		return EINVAL;

	const int fd = open(path, mode & HW_FILE_WRITE ? O_RDWR : O_RDONLY);
	if (fd < 0)
		return errno;

	if (!stat_length(&len, off, fd)) {
		close(fd);
		return errno;
	}

	void *const mapped = mmap(NULL, len, prot, MAP_SHARED, fd, (off_t)off);
	close(fd);
	if (mapped == MAP_FAILED)
		return errno;

	*addr = (uint64_t)(uintptr_t)mapped;
	return 0;
}

static int map_fill_from_file(uint64_t *const addr,
			      const char *const path,
			      size_t len,
			      const size_t off,
			      const uint32_t mode)
{
	size_t xferred;
	ssize_t read_ret;

	if (mode != HW_FILE_READ)
		return EINVAL;

	const int fd = open(path, O_RDONLY);
	if (fd < 0)
		return errno;

	if (!stat_length(&len, off, fd))
		goto _close;

	void *const mapped = mmap(NULL, len, PROT_READ | PROT_WRITE,
				  MAP_ANONYMOUS | MAP_PRIVATE, -1, (off_t)off);
	if (mapped == MAP_FAILED)
		goto _close;

	xferred = 0;
	do {
		read_ret = read(fd, (uint8_t *)mapped + xferred, len - xferred);
		if (read_ret > 0)
			xferred += read_ret;
	} while ((read_ret > 0 || (read_ret == -1 && errno == EINTR))
		 && xferred < len);

	if (xferred < len)
		goto _munmap;

	if (mprotect(mapped, len, PROT_READ))
		goto _munmap;

	close(fd);

	*addr = (uint64_t)(uintptr_t)mapped;
	return 0;

_munmap:
	munmap(mapped, len);
_close:
	close(fd);
	return errno;
}

int hw_file_map(uint64_t *const addr,
		const char *const path,
		const uint32_t len,
		const uint32_t off,
		const uint32_t mode,
		const int copy)
{
	if (copy)
		return map_fill_from_file(
				addr, path, (size_t)len, (size_t)off, mode);
	else
		return map_file(addr, path, (size_t)len, (size_t)off, mode);
}
