#ifndef __LBUS_H__
#define __LBUS_H__

#include <pthread.h>
#include "socket.h"
#include "list.h"
#include "kvlist.h"
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
	socket_info_t iface;
	unsigned char *payload;
}__attribute__ ((packed))lbus_msg_t;

#define LMSG_MAX_SIZE 65535
#define LMSG_HEADER_SIZE (sizeof(lbus_msg_t))
#define LMSG_MAX_DATA_SIZE (LMSG_MAX_SIZE - LMSG_HEADER_SIZE)

typedef struct{
	char name[32];
	int running;
	int bond;
	int (*recv_cb)(void *msg,void *userdata);
	void *runtime;
	void *queue;
	void *userdata;
	void *timer;
	pthread_t tid;
	pthread_mutex_t mtx;
	socket_info_t iface;
	struct list_head entry;
}__attribute__ ((packed))lbus_endpoint_t;

typedef struct {
	int running;
	void *runtime;
	int (*dispatch)(void *msg,void *userdata);
	lbus_endpoint_t ep;
	pthread_mutex_t mtx;
	struct list_head peers;
	struct kvlist route_table;
}__attribute__ ((packed))lbus_broker_t;

extern void *lbus_payload_get(void *msg);
extern void * lbus_msg_new(unsigned int data_size);
extern void lbus_msg_destroy(void *msg);

extern int lbus_send_ping(lbus_endpoint_t *ep);
extern int lbus_send_pong(lbus_endpoint_t *ep, void *src_msg);

extern int lbus_borker_create(lbus_broker_t *bk);
extern int lbus_endpoint_create(lbus_endpoint_t *ep);
extern void lbus_endpoint_destroy(lbus_endpoint_t *ep);

extern int lbus_msg_broadcast(lbus_endpoint_t *ep,void *msg);

extern lsvc_t lbus_svc;

#endif /* __LBUS_H__ */