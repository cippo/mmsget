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
