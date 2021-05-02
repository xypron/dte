// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright, Heinrich Schuchardt <xypron.glpk@gmx.de>
 *
 * This program demonstrates how an area for metadata can be added to an
 * existing flattened device tree.
 */
#include <arpa/inet.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

struct fdt_header_new {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
	uint32_t off_meta_data;
	uint32_t size_meta_data;
};

#define DELTA 1032

void usage(const char *prog)
{
	printf("Usage: %s <file_in> <file_out>\n", prog);
	exit(EXIT_FAILURE);
}

int main(int argc, const char *argv[])
{
	int fd;
	uint8_t *buf;
	uint8_t *buf_out;
	int ret;
	off_t size;
	ssize_t r;
	struct stat statbuf;
	struct fdt_header_new *header;

	if (argc != 3)
		usage(argv[0]);
	
	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	ret = fstat(fd, &statbuf);
	if (fd == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	buf = mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (!buf) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	ret = close(fd);
	if (fd == -1) {
		perror("close");
		exit(EXIT_FAILURE);
	}

	size = statbuf.st_size + DELTA;
	buf_out = malloc(size);
	if (!buf_out) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	memset(buf_out, 0, size);
	memcpy(buf_out, buf, sizeof(struct fdt_header));

	header = (struct fdt_header_new *)buf_out;
	if (ntohl(header->version) != 17)
	{
		fprintf(stderr, "Only supporting FDT version 17\n");
		exit(EXIT_FAILURE);
	}

	memcpy(buf_out + DELTA + ntohl(header->off_mem_rsvmap),
	       buf + ntohl(header->off_mem_rsvmap),
	       ntohl(header->totalsize) - ntohl(header->off_mem_rsvmap));
	header->totalsize = htonl(ntohl(header->totalsize) + DELTA);
	header->off_dt_struct = htonl(ntohl(header->off_dt_struct) + DELTA);
	header->off_dt_strings = htonl(ntohl(header->off_dt_strings) + DELTA);
	header->off_mem_rsvmap = htonl(ntohl(header->off_mem_rsvmap) + DELTA);
	header->version = htonl(18);
	header->off_meta_data = htonl(sizeof(struct fdt_header_new));
	header->size_meta_data = htonl(DELTA + sizeof(struct fdt_header) -
				       sizeof(struct fdt_header_new));

	ret = munmap(buf, statbuf.st_size);
	if (fd == -1) {
		perror("munmap");
		exit(EXIT_FAILURE);
	}

	fd = open(argv[2], O_WRONLY | O_CREAT);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	ret = fchmod(fd, 0644);
	if (ret == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	for (; size;) {
		r = write(fd, buf_out, size);
		if (r == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		size -= r;
	}

	close(fd);
	free(buf_out);
	
	return EXIT_SUCCESS;
}
