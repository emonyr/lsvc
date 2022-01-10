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
	
	//if(setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,
	//					(void *)&val,sizeof(val)) == -1) {
    //    fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
	//	return -1;
    //}
	
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

int socket_bind(socket_info_t *info)
{	
    info->src.sin_family = AF_INET;
    info->src.sin_addr.s_addr = inet_addr(info->addr);
	info->src.sin_port = htons(atoi(info->port));
	
	if (bind(info->fd, (struct sockaddr *)&info->src, sizeof(struct sockaddr)) != 0) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}
	
	return 0;
}

int socket_listen(socket_info_t *info)
{	
	if (listen(info->fd, 1024) != 0) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}
	
	return 0;
}

int socket_connect(socket_info_t *info)
{	
	int err;
	char ip[SOCKET_ADDR_MAX] = {0};

	err = socket_get_host_ip(info->addr, ip);
	if (err) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}

    info->des.sin_family = AF_INET;
    info->des.sin_addr.s_addr = inet_addr(ip);
	info->des.sin_port = htons(atoi(info->port));
	
	if (connect(info->fd, (struct sockaddr *)&info->des, sizeof(struct sockaddr)) != 0) {
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}
	
	return 0;
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

int socket_close(socket_info_t *info)
{
	if (info->fd > 0)
		close(info->fd);
	
	info->fd = -1;
	
	return 0;
}

int socket_udp_recv(socket_info_t *iface, void *data, int size);
int socket_udp_send(socket_info_t *iface, void *data, int size);
int socket_tcp_recv(socket_info_t *iface, void *data, int size);
int socket_tcp_send(socket_info_t *iface, void *data, int size);

int socket_recv(socket_info_t *iface, void *data, int size)
{
	if (!iface || !data || !size)
		return -1;

	switch (iface->type) {
		case TYPE_UDP:
			return socket_udp_recv(iface, data, size);

		case TYPE_TCP:
			return socket_tcp_recv(iface, data, size);
		
		default:
			break;
	}

	return -1;
}

int socket_send(socket_info_t *iface, void *data, int size)
{
	if (!iface || !data || !size)
		return -1;

	switch (iface->type) {
		case TYPE_UDP:
			return socket_udp_send(iface, data, size);

		case TYPE_TCP:
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
	struct addrinfo hints = {0};
	struct addrinfo *result,*rp;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	err = getaddrinfo(NULL, iface->port, &hints, &result);
	if(err != 0){
		fprintf(stderr, "%s failed: %s\n", __func__, gai_strerror(err));
		return -1;
	}

	for (rp=result; rp!=NULL; rp=rp->ai_next) {
		socket_close(iface);
		iface->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(iface->fd < 0) {
			fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
			return -1;
		}
		
		if(socket_set_close_on_exec(iface->fd, 1))
			continue;
		
		if(socket_set_non_block(iface->fd, 1))
			continue;
		
		if(socket_set_reuse(iface->fd, 1))
			continue;

		if(socket_bind(iface))
			continue;

		if(socket_listen(iface) == 0)
			break;
	}

	if (rp == NULL) {
		socket_close(iface);
		err = -1;
	} else {
		iface->type = TYPE_TCP;
		err = 0;
	}

	freeaddrinfo(result);

    return err;
}

int socket_tcp_client_init(socket_info_t *iface)
{
	int err;
	struct addrinfo hints = {0};
	struct addrinfo *result,*rp;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	err = getaddrinfo(iface->addr, iface->port, &hints, &result);
	if(err != 0){
		fprintf(stderr, "%s failed: %s\n", "getaddrinfo", gai_strerror(err));
		return -1;
	}

	for (rp=result; rp!=NULL; rp=rp->ai_next) {
		socket_close(iface);
		iface->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(iface->fd < 0) {
			fprintf(stderr, "%s failed: %s\n", "Create socket", strerror(errno));
			return -1;
		}
		
		if(socket_set_close_on_exec(iface->fd, 1))
			continue;
		
		if(socket_connect(iface) == 0)
			break;
	}

	if (rp == NULL) {
		socket_close(iface);
		err = -1;
	} else {
		iface->type = TYPE_TCP;
		err = 0;
	}

	freeaddrinfo(result);

    return err;
}

int socket_tcp_send(socket_info_t *iface, void *data, int size)
{
	int nbyte=0,sent=0;
	
	while(sent != size){
		nbyte = write(iface->fd, &((unsigned char *)data)[sent], size-sent);
		if(nbyte < 0){
			if(nbyte == EAGAIN)
				continue;
			else
				break;
		}
		sent += nbyte;
	}

	if(nbyte < 0)
		fprintf(stderr, "%s failed: %s:%s nbyte %d fd %d\n", __func__, iface->addr, iface->port, nbyte, iface->fd);

	return nbyte;
}

int socket_tcp_recv(socket_info_t *iface, void *data, int size)
{

	int len=0,nbyte=0;
	do{
		nbyte = read(iface->fd, &((unsigned char *)data)[len], size);
		if(nbyte < 0){
			if(nbyte == EAGAIN){
				continue;
			}else{
				break;
			}
		}
		len += nbyte;
	}while(nbyte != 0 && len < size);

	return len;
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
	struct addrinfo hints = {0};
	struct addrinfo *result,*rp;

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	err = getaddrinfo(NULL, iface->port, &hints, &result);
	if(err != 0){
		fprintf(stderr, "%s failed: %s\n", "getaddrinfo", gai_strerror(err));
		return -1;
	}

	for (rp=result; rp!=NULL; rp=rp->ai_next) {
		socket_close(iface);
		iface->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(iface->fd < 0)
			continue;
		
		if(socket_set_close_on_exec(iface->fd, 1))
			continue;
		
		if(socket_set_non_block(iface->fd, 1))
			continue;
		
		if(socket_set_broadcast(iface->fd, 1))
			continue;
		
		//if(socket_set_reuse(iface->fd, 1))
		//	continue;

		if(socket_bind(iface) == 0)
			break;
	}
	
	if (rp == NULL) {
		socket_close(iface);
		err = -1;
	} else {
		iface->type = TYPE_UDP;
		err = 0;
	}

	freeaddrinfo(result);

    return err;
}

int socket_udp_send(socket_info_t *iface, void *data, int size)
{
	int nbyte;
	struct sockaddr_in address = {0};
		
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(iface->addr);
	address.sin_port = htons(atoi(iface->port));
	
	nbyte = sendto(iface->fd, data, size, MSG_DONTWAIT | MSG_NOSIGNAL, 
						(struct sockaddr*)&address, sizeof(address));
	if(nbyte < 0)
		fprintf(stderr, "%s failed: %s:%s nbyte %d fd %d\n", __func__, iface->addr, iface->port, nbyte, iface->fd);
	
	return nbyte;
}

int socket_udp_recv(socket_info_t *iface, void *data, int size)
{
	return recvfrom(iface->fd, data, size, 0, 
						(struct sockaddr*)&iface->src, &iface->src_len);
}



