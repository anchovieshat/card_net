#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <pthread.h>

#include "common.h"
#include "queue.h"

#define port 3000
#define max_connections 5


int main() {
	struct sockaddr_in server_addr, client_addr;
	i32 server_fd, client_fd;

	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		puts("error creating socket");
		return 1;
	}

	server_addr.sin_family = AF_UNSPEC;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);

	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		puts("binding error!");
		return 1;
	}

	listen(server_fd, 2);
	printf("Waiting for connections on port %d\n", port);

	socklen_t client_addr_len = sizeof(client_addr);
 	MessageQueue *queue = malloc(sizeof(MessageQueue));
    ConList *list = new_list(max_connections, queue);
	init_queue(queue, list);

	while ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len))) {
    	start_connection(list, client_fd);
	}

    cleanup_list(list);
	close(server_fd);
	return 0;
}
