#ifndef __LSVC_H__
#define __LSVC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <getopt.h>

typedef enum {
	LSVC_LBUS_EVENT = 0x10000000,
	LSVC_LOG_EVENT = 0x20000000,
	LSVC_NETWORK_EVENT = 0x30000000,
	LSVC_EVENT_MASK = 0xFFFF0000,
}lsvc_ev_t;

static inline int lsvc_valid_event(int ev, lsvc_ev_t ev_base)
{
	return ((ev & LSVC_EVENT_MASK) == ev_base);
}

typedef struct {
	int ev_base;
	char *name;
	int (*getopt)(int argc, const char *argv[], void **msg);
	int (*init)(void *runtime);
	int (*exit)(void *runtime);
	int (*ioctl)(void *runtime, void *msg);
}__attribute__ ((packed))lsvc_t;

typedef struct {
	uint8_t running;
	lsvc_t **svc_list;
	void *priv;
}__attribute__ ((packed))lsvc_runtime_t;

extern void *lsvc_runtime_get(void);
extern int lsvc_thread_call(void *runtime, void *msg);
extern int lsvc_bus_call(void *runtime, void *msg);
extern void lsvc_shutdown(void *runtime);
extern int lsvc_event_send(int event, void *data, uint32_t size, 
						uint32_t flags, void *_src_msg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LSVC_H__ */