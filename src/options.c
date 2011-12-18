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

#include "options.h"
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>

const char *short_options = "hVvbpf:t:B:";
static struct option long_options[] = {
	{"help",      no_argument,       0, 'h'},
	{"version",   no_argument,       0, 'V'},
	{"verbose",   no_argument,       0, 'v'},
	{"brief",     no_argument,       0, 'b'},
	{"progress",  no_argument,       0, 'p'},
	{"file",      required_argument, 0, 'f'},
	{"threads",   required_argument, 0, 't'},
	{"bandwidth", required_argument, 0, 'B'},
};

static void
print_usage (const char *prog)
{
	printf ("Usage: %s [OPTIONS] URL\n"
			"Downloads the stream given by the URL (must be mms:// or mmsh://)\n"
			"If no filename is specified, the filename in the URL will be used\n\n"
			"Options:\n"
			"  -h --help        show this help and exit\n"
			"  -V --version     show version and exit\n"
			"  -v --verbose     increase the verbosity level\n"
			"  -b --brief       descrease the verbosity level\n"
			"  -p --progress    show a progress bar\n"
			"  -f --file        the file to save to\n"
			"  -t --threads     the number of threads to use\n"
			"  -B --bandwidth   the bandwidth to use per thread (in KiB/s)\n",
			prog
		   );
}

static void
print_version ()
{
	printf ("mmsget - %s\n"
			"Copyright (C) 2011 Sivert Berg\n\n"
			"License GPLv3: GNU GPL version 3 <http://gnu.org/licenses/gpl.html>.\n"
			"This is free software: you are free to change and redistribute it.\n"
			"There is NO WARRANTY, to the extent permitted by law.\n",
			VERSION
		   );
}

static bool
str_to_int (const char *str, int *val)
{
	char *endptr;

	*val = strtol (str, &endptr, 10);

	return (*endptr == '\0');
}

static const char*
get_filename (const char *url)
{
	return strrchr (url, '/') + 1;
}

bool
options_parse (int argc, char *argv[], options_t *options)
{
	int c;

	/* Set default options */
	options->url = NULL;
	options->filename = NULL;
	options->thread_count = 10;
	options->bandwidth = INT_MAX;
	options->verbosity_level = 1;
	options->progress_bar = false;

	/* Parse commandline arguments */
	while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
		switch (c) {
		case 'V':
			print_version ();
			return false;

		case 'h':
			print_usage (argv[0]);
			return false;

		case 'v':
			options->verbosity_level = 2;
			break;

		case 'b':
			options->verbosity_level = 0;
			break;

		case 'p':
			options->progress_bar = true;
			break;

		case 'f':
			options->filename = strdup (optarg);
			break;

		case 't':
			if (!str_to_int (optarg, &options->thread_count))
				return false;
			break;

		case 'B':
			if (!str_to_int (optarg, &options->bandwidth))
				return false;
			break;

		default:
			break;
		}
	}

	if (optind >= argc) {
		fprintf (stderr, "%s: No URL given\n", argv[0]);
		print_usage (argv[0]);
		return false;
	}

	options->url = argv[optind];

	if (options->filename == NULL)
		options->filename = get_filename (options->url);

	return true;
}
