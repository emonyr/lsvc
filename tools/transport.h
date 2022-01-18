#ifndef __TRANSPORT_TOOL_H__
#define __TRANSPORT_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"
#include "socket.h"
#include "parallel.h"

#define TRANSPORT_MAX_URI (128)
#define TRANSPORT_MAX_PAYLOAD (4096)
#define TRANSPORT_MAX_ERR (128)

typedef void *(transport_callback_t)(void *arg, void *payload, int nbyte);

typedef struct {
	int open;
	char uri[TRANSPORT_MAX_URI];
	transport_callback_t *cb;
	void *arg;
	int wait_err;
	int recv_err;
	socket_info_t iface;
	parallel_spin_t lock;
	parallel_thread_t thread;
	struct list_head li;
}transport_t;

extern void *transport_create(const char *uri, transport_callback_t *cb, void *arg);
extern void transport_destroy(transport_t *t);
extern int transport_xfer(const char *uri, transport_t *t, void *data, int size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __TRANSPORT_TOOL_H__ */