/**
 * Multithreaded, libevent-based socket server.
 * Copyright (c) 2012 Ronald Bennett Cemer
 * Copyright (c) 2015 Sandeep Panwar
 * This software is licensed under the BSD license.
 * See the accompanying LICENSE.txt for details.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <event.h>
#include <signal.h>

#include "workqueue.h"
#include "rand_string.h"


#define SOCKET_READ_TIMEOUT_SECONDS 10
#define SOCKET_WRITE_TIMEOUT_SECONDS 10

#define NUM_THREADS 8


#define errorOut(...) {\
	fprintf(stderr, "%s:%d: %s():\t", __FILE__, __LINE__, __FUNCTION__);\
	fprintf(stderr, __VA_ARGS__);\
}

int server_port = 8787;

/* client specific data. */
typedef struct client {
	int fd; /* The client's socket. */
	struct event_base *evbase;
	struct bufferevent *buf_ev;
	struct evbuffer *output_buffer;
} client_t;

static struct event_base *evbase_accept;
static workqueue_t workqueue;


static void sighandler(int signal);


static int setnonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

static void closeClient(client_t *client) {
	if (client != NULL) {
		if (client->fd >= 0) {
			close(client->fd);
			client->fd = -1;
		}
	}
}

static void closeAndFreeClient(client_t *client) {
	if (client != NULL) {
		closeClient(client);
		if (client->buf_ev != NULL) {
			bufferevent_free(client->buf_ev);
			client->buf_ev = NULL;
		}
		if (client->evbase != NULL) {
			event_base_free(client->evbase);
			client->evbase = NULL;
		}
		if (client->output_buffer != NULL) {
			evbuffer_free(client->output_buffer);
			client->output_buffer = NULL;
		}
		free(client);
	}
}

void buffered_on_read(struct bufferevent *bev, void *arg) {
	client_t *client = (client_t *)arg;
	char data[255];
	int nb;
    int is_bye = 0;

    nb = EVBUFFER_LENGTH(bev->input);
    printf("on_read, nb: %d\n",nb);
    if (nb > 255)
        nb = 255;
    evbuffer_remove(bev->input, data, nb);
    
    printf("Data read: %s\n",data);
    if (strncmp((const char *)data, (const char *)"GETMESSAGE\n", 11) == 0)
    {
        strcpy(data,"The message is ");
        get_rand_str(&data[15], 50);
    }
    else if (strncmp((const char *)data, (const char *)"BYE\n", 4) == 0)
    {
        is_bye = 1;
    }
    else
    {
        strcpy(data,"Bad Request\n");
    }
    
    if (is_bye == 1)
    {
        closeClient(client);
    }
    else
    {
        nb = strlen(data);
        evbuffer_add(client->output_buffer, data, nb);
        if (bufferevent_write_buffer(bev, client->output_buffer)) {
            perror("bufferevent_write_buffer");
            closeClient(client);
        }
    }
}


void buffered_on_write(struct bufferevent *bev, void *arg) {
}


void buffered_on_error(struct bufferevent *bev, short what, void *arg) {
	closeClient((client_t *)arg);
}

static void server_job_function(struct job *job) {
	client_t *client = (client_t *)job->user_data;

	event_base_dispatch(client->evbase);
	closeAndFreeClient(client);
	free(job);
}


void on_accept(int fd, short ev, void *arg) {
	int client_fd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	workqueue_t *workqueue = (workqueue_t *)arg;
	client_t *client;
	job_t *job;

    printf("Accept new client, fd: %d\n",client_fd);
	client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
	if (client_fd < 0) {
		warn("accept failed");
		return;
	}

	if (setnonblock(client_fd) < 0) {
		warn("failed to set client socket to non-blocking");
		close(client_fd);
		return;
	}

	if ((client = malloc(sizeof(*client))) == NULL) {
		warn("failed to allocate memory for client state");
		close(client_fd);
		return;
	}
	memset(client, 0, sizeof(*client));
	client->fd = client_fd;


	if ((client->output_buffer = evbuffer_new()) == NULL) {
		warn("client output buffer allocation failed");
		closeAndFreeClient(client);
		return;
	}

	if ((client->evbase = event_base_new()) == NULL) {
		warn("client event_base creation failed");
		closeAndFreeClient(client);
		return;
	}


	if ((client->buf_ev = bufferevent_new(client_fd, buffered_on_read, buffered_on_write, buffered_on_error, client)) == NULL) {
		warn("client bufferevent creation failed");
		closeAndFreeClient(client);
		return;
	}
	bufferevent_base_set(client->evbase, client->buf_ev);

	bufferevent_settimeout(client->buf_ev, SOCKET_READ_TIMEOUT_SECONDS, SOCKET_WRITE_TIMEOUT_SECONDS);


	bufferevent_enable(client->buf_ev, EV_READ);


	if ((job = malloc(sizeof(*job))) == NULL) {
		warn("failed to allocate memory for job state");
		closeAndFreeClient(client);
		return;
	}
	job->job_function = server_job_function;
	job->user_data = client;

	workqueue_add_job(workqueue, job);
}


int runServer(void) {
	int listenfd;
	struct sockaddr_in listen_addr;
	struct event ev_accept;
	int reuseaddr_on;

	/* Initialize libevent. */
	event_init();

	/* Set signal handlers */
	sigset_t sigset;
	sigemptyset(&sigset);
	struct sigaction siginfo = {
		.sa_handler = sighandler,
		.sa_mask = sigset,
		.sa_flags = SA_RESTART,
	};
	sigaction(SIGINT, &siginfo, NULL);
	sigaction(SIGTERM, &siginfo, NULL);

	/* Create our listening socket. */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		err(1, "listen failed");
	}
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(server_port);
	if (bind(listenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
		err(1, "bind failed");
	}
	if (listen(listenfd, 5) < 0) {
		err(1, "listen failed");
	}
	reuseaddr_on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on));


	if (setnonblock(listenfd) < 0) {
		err(1, "failed to set server socket to non-blocking");
	}

	if ((evbase_accept = event_base_new()) == NULL) {
		perror("Unable to create socket accept event base");
		close(listenfd);
		return 1;
	}


	if (workqueue_init(&workqueue, NUM_THREADS)) {
		perror("Failed to initialize the work queue");
		close(listenfd);
		workqueue_shutdown(&workqueue);
		return 1;
	}


	event_set(&ev_accept, listenfd, EV_READ|EV_PERSIST, on_accept, (void *)&workqueue);
	event_base_set(evbase_accept, &ev_accept);
	event_add(&ev_accept, NULL);

	printf("Server Started.\n");


	event_base_dispatch(evbase_accept);

	event_base_free(evbase_accept);
	evbase_accept = NULL;

	close(listenfd);

	printf("Server shutdown.\n");

	return 0;
}


void killServer(void) {
	fprintf(stdout, "Stopping socket listener event loop.\n");
	if (event_base_loopexit(evbase_accept, NULL)) {
		perror("Error shutting down server");
	}
	fprintf(stdout, "Stopping workers.\n");
	workqueue_shutdown(&workqueue);
}

static void sighandler(int signal) {
	fprintf(stdout, "Received signal %d: %s.  Shutting down.\n", signal, strsignal(signal));
	killServer();
}

int main(int argc, char *argv[]) 
{
    int c;
    int port = 0;
    
    while ((c = getopt(argc, argv, "p:")) != -1)
    {
        switch(c)
        {
            case 'p':
            port = atoi(optarg);
            printf("\noptarg: %s\n",optarg);
            break;
            default:
            printf("\nInvalid argument: %c\n",c);
            break;
        }
    }
    if (port != 0)
    {
       server_port = port; 
    }
    printf("Using port: %d\n", server_port);
     /* seed the random function */
     srand(time(NULL));
    
 
     if (fork() == 0) {
        return runServer();
     }
	
    return 0;
}
