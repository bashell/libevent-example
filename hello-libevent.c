#include <event2/event.h>
#include <signal.h>

void signal_cb(int fd, short event, void *arg) {
    struct event_base *base = (struct event_base*)arg;
    struct timeval delay;
    delay.tv_sec = 2;
    delay.tv_usec = 0;
    printf("Hello Libevent! We caught an interrupt signal; exiting cleanly in two seconds...\n");
    event_base_loopexit(base, &delay);  // 2秒后停止循环
}

void timeout_cb(int fd, short event, void *arg) {
    printf("timeout\n");
}

int main()
{
    struct event_base *base = event_base_new();
    struct event *signal_event = evsignal_new(base, SIGINT, signal_cb, base);
    event_add(signal_event, NULL);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    struct event *timeout_event = evtimer_new(base, timeout_cb, NULL);
    event_add(timeout_event, &tv);

    event_base_dispatch(base);  // loop

    event_free(timeout_event);
    event_free(signal_event);
    event_base_free(base);
}
