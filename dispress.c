#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

/* dispress - a dissociated press program.
 * Written by Graeme Cole, and placed in the public domain.
 */

struct wodge {
	char **words;
	int num_words;
	char *data;
};

typedef struct wodge WODGE;

void *xmalloc(size_t s) {
	void *p = malloc(s);
	if (p == NULL) {
		fprintf(stderr, "Failed to malloc %d bytes.\n", s);
		exit(1);
	}
	return p;
}

void *xrealloc(void *p, size_t s) {
	void *q = realloc(p, s);
	if (q == NULL) {
		fprintf(stderr, "Failed to realloc to %d bytes.\n", s);
		exit(1);
	}
	return q;
}

void free_wodge(struct wodge *wodge) {
	if (wodge == NULL)
		return;
	free(wodge->words);
	free(wodge->data);
	free(wodge);
}

WODGE *get_words(FILE *f) {
	int data_p = 0;
	int data_sz = 64;
	int words_sz;
	int words_p;
	int c, i;

	WODGE *wodge = (WODGE *) xmalloc (sizeof(WODGE));

	wodge->data = (char *) xmalloc (data_sz);

	/* Slurp all the data into `data', changing all whitespace for nulls */
	while ((c = fgetc(f)) != EOF) {
		wodge->data[data_p++] = (char) (isspace(c) ? '\0' : c);
		if (data_p >= data_sz) {
			wodge->data = (char *) xrealloc(wodge->data,data_sz*=2);
		}
	}
	wodge->data[data_p] = '\0';

	words_sz = 64;
	words_p = 0;
	wodge->words = (char **) xmalloc(sizeof(char *) * words_sz);
	i = 0;
	while (i < data_p) {
		wodge->words[words_p++] = &wodge->data[i];
		if (words_p >= words_sz) {
			wodge->words = (char **) xrealloc(wodge->words,
					sizeof(char *) * (words_sz *= 2));
		}
		
		// navigate to end of word
		while (wodge->data[++i]);

		// navigate to start of the next one (or the end)
		while (i < data_p && wodge->data[++i] == '\0');
	}
	wodge->words[words_p] = NULL;
	wodge->num_words = words_p;

	return wodge;
}

void print_word(char *word) {
	static int column = 0;
	int len = strlen(word);
	
	if (column + len == 79) {
		printf("%s\n", word);
		column = 0;
	}
	else if (column + len > 79) {
		printf("\n%s ", word);
		column = len + 1;
	}
	else {
		printf("%s ", word);
		column += len + 1;
	}
}

void disassociated_press(WODGE *wodge, int n, int length, int avoid_normality,
		int show_boundaries) {
	char **word_buffer;
	int *matches;
	int matches_sz, matches_p;
	int i, j;
	int index, oldindex;
	int buf_ptr;

	word_buffer = (char **) xmalloc(sizeof(char *) * n);

	matches_sz = 8;
	matches = (int *) xmalloc (sizeof(int) * 8);

	index = rand() % (wodge->num_words - (n - 1));

	// fill up the buffer first
	for (i = 0; i < n; ++i) {
		int len;
		word_buffer[i] = wodge->words[index + i];
		print_word(word_buffer[i]);
	}
	buf_ptr = 0;
	
	for (; length > 0; --length) {
		// compute next index
		matches_p = 0;
		for (i = 0; i < wodge->num_words - (n - 1); ++i) {
			for (j = i; j < i + n; ++j) {
				if (strcasecmp(word_buffer[(buf_ptr + j - i) % n], wodge->words[j]))
					break;
			}
			if (j == i + n) {
				// match
				matches[matches_p++] = i;
				if (matches_p >= matches_sz) {
					matches = (int *) xrealloc(matches, sizeof(int) * (matches_sz *= 2));
				}
			}
		}

		if (matches_p == 0) {
			// Very odd. Perhaps we took the last and first word
			// into the buffer, so there's no match. Clear the
			// buffer and start somewhere else.
			index = rand() % (wodge->num_words - (n - 1));
			for (i = 0; i < n; ++i) {
				word_buffer[i] = wodge->words[index + i];
				print_word(word_buffer[i]);
			}
			buf_ptr = 0;

			continue;
		}
		
		oldindex = index;
		if (avoid_normality) {
			do {
				index = (matches[rand() % matches_p] + n) % wodge->num_words;
				// jump to a different place if we can
			} while (matches_p > 1 && index == oldindex + 1);
		}
		else {
			index = (matches[rand() % matches_p] + n) % wodge->num_words;
		}
		if (show_boundaries && index != oldindex + 1) {
			print_word("//");
		}
		
		// words[index] is the next word to be printed
		print_word(wodge->words[index]);

		// overwrite the oldest word in the buffer with this one, and
		// advance the pointer
		word_buffer[buf_ptr] = wodge->words[index];
		buf_ptr = (buf_ptr + 1) % n;
	}

	printf("\n");
	free(matches);
	free(word_buffer);
}

int main (int argc, char **argv) {
	int i;
	int n = 2;
	int length = 100;
	int avoid_normality = 0;
	int show_boundaries = 0;
	int verbose = 0;
	char *filename = NULL;
	WODGE *wodge;
	
	srand(time(NULL) + getpid());

	for (i = 1; i < argc; ++i) {
		if (!strcmp("-n", argv[i])) {
			if (++i >= argc || sscanf(argv[i], "%d", &n) < 1 ||
					n <= 0) {
				fprintf(stderr, "-n requires a positive numeric argument.\n");
				exit(1);
			}
		}
		else if (!strcmp("-l", argv[i])) {
			if (++i >= argc || sscanf(argv[i], "%d", &length) < 1 ||
					length <= 0) {
				fprintf(stderr, "-l requires a positive numeric argument.\n");
				exit(1);
			}
		}
		else if (!strcmp("-a", argv[i])) {
			avoid_normality = 1;
		}
		else if (!strcmp("-b", argv[i])) {
			show_boundaries = 1;
		}
		else if (!strcmp("-v", argv[i])) {
			++verbose;
		}
		else if (!strcmp("-h", argv[i]) || !strcmp("--help", argv[i])) {
			printf("Usage:\n\t"
"dispress [options] [filename]\n"
"If no filename is given, data is taken from standard input.\n"
"Here is a (slightly altered) quote from the Jargon File\n"
"(http://catb.org/~esr/jargon/html/D/Dissociated-Press.html) explaining it:\n"
"  Dissociated Press starts by printing any N consecutive words in the text.\n"
"  Then at every step it searches for any random occurrence in the original text\n"
"  of the last N words already printed and then prints the next word or letter.\n"
"-n N	specify value of N\n"
"-b	show places where it jumps to another part of the document\n"
"-a	avoid staying in the same place if we can possibly help it\n"
"-l 50	length: give (approximately) 50 words\n"
"-h	(or --help) print this help text\n");
			exit(0);
		}
		// Treat as a filename, not an option
		else if (filename != NULL) {
			fprintf(stderr, "Invalid argument %s\n", argv[i]);
			exit(1);
		}
		else {
			filename = argv[i];
		}
	}

	if (filename == NULL || !strcmp(filename, "-")) {
		wodge = get_words(stdin);
	}
	else {
		FILE *f = fopen(filename, "r");
		if (f == NULL) {
			perror("fopen");
			exit(1);
		}
		wodge = get_words(f);
		fclose(f);
	}

	if (wodge->num_words < 1) {
		/* "If a program has nothing interesting to say, it should say
		   nothing." */
		exit(1);
	}
	
	if (verbose)
		fprintf(stderr, "n=%d, length=%d, avoid=%d\n%d words in text\n", n, length, avoid_normality, wodge->num_words);

	disassociated_press(wodge, n, length, avoid_normality, show_boundaries);

	free_wodge(wodge);

	return 0;
}
