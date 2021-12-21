#include "file_watcher.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "dwn.h"
#include "module.h"

#define MAX_WATCHER 256

struct file_watcher_sub_t {
    file_watcher_callback_t cb;
    void* private;
};

struct file_watcher_t {
    int nfd;
    struct file_watcher_sub_t subs[MAX_WATCHER];
};

struct file_watcher_t file_watcher;

static void file_watcher_setup(struct file_watcher_t* w) {
    w->nfd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (w->nfd == -1) {
        perror("inotify_init1");
        exit(EXIT_FAILURE);
    }
}

static int file_watcher_add(struct file_watcher_t* w, const char* filename,
                            file_watcher_callback_t cb, void* private) {
    int wd = inotify_add_watch(w->nfd, filename, IN_MODIFY);
    if (wd == -1) return -1;

    w->subs[wd].cb = cb;
    w->subs[wd].private = private;
    return 0;
}

static int file_watcher_event_handler(void* private, struct pollfd* pfd) {
    char buf[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    int retval;
    struct inotify_event* event;
    struct file_watcher_t* watcher = private;

    if (!(pfd->revents & POLLIN)) return 0;

    while (1) {
        retval = read(watcher->nfd, buf, sizeof(buf));
        if (retval == -1) {
            if (errno == EINTR) continue;
            if (errno != EAGAIN) return retval;
        }

        if (retval <= 0) break;

        for (char* ptr = buf; ptr < buf + retval;
             ptr += sizeof(struct inotify_event) + event->len) {
            event = (struct inotify_event*)ptr;
            if (watcher->subs[event->wd].cb) {
                // TODO(patrick) handle event code.
                watcher->subs[event->wd].cb(watcher->subs[event->wd].private);
            }
        }
    }

    return 0;
}

static void init_file_watcher() {
    struct pollfd pfd;

    file_watcher_setup(&file_watcher);

    pfd.fd = file_watcher.nfd;
    pfd.events = POLLIN;
    if (add_poll_sub(&pfd, file_watcher_event_handler, &file_watcher) == -1) {
        fprintf(stderr, "fail to add file watcher event handler\n");
        exit(EXIT_FAILURE);
    }
}

static void destroy_file_watcher() {}

int add_file_watcher(const char* filename, file_watcher_callback_t cb, void* private) {
    return file_watcher_add(&file_watcher, filename, cb, private);
}

DEFINE_KERNEL_MODULE(file_watcher, init_file_watcher, destroy_file_watcher);
