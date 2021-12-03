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
#include <sys/ioctl.h>

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

#define SNIP		" (...) "
#define SNIP_LEN	(sizeof(SNIP) - 1)
#define TAB_WIDTH	8

#define mem_free(ptr)	do { \
		if (ptr) { \
			free(ptr); \
			(ptr) = NULL; \
		} \
	} while(0);

#define HEADTAIL	0
#define COMPCAT		1

static int functionality = HEADTAIL;
static int num_lines = 10;
static int num_cols = 0;
static int show_line_nos = 0;
static int show_header;
static int tab_width = TAB_WIDTH;

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

const char UTF8len[256] = {
	// CTRL codes
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	// ASCII
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	// UTF-8 (not all are legal)
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3, 4,4,4,4,4,4,4,4,5,5,5,5,6,6,6,6
};

typedef struct {
	int in_width;
	int out_width;
} char_width;

/*
 * Input is a pointer to a char, which might be multibyte.
 * Output is { input width, output width }.
 *
 * LF    -> { 1, 0 }
 * tab   -> { 1, 8 }
 * ascii -> { 1, 1 }
 * ö     -> { 2, 1 }
 * €     -> { 3, 1 }
 *
 * The backspace character technically has an output width of -1 but that would be dangerous.  I'll
 * probably handle that and some others with substitutions later.
 *
 * Some Unicode characters have an output width != 1 but I'm ignoring that until the day it hits me.
 */
static char_width width(char c)
{
	if (c == '\t')
		return (char_width){ 1, tab_width };
	else if (c < ' ' || c == 0x7f)
		return (char_width){ 1, 0 };
	else
		return (char_width){ UTF8len[(unsigned char)c], 1};
}

static size_t string_width(char *string)
{
	char *c = string;
	size_t ret = 0;
	int col = 0;

	while (*c) {
		char_width cw = width(*c);

		if (*c == '\t')
			cw.out_width -= col % tab_width;
		else if (c[0] == '\x1b' && c[1] == '[') { // ANSI SGI (color) code
			char *m = c + 2;

			while (*m && *m != 'm')
				m++;
			if (*m == 'm')
				cw.in_width += (m - c);
		}

		ret += cw.out_width;
		col += cw.out_width;

		while (cw.in_width--)
			if (!*++c)
				break;
	}
	return ret;
}

/*
 * Print line as "Lorem ipsum dolor sit (...) elit.", with knowledge of character width
 * even where they're contain tabs (one to many) or multi-byte characters (many to one)
 */
void print_trunc(char *line)
{
	int header_len = (num_cols - SNIP_LEN) * 3 / 4;
	int trailer_len = (num_cols - SNIP_LEN) / 4;
	int col = 0;
	char *c = line;

	// Take care of rounding
	if (header_len + SNIP_LEN + trailer_len < num_cols)
		trailer_len++;
	if (header_len + SNIP_LEN + trailer_len < num_cols)
		header_len++;

	while (*c && col <= header_len) {
		char_width cw = width(*c);

		if (*c == '\t')
			cw.out_width -= (col % tab_width);
		else if (c[0] == 0x1b && c[1] == '[') { // ANSI SGI (color) code
			char *m = c + 2;

			while (*m && *m != 'm')
				m++;
			if (*m == 'm')
				cw.in_width += (m - c);
		}

		if (cw.out_width && col + cw.out_width <= header_len) { // We print it
			while (cw.in_width--)
				putchar(*c++);
		} else // We eat it
			c += cw.in_width;

		col += cw.out_width;
	}
	if (col > header_len) {
		fputs(SNIP, stdout);
		col += SNIP_LEN;
	}

	// Eat everything but the trailer. First a quick skip...
	size_t len = strlen(c);
	if (len > trailer_len * tab_width)
		c += len - trailer_len * tab_width;

	// Eat everything but the trailer.
	while (string_width(c) > trailer_len) {
		char_width cw = width(*c);

		if (*c == '\t')
			cw.out_width -= (col % tab_width);
		else if (c[0] == 0x1b && c[1] == '[') { // ANSI SGI (color) code
			char *m = c + 2;

			while (*m && *m != 'm')
				m++;
			if (*m == 'm')
				cw.in_width += (m - c);
		}

		c += cw.in_width;
	}

	// Print the trailer
	while (*c) {
		char_width cw = width(*c);

		if (*c == '\t')
			cw.out_width -= (col % tab_width);
		else if (c[0] == 0x1b && c[1] == '[') { // ANSI SGI (color) code
			char *m = c + 2;

			while (*m && *m != 'm')
				m++;
			if (*m == 'm')
				cw.in_width += (m - c);
		}

		if (cw.out_width && col + cw.out_width <= header_len) { // We print it
			while (cw.in_width--)
				putchar(*c++);
		} else // We eat it
			c += cw.in_width;

		col += cw.out_width;
	}

	// Reset colors - we might have snipped a reset.
	// Also add a LF since we snipped any in there.
	fputs("\x1b[m\n", stdout);
}

static int head_tail(char *name)
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

		if (line++ < num_lines || !num_lines) {
			if (show_line_nos)
				printf("%6d: ", line);
			print_trunc(buf[mod]);
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

	// Instead of outputting "1 line skipped", we might just as well output that line
	if (num_lines && line == 2 * num_lines + 1)
		num_tail += 1;

	if (num_lines && line > 2 * num_lines + 1) {
		if (tty_out) {
			printf("(... %d lines skipped ...)\n", line - num_lines - num_tail);
		} else
			printf(" %d lines skipped ...)\n", line - num_lines - num_tail);
	}

	if (num_lines && line > num_lines) {
		for (i = 0; i < num_tail; i++) {
			int n = line - num_tail + i;
			int nmod = n % (num_lines + 1);

			if (show_line_nos)
				printf("%6d: ", n + 1);
			print_trunc(buf[nmod]);
			fflush(stdout);
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
	printf("(c) magnum 2021\nUsage: %s [OPTION]... [FILE]...\n", name);
	puts("\nOptions:");
	if (functionality == HEADTAIL) {
		puts("  -n <lines> max. number of head and tail lines (default is half terminal height)");
		puts("  -w         compress horizontally as well: Snip long lines with \"(...)\"");
		puts("  -c <cols>  specify width for -w (default is terminal width)");
	} else
		puts("  -c <cols>  specify width (default is terminal width)");
	puts("  -l         show line numbers");
	printf("  -t <width> set tab width (default %d)\n", TAB_WIDTH);
	puts("  -q         never output filename headers");
	puts("  -h         this help");
	if (functionality == HEADTAIL) {
		puts("\nWorks similar to 'head' and 'tail' on each file but does both at once, even for");
		puts("stdin (which is impossible with head/tail).");
		puts("If file fits terminal height (using defaults), just output the whole file.  If");
		puts("it is longer, output the head followed by \"(... n lines skipped ...)\" on a");
		puts("separate line, then the tail.  Lines can optionally be horizontally compressed");
		puts("similarly, using -w and/or -c.  If no file is given, standard input is used.");
		puts("Unlike 'head'  and 'tail', this tool adds a final LF in case the last line was");
		puts("lacking it.");
	} else {
		puts("\nHorizontally compress output, as in \"Lorem ipsum dolor sit (...) elit.\".  If no");
		puts("file is given, standard input is used.  Unlike 'cat', this tool adds a final LF");
		puts("in case the last line was lacking it.");
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int c;
	int exit_ret = EXIT_SUCCESS;
	int quiet = 0, term_cols = 80, snip_width = 0;
	struct winsize w;
	char *env;

	if (ioctl(fileno(stdout), TIOCGWINSZ, &w) >= 0 || ioctl(fileno(stderr), TIOCGWINSZ, &w) >= 0 ||
	    ioctl(fileno(stdin), TIOCGWINSZ, &w) >= 0) {
		num_lines = MAX(10, (w.ws_row - 1) / 2 - 1);
		term_cols = MAX(20, w.ws_col);
	}

	char *optstring = "n:wc:t:qlh";

	// Morph!
	if (!strncmp(argv[0], "compcat", 7)) {
	    functionality = COMPCAT;
		optstring = "c:t:qlh";
		num_lines = 0;
		snip_width = 1;
		env = getenv("COMPCAT");
	} else
		env = getenv("HEADTAIL");

	if (env) {
		char **new_argv = calloc(argc + 2, sizeof(char*));
		int i;

		new_argv[0] = argv[0];
		new_argv[1] = env;

		for (i = 1; i < argc; i++)
			new_argv[i + 1] = argv[i];

		argv = new_argv;
		argc++;
	}

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'n':
			if (!sscanf(optarg, "%i", &num_lines)) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			// We allow "-n 0" for disabling the headtail functionality and just go for -w
			if (num_lines == 0)
				snip_width = 1;
			else if (num_lines && num_lines < 10) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'w':
			snip_width = 1;
			break;
		case 'c':
			if (!sscanf(optarg, "%i", &term_cols) || term_cols < 20) {
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			snip_width = 1;
			break;
		case 't':
			if (!sscanf(optarg, "%i", &tab_width) || tab_width < 2) {
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
		case '?':
			exit(usage(argv[0]));
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (show_line_nos)
		term_cols -= 10;

	if (snip_width)
		num_cols = term_cols;

	show_header = (!quiet && argc > 1);

	if (!argc)
		return head_tail("-");

	while (*argv) {
		int ret;

		ret = head_tail(*argv++);
		if (ret != EXIT_SUCCESS)
			exit_ret = ret;
	}

	return exit_ret;
}
