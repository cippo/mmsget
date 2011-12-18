#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <libmms/mmsx.h>
#include "fifo.h"
#include "print.h"
#include "options.h"

#define BUF_SIZE     (16 * 1024)

typedef struct {
	char data[BUF_SIZE];
	uint32_t len, off;
} buf_t;

fifo_t dirty_bufs = FIFO_INITIALIZER;
fifo_t clean_bufs = FIFO_INITIALIZER;

#define add_dirty_buf(b) fifo_push (&dirty_bufs, b)
#define add_clean_buf(b) fifo_push (&clean_bufs, b)
#define get_dirty_buf()  fifo_pop  (&dirty_bufs)
#define get_clean_buf()  fifo_pop  (&clean_bufs)

typedef struct {
	uint32_t len;
	FILE *file;
	const char *filename;
	bool progress_bar;
} write_info_t;

static void *
write_thread (void *arg)
{
	write_info_t *info = arg;
	uint32_t bytes_transfered = 0;

	if (info->progress_bar)
		print_progress (info->filename, 0, info->len);

	while (1) {
		buf_t *buf = get_dirty_buf ();

		/* If buf is NULL main called fifo_signal, it's time ot quit */
		if (buf == NULL)
			break;

		fseek (info->file, buf->off, SEEK_SET);
		fwrite (buf->data, 1, buf->len, info->file);

		bytes_transfered += buf->len;
		if (info->progress_bar)
			print_progress (info->filename, bytes_transfered, info->len);

		add_clean_buf (buf);
	}

	print_progress (info->filename, info->len, info->len);

	return NULL;
}

typedef struct {
	uint32_t start, len;
	int bandwidth;
	const char *url;
	bool finished;
} download_info_t;


static void
seek (mmsx_t *conn, uint32_t pos)
{
	static char seek_buf[BUF_SIZE];
	mms_off_t off;

	off = mmsx_seek (NULL, conn, pos, SEEK_SET);

	if (off != pos) {
		/* TODO: Implement binary search with mmsx_time_seek */
		print_error ("mmsx_seek not supported\n");

		while (off < pos) {
			mms_off_t left = pos - off;
			int data_read  = mmsx_read (NULL, conn, seek_buf, left > BUF_SIZE ? BUF_SIZE : left);

			/* At the end of the stream, can not seek any further */
			if (data_read == 0)
				break;

			pos += data_read;
		}
	}
}

static void *
download_thread (void *arg)
{
	download_info_t *info = arg;
	uint32_t pos  = info->start;
	uint32_t left = info->len;
	mmsx_t *conn;

	conn = mmsx_connect (NULL, NULL, info->url, info->bandwidth);

	if (conn == NULL) {
		print_error ("Could not open %s\n", info->url);
		info->finished = false;
		pthread_exit (NULL);
	}

	seek (conn, pos);

	while (left > 0) {
		int bytes_read;
		buf_t *buf = get_clean_buf ();

		bytes_read = mmsx_read (NULL, conn, buf->data,
				left < BUF_SIZE ? left : BUF_SIZE);

		if (bytes_read == 0) break;

		buf->off = pos;
		buf->len = bytes_read;

		pos += bytes_read;
		left -= bytes_read;

		add_dirty_buf (buf);
	}

	mmsx_close (conn);

	info->finished = true;

	return NULL;
}

/* Tries to connect to the stream to retrieve some information about it
 * Returns true on success, false on error.
 */
static bool
mmsx_get_info (const char *url, uint32_t *len, bool *seekable, int bandwidth)
{
	mmsx_t *mmsx;

	print_info (1, "Connecting to %s...\n", url);

	mmsx = mmsx_connect (NULL, NULL, url, bandwidth);

	if (mmsx == NULL) {
		print_error ("Could not connect to %s\n", url);
		return false;
	}

	*len      = mmsx_get_length (mmsx);
	*seekable = mmsx_get_seekable (mmsx);

	print_info (2, "Stream length:   %i bytes\n"
	               "Stream seekable: %s\n",
	               len, seekable ? "true" : "false");

	mmsx_close (mmsx);

	return true;
}

static bool
download (options_t *options)
{
	FILE *file;
	uint32_t len, len_per_thread;
	bool seekable;
	int thread_count = options->thread_count;
	pthread_t threads[thread_count + 1];
	download_info_t download_infos[thread_count];
	write_info_t write_info;
	int remaining;

	if (!mmsx_get_info (options->url, &len, &seekable, options->bandwidth))
		return false;

	if (!seekable) {
		print_error ("Stream is not seekable, using a single thread\n");
		thread_count = 1;
		/* TODO: Find out if len is correct in this case */
	}

	if (!(file = fopen (options->filename, "w"))) {
		print_error ("Could not open %s - %s\n",
				options->filename,
				strerror (errno));
		return false;
	}

	if (ftruncate (fileno (file), len)) {
		print_error ("ftruncate failed %s\n",
				strerror (errno));
		return false;
	}

	print_info (1, "Starting download\nUsing %i threads\n", thread_count);

	/* Get the amount of data each thread should download (rounded up) */
	len_per_thread = (len + (thread_count - 1)) / thread_count;

	/* Fill in the info structures for the download threads */
	for (int i = 0; i < thread_count; i++) {
		download_info_t *info = download_infos + i;

		info->bandwidth = options->bandwidth;
		info->url       = options->url;
		info->start     = len_per_thread * i;
		info->finished  = false;

		/* The last thread might have less data to download */
		if (i == thread_count - 1) {
			info->len = len - len_per_thread * (thread_count - 1);
		} else {
			info->len = len_per_thread;
		}
	}

	/* Create the thread that writes the data to file */
	write_info.file = file;
	write_info.len  = len;
	write_info.filename     = options->filename;
	write_info.progress_bar = options->progress_bar;
	pthread_create (threads + thread_count, NULL, write_thread, &write_info);

	do {
		/* Start the remaining threads */
		for (int i = 0; i < thread_count; i++) {
			if (!download_infos[i].finished)
				pthread_create (threads + i, NULL, download_thread, download_infos + i);
		}

		remaining = thread_count;

		/* Wait for the download threads to finish */
		for (int i = 0; i < thread_count; i++) {
			if (!download_infos[i].finished)
				pthread_join (threads[i], NULL);

			if (download_infos[i].finished)
				remaining--;
		}

		if (remaining == thread_count) {
			print_error ("All download threads failed\n");
			break;
		} else if (remaining > 0) {
			print_info (1, "%i threads did not finish, restarting them\n"
			               "Try lowering the number of threads\n", remaining);
		}
	} while (remaining > 0);


	/* Wait for the write thread to write all the dirty data */
	fifo_signal (&dirty_bufs);
	pthread_join (threads[thread_count], NULL);

	fclose (file);

	print_info (1, "\nDownload complete\n");

	return true;
}

int
main (int argc, char *argv[])
{
	options_t options;

	if (!options_parse (argc, argv, &options))
		return 1;

	print_set_verbosity_level (options.verbosity_level);

	for (int i = 0; i < 2 * options.thread_count; i++) {
		add_clean_buf (malloc (sizeof (buf_t)));
	}

	if (!download (&options))
		return 1;

	return 0;
}
