#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

#define PORT 8888
#define MAXLINE 1024


void read_cb(struct bufferevent *bev, void *ctx) {
    char line[MAXLINE+1];
    int n;
    evutil_socket_t fd = bufferevent_getfd(bev);
    while((n = bufferevent_read(bev, line, MAXLINE)) > 0) {
        line[n] = '\0';
        printf("fd = %u, read line: %s\n", fd, line);
        bufferevent_write(bev, line, n);
    }
}

void write_cb(struct bufferevent *bev, void *ctx) {

}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    evutil_socket_t fd = bufferevent_getfd(bev);
    printf("fd = %u\n", fd);
    if(events & BEV_EVENT_TIMEOUT) {
        printf("Timeout\n");
    } else if(events & BEV_EVENT_EOF) {
        printf("connection closed\n");
    } else if(events & BEV_EVENT_ERROR) {
        printf("error\n");
    }
    bufferevent_free(bev);
}

void do_accept(evutil_socket_t listenfd, short event, void *arg) {
    struct sockaddr_in addr;
    socklen_t len;

    memset(&addr, 0, sizeof(addr));
    evutil_socket_t connfd = accept(listenfd, (struct sockaddr*)&addr, &len);
    if(connfd == -1) {
        perror("accept");
        return;
    }
    printf("Accept: connfd = %u\n", connfd);

    struct event_base *base = (struct event_base*)arg;
    struct bufferevent *bev = bufferevent_socket_new(base, connfd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, arg);
    bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
}

int main(int argc, char *argv[])
{
    evutil_socket_t listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    evutil_make_listen_socket_reuseable(listenfd);  // reuseable
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if(listen(listenfd, SOMAXCONN) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    evutil_make_socket_nonblocking(listenfd);  // set nonblocking

    struct event_base *base = event_base_new();
    assert(base != NULL);
    struct event *listen_event = event_new(base, listenfd, EV_READ|EV_PERSIST, do_accept, (void*)base);  // 读持久事件
    event_add(listen_event, NULL);

    event_base_dispatch(base);

    close(listenfd);
    exit(EXIT_SUCCESS);
}
