#include <signal.h>

#include <event2/util.h>
#include <event2/event.h>

void signal_cb(evutil_socket_t fd, short event, void *arg) {
    struct event_base *base = (struct event_base*)arg;
    struct timeval delay = {2, 0};
    printf("Caught an interrupt signal; exiting cleanly in two seconds...\n");
    event_base_loopexit(base, &delay);
}

void timeout_cb(evutil_socket_t fd, short event, void *arg) {
    struct event_base *base = (struct event_base*)arg;
    printf("Timeout\n");
    event_base_loopexit(base, NULL);
}

int main()
{
    struct event_base *base = event_base_new();
    struct event *signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base);
    evsignal_add(signal_event, NULL);

    struct timeval tv = {10, 0};
    struct event *timeout_event = evtimer_new(base, timeout_cb, NULL);
    evtimer_add(timeout_event, &tv);

    event_base_dispatch(base);

    event_free(timeout_event);
    event_free(signal_event);
    event_base_free(base);
}
