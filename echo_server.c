#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/socket.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#define PORT 8888

void read_cb(struct bufferevent *bev, void *ctx) {
    /*
    #define MAXLINE 1024
    char line[MAXLINE+1];
    int n;
    evutil_socket_t fd = bufferevent_getfd(bev);
    while((n = bufferevent_read(bev, line, MAXLINE)) > 0) {
        line[n] = '\0';
        printf("fd = %u, read line: %s\n", fd, line);
        bufferevent_write(bev, line, n);
    }
    */
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_add_buffer(output, input);  // input --> output
}

void write_cb(struct bufferevent *bev, void *ctx) {}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if(events & BEV_EVENT_ERROR) {
        perror("Error from bufferevent");
    }
    if(events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

void do_accept(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ctx) {
    printf("Accept: fd = %u\n", fd);
    
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
}

void signal_cb(evutil_socket_t fd, short events, void *arg) {
    struct event_base *base = (struct event_base*)arg;
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in 2 seconds...\n");
    event_base_loopexit(base, &delay);
}

int main(int argc, char *argv[])
{
    struct event_base *base;
    struct event *signal_event;
    struct evconnlistener *listener;
    struct sockaddr_in addr;

    base = event_base_new();
    assert(base != NULL);

    signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base); 
    evsignal_add(signal_event, NULL);
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    listener = evconnlistener_new_bind(base, do_accept, NULL, 
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, 
            (struct sockaddr*)&addr, sizeof(addr));
    assert(listener != NULL);

    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);
    exit(EXIT_SUCCESS);
}
