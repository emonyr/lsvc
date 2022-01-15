#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mem.h"
#include "transport.h"

void *_transport_recv_routine(void *arg)
{
	int err,nbyte;
	unsigned char payload[TRANSPORT_MAX_PAYLOAD];
	transport_t *t = arg;

	while (t->open) {
		err = socket_wait(&t->iface, 2);
		if (!err) {
			nbyte = socket_recv(&t->iface, payload, sizeof(payload));
			if (nbyte>0)
				t->cb(t->arg, payload, nbyte);
		}
	}

	return NULL;
}

void *_transport_accept_routine(void *arg)
{
	int err;
	unsigned char payload[TRANSPORT_MAX_PAYLOAD];
	transport_t *t = arg;

	while (t->open) {
		err = socket_wait(&t->iface, 2);
		if (!err) {
			err = socket_recv(&t->iface, payload, sizeof(payload));
			if (!err) {
				char uri[TRANSPORT_MAX_URI] = {0};
				transport_t *cli = NULL;
				socket_info_t *cli_iface = (void *)payload;

				sprintf(uri, "tcp://%s:%s", cli_iface->des.addr, cli_iface->des.port);
				cli = transport_create(uri, t->cb, t->arg);
				if (cli) {
					list_add_tail(&cli->li, &t->li);
					printf("+++++++ %s %d cli_iface.fd %d\n", __func__, __LINE__, cli_iface->fd);
					printf("+++++++ %s %d t->iface.fd %d\n", __func__, __LINE__, t->iface.fd);
				} else {
					fprintf(stderr, "Failed to establish connection to client: %s\n", uri);
					continue;
				}
			}
		}
	}

	return NULL;
}

void *transport_create(const char *uri, transport_callback_t *cb, void *arg)
{
	int err=-1;
	transport_t *t=NULL;

	if (!uri || !cb) {
		fprintf(stderr, "Invalid param: uri %p, cb %p\n", uri, cb);
		return NULL;
	}

	t = mem_alloc(sizeof(transport_t));
	if (!t) {
		fprintf(stderr, "Failed to allocate transport memory\n");
		return NULL;
	}

	memset(t, 0, sizeof(transport_t));
	snprintf(t->uri, TRANSPORT_MAX_URI-1, "%s", uri);
	t->cb = cb;
	t->arg = arg;
	INIT_LIST_HEAD(&t->li);

	err = socket_open(t->uri, &t->iface);
	if (err) {
		fprintf(stderr, "Failed to create transport socket\n");
		goto failed;
	}
	
	t->thread.routine = t->iface.type == TYPE_TCP_SERVER ? 
						_transport_accept_routine : _transport_recv_routine;
	t->thread.arg = t;
	t->open = 1;
	err = parallel_thread_create(&t->thread);
	if (err) {
		fprintf(stderr, "Failed to create transport thread\n");
		goto failed;
	}

	return t;

failed:
	transport_destroy(t);
	return NULL;
}

void transport_destroy(transport_t *t)
{
	transport_t *target,*tmp;
	if (t) {
		t->open = 0;
		parallel_thread_kill(&t->thread);
		socket_close(&t->iface);
		// destroy client list
		list_for_each_entry_safe(target,tmp, &t->li, li) {
			transport_destroy(target);
		}
		mem_free(t);
	}
}

int transport_xfer(const char *uri, transport_t *t, void *data, int size)
{
	if (!t || !data || !size) {
		fprintf(stderr, "%s: Invalid param\n", __func__);
		return -1;
	}

	if (socket_parse_uri(uri, &t->iface)) {
		fprintf(stderr, "Invalid destination address\n");
		return -1;
	}
	
	return socket_send(&t->iface, data, size);
}