#ifndef __SOCKET_TOOL_H__
#define __SOCKET_TOOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SOCKET_ADDR_MAX (32)
#define SOCKET_PORT_MAX (24)

typedef enum {
	TYPE_UDP,
	TYPE_TCP_SERVER,
	TYPE_TCP_CLIENT,
}socket_type_t;

typedef struct {
	char addr[SOCKET_ADDR_MAX];
	char port[SOCKET_PORT_MAX];
}socket_addr_t;

typedef struct {
	int fd;
	int type;
	socket_addr_t src;
	socket_addr_t des;
	void *posix_addr_info;
	int posix_addr_len;
}__attribute__ ((packed))socket_info_t;


extern int socket_set_close_on_exec(int fd, int val);
extern int socket_set_non_block(int fd, int val);
extern int socket_set_reuse(int fd, int val);
extern int socket_set_broadcast(int fd, int val);
extern int socket_bind(socket_info_t *iface);
extern int socket_listen(socket_info_t *iface);
extern int socket_accept(socket_info_t *server, socket_info_t *client);
extern int socket_connect(socket_info_t *iface);
extern int socket_wait(socket_info_t *iface, int second, unsigned long usecond);
extern int socket_get_host_ip(const char *des, char *output);
extern int socket_parse_uri(const char *uri, socket_info_t *iface);
extern int socket_open(const char *uri, socket_info_t *iface);
extern int socket_close(socket_info_t *iface);
extern int socket_recv(socket_info_t *iface, void *data, size_t size);
extern int socket_send(socket_info_t *iface, void *data, size_t size);

extern int socket_tcp_server_init(socket_info_t *iface);
extern int socket_tcp_client_init(socket_info_t *iface);

extern int socket_udp_rcvbuf_get(int fd);
extern int socket_udp_rcvbuf_set(int fd, int val);
extern int socket_udp_init(socket_info_t *udp);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif	/* __SOCKET_TOOL_H__ */
