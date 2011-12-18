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

#include "fifo.h"
#include <stdlib.h>

struct fifo_item_St {
	void *data;

	fifo_item_t *next;
	fifo_item_t *prev;
};

/* Adds an element to the front of the FIFO */
void
fifo_push (fifo_t *fifo, void *elem)
{
	fifo_item_t *item = malloc (sizeof (fifo_item_t));

	pthread_mutex_lock (&fifo->lock);

	item->data = elem;
	item->next = fifo->head;
	item->prev = NULL;

	if (fifo->head != NULL)
		fifo->head->prev = item;

	fifo->head = item;

	if (fifo->tail == NULL)
		fifo->tail = item;

	pthread_cond_signal (&fifo->cond);
	pthread_mutex_unlock (&fifo->lock);
}

/* Removes the oldest element from the FIFO and returns it.
 * If the FIFO is empty and fifo_signal has been called, it will return NULL.
 */
void*
fifo_pop (fifo_t *fifo)
{
	void *ret = NULL;
	fifo_item_t *item = NULL;

	pthread_mutex_lock (&fifo->lock);

	while (fifo->tail == NULL && !fifo->signal) {
		pthread_cond_wait (&fifo->cond, &fifo->lock);
	}

	if (fifo->tail != NULL) {
		item = fifo->tail;
		fifo->tail = item->prev;
	}

	if (fifo->tail == NULL) {
		fifo->head = NULL;
	} else {
		fifo->tail->next = NULL;
	}

	pthread_mutex_unlock (&fifo->lock);

	if (item) {
		ret = item->data;
		free (item);
	}

	return ret;
}

/* Wakes up any threads sleeping in fifo_pop, making them return NULL */
void
fifo_signal (fifo_t *fifo)
{
	pthread_mutex_lock (&fifo->lock);

	fifo->signal = true;

	pthread_cond_signal (&fifo->cond);
	pthread_mutex_unlock (&fifo->lock);
}
