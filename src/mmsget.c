#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <libmms/mmsx.h>

#define THREAD_COUNT 10
#define BANDWIDTH    INT_MAX
#define BUF_SIZE     (12 * 1024)

typedef struct buf_St {
	char data[BUF_SIZE];
	int  len;
	mms_off_t off;

	struct buf_St *next;
} buf_t;

typedef struct {
	buf_t *root;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	pthread_cond_t  empty_cond;
} buf_list_t;

buf_list_t dirty_bufs = {
	.root = NULL,
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.cond = PTHREAD_COND_INITIALIZER,
	.empty_cond = PTHREAD_COND_INITIALIZER,
};
buf_list_t clean_bufs = {
	.root = NULL,
	.lock = PTHREAD_MUTEX_INITIALIZER,
	.cond = PTHREAD_COND_INITIALIZER,
	.empty_cond = PTHREAD_COND_INITIALIZER,
};
uint32_t file_len;

static void
add_buf (buf_t *buf, buf_list_t *list)
{
	pthread_mutex_lock (&list->lock);

	buf->next = list->root;
	list->root = buf;

	pthread_cond_signal (&list->cond);
	pthread_mutex_unlock (&list->lock);
}

static buf_t*
get_buf (buf_list_t *list)
{
	buf_t *ret;

	pthread_mutex_lock (&list->lock);

	while (list->root == NULL) {
		pthread_cond_wait (&list->cond, &list->lock);
	}

	ret = list->root;
	list->root = ret->next;

	if (list->root == NULL) {
		pthread_cond_signal (&list->empty_cond);
	}

	pthread_mutex_unlock (&list->lock);

	return ret;
}

static void
wait_for_empty (buf_list_t *list)
{
	pthread_mutex_lock (&list->lock);

	while (list->root != NULL) {
		pthread_cond_wait (&list->empty_cond, &list->lock);
	}

	pthread_mutex_unlock (&list->lock);
}

#define add_dirty_buf(b) add_buf (b, &dirty_bufs)
#define add_clean_buf(b) add_buf (b, &clean_bufs)
#define get_dirty_buf() get_buf (&dirty_bufs)
#define get_clean_buf() get_buf (&clean_bufs)


static double
timeval_diff (struct timeval *a, struct timeval *b)
{
	return (double)(b->tv_sec - a->tv_sec) +
		(double)(b->tv_usec - a->tv_usec) / 1e6;
}

static void
print_bytes (uint32_t bytes)
{
	if (bytes >= (1024*1024)) {
		printf ("%#5.4gMiB", ((double)bytes) / (1024.0*1024.0));
	} else if (bytes >= 1024) {
		printf ("%#5.4gKiB", ((double)bytes) / 1024.0);
	} else {
		printf ("%#5.4g  B", ((double)bytes));
	}
}

static void
update_progress ()
{
	static struct timeval times[THREAD_COUNT * 2];
	static int count = 0;
	static int time = 0;

	double bytes;
	double secs;
	uint64_t done = count * BUF_SIZE;
	int progress = done * 50 / file_len;

	if ((count % 10) == 0) {
		gettimeofday (times + time, NULL);
		time = (time + 1) % (THREAD_COUNT * 2);
	}

	if ((count / 10) < (THREAD_COUNT * 2)) {
		secs = timeval_diff (times, times + ((time - 1) % (THREAD_COUNT * 2)));
		bytes = BUF_SIZE * 10 * (time - 1);
	} else {
		secs = timeval_diff (times + time, times + ((time - 1) % (THREAD_COUNT * 2)));
		bytes = BUF_SIZE * 10 * THREAD_COUNT * 2;
	}

	/* Print progress bar */
	printf ("\r[");
	for (int i = 0; i < 50; i++) {
		if (i < progress) {
			printf ("=");
		} else {
			printf ("-");
		}
	}
	printf ("]");

	switch ((count / 4) % 4) {
	case 0:
		printf (" - ");
		break;
	case 1:
		printf (" \\ ");
		break;
	case 2:
		printf (" | ");
		break;
	case 3:
		printf (" / ");
	}

	print_bytes (done);
	printf (" of ");
	print_bytes (file_len);
	printf (" at ");

	print_bytes (bytes/secs);
	printf("/s");
	fflush (stdout);

	count++;
}

static void *
write_thread (void *arg)
{
	FILE *file = arg;
	printf ("\n");

	update_progress ();

	while (1) {
		buf_t *buf = get_dirty_buf ();

		fseek (file, buf->off, SEEK_SET);
		fwrite (buf->data, 1, buf->len, file);

		add_clean_buf (buf);

		update_progress ();
	}

	return NULL;
}

typedef struct {
	int id;
	uint32_t start, len;
	const char *url;
} download_info_t;


static void
seek (mmsx_t *conn, uint32_t pos)
{
	static char seek_buf[BUF_SIZE];
	mms_off_t off;

	off = mmsx_seek (NULL, conn, pos, SEEK_SET);

	if (off != pos) {
		/* TODO: Implement binary search with mmsx_time_seek */
		fprintf (stderr, "mmsx_seek not supported\n");

		while (off < pos) {
			mms_off_t left = pos - off;
			mmsx_read (NULL, conn, seek_buf, left > BUF_SIZE ? BUF_SIZE : left);
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

	conn = mmsx_connect (NULL, NULL, info->url, BANDWIDTH);

	if (conn == NULL) {
		fprintf (stderr, "Could not open %s\n", info->url);
		pthread_exit (NULL);
	}

	seek (conn, pos);

	while (left > 0) {
		int bytes_read;
		buf_t *buf = get_clean_buf ();

		bytes_read  = mmsx_read (NULL, conn, buf->data,
				left < BUF_SIZE ? left : BUF_SIZE);
		buf->off = pos;
		buf->len = bytes_read;

		pos += bytes_read;
		left -= bytes_read;

		add_dirty_buf (buf);
	}

	mmsx_close (conn);
	free (info);

	return NULL;
}

int
main (int argc, char *argv[])
{
	const char *url = argv[1];
	const char *filename = "test.wmv";
	FILE *file;
	mmsx_t *mmsx;
	pthread_t threads[THREAD_COUNT + 1];
	uint32_t len_per_thread;

	file = fopen (filename, "w");

	if (file == NULL) {
		fprintf (stderr, "Could not open %s - %s\n",
				filename,
				strerror (errno));
	}

	for (int i = 0; i < 2 * THREAD_COUNT; i++) {
		add_clean_buf (malloc (sizeof (buf_t)));
	}

	printf ("Connecting to %s...\n", url);

	mmsx = mmsx_connect (NULL, NULL, url, BANDWIDTH);

	if (mmsx == NULL) {
		fprintf (stderr, "Could not connect to %s\n", url);
		return 1;
	}

	file_len = mmsx_get_length (mmsx);
	mmsx_close (mmsx);

	ftruncate (fileno (file), file_len);

	len_per_thread = (file_len + THREAD_COUNT - 1) / THREAD_COUNT;

	printf ("Connected\nUsing %i threads\n", THREAD_COUNT);

	for (int i = 0; i < THREAD_COUNT; i++) {
		download_info_t *info = malloc (sizeof (download_info_t));

		info->id = i;
		info->url = url;
		info->start = len_per_thread * i;

		if (i == THREAD_COUNT - 1) {
			info->len = file_len - len_per_thread * (THREAD_COUNT - 1);
		} else {
			info->len = len_per_thread;
		}

		pthread_create (threads + i, NULL, download_thread, info);
	}

	pthread_create (threads + THREAD_COUNT, NULL, write_thread, file);

	for (int i = 0; i < THREAD_COUNT; i++) {
		pthread_join (threads[i], NULL);
	}

	wait_for_empty (&dirty_bufs);

	pthread_cancel (threads[THREAD_COUNT]);
	pthread_join (threads[THREAD_COUNT], NULL);

	fclose (file);

	printf ("\nDownload complete\n");

	return 0;
}
