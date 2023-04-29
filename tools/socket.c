#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"


/*
 * common socket api
 */

int socket_set_close_on_exec(int fd, int val)
{
	int flags = 0;
	
	if(fd < 0 || val < 0)
		return -1;
	
	flags = fcntl(fd, F_GETFL);
	if(flags < 0)
		return -1;
	
	if(val)
		flags |= O_CLOEXEC;
	else
		flags &= ~O_CLOEXEC;
	
	if(fcntl(fd,F_SETFL,flags) == -1) {
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	return 0;
}

int socket_set_non_block(int fd, int val)
{
	int flags = 0;
	
	if(fd < 0 || val < 0)
		return -1;
	
	flags = fcntl(fd, F_GETFL);
	if(flags < 0)
		return -1;
	
	if(val)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	
	if(fcntl(fd,F_SETFL,flags) == -1) {
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	return 0;
}

int socket_set_reuse(int fd, int val)
{
	if(fd < 0 || val < 0)
		return -1;
	
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,
						(void *)&val,sizeof(val)) == -1) {
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,
						(void *)&val,sizeof(val)) == -1) {
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	return 0;
}

int socket_set_broadcast(int fd, int val)
{
	if (fd < 0 || val < 0)
		return -1;
	
	if (setsockopt(fd,SOL_SOCKET,SO_BROADCAST,
						(void *)&val,sizeof(val)) == -1) {
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	return 0;
}

int socket_bind(socket_info_t *iface)
{
	struct sockaddr_in address;

	if (iface->posix_addr_info && iface->posix_addr_len) {
		// address info from caller
		if (bind(iface->fd, (struct sockaddr *)iface->posix_addr_info, iface->posix_addr_len) != 0) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			return -1;
		}
	} else {
		// dafault to ipv4 address info
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr(iface->des.addr);
		address.sin_port = htons(atoi(iface->des.port));
		
		if (bind(iface->fd, (struct sockaddr *)&address, sizeof(struct sockaddr)) != 0) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			return -1;
		}
	}
	
	
	return 0;
}

int socket_listen(socket_info_t *iface)
{	
	if (listen(iface->fd, 1024) != 0) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}
	
	return 0;
}

int socket_accept(socket_info_t *server, socket_info_t *client)
{
	int len=sizeof(struct sockaddr_in);
	struct sockaddr_in address;

	client->fd = accept(server->fd, (struct sockaddr *)&address, &len);
	if (client->fd == -1) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}

	if (socket_set_non_block(client->fd, 1)) {
		socket_close(client);
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}

	client->type = TYPE_TCP_CLIENT;
	sprintf(client->des.addr, "%s", inet_ntoa(address.sin_addr));
	sprintf(client->des.port, "%d", ntohs(address.sin_port));
	
	return 0;
}

int socket_connect(socket_info_t *iface)
{	
	char ip[SOCKET_ADDR_MAX]={0};
	struct sockaddr_in address;

	if (iface->posix_addr_info && iface->posix_addr_len) {
		// address info from caller
		if (connect(iface->fd, (struct sockaddr *)iface->posix_addr_info, iface->posix_addr_len) != 0) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			return -1;
		}
	} else {
		// dafault to ipv4 address info
		if (socket_get_host_ip(iface->des.addr, ip)) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			return -1;
		}

		memset(&address, 0, sizeof(struct sockaddr_in));
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr(ip);
		address.sin_port = htons(atoi(iface->des.port));
		
		if (connect(iface->fd, (struct sockaddr *)&address, sizeof(struct sockaddr)) != 0) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			return -1;
		}
	}
	
	return 0;
}

int socket_wait(socket_info_t *iface, int second, unsigned long usecond)
{
	fd_set readfds;
	struct timeval tv;

	FD_ZERO(&readfds);
	FD_SET(iface->fd, &readfds);
	
	tv.tv_sec = second;
	tv.tv_usec = usecond;

	return select(FD_SETSIZE,&readfds,NULL,NULL,&tv);
}

int socket_get_host_ip(const char *des, char *output)
{
	struct in_addr inaddr;
	struct hostent * host;

	host = gethostbyname(des);
	if (!host) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}else{
		memcpy(&inaddr, host->h_addr_list[0], host->h_length);
		sprintf(output, "%s", inet_ntoa(inaddr));
	}

	return 0;
}

int socket_get_socket_name(socket_info_t *iface)
{
	if (iface->fd < 0)
		return -1;

	struct sockaddr_in address;
	socklen_t len = sizeof(address);

	if (getsockname(iface->fd, (struct sockaddr*)&address, &len) != 0) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}

	sprintf(iface->src.addr, "%s", inet_ntoa(address.sin_addr));
	sprintf(iface->src.port, "%d", ntohs(address.sin_port));

	return 0;
}

int socket_parse_uri(const char *uri, socket_info_t *iface)
{
	int len=0;
	char *c,*p;
	
	c = uri;
	len = strlen(uri);
	if (strncmp(c, "udp://", 6) == 0) {
		iface->type = TYPE_UDP;
	} else if (strncmp(c, "tcp://", 6) == 0) {
		iface->type = c[6] == ':' ? TYPE_TCP_SERVER : TYPE_TCP_CLIENT;
	} else {
		return -1;
	}

	c += 6;
	len -= 5;	// including '\0'
	p = iface->des.addr;
	for (int i=0,j=0; i<len; i++,j++) {
		if (c[i] == ':') {
			p[j] = '\0';
			p = iface->des.port;
			j=0;
			i++;
		}
		
		p[j] = c[i];
	}

	return (p == iface->des.port) ? 0 : -1;
}

char _usage[] = "\nUsage: \n									\
				\t UDP fix port: udp://127.0.0.1:18080\n		\
				\t UDP random port: udp://127.0.0.1:0\n			\
				\t TCP client: tcp://127.0.0.1:8080\n			\
				\t TCP server: tcp://:8080\n					\
				\n";

int socket_open(const char *uri, socket_info_t *iface)
{
	int err=-1;

	if (!iface || !uri) {
		fprintf(stderr, "Invalid param: iface %p, uri %p\n", iface, uri);
		return -1;
	}

	memset(iface, 0, sizeof(socket_info_t));
	err = socket_parse_uri(uri, iface);
	if (err) 
		goto failed;

	if (iface->type == TYPE_UDP) {
		err = socket_udp_init(iface);
	} else if (iface->type == TYPE_TCP_CLIENT) {
		err = socket_tcp_client_init(iface);
	} else if (iface->type == TYPE_TCP_SERVER) {
		err = socket_tcp_server_init(iface);
	} else {
		goto failed;
	}

	return err;

failed:
	fprintf(stderr, "%s", _usage);
	return -1;
}

int socket_close(socket_info_t *iface)
{
	if (iface->fd > 0)
		close(iface->fd);
	
	iface->fd = -1;
	iface->posix_addr_info = NULL;
	iface->posix_addr_len = 0;
	
	return 0;
}

int socket_udp_recv(socket_info_t *iface, void *data, size_t size);
int socket_udp_send(socket_info_t *iface, void *data, size_t size);
int socket_tcp_recv(socket_info_t *iface, void *data, size_t size);
int socket_tcp_send(socket_info_t *iface, void *data, size_t size);

int socket_recv(socket_info_t *iface, void *data, size_t size)
{
	if (!iface || !data || !size)
		return -1;

	switch (iface->type) {
		case TYPE_UDP:
			return socket_udp_recv(iface, data, size);

		case TYPE_TCP_CLIENT:
			return socket_tcp_recv(iface, data, size);
		
		case TYPE_TCP_SERVER:
			return socket_accept(iface, (socket_info_t *)data);
	}

	return -1;
}

int socket_send(socket_info_t *iface, void *data, size_t size)
{
	if (!iface || !data || !size)
		return -1;

	switch (iface->type) {
		case TYPE_UDP:
			return socket_udp_send(iface, data, size);

		case TYPE_TCP_CLIENT:
		case TYPE_TCP_SERVER:
			return socket_tcp_send(iface, data, size);
		
		default:
			break;
	}

	return -1;
}

/*
 * tcp socket api
 */
int socket_tcp_server_init(socket_info_t *iface)
{
	int err;
	struct addrinfo hints;
	struct addrinfo *result,*rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	err = getaddrinfo(NULL, iface->des.port, &hints, &result);
	if(err != 0){
		fprintf(stderr, "%s failed: %s\n", __func__, gai_strerror(err));
		return -1;
	}

	for (rp=result; rp!=NULL; rp=rp->ai_next) {
		socket_close(iface);
		iface->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(iface->fd < 0) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			continue;
		}
		
		iface->posix_addr_info = rp->ai_addr;
		iface->posix_addr_len = rp->ai_addrlen;

		if(socket_set_close_on_exec(iface->fd, 1))
			continue;

		if(socket_set_reuse(iface->fd, 1))
			continue;

		if(socket_bind(iface))
			continue;

		if(socket_listen(iface))
			continue;

		if(socket_set_non_block(iface->fd, 1))
			continue;

		if (socket_get_socket_name(iface) == 0)
			break;
	}

	if (rp == NULL) {
		socket_close(iface);
		err = -1;
	} else {
		err = 0;
	}

	freeaddrinfo(result);

    return err;
}

int socket_tcp_client_init(socket_info_t *iface)
{
	int err;
	struct addrinfo hints;
	struct addrinfo *result,*rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	err = getaddrinfo(iface->des.addr, iface->des.port, &hints, &result);
	if(err != 0){
		fprintf(stderr, "%s failed: %s\n", "getaddrinfo", gai_strerror(err));
		return -1;
	}

	for (rp=result; rp!=NULL; rp=rp->ai_next) {
		socket_close(iface);
		iface->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(iface->fd < 0) {
			fprintf(stderr, "%s failed: %s\n", "Create socket", strerror(errno));
			continue;
		}

		iface->posix_addr_info = rp->ai_addr;
		iface->posix_addr_len = rp->ai_addrlen;
		
		if(socket_set_close_on_exec(iface->fd, 1))
			continue;

		if(socket_connect(iface))
			continue;

		if(socket_set_non_block(iface->fd, 1))
			continue;

		if (socket_get_socket_name(iface) == 0)
			break;
	}

	if (rp == NULL) {
		socket_close(iface);
		err = -1;
	} else {
		err = 0;
	}

	freeaddrinfo(result);

    return err;
}

int socket_tcp_send(socket_info_t *iface, void *data, size_t size)
{
	int nbyte=0,sent=0;
	
	while(sent != size){
		nbyte = write(iface->fd, &((uint8_t *)data)[sent], size-sent);
		if(nbyte < 0){
			if(errno == EAGAIN)
				continue;
			else
				break;
		}
		sent += nbyte;
	}

	if(nbyte < 0)
		fprintf(stderr, "%s failed: %s:%s nbyte %d fd %d\n", __func__, iface->des.addr, iface->des.port, nbyte, iface->fd);

	return nbyte;
}

int socket_tcp_recv(socket_info_t *iface, void *data, size_t size)
{
	return read(iface->fd, data, size);
}

/*
 * udp socket api
 */
int socket_udp_rcvbuf_get(int fd)
{
	int rx_size = -1;
    socklen_t len = sizeof(rx_size);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rx_size, &len) < 0) {
        fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
        return -1;
    }else{
		printf("%s: %d\n", __func__, rx_size);
	}
	
	return rx_size;
}

int socket_udp_rcvbuf_set(int fd, int val)
{
	int rx_size = val;
    socklen_t len = sizeof(rx_size);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rx_size, len) < 0) {
        fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
        return -1;
    }else{
		printf("%s: %d\n", __func__, rx_size);
	}
	
	return 0;
}

int socket_udp_init(socket_info_t *iface)
{
	int err;
	struct addrinfo hints;
	struct addrinfo *result,*rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	err = getaddrinfo(iface->des.addr, iface->des.port, &hints, &result);
	if(err != 0){
		fprintf(stderr, "%s failed: %s\n", "getaddrinfo", gai_strerror(err));
		return -1;
	}

	for (rp=result; rp!=NULL; rp=rp->ai_next) {
		socket_close(iface);
		iface->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(iface->fd < 0) {
			fprintf(stderr, "%s failed: %s\n", "Create socket", strerror(errno));
			continue;
		}

		iface->posix_addr_info = rp->ai_addr;
		iface->posix_addr_len = rp->ai_addrlen;

		if(socket_set_close_on_exec(iface->fd, 1))
			continue;

		if(socket_set_non_block(iface->fd, 1))
			continue;

		if(socket_set_broadcast(iface->fd, 1))
			continue;

		//if(socket_set_reuse(iface->fd, 1))
		//	continue;

		if(socket_bind(iface))
			continue;

		if (socket_get_socket_name(iface) == 0)
			break;
	}
	
	if (rp == NULL) {
		socket_close(iface);
		err = -1;
	} else {
		err = 0;
	}

	freeaddrinfo(result);

    return err;
}

int socket_udp_send(socket_info_t *iface, void *data, size_t size)
{
	int nbyte;
	struct sockaddr_in address;

	memset(&address, 0, sizeof(struct sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(iface->des.addr);
	address.sin_port = htons(atoi(iface->des.port));

	nbyte = sendto(iface->fd, data, size, MSG_DONTWAIT | MSG_NOSIGNAL, 
						(struct sockaddr*)&address, sizeof(address));
	if(nbyte < 0)
		fprintf(stderr, "%s failed: %s address %s:%s nbyte %d fd %d\n", __func__, strerror(errno), iface->des.addr, iface->des.port, nbyte, iface->fd);

	return nbyte;
}

int socket_udp_recv(socket_info_t *iface, void *data, size_t size)
{
	int nbyte=0,len=sizeof(struct sockaddr_in);
	struct sockaddr_in address;

	memset(&address, 0, sizeof(struct sockaddr_in));
	nbyte = recvfrom(iface->fd, data, size, 0, 
						(struct sockaddr*)&address, &len);

	if (nbyte < 0)
		fprintf(stderr, "%s failed: %s address %s:%s nbyte %d fd %d\n", __func__, strerror(errno), iface->src.addr, iface->src.port, nbyte, iface->fd);

	sprintf(iface->src.addr, "%s", inet_ntoa(address.sin_addr));
	sprintf(iface->src.port, "%d", ntohs(address.sin_port));

	return nbyte;
}



