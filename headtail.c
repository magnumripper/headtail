/*
 * headtail(1) utility
 *
 * Works similar to 'head' and 'tail' on each file but does both at once, even for
 * stdin (which is impossible with head/tail).
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
#include <sys/time.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))

#define mem_free(ptr)	do { \
		if (ptr) { \
			free(ptr); \
			(ptr) = NULL; \
		} \
	} while(0);

static void* mem_calloc(size_t nmemb, size_t size)
{
	void *res;

	if (!nmemb || !size)
		return NULL;

	if (!(res = calloc(nmemb, size))) {
		fprintf(stderr, "mem_calloc(): %s trying to allocate %zu bytes\n", strerror(ENOMEM), nmemb * size);
		exit(EXIT_FAILURE);
	}

	return res;
}

static void print_error(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	fprintf(stderr, ": %s\n", strerror(errno));
}

static int process_file(char *name, int num_lines, int show_line_nos, int show_header)
{
	FILE *fh;
	char **buf = mem_calloc(sizeof(char*), num_lines + 1);
	size_t *size = mem_calloc(sizeof(size_t), num_lines + 1);
	int i, line = 0, mod = 0;
	int tty_out = isatty(fileno(stdout));

	if (!strcmp(name, "-")) {
		fh = stdin;
	} else {
		if (!(fh = fopen(name, "r"))) {
			print_error("headtail: %s", name);
			mem_free(buf);
			mem_free(size);
			return EXIT_FAILURE;
		}
	}

	if (show_header)
		printf("==> %s <==\n", name);

	while (!feof(fh)) {
		ssize_t len = getline(&buf[mod], &size[mod], fh);

		if (len < 0) {
			if (ferror(fh)) {
				print_error("headtail: %s", name);
				fclose(fh);
				mem_free(buf);
				mem_free(size);
				return EXIT_FAILURE;
			}
			break;
		}

		if (line++ < num_lines) {
			if (show_line_nos)
				printf("%6d: ", line);
			printf("%s%s", buf[mod], (buf[mod][len - 1] == '\n') ? "" : "\n");
			fflush(stdout);
		}
		else if (tty_out && line >= 2 * num_lines + 2) {
			struct timeval tp;
			static int last;

			gettimeofday(&tp, NULL);
			int current = tp.tv_sec;

			if (current != last) {
				printf("(... %d lines skipped ...)\r", line - num_lines);
				fflush(stdout);
				last = current;
			}
		}
		else if (line == 2 * num_lines + 2) {
			printf("(...");
			fflush(stdout);
		}

		if (++mod >= num_lines + 1)
			mod = 0;
	}

	int num_tail = MIN(num_lines, line - num_lines);

	/* Instead of outputting "1 line skipped", we might just as well output that line */
	if (line == 2 * num_lines + 1)
		num_tail += 1;

	if (line > 2 * num_lines + 1) {
		if (tty_out) {
			printf("(... %d lines skipped ...)\n", line - num_lines - num_tail);
		} else
			printf(" %d lines skipped ...)\n", line - num_lines - num_tail);
	}

	if (line > num_lines) {
		for (i = 0; i < num_tail; i++) {
			int n = line - num_tail + i;
			int nmod = n % (num_lines + 1);

			if (show_line_nos)
				printf("%6d: ", n + 1);
			printf("%s%s", buf[nmod], (buf[nmod][strlen(buf[nmod]) - 1] == '\n') ? "" : "\n");
		}
	}

	fflush(stdout);
	fclose(fh);
	mem_free(buf);
	mem_free(size);
	return EXIT_SUCCESS;
}

int usage(char *name)
{
	printf("Usage: %s [OPTION] [FILE]...\n", name);
	puts("\nOptions:");
	puts("  -n <num>   max. number of head and tail lines (default 10)");
	puts("  -l         show line numbers");
	puts("  -q         never output filename headers");
	puts("  -h         this help");
	puts("\nHeadTail utility (c) magnum 2021");
	puts("\nWorks similar to 'head' and 'tail' on each file but does both at once, even for");
	puts("stdin (which is impossible with head/tail).");
	puts("If file is at most 21 lines (using defaults), just output the whole file.  If");
	puts("it is longer, output the head followed by \"(... n lines skipped ...)\" on a");
	puts("separate line, then the tail.");
	puts("\nWe'll never skip a single line because outputting \"1 line skipped\" would take");
	puts("the same screen estate, that's the reason for \"at most 21 lines\" above.");
	puts("\nIf no file name is given, standard input is used.");
	puts("\nUnlike 'head' and 'tail', this tools adds a final LF in case the last line");
	puts("was lacking it.");

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int c, show_line_nos = 0;
	int exit_ret = EXIT_SUCCESS;
	int quiet = 0;
	int num_lines = 10;

	while ((c = getopt(argc, argv, "n:qlh")) != -1) {
		switch (c) {
		case 'n':
			sscanf(optarg, "%i", &num_lines);
			if (num_lines <= 0) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'q':
			quiet = 1;
			break;
		case 'l':
			show_line_nos = 1;
			break;
		case 'h':
			exit(usage(argv[0]));
			break;
		}
	}
	argc -= optind;
	argv += optind;

	int show_header = (!quiet && argc > 1);

	if (!argc)
		return process_file("-", num_lines, show_line_nos, show_header);

	while (*argv) {
		int ret;

		ret = process_file(*argv++, num_lines, show_line_nos, show_header);
		if (ret != EXIT_SUCCESS)
			exit_ret = ret;
	}

	return exit_ret;
}
