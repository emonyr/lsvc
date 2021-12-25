#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket.h"

#define VAL_ON 1
#define VAL_OFF 0


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
	
	if(fcntl(fd,F_SETFL,flags) == -1){
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
	
	if(fcntl(fd,F_SETFL,flags) == -1){
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
						(void *)&val,sizeof(val)) == -1){
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	//if(setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,
	//					(void *)&val,sizeof(val)) == -1){
    //    fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
	//	return -1;
    //}
	
	return 0;
}

int socket_set_broadcast(int fd, int val)
{
	if(fd < 0 || val < 0)
		return -1;
	
	if(setsockopt(fd,SOL_SOCKET,SO_BROADCAST,
						(void *)&val,sizeof(val)) == -1){
        fprintf(stderr, "%s failed: %s\n" , __func__, strerror(errno));
		return -1;
    }
	
	return 0;
}

int socket_bind(socket_info_t *info)
{
	struct sockaddr_in address = {0};
	
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(info->addr);
	address.sin_port = htons(info->port);
	
	if(bind(info->fd,&address,sizeof(struct sockaddr)) != 0){
		fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
		return -1;
	}
	
	return 0;
}

int socket_close(socket_info_t *info)
{
	if(info->fd > 0)
		close(info->fd);
	
	info->fd = -1;
	
	return 0;
}


/*
 * tcp socket api
 */






/*
 * udp socket api
 */
int socket_udp_rcvbuf_get(int fd)
{
	int rx_size = -1;
    socklen_t len = sizeof(rx_size);
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rx_size, &len) < 0){
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
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rx_size, &len) < 0){
        fprintf(stderr, "%s failed: %s\n", __func__, strerror(errno));
        return -1;
    }else{
		printf("%s: %d\n", __func__, rx_size);
	}
	
	return 0;
}

int socket_udp_init(socket_info_t *iface)
{
	if(!iface || iface->fd > 0)
		return -1;
		
    iface->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(iface->fd < 0){
        printf("socket_udp_init failed: %s\n",strerror(errno));
        return -1;
    }
	
	if(socket_set_close_on_exec(iface->fd, VAL_ON) < 0)
		goto failed;
	
	if(socket_set_non_block(iface->fd, VAL_ON) < 0)
		goto failed;
	
	if(socket_set_broadcast(iface->fd, VAL_ON) < 0)
		goto failed;
	
	//if(socket_set_reuse(iface->fd, VAL_ON) < 0)
	//	goto failed;
	
    return iface->fd;
	
failed:
	close(iface->fd);
	iface->fd = -1;
	return iface->fd;
}

int socket_udp_send(socket_info_t *iface, void *data, int size)
{
	int nbyte;
		
	iface->des.sin_family = AF_INET;
	iface->des.sin_addr.s_addr = inet_addr(iface->addr);
	iface->des.sin_port = htons(iface->port);
	
	nbyte = sendto(iface->fd, data, size, MSG_DONTWAIT | MSG_NOSIGNAL, 
						(struct sockaddr*)&iface->des, sizeof(iface->des));
	if(nbyte < 0)
		fprintf(stderr, "socket_udp_send failed: %s:%d nbyte %d fd %d\n", iface->addr, iface->port, nbyte, iface->fd);
	
	return nbyte;
}

int socket_udp_recv(socket_info_t *iface, void *data, int size)
{
	return recvfrom(iface->fd, data, size, 0, 
						(struct sockaddr*)&iface->src, &iface->src_len);
}



