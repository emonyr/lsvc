#ifndef __SOCKET_TOOL_H__
#define __SOCKET_TOOL_H__

#include <netinet/in.h>

typedef struct {
	int fd;
	char addr[32];
	unsigned int port;
	
	struct sockaddr_in src;
	unsigned int src_len;

	struct sockaddr_in des;
	unsigned int des_len;

}__attribute__ ((packed))socket_info_t;


extern int socket_set_close_on_exec(int fd, int val);
extern int socket_set_non_block(int fd, int val);
extern int socket_set_reuse(int fd, int val);
extern int socket_set_broadcast(int fd, int val);
extern int socket_bind(socket_info_t *info);
extern int socket_close(socket_info_t *info);

extern int socket_udp_rcvbuf_get(int fd);
extern int socket_udp_rcvbuf_set(int fd, int val);
extern int socket_udp_init(socket_info_t *udp);
extern int socket_udp_send(socket_info_t *udp, void *data, int size);
extern int socket_udp_recv(socket_info_t *udp, void *data, int size);



#endif	/* __SOCKET_TOOL_H__ */