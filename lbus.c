#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <assert.h>

#include "kmsg.h"
#include "file.h"
#include "mem.h"
#include "timer.h"
#include "socket.h"
#include "log.h"
#include "lbus.h"

#define LBUS_UDP_PORT 13333

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
	lbus_msg_t *m = msg;
	
	if (!m->msg_id)
		m->msg_id = time(NULL);
	
	pthread_mutex_lock(&ep->mtx);
	m->iface.fd = ep->iface.fd,
	sprintf(m->iface.addr, "%s", ep->iface.addr);
	m->iface.port = LBUS_UDP_PORT,	// sendto broker port
	
	nbyte = socket_udp_send(&m->iface, m, LMSG_HEADER_SIZE + m->size);
	if (nbyte < 0)
		log_err("error: %s\n", strerror(errno));
	pthread_mutex_unlock(&ep->mtx);
	
	return nbyte;
}

int lbus_push_data(lbus_endpoint_t *ep, void *data, int size)
{
	int nbyte;
	
	struct kmsg *kqueue = ep->queue;
	
	pthread_mutex_lock(&ep->mtx);
	nbyte = kmsg_push(kqueue, data, size);
	// log_debug("kmsg_push: nbyte %d kqueue->size %d\n", nbyte, kqueue->size);
	if (!kqueue->size)
		log_err("Invalid kqueue size %d\n", kqueue->size);
	pthread_mutex_unlock(&ep->mtx);
	
	// log_debug("lbus_push_data: nbyte %d\n", nbyte);
	
	return nbyte;
}

int lbus_pop_data(lbus_endpoint_t *ep, void **bucket)
{
	int nbyte;
	
	if (!ep->running)
		return -1;
	
	pthread_mutex_lock(&ep->mtx);
	
	nbyte = kmsg_check(ep->queue);
	//log_debug(">>>>>>>>>>>>>kmsg_check: nbyte %d\n", nbyte);
	if (!nbyte || nbyte < LMSG_HEADER_SIZE) {
		pthread_mutex_unlock(&ep->mtx);
		return -1;
	}
	
	*bucket = lbus_msg_new(nbyte);
	if (!(*bucket)) {
		log_err("Failed to create new msg buffer\n");
		sleep(1);
		pthread_mutex_unlock(&ep->mtx);
		return -1;
	}
	
	nbyte = kmsg_pop(ep->queue, *bucket);
	if (nbyte < 0) {
		log_err("Invlid msg: nbyte %d\n", nbyte);
		mem_free(*bucket);
		*bucket = NULL;
		pthread_mutex_unlock(&ep->mtx);
		return -1;
	}
	pthread_mutex_unlock(&ep->mtx);
	
	//log_debug("nbyte %d\n", nbyte);
	
	return nbyte;
}

void lbus_udp_push_routine(void *data)
{
	int nbyte = 0;
	lbus_msg_t *m = lbus_msg_new(LMSG_MAX_DATA_SIZE);
    lbus_endpoint_t *ep = (lbus_endpoint_t *)data;
	
	assert(m);
	
	//log_debug("Init\n");
	pthread_detach(pthread_self());

    while(ep->running) {
		usleep(5000);	//sleep 5ms to release CPU
		
		nbyte = socket_udp_recv(&ep->iface, m, LMSG_MAX_SIZE);
		if (nbyte > 0) {
			// log_debug("### recv nbyte %d\n\n",nbyte);

			/* 
			 * Copy endpoint source interface info into message, 
			 * so we can receive response from broker.
			 */
			memcpy(&m->iface.src, &ep->iface.src, sizeof(m->iface.src));
			lbus_push_data(ep,m,nbyte);
		}
    }
	
	lbus_msg_destroy(m);
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

void lbus_udp_pop_routine(void *data)
{
	int nbyte = 0;
	lbus_msg_t *m = NULL;
    lbus_endpoint_t *ep = (lbus_endpoint_t *)data;
	
	//log_debug("Init\n");
	pthread_detach(pthread_self());

	ep->timer = utils_timer_start(lbus_send_ping, ep, 1, 2, 1);

    while(ep->running) {
		usleep(20000);
		
		nbyte = lbus_pop_data(ep, &m);
		if (nbyte > 0 && m && ep->recv_cb) {
			// log_debug("Pop nbyte %d\n", nbyte);
			// log_warn("%s m->event: 0x%08X\n", ep->name, m->event);
			if (!ep->bond && lbus_try_bond(m, ep) == -1) {
				lbus_push_data(ep,m,nbyte);	// Not yet bonded, push back msg to the queue
			}else{
				m->payload = lbus_payload_get(m);
				ep->recv_cb(m, ep->userdata);
			}
			
			lbus_msg_destroy(m);
			m = NULL;
		}
	}
}

void lbus_endpoint_destroy(lbus_endpoint_t *ep)
{
	if (ep->running == 0)
		return;
	
	ep->running = 0;
	utils_timer_stop(ep->timer);
	pthread_mutex_destroy(&ep->mtx);
	socket_close(&ep->iface);
	if (ep->queue)
		kmsg_delete(ep->queue);
}

void *lbus_queue_init(void)
{
	int err;
	void *queue;
	
	queue = malloc(sizeof(struct kmsg));
	if (!queue) {
		log_err("Failed to allocate queue buffer\n");
		goto out;
   	}
	
	err = kmsg_new(queue, MAX_MSG_NUMBER * 8192); //128 * 8192
    if (err < 0) {
		log_err("Failed to create queue\n");
		free(queue);
		queue = NULL;
		goto out;
   	}
	
out:
	return queue;
}

int lbus_broker_create(lbus_broker_t *bk)
{
	int err;
	
	if (bk->running) {
		log_err("lbus broker already running\n");
		return 1;
	}
	
	if (!bk->dispatch) {
		log_err("dispatch function not defined\n");
		return -1;
	}
	
	bk->ep.recv_cb = bk->dispatch;
	bk->ep.iface.port = LBUS_UDP_PORT;
	bk->ep.userdata = bk;
	bk->ep.runtime = bk->runtime;
	bk->ep.bond = 1; // broker auto bonded
	err = lbus_endpoint_create(&bk->ep);
	if (err < 0) {
		log_debug("can not create endpoint\n");
		return -1;
	}
	bk->running = 1;
	sprintf(bk->ep.name, "%s", "broker");
	
	return 0;
}

int lbus_endpoint_create(lbus_endpoint_t *ep)
{
	int err = -1;
	pthread_t pop_tid;
	
	if (ep->running) {
		log_err("lbus endpoint already running\n");
		return 1;
	}
	
	sprintf(ep->iface.addr, "127.0.0.1");
	
	err = socket_udp_init(&ep->iface);
	if (err < 0) {
		log_err("Udp socket invalid\n");
		return -1;
	}
	
	INIT_LIST_HEAD(&ep->entry);
	pthread_mutex_init(&ep->mtx, NULL);
	ep->queue = lbus_queue_init();
	if (!ep->queue) {
		log_err("Failed to init queue\n");
		socket_close(&ep->iface);
		return -1;
	}
	
	ep->running = 1;
	if (0 != pthread_create(&ep->tid, NULL, (void *)lbus_udp_push_routine, ep)) {
		log_err("can not create push routine\n");
		return -1;
	}
	
	if (0 != pthread_create(&pop_tid, NULL, (void *)lbus_udp_pop_routine, ep)) {
		log_err("can not create pop routine\n");
		return -1;
	}
	
	err = socket_bind(&ep->iface);
	if (err < 0) {
		lbus_endpoint_destroy(ep);
		return -1;
	}
	
	return err;
}

int lbus_direct_send(lbus_endpoint_t *ep, void *msg)
{
	int nbyte;
	lbus_msg_t *m = msg;
	socket_info_t iface = {0};

	if (ep->iface.fd <= 0 || m->iface.des.sin_port <= 0) {
		log_err("Invalid param: fd %d port %d\n", ep->iface.fd, ntohs(m->iface.des.sin_port));
		return -1;
	}

	iface.fd = ep->iface.fd;
	sprintf(iface.addr, "%s", ep->iface.addr);
	iface.port = ntohs(m->iface.des.sin_port);
	
	nbyte = socket_udp_send(&iface, m, LMSG_HEADER_SIZE + m->size);
	if (nbyte < 0)
		log_err("error: %s\n", strerror(errno));

	return nbyte;
}

int lbus_send_ping(lbus_endpoint_t *ep)
{
	int err;
	lbus_msg_t *msg = NULL;

	if (ep->bond)
		return 0;
	
	msg = lbus_msg_new(sizeof(lbus_endpoint_t));
	if (!msg) {
		log_err("Failed to alloc lbus msg buffer\n");
		return -1;
	}
	
	msg->event = LBUS_EV_PING;	
	memcpy(msg->payload,ep,sizeof(lbus_endpoint_t)); 		
	
	err = lbus_msg_broadcast(ep, msg);
	lbus_msg_destroy(msg);
	
	return err;
}

int lbus_send_pong(lbus_endpoint_t *ep, void *_msg)
{
	int err;
	lbus_msg_t *src_msg = _msg;
	lbus_msg_t *msg = NULL;
	
	if (!ep) {
		log_err("Invalid pointer\n");
		return -1;
	}
	
	msg = lbus_msg_new(sizeof(lbus_endpoint_t));
	if (!msg) {
		log_err("Failed to alloc msg buffer\n");
		return -1;
	}
	
	msg->event = LBUS_EV_PONG;
	memcpy(msg->payload, ep, sizeof(lbus_endpoint_t));
	memcpy(&msg->iface.des, &src_msg->iface.src, sizeof(msg->iface.des));
	
	err = lbus_direct_send(ep, msg);
	lbus_msg_destroy(msg);
	
	return err;
}

int lbus_route_table_update(lbus_msg_t *msg)
{
	int err = 0;
	lbus_broker_t *bk = &broker;
	lbus_endpoint_t *target, *ep;
	int value = ntohs(bk->ep.iface.src.sin_port);
	char port[24] = {0};
	
	if (!value || value == LBUS_UDP_PORT) {
		log_debug("Ignoring invalid port\n");
		return -1;
	}
	
	pthread_mutex_lock(&bk->mtx);
	memcpy(&msg->iface.src, &bk->ep.iface.src, sizeof(msg->iface.src));
	sprintf(port, "%d", ntohs(bk->ep.iface.src.sin_port));
	log_debug("Handling port: %s\n", port);
	
	target = kvlist_get(&bk->route_table, port);
	if (target) {
		err = -1;
		goto out;
	}
	
	ep = malloc(sizeof(lbus_endpoint_t));
	if (!ep) {
		err = -1;
		goto out;
	}
	
	memcpy(ep, &bk->ep, sizeof(lbus_endpoint_t));
	kvlist_set(&bk->route_table, port, (void *)ep);
	list_add_tail(&ep->entry, &bk->peers);
	log_info("new port added: %s\n", port);
	
out:
	pthread_mutex_unlock(&bk->mtx);
	return err;
}

int lbus_dispatch_callback(void *msg, void *userdata)
{
	int nbyte;
	lbus_broker_t *bk = &broker;
	lbus_msg_t *m = msg;
	lbus_endpoint_t *target, *tmp;
	
	//log_debug("\n");
	//log_debug("m->event: 0x%08X\n", m->event);
	//log_debug("m->size: %d\n", m->size);
	
	if (!bk->running) {
		log_err("Invalid broker\n");
		return -1;
	}
	
	/* Respond to LBUS event directly */
	if (lsvc_valid_event(m->event, LSVC_LBUS_EVENT))
		return lsvc_thread_call(NULL, m);
	
	
	if (m->flags & LMSG_RESPONSE) {
		log_debug("Handle lbus response 0x%08X\n", m->event);
		return lbus_direct_send(&bk->ep, m);
	}
	
	/* Broadcast message to each recorded endpoint */
	list_for_each_entry_safe(target,tmp, &bk->peers, entry) {
		/* 
		 * Copy message source interface info into destination, 
		 * so services can send response to it.
		 */
		memcpy(&m->iface.des, &m->iface.src, sizeof(m->iface.des));
		nbyte = sendto(bk->ep.iface.fd, m, LMSG_HEADER_SIZE + m->size, 
						MSG_DONTWAIT | MSG_NOSIGNAL, 
						(struct sockaddr*)&target->iface.src, sizeof(target->iface.src));
		if (nbyte < 0)
			log_err("Failed to send: port %d\n", ntohs(target->iface.src.sin_port));
		if (nbyte > 0)
			log_debug("Msg sent to port: %d\n", ntohs(target->iface.src.sin_port));
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
	
	pthread_mutex_init(&bk->mtx, NULL);
	INIT_LIST_HEAD(&bk->peers);
	kvlist_init(&bk->route_table, kvlist_strlen);
	
	bk->dispatch = lbus_dispatch_callback;
	bk->runtime = runtime;
	err = lbus_broker_create(bk);
	if (err < 0) {
		log_err("Failed to create broker, skip\n");
		lbus_endpoint_destroy(&bk->ep);
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
				err = lbus_send_pong(&bk->ep, msg);
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
