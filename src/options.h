#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <stdbool.h>

typedef struct {
	const char *filename;
	const char *url;
	int thread_count;
	int bandwidth;
	int verbosity_level;
	bool progress_bar;
} options_t;

bool options_parse (int argc, char *argv[], options_t *options);

#endif /* _OPTIONS_H_ */
