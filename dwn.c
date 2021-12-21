#include "dwn.h"

#include <errno.h>
#include <fcntl.h>
#include <libnotify/notify.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <poll.h>
#include <signal.h>

#include "module.h"

#define MAX_POLL_FDS 256

struct poll_sub_t {
    void* private;
    poll_callback_t cb;
};

struct poll_ctl_t {
    struct pollfd pfds[MAX_POLL_FDS];
    struct poll_sub_t subs[MAX_POLL_FDS];
    size_t num_pfds;
};

int poll_ctl_add_sub(struct poll_ctl_t* ctl, const struct pollfd* pfd, poll_callback_t cb,
                     void* private) {
    if (ctl->num_pfds == MAX_POLL_FDS) return -1;

    memcpy(&ctl->pfds[ctl->num_pfds], pfd, sizeof(*pfd));
    ctl->subs[ctl->num_pfds].cb = cb;
    ctl->subs[ctl->num_pfds].private = private;
    ctl->num_pfds += 1;

    return 0;
}

int poll_ctl_advance(struct poll_ctl_t* ctl) {
    int retval;

    retval = poll(ctl->pfds, ctl->num_pfds, -1);
    if (retval == -1) {
        return retval;
    }

    for (size_t i = 0; i < ctl->num_pfds; ++i) {
        if (ctl->subs[i].cb) {
            // TODO(patrick) handle error code.
            ctl->subs[i].cb(ctl->subs[i].private, ctl->pfds + i);
        }
    }

    return retval;
}

void init_modules() {
    for (struct module_t* m = &module_kernel_start; m < &module_kernel_end; ++m) {
        m->init();
    }
    for (struct module_t* m = &module_config_start; m < &module_config_end; ++m) {
        m->init();
    }
}

void destroy_modules() {
    for (struct module_t* m = &module_config_start; m < &module_config_end; ++m) {
        m->destroy();
    }
    for (struct module_t* m = &module_kernel_start; m < &module_kernel_end; ++m) {
        m->destroy();
    }
}

struct poll_ctl_t ctl;

int add_poll_sub(const struct pollfd* pfd, poll_callback_t cb, void* private) {
    return poll_ctl_add_sub(&ctl, pfd, cb, private);
}

int main(int argc, char** argv) {
    int retval;

    ctl.num_pfds = 0;

    init_modules();
    while (1) {
        retval = poll_ctl_advance(&ctl);
        if (retval == -1) {
            if (errno != EINTR) {
                perror("poll");
            }
        }
    }
    destroy_modules();

    return 0;
}
