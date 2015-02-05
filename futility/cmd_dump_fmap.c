/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fmap.h"
#include "futility.h"

enum { FMT_NORMAL, FMT_PRETTY, FMT_FLASHROM, FMT_HUMAN };

/* global variables */
static int opt_extract;
static int opt_format = FMT_NORMAL;
static int opt_overlap;
static char *progname;
static void *base_of_rom;
static size_t size_of_rom;
static int opt_gaps;

/* Return 0 if successful */
static int dump_fmap(const FmapHeader *fmh, int argc, char *argv[])
{
	int i, retval = 0;
	char buf[80];		/* DWR: magic number */
	const FmapAreaHeader *ah;
	ah = (const FmapAreaHeader *) (fmh + 1);
	char *extract_names[argc];
	char *outname = 0;

	if (opt_extract) {
		/* prepare the filenames to write areas to */
		memset(extract_names, 0, sizeof(extract_names));
		for (i = 0; i < argc; i++) {
			char *a = argv[i];
			char *f = strchr(a, ':');
			if (!f)
				continue;
			if (a == f || *(f+1) == '\0') {
				fprintf(stderr,
					"argument \"%s\" is bogus\n", a);
				retval = 1;
				continue;
			}
			*f++ = '\0';
			extract_names[i] = f;
		}
		if (retval)
			return retval;
	}

	if (FMT_NORMAL == opt_format) {
		snprintf(buf, FMAP_SIGNATURE_SIZE + 1, "%s",
			 fmh->fmap_signature);
		printf("fmap_signature   %s\n", buf);
		printf("fmap_version:    %d.%d\n",
		       fmh->fmap_ver_major, fmh->fmap_ver_minor);
		printf("fmap_base:       0x%" PRIx64 "\n", fmh->fmap_base);
		printf("fmap_size:       0x%08x (%d)\n", fmh->fmap_size,
		       fmh->fmap_size);
		snprintf(buf, FMAP_NAMELEN + 1, "%s", fmh->fmap_name);
		printf("fmap_name:       %s\n", buf);
		printf("fmap_nareas:     %d\n", fmh->fmap_nareas);
	}

	for (i = 0; i < fmh->fmap_nareas; i++, ah++) {
		snprintf(buf, FMAP_NAMELEN + 1, "%s", ah->area_name);

		if (argc) {
			int j, found = 0;
			outname = NULL;
			for (j = 0; j < argc; j++)
				if (!strcmp(argv[j], buf)) {
					found = 1;
					outname = extract_names[j];
					break;
				}
			if (!found)
				continue;
		}

		switch (opt_format) {
		case FMT_PRETTY:
			printf("%s %d %d\n", buf, ah->area_offset,
			       ah->area_size);
			break;
		case FMT_FLASHROM:
			if (ah->area_size)
				printf("0x%08x:0x%08x %s\n", ah->area_offset,
				       ah->area_offset + ah->area_size - 1,
				       buf);
			break;
		default:
			printf("area:            %d\n", i + 1);
			printf("area_offset:     0x%08x\n", ah->area_offset);
			printf("area_size:       0x%08x (%d)\n", ah->area_size,
			       ah->area_size);
			printf("area_name:       %s\n", buf);
		}

		if (opt_extract) {
			char *s;
			if (!outname) {
				for (s = buf; *s; s++)
					if (*s == ' ')
						*s = '_';
				outname = buf;
			}
			FILE *fp = fopen(outname, "wb");
			if (!fp) {
				fprintf(stderr, "%s: can't open %s: %s\n",
					progname, outname, strerror(errno));
				retval = 1;
			} else if (!ah->area_size) {
				fprintf(stderr,
					"%s: section %s has zero size\n",
					progname, buf);
			} else if (ah->area_offset + ah->area_size >
				   size_of_rom) {
				fprintf(stderr, "%s: section %s is larger"
					" than the image\n", progname, buf);
				retval = 1;
			} else if (1 != fwrite(base_of_rom + ah->area_offset,
					       ah->area_size, 1, fp)) {
				fprintf(stderr, "%s: can't write %s: %s\n",
					progname, buf, strerror(errno));
				retval = 1;
			} else {
				if (FMT_NORMAL == opt_format)
					printf("saved as \"%s\"\n", outname);
			}
			fclose(fp);
		}
	}

	return retval;
}

/****************************************************************************/
/* Stuff for human-readable form */

struct dup_s {
	char *name;
	struct dup_s *next;
};

struct node_s {
	char *name;
	uint32_t start;
	uint32_t size;
	uint32_t end;
	struct node_s *parent;
	int num_children;
	struct node_s **child;
	struct dup_s *alias;
};

static struct node_s *all_nodes;

static void sort_nodes(int num, struct node_s *ary[])
{
	int i, j;
	struct node_s *tmp;

	/* bubble-sort is quick enough with only a few entries */
	for (i = 0; i < num; i++) {
		for (j = i + 1; j < num; j++) {
			if (ary[j]->start > ary[i]->start) {
				tmp = ary[i];
				ary[i] = ary[j];
				ary[j] = tmp;
			}
		}
	}
}

static void line(int indent, char *name,
		 uint32_t start, uint32_t end, uint32_t size, char *append)
{
	int i;
	for (i = 0; i < indent; i++)
		printf("  ");
	printf("%-25s  %08x    %08x    %08x%s\n", name, start, end, size,
	       append ? append : "");
}

static int gapcount;
static void empty(int indent, uint32_t start, uint32_t end, char *name)
{
	char buf[80];
	if (opt_gaps) {
		sprintf(buf, "  // gap in %s", name);
		line(indent + 1, "", start, end, end - start, buf);
	}
	gapcount++;
}

static void show(struct node_s *p, int indent, int show_first)
{
	int i;
	struct dup_s *alias;
	if (show_first) {
		line(indent, p->name, p->start, p->end, p->size, 0);
		for (alias = p->alias; alias; alias = alias->next)
			line(indent, alias->name, p->start, p->end, p->size,
			     "  // DUPLICATE");
	}
	sort_nodes(p->num_children, p->child);
	for (i = 0; i < p->num_children; i++) {
		if (i == 0 && p->end != p->child[i]->end)
			empty(indent, p->child[i]->end, p->end, p->name);
		show(p->child[i], indent + show_first, 1);
		if (i < p->num_children - 1
		    && p->child[i]->start != p->child[i + 1]->end)
			empty(indent, p->child[i + 1]->end, p->child[i]->start,
			      p->name);
		if (i == p->num_children - 1 && p->child[i]->start != p->start)
			empty(indent, p->start, p->child[i]->start, p->name);
	}
}

static int overlaps(int i, int j)
{
	struct node_s *a = all_nodes + i;
	struct node_s *b = all_nodes + j;

	return ((a->start < b->start) && (b->start < a->end) &&
		(b->start < a->end) && (a->end < b->end));
}

static int encloses(int i, int j)
{
	struct node_s *a = all_nodes + i;
	struct node_s *b = all_nodes + j;

	return ((a->start <= b->start) && (a->end >= b->end));
}

static int duplicates(int i, int j)
{
	struct node_s *a = all_nodes + i;
	struct node_s *b = all_nodes + j;

	return ((a->start == b->start) && (a->end == b->end));
}

static void add_dupe(int i, int j, int numnodes)
{
	int k;
	struct dup_s *alias;

	alias = (struct dup_s *) malloc(sizeof(struct dup_s));
	alias->name = all_nodes[j].name;
	alias->next = all_nodes[i].alias;
	all_nodes[i].alias = alias;
	for (k = j; k < numnodes; k++)
		all_nodes[k] = all_nodes[k + 1];
}

static void add_child(struct node_s *p, int n)
{
	int i;
	if (p->num_children && !p->child) {
		p->child =
		    (struct node_s **)calloc(p->num_children,
					     sizeof(struct node_s *));
		if (!p->child) {
			perror("calloc failed");
			exit(1);
		}
	}
	for (i = 0; i < p->num_children; i++)
		if (!p->child[i]) {
			p->child[i] = all_nodes + n;
			return;
		}
}

static int human_fmap(const FmapHeader *fmh)
{
	FmapAreaHeader *ah;
	int i, j, errorcnt = 0;
	int numnodes;

	ah = (FmapAreaHeader *) (fmh + 1);

	/* The challenge here is to generate a directed graph from the
	 * arbitrarily-ordered FMAP entries, and then to prune it until it's as
	 * simple (and deep) as possible. Overlapping regions are not allowed.
	 * Duplicate regions are okay, but may require special handling. */

	/* Convert the FMAP info into our format. */
	numnodes = fmh->fmap_nareas;

	/* plus one for the all-enclosing "root" */
	all_nodes = (struct node_s *) calloc(numnodes + 1,
					     sizeof(struct node_s));
	if (!all_nodes) {
		perror("calloc failed");
		exit(1);
	}
	for (i = 0; i < numnodes; i++) {
		char buf[FMAP_NAMELEN + 1];
		strncpy(buf, ah[i].area_name, FMAP_NAMELEN);
		buf[FMAP_NAMELEN] = '\0';
		all_nodes[i].name = strdup(buf);
		if (!all_nodes[i].name) {
			perror("strdup failed");
			exit(1);
		}
		all_nodes[i].start = ah[i].area_offset;
		all_nodes[i].size = ah[i].area_size;
		all_nodes[i].end = ah[i].area_offset + ah[i].area_size;
	}
	/* Now add the root node */
	all_nodes[numnodes].name = strdup("-entire flash-");
	all_nodes[numnodes].start = fmh->fmap_base;
	all_nodes[numnodes].size = fmh->fmap_size;
	all_nodes[numnodes].end = fmh->fmap_base + fmh->fmap_size;

	/* First, coalesce any duplicates */
	for (i = 0; i < numnodes; i++) {
		for (j = i + 1; j < numnodes; j++) {
			if (duplicates(i, j)) {
				add_dupe(i, j, numnodes);
				numnodes--;
			}
		}
	}

	/* Each node should have at most one parent, which is the smallest
	 * enclosing node. Duplicate nodes "enclose" each other, but if there's
	 * already a relationship in one direction, we won't create another.
	 */
	for (i = 0; i < numnodes; i++) {
		/* Find the smallest parent, which might be the root node. */
		int k = numnodes;
		for (j = 0; j < numnodes; j++) { /* full O(N^2) comparison */
			if (i == j)
				continue;
			if (overlaps(i, j)) {
				printf("ERROR: %s and %s overlap\n",
				       all_nodes[i].name, all_nodes[j].name);
				printf("  %s: 0x%x - 0x%x\n", all_nodes[i].name,
				       all_nodes[i].start, all_nodes[i].end);
				printf("  %s: 0x%x - 0x%x\n", all_nodes[j].name,
				       all_nodes[j].start, all_nodes[j].end);
				if (opt_overlap < 2) {
					printf("Use more -h args to ignore"
					       " this error\n");
					errorcnt++;
				}
				continue;
			}
			if (encloses(j, i)
			    && all_nodes[j].size < all_nodes[k].size)
				k = j;
		}
		all_nodes[i].parent = all_nodes + k;
	}
	if (errorcnt)
		return 1;

	/* Force those deadbeat parents to recognize their children */
	for (i = 0; i < numnodes; i++)	/* how many */
		if (all_nodes[i].parent)
			all_nodes[i].parent->num_children++;
	for (i = 0; i < numnodes; i++)	/* here they are */
		if (all_nodes[i].parent)
			add_child(all_nodes[i].parent, i);

	/* Ready to go */
	printf("# name                     start       end         size\n");
	show(all_nodes + numnodes, 0, opt_gaps);

	if (gapcount && !opt_gaps)
		printf("\nWARNING: unused regions found. Use -H to see them\n");

	return 0;
}

/* End of human-reable stuff */
/****************************************************************************/

static const char usage[] =
	"\nUsage:  " MYNAME " %s [OPTIONS] FLASHIMAGE [NAME...]\n\n"
	"Display (and extract) the FMAP components from a BIOS image.\n"
	"\n"
	"Options:\n"
	"  -x             Extract the named sections from the file\n"
	"  -h             Use a human-readable format\n"
	"  -H             With -h, display any gaps\n"
	"  -p             Use a format easy to parse by scripts\n"
	"  -F             Use the format expected by flashrom\n"
	"\n"
	"Specify one or more NAMEs to dump only those sections.\n"
	"\n";

static void print_help(const char *name)
{
	printf(usage, name);
}

static int do_dump_fmap(int argc, char *argv[])
{
	int c;
	int errorcnt = 0;
	struct stat sb;
	int fd;
	const FmapHeader *fmap;
	int retval = 1;

	progname = argv[0];

	opterr = 0;		/* quiet, you */
	while ((c = getopt(argc, argv, ":xpFhH")) != -1) {
		switch (c) {
		case 'x':
			opt_extract = 1;
			break;
		case 'p':
			opt_format = FMT_PRETTY;
			break;
		case 'F':
			opt_format = FMT_FLASHROM;
			break;
		case 'H':
			opt_gaps = 1;
			/* fallthrough */
		case 'h':
			opt_format = FMT_HUMAN;
			opt_overlap++;
			break;
		case '?':
			fprintf(stderr, "%s: unrecognized switch: -%c\n",
				progname, optopt);
			errorcnt++;
			break;
		case ':':
			fprintf(stderr, "%s: missing argument to -%c\n",
				progname, optopt);
			errorcnt++;
			break;
		default:
			errorcnt++;
			break;
		}
	}

	if (errorcnt || optind >= argc) {
		print_help(progname);
		return 1;
	}

	if (0 != stat(argv[optind], &sb)) {
		fprintf(stderr, "%s: can't stat %s: %s\n",
			progname, argv[optind], strerror(errno));
		return 1;
	}

	fd = open(argv[optind], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: can't open %s: %s\n",
			progname, argv[optind], strerror(errno));
		return 1;
	}

	base_of_rom =
	    mmap(0, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (base_of_rom == (char *)-1) {
		fprintf(stderr, "%s: can't mmap %s: %s\n",
			progname, argv[optind], strerror(errno));
		close(fd);
		return 1;
	}
	close(fd);		/* done with this now */
	size_of_rom = sb.st_size;

	fmap = fmap_find(base_of_rom, size_of_rom);
	if (fmap) {
		switch (opt_format) {
		case FMT_HUMAN:
			retval = human_fmap(fmap);
			break;
		case FMT_NORMAL:
			printf("hit at 0x%08x\n",
			       (uint32_t) ((char *)fmap - (char *)base_of_rom));
			/* fallthrough */
		default:
			retval =
			    dump_fmap(fmap, argc - optind - 1,
				      argv + optind + 1);
		}
	}

	if (0 != munmap(base_of_rom, sb.st_size)) {
		fprintf(stderr, "%s: can't munmap %s: %s\n",
			progname, argv[optind], strerror(errno));
		return 1;
	}

	return retval;
}

DECLARE_FUTIL_COMMAND(dump_fmap, do_dump_fmap,
		      VBOOT_VERSION_ALL,
		      "Display FMAP contents from a firmware image",
		      print_help);
