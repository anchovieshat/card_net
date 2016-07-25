#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "common.h"

typedef struct ConNode {
    i32 socket_fd;
	pthread_t t;
	struct ConNode *next;
	struct ConList *list;
} ConNode;

typedef struct ConList {
	ConNode *head;
	u32 size;
	u32 max_size;
	pthread_mutex_t list_lock;
	struct MessageQueue *queue;
} ConList;

typedef struct MessageNode {
	i32 origin_fd;
	char *message;
	struct MessageNode *next;
} MessageNode;

typedef struct MessageQueue {
	MessageNode *head;
	MessageNode *tail;
	u32 size;
	pthread_mutex_t queue_lock;
	pthread_cond_t has_things;
	pthread_t r_t;
	ConList *list;
} MessageQueue;

void start_repeater(MessageQueue *q) {
	while (1) {
		pthread_mutex_lock(&q->queue_lock);
		while (q->size == 0) {
			pthread_mutex_unlock(&q->queue_lock);
			pthread_cond_wait(&q->has_things, &q->queue_lock);
		}

		// Send top message to all connections
		pthread_mutex_lock(&q->list->list_lock);
		ConNode *tmp_conn = q->list->head;
		char *message_mod = malloc(strlen(q->head->message) + 10);
		sprintf(message_mod, "%d %s", q->head->origin_fd, q->head->message);

		while (tmp_conn != NULL) {
			send(tmp_conn->socket_fd, message_mod, strlen(message_mod), 0);
			tmp_conn = tmp_conn->next;
		}
		free(message_mod);
		pthread_mutex_unlock(&q->list->list_lock);

		// Pop message from message queue and free
		MessageNode *tmp_msg = q->head;
		if (q->size == 1) {
			q->head = NULL;
			q->tail = NULL;
			free(tmp_msg->message);
			free(tmp_msg);
			q->size--;
		} else {
			q->head = q->head->next;
			free(tmp_msg->message);
			free(tmp_msg);
			q->size--;
		}

		pthread_mutex_unlock(&q->queue_lock);
	}
}

void init_queue(MessageQueue *q, ConList *l) {
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
	q->list = l;
	pthread_mutex_init(&q->queue_lock, NULL);
	pthread_cond_init(&q->has_things, NULL);

	pthread_create(&q->r_t, NULL, (void *)start_repeater, q);
}

void push_message(MessageQueue *q, i32 origin_fd, char *message) {
	MessageNode *tmp = malloc(sizeof(MessageNode));
	tmp->origin_fd = origin_fd;
	tmp->message = message;
	tmp->next = NULL;
	pthread_mutex_lock(&q->queue_lock);
	if (q->head == NULL && q->tail == NULL) {
		q->head = tmp;
		q->tail = tmp;
	} else {
    	q->tail->next = tmp;
	}
	q->size++;
	pthread_mutex_unlock(&q->queue_lock);
	pthread_cond_signal(&q->has_things);
}


ConList *new_list(u32 max_size, MessageQueue *queue) {
	ConList *l = malloc(sizeof(ConList));
	l->head = NULL;
	l->size = 0;
	l->max_size = max_size;
	l->queue = queue;
	pthread_mutex_init(&l->list_lock, NULL);
	return l;
}

void close_connection(ConNode *n) {
	printf("closing connection %d\n", n->socket_fd);
	pthread_mutex_lock(&n->list->list_lock);

	ConNode *cur = n->list->head;
	ConNode *prior = NULL;
	while (cur != n && cur != NULL) {
		prior = cur;
		cur = cur->next;
	}

	if (cur != NULL && prior != NULL) {
		puts("removing from middle");
		prior->next = cur->next;
	}

	close(n->socket_fd);
	if (n->list->size > 0) {
		n->list->size--;
	} else {
		puts("empty list!");
	}
	free(n);

	pthread_mutex_unlock(&n->list->list_lock);
	printf("connection closed %d\n", n->socket_fd);
	pthread_exit(NULL);
}

void *handle_client(ConNode *n) {
    printf("%d connected\n", n->socket_fd);

	while (1) {
		u32 msg_size = 1024;
		char *msg_buffer = malloc(msg_size);
		memset(msg_buffer, 0, msg_size);
		i32 recv_err = recv(n->socket_fd, msg_buffer, msg_size, 0);

        if (recv_err == -1) {
			puts("recv err!");

			close_connection(n);
			return NULL;
		}

        if (recv_err == 0) {
			close_connection(n);
			return NULL;
		}

		printf("[%d] %s\n", n->socket_fd, msg_buffer);
		push_message(n->list->queue, n->socket_fd, msg_buffer);
		usleep(10);
	}

	puts("shouldn't be here");

	pthread_exit(NULL);
}

void start_connection(ConList *l, i32 socket_fd) {
	pthread_mutex_lock(&l->list_lock);
	if (l->max_size > l->size) {
		ConNode *tmp = malloc(sizeof(ConNode));
		tmp->socket_fd = socket_fd;
		tmp->list = l;
		tmp->next = l->head;
		char *new_conn = malloc(50);
		sprintf(new_conn, "%d registered client", socket_fd);
		send(socket_fd, new_conn, strlen(new_conn), 0);
		free(new_conn);
		pthread_create(&tmp->t, NULL, (void *)handle_client, tmp);

		l->head = tmp;
		l->size++;
	} else {
		char *full = "list full!";
		puts(full);
		send(socket_fd, full, strlen(full), 0);
		close(socket_fd);
	}
	pthread_mutex_unlock(&l->list_lock);
}


void cleanup_list(ConList *l) {
	pthread_mutex_lock(&l->list_lock);
	ConNode *tmp = l->head;
	puts("cleaning up!");
	while (tmp != NULL) {
		pthread_mutex_unlock(&l->list_lock);
		pthread_join(tmp->t, NULL);

		pthread_mutex_lock(&l->list_lock);
		close(tmp->socket_fd);
		tmp = tmp->next;
	}
	pthread_mutex_unlock(&l->list_lock);
}

#endif
