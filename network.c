#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>


#include "log.h"
#include "timer.h"
#include "lbus.h"
#include "socket.h"
#include "mem.h"
#include "file.h"
#include "network.h"

#define ICMP_HEADSIZE 8
#define PACKET_SIZE 1024

network_svc_state_t network_state;

int network_svc_state(const void *_msg)
{
	network_svc_state_t *s = &network_state;

	return lsvc_event_send(NETWORK_EV_GET_STATE, s, sizeof(network_svc_state_t),
							LMSG_RESPONSE, _msg);
}

typedef struct {
	socket_info_t sock;
	pid_t pid;
	void *tid;
	uint32_t timeout;
	char tx_buf[PACKET_SIZE];
	char rx_buf[PACKET_SIZE];
	uint32_t rx_len;
	uint32_t ttime;
}__attribute__ ((packed))network_ping_t;

uint16_t network_ping_chksum(uint16_t *data,int len)
{
	int nbyte = len;
	int sum=0;
	uint16_t *w=data;
	uint16_t answer=0;
	while(nbyte > 1) {
		sum += *w++;
		nbyte -= 2;
	}

	if (nbyte == 1) {
		*(uint8_t *)(&answer)=*(uint8_t *)w;
		sum += answer;
	}
	sum = (sum>>16)+ (sum&0xffff);
	sum += (sum>>16);
	answer = ~sum;

	return answer;
}

int network_ping_pack(int num, network_ping_t *p)
{
	int size;
	struct icmp *icmp = (struct icmp*)p->tx_buf;

	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_cksum = 0;
	icmp->icmp_seq = num;
	icmp->icmp_id = p->pid;

	size = ICMP_HEADSIZE + sizeof(struct timeval);
	gettimeofday((void *)icmp->icmp_data,NULL);
	icmp->icmp_cksum = network_ping_chksum((uint16_t *)icmp, size);

	return size;
}

int network_ping_unpack(int num, network_ping_t *p)
{
	int header_len,nbyte;
	struct ip *ip;
	struct icmp *icmp;
	ip = (struct ip *)p->rx_buf;

	header_len = ip->ip_hl << 2;
	icmp = (struct icmp *)(p->rx_buf + header_len);

	nbyte = p->rx_len;
	nbyte -= header_len;
	if (nbyte < 8)
		return -1;

	if ((icmp->icmp_type == ICMP_ECHOREPLY)
		&& (icmp->icmp_id == p->pid) && (icmp->icmp_seq == num)) {
		p->ttime = icmp->icmp_ttime;
		return 0;
	}

	return -1;
}

void network_ping_timeout(void *arg)
{
	network_ping_t *p = arg;

	p->timeout = 1;
}

int network_ping_send(int num, network_ping_t *p)
{
	int size;
	size = network_ping_pack(num, p);

	return socket_send(&p->sock, p->tx_buf, size);
}

int network_ping_recv(int num, network_ping_t *p)
{
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(p->sock.fd,&rfds);
	struct timeval tv;

	p->tid = utils_timer_start(network_ping_timeout,p,2,0,0);
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	while(!p->timeout) {
		select(p->sock.fd + 1, &rfds, NULL, NULL, &tv);
		if (FD_ISSET(p->sock.fd,&rfds)) {
			p->rx_len = socket_recv(&p->sock, p->rx_buf, sizeof(p->rx_buf));
			if (p->rx_len <= 0) {
				log_err("Failed to recv icmp packet\n");
				return -1;
			}
		}

		if (network_ping_unpack(num,p) == 0) {
			utils_timer_stop(p->tid);
			return 0;
		}
	}

	if (p->timeout)
		return -1;

	return 0;
}

int network_ping(const char *des, uint32_t count)
{
	int try=0,success=0;
	
	network_ping_t ping_info;
	network_ping_t *p = &ping_info;
	
	memset(p, 0, sizeof(network_ping_t));
	if (!des || socket_get_host_ip(des, p->sock.des.addr) != 0) {
		log_err("Invalid address to ping\n");
		goto out;
	}

	count = count == 0 ? 1 : count;
	log_info("Try to ping %s (%s) %d times\n", des, p->sock.des.addr, count);

	if ((p->sock.fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP))< 0) {
		log_err("Failed to create socket\n");
		goto out;
	}

	socket_set_non_block(p->sock.fd, 1);
	p->pid = getpid();
	success = 0;
	for (try=0; try<count; try++) {
		if (network_ping_send(try,p) < 0) {
			log_err("Failed to send ping packet: icmp_seq=%d\n", try);
		}

		if (network_ping_recv(try,p) == 0) {
			log_debug("%d bytes from %s (%s) icmp_seq=%d time=%f ms\n",
						p->rx_len, des, p->sock.des.addr, try, p->ttime/1000.0);
			success++;
		}
	}

out:
	socket_close(&p->sock);
	log_info("ping %s (%s) %d/%d packet received\n", des, p->sock.des.addr, success, try);
	return success == count ? 0 : -1;
}

int network_eth_info_update(const char *eth_inf)
{
	int sock;
	struct sockaddr_in *p_sin;
	struct ifreq ifr;
	network_svc_state_t *s = &network_state;
	
	if (!eth_inf) {
		log_err("Invalid ethernet interface\n");
		return -1;
	}
	
	memset(&ifr, 0, sizeof(struct ifreq));
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		log_err("Get %s mac address socket creat error\n", eth_inf);
		return -1;
	}
	
	strncpy(ifr.ifr_name, eth_inf, sizeof(ifr.ifr_name) - 1);
	
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
	{
		log_err("Get %s mac address error\n", eth_inf);
		goto failed;
	}
	
	snprintf(s->mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
	(uint8_t)ifr.ifr_hwaddr.sa_data[0],
	(uint8_t)ifr.ifr_hwaddr.sa_data[1],
	(uint8_t)ifr.ifr_hwaddr.sa_data[2],
	(uint8_t)ifr.ifr_hwaddr.sa_data[3],
	(uint8_t)ifr.ifr_hwaddr.sa_data[4],
	(uint8_t)ifr.ifr_hwaddr.sa_data[5]);
	
	if (ioctl(sock, SIOCGIFADDR, &ifr) < 0) {
		log_err("Ioctl error: %s\n", strerror(errno));
		goto failed;
	}
	
	p_sin = (struct sockaddr_in *)&ifr.ifr_addr;
	snprintf(s->ip, 18, "%s", inet_ntoa(p_sin->sin_addr));
	
	close(sock);
	return 0;
failed:
	close(sock);
	return -1;
}

void network_state_report(void *arg, timer_t id)
{
	int err;
	network_svc_state_t *s = arg;
	
	if (s->v == NW_WIFI_CONFIG)
		return;
	
	err = network_ping("www.baidu.com", 2);

	if (!err) {
		s->v = NW_WIFI_CONNECTED;
	}else if (s->v == NW_WIFI_CONNECTED) {
		s->v = NW_WIFI_DISCONNECTED;
	}
	
	err = network_eth_info_update("eth0");
	if (err < 0)
		log_err("Failed to update eth info\n");
	
	log_info("Network state update: %d\n", s->v);
	
	err = lsvc_event_send(NETWORK_EV_REPORT_STATE, s, sizeof(network_svc_state_t),
							LMSG_BUS_CALL, NULL);
	
	return;
}

// intent_handler for vendor
int network_svc_intent_handler(int event, void *data, size_t size)
{
	int ret;
	network_svc_state_t *s = &network_state;
	log_debug("event 0x%08X\n", event);
	
	switch(event) {
		case NW_WIFI_CONFIG:
			s->v = NW_WIFI_CONFIG;
			ret = 0;
		break;
		
		case NW_WIFI_CONNECTING:
			s->v = NW_WIFI_CONNECTING;
			ret = lsvc_event_send(NETWORK_EV_WIFI_CONNECTING, NULL, 0, LMSG_BUS_CALL, NULL);
		break;
		
		case NW_WIFI_CONNECTED:
			s->v = NW_WIFI_CONNECTED;
			ret = lsvc_event_send(NETWORK_EV_WIFI_CONFIG_SUCCESS, NULL, 0, LMSG_BUS_CALL, NULL);
		break;
		
		case NW_WIFI_DISCONNECTED:
			s->v = NW_WIFI_DISCONNECTED;
			ret = lsvc_event_send(NETWORK_EV_WIFI_CONFIG_FAILED, NULL, 0, LMSG_BUS_CALL, NULL);
		break;
	}
	
	if (ret < 0)
		log_err("Failed to process intent\n");
	
	return ret;
}



/*
 * lsvc api implementation
 */

int network_svc_init(void *runtime)
{
	network_svc_state_t *s = &network_state;
	
	memset(s, 0, sizeof(network_svc_state_t));
	s->v = NW_WIFI_DISCONNECTED;

	//s->tid = utils_timer_start(network_state_report,s,1,5,1);

	return 0;
}

int network_svc_exit(void *runtime)
{
	network_svc_state_t *s = &network_state;
	
	if (s->tid) {
		utils_timer_stop(s->tid);
		s->tid = NULL;
	}

	return 0;
}

/*
 * shell command example
 * #ping www.bing.com
 * /usr/local/bin/lsvc network -p www.bing.com -c 4
 */
int network_svc_getopt(int argc, const char *argv[], void **msg)
{
	int opt,err=-1;
	lbus_msg_t *m;
	network_ctl_t *ctl;
	
	*msg = lbus_msg_new(sizeof(network_ctl_t));
	if (!*msg) {
		log_err("Failed to alloc msg buffer\n");
		return -1;
	}

	m = *msg;
	ctl = (void *)m->payload;
	while((opt = getopt(argc, (void *)argv, "vp:c:")) != -1) {
		switch (opt) {
			case 'v':
				m->event = NETWORK_EV_GET_STATE;
				err = 0;
				break;
			case 'p':
				m->event = NETWORK_EV_PING;
				sprintf(ctl->ping_address, "%s", (char *)optarg);
				err = 0;
				break;

			case 'c':
				m->event = NETWORK_EV_PING;
				ctl->count = atoi(optarg);
				err = 0;
				break;

			case '?':
			case '-':
				err = -1;
				break;
		}
	}

	return err;
}

int network_svc_ioctl(void *runtime, void *_msg)
{
	int err = -1;
	lbus_msg_t *msg = _msg;
	network_ctl_t *ctl = (void *)msg->payload;

	switch(msg->event) {
		case NETWORK_EV_GET_STATE:
			err = network_svc_state(msg);
		break;
		
		case NETWORK_EV_PING:
			err = network_ping(ctl->ping_address, ctl->count);
		break;
	}

	if (err < 0)
		log_err("Ioctl failed: ev 0x%08X\n", msg->event);

	return err;
}


lsvc_t network_svc = {
	.ev_base = LSVC_NETWORK_EVENT,
	.name = "network",
	.getopt = network_svc_getopt,
	.init = network_svc_init,
	.exit = network_svc_exit,
	.ioctl = network_svc_ioctl,
};


