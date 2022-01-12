#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "kmsg.h"
#include "file.h"
#include "mem.h"
#include "timer.h"
#include "socket.h"
#include "log.h"
#include "lbus.h"

#define LBUS_HOST "127.0.0.1"
#define LBUS_PORT "13333"

lbus_broker_t broker = {0};

void *lbus_payload_get(void *msg)
{
	lbus_msg_t *m = msg;
	void *p = NULL;
	
	if (m->size)
		p = msg + LMSG_HEADER_SIZE;

	return p;
}

void * lbus_msg_new(unsigned int data_size)
{
	int alloc_len = LMSG_HEADER_SIZE + data_size;
	lbus_msg_t *m = NULL;
	
	if (data_size > LMSG_MAX_DATA_SIZE) {
		log_err("Request oversize\n");
		return NULL;
	}
	
	m = mem_alloc(alloc_len);
	if (!m) {
		log_err("mem_alloc failed\n");
		return NULL;
	}
	
	m->size = data_size;
	m->payload = lbus_payload_get(m);
	
	return m;
}

void lbus_msg_destroy(void *msg)
{
	lbus_msg_t *m = msg;
	if (m) {
		mem_free(m);
	}
}

int lbus_msg_broadcast(lbus_endpoint_t *ep,void *msg)
{
	int nbyte;
	char uri[TRANSPORT_MAX_URI]={0};
	lbus_msg_t *m = msg;
	
	if (!m->msg_id)
		m->msg_id = utils_timer_now();
	
	parallel_spin_lock(&ep->lock);
	sprintf(m->des.addr, "%s", LBUS_HOST);
	sprintf(m->des.port, "%s", LBUS_PORT);	// sendto broker
	sprintf(uri, "udp://%s:%s", m->des.addr, m->des.port);
	
	nbyte = transport_xfer(uri, ep->transport, m, LMSG_HEADER_SIZE + m->size);
	if (nbyte < 0)
		log_err("Failed to send broadcast msg\n");
	parallel_spin_unlock(&ep->lock);
	
	return nbyte;
}

int lbus_try_bond(void *msg, void *endpoint)
{
	lbus_msg_t *m = msg;
	lbus_endpoint_t *ep = endpoint;
	
	if (m->event  == LBUS_EV_PONG) {
		ep->bond = 1;
		return 0;
	}
	
	return -1;
}

void *lbus_handle_recv(void *arg, void *data, int nbyte)
{
	lbus_endpoint_t *ep = (lbus_endpoint_t *)arg;
	lbus_msg_t *m = data;
	
	if (!m || nbyte <=0) {
		log_err("Invalid recv data\n");
		return NULL;
	}
	
	if (!ep->bond) {
		lbus_try_bond(m, ep);
	}else{
		m->payload = lbus_payload_get(m);
		ep->recv_cb(m, ep->userdata);
	}

	return NULL;
}

void lbus_endpoint_destroy(lbus_endpoint_t *ep)
{
	utils_timer_stop(ep->timer);
	transport_destroy(ep->transport);
	memset(ep, 0, sizeof(lbus_endpoint_t));
}

int lbus_dispatch_callback(void *msg, void *userdata);

int lbus_broker_init(lbus_broker_t *bk)
{	
	parallel_spin_init(&bk->lock);
	bk->ep = lbus_endpoint_create("broker", LBUS_HOST, LBUS_PORT, lbus_dispatch_callback, bk);
	if (!bk->ep) {
		log_debug("Failed to create broker endpoint\n");
		return -1;
	}
	bk->running = 1;
	bk->ep->bond = 1;	// broker auto bonded
	
	return 0;
}

void *lbus_endpoint_create(const char *name, const char *host, const char *port, 
								lbus_endpoint_callback_t *cb, void *userdata)
{
	lbus_endpoint_t *ep;

	if (!cb) {
		log_err("Invalid endpoint callback\n");
		return NULL;
	}

	ep = mem_alloc(sizeof(lbus_endpoint_t));
	if (!ep) {
		log_err("Failed to alloc endpoint memory\n");
		goto failed;
	}
	
	memset(ep, 0, sizeof(lbus_endpoint_t));
	sprintf(ep->name, "%s", name?name:"endpoint");
	parallel_spin_init(&ep->lock);
	ep->recv_cb = cb;
	ep->userdata = userdata;
	sprintf(ep->uri, "udp://%s:%s", host?host:LBUS_HOST, port?port:"0");
	ep->transport = transport_create(ep->uri, lbus_handle_recv, ep);
	ep->timer = utils_timer_start(lbus_send_ping, ep, 0, 2, 1);
	if (!ep->timer) {
		log_err("Failed to create endpoint timer\n");
		goto failed;
	}
	
	return ep;

failed:
	lbus_endpoint_destroy(ep);
	return NULL;
}

int lbus_msg_respond(lbus_endpoint_t *ep, void *msg)
{
	int nbyte;
	char uri[TRANSPORT_MAX_URI]={0};
	lbus_msg_t *m = msg;

	memcpy(&m->des, &m->src, sizeof(m->src));
	sprintf(uri, "udp://%s:%s", m->des.addr, m->des.port);
	
	nbyte = transport_xfer(uri, ep->transport, m, LMSG_HEADER_SIZE + m->size);
	if (nbyte < 0)
		log_err("Failed to send direct msg\n");

	return nbyte;
}

int lbus_send_ping(lbus_endpoint_t *ep)
{
	lbus_msg_t msg={0};
	
	if (ep->bond)
		return 0;
	
	msg.event = LBUS_EV_PING;
	
	return lbus_msg_broadcast(ep, &msg);
}

int lbus_send_pong(lbus_endpoint_t *ep, void *_msg)
{
	lbus_msg_t *src_msg = _msg;
	lbus_msg_t msg={0};
	
	if (!ep || !src_msg) {
		log_err("Invalid pointer\n");
		return -1;
	}

	msg.event = LBUS_EV_PONG;
	memcpy(&msg.src, &src_msg->src, sizeof(src_msg->src));
	
	return lbus_msg_respond(ep, &msg);
}

int lbus_route_table_update(lbus_msg_t *msg)
{
	int err = 0;
	lbus_broker_t *bk = &broker;
	lbus_endpoint_t *target, *ep;
	char uri[TRANSPORT_MAX_URI] = {0};
	
	if (atoi(msg->src.port) <= 0 || 
		strcmp(msg->src.port, LBUS_PORT) == 0) {
		log_debug("Ignoring invalid port\n");
		return -1;
	}
	
	parallel_spin_lock(&bk->lock);
	sprintf(uri, "udp://%s:%s", msg->src.addr, msg->src.port);
	log_debug("Handling address: %s\n", uri);
	
	target = kvlist_get(&bk->route_table, uri);
	if (target) {
		err = -1;
		goto out;
	}
	
	ep = mem_alloc(sizeof(lbus_endpoint_t));
	if (!ep) {
		err = -1;
		goto out;
	}
	
	sprintf(ep->uri, "%s", uri);
	kvlist_set(&bk->route_table, uri, (void *)ep);
	list_add_tail(&ep->entry, &bk->peers);
	log_info("New endpoint added: %s\n", ep->uri);
	
out:
	parallel_spin_unlock(&bk->lock);
	return err;
}

int lbus_dispatch_callback(void *msg, void *userdata)
{
	int nbyte;
	lbus_broker_t *bk = userdata;
	lbus_msg_t *m = msg;
	lbus_endpoint_t *target, *tmp;
	
	//log_debug("\n");
	//log_debug("m->event: 0x%08X\n", m->event);
	//log_debug("m->size: %d\n", m->size);
	
	if (!bk->running) {
		log_err("Invalid broker\n");
		return -1;
	}

	/* 
	 * Copy source address into message destination, 
	 * so services can send response to it.
	 */
	memcpy(&m->src, &bk->ep->transport->iface.src, sizeof(bk->ep->transport->iface.src));
	
	/* Respond to LBUS event directly */
	if (lsvc_valid_event(m->event, LSVC_LBUS_EVENT))
		return lsvc_thread_call(NULL, m);
	
	/* Broadcast message to each recorded endpoint */
	list_for_each_entry_safe(target,tmp, &bk->peers, entry) {
		nbyte = transport_xfer(target->uri, bk->ep->transport, m, LMSG_HEADER_SIZE + m->size);
		if (nbyte <= 0) {
			log_err("Failed sending to endpoint: %s\n", target->uri);
		} else {
			log_debug("Msg 0x%08X sent to endpoint: %s\n", m->event, target->uri);
		}
	}
	
	return 0;
}


/*
 * lsvc api implementation
 */

int lbus_svc_init(void *runtime)
{
	int err;
	lbus_broker_t *bk = &broker;
	
	INIT_LIST_HEAD(&bk->peers);
	kvlist_init(&bk->route_table, kvlist_strlen);
	
	err = lbus_broker_init(bk);
	if (err < 0) {
		log_err("Failed to create broker, skip\n");
		lbus_endpoint_destroy(bk->ep);
		return -1;
	}
	
	log_debug("lbus_svc_init success\n");
	
	return err;
}

int lbus_svc_exit(void *runtime)
{
	
	return 0;
}

int lbus_svc_getopt(int argc, const char *argv[], void **msg)
{
	//int opt,err=0;
	// lbus_msg_t *m;
	// lbus_ctl_t *ctl;

	// *msg = lbus_msg_new(sizeof(lbus_ctl_t));
	// if (!*msg) {
	// 	log_err("Failed to alloc msg buffer\n");
	// 	return -1;
	// }

	// m = msg;
	// ctl = m->payload;
	// while((opt = getopt(argc, (void *)argv, "M:")) != -1) {
	// 	switch (opt) {
	// 		case 'l':
	// 			m->event = LOG_EV_SET_LEVEL;
	// 			ctl->mode = atoi(optarg);
	// 			break;

	// 		case '?':
	// 		case '-':
	// 			err = -1;
	// 			break;
	// 	}
	// }
	
	return 0;
}

int lbus_svc_ioctl(void *runtime, void *_msg)
{
	int err;
	lbus_msg_t *msg = _msg;
	lbus_broker_t *bk = &broker;
	
	switch(msg->event) {
		case LBUS_EV_PING:
			if (!lbus_route_table_update(msg))
				err = lbus_send_pong(bk->ep, msg);
		break;
	}
	
	if (err < 0)
		log_err("ev 0x%08X\n", msg->event);
	
	return err;
}

lsvc_t lbus_svc = {
	.ev_base = LSVC_LBUS_EVENT,
	.name = "lbus",
	.getopt = NULL,
	.init = lbus_svc_init,
	.exit = lbus_svc_exit,
	.ioctl = lbus_svc_ioctl,
};
