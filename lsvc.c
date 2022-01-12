#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "utils.h"
#include "log.h"
#include "file.h"
#include "lbus.h"
#include "network.h"
#include "lsvc.h"


static int saved_argc = 0;
static const char **saved_argv = NULL;
char *dummy_argv[] = {"lsvc"};

// Be careful with the sequence
lsvc_t *user_lsvc[] = {
	&log_svc,
	&lbus_svc,
	&network_svc,
	NULL,
};

lsvc_runtime_t g_lsvc = {
	.running = 0,
	.svc_list = user_lsvc,
};

void *lsvc_runtime_init(void);

void *lsvc_runtime_get(void)
{
	if (!g_lsvc.running)
		return lsvc_runtime_init();
	
	return &g_lsvc;
}


int lsvc_service_ioctl(void *_msg)
{
	int err;
	lsvc_t **svc;
	lbus_msg_t *msg = _msg;
	lsvc_runtime_t *r = lsvc_runtime_get();
	if (!r) {
		log_err("Invalid runtime\n");
		return -1;
	}
		
	for(svc=r->svc_list; *svc; svc++) {
		if (lsvc_valid_event(msg->event, (*svc)->ev_base)) {
			log_info("------ handling %s 0x%08X ------\n", (*svc)->name, msg->event);
			if ((*svc)->ioctl) {
				err = (*svc)->ioctl(r, msg);
				if (err < 0)
					log_err("Service ioctl failed: ev 0x%08X\n", msg->event);
			}
		}
	}
	
	return err;
}

// inter-service call
int lsvc_thread_call(void *runtime, void *msg)
{
	lbus_msg_t *m = msg;
	lsvc_runtime_t *r = lsvc_runtime_get();
	if (!r) {
		log_err("Invalid runtime\n");
		return -1;
	}
	
	if (!m || m->flags & LMSG_RESPONSE) {
		log_err("Invalid msg\n");
		return -1;
	}
	
	log_debug("------ lsvc thread call ------\n");
	log_debug("ep->name: %s\n", ((lbus_endpoint_t *)r->priv)->name);
	log_debug("m->event: 0x%08X\n", m->event);
	log_debug("m->size: %d\n", m->size);
	log_debug("m->flags: %d\n", m->flags);
	
	return lsvc_service_ioctl(m);
}

int lsvc_handle_bus_call_routine(void *msg, void *userdata)
{
	lbus_msg_t *m = msg;
	lsvc_runtime_t *r = NULL;

	if (userdata) {
		r = userdata;
	}else if (!(r = lsvc_runtime_get())) {
		log_err("Invalid runtime\n");
		return -1;
	}
	
	if (!m || m->flags & LMSG_RESPONSE) {
		log_err("Invalid msg\n");
		return -1;
	}
	
	log_debug("------ lsvc bus call ------\n");
	log_debug("ep->name: %s\n", ((lbus_endpoint_t *)r->priv)->name);
	log_debug("m->event: 0x%08X\n", m->event);
	log_debug("m->size: %d\n", m->size);
	log_debug("m->flags: %d\n", m->flags);
	
	return lsvc_service_ioctl(m);
}

// inter-process call
int lsvc_bus_call(void *runtime, void *msg)
{
	lsvc_runtime_t *r;
	
	if (runtime) {
		r = runtime;
	}else{
		r = lsvc_runtime_get();
	}
	
	if (!r) {
		log_err("Invalid runtime\n");
		return -1;
	}
	
	return lbus_msg_broadcast(r->priv, msg);
}

void lsvc_service_init(void)
{
	lsvc_t **svc;
	lsvc_runtime_t *r = lsvc_runtime_get();
	if (!r) {
		log_err("Invalid runtime\n");
		return;
	}
		
	for(svc = r->svc_list; *svc; svc++) {
		if ((*svc)->init)
			(*svc)->init(r);
	}
	return;
}

void lsvc_service_exit(void)
{
	lsvc_t **svc;
	lsvc_runtime_t *r = lsvc_runtime_get();
	if (!r) {
		log_err("Invalid runtime\n");
		return;
	}
		
	for(svc = r->svc_list; *svc; svc++) {
		if ((*svc)->exit)
			(*svc)->exit(r);
	}
}

int lsvc_shell_routine(void *msg, void *userdata)
{
	lbus_msg_t *m = msg;
	
	log_debug("m->event: 0x%08X\n", m->event);
	log_debug("m->size: %d\n", m->size);
	log_debug("m->flags: %d\n", m->flags);
	
	if (m->payload)
		log_hex_dump(m->payload, m->size);
	
	return 0;
}

void *lsvc_runtime_init(void)
{
	void *callback;
	
	if (!saved_argv) {
		saved_argv = dummy_argv;
		saved_argc = ARRAY_SIZE(dummy_argv);
	}
	
	if (saved_argc < 2) {
		callback = lsvc_handle_bus_call_routine;
	}else{
		callback = lsvc_shell_routine;
	}
	
	g_lsvc.priv = lbus_endpoint_create("lsvc", NULL, NULL, callback, &g_lsvc);
	if (!g_lsvc.priv) {
		log_err("Failed to create lsvc endpoint\n");
		return NULL;
	}
	
	g_lsvc.running = 1;
	
	return &g_lsvc;
}

int lsvc_handle_sys_command(int argc, const char *argv[])
{
	int err = -1;
	lbus_msg_t *msg = NULL;
	lsvc_t **svc;
	lsvc_runtime_t *r;

	if (argc < 2) {
		log_err("Usage: %s MODULE [OPTION]\n", file_get_striped_name(argv[0]));
		return -1;
	}

	saved_argc = argc;
	saved_argv = argv;

	r = lsvc_runtime_get();
	if (!r) {
		log_err("Invalid runtime\n");
		return -1;
	}
	
	for (svc=r->svc_list; *svc; svc++) {
		if (strcmp(argv[1], (*svc)->name) != 0)
			continue;

		if ((*svc)->getopt) {
			msg = NULL;
			err = (*svc)->getopt(argc, &argv[0], &msg);
			
			if (err) {
				log_err("Failed to match module event\n");
				break;
			}
		}
		
		// use lsvc_bus_call since it's a different process
		if (msg) 
			err = lsvc_bus_call(r, msg);
		break;
	}
	
	if (err < 0)
		log_err("Usage: %s MODULE [OPTION]\n", file_get_striped_name(argv[0]));
	
	lbus_msg_destroy(msg);
	return err;
}

void lsvc_main_loop(void)
{
	while (g_lsvc.running) {
		sleep(1);
	}
}

void lsvc_shutdown(void *runtime)
{
	lsvc_runtime_t *r;
	
	if (runtime) {
		r = runtime;
	}else{
		r = &g_lsvc;
	}
	
	r->running = 0;
	lbus_endpoint_destroy(r->priv);
	r->priv = NULL;
}

int lsvc_event_send(int event, unsigned char *data, unsigned int size, 
						unsigned int flags, const void *_msg)
{
	int err;
	lbus_msg_t *src_msg = _msg;
	lbus_msg_t *m;
	lsvc_runtime_t *r;
	
	r = lsvc_runtime_get();
	if (!r) {
		log_err("Invalid runtime\n");
		return -1;
	}
	
	m = lbus_msg_new(size);
	if (!m) {
		log_err("Failed to alloc msg buffer\n");
		return -1;
	}
	
	m->event = event;
	memcpy(m->payload, data, size);
	m->flags = flags;

	if ((m->flags & LMSG_RESPONSE) && src_msg) {
		memcpy(&m->src, &src_msg->src, sizeof(src_msg->src));
		return lbus_msg_respond(r->priv, m);
	}
	
	if (m->flags & LMSG_BUS_CALL) {
		err = lsvc_bus_call(NULL, m);
	} else {
		err = lsvc_thread_call(NULL, m);
	}
	
	lbus_msg_destroy(m);
	
	return err;
}


