
#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdbool.h>
#include <pthread.h>

typedef struct fifo_item_St fifo_item_t;

typedef struct {
	fifo_item_t *head;
	fifo_item_t *tail;

	pthread_mutex_t lock;
	pthread_cond_t  cond;
	bool signal;
} fifo_t;

#define FIFO_INITIALIZER {\
	.head = NULL,\
	.tail = NULL,\
	.lock = PTHREAD_MUTEX_INITIALIZER,\
	.cond = PTHREAD_COND_INITIALIZER,\
	.signal = false\
}

void   fifo_push   (fifo_t *fifo, void *elem);
void  *fifo_pop    (fifo_t *fifo);
void   fifo_signal (fifo_t *fifo);

#endif /* _FIFO_H_ */
