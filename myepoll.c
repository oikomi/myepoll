/* A simple http server .
   Copyright (C) 2013 Hong Miao <miaohonghit@gmail.com> */

#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h> 
#include <errno.h> 
#include <getopt.h> 

#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <sys/epoll.h>
#include <pthread.h>

 
#define MAX_EVENTS 500
 
#define PORT 9527
 
FILE *g_logger;
 
//设置socket连接为非阻塞模式
 
void setnonblocking(int sockfd) {
 
    int opts;
 
    opts = fcntl(sockfd, F_GETFL);
 
    if(opts < 0) {
 
        perror("fcntl(F_GETFL)\n");
 
        exit(1);
 
    }
 
    opts = (opts | O_NONBLOCK);
 
    if(fcntl(sockfd, F_SETFL, opts) < 0) {
 
        perror("fcntl(F_SETFL)\n");
 
        exit(1);
 
    }
 
}

static void usage()
{
    printf("usage:  myepoll -p <port> \n");
}
 
 
static void httpstub_log(const char *fmt, ...)
{
        //if(0 == g_quiet){
                char msg[4096];
                char buf[64];
                time_t now = time(NULL);
                va_list ap;
                va_start(ap, fmt);
                vsnprintf(msg, sizeof(msg), fmt, ap);
                va_end(ap);
                strftime(buf, sizeof(buf), "%d %b %H:%M:%S", localtime(&now));
                fprintf(g_logger, "[%d] %s %s\n", (int)getpid(), buf, msg);
                fflush(g_logger);
        //}
}
 
 
int main(int argc, char **argv)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int addrlen, listenfd, conn_sock, nfds, epfd, fd, i, nread, n, opt;;
    struct sockaddr_in local, remote;
    char buf[BUFSIZ];
	int port_listening;
	
    if (argc == 1) {
        usage();
        return 1;
    }
	
	while ((opt = getopt(argc, argv, "p:h")) != -1) {
        switch (opt) {
		case 'p':
			port_listening = atoi(optarg);
			if (port_listening == 0) {
				printf(">> invalid port : %s\n", optarg);
				exit(1);
			}
			break;

		case 'h':
			usage();
			return 1;
        }
    }

    g_logger = fopen("stub.log", "a");
    if (g_logger ==NULL) {
		perror("create log file stub.log failed.");
        exit(1);
    }
 

    //创建listen socket
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("sockfd\n");
        exit(1);
    }
	
    setnonblocking(listenfd);
 
    bzero(&local, sizeof(local));
 
    local.sin_family = AF_INET;
 
    local.sin_addr.s_addr = htonl(INADDR_ANY);;
 
    local.sin_port = htons(PORT);
 
    if( bind(listenfd, (struct sockaddr *) &local, sizeof(local)) < 0) {
        perror("bind\n");
        exit(1);
    }
 
    listen(listenfd, 20);

    epfd = epoll_create(MAX_EVENTS);
 
    if (epfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
 
    ev.events = EPOLLIN;
 
    ev.data.fd = listenfd;
 
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }
 
    for (;;) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_pwait");
            exit(EXIT_FAILURE);
        }
 
        for (i = 0; i < nfds; ++i) {
            fd = events[i].data.fd;
            if (fd == listenfd) {
				printf("a new connection... \n");
				httpstub_log("a new connection\n");
                while ((conn_sock = accept(listenfd,(struct sockaddr *) &remote, 
                                (socklen_t *)&addrlen)) > 0) {
                    setnonblocking(conn_sock);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = conn_sock; 
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock,
                                &ev) == -1) {
                        perror("epoll_ctl: add");
                        exit(EXIT_FAILURE);
                    }
                }
                if (conn_sock == -1) {
                    if (errno != EAGAIN && errno != ECONNABORTED 
                            && errno != EPROTO && errno != EINTR) 
                        perror("accept");
                }
                continue;
            }  
 
            if (events[i].events & EPOLLIN) {
			    printf("EPOLLIN...\n");
                n = 0;
                while ((nread = read(fd, buf + n, BUFSIZ-1)) > 0) {
                    n += nread;
                }
                if (nread == -1 && errno != EAGAIN) {
                    perror("read error");
                }
 
                ev.data.fd = fd;
                ev.events = events[i].events | EPOLLOUT;
 
                if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                    perror("epoll_ctl: mod");
                }
            }
 
            if (events[i].events & EPOLLOUT) {
				printf("EPOLLOUT...\n");
                sprintf(buf, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nHello World", 11);
                int nwrite, data_size = strlen(buf);
                n = data_size;
                while (n > 0) {
                    nwrite = write(fd, buf + data_size - n, n);
                    if (nwrite < n) {
                        if (nwrite == -1 && errno != EAGAIN) { 
                            perror("write error");
                        }
                        break;
                    }
                    n -= nwrite;
                }
                close(fd);
            }
        } 
    } 
    return 0; 
} 
