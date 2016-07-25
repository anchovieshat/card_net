#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <pthread.h>

#include "common.h"

#define url "127.0.0.1"
#define port 3000

typedef struct Connection {
	i32 socket_fd;
	i32 server_id;
	pthread_mutex_t run_lock;
	u8 running;
} Connection;

char *split_on_first_whitespace(char *input, char *output2) {

	char *tmp = input;
	u32 idx = 0;
	while (*tmp != ' ' && tmp != NULL) {
		tmp++;
		idx++;
	}
	tmp++;

	char *output1 = malloc(idx + 1);
	strncpy(output1, input, idx);
	output1[idx] = '\0';

	strncpy(output2, tmp, strlen(tmp));
	return output1;
}

void *read_incoming(Connection *c) {
	u32 msg_size = 512;
	u8 first_run = 1;

	char msg_buffer[msg_size + 1];
	while (1) {
		memset(msg_buffer, 0, msg_size);

		i32 recv_err = recv(c->socket_fd, msg_buffer, msg_size, 0);
		if (recv_err == -1) {
			pthread_mutex_lock(&c->run_lock);
			c->running = 0;
			close(c->socket_fd);
			pthread_mutex_unlock(&c->run_lock);
			puts("recv error!");
			return NULL;
		} else if (recv_err == 0) {
			pthread_mutex_lock(&c->run_lock);
			c->running = 0;
			close(c->socket_fd);
			pthread_mutex_unlock(&c->run_lock);
			puts("connection closed");
			return NULL;
		}

		if (first_run) {
			char *out_str = malloc(strlen(msg_buffer));
			memset(out_str, 0, strlen(msg_buffer));
			char *id_num = split_on_first_whitespace(msg_buffer, out_str);
			c->server_id = atoi(id_num);
			printf("%s as [%d]\n", out_str, c->server_id);
			free(id_num);
			free(out_str);

			first_run = 0;
		} else {
			char *out_str = malloc(strlen(msg_buffer));
			memset(out_str, 0, strlen(msg_buffer));
			char *id_num = split_on_first_whitespace(msg_buffer, out_str);
			if (atoi(id_num) != c->server_id) {
				printf("[%d] %s\n", atoi(id_num), out_str);
			}
			free(id_num);
			free(out_str);
		}

		usleep(10);
	}

	return NULL;
}

int main() {
	i32 client_fd;

	if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		return 1;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_aton(url, (struct in_addr *)&server_addr.sin_addr.s_addr) == 0) {
		perror(url);
		return 1;
	}

	if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		perror("Connect ");
		return 1;
	}

	printf("connected to %s\n", url);

	pthread_t reader;
	Connection c;
	c.socket_fd = client_fd;
	c.running = 1;
	pthread_mutex_init(&c.run_lock, NULL);

	pthread_create(&reader, NULL, (void *)read_incoming, &c);

	u32 in_size = 100;
	char in_buffer[in_size];
	memset(&in_buffer, 0, in_size);

	fgets(in_buffer, in_size, stdin);
	pthread_mutex_lock(&c.run_lock);
	while (c.running) {
		pthread_mutex_unlock(&c.run_lock);
		if (strlen(in_buffer) > 0) {
			in_buffer[strlen(in_buffer) - 1] = 0;
		}

		send(client_fd, in_buffer, strlen(in_buffer) + 1, 0);
		memset(&in_buffer, 0, in_size);
		fgets(in_buffer, in_size, stdin);
		pthread_mutex_lock(&c.run_lock);
	}

	return 0;
}
