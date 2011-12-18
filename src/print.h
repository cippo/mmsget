/*
 * Copyright (C) 2011 Sivert Berg <sivertb@stud.ntnu.no>
 *
 * This file is part of mmsget - a threaded mms-stream downloader
 *
 * mmsget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mmsget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mmsget.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PRINT_H_
#define _PRINT_H_

#include <stdint.h>

void print_set_verbosity_level (int level);
void print_info (int level, const char *fmt, ...);
void print_error (const char *fmt, ...);
void print_progress (const char *title, uint64_t cur_pos, uint64_t total_size);

#endif /* _PRINT_H_ */
