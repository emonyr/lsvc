#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>

#include "mem.h"
#include "transport.h"

int _transport_wait(transport_t *t, int second)
{
	int err;
	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	FD_SET(t->iface.fd, &readfds);
	
	tv.tv_sec = second;
	tv.tv_usec = 5000;	// at least wait 5ms

	err = select(FD_SETSIZE,&readfds,NULL,NULL,&tv);
	if (err > 0 && FD_ISSET(t->iface.fd, &readfds)) {
		return 0;
	} 

	return -1;
}

void *_transport_std_routine(void *arg)
{
	int err,nbyte;
	unsigned char payload[TRANSPORT_MAX_PAYLOAD];
	transport_t *t = arg;

	while (t->open) {
		err = _transport_wait(t, 2);
		if (!err) {
			nbyte = socket_recv(&t->iface, payload, sizeof(payload));
			if (nbyte>0)
				t->cb(t->arg, payload, nbyte);
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

	err = socket_open(t->uri, &t->iface);
	if (err) {
		fprintf(stderr, "Failed to create transport socket\n");
		goto failed;
	}
	
	t->thread.routine = _transport_std_routine;
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
	if (t) {
		t->open = 0;
		parallel_thread_kill(&t->thread);
		socket_close(&t->iface);
		mem_free(t);
	}
}

int transport_xfer(const char *uri, transport_t *t, void *data, int size)
{
	if (!uri || !t || !data || !size) {
		fprintf(stderr, "%s: Invalid param\n", __func__);
		return -1;
	}

	if (socket_parse_uri(uri, &t->iface)) {
		fprintf(stderr, "Invalid destination address\n");
		return -1;
	}
	
	return socket_send(&t->iface, data, size);
}