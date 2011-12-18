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
