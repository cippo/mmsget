#include "print.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/ioctl.h>

/* Update the progress bar at most twice a second */
#define PROGRESS_BAR_UPDATE_SPEED 0.5

static int verbosity_level = 1;

/* Returns the column width of the terminal
 * Borrowed from archlinux's pacman code
 */
static int
column_width ()
{
	if (!isatty (1)) {
		return 80;
	} else {
#ifdef TIOCGSIZE
		struct ttysize win;
		if(ioctl(1, TIOCGSIZE, &win) == 0) {
			return win.ts_cols;
		}
#elif defined(TIOCGWINSZ)
		struct winsize win;
		if(ioctl(1, TIOCGWINSZ, &win) == 0) {
			return win.ws_col;
		}
#endif
		/* If we can't figure anything out, we'll just assume 80 columns */
		/* TODO any problems caused by this assumption? */
		return 80;
	}
}

void
print_set_verbosity_level (int level)
{
	verbosity_level = level;
}

void
print_info (int level, const char *fmt, ...)
{
	va_list ap;

	if (level > verbosity_level)
		return;

	va_start (ap, fmt);
	vprintf (fmt, ap);
	va_end (ap);
}

void
print_error (const char *fmt, ...)
{
	va_list ap;

	va_start (ap, fmt);
	fprintf (stderr, "ERROR: ");
	vfprintf (stderr, fmt, ap);
	va_end (ap);
}

/* Prints a byte count with the correct unit (B, KiB, MiB, GiB). */
static void
print_bytes (uint64_t bytes)
{
	if (bytes >= 2L * 1024 * 1024 * 1024) {
		printf ("%#6.1fGiB", ((double)bytes) / (1024.0 * 1024.0 * 1024.0));
	} else if (bytes >= 2L * 1024 * 1024) {
		printf ("%#6.1fMiB", ((double)bytes) / (1024.0 * 1024.0));
	} else if (bytes >= 2L * 1024) {
		printf ("%#6.1fKiB", ((double)bytes) / (1024.0));
	} else {
		printf ("%#6.1f  B", ((double)bytes));
	}
}

static double
timeval_diff (struct timeval *last_time, struct timeval *cur_time)
{
	return (double)(cur_time->tv_sec - last_time->tv_sec) +
		(double)(cur_time->tv_usec - last_time->tv_usec) / 1e6;
}

/* Calculates the average speed of a transfer.
 * Returns false if the last call to this was less than
 * PROGRESS_BAR_UPDATE_SPEED seconds ago.
 */
static bool
calculate_avg_speed (uint64_t cur_pos, uint64_t *speed)
{
	static struct timeval last_time;
	static uint64_t last_pos;
	static uint64_t last_speed;
	struct timeval cur_time;
	uint64_t cur_speed;
	double time_diff;

	gettimeofday (&cur_time, NULL);

	if (cur_pos == 0) {
		last_time  = cur_time;
		last_pos   = cur_pos;
		last_speed = 0;

		*speed = 0;
		return true;
	}

	time_diff = timeval_diff (&last_time, &cur_time);

	if (time_diff < PROGRESS_BAR_UPDATE_SPEED) {
		*speed = last_speed;
		return false;
	}

	cur_speed = (cur_pos - last_pos) / time_diff;

	/* Return a weighted average of the current and the last speed */
	*speed = (cur_speed * 2 + last_speed) / 3;

	last_speed = *speed;
	last_time = cur_time;
	last_pos = cur_pos;

	return true;
}

/* Prints a progress bar. */
void
print_progress (const char *title, uint64_t cur_pos, uint64_t total_size)
{
	int progress = cur_pos * 100 / total_size;
	int bar_size = column_width ();
	uint64_t speed;

	if (!calculate_avg_speed (cur_pos, &speed) && progress < 100)
		return;

	/* Print the title */
	printf ("\r%s  ", title);
	bar_size -= strlen (title) + 2;

	/* If there is room, make some space */
	while (bar_size > 100) {
		printf (" ");
		bar_size--;
	}

	/* Print number of bytes received and avg speed (23 characters) */
	print_bytes (cur_pos);
	printf (" ");
	print_bytes (speed);
	printf ("/s [");

	/* Print the progress bar */
	bar_size -= 23 + 7;
	for (int i = 0; i < bar_size; i++) {
		if ((i * 100 / bar_size) < progress) {
			printf ("#");
		} else {
			printf ("-");
		}
	}

	/* Print the progress in percent (7 characters) */
	printf ("] %3i %%", progress);
	fflush (stdout);
}
