#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mem.h"
#include "transport.h"

void *_transport_do_create(const char *uri, 
					transport_callback_t *cb, void *arg, void *iface);

void *_transport_recv_routine(void *arg)
{
	int nbyte;
	unsigned char payload[TRANSPORT_MAX_PAYLOAD];
	transport_t *t = arg;

	while (t->open) {
		if (socket_wait(&t->iface, 2) > 0) {
			nbyte = socket_recv(&t->iface, payload, sizeof(payload));
			if (nbyte > 0) {
				t->cb(t->arg, payload, nbyte);
			} else if (t->recv_err++ > TRANSPORT_MAX_ERR)
				break;
		}
	}

	return NULL;
}

void _transport_prune_client(transport_t *server)
{
	transport_t *target, *next;

	list_for_each_entry_safe(target, next, &server->li, li) {
		if (target->recv_err > TRANSPORT_MAX_ERR) {
			list_del(&target->li);
			transport_destroy(target);
		}
	}
}

void *_transport_accept_routine(void *arg)
{
	int ret;
	unsigned char payload[TRANSPORT_MAX_PAYLOAD];
	transport_t *t = arg;
	
	while (t->open) {
		ret = socket_wait(&t->iface, 2);
		if (ret > 0) {
			ret = socket_recv(&t->iface, payload, sizeof(payload));
			if (ret >= 0) {
				char uri[TRANSPORT_MAX_URI] = {0};
				transport_t *cli = NULL;
				socket_info_t *cli_iface = (void *)payload;

				sprintf(uri, "tcp://%s:%s", cli_iface->des.addr, cli_iface->des.port);
				cli = _transport_do_create(uri, t->cb, t->arg, cli_iface);
				if (cli) {
					list_add_tail(&cli->li, &t->li);
				} else {
					fprintf(stderr, "Failed to establish connection to client: %s\n", uri);
					continue;
				}
			} else if (t->recv_err++ > TRANSPORT_MAX_ERR)
				break;
		} else if (ret == 0)
			_transport_prune_client(t);
	}

	return NULL;
}

void *_transport_do_create(const char *uri, 
					transport_callback_t *cb, void *arg, void *iface)
{
	int err=-1;
	transport_t *t=NULL;

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

	if (iface) {
		memcpy(&t->iface, iface, sizeof(t->iface));
	} else {
		err = socket_open(t->uri, &t->iface);
		if (err) {
			fprintf(stderr, "Failed to create transport socket\n");
			goto failed;
		}
	}
	
	parallel_spin_init(&t->lock);
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

void *transport_create(const char *uri, transport_callback_t *cb, void *arg)
{
	if (!uri || !cb) {
		fprintf(stderr, "Invalid param: uri %p, cb %p\n", uri, cb);
		return NULL;
	}

	return _transport_do_create(uri, cb, arg, NULL);
}

void transport_destroy(transport_t *t)
{
	transport_t *target,*next;
	if (t) {
		t->open = 0;
		parallel_thread_kill(&t->thread);
		socket_close(&t->iface);
		// destroy client list
		if (t->iface.type == TYPE_TCP_SERVER) {
			list_for_each_entry_safe(target, next, &t->li, li) {
				list_del(&target->li);
				transport_destroy(target);
			}
		}
		mem_free(t);
	}
}

int transport_xfer(const char *uri, transport_t *t, void *data, int size)
{
	int nbyte;
	if (!t || !data || !size) {
		fprintf(stderr, "%s: Invalid param\n", __func__);
		return -1;
	}

	if (socket_parse_uri(uri, &t->iface)) {
		fprintf(stderr, "Invalid destination address\n");
		return -1;
	}
	
	parallel_spin_lock(&t->lock);
	nbyte = socket_send(&t->iface, data, size);
	parallel_spin_unlock(&t->lock);

	return nbyte;
}