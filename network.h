#ifndef __NETWORK_SVC_H_
#define __NETWORK_SVC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "lsvc.h"

#define NETWORK_SSID_MAX_LENGTH 32
#define NETWORK_PWD_MAX_LENGTH 64

typedef enum {
	NETWORK_EV_GET_STATE = LSVC_NETWORK_EVENT,
	NETWORK_EV_REPORT_STATE,
	NETWORK_EV_PING,
	NETWORK_EV_WIFI_CONFIG_SUCCESS,
	NETWORK_EV_WIFI_CONFIG_START,
	NETWORK_EV_WIFI_CONFIG_TIMEOUT,
	NETWORK_EV_WIFI_CONFIG_FAILED,
	NETWORK_EV_WIFI_CONFIG_USER_CONFIRM,
	NETWORK_EV_WIFI_CONFIG_ENTER,
	NETWORK_EV_WIFI_CONFIG_QUIT,
	NETWORK_EV_WIFI_CONNECTED,
	NETWORK_EV_WIFI_CONNECTING,
	NETWORK_EV_WIFI_DISCONNECTED,
}network_svc_event_t;

typedef enum {
	NW_WIFI_DISCONNECTED,
	NW_WIFI_CONNECTING,
	NW_WIFI_CONNECTED,
	NW_WIFI_CONFIG,
}network_state_val_t;

typedef struct {
	network_state_val_t v;
	void *tid;
	uint8_t ssid[NETWORK_SSID_MAX_LENGTH+1];	//including '\0'
	uint8_t ssid_len;
	uint8_t pwd[NETWORK_PWD_MAX_LENGTH+1];	//including '\0'
	uint8_t pwd_len;
	uint32_t security;
	uint8_t mac[32];
	uint8_t ip[32];
}__attribute__ ((packed))network_svc_state_t;

typedef struct {
	char ping_address[64];
	uint32_t count;
}network_ctl_t;

extern int network_ping(const char *des, uint32_t count);
extern int network_svc_intent_handler(int event, void *data, size_t size);

extern lsvc_t network_svc;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __NETWORK_SVC_H_ */