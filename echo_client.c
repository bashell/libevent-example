#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <event2/event.h>
#include <event2/bufferevent.h>

#define PORT 8888

void read_cb(struct bufferevent *bev, void *ctx) {
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    int size = evbuffer_get_length(input);
    evbuffer_write(input, 1);
    bufferevent_write(bev, "hello world", 12);
}

void write_cb(struct bufferevent *bev, void *ctx) {}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if(events & BEV_EVENT_CONNECTED) {
        printf("Connected\n");
    } else if(events & BEV_EVENT_ERROR) {
        printf("error\n");
    }
    bufferevent_free(bev);
}

void timeout_cb(evutil_socket_t fd, short events, void *arg) {
    struct event_base *base = (struct event_base*)arg;
    printf("Timeout\n");
    event_base_loopexit(base, NULL);
}

int main(int argc, char *argv[])
{
    struct event_base *base;
    struct event *ev_timeout;
    struct bufferevent *bev;
    struct timeval timeout;
    struct sockaddr_in addr;

    base = event_base_new();
    assert(base != NULL);

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    ev_timeout = evtimer_new(base, timeout_cb, (void*)base);
    evtimer_add(ev_timeout, &timeout);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
  
    bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
    if(bufferevent_socket_connect(bev, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "cannot connect\n");
        bufferevent_free(bev);
        exit(EXIT_FAILURE);
    }

    event_base_dispatch(base);

    event_free(ev_timeout);
    event_base_free(base);
    exit(EXIT_SUCCESS);
}
