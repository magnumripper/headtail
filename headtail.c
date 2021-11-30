/*
 * headtail(1) utility
 *
 * If file is at most 21 lines, just print the whole file.  If it is longer,
 * print first 10 lines, followed by "(... n lines snipped ...)" on a separate
 * line, then the last 10 lines.
 *
 * This code is Copyright (c) 2021 magnum, and it is hereby released to the
 * general public under the following terms:
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))

static void print_error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, ": %s\n", strerror(errno));
}

static int process_file(char *name, int show_line_nos, int show_header)
{
	FILE *fh;
	char *buf[11] = { NULL };
	size_t size[11] = { 0 };
	int i, line = 0, mod = 0;

	if (!strcmp(name, "-")) {
		fh = stdin;
	} else {
		if (!(fh = fopen(name, "r"))) {
			print_error("headtail: %s", name);
			return EXIT_FAILURE;
		}
	}

	if (show_header)
		printf("==> %s <==\n", name);

	while (!feof(fh)) {
		ssize_t len;

		len = getline(&buf[mod], &size[mod], fh);

		if (len < 0) {
			if (ferror(fh)) {
				print_error("headtail: %s", name);
				return EXIT_FAILURE;
			}
			break;
		}
		if (line++ < 10) {
			if (show_line_nos)
				printf("%6d: ", line);
			printf("%s%s", buf[mod], (buf[mod][len - 1] == '\n') ? "" : "\n");
			fflush(stdout);
		}

		if (line == 22) {
			printf("(...");
			fflush(stdout);
		}

		if (++mod >= 11)
			mod = 0;
	}

	int num_tail = MIN(10, line - 10);

	if (line == 21)
		num_tail = 11;
	else if (line > 21)
		printf(" %d lines snipped ...)\n", line - num_tail - 10);

	if (line > 10) {
		for (i = 0; i < num_tail; i++) {
			int n = line - num_tail + i;
			int nmod = n % 11;

			if (show_line_nos)
				printf("%6d: ", n + 1);
			printf("%s%s", buf[nmod], (buf[nmod][strlen(buf[nmod]) - 1] == '\n') ? "" : "\n");
		}
	}

	return EXIT_SUCCESS;
}

int usage(char *name)
{
	printf("Usage: %s [OPTION] [FILE]...\n", name);
	puts("\nOptions:");
	puts("  -n  show line numbers");
	puts("  -h  this help");
	puts("\nHeadTail utility (c) magnum 2021");
	puts("\nWorks like 'head' and 'tail' on each file but does both at once, even for");
	puts("stdin (which is impossible with head/tail).");
	puts("If file is at most 21 lines, print whole file.  If it is longer, print first");
	puts("ten lines, followed by \"(... n lines snipped ...)\" on a separate line, then");
	puts("last ten lines.");
	puts("\nIf no file name is given, standard input is used.");

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int c, show_line_nos = 0;
	int exit_ret = EXIT_SUCCESS;

	while ((c = getopt(argc, argv, "nh")) != -1) {
		switch (c) {
		case 'n':
			show_line_nos = 1;
			break;
		case 'h':
			exit(usage(argv[0]));
			break;
		}
	}
	argc -= optind;
	argv += optind;

	int show_header = (argc > 1);

	if (!argc)
		return process_file("-", show_line_nos, show_header);

	while (*argv) {
		int ret;

		ret = process_file(*argv++, show_line_nos, show_header);
		if (ret != EXIT_SUCCESS)
			exit_ret = ret;
	}

	return exit_ret;
}
