#ifndef __LBUS_H__
#define __LBUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "socket.h"
#include "list.h"
#include "kvlist.h"
#include "parallel.h"
#include "transport.h"
#include "lsvc.h"

typedef enum {
	LBUS_EV_GET_STATE = LSVC_LBUS_EVENT,
	LBUS_EV_PING,
	LBUS_EV_PONG,
}bus_svc_event_t;

typedef enum {
	LMSG_REQUEST 		= (1<<0),
	LMSG_RESPONSE 		= (1<<1),
	LMSG_BUS_CALL 		= (1<<2),
	LMSG_THREAD_CALL 	= (1<<3),
}lbus_flag_t;

typedef struct {
	unsigned int event;
	unsigned int msg_id;
	unsigned int size;
	unsigned int flags;
	socket_addr_t src;
	socket_addr_t des;
	unsigned char *payload;
}__attribute__ ((packed))lbus_msg_t;

#define LMSG_MAX_SIZE (10240)
#define LMSG_HEADER_SIZE (sizeof(lbus_msg_t))
#define LMSG_MAX_DATA_SIZE (LMSG_MAX_SIZE - LMSG_HEADER_SIZE)

typedef struct{
	char name[32];
	int bond;
	int (*recv_cb)(void *msg,void *userdata);
	void *userdata;
	void *timer;
	char uri[TRANSPORT_MAX_URI];
	transport_t *transport;
	parallel_spin_t lock;
}__attribute__ ((packed))lbus_endpoint_t;

#define LBUS_RECORD_KEY_SIZE (64)
#define LBUS_RECORD_PAYLOAD_SIZE (128)
#define LBUS_RECORD_TIMEOUT (10)
#define LBUS_PING_CYCLE (5)

typedef struct {
	void *self;
	int timestamp;
	char key[LBUS_RECORD_KEY_SIZE];
	unsigned char payload[LBUS_RECORD_PAYLOAD_SIZE];
	struct list_head entry;
}__attribute__ ((packed))lbus_route_record_t;

typedef struct {
	int running;
	int (*dispatch)(void *msg,void *userdata);
	lbus_endpoint_t *ep;
	parallel_spin_t lock;
	struct list_head peers;
	struct kvlist route_table;
}__attribute__ ((packed))lbus_broker_t;

extern void *lbus_payload_get(void *msg);
extern void * lbus_msg_new(unsigned int data_size);
extern void lbus_msg_destroy(void *msg);

extern int lbus_send_ping(lbus_endpoint_t *ep);
extern int lbus_send_pong(lbus_endpoint_t *ep, void *src_msg);

typedef void *(lbus_endpoint_callback_t)(void *, void*);
extern void *lbus_endpoint_create(const char *name, const char *host, const char *port, 
								lbus_endpoint_callback_t *cb, void *userdata);
extern void lbus_endpoint_destroy(lbus_endpoint_t *ep);

extern int lbus_msg_broadcast(lbus_endpoint_t *ep,void *msg);
extern int lbus_msg_respond(lbus_endpoint_t *ep, void *msg);

extern lsvc_t lbus_svc;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LBUS_H__ */