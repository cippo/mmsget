#ifndef _PRINT_H_
#define _PRINT_H_

#include <stdint.h>

void print_set_verbosity_level (int level);
void print_info (int level, const char *fmt, ...);
void print_error (const char *fmt, ...);
void print_progress (const char *title, uint64_t cur_pos, uint64_t total_size);

#endif /* _PRINT_H_ */
